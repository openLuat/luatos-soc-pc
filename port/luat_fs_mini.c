#include "luat_base.h"
#include "luat_fs.h"
#include "luat_malloc.h"

#define LUAT_LOG_TAG "fs"
#include "luat_log.h"

extern const struct luat_vfs_filesystem vfs_fs_posix;
extern const struct luat_vfs_filesystem vfs_fs_luadb;
// extern const struct luat_vfs_filesystem vfs_fs_ramfs;

extern int cmdline_argc;
extern char** cmdline_argv;

// 从命令行参数构建luadb

void* build_luadb_from_cmd(void);

int luat_fs_init(void) {
	// vfs进行必要的初始化
	luat_vfs_init(NULL);
	// 注册vfs for posix 实现
	luat_vfs_reg(&vfs_fs_posix);
	luat_vfs_reg(&vfs_fs_luadb);
	// luat_vfs_reg(&vfs_fs_ramfs);

	luat_fs_conf_t conf = {
		.busname = "",
		.type = "posix",
		.filesystem = "posix",
		.mount_point = "",
	};
	luat_fs_mount(&conf);

	// 挂载虚拟的/luadb
	void* ptr = build_luadb_from_cmd();
	if (ptr != NULL) {
		luat_fs_conf_t conf2 = {
			.busname = ptr,
			.type = "luadb",
			.filesystem = "luadb",
			.mount_point = "/luadb/",
		};
		luat_fs_mount(&conf2);
	}
	return 0;
}

void* build_luadb_from_cmd(void) {
	size_t len = 0;
	int ret = 0;
	void* ptr = NULL;
	if (cmdline_argc == 1) {
		return NULL;
	}
	const char* path = cmdline_argv[1];
	if (strlen(path) < 4) {
		return NULL;
	}

	if (!memcmp(path + strlen(path) - 4, ".bin", 4)) {
		FILE* f = fopen(path, "rb");
		if (!f) {
			LLOGE("无法打开luadb镜像文件 %s", path);
			return NULL;
		}
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		ptr = luat_heap_malloc(len);
		if (ptr == NULL) {
			LLOGE("luadb镜像文件太大,内存放不下 %s", path);
			return NULL;
		}
		fread(ptr, len, 1, f);
		fclose(f);
		return ptr;
	}
	if (!memcmp(path + strlen(path) - 4, ".lua", 4)) {
		LLOGD("把%s当做main.lua运行", path);
		FILE* f = fopen(path, "rb");
		if (!f) {
			LLOGE("lua文件 %s", path);
			return NULL;
		}
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		// void* fptr = luat_heap_malloc(len);
		uint8_t* tmp = luat_heap_malloc(len + 512);
		if (tmp == NULL) {
			LLOGE("lua文件太大,内存放不下 %s", path);
			return NULL;
		}
		memset(tmp, 0, 512);
		fread(tmp + 512, 1, len, f);
		fclose(f);
		// 开始合成luadb结构

		size_t offset = 0;
		// magic of luadb
		tmp[offset + 0] = 0x01;
		tmp[offset + 1] = 0x04;
		tmp[offset + 2] = 0x5A;
		tmp[offset + 3] = 0xA5;
		tmp[offset + 4] = 0x5A;
		tmp[offset + 5] = 0xA5;
		offset += 6;

		// version 0x00 0x02
		tmp[offset + 0] = 0x02;
		tmp[offset + 1] = 0x02;
		tmp[offset + 2] = 0x02;
		tmp[offset + 3] = 0x00;
		offset += 4;

		// headers total size
		tmp[offset + 0] = 0x03;
		tmp[offset + 1] = 0x04;
		tmp[offset + 2] = 0x00;
		tmp[offset + 3] = 0x00;
		tmp[offset + 4] = 0x00;
		tmp[offset + 5] = 0x16;
		offset += 6;

		// file count
		tmp[offset + 0] = 0x04;
		tmp[offset + 1] = 0x02;
		tmp[offset + 2] = 0x01;
		tmp[offset + 3] = 0x00;
		offset += 4;

		// crc
		tmp[offset + 0] = 0xFE;
		tmp[offset + 1] = 0x02;
		tmp[offset + 2] = 0x00;
		tmp[offset + 3] = 0x00;
		offset += 4;

		// 下面是文件了
		
		// magic of file
		tmp[offset + 0] = 0x01;
		tmp[offset + 1] = 0x04;
		tmp[offset + 2] = 0x5A;
		tmp[offset + 3] = 0xA5;
		tmp[offset + 4] = 0x5A;
		tmp[offset + 5] = 0xA5;
		offset += 6;

		// name of file
		tmp[offset + 0] = 0x02;
		tmp[offset + 1] = (uint8_t)(strlen("main.lua") & 0xFF);
		memcpy(tmp + offset + 2, "main.lua", strlen("main.lua"));
		offset += 2 + strlen("main.lua");

		// len of file data
		tmp[offset + 0] = 0x03;
		tmp[offset + 1] = 0x04;
		tmp[offset + 2] = (len >> 0) & 0xFF;
		tmp[offset + 3] = (len >> 8) & 0xFF;
		tmp[offset + 4] = (len >> 16) & 0xFF;
		tmp[offset + 5] = (len >> 24) & 0xFF;
		offset += 6;

		// crc
		tmp[offset + 0] = 0xFE;
		tmp[offset + 1] = 0x02;
		tmp[offset + 2] = 0x00;
		tmp[offset + 3] = 0x00;
		offset += 4;

		memcpy(tmp + offset, tmp + 512, len);

		offset += len;

		return tmp;
	}
	return NULL;
}