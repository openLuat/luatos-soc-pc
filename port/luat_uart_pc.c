#include <stdlib.h>
#include <string.h>//add for memset
#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_uart.h"

#define LUAT_LOG_TAG "uart"
#include "luat_log.h"

int luat_uart_setup(luat_uart_t* uart) {
    return -1;
}

int luat_uart_write(int uart_id, void* data, size_t length) {
    return -1;
}

int luat_uart_read(int uart_id, void* buffer, size_t length) {
    return -1;
}

// void luat_uart_clear_rx_cache(int uart_id) {
//     return 0;
// }

int luat_uart_close(int uart_id) {
    return 0;
}

int luat_uart_exist(int uart_id) {
    return 0;
}

int luat_setup_cb(int uartid, int received, int sent) {
    return 0;
}
