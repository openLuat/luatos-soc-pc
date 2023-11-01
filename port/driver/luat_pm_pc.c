#include "luat_base.h"

int luat_pm_request(int mode);

int luat_pm_release(int mode);

int luat_pm_dtimer_start(int id, size_t timeout);

int luat_pm_dtimer_stop(int id);

int luat_pm_dtimer_check(int id);

// void luat_pm_cb(int event, int arg, void* args);

int luat_pm_last_state(int *lastState, int *rtcOrPad);

int luat_pm_force(int mode);

int luat_pm_check(void);

int luat_pm_dtimer_list(size_t* count, size_t* list);

int luat_pm_dtimer_wakeup_id(int* id);

int luat_pm_poweroff(void);

int luat_pm_reset(void);

int luat_pm_power_ctrl(int id, uint8_t val);

int luat_pm_get_poweron_reason(void) {
    return 0; // 暂时就0吧
}

int luat_pm_iovolt_ctrl(int id, int val);

int luat_pm_wakeup_pin(int pin, int val);

int luat_pm_set_power_mode(uint8_t mode, uint8_t sub_mode);