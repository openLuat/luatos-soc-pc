
#include "luat_base.h"
#include "luat_gpio.h"
#include "luat_msgbus.h"
#include "luat_spi.h"

// 模拟SPI在win32下的实现
// TODO 当需要返回数据时, 调用lua方法获取需要返回的数据

#define LUAT_WIN32_SPI_COUNT (3)

typedef struct win32spi {
    luat_spi_t spi;
    uint8_t open;
}win32spi_t;

win32spi_t win32spis[LUAT_WIN32_SPI_COUNT] = {0};

int luat_spi_device_config(luat_spi_device_t* spi_dev){
    return 0;
}

int luat_spi_bus_setup(luat_spi_device_t* spi_dev){
    int bus_id = spi_dev->bus_id;
    if (bus_id < 0 || bus_id >= LUAT_WIN32_SPI_COUNT) {
        return -1;
    }
    memcpy(&win32spis[bus_id].spi, &(spi_dev->spi_config), sizeof(luat_spi_t));
    win32spis[bus_id].open = 1;
    return 0;
}

int luat_spi_setup(luat_spi_t* spi) {
    if (spi->id < 0 || spi->id >= LUAT_WIN32_SPI_COUNT) {
        return -1;
    }
    memcpy(&win32spis[spi->id].spi, spi, sizeof(luat_spi_t));
    win32spis[spi->id].open = 1;
    return 0;
}
//关闭SPI，成功返回0
int luat_spi_close(int spi_id) {
    if (spi_id < 0 || spi_id >= LUAT_WIN32_SPI_COUNT) {
        return -1;
    }
    win32spis[spi_id].open = 0;
    return 0;
}
//收发SPI数据，返回接收字节数
int luat_spi_transfer(int spi_id, const char* send_buf, size_t send_length, char* recv_buf, size_t recv_length) {
    if (spi_id < 0 || spi_id >= LUAT_WIN32_SPI_COUNT) {
        return -1;
    }
    if (win32spis[spi_id].open == 0)
        return -1;
    memset(recv_buf, 0, recv_length);
    return recv_length;
}
//收SPI数据，返回接收字节数
int luat_spi_recv(int spi_id, char* recv_buf, size_t length) {
    if (spi_id < 0 || spi_id >= LUAT_WIN32_SPI_COUNT) {
        return -1;
    }
    if (win32spis[spi_id].open == 0)
        return -1;
    memset(recv_buf, 0, length);
    return length;
}
//发SPI数据，返回发送字节数
int luat_spi_send(int spi_id, const char* send_buf, size_t length) {
    if (spi_id < 0 || spi_id >= LUAT_WIN32_SPI_COUNT) {
        return -1;
    }
    if (win32spis[spi_id].open == 0)
        return -1;
    return length;
}

int luat_spi_change_speed(int spi_id, uint32_t speed){
    return 0;
}

#ifdef LUAT_USE_LCD
#include "luat_lcd.h"

int luat_lcd_qspi_config(luat_lcd_conf_t* conf, luat_lcd_qspi_conf_t *qspi_config) {
    return -1;
};

int luat_lcd_qspi_auto_flush_on_off(luat_lcd_conf_t* conf, uint8_t on_off) {
    return -1;
}
int luat_lcd_run_api_in_service(luat_lcd_api api, void *param, uint32_t param_len) {
    return -1;
};


#endif

int luat_spi_lock(int spi_id)
{
    return -1;
}

int luat_spi_unlock(int spi_id)
{
	return -1;
}