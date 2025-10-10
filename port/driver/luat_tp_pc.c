#ifdef LUAT_USE_GUI
#include "luat_base.h"
#include "luat_tp.h"
#include "luat_mem.h"
#include "luat_mcu.h"

#include "uv.h"

#include "SDL2/SDL.h"

#define LUAT_LOG_TAG "tp_pc"
#include "luat_log.h"

// 前置声明
static int luat_tp_pc_init(luat_tp_config_t* conf);
static int luat_tp_pc_read(luat_tp_config_t* conf, uint8_t* data);
static void luat_tp_pc_read_done(luat_tp_config_t* conf);
static void luat_tp_pc_deinit(luat_tp_config_t* conf);

luat_tp_opts_t tp_config_pc = {
    .name = "pc",
    .init = luat_tp_pc_init,
    .read = luat_tp_pc_read,
    .read_done = luat_tp_pc_read_done,
    .deinit = luat_tp_pc_deinit,
};

typedef struct tp_pc_state
{
    uv_mutex_t lock;
    uint8_t has_event;
    luat_tp_data_t ev; // single-point pending event (保持兼容性)
    uint16_t last_valid_x; // 最后一次有效X坐标
    uint16_t last_valid_y; // 最后一次有效Y坐标
    uint8_t data_consumed; // 数据是否已被消费标志

    // 队列机制 - 新增
    uint8_t event_queue_size; // 事件队列大小
    luat_tp_data_t event_queue[8]; // 事件队列，最多保存8个事件
    uint8_t queue_head; // 队列头
    uint8_t queue_tail; // 队列尾
    uint8_t queue_enabled; // 是否启用队列机制
} tp_pc_state_t;

static tp_pc_state_t s_tp_state;

// 队列管理函数 - 兼容原有框架
static int enqueue_event(luat_tp_data_t* event) {
    if (!event || !s_tp_state.queue_enabled) return -1;

    uv_mutex_lock(&s_tp_state.lock);

    // 计算下一个尾位置
    uint8_t next_tail = (s_tp_state.queue_tail + 1) % s_tp_state.event_queue_size;

    if (next_tail == s_tp_state.queue_head) {
        // 队列已满，丢弃最旧的事件（头部）
        s_tp_state.queue_head = (s_tp_state.queue_head + 1) % s_tp_state.event_queue_size;
        LLOGW("Event queue full, dropping oldest event");
    }

    // 将事件添加到队列尾部
    s_tp_state.event_queue[s_tp_state.queue_tail] = *event;
    s_tp_state.queue_tail = next_tail;

    uv_mutex_unlock(&s_tp_state.lock);

    return 0;
}

static int dequeue_event(luat_tp_data_t* event) {
    if (!event || !s_tp_state.queue_enabled) return -1;

    uv_mutex_lock(&s_tp_state.lock);

    if (s_tp_state.queue_head == s_tp_state.queue_tail) {
        // 队列为空
        uv_mutex_unlock(&s_tp_state.lock);
        return -1;
    }

    // 从队列头部取出事件
    *event = s_tp_state.event_queue[s_tp_state.queue_head];
    s_tp_state.queue_head = (s_tp_state.queue_head + 1) % s_tp_state.event_queue_size;

    uv_mutex_unlock(&s_tp_state.lock);

    return 0;
}

static int get_queue_count(void) {
    if (!s_tp_state.queue_enabled) return 0;

    uv_mutex_lock(&s_tp_state.lock);

    int count;
    if (s_tp_state.queue_tail >= s_tp_state.queue_head) {
        count = s_tp_state.queue_tail - s_tp_state.queue_head;
    } else {
        count = s_tp_state.event_queue_size - s_tp_state.queue_head + s_tp_state.queue_tail;
    }

    uv_mutex_unlock(&s_tp_state.lock);

    return count;
}

static int SDLCALL tp_sdl_watch(void* userdata, SDL_Event* e) {
    luat_tp_config_t* conf = (luat_tp_config_t*)userdata;
    if (!conf) return 0;
    uint8_t set = 0;
    luat_tp_data_t ev = {0};
    ev.track_id = 0;
    ev.width = 1;
    ev.timestamp = luat_mcu_ticks();
    switch (e->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (e->button.button == SDL_BUTTON_LEFT) {
                ev.event = TP_EVENT_TYPE_DOWN;
                ev.x_coordinate = (uint16_t)e->button.x;
                ev.y_coordinate = (uint16_t)e->button.y;
                set = 1;
                LLOGD("SDL_MOUSEBUTTONDOWN: DOWN at (%d,%d)", ev.x_coordinate, ev.y_coordinate);

                uv_mutex_lock(&s_tp_state.lock);
                s_tp_state.last_valid_x = ev.x_coordinate;
                s_tp_state.last_valid_y = ev.y_coordinate;
                uv_mutex_unlock(&s_tp_state.lock);
            }
            break;
        case SDL_MOUSEMOTION:
            // 直接依赖SDL状态处理MOVE事件
            if (e->motion.state & SDL_BUTTON_LMASK) {
                // 检查坐标是否在有效范围内
                int x = e->motion.x;
                int y = e->motion.y;

                if (x < 0 || x >= conf->w || y < 0 || y >= conf->h) {
                    // 移出边界，触发UP事件
                    ev.event = TP_EVENT_TYPE_UP;
                    ev.x_coordinate = s_tp_state.last_valid_x;
                    ev.y_coordinate = s_tp_state.last_valid_y;
                    set = 1;
                    LLOGD("SDL_MOUSEMOTION: OUT_OF_BOUNDS UP at (%d,%d)", ev.x_coordinate, ev.y_coordinate);
                } else {
                    // 在有效范围内，正常处理MOVE事件
                    ev.event = TP_EVENT_TYPE_MOVE;
                    ev.x_coordinate = (uint16_t)x;
                    ev.y_coordinate = (uint16_t)y;
                    set = 1;
                    LLOGD("SDL_MOUSEMOTION: MOVE at (%d,%d)", ev.x_coordinate, ev.y_coordinate);

                    uv_mutex_lock(&s_tp_state.lock);
                    s_tp_state.last_valid_x = ev.x_coordinate;
                    s_tp_state.last_valid_y = ev.y_coordinate;
                    uv_mutex_unlock(&s_tp_state.lock);
                }
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_LEFT) {
                ev.event = TP_EVENT_TYPE_UP;
                // 如果坐标在有效范围内，使用实际坐标，否则使用最后有效坐标
                if (e->button.x >= 0 && e->button.x < conf->w &&
                    e->button.y >= 0 && e->button.y < conf->h) {
                    ev.x_coordinate = (uint16_t)e->button.x;
                    ev.y_coordinate = (uint16_t)e->button.y;
                    LLOGD("SDL_MOUSEBUTTONUP: UP at (%d,%d)", ev.x_coordinate, ev.y_coordinate);
                } else {
                    ev.x_coordinate = s_tp_state.last_valid_x;
                    ev.y_coordinate = s_tp_state.last_valid_y;
                    LLOGD("SDL_MOUSEBUTTONUP: OUT_OF_BOUNDS UP at (%d,%d)", ev.x_coordinate, ev.y_coordinate);
                }
                set = 1;
            }
            break;
        case SDL_FINGERDOWN:
            ev.event = TP_EVENT_TYPE_DOWN;
            ev.x_coordinate = (uint16_t)(e->tfinger.x * conf->w);
            ev.y_coordinate = (uint16_t)(e->tfinger.y * conf->h);
            set = 1;
            LLOGD("SDL_FINGERDOWN: DOWN at (%d,%d)", ev.x_coordinate, ev.y_coordinate);
            uv_mutex_lock(&s_tp_state.lock);
            s_tp_state.last_valid_x = ev.x_coordinate;
            s_tp_state.last_valid_y = ev.y_coordinate;
            uv_mutex_unlock(&s_tp_state.lock);
            break;
        case SDL_FINGERMOTION:
            ev.event = TP_EVENT_TYPE_MOVE;
            ev.x_coordinate = (uint16_t)(e->tfinger.x * conf->w);
            ev.y_coordinate = (uint16_t)(e->tfinger.y * conf->h);
            set = 1;
            LLOGD("SDL_FINGERMOTION: MOVE at (%d,%d)", ev.x_coordinate, ev.y_coordinate);
            uv_mutex_lock(&s_tp_state.lock);
            s_tp_state.last_valid_x = ev.x_coordinate;
            s_tp_state.last_valid_y = ev.y_coordinate;
            uv_mutex_unlock(&s_tp_state.lock);
            break;
        case SDL_FINGERUP:
            ev.event = TP_EVENT_TYPE_UP;
            ev.x_coordinate = (uint16_t)(e->tfinger.x * conf->w);
            ev.y_coordinate = (uint16_t)(e->tfinger.y * conf->h);
            set = 1;
            LLOGD("SDL_FINGERUP: UP at (%d,%d)", ev.x_coordinate, ev.y_coordinate);
            break;
        default:
            break;
    }
    if (set) {
        if (s_tp_state.queue_enabled) {
            // 使用队列机制
            int queue_count = get_queue_count();
            int result = enqueue_event(&ev);

            if (result == 0) {
                const char* event_names[] = {"NONE", "DOWN", "UP", "MOVE"};
                const char* event_name = (ev.event >= 0 && ev.event <= 3) ? event_names[ev.event] : "UNKNOWN";
                LLOGD("Event enqueued: %s (type=%d) at (%d,%d), queue_depth=%d->%d",
                      event_name, ev.event, ev.x_coordinate, ev.y_coordinate, queue_count, get_queue_count());

                // 如果队列深度过高，打印警告
                if (get_queue_count() > s_tp_state.event_queue_size * 0.8) {
                    LLOGW("Event queue getting full: %d/%d", get_queue_count(), s_tp_state.event_queue_size);
                }

                // 始终保持与原有框架的兼容性 - 更新单事件缓冲区
                uv_mutex_lock(&s_tp_state.lock);
                s_tp_state.ev = ev;
                s_tp_state.has_event = 1;
                uv_mutex_unlock(&s_tp_state.lock);

                // wake tp task
                luat_rtos_message_send(conf->task_handle, 1, conf);

                // 为了解决快速连续的DOWN-MOVE事件，添加小延迟让框架有时间处理DOWN事件
                if (ev.event == TP_EVENT_TYPE_DOWN) {
                    luat_rtos_task_sleep(5); // 5ms延迟，让框架有时间处理DOWN事件
                }
            } else {
                LLOGE("Failed to enqueue event");
            }
        } else {
            // 使用原有的单事件机制
            uv_mutex_lock(&s_tp_state.lock);
            s_tp_state.ev = ev;
            s_tp_state.has_event = 1;
            uv_mutex_unlock(&s_tp_state.lock);

            // wake tp task
            luat_rtos_message_send(conf->task_handle, 1, conf);
        }
    }
    return 0;
}

static int luat_tp_pc_init(luat_tp_config_t* conf) {
    if (!conf) return -1;
    if (conf->tp_num == 0) conf->tp_num = 1;

    // 初始化tp_data数组
    for (int i = 0; i < LUAT_TP_TOUCH_MAX; i++) {
        conf->tp_data[i].event = TP_EVENT_TYPE_NONE;
        conf->tp_data[i].track_id = (uint8_t)i;
        conf->tp_data[i].x_coordinate = 0;
        conf->tp_data[i].y_coordinate = 0;
        conf->tp_data[i].width = 1;
        conf->tp_data[i].timestamp = 0;
    }

    // 初始化状态结构
    memset(&s_tp_state, 0, sizeof(s_tp_state));
    uv_mutex_init(&s_tp_state.lock);
    s_tp_state.last_valid_x = 0;
    s_tp_state.last_valid_y = 0;
    s_tp_state.data_consumed = 1; // 初始状态为已消费，避免被清空

    // 初始化队列机制
    s_tp_state.event_queue_size = 8; // 事件队列大小
    s_tp_state.queue_head = 0;
    s_tp_state.queue_tail = 0;
    s_tp_state.queue_enabled = 1; // 启用队列机制

    // 清空事件队列
    for (int i = 0; i < s_tp_state.event_queue_size; i++) {
        memset(&s_tp_state.event_queue[i], 0, sizeof(luat_tp_data_t));
        s_tp_state.event_queue[i].event = TP_EVENT_TYPE_NONE;
    }

    SDL_AddEventWatch(tp_sdl_watch, conf);
    LLOGI("pc tp init ok, w=%d h=%d tp_num=%d, queue_enabled=%d, queue_size=%d",
           conf->w, conf->h, conf->tp_num, s_tp_state.queue_enabled, s_tp_state.event_queue_size);
    return 0;
}

static int luat_tp_pc_read(luat_tp_config_t* conf, uint8_t* data) {
    (void)data;
    if (!conf) return -1;

    luat_tp_data_t event_to_deliver;
    int event_found = 0;

    if (s_tp_state.queue_enabled) {
        // 优先从队列读取事件
        if (dequeue_event(&event_to_deliver) == 0) {
            event_found = 1;
            LLOGD("tp_pc_read: delivering from queue, remaining events: %d", get_queue_count());
        } else if (s_tp_state.has_event) {
            // 队列为空，尝试使用单事件缓冲区（兼容性）
            uv_mutex_lock(&s_tp_state.lock);
            event_to_deliver = s_tp_state.ev;
            s_tp_state.has_event = 0;
            uv_mutex_unlock(&s_tp_state.lock);
            event_found = 1;
            LLOGD("tp_pc_read: delivering from single buffer (fallback)");
        }
    } else {
        // 使用原有的单事件机制
        if (s_tp_state.has_event) {
            uv_mutex_lock(&s_tp_state.lock);
            event_to_deliver = s_tp_state.ev;
            s_tp_state.has_event = 0;
            uv_mutex_unlock(&s_tp_state.lock);
            event_found = 1;
            LLOGD("tp_pc_read: delivering from single buffer (queue disabled)");
        }
    }

    if (event_found) {
        // 设置数据到Lua可访问的位置（保持原有框架兼容性）
        conf->tp_data[0] = event_to_deliver;
        // 为了兼容Lua端的索引（从1开始），同时设置tp_data[1]
        if (conf->tp_num > 1 && LUAT_TP_TOUCH_MAX > 1) {
            conf->tp_data[1] = event_to_deliver;
        }

        s_tp_state.data_consumed = 0; // 标记数据已设置但未消费

        // 添加读取调试信息
        const char* event_names[] = {"NONE", "DOWN", "UP", "MOVE"};
        const char* event_name = (event_to_deliver.event >= 0 && event_to_deliver.event <= 3) ?
                                event_names[event_to_deliver.event] : "UNKNOWN";
        LLOGD("tp_pc_read: delivering %s(type=%d) at (%d,%d) to Lua",
              event_name, event_to_deliver.event, event_to_deliver.x_coordinate, event_to_deliver.y_coordinate);
    } else {
        // 没有新事件时，检查是否需要清空数据
        if (s_tp_state.data_consumed) {
            // 数据已被消费，可以安全清空
            for (int i = 0; i < conf->tp_num && i < LUAT_TP_TOUCH_MAX; i++) {
                conf->tp_data[i].event = TP_EVENT_TYPE_NONE;
            }
            LLOGD("tp_pc_read: cleared data (consumed)");
        } else {
            // 数据尚未消费，保持原有数据不变，让Lua层有机会处理
            LLOGD("tp_pc_read: preserving existing data (not consumed yet)");
        }
    }

    return 0;
}

static void luat_tp_pc_read_done(luat_tp_config_t* conf) {
    if (!conf) return;
    uv_mutex_lock(&s_tp_state.lock);
    s_tp_state.data_consumed = 1; // 标记数据已被消费
    uv_mutex_unlock(&s_tp_state.lock);
}

static void luat_tp_pc_deinit(luat_tp_config_t* conf) {
    if (conf) {
        SDL_DelEventWatch(tp_sdl_watch, conf);

        // 清理队列
        if (s_tp_state.queue_enabled) {
            uv_mutex_lock(&s_tp_state.lock);
            s_tp_state.queue_head = 0;
            s_tp_state.queue_tail = 0;
            s_tp_state.queue_enabled = 0;
            uv_mutex_unlock(&s_tp_state.lock);
            LLOGD("Event queue cleaned up");
        }

        LLOGI("pc tp deinit complete");
    }
}


#endif
