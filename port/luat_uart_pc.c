#include <stdlib.h>
#include <string.h>//add for memset
#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_uart.h"

#include "luat_uart_drv.h"

#define LUAT_LOG_TAG "uart"
#include "luat_log.h"

const luat_uart_drv_opts_t* uart_drvs[16];

int luat_uart_setup(luat_uart_t* uart) {
    if (!luat_uart_exist(uart->id))
        return -1;
    return uart_drvs[uart->id]->setup(NULL, uart);
}

int luat_uart_write(int uart_id, void* buffer, size_t length) {
    if (!luat_uart_exist(uart_id))
        return -1;
    return uart_drvs[uart_id]->write(NULL, uart_id, buffer, length);
}

int luat_uart_read(int uart_id, void* buffer, size_t length) {
    if (!luat_uart_exist(uart_id))
        return -1;
    return uart_drvs[uart_id]->read(NULL, uart_id, buffer, length);
}

// void luat_uart_clear_rx_cache(int uart_id) {
//     return 0;
// }

int luat_uart_close(int uart_id) {
    if (!luat_uart_exist(uart_id))
        return 0;
    return uart_drvs[uart_id]->close(NULL, uart_id);
}

int luat_uart_exist(int uart_id) {
    if (uart_id < 0 || uart_id >= 16)
        return 0;
    if (uart_drvs[uart_id] != NULL)
        return 1;
    return 0;
}

int luat_setup_cb(int uartid, int received, int sent) {
    return 0;
}
