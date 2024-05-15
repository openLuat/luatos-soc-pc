#ifndef LUAT_LUAC_REPORT_T
#define LUAT_LUAC_REPORT_T

typedef struct luac_data {
    size_t i; // 文件索引
    union 
    {
      char* data;
      lua_Number nnum;
      lua_Integer nint;
      lu_byte nbyte;
    };
    size_t len;
    lu_byte mark;
    lu_byte type;
}luac_data_t;

typedef struct luac_data_group {
  luac_data_t data[32*1024];
  size_t count;
  size_t max_len;
  size_t total_len;
  size_t uni_count; // 去重后的数量
  size_t emtrys;
} luac_data_group_t;

typedef struct luac_report {
  luac_data_group_t strs;
  luac_data_group_t funcs;
  luac_data_group_t codes;
  luac_data_group_t numbers;
  luac_data_group_t upvalues;
  luac_data_group_t line_numbers;
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


#endif

