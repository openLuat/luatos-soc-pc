#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_timer.h"
#include "luat_luadb2.h"
#include "luat_luac_report.h"
#include "lundump.h"

#include "stdio.h"

#define LUAT_LOG_TAG "luf"
#include "luat_log.h"

static void DumpByte(tio_t *io, lu_byte c) {
    memcpy(io->ptr, (char*)&c, 1);
    io->ptr += 1;
}

static void DumpBlock(tio_t *io, void *b, size_t size) {
    memcpy(io->ptr, (char*)b, size);
    io->ptr += size;
}

static void DumpInt(tio_t *io, int x) {
    DumpBlock(io, &x, sizeof(int));
}

static void DumpInteger(tio_t *io, lua_Integer x) {
    DumpBlock(io, &x, sizeof(lua_Integer));
}

static void DumpNumber(tio_t *io, lua_Number x) {
    DumpBlock(io, &x, sizeof(lua_Number));
}

static void DumpString(tio_t *io, luac_data_t *s) {
    if (s == NULL || s->len == 0) {
        DumpByte(io, 0);
    }
    else {
        size_t len = s->len;
        if (len < 0xFF) {
            DumpByte(io, (lu_byte)len);
        } else {
            DumpByte(io, 0xFF);
            DumpInt(io, len);
        }
        DumpBlock(io, s->data, len - 1);
    }
}

static void DumpData(tio_t *io, luac_data_t *s) {
    if (s == NULL || s->len == 0) {
        return;
    }
    // LLOGD("DumpData???");
    DumpBlock(io, s->data, s->len);
    // LLOGD("DumpData222???");
}

static void DumpCode(tio_t *io, luf_func_t* func) {
    DumpInt(io, func->code->len / sizeof(Instruction));
    DumpBlock(io, func->code->data, func->code->len);
}

static void DumpConstants(tio_t *io, luf_func_t* func) {
    // LLOGD("导出常量表 数量: %d", func->sizek);
    DumpInt(io, func->sizek);
    for (size_t i = 0; i < func->sizek; i++) {
        luac_data_t* c = func->k[i];
        // LLOGD("常量指针 %p", c);
        if (c == NULL) {
            DumpByte(io, LUA_TNIL);
            continue;
        }
        if (c->type == LUA_TSTRING || c->type == LUA_TLNGSTR) {
            if (c->len > 41) {
                DumpByte(io, LUA_TLNGSTR);
            }
            else {
                DumpByte(io, LUA_TSTRING);
            }
        }
        else {
            DumpByte(io, c->type);
        }
        switch (c->type & 0x3F) {
            case LUA_TNIL:
                break;
            case LUA_TBOOLEAN:
                DumpByte(io, c->nbyte);
                break;
            case LUA_TNUMFLT:
                DumpNumber(io, c->nnum);
                break;
            case LUA_TNUMINT:
                DumpInteger(io, c->nint);
                break;
            case LUA_TSTRING:
            case LUA_TLNGSTR:
                // LLOGD("导出字符串 %d %.*s", c->len, c->len, (char*)c->data);
                DumpString(io, c);
                break;
            default:
                LLOGE("不支持的常量类型: 0x%02X 肯定出bug", c->type);
                break;
        }
    }
}

static void DumpUpvalues(tio_t *io, luf_func_t* func) {
    DumpInt(io, func->sizeupvalues);
    DumpBlock(io, func->upvalues, func->sizeupvalues * 2);
}

static void DumpDebug(tio_t *io, luf_func_t* func) {
    // 首先是行号
    // LLOGD("导出行号 %d %p", func->sizelineinfo, func->lineinfo);
    DumpInt(io, func->sizelineinfo);
    // LLOGD("行号数据有问题?? %d %p", func->lineinfo->len, func->lineinfo->data);
    // LLOGD("行号 %08X 代码 %08X", func->lineinfo->len / 4, func->code->len / 4);
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    // DumpData(io, func->lineinfo);
    DumpBlock(io, func->lineinfo->data, func->lineinfo->len);
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    // 其次是变量名
    // LLOGD("导出变量名");
    DumpInt(io, func->sizelocvars);
    for (size_t i = 0; i < func->sizelocvars; i++) {
        DumpString(io, func->locvars2[i]);
        DumpInt(io, func->locvars[i*2]);
        DumpInt(io, func->locvars[i*2+1]);
    }
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    // LLOGD("导出upvalues");
    DumpInt(io, func->sizeupvalues);
    for (size_t i = 0; i < func->sizeupvalues; i++) {
        DumpString(io, func->upvalues2[i]);
    }
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
}

static void DumpFunction(tio_t *io, luf_func_t* func);

static void DumpProtos(tio_t *io, luf_func_t* func) {
    DumpInt(io, func->sizep);
    // LLOGD("导出子函数 数量: %d", func->sizep);
    if (func->sizep > 0) {
        for (int i = 0; i < func->sizep; i++) {
            // LLOGD("导出[%p]的子函数: %d %p", func, i+1, func->p[i]);
            if (func == func->p[i + 1]) {
                // LLOGE("导出子函数时, 发现自己!!");
                continue;
            }
            DumpFunction(io, func->p[i]);
            // LLOGD("导出[%p]的子函数: %d %p 结束", func, i+1, func->p[i]);
        }
    }
}

static void DumpFunction(tio_t *io, luf_func_t* func) {
    // 首先, 输出源文件数据
    // LLOGD("导出函数 %d %d", func->code->len / 4, func->lineinfo->len / 4);
    // LLOGD("导出源文件名 %p %p %d", func, func->source, func->source ? func->source->len : 0);
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    DumpString(io, func->source);
    // 基础数据
    // LLOGD("导出基础数据 %p", func);
    DumpInt(io, func->linedefined);
    DumpInt(io, func->lastlinedefined);
    DumpByte(io, func->numparams);
    DumpByte(io, func->is_vararg);
    DumpByte(io, func->maxstacksize);
    // 输出代码
    // LLOGD("导出代码 %p", func);
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    DumpCode(io, func);
    // 输出常量表
    // LLOGD("导出常量表 %p", func);
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    DumpConstants(io, func);
    
    // 输出upvalues
    // LLOGD("导出upvalues %p", func);
    // LLOGD("%s:%-4d当前位置 0x%08X", __FILE__, __LINE__, (int)(io->ptr - io->begin));
    DumpUpvalues(io, func);
    // 输出子函数
    // LLOGD("导出子函数 %p %d", func, func->sizep);
    DumpProtos(io, func);

    // 输出调试信息
    // LLOGD("导出调试信息 %p", func);
    DumpDebug(io, func);
}

void luat_luf_toluac(luac_file_t *cf) {
    LLOGD("验证文件 %s", cf->source_file);
    // 首先, 创建缓冲区
    char* buff = luat_heap_malloc(cf->fileSize * 8);
    memset(buff, 0, cf->fileSize + 1);
    tio_t tio = {.ptr = buff, .begin = buff};
    // 然后, 逐步合成luac文件

    // 1. 写入文件头
    DumpBlock(&tio, LUA_SIGNATURE, 4);
    DumpByte(&tio, LUAC_VERSION);
    DumpByte(&tio, LUAC_FORMAT);
    DumpBlock(&tio, LUAC_DATA, 6);
    DumpByte(&tio, sizeof(int));
    DumpByte(&tio, sizeof(size_t));
    DumpByte(&tio, sizeof(Instruction));
    DumpByte(&tio, sizeof(lua_Integer));
    DumpByte(&tio, sizeof(lua_Number));
    DumpInteger(&tio, LUAC_INT);
    DumpNumber(&tio, LUAC_NUM);

    // 还有个 sizeofupvalues
    DumpByte(&tio, cf->report->luf_funcs[0].sizeupvalues);
    
    // 导出函数
    DumpFunction(&tio, &cf->report->luf_funcs[0]);

    // 进行最后修正

    // 对比数据, 校验差异
    int diff = 0;
    for (size_t i = 0; i < cf->fileSize; i++)
    {
        if (buff[i] != cf->ptr[i])
        {
            diff= i;
            break;
        }
    }
    
    // LLOGD("当前差距[%s] %08X %08X %08X", cf->source_file, diff, (int)(tio.ptr - buff), cf->fileSize);
    if (diff > 0)
    {
        LLOGD("luac/luf不一致");
        char tmppath[256];
        sprintf(tmppath, "tmp\\%s", cf->source_file);
        FILE* fp = fopen(tmppath, "wb+");
        if (fp) {
            LLOGD("输出luac数据");
            fwrite(cf->ptr, cf->fileSize, 1, fp);
            fclose(fp);
        }
        else {
            LLOGE("无法写入文件: %s", tmppath);
        }
        sprintf(tmppath, "tmp\\%s.luf", cf->source_file);
        fp = fopen(tmppath, "wb+");
        if (fp) {
            LLOGD("输出luf数据");
            fwrite(buff, cf->fileSize, 1, fp);
            fclose(fp);
        }
        else {
            LLOGE("无法写入文件: %s", tmppath);
        }
    }
    else {
        // LLOGD("luac/luf一致");
    }

    // 完成输出
}

