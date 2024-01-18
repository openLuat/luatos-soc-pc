

void lwip_init(void);

#include "luat_mcu.h"
#include "luat_timer.h"
#include "luat_malloc.h"

void luat_lwip_init(void) {
    lwip_init();
}

unsigned int lwip_port_rand(void) {
    return rand();
}

unsigned int sys_now(void) {
    return luat_mcu_ticks();
}
