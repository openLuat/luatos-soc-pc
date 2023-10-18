
#ifndef LUAT_CONF_BSP
#define LUAT_CONF_BSP

#include "stdint.h"

#define LUAT_BSP_VERSION "V1106"
#define LUAT_USE_CMDLINE_ARGS 1
// 启用64位虚拟机
// #define LUAT_CONF_VM_64bit
#define LUAT_RTOS_API_NOTOK 1
// #define LUAT_RET void
#define LUAT_RT_RET_TYPE	void
#define LUAT_RT_CB_PARAM void *param

#define LUA_USE_VFS_FILENAME_OFFSET 1

#define LUAT_USE_FS_VFS 1

#define LUAT_USE_VFS_INLINE_LIB 1

#define LUAT_COMPILER_NOWEAK 1

#define LUAT_USE_LOG_ASYNC_THREAD 0

#define LUAT_USE_NETWORK 1
#define LUAT_USE_SNTP 1
#define LUAT_USE_TLS  1

//----------------------------
// 外设,按需启用, 最起码启用uart和wdt库
#define LUAT_USE_UART 1
#define LUAT_USE_GPIO 1
// #define LUAT_USE_I2C  1
// #define LUAT_USE_SPI  1
// #define LUAT_USE_ADC  1
// #define LUAT_USE_PWM  1
// #define LUAT_USE_WDT  1
// #define LUAT_USE_PM  1
#define LUAT_USE_MCU  1
#define LUAT_USE_RTC 1

#define LUAT_USE_IOTAUTH 1
#define LUAT_USE_MINIZ 1
#define LUAT_USE_GMSSL 1

//----------------------------
// 常用工具库, 按需启用, cjson和pack是强烈推荐启用的
#define LUAT_USE_CRYPTO  1
#define LUAT_USE_CJSON  1
#define LUAT_USE_ZBUFF  1
#define LUAT_USE_PACK  1
#define LUAT_USE_LIBGNSS  1
#define LUAT_USE_MQTTCORE 1
#define LUAT_USE_LIBCOAP 1
#define LUAT_USE_FS  1
// #define LUAT_USE_SENSOR  1
// #define LUAT_USE_SFUD  1
// #define LUAT_USE_STATEM 1
// 性能测试
// #define LUAT_USE_COREMARK 1
// #define LUAT_USE_IR 1
// FDB 提供kv数据库, 与nvm库类似
// #define LUAT_USE_FDB 1
// FSKV库提供fdb库的兼容API, 目标是替代fdb库
#define LUAT_USE_FSKV 1
#define LUAT_CONF_FSKV_CUSTOM 1
// #define LUAT_USE_OTA 1
// #define LUAT_USE_I2CTOOLS 1
// #define LUAT_USE_LORA 1
// #define LUAT_USE_LORA2 1
// #define LUAT_USE_MAX30102 1
// #define LUAT_USE_MLX90640 1
#define LUAT_USE_YMODEM 1

// #define LUAT_USE_FATFS
// #define LUAT_USE_FATFS_CHINESE 3

//----------------------------
// 高级功能, 推荐使用repl, shell已废弃
// #define LUAT_USE_SHELL 1
// #define LUAT_USE_DBG
// #define LUAT_USE_REPL 1
// 多虚拟机支持,实验性,一般不启用
// #define LUAT_USE_VMX 1
#define LUAT_USE_PROTOBUF 1

#define LUAT_USE_RSA 1
#define LUAT_USE_ICONV 1
#define LUAT_USE_BIT64 1
#define LUAT_USE_FASTLZ 1

#endif
