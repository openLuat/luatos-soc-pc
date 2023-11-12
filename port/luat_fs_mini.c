#include "luat_base.h"
#include "luat_fs.h"
#include "luat_malloc.h"

#define LUAT_LOG_TAG "fs"
#include "luat_log.h"

#include "dirent.h"

extern const struct luat_vfs_filesystem vfs_fs_posix;
extern const struct luat_vfs_filesystem vfs_fs_luadb;
extern const struct luat_vfs_filesystem vfs_fs_ram;

extern int cmdline_argc;
extern char **cmdline_argv;

extern char *luadb_ptr;

// 从命令行参数构建luadb

void *build_luadb_from_cmd(void);

static void lvgl_fs_init(void);

int luat_fs_init(void)
{
	// vfs进行必要的初始化
	luat_vfs_init(NULL);
	// 注册vfs for posix 实现
	luat_vfs_reg(&vfs_fs_posix);
	luat_vfs_reg(&vfs_fs_luadb);
	luat_vfs_reg(&vfs_fs_ram);

	luat_fs_conf_t conf = {
		.busname = "",
		.type = "posix",
		.filesystem = "posix",
		.mount_point = "",
	};
	luat_fs_mount(&conf);

	// 挂载虚拟的/ram
	luat_fs_conf_t conf_ram = {
		.busname = "",
		.type = "ram",
		.filesystem = "ram",
		.mount_point = "/ram/",
	};
	luat_fs_mount(&conf_ram);

	// 挂载虚拟的/luadb
	void *ptr = luadb_ptr;
	if (ptr != NULL)
	{
		luat_fs_conf_t conf2 = {
			.busname = ptr,
			.type = "luadb",
			.filesystem = "luadb",
			.mount_point = "/luadb/",
		};
		luat_fs_mount(&conf2);
	}

#ifdef LUAT_USE_LVGL
	lvgl_fs_init();
#endif
	return 0;
}


#ifdef LUAT_USE_LVGL
#include "luat_lvgl.h"
static void lvgl_fs_init(void) {
	luat_lv_fs_init();
    #ifdef LUAT_USE_LVGL_BMP
	lv_bmp_init();
    #endif
    #ifdef LUAT_USE_LVGL_PNG
	lv_png_init();
    #endif
    #ifdef LUAT_USE_LVGL_JPG
	lv_split_jpeg_init();
    #endif
}
#endif
