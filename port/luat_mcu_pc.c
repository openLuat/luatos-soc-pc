
#include "uv.h"

#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_malloc.h"
#include "luat_mcu.h"

#define LUAT_LOG_TAG "mcu"
#include "luat_log.h"



int luat_mcu_set_clk(size_t mhz) {
    return 0;
}
int luat_mcu_get_clk(void) {
    return 1024;
}

char mcu_unique_id[] = "";
extern uv_timespec64_t boot_ts;

const char* luat_mcu_unique_id(size_t* t) {
    return (const char*)mcu_unique_id;
}

long luat_mcu_ticks(void) {
    return luat_mcu_tick64() / 1000;
}
uint32_t luat_mcu_hz(void) {
    return 1;
}

uint64_t luat_mcu_tick64(void) {
    uv_timespec64_t ts;
    int ret = uv_clock_gettime(UV_CLOCK_MONOTONIC, &ts);
    if (ret) {
        return 0;
    }
    uint64_t tmp = ts.tv_sec * 1000000 + ts.tv_nsec;
    uint64_t tmp2 = boot_ts.tv_sec * 1000000 + boot_ts.tv_nsec;
    return tmp - tmp2;
}

int luat_mcu_us_period(void) {
    return 1;
}

uint64_t luat_mcu_tick64_ms(void) {
    return luat_mcu_tick64() / luat_mcu_us_period();
}

void luat_mcu_set_clk_source(uint8_t source_main, uint8_t source_32k, uint32_t delay) {
    // nop
}

void luat_mcu_iomux_ctrl(uint8_t type, uint8_t sn, int pad_index, uint8_t alt, uint8_t is_input) {
    // nop
}

void luat_mcu_set_hardfault_mode(int mode) {
    // nop
}
