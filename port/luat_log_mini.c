
#include "luat_base.h"
#include "luat_log.h"
#include "luat_uart.h"
#include "luat_malloc.h"
#include "printf.h"
#include "luat_mcu.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct log_msg {
    char* buff;
}log_msg_t;

static uint8_t luat_log_uart_port = 0;
static uint8_t luat_log_level_cur = LUAT_LOG_DEBUG;

#define LOGLOG_SIZE 4096

void luat_log_init_win32(void) {
}

void luat_log_set_uart_port(int port) {
    luat_log_uart_port = (uint8_t)port;
}

void luat_print(const char* _str) {
    luat_nprint((char*)_str, strlen(_str));
}

void luat_nprint(char *s, size_t l) {
    luat_log_write(s, l);
}
void luat_log_write(char *s, size_t l) {
    char buff[LOGLOG_SIZE] = {0};
    char *tmp = (char *)buff;
    uint64_t t = luat_mcu_tick64_ms();
    uint32_t sec = (uint32_t)(t / 1000);
    uint32_t ms = t % 1000;
    sprintf_(tmp, "[%08lu.%03lu]", sec, ms);
    tmp += strlen(tmp);
    memcpy(tmp, s, l);
    printf("%.*s", strlen(buff), buff);
}

void luat_log_set_level(int level) {
    luat_log_level_cur = (uint8_t)level;
}
int luat_log_get_level() {
    return luat_log_level_cur;
}

void luat_log_log(int level, const char* tag, const char* _fmt, ...) {
    if (luat_log_level_cur > level) return;
    char buff[LOGLOG_SIZE] = {0};
    char *tmp = (char *)buff;
    switch (level)
        {
        case LUAT_LOG_DEBUG:
            tmp[0] = 'D';
            break;
        case LUAT_LOG_INFO:
            tmp[0] = 'I';
            break;
        case LUAT_LOG_WARN:
            tmp[0] = 'W';
            break;
        case LUAT_LOG_ERROR:
            tmp[0] = 'E';
            break;
        default:
            tmp[0] = '?';
            break;
        }
    tmp ++;
    tmp[0] = '/';
    tmp ++;
    size_t taglen = strlen(tag);
    if (taglen > LOGLOG_SIZE / 2)
        taglen = LOGLOG_SIZE / 2;
    memcpy(tmp, tag, taglen);
    tmp += taglen;
    tmp[0] = ' ';
    tmp ++;
    size_t len = 0;
    va_list args;
    va_start(args, _fmt);
    len = vsnprintf_(tmp, LOGLOG_SIZE - strlen(buff), _fmt, args);
    va_end(args);
    if (len > 0) {
        len = strlen(buff);
        if (len > LOGLOG_SIZE - 1)
            len = LOGLOG_SIZE - 1;
        buff[len] = '\n';
        luat_log_write(buff, len+1);
    }
}

void luat_debug_assert(const char *fun_name, unsigned int line_no, const char *fmt, ...) {
    printf("ASSERT: %s:%u: ", fun_name, line_no);
    char buff[256] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf_(buff, sizeof(buff) - 1, fmt, args);
    va_end(args);
    printf("%s\n", buff);
    // 这里不做任何处理，直接退出
    exit(16);
}
