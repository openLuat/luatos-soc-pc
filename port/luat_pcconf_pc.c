
#include "uv.h"

#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "luat_mcu.h"
#include "luat_uart_drv.h"

#include "luat_pcconf.h"

#define LUAT_LOG_TAG "pc"
#include "luat_log.h"

luat_pcconf_t g_pcconf;
extern const luat_uart_drv_opts_t* uart_drvs[];
extern const luat_uart_drv_opts_t uart_udp;

void luat_pcconf_init(void) {
    memcpy(g_pcconf.mcu_unique_id, "LuatOS@PC", strlen("LuatOS@PC"));
    g_pcconf.mcu_unique_id_len = strlen("LuatOS@PC");
    
    uart_drvs[0] = &uart_udp;
    uart_drvs[1] = &uart_udp;
    uart_drvs[2] = &uart_udp;
}

void luat_pcconf_save(void) {

}

