#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_malloc.h"

#include "uv.h"

#define LUAT_LOG_TAG "rtos.timer"
#include "luat_log.h"


typedef struct timer_data
{
    void *cb;
    void *param;
    void *task_handle;
    int is_repeat;
    size_t timeout;
}timer_data_t;

typedef void (*tcb)(void*);

extern uv_loop_t *main_loop;

static void timer_cb(uv_timer_t *handle) {
    timer_data_t* data = (timer_data_t*)handle->data;
    if (data->cb)
        ((tcb)data->cb)(data->param);
    if (data->is_repeat) {
        uv_timer_start(handle, timer_cb, data->timeout, 0);
    }
}

// Timer类
void *luat_create_rtos_timer(void *cb, void *param, void *task_handle) {
    uv_timer_t *t = luat_heap_malloc(sizeof(uv_timer_t));
    if (t == NULL)
        return NULL;
    memset(t, 0, sizeof(uv_timer_t));
    uv_timer_init(main_loop, t);
    timer_data_t *data = luat_heap_malloc(sizeof(timer_data_t));
    if (data == NULL) {
        luat_heap_free(t);
        return NULL;
    }
    memset(data, 0, sizeof(timer_data_t));
    t->data = data;
    data->cb = cb;
    data->param = param;
    data->task_handle = task_handle;
    // LLOGD("创建rtos timer %p", t);
    return t;
}

int luat_start_rtos_timer(void *timer, uint32_t ms, uint8_t is_repeat) {
    uv_timer_t *t = (uv_timer_t *)timer;
    // LLOGD("启动rtos timer %p", t, ms, is_repeat);
    ((timer_data_t*)t->data)->is_repeat = is_repeat;
    ((timer_data_t*)t->data)->timeout = ms;
    return uv_timer_start(t, timer_cb, ms, 0);
}

// int luat_start_rtos_timer_us(void *timer, uint32_t us) {
//     LLOGD("luat_start_rtos_timer_us NOT support");
//     return -1;
// }
void luat_stop_rtos_timer(void *timer) {
    uv_timer_t *t = (uv_timer_t *)timer;
    uv_timer_stop(t);
}

void luat_release_rtos_timer(void *timer) {
    uv_timer_t *t = (uv_timer_t *)timer;
    uv_timer_stop(t);
    luat_heap_free(t->data);
    luat_heap_free(t);
}


void luat_task_suspend_all(void) {

}

void luat_task_resume_all(void) {

}

int luat_rtos_timer_create(luat_rtos_timer_t *timer_handle)
{
	if (!timer_handle) return -1;
	*timer_handle = luat_create_rtos_timer(NULL, NULL, NULL);
	return (*timer_handle)?0:-1;
}

int luat_rtos_timer_delete(luat_rtos_timer_t timer_handle)
{
	if (!timer_handle) return -1;
	luat_release_rtos_timer(timer_handle);
	return 0;
}

int luat_rtos_timer_start(luat_rtos_timer_t timer_handle, uint32_t timeout, uint8_t repeat, luat_rtos_timer_callback_t callback_fun, void *user_param)
{
	if (!timer_handle) return -1;
	uv_timer_t *t = (uv_timer_t *)timer_handle;
    ((timer_data_t*)t->data)->is_repeat = repeat;
    ((timer_data_t*)t->data)->timeout = timeout;
    ((timer_data_t*)t->data)->cb = callback_fun;
    ((timer_data_t*)t->data)->param = user_param;
    return uv_timer_start(t, timer_cb, timeout, 0);
}

int luat_rtos_timer_stop(luat_rtos_timer_t timer_handle)
{
	if (!timer_handle) return -1;
	luat_stop_rtos_timer(timer_handle);
	return 0;
}

