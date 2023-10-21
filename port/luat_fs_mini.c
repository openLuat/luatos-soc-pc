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

// 从命令行参数构建luadb

void *build_luadb_from_cmd(void);

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
	void *ptr = build_luadb_from_cmd();
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
	return 0;
}

static char *luadb_ptr;
static size_t luadb_offset;
static int luadb_init(void)
{
	char *tmp = luat_heap_malloc(0x20);
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
	tmp[offset + 5] = 0x18;
	offset += 6;

	// file count
	tmp[offset + 0] = 0x04;
	tmp[offset + 1] = 0x02;
	tmp[offset + 2] = 0x00;
	tmp[offset + 3] = 0x00;
	offset += 4;

	// crc
	tmp[offset + 0] = 0xFE;
	tmp[offset + 1] = 0x02;
	tmp[offset + 2] = 0x00;
	tmp[offset + 3] = 0x00;
	offset += 4;

	luadb_ptr = tmp;
	luadb_offset = offset;
	return 0;
}

static int luadb_addfile(const char *name, char *data, size_t len)
{
	if (luadb_ptr == NULL)
	{
		luadb_init();
	}
	if (luadb_ptr == NULL)
	{
		return -1;
	}
	size_t offset = luadb_offset;
	char *tmp = luat_heap_realloc(luadb_ptr, luadb_offset + len + 512);
	if (tmp == NULL)
	{
		return -2;
	}

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
	tmp[offset + 1] = (uint8_t)(strlen(name) & 0xFF);
	memcpy(tmp + offset + 2, name, strlen(name));
	offset += 2 + strlen(name);

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

	memcpy(tmp + offset, data, len);

	offset += len;

	luadb_offset = offset;
	luadb_ptr = tmp;

	// 调整文件数量, TODO 兼容256个以上的文件
	luadb_ptr[0x12]++;

	return 0;
}

void *check_cmd_args(int index);
void *build_luadb_from_cmd(void) {
	if (cmdline_argc == 1)
	{
		return NULL;
	}
	for (size_t i = 1; i < (size_t)cmdline_argc; i++)
	{
		const char *argv = cmdline_argv[i];
		if (argv[0] == '-') {
			continue;
		}
		check_cmd_args(i);
	}
	return luadb_ptr;
}

void *check_cmd_args(int index)
{
	size_t len = 0;
	// int ret = 0;
	void *ptr = NULL;
	const char *path = cmdline_argv[index];
	if (strlen(path) < 4 || strlen(path) >= 512)
	{
		return NULL;
	}

	if (!memcmp(path + strlen(path) - 4, ".bin", 4))
	{
		FILE *f = fopen(path, "rb");
		if (!f)
		{
			LLOGE("无法打开luadb镜像文件 %s", path);
			return NULL;
		}
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		ptr = luat_heap_malloc(len);
		if (ptr == NULL)
		{
			fclose(f);
			LLOGE("luadb镜像文件太大,内存放不下 %s", path);
			return NULL;
		}
		fread(ptr, len, 1, f);
		fclose(f);
		return ptr;
	}
	if (!memcmp(path + strlen(path) - 4, ".lua", 4))
	{
		// LLOGD("把%s当做main.lua运行", path);
		char tmpname[512] = {0};
		FILE *f = fopen(path, "rb");
		if (!f)
		{
			LLOGE("lua文件 %s", path);
			return NULL;
		}
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
		// void* fptr = luat_heap_malloc(len);
		char *tmp = luat_heap_malloc(len);
		if (tmp == NULL)
		{
			fclose(f);
			LLOGE("lua文件太大,内存放不下 %s", path);
			return NULL;
		}
		fread(tmp, 1, len, f);
		fclose(f);
		for (size_t i = strlen(path); i > 0; i--)
		{
			if (path[i-1] == '/' || path[i-1] == '\\') {
				memcpy(tmpname, path + i, strlen(path) - 1);
				break;
			}
		}
		if (tmpname[0] == 0x00) {
			memcpy(tmpname, path, strlen(path));
		}

		// 开始合成luadb结构
		luadb_addfile(tmpname, tmp, len);
		luat_heap_free(tmp);
		return luadb_ptr;
	}
	// 目录模式
	if (!memcmp(path + strlen(path) - 1, "/", 1) || !memcmp(path + strlen(path) - 1, "\\", 1))
	{
		DIR *dp;
		struct dirent *ep;
		// int index = 0;
		FILE* f = NULL;
		char buff[512] = {0};

		// LLOGD("加载目录 %s", path);
		#ifdef LUA_USE_WINDOWS
		memcpy(buff, path, strlen(path));
		#else
		memcpy(buff, path, strlen(path) - 1);
		#endif;
		dp = opendir(buff);
		// LLOGD("目录打开 %p", dp);
		if (dp != NULL)
		{
			// LLOGD("开始遍历目录 %s", path);
			while ((ep = readdir(dp)) != NULL)
			{
				// LLOGD("文件/目录 %s %d", ep->d_name, ep->d_type);
				if (ep->d_type != DT_REG) {
					continue;
				}
				#ifdef LUA_USE_WINDOWS
				sprintf(buff, "%s\\%s", path, ep->d_name);
				#else
				sprintf(buff, "%s/%s", path, ep->d_name);
				#endif
				// index++;
				f = fopen(buff, "rb");
				if (f == NULL) {
					LLOGW("打开文件失败,跳过 %s", buff);
				}
				fseek(f, 0, SEEK_END);
				len = ftell(f);
				fseek(f, 0, SEEK_SET);
				char* tmp = luat_heap_malloc(len);
				if (tmp == NULL) {
					LLOGE("内存不足,无法加载文件 %s", buff);
				}
				else {
					fread(tmp, 1, len, f);
				}
				fclose(f);
				if (tmp) {
					luadb_addfile(ep->d_name, tmp, len);
					luat_heap_free(tmp);
				}
				// continue;
			}
			// LLOGD("遍历结束");
			(void)closedir(dp);
			return luadb_ptr;
		}
		else
		{
			LLOGW("opendir file %s failed", path);
			return NULL;
		}
	}
	// LLOGD("啥模式都不是, 没法加载 %s", path);
	return NULL;
}