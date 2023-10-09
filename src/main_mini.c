
#include <stdio.h>

#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include <stdlib.h>

#include "bget.h"

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

#include "uv.h"

#define LUAT_HEAP_SIZE (1024*1024)
uint8_t luavm_heap[LUAT_HEAP_SIZE] = {0};

int cmdline_argc;
char** cmdline_argv;

int lua_main (int argc, char **argv);

void luat_log_init_win32(void);
void luat_uart_initial_win32(void);

uv_loop_t *main_loop;

void uv_luat_main(void* args) {
    lua_main(cmdline_argc, cmdline_argv);
}

static void _idle(uv_idle_t* handle) {

}

// boot
int main(int argc, char** argv) {   
    main_loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(main_loop);

    luat_log_init_win32();
    bpool(luavm_heap, LUAT_HEAP_SIZE);
    luat_fs_init();
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
/*
#include <uv.h>

void hare(void *arg) {
    int tracklen = *((int *) arg);
    while (tracklen) {
        tracklen--;
        uv_sleep(1000);
        fprintf(stderr, "Hare ran another step\n");
    }
    fprintf(stderr, "Hare done running!\n");
}

void tortoise(void *arg) {
    int tracklen = *((int *) arg);
    while (tracklen) {
        tracklen--;
        fprintf(stderr, "Tortoise ran another step\n");
        uv_sleep(3000);
    }
    fprintf(stderr, "Tortoise done running!\n");
}

int main() {
    int tracklen = 10;
    uv_thread_t hare_id;
    uv_thread_t tortoise_id;
    uv_thread_create(&hare_id, hare, &tracklen);
    uv_thread_create(&tortoise_id, tortoise, &tracklen);

    uv_thread_join(&hare_id);
    uv_thread_join(&tortoise_id);
    return 0;
}
*/
