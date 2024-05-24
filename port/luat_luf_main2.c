#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_timer.h"
#include "luat_luadb2.h"
#include "luat_luac_report.h"
#include "lundump.h"
#include "luat_luf.h"

#include "stdio.h"
#include "stdlib.h"

#define LUAT_LOG_TAG "luf"
#include "luat_log.h"

typedef struct luf_ctx {
    luac_data_group_t *shrstrs;
    luac_data_group_t *lngstrs;
    luac_data_group_t *numbers;
    luac_data_group_t *ints;
} luf_ctx_t;

static int luac_data_equal(const luac_data_t *a, const luac_data_t *b) {
    if (a == b) {
        return 1;
    }
    if (a == NULL || b == NULL) {
        return 0;
    }
    if (a->type != b->type) {
        // LLOGD("类型不同: %d, %d", a->type, b->type);
        return memcmp(a->data, b->data, a->len - 1);
    }
    if (a->type == LUA_TSTRING || a->type == LUA_TLNGSTR) {
        if (a->len != b->len) {
            return 0;
        }
        if (memcmp(a->data, b->data, a->len)) {
            return 0;
        }
        return 1;
    }
    lu_byte type = a->type;
    if (type == LUA_TNUMFLT) {
        if (a->nnum == b->nnum) {
            return 1;
        }
        return 0;
    }
    if (type == LUA_TNUMINT) {
        if (a->nint == b->nint) {
            return 1;
        }
        return 0;
    }
    LLOGE("未知类型: %d", a->type);
    return 0;
}

int luac_data_cmp(const luac_data_t *a, const luac_data_t *b) {
    if (luac_data_equal(a, b)) {
        return 0;
    }
    if (a->len > b->len) {
        return 1;
    }
    if (a->len < b->len) {
        return -1;
    }
    if (a->type == LUA_TSTRING || a->type == LUA_TLNGSTR) {
        return memcmp(a->data, b->data, a->len); // 字符串暂不排序
    }
    lu_byte type = a->type;
    if (type == LUA_TNUMFLT) {
        if (a->nnum == b->nnum) {
            return 0;
        }
        if (a->nnum > b->nnum) {
            return 1;
        }
        return -1;
    }
    if (type == LUA_TNUMINT) {
        if (a->nint == b->nint) {
            return 0;
        }
        if (a->nint > b->nint) {
            return 1;
        }
        return -1;
    }
    return 0;
}

static int luac_data_find_in_group(luac_data_group_t *pool, luac_data_t *item) {
    for (size_t i = 0; i < pool->count; i++)
    {
        if (luac_data_equal(&pool->data[i], item)) {
            return i;
        }
    }
    // LLOGE("找不到数据!!!: %d", item->type);
    return -1;
}

static int luac_data_find(luf_ctx_t *ctx, luac_data_t *item) {
    if (item == NULL) {
        LLOGE("查找一个空的luac_data_t");
        return 0;
    }
    lu_byte type = item->type;
    switch (type)
    {
    case LUA_TSTRING:
        return luac_data_find_in_group(ctx->shrstrs, item);
    case LUA_TLNGSTR:
        return luac_data_find_in_group(ctx->lngstrs, item);
    case LUA_TNUMFLT:
        return luac_data_find_in_group(ctx->numbers, item);
    case LUA_TNUMINT:
        return luac_data_find_in_group(ctx->ints, item);
    default:
        break;
    }
    LLOGE("不认识的luac_data_t类型: %d", type);
    return -1;
}

static void add2pool(luac_data_group_t *pool, luac_data_group_t *item, size_t count, lu_byte etype) {
    int match = 0;
    size_t j = 0;
    lu_byte type = 0;
    for (size_t i = 0; i < count; i++) {
        match = 0;
        type = item->data[i].type;
        if (etype != type) {
            continue; // 不同的类型不比较
        }
        if (type == LUA_TNUMINT && 0 == (0x7F000000 & item->data[i].nint)) {
            // 忽略小于 2^24 的整数
            continue;
        }
        for (j = 0; j < pool->count + 1; j++)
        {
            if (pool->data[j].type == 0) {
                // 新的元素
                break;
            }
            if (type == LUA_TSTRING || type == LUA_TLNGSTR) {
                if (pool->data[j].len == item->data[i].len) {
                    if (memcmp(pool->data[j].data, item->data[i].data, pool->data[j].len) == 0) {
                        match = 1;
                        break;
                    }
                }
            }
            else if (type == LUA_TNUMFLT) {
                if (pool->data[j].nnum == item->data[i].nnum) {
                    match = 1;
                    break;
                }
            }
            else if (type == LUA_TNUMINT) {
                if (pool->data[j].nint == item->data[i].nint) {
                    match = 1;
                    break;
                }
            }
            else {
                LLOGE("未知类型: %d", item->data[i].type);
            }
        }
        if (!match) {
            memcpy(&pool->data[j], &item->data[i], sizeof(luac_data_t));
            pool->count++;
        }
    }
}

void luac_to_luf(luac_file_t *cfs, size_t count, luac_report_t *rpt) {
    int ret = 0;
    // 转换luac文件到luf文件

    // 字符串池
    luac_data_group_t *shrstrs = luat_heap_malloc(sizeof(luac_data_group_t));
    memset(shrstrs, 0, sizeof(luac_data_group_t));
    add2pool(shrstrs, &rpt->strs, rpt->strs.count, LUA_TSTRING);
    LLOGD("短字符串池: %10d", shrstrs->count);
    luac_data_group_t *lngstrs = luat_heap_malloc(sizeof(luac_data_group_t));
    memset(lngstrs, 0, sizeof(luac_data_group_t));
    add2pool(lngstrs, &rpt->strs, rpt->strs.count, LUA_TLNGSTR);
    LLOGD("长字符串池: %10d", lngstrs->count);

    // TODO 生成函数池

    // 整数和浮点数池
    luac_data_group_t *ints = luat_heap_malloc(sizeof(luac_data_group_t));
    memset(ints, 0, sizeof(luac_data_group_t));
    add2pool(ints,   &rpt->numbers, rpt->numbers.count, LUA_TNUMINT);
    LLOGD("整数池:     %10d", ints->count);

    luac_data_group_t *floats = luat_heap_malloc(sizeof(luac_data_group_t));
    memset(floats, 0, sizeof(luac_data_group_t));
    add2pool(floats, &rpt->numbers, rpt->numbers.count, LUA_TNUMFLT);
    LLOGD("浮点数池:   %10d", floats->count);

    // 对上述的pool进行排序, 以便查找
    qsort(&shrstrs->data[0], shrstrs->count, sizeof(luac_data_t), luac_data_cmp);
    qsort(&lngstrs->data[0], lngstrs->count, sizeof(luac_data_t), luac_data_cmp);
    qsort(&floats->data[0],  floats->count,  sizeof(luac_data_t), luac_data_cmp);
    qsort(&ints->data[0],    ints->count,    sizeof(luac_data_t), luac_data_cmp);

    // 咱们先验证
    for (size_t i = 0; i < count; i++)
    {
        if (cfs[i].report == NULL)
            continue;// 非脚本文件
        // 遍历一下字符串
        for (size_t j = 0; j < cfs[i].report->strs.count; j++)
        {
            ret = 0;
            if (cfs[i].report->strs.data[j].type == LUA_TLNGSTR) {
                ret = luac_data_find_in_group(lngstrs, &cfs[i].report->strs.data[j]);
            }
            else if (cfs[i].report->strs.data[j].type == LUA_TSTRING && cfs[i].report->strs.data[j].len > 0) {
                ret = luac_data_find_in_group(shrstrs, &cfs[i].report->strs.data[j]);
            }
            if (ret < 0) {
                // LLOGD("字符串长度 %d", cfs[i].report->strs.data[j].len);
            }
        }
        // 遍历一下数值
        for (size_t j = 0; j < cfs[i].report->numbers.count; j++)
        {
            if (cfs[i].report->numbers.data[j].type == LUA_TNUMFLT) {
                ret = luac_data_find_in_group(floats, &cfs[i].report->numbers.data[j]);
                if (ret < 0)
                    LLOGD("找不到浮点数??");
            }
            else if (cfs[i].report->numbers.data[j].type == LUA_TNUMINT){
                if (0 == (0x7F000000 & cfs[i].report->numbers.data[j].nint)) {
                    continue; // 存储优化, 小于0x7F的整数, 直接存储
                }
                ret = luac_data_find_in_group(ints, &cfs[i].report->numbers.data[j]);
                if (ret < 0)
                    LLOGD("找不到整数?? %d", cfs[i].report->numbers.data[j].nint);
            }
            else {
                // 还有布尔值, 但不需要检查
            }
        }
    }

    // 打印一下字符串, 看看顺序对不对
    #if 0
    for (size_t i = 0; i < shrstrs->count; i++)
    {
        if (shrstrs->data[i].len > 1)
            LLOGD("短字符串: %.*s", shrstrs->data[i].len - 1, shrstrs->data[i].data);
    }
    #endif

    // 逐个函数转换luf格式
    luf_func_head_t fhead = {0};
    luf_func_t* func = NULL;
    size_t luf_file_size = 0;
    size_t luf_size_count = 0;
    size_t luac_size_count = 0;
    for (size_t i = 0; i < count; i++)
    {
        if (cfs[i].report == NULL)
            continue;// 非脚本文件
        luf_file_size = 0;
        for (size_t j = 0; j < cfs[i].report->func_count; j++) {
            func = &cfs[i].report->luf_funcs[j];
            // 填充数据
            fhead.sizecode = func->sizecode;
            fhead.sizek = func->sizek;
            fhead.sizep = func->sizep;
            fhead.sizeupvalues = func->sizeupvalues;
            fhead.sizeupvalues2 = func->sizeupvalues2;
            fhead.linedefined = func->linedefined;
            fhead.lastlinedefined = func->lastlinedefined;
            fhead.numparams = func->numparams;
            fhead.is_vararg = func->is_vararg;
            fhead.maxstacksize = func->maxstacksize;
            fhead.sizelineinfo = func->sizelineinfo;
            fhead.sizelocvars = func->sizelocvars;

            luf_file_size += sizeof(luf_func_head_t);
            luf_file_size += 4 * fhead.sizecode;
            luf_file_size += 4 * fhead.sizek;
            luf_file_size += 4 * fhead.sizep;
            luf_file_size += 4 * fhead.sizelineinfo;
            luf_file_size += 4 * fhead.sizelocvars + 8 * fhead.sizelocvars;
            luf_file_size += 2 * fhead.sizeupvalues;
            luf_file_size += 4 * fhead.sizeupvalues2;
        }
        // LLOGD("luac文件映射到luf的大小: %10d -> %10d %s", cfs[i].fileSize, luf_file_size, cfs[i].source_file);
        luf_size_count += luf_file_size;
        luac_size_count += cfs[i].fileSize;
    }
    LLOGD("函数区总大小 luac -> luf %10d -> %10d", luac_size_count, luf_size_count);

    // 计算各种池的占用情况
    // 字符串池
    // LLOGD("shrstrs: %10d", shrstrs->count);
    for (size_t i = 0; i < shrstrs->count; i++) {
        if (shrstrs->data[i].len > 1) {
            luf_size_count += (shrstrs->data[i].len + 3) & 0xfffffffc;
            luf_size_count += 16; // 字符串长度
        }
    }
    for (size_t i = 0; i < lngstrs->count; i++) {
        if (lngstrs->data[i].len > 1) {
            luf_size_count += (lngstrs->data[i].len + 3) & 0xfffffffc;
            luf_size_count += 16; // 字符串长度
        }
    }
    // 浮点数池
    // LLOGD("floats: %10d", floats->count);
    for (size_t i = 0; i < floats->count; i++) {
        luf_size_count += 4;
    }
    // 整数池
    // LLOGD("ints: %10d", ints->count);
    for (size_t i = 0; i < ints->count; i++) {
        luf_size_count += 4;
    }

    // 最后是一个估算头部, 假设1k吧
    luf_size_count += 1024;

    LLOGD("总大小       luac -> luf %10d -> %10d   %.2f%%", luac_size_count, luf_size_count, luf_size_count * 100.0 / luac_size_count);

    // 输出整体文件
}

