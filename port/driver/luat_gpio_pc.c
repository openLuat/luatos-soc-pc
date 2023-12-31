#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_timer.h"
#include "luat_gpio.h"
#include "luat_irq.h"
#include "luat_gpio.h"
#include "luat_gpio_drv.h"

#include "luat_ztt.h"

luat_gpio_drv_opts_t* gpio_drvs[128];

luat_gpio_t gpio_confs[128];
uint8_t gpio_levels[128];

int luat_gpio_setup(luat_gpio_t *gpio){
    if (gpio->pin >= 128)
        return -1;
    memcpy(&gpio_confs[gpio->pin], gpio, sizeof(luat_gpio_t));

    luat_ztt_t* ztt = ztt_create("gpio");
    ztt_add(ztt, "action", "setup", strlen("setup"));
    ztt_addf(ztt, "pin", "%d", gpio->pin);
    ztt_addf(ztt, "mode", "%d", gpio->mode);
    ztt_addf(ztt, "pull", "%d", gpio->pull);
    ztt_addf(ztt, "irq", "%d", gpio->irq);
    ztt_commit(ztt);

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

    luat_ztt_t* ztt = ztt_create("gpio");
    ztt_add(ztt, "action", "set", strlen("set"));
    ztt_addf(ztt, "pin", "%d", pin);
    ztt_addf(ztt, "level", "%d", level);
    ztt_commit(ztt);

    if (gpio_drvs[pin]) {
        return gpio_drvs[pin]->write(NULL, pin, level);
    }
    return 0;
}

//hyj
void luat_gpio_pulse(int pin, uint8_t *level, uint16_t len,uint16_t delay_ns)
{
    (void)pin;
    (void)level;
    (void)len;
    (void)delay_ns;
    return;
}

int luat_gpio_get(int pin)
{
    if (pin < 0 || pin >= 128)
        return -1;

    luat_ztt_t* ztt = ztt_create("gpio");
    ztt_add(ztt, "action", "get", strlen("get"));
    ztt_addf(ztt, "pin", "%d", pin);
    ztt_commit(ztt);

    if (gpio_drvs[pin]) {
        return gpio_drvs[pin]->read(NULL, pin);
    }
    if (gpio_confs[pin].mode == LUAT_GPIO_INPUT || gpio_confs[pin].mode == LUAT_GPIO_IRQ) {
        return gpio_levels[pin];
    }
    return 0;
}

void luat_gpio_close(int pin)
{
    if (pin < 0 || pin >= 128)
        return;

    luat_ztt_t* ztt = ztt_create("gpio");
    ztt_add(ztt, "action", "close", strlen("close"));
    ztt_addf(ztt, "pin", "%d", pin);
    ztt_commit(ztt);

    memset(&gpio_confs[pin], 0, sizeof(luat_gpio_t));
    gpio_confs[pin].mode = LUAT_GPIO_INPUT;
    if (gpio_drvs[pin]) {
        gpio_drvs[pin]->close(NULL, pin);
    }
}

