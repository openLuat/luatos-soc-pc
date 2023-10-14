#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_gpio.h"
#include "luat_irq.h"
#include "luat_gpio.h"
#include "luat_gpio_drv.h"

luat_gpio_drv_opts_t* gpio_drvs[128];

static luat_gpio_t gpios[128];
static uint8_t gpio_levels[128];

int luat_gpio_setup(luat_gpio_t *gpio){
    if (gpio->pin >= 128)
        return -1;
    memcpy(&gpios[gpio->pin], gpio, sizeof(luat_gpio_t));
    if (gpio_drvs[gpio->pin]) {
        return gpio_drvs[gpio->pin]->setup(NULL, gpio);
    }
    return 0;
}

int luat_gpio_set(int pin, int level)
{
    if (pin < 0 || pin >= 128)
        return -1;
    gpio_levels[pin] = level == 0 ? 0 : 1;
    if (gpio_drvs[pin]) {
        return gpio_drvs[pin]->write(NULL, pin, level);
    }
    return 0;
}

//hyj
void luat_gpio_pulse(int pin, uint8_t *level, uint16_t len,uint16_t delay_ns)
{
    return;
}

int luat_gpio_get(int pin)
{
    if (pin < 0 || pin >= 128)
        return -1;
    if (gpio_drvs[pin]) {
        return gpio_drvs[pin]->read(NULL, pin);
    }
    if (gpios[pin].mode == LUAT_GPIO_INPUT || gpios[pin].mode == LUAT_GPIO_IRQ) {
        return gpio_levels[pin];
    }
    return 0;
}

void luat_gpio_close(int pin)
{
    if (pin < 0 || pin >= 128)
        return;
    memset(&gpios[pin], 0, sizeof(luat_gpio_t));
    gpios[pin].mode = LUAT_GPIO_INPUT;
    if (gpio_drvs[pin]) {
        gpio_drvs[pin]->close(NULL, pin);
    }
}

