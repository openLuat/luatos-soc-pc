
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

#define LUAT_HEAP_SIZE (1024*1024)
uint8_t luavm_heap[LUAT_HEAP_SIZE] = {0};

int cmdline_argc;
char** cmdline_argv;
uv_timespec64_t boot_ts;

int lua_main (int argc, char **argv);

void luat_log_init_win32(void);
void luat_uart_initial_win32(void);
void luat_network_init(void);

uv_loop_t *main_loop;

void uv_luat_main(void* args) {
    (void)args;
    // printf("cmdline_argc %d\n", cmdline_argc);
    if (cmdline_argc == 1) {
        lua_main(cmdline_argc, cmdline_argv);
    }
    else {
        luat_main();
    }
}

static void timer_nop(uv_timer_t *handle) {
    // 不需要操作东西
}

// boot
int main(int argc, char** argv) {
    cmdline_argc = argc;
    cmdline_argv = argv;
    main_loop = malloc(sizeof(uv_loop_t));
    // uv_replace_allocator(luat_heap_malloc, luat_heap_realloc, luat_heap_calloc, luat_heap_free);
    uv_loop_init(main_loop);
    uv_clock_gettime(UV_CLOCK_MONOTONIC, &boot_ts);

    luat_pcconf_init();

    luat_log_init_win32();
    bpool(luavm_heap, LUAT_HEAP_SIZE);
    luat_fs_init();
    luat_network_init();

    uv_thread_t l_main;
    uv_timer_t t;
    uv_timer_init(main_loop, &t);
    uv_timer_start(&t, timer_nop, 10000000, 10000000);
    uv_thread_create(&l_main, uv_luat_main, NULL);

    // uv_thread_join(&l_main);
    uv_run(main_loop, UV_RUN_DEFAULT);

    uv_loop_close(main_loop);
    free(main_loop);
    return 0;
}
