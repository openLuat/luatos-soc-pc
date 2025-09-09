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
    luat_tp_data_t ev; // single-point pending event
} tp_pc_state_t;

static tp_pc_state_t s_tp_state;

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
            }
            break;
        case SDL_MOUSEMOTION:
            if (e->motion.state & SDL_BUTTON_LMASK) {
                ev.event = TP_EVENT_TYPE_MOVE;
                ev.x_coordinate = (uint16_t)e->motion.x;
                ev.y_coordinate = (uint16_t)e->motion.y;
                set = 1;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (e->button.button == SDL_BUTTON_LEFT) {
                ev.event = TP_EVENT_TYPE_UP;
                ev.x_coordinate = (uint16_t)e->button.x;
                ev.y_coordinate = (uint16_t)e->button.y;
                set = 1;
            }
            break;
        case SDL_FINGERDOWN:
            ev.event = TP_EVENT_TYPE_DOWN;
            ev.x_coordinate = (uint16_t)(e->tfinger.x * conf->w);
            ev.y_coordinate = (uint16_t)(e->tfinger.y * conf->h);
            set = 1;
            break;
        case SDL_FINGERMOTION:
            ev.event = TP_EVENT_TYPE_MOVE;
            ev.x_coordinate = (uint16_t)(e->tfinger.x * conf->w);
            ev.y_coordinate = (uint16_t)(e->tfinger.y * conf->h);
            set = 1;
            break;
        case SDL_FINGERUP:
            ev.event = TP_EVENT_TYPE_UP;
            ev.x_coordinate = (uint16_t)(e->tfinger.x * conf->w);
            ev.y_coordinate = (uint16_t)(e->tfinger.y * conf->h);
            set = 1;
            break;
        default:
            break;
    }
    if (set) {
        uv_mutex_lock(&s_tp_state.lock);
        s_tp_state.ev = ev;
        s_tp_state.has_event = 1;
        uv_mutex_unlock(&s_tp_state.lock);
        // wake tp task
        luat_rtos_message_send(conf->task_handle, 1, conf);
    }
    return 0;
}

static int luat_tp_pc_init(luat_tp_config_t* conf) {
    if (!conf) return -1;
    if (conf->tp_num == 0) conf->tp_num = 1;
    for (int i = 0; i < LUAT_TP_TOUCH_MAX; i++) {
        conf->tp_data[i].event = TP_EVENT_TYPE_NONE;
        conf->tp_data[i].track_id = (uint8_t)i;
        conf->tp_data[i].x_coordinate = 0;
        conf->tp_data[i].y_coordinate = 0;
        conf->tp_data[i].width = 1;
        conf->tp_data[i].timestamp = 0;
    }
    memset(&s_tp_state, 0, sizeof(s_tp_state));
    uv_mutex_init(&s_tp_state.lock);
    SDL_AddEventWatch(tp_sdl_watch, conf);
    LLOGI("pc tp init ok, w=%d h=%d tp_num=%d", conf->w, conf->h, conf->tp_num);
    return 0;
}

static int luat_tp_pc_read(luat_tp_config_t* conf, uint8_t* data) {
    (void)data;
    if (!conf) return -1;
    uv_mutex_lock(&s_tp_state.lock);
    if (s_tp_state.has_event) {
        conf->tp_data[0] = s_tp_state.ev;
        s_tp_state.has_event = 0;
    } else {
        for (int i = 0; i < conf->tp_num && i < LUAT_TP_TOUCH_MAX; i++) {
            conf->tp_data[i].event = TP_EVENT_TYPE_NONE;
        }
    }
    uv_mutex_unlock(&s_tp_state.lock);
    return 0;
}

static void luat_tp_pc_read_done(luat_tp_config_t* conf) {
    (void)conf;
}

static void luat_tp_pc_deinit(luat_tp_config_t* conf) {
    if (conf) {
        SDL_DelEventWatch(tp_sdl_watch, conf);
    }
}


#endif
