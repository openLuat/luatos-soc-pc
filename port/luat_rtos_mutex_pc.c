#include "luat_base.h"
#include "luat_rtos.h"
#include "luat_malloc.h"

#include "uv.h"

#define LUAT_LOG_TAG "rtos.mutex"
#include "luat_log.h"

/* -----------------------------------信号量模拟互斥锁，可以在中断中unlock-------------------------------*/
void *luat_mutex_create(void) {
    uv_mutex_t* m = luat_heap_malloc(sizeof(uv_mutex_t));
    uv_mutex_init(m);
    return m;
}
LUAT_RET luat_mutex_lock(void *mutex) {
    uv_mutex_t* m = (uv_mutex_t*)mutex;
    // LLOGD("mutex lock1 %p", m);
    uv_mutex_lock(m);
    // LLOGD("mutex lock2 %p", m);
}

LUAT_RET luat_mutex_unlock(void *mutex) {
    uv_mutex_t* m = (uv_mutex_t*)mutex;
    // LLOGD("mutex unlock1 %p", m);
    uv_mutex_unlock(m);
    // LLOGD("mutex unlock2 %p", m);
}

void luat_mutex_release(void *mutex) {
    uv_mutex_t* m = (uv_mutex_t*)mutex;
    // LLOGD("mutex release %p", m);
    // uv_mutex_destroy(m);
    luat_heap_free(m);
}
