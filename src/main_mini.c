
#include <stdio.h>

#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include <stdlib.h>

#include "luat_pcconf.h"

#include "bget.h"

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

#include "uv.h"
#include "luat_mem.h"

#ifdef LUAT_USE_LVGL
uv_timer_t lvgl_timer;
#include "lvgl.h"
#endif

#define LUAT_HEAP_SIZE (2*1024*1024)
uint8_t luavm_heap[LUAT_HEAP_SIZE] = {0};

int cmdline_argc;
char** cmdline_argv;
// uv_timespec64_t boot_ts;
extern uint64_t uv_startup_ns;
extern void luat_lwip_init(void);

int lua_main (int argc, char** argv);

void luat_log_init_win32(void);
void luat_uart_initial_win32(void);
void luat_network_init(void);

uv_loop_t *main_loop;
uv_mutex_t timer_lock;

int luat_cmd_parse(int argc, char** argv);
static int luat_lvg_handler(lua_State* L, void* ptr);
static void lvgl_timer_cb(uv_timer_t* lvgl_timer);

int32_t luatos_pc_climode;

void uv_luat_main(void* args) {
    (void)args;
    // printf("cmdline_argc %d\n", cmdline_argc);
    if (cmdline_argc == 1) {
        luatos_pc_climode = 1;
        #ifdef LUAT_CONF_VM_64bit
        LLOGI("LuatOS@%s %s, Build: " __DATE__ " " __TIME__ " 64bit", "PC", LUAT_VERSION);
        #else
        LLOGI("LuatOS@%s %s, Build: " __DATE__ " " __TIME__ " 32bit", "PC", LUAT_VERSION);
        #endif
        lua_main(cmdline_argc, cmdline_argv);
    }
    else {
        luat_main();
    }
}

static void timer_nop(uv_timer_t *handle) {
    // 不需要操作东西
    (void)handle;
}

static void timer_lwip(uv_timer_t *handle);

// boot
int main(int argc, char** argv) {
    cmdline_argc = argc;
    cmdline_argv = argv;
    
#ifdef LUAT_USE_WINDOWS
    // Windows平台下自动设置控制台编码
    extern void luat_console_auto_encoding(void);
    luat_console_auto_encoding();
#endif

    main_loop = malloc(sizeof(uv_loop_t));
    // uv_replace_allocator(luat_heap_malloc, luat_heap_realloc, luat_heap_calloc, luat_heap_free);
    uv_loop_init(main_loop);
    // uv_clock_gettime(UV_CLOCK_MONOTONIC, &boot_ts);
    uv_startup_ns = uv_hrtime();
    uv_mutex_init(&timer_lock);

    luat_pcconf_init();

    luat_log_init_win32();
    bpool(luavm_heap, LUAT_HEAP_SIZE);
    luat_heap_opt_init(LUAT_HEAP_PSRAM);
    
    #ifdef LUAT_USE_LVGL
    lv_init();
    #endif
    luat_fs_init();
    luat_network_init();

    
    int ret = luat_cmd_parse(cmdline_argc, cmdline_argv);
    if (ret) {
        return ret;
    }

    #ifdef LUAT_USE_LVGL
    uv_timer_init(main_loop, &lvgl_timer);
    uv_timer_start(&lvgl_timer, lvgl_timer_cb, 25, 25);
    #endif

    #ifdef LUAT_USE_LWIP
    //LLOGD("初始化lwip");
    luat_lwip_init();
    #endif

    // uv_thread_t l_main;
    // 加一个NOP的timer，防止uv_run 立即退出
    uv_timer_t t;
    uv_timer_init(main_loop, &t);
    #if defined(LUAT_USE_LWIP)
    uv_timer_start(&t, timer_lwip, 5, 5);
    #else
    uv_timer_start(&t, timer_nop, 1000, 1000);
    #endif

    uv_luat_main(NULL);

    uv_loop_close(main_loop);
    free(main_loop);
    return 0;
}

// UI相关

#ifdef LUAT_USE_LVGL
static int luat_lvg_handler(lua_State* L, void* ptr) {
    (void)L;
    (void)ptr;
    lv_tick_inc(25);
    lv_task_handler();
    return 0;
}
static void lvgl_timer_cb(uv_timer_t* lvgl_timer) {
    rtos_msg_t msg = {
        .handler = luat_lvg_handler
    };
    luat_msgbus_put(&msg, 0);
}
#endif


void sys_check_timeouts(void);
static void timer_lwip(uv_timer_t *handle) {
    (void)handle;
    sys_check_timeouts();
}
