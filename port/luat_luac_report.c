
#include "luat_base.h"
#include "luat_luadb2.h"
#include "luat_malloc.h"
#include "lundump.h"

#define LUAT_LOG_TAG "luat.luac"
#include "luat_log.h"


typedef struct luac_data {
    size_t i; // 文件索引
    char* data;
    size_t len;
    char mark;
}luac_data_t;

typedef struct luac_report {
    // 预留1M个字符串空间
    luac_data_t strs[32*1024];
    size_t str_count;

    // 函数空间 128k 个, 数据部分是函数名, 如果存在的话
    luac_data_t funcs[32*1024];
    size_t func_count;

    
    // 函数代码空间 128k 个
    luac_data_t codes[32*1024];
    size_t code_count;

    // 数值空间 128k 个
    luac_data_t numbers[32*1024];
    size_t number_count;

    // upvalue 空间 128k 个
    luac_data_t upvalues[32*1024];
    size_t upvalue_count;

    // 调试信息中的行数空间 128k 个
    luac_data_t line_numbers[32*1024];
    size_t line_number_count;

    
    // 以下是统计数据

    // 字符串类
    size_t str_max;
    size_t str_hits;
    size_t str_total;
    size_t str_emtrys;

    // 函数类
    size_t func_max;
    size_t func_total;
    // 代码类
    size_t code_max;
    size_t code_total;
    // 数值类
    size_t number_max;
    size_t number_total;
    // upvalue 类
    size_t upvalue_max;
    size_t upvalue_total;
    // 行数类
    // size_t line_number_max;
    // size_t line_number_total;
} luac_report_t;


typedef struct luac_file {
    char source_file[32];
    luac_report_t* report;
    size_t i;
    const char* ptr;
    size_t fileSize;

}luac_file_t;

typedef struct TIO {
    const char* ptr;
}tio_t;


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
      LoadByte(tio); // TODO 记录它
      break;
    case LUA_TNUMFLT:
      LoadNumber(tio);
      break;
    case LUA_TNUMINT:
      LoadInteger(tio);
      break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
      cf->report->strs[cf->report->str_count].data = LoadString(tio, &len);
      cf->report->strs[cf->report->str_count].len = len;
      cf->report->str_count ++;
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
    LoadFunction(cf, tio, cf->report->func_count);
  }
}


static void LoadDebug (tio_t *tio, luac_file_t *cf) {
  int i, n;
  n = LoadInt(tio);

//   LLOGD("debug信息: 行数数据 %04X 当前偏移量 %04X", n, (uint32_t)(tio->ptr - cf->ptr));
  if (n > 0) {
    size_t len = n * sizeof(int);
    cf->report->line_numbers[cf->report->line_number_count].data = luat_heap_malloc(len);
    LoadBlock(tio, cf->report->line_numbers[cf->report->line_number_count].data, len);
    cf->report->line_numbers[cf->report->line_number_count].len = len;
  }
  cf->report->line_number_count ++;


  n = LoadInt(tio);
//   LLOGD("debug信息: 局部变量名 %d", n);
  size_t len;
  for (i = 0; i < n; i++) {
    // 变量名称
    cf->report->strs[cf->report->str_count].data = LoadString(tio, &len);
    cf->report->strs[cf->report->str_count].len = len;
    cf->report->str_count ++;
    LoadInt(tio);
    LoadInt(tio);
  }
  n = LoadInt(tio);
//   LLOGD("debug信息: upvalue变量名 %d", n);
  for (i = 0; i < n; i++) {
    cf->report->strs[cf->report->str_count].data = LoadString(tio, &len);
    cf->report->strs[cf->report->str_count].len = len;
    cf->report->str_count ++;
    // LLOGD("debug信息: upvalue变量名长度 %d", len);
  }
}


static void LoadFunction(luac_file_t *cf, tio_t* tio, size_t id) {
    Proto f2 = {0};
    Proto* f = &f2;
    size_t len = 0;
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

    cf->report->codes[cf->report->code_count].data = LoadCode(tio, &len);
    cf->report->codes[cf->report->code_count].len = len;
    cf->report->code_count ++;
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
    LoadFunction(cf, &tio, cf->report->func_count);

    // LLOGD("分析完成");
    return 0;
}

int luadb_print_report_file(luac_file_t *cf) {
    // LLOGD("\r\n\r\n\r\n");
    LLOGD("==============================================================");
    LLOGD("单文件报告: %s", cf->source_file);
    LLOGD("文件大小: %d byte", cf->fileSize);
    // LLOGD("函数数量: %d", cf->report->code_count);
    // LLOGD("字符串数量: %d", cf->report->str_count);
    // LLOGD("debug信息: 行号数据 %d byte", cf->report->line_number_count);
    // 字符串数据分析
    // 首先, 获取最大长度和总长度
    size_t max_len = 0;
    size_t total_len = 0;
    size_t hits = 0;
    size_t emtry = 0;
    luac_report_t* rpt = cf->report;
    for (size_t i = 0; i < rpt->str_count; i++) {
        if (rpt->strs[i].len > max_len) {
            max_len = rpt->strs[i].len;
        }
        total_len += rpt->strs[i].len;
        // 使用统计不重复的字符串
        if (rpt->strs[i].len == 0 || rpt->strs[i].len == 1) {
          emtry++;
          continue;
        }
        hits++;
        if (i > 0) {
            for (size_t j = 0; j < i; j++)
            {
                if (rpt->strs[i].len == rpt->strs[j].len && memcmp(rpt->strs[i].data, rpt->strs[j].data, rpt->strs[i].len) == 0) {
                    hits --;
                    break;
                }
            }
        }
    }
    
    LLOGD("字符串: 数量 %8d", cf->report->str_count);
    LLOGD("字符串: 最长 %8d 平均 %8d 总长 %8d", max_len, total_len / (cf->report->str_count > 0 ? cf->report->str_count : 1), total_len);
    LLOGD("字符串: 去重 %8d 占比 %8d%%", hits, hits * 100 / (cf->report->str_count > 0 ? cf->report->str_count : 1));
    
    rpt->str_max = max_len;
    rpt->str_total = total_len;
    rpt->str_hits = hits;
    rpt->str_emtrys = emtry;

    // 函数类统计
    // 函数数量
    // cf->func_count = rpt->func_count;
    // 最大函数大小
    size_t max_code_len = 0;
    size_t total_code_len = 0;
    for (size_t i = 0; i < rpt->code_count; i++) {
        if (rpt->codes[i].len > max_code_len) {
            max_code_len = rpt->codes[i].len;
        }
        total_code_len += (rpt->codes[i].len > 1 ? rpt->codes[i].len - 1 : 0);
    }
    
    LLOGD("函数:   数量 %8d", rpt->code_count);
    LLOGD("函数:   最长 %8d 平均 %8d 总长 %8d", max_code_len, total_code_len / (rpt->code_count > 0 ? rpt->code_count : 1), total_code_len);
    
    rpt->code_max = max_code_len;
    rpt->code_total = total_code_len;
    return 0;
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
            luadb_print_report_file(&cfs[i]);
        }
    }

    LLOGD("==============================================================");
    LLOGD("                       分类汇总报告");
    LLOGD("==============================================================\r\n");
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
    LLOGD("| %-30s | %6d | %s |", "total", file_total_len, "总计");
    LLOGD("| %-30s | %6d | %s |", "max", file_max_len, "最大");
    LLOGD("| %-30s | %6d | %s |", "avg", file_total_len / (ctx->fs->filecount > 0 ? ctx->fs->filecount : 1), "平均");
    LLOGD("==============================================================\r\n");

    // 字符串表格
    LLOGD("==============================================================");
    LLOGD("                        字符串统计");
    LLOGD("==============================================================\r\n");
    LLOGD("| 文件名                         | 非空   |   全部 | 最大长度 | 平均长度 | 总长度  | 去重数量  | 占比   |");
    LLOGD("|--------------------------------|--------|--------|----------|----------|---------|-----------|--------|");
    for (size_t i = 0; i < ctx->fs->filecount; i++) {
        if (cfs[i].report) {
            LLOGD("| %-30s | %6d | %6d | %8d | %8d | %7d | %9d | %5d%% |", cfs[i].source_file, cfs[i].report->str_count - cfs[i].report->str_emtrys, cfs[i].report->str_count, cfs[i].report->str_max, cfs[i].report->str_total / (cfs[i].report->str_count > 0 ? cfs[i].report->str_count : 1), cfs[i].report->str_total, cfs[i].report->str_hits, cfs[i].report->str_hits * 100 / (cfs[i].report->str_count > 0 ? cfs[i].report->str_count : 1));
            // 统计全局最长的字符串
            if(cfs[i].report->str_max > report->str_max) {
                report->str_max = cfs[i].report->str_max;
            }
            // 统计全部字符串的数量
            // report->str_count += cfs[i].report->str_count;
            // 统计全部字符串的长度
            report->str_total += cfs[i].report->str_total;
            for (size_t j = 0; j < cfs[i].report->str_count; j++)
            {
              report->strs[report->str_count] = cfs[i].report->strs[j];
              report->str_count++;
            }
            // LLOGD("report->str_count %d", report->str_count);
        }
    }
    // 找出全部字符串中不重复的字符串
    for (size_t i = 0; i < report->str_count; i++)
    {
        if (report->strs[i].len == 0 || report->strs[i].len == 1) {
          report->str_emtrys ++;
          continue;
        }
        report->str_hits ++;
        int hit = 0;
        for (size_t j = 0; j < i; j++)
        {
          if(report->strs[i].len == report->strs[j].len && memcmp(report->strs[i].data, report->strs[j].data, report->strs[i].len) == 0) {
              // 重复的字符串
              report->str_hits --;
              hit = 1;
              break;
          }
        }
        if (hit == 0) {
          // 未重复的字符串
          // LLOGD("不重复的字符串 %s", report->strs[i].data);
        }
    }
    // 汇总
    LLOGD("| %-30s | %6d | %6d | %8d | %8d | %7d | %9d | %5d%% |", "total", report->str_count - report->str_emtrys, report->str_count, report->str_max, report->str_total / (report->str_count > 0 ? report->str_count : 1), report->str_total, report->str_hits, report->str_hits * 100 / (report->str_count > 0 ? report->str_count : 1));
    LLOGD("=============================================================\r\n");

    // 函数统计
    LLOGD("==============================================================");
    LLOGD("                    函数统计");
    LLOGD("==============================================================\r\n");
    LLOGD("| 文件名                         | 数量   | 平均长度 | 总长度  |");
    LLOGD("|--------------------------------|--------|----------|---------|");
    for (size_t i = 0; i < ctx->fs->filecount; i++) {
        if (cfs[i].report) {
            LLOGD("| %-30s | %6d | %8d | %7d |", cfs[i].source_file, cfs[i].report->code_count, cfs[i].report->code_total / (cfs[i].report->code_count > 0 ? cfs[i].report->code_count : 1), cfs[i].report->code_total);
            // 统计全局最长的函数
            if(cfs[i].report->code_max > report->code_max) {
                report->code_max = cfs[i].report->code_max;
            }
            // 统计全部函数的数量
            report->code_count += cfs[i].report->code_count;
            // 统计全部函数的长度
            report->code_total += cfs[i].report->code_total;
        }
    }
    // 汇总
    LLOGD("| %-30s | %6d | %8d | %7d |", "total", report->code_count, report->code_total / (report->code_count > 0 ? report->code_count : 1), report->code_total);
    
    return 0;
}
