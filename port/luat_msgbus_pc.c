#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"

#include "uv.h"


#define LUAT_LOG_TAG "msgbus"
#include "luat_log.h"


typedef struct uv_queue_item
{
    size_t is_head;
    void* next;
    void* prev;
    rtos_msg_t msg;
}uv_queue_item_t;

static uv_queue_item_t head;


void luat_msgbus_init(void) {
    head.is_head = 1;
}
uint32_t luat_msgbus_put(rtos_msg_t* msg, size_t timeout) {
    uv_queue_item_t *item = luat_heap_malloc(sizeof(uv_queue_item_t));
    if (item == NULL) {
        LLOGE("out of memory when malloc uv_queue_item_t");
        return -1;
    }
    memset(item, 0, sizeof(uv_queue_item_t));
    memcpy(&item->msg, msg, sizeof(rtos_msg_t));
    uv_queue_item_t* q = &head;
    while (1) {
        if (q->next == NULL) {
            q->next = item;
            item->prev = q;
            break;
        }
        q = (uv_queue_item_t*)q->next;
    }
    return 0;
}
uint32_t luat_msgbus_get(rtos_msg_t* msg, size_t timeout) {
    // LLOGD("luat_msgbus_get");
    uv_queue_item_t* q = &head;
    if (timeout > 0) {
        while (timeout >0) {
            if (head.next == NULL) {
                uv_sleep(1);
                timeout --;
                continue;
            }
            while (1) {
                if (q->next == NULL) {
                    memcpy(msg, &q->msg, sizeof(rtos_msg_t));
                    ((uv_queue_item_t*)q->prev)->next = NULL;
                    luat_heap_free(q);
                    return 0;
                }
                q = (uv_queue_item_t*)q->next;
            }
        }
    }
    else {
        while (1) {
            if (head.next == NULL) {
                uv_sleep(1);
                continue;
            }
            while (1) {
                if (q->next == NULL) {
                    memcpy(msg, &q->msg, sizeof(rtos_msg_t));
                    ((uv_queue_item_t*)q->prev)->next = NULL;
                    luat_heap_free(q);
                    return 0;
                }
                q = (uv_queue_item_t*)q->next;
            }
        }
    }
    return 1;
}
uint32_t luat_msgbus_freesize(void) {
    return 1;
}

uint8_t luat_msgbus_is_empty(void) {
    return head.next == NULL ? 1 : 0;
}
