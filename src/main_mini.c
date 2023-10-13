
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
    lua_main(cmdline_argc, cmdline_argv);
}

static void _idle(uv_idle_t* handle) {

}

// boot
int main(int argc, char** argv) {
    uv_clock_gettime(UV_CLOCK_MONOTONIC, &boot_ts);
    main_loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(main_loop);

    luat_pcconf_init();

    luat_log_init_win32();
    bpool(luavm_heap, LUAT_HEAP_SIZE);
    luat_fs_init();
    luat_network_init();
#ifdef LUAT_USE_LUAC
    extern int luac_main(int argc, char* argv[]);
    luac_main(argc, argv);
#else
    cmdline_argc = argc;
    cmdline_argv = argv;
    if (cmdline_argc > 1) {
        size_t len = strlen(cmdline_argv[1]);
        if (cmdline_argv[1][0] != '-') {
            if (cmdline_argv[1][len - 1] == '/' || cmdline_argv[1][len - 1] == '\\') {
                printf("chdir %s %d\n", cmdline_argv[1], chdir(cmdline_argv[1]));
                cmdline_argc = 1;
            }
        }
    }

    uv_thread_t l_main;
    uv_idle_t idle;
    uv_idle_init(main_loop, &idle);
    uv_idle_start(&idle, _idle);
    uv_thread_create(&l_main, uv_luat_main, NULL);

    // uv_thread_join(&l_main);
    uv_run(main_loop, UV_RUN_DEFAULT);

    // uv_loop_close(main_loop);
    // free(main_loop);
    LLOGD("uv_run is done");
#endif
    return 0;
}
