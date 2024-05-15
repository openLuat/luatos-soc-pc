
#include "luat_base.h"
#include "luat_luadb2.h"
#include "luat_malloc.h"
#include "lundump.h"

#define LUAT_LOG_TAG "luat.luac"
#include "luat_log.h"


#include "luat_luac_report.h"

static void LoadFunction(luac_file_t *cf, tio_t* tio, size_t id);

static void LoadBlock (tio_t* t, void *b, size_t size) {
  memcpy(b, t->ptr, size);
  t->ptr += size;
}

static int LoadInt(tio_t *tio) {
    int t = 0;
    LoadBlock(tio, &t, sizeof(int));
    return t;
}
static lu_byte LoadByte(tio_t *tio) {
    lu_byte t = 0;
    LoadBlock(tio, &t, sizeof(lu_byte));
    return t;
}

static lua_Number LoadNumber (tio_t *tio) {
  lua_Number x;
  LoadBlock(tio, &x, sizeof(lua_Number));
  return x;
}


static lua_Integer LoadInteger (tio_t *tio) {
  lua_Integer x;
  LoadBlock(tio, &x, sizeof(lua_Integer));
  return x;
}

static char* LoadString(tio_t *tio, size_t* len2) {
    size_t len = LoadByte(tio);
    char* buff;
    if (len == 0) {
        // LLOGD("字符串长度为0");
        *len2 = 0;
        return NULL;
    }
    if (len == 0xFF)
        len = LoadInt(tio);
    buff = luat_heap_malloc(len);
    memset(buff, 0, len);
    if (len > 1) {
        LoadBlock(tio, buff, len - 1);
    }
    *len2 = len;
    // LLOGD("载入字符串 %d %.*s", len, len, buff);
    // LLOGD("载入字符串 %02X%02X%02X%02X", buff[0], buff[1], buff[2]);
    return buff;
}

static char* LoadCode(tio_t *tio, size_t* len2) {
    size_t len = LoadInt(tio) * sizeof(Instruction);
    // LLOGD("载入code 长度 %d", len);
    char* buff = luat_heap_malloc(len);
    memcpy(buff, tio->ptr, len);
    tio->ptr += len;
    *len2 = len;
    return buff;
}

static void LoadConstants (tio_t *tio, luac_file_t* cf) {
  (void)cf;
  int i;
  int n = LoadInt(tio);
  luac_report_t* rpt = cf->report;
//   LLOGD("常数数量 %d", n);
  size_t len = 0;
  for (i = 0; i < n; i++) {
    // TValue *o = &f->k[i];
    int t = LoadByte(tio);
    // LLOGD("载入常数 类型%d", t);
    switch (t) {
    case LUA_TNIL:
    //   setnilvalue(o);
      break;
    case LUA_TBOOLEAN:
    //   setbvalue(o, LoadByte(S));
      // LoadByte(tio); // TODO 记录它
      rpt->numbers.data[rpt->numbers.count].nint = LoadByte(tio);;
      rpt->numbers.data[rpt->numbers.count].len = sizeof(lua_Integer);
      rpt->numbers.data[rpt->numbers.count].mark = LUA_TBOOLEAN;
      rpt->numbers.data[rpt->numbers.count].type = LUA_TBOOLEAN;
      rpt->numbers.count ++;
      break;
    case LUA_TNUMFLT:
      // LoadNumber(tio);
      rpt->numbers.data[rpt->numbers.count].nnum = LoadNumber(tio);;
      rpt->numbers.data[rpt->numbers.count].len = sizeof(lua_Integer);
      rpt->numbers.data[rpt->numbers.count].mark = LUA_TNUMFLT;
      rpt->numbers.data[rpt->numbers.count].type = LUA_TNUMFLT;
      rpt->numbers.count ++;
      break;
    case LUA_TNUMINT:
      // LoadInteger(tio);
      rpt->numbers.data[rpt->numbers.count].nint = LoadInteger(tio);
      rpt->numbers.data[rpt->numbers.count].len = sizeof(lua_Integer);
      rpt->numbers.data[rpt->numbers.count].mark = 1;
      rpt->numbers.data[rpt->numbers.count].type = t;
      rpt->numbers.count ++;
      break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
      rpt->strs.data[rpt->strs.count].data = LoadString(tio, &len);
      rpt->strs.data[rpt->strs.count].len = len;
      rpt->strs.data[rpt->strs.count].type = t;
      rpt->strs.count ++;
      break;
    default:
      LLOGD("不对劲");
    //   lua_assert(0);
      return;
    }
  }
}

static void LoadUpvalues (tio_t *tio, luac_file_t* cf) {
  (void)cf;
  int i, n;
  n = LoadInt(tio);
//   LLOGD("upvalue数量 %d", n);
  for (i = 0; i < n; i++) {
    // f->upvalues[i].instack = LoadByte(S);
    // f->upvalues[i].idx = LoadByte(S);
    LoadByte(tio);
    LoadByte(tio);
  }
}

static void LoadProtos (tio_t *tio, luac_file_t *cf) {
  int i;
  int n = LoadInt(tio);
//   LLOGD("子函数数量 %d", n);
  for (i = 0; i < n; i++) {
    LoadFunction(cf, tio, cf->report->funcs.count);
  }
}


static void LoadDebug (tio_t *tio, luac_file_t *cf) {
  int i, n;
  n = LoadInt(tio);
  luac_report_t* rpt = cf->report;
//   LLOGD("debug信息: 行数数据 %04X 当前偏移量 %04X", n, (uint32_t)(tio->ptr - cf->ptr));
  if (n > 0) {
    size_t len = n * sizeof(int);
    rpt->line_numbers.data[rpt->line_numbers.count].data = luat_heap_malloc(len);
    LoadBlock(tio, rpt->line_numbers.data[rpt->line_numbers.count].data, len);
    rpt->line_numbers.data[rpt->line_numbers.count].len = len;
  }
  rpt->line_numbers.count ++;


  n = LoadInt(tio);
//   LLOGD("debug信息: 局部变量名 %d", n);
  size_t len;
  for (i = 0; i < n; i++) {
    // 变量名称
    rpt->strs.data[rpt->strs.count].data = LoadString(tio, &len);
    rpt->strs.data[rpt->strs.count].len = len;
    rpt->strs.count ++;
    LoadInt(tio);
    LoadInt(tio);
  }
  n = LoadInt(tio);
//   LLOGD("debug信息: upvalue变量名 %d", n);
  for (i = 0; i < n; i++) {
    rpt->strs.data[rpt->strs.count].data = LoadString(tio, &len);
    rpt->strs.data[rpt->strs.count].len = len;
    rpt->strs.count ++;
    // LLOGD("debug信息: upvalue变量名长度 %d", len);
  }
}


static void LoadFunction(luac_file_t *cf, tio_t* tio, size_t id) {
    Proto f2 = {0};
    Proto* f = &f2;
    size_t len = 0;

    luac_report_t* rpt = cf->report;

    // char* tmp = NULL;
    LoadString(tio, &len);
    // f->source = LoadString(S, f);
    // if (f->source == NULL)  /* no source in dump? */
    //     f->source = psource;  /* reuse parent's source */
    f->linedefined = LoadInt(tio);
    f->lastlinedefined = LoadInt(tio);
    f->numparams = LoadByte(tio);
    f->is_vararg = LoadByte(tio);
    f->maxstacksize = LoadByte(tio);

    rpt->codes.data[rpt->codes.count].data = LoadCode(tio, &len);
    rpt->codes.data[rpt->codes.count].len = len;
    rpt->codes.count ++;
    LoadConstants(tio, cf);
    LoadUpvalues(tio, cf);
    LoadProtos(tio, cf);
    LoadDebug(tio, cf);
}

int luadb_do_report_file(luac_file_t *cf, const char* data) {
    LLOGD("分析文件%s %p", cf->source_file, data);
    // char tmpbuff[8] = {0};
    const char* ptr = data;
    // 首先, 分析头部
    // 前4个字节是 \x1bLua
    if (memcmp(ptr, LUA_SIGNATURE, 4) != 0) {
        LLOGE("文件%s不是lua文件 %02X%02X%02X%02X", cf->source_file, data[0], data[1], data[2], data[3]);
        return -1;
    };
    ptr += 4;

    // 分析版本号, 直接跳过
    ptr += 2;

    // 检查LUAC_DATA
    if (memcmp(ptr, LUAC_DATA, strlen(LUAC_DATA)) != 0) {
        LLOGE("文件%s不是luac文件 LUAC_DATA不正确 %02X%02X%02X%02X", cf->source_file, ptr[0], ptr[1], ptr[2], ptr[3]);
        return -1;
    };
    ptr += strlen(LUAC_DATA);

    // checksize(S, int);
    // checksize(S, size_t);
    // checksize(S, Instruction);
    // checksize(S, lua_Integer);
    // checksize(S, lua_Number);
    ptr += 5;
    lua_Integer t = 0;
    lua_Number n = 0;
    memcpy(&t, ptr, sizeof(lua_Integer));
    if (t != LUAC_INT) {
        LLOGE("endianness mismatch in");
        return -1;
    }
    ptr += sizeof(lua_Integer);

    memcpy(&n, ptr, sizeof(lua_Number));
    if (n != LUAC_NUM) {
        LLOGE("float format mismatch in");
        return -3;
    }
    ptr += sizeof(lua_Number);

    // 还有一个cl值
    ptr ++;

    
    tio_t tio = {.ptr = ptr};
    LoadFunction(cf, &tio, cf->report->funcs.count);

    // LLOGD("分析完成");
    return 0;
}

static void do_data_group_report(luac_data_group_t *dg) {
    size_t hits = 0;
    for (size_t i = 0; i < dg->count; i++) {
        if (dg->data[i].len > dg->max_len) {
            dg->max_len = dg->data[i].len;
        }
        dg->total_len += dg->data[i].len;
        // 使用统计不重复的字符串
        if (dg->data[i].len == 0 || dg->data[i].len == 1) {
          dg->emtrys++;
          continue;
        }
        hits++;
        if (i > 0 && dg->data->mark == 0) {
            for (size_t j = 0; j < i; j++)
            {
                if (dg->data[i].len == dg->data[j].len && memcmp(dg->data[i].data, dg->data[j].data, dg->data[i].len) == 0) {
                    hits --;
                    break;
                }
            }
        }
    }
    dg->uni_count = hits;
}

void luac_data_group_add(luac_data_group_t *src, luac_data_group_t* dst) {
  for (size_t i = 0; i < src->count; i++) {
    dst->data[dst->count] = src->data[i];
    dst->count++;
  }
}

int luac_data_report_exec(luac_report_t *rpt, luac_report_t* up_rpt) {
    // 逐项数据分析
    do_data_group_report(&rpt->strs);
    do_data_group_report(&rpt->codes);
    do_data_group_report(&rpt->funcs);
    do_data_group_report(&rpt->upvalues);
    do_data_group_report(&rpt->numbers);
    do_data_group_report(&rpt->line_numbers);

    // 将数据汇总到上层报告中
    if (up_rpt) {
        // 字符串
        luac_data_group_add(&rpt->strs, &up_rpt->strs);
        luac_data_group_add(&rpt->codes, &up_rpt->codes);
        luac_data_group_add(&rpt->funcs, &up_rpt->funcs);
        luac_data_group_add(&rpt->upvalues, &up_rpt->upvalues);
        luac_data_group_add(&rpt->numbers, &up_rpt->numbers);
        luac_data_group_add(&rpt->line_numbers, &up_rpt->line_numbers);
    }
    return 0;
}

void luac_data_report_print_group(luac_file_t* cfs, size_t filecount, luac_report_t *rpt, size_t offset, const char* title) {
    luac_data_group_t* dg = NULL;
    // 表格
    LLOGD("=========================================================================================================");
    LLOGD("                        %s", title);
    LLOGD("=========================================================================================================\r\n");
    LLOGD("| 文件名                         | 非空   |   全部 | 最大长度 | 平均长度 | 总长度  | 去重数量  | 占比   |");
    LLOGD("|--------------------------------|--------|--------|----------|----------|---------|-----------|--------|");
    for (size_t i = 0; i < filecount; i++) {
        if (cfs[i].report) {
            dg = &cfs[i].report->strs + (offset);
            LLOGD("| %-30s | %6d | %6d | %8d | %8d | %7d | %9d | %5d%% |", cfs[i].source_file, dg->count - dg->emtrys, dg->count, dg->max_len, dg->total_len / (dg->count > 0 ? dg->count : 1), dg->total_len, dg->uni_count, dg->uni_count * 100 / (dg->count > 0 ? dg->count : 1));
        }
    }
    // 汇总
    dg = &rpt->strs + (offset);
    LLOGD("| %-30s | %6d | %6d | %8d | %8d | %7d | %9d | %5d%% |", "==total==", dg->count - dg->emtrys, dg->count, dg->max_len, dg->total_len / (dg->count > 0 ? dg->count : 1), dg->total_len, dg->uni_count, dg->uni_count * 100 / (dg->count > 0 ? dg->count : 1));
    // LLOGD("总长度与去重长度对比: %d %d --> %d%%", report->strs.total_len, str_noemtry_len, str_noemtry_len * 100 / (report->strs.total_len > 0 ? report->strs.total_len : 1));
    LLOGD("=========================================================================================================\r\n");
    
    // LLOGD("子报告的数据数量: %d 指针位置 0x%p", dg->count, dg);
    // LLOGD("子报告的字符串数据: %d 指针位置 0x%p", dg->count, &dg->data);
}

int luadb_do_report(luat_luadb2_ctx_t *ctx) {
    // 加载luadb数据成文件系统
    // 总的report
    luac_report_t *report = luat_heap_malloc(sizeof(luac_report_t));
    memset(report, 0, sizeof(luac_report_t));
    ctx->fs = luat_luadb_mount(ctx->dataptr);
    if (ctx->fs == NULL) {
        LLOGE("加载luadb失败, 无法解析文件系统");
        return -1;
    }
    LLOGD("加载luadb成功, 文件数量: %d", ctx->fs->filecount);
    // 遍历文件系统, 找到所有的luac文件, 解析出数据
    luac_file_t *cfs = luat_heap_malloc(sizeof(luac_file_t) * ctx->fs->filecount);
    memset(cfs, 0, sizeof(luac_file_t) * ctx->fs->filecount);
    luac_report_t *rpts = luat_heap_malloc(sizeof(luac_report_t) * ctx->fs->filecount);
    memset(rpts, 0, sizeof(luac_report_t) * ctx->fs->filecount);
    for (size_t i = 0; i < ctx->fs->filecount; i++)
    {
        
        memcpy(cfs[i].source_file, ctx->fs->files[i].name, strlen(ctx->fs->files[i].name));
        cfs[i].fileSize = ctx->fs->files[i].size;
        if (!strcmp(ctx->fs->files[i].name + strlen(ctx->fs->files[i].name) - 4, ".lua") ||
            !strcmp(ctx->fs->files[i].name + strlen(ctx->fs->files[i].name) - 5, ".luac")
        ) 
        {
            cfs[i].report = &rpts[i];
            cfs[i].ptr = ctx->fs->files[i].ptr;
            luadb_do_report_file(&cfs[i], ctx->fs->files[i].ptr);
        }
        else {
            LLOGD("跳过非lua文件 %s", ctx->fs->files[i].name);
        }
    }
    

    // 打印报告
    LLOGD("==============================================================");
    LLOGD("LuatOS脚本及资源报告 v1");
    LLOGD("报告解读和名称解析: https://wiki.luatos.com/page/report_v1.html");
    LLOGD("==============================================================");
    LLOGD("==============================================================\r\n\r\n");

    // TODO 打印抬头
    memset(report, 0, sizeof(luac_report_t));
    // 打印单文件报告
    for (size_t i = 0; i < ctx->fs->filecount; i++)
    {
        if (cfs[i].report) {
            luac_data_report_exec(cfs[i].report, report);
        }
    }
    luac_data_report_exec(report, NULL);

    // LLOGD("==============================================================");
    // LLOGD("                       分类汇总报告");
    // LLOGD("==============================================================\r\n");
    // 分类报告

    // 文件信息表格
    size_t file_max_len = 0;
    size_t file_total_len = 0;
    LLOGD("==============================================================");
    LLOGD("                        文件信息统计");
    LLOGD("==============================================================\r\n");
    LLOGD("| 文件名                         | 大小   | 类型   |");
    LLOGD("|--------------------------------|--------|--------|");
    for (size_t i = 0; i < ctx->fs->filecount; i++) {
        if (cfs[i].report) {
            LLOGD("| %-30s | %6d | %s |", cfs[i].source_file, cfs[i].fileSize, "脚本");
        }
        else {
            LLOGD("| %-30s | %6d | %s |", cfs[i].source_file, cfs[i].fileSize, "资源");
        }
        // 统计全局最长的文件名
        if(cfs[i].fileSize > file_max_len) {
            file_max_len = cfs[i].fileSize;
        }
        // 统计全部文件的大小
        file_total_len += cfs[i].fileSize;
    }
    LLOGD("| %-30s | %6d | %s |", "==total==", file_total_len, "总计");
    // LLOGD("| %-30s | %6d | %s |", "==max==", file_max_len, "最大");
    // LLOGD("| %-30s | %6d | %s |", "==avg==", file_total_len / (ctx->fs->filecount > 0 ? ctx->fs->filecount : 1), "平均");
    LLOGD("==============================================================\r\n");

    luac_data_report_print_group(cfs, ctx->fs->filecount, report, 0, "字符串/二进制数据统计");
    // luac_data_report_print_group(cfs, ctx->fs->filecount, report, 1, "函数信息统计");
    luac_data_report_print_group(cfs, ctx->fs->filecount, report, 2, "函数代码统计");
    luac_data_report_print_group(cfs, ctx->fs->filecount, report, 3, "常量数值统计");
    // luac_data_report_print_group(cfs, ctx->fs->filecount, report, 4, "upvalue统计");
    // luac_data_report_print_group(cfs, ctx->fs->filecount, report, 5, "代码行数映射信息统计");


    return 0;
}
