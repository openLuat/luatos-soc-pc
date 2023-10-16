#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_malloc.h"

#include "uv.h"

#define LUAT_LOG_TAG "rtos.mutex"
#include "luat_log.h"
#include "luat_queue_pc.h"

typedef struct utask
{
    uv_thread_t t;
    uv_queue_item_t queue;
}utask_t;


LUAT_RET luat_send_event_to_task(void *task_handle, uint32_t id, uint32_t param1, uint32_t param2, uint32_t param3) {
    LLOGD("luat_send_event_to_task 暂未实现");
}
LUAT_RET luat_wait_event_from_task(void *task_handle, uint32_t wait_event_id, luat_event_t *out_event, void *call_back, uint32_t ms) {
    LLOGD("luat_wait_event_from_task 暂未实现");
}

void *luat_get_current_task(void) {

}
