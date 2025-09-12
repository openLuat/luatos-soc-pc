#ifdef LUAT_USE_GUI

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "luat_base.h"
#include "GT5SLCD2E_1A.h"

#include <ft2build.h>
#include FT_FREETYPE_H

// PC 仿真实现（FreeType 渲染）：
// - get_font: 用 FreeType 渲染到 8bpp，再阈值打包为 1bpp
// - get_Font_Gray: 用 FreeType 渲染到 8bpp，再量化为 2/4 阶灰度并打包
// - GT_Font_Init: 初始化 FreeType，加载字体文件
// - U2G: 为兼容 UTF8 流程，直接返回 unicode（等同于“U->G”的恒等映射）

static inline uint32_t min_u32(uint32_t a, uint32_t b) { return a < b ? a : b; }

static FT_Library s_ft_lib = NULL;
static FT_Face    s_ft_face = NULL;
static int        s_ft_ok   = 0;

static int file_accessible(const char* path) {
    if (!path || !*path) return 0;
    FILE* fp = fopen(path, "rb");
    if (fp) { fclose(fp); return 1; }
    return 0;
}

static const char* pick_default_font(void) {
#ifdef _WIN32
    static const char* cands[] = {
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/mingliu.ttc",
        NULL
    };
#elif __APPLE__
    static const char* cands[] = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Medium.ttc",
        "/System/Library/Fonts/PingFang.ttc",
        NULL
    };
#else
    static const char* cands[] = {
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        NULL
    };
#endif
    for (int i = 0; cands[i]; i++)
        if (file_accessible(cands[i])) return cands[i];
    return NULL;
}

int GT_Font_Init(void) {
    if (s_ft_ok) return 1;
    if (FT_Init_FreeType(&s_ft_lib)) return 0;
    const char* font_file = getenv("LUAT_GTFONT_FILE");
    if (!file_accessible(font_file)) font_file = pick_default_font();
    if (!font_file) return 0;
    if (FT_New_Face(s_ft_lib, font_file, 0, &s_ft_face)) return 0;
    s_ft_ok = 1;
    return 1;
}

// 将 (x,y) 位置对应的像素置 1（1bpp，MSB-first）
static inline void set_bit_1bpp(uint8_t* buf, uint32_t w, uint32_t x, uint32_t y) {
    uint32_t bytes_per_row = (w + 7) / 8;
    uint32_t byte_index = y * bytes_per_row + (x / 8);
    uint8_t  bit_pos = 7 - (x % 8);
    buf[byte_index] |= (uint8_t)(1u << bit_pos);
}

// 将 (x,y) 位置对应的像素写入 2bpp（每字节 4 像素，先高 2bit）
static inline void set_pix_2bpp(uint8_t* buf, uint32_t w, uint32_t x, uint32_t y, uint8_t v2) {
    // v2 仅取低 2bit
    v2 &= 0x03;
    uint32_t groups_per_row = (w + 7) / 8; // 与灰度路径的 t 公式保持一致：t = ((w+7)/8) * grade
    uint32_t byte_index_in_row = (x / 4);  // 4 像素/字节
    uint32_t byte_index = y * (groups_per_row * 2) + byte_index_in_row;
    uint8_t shift = (uint8_t)((3 - (x % 4)) * 2); // 先写高位像素
    uint8_t mask = (uint8_t)(0x03u << shift);
    buf[byte_index] = (uint8_t)((buf[byte_index] & ~mask) | ((uint8_t)(v2 << shift)));
}

unsigned int get_font(unsigned char *pBits,
                      unsigned char sty,
                      unsigned long fontCode,
                      unsigned char width,
                      unsigned char height,
                      unsigned char thick) {
    (void)sty; (void)thick;
    if (!pBits || width == 0 || height == 0 || !s_ft_ok) return 0;
    const uint32_t w = width;
    const uint32_t h = height;
    memset(pBits, 0, ((w + 7) / 8) * h);

    // 约定：fontCode 在 PC 仿真中视为 Unicode（UTF8 流程通过 U2G 恒等实现）
    uint32_t codepoint = (uint32_t)fontCode;
    if (FT_Set_Pixel_Sizes(s_ft_face, 0, h)) return 0;
    if (FT_Load_Char(s_ft_face, codepoint, FT_LOAD_RENDER)) return 0;
    FT_GlyphSlot slot = s_ft_face->glyph;
    FT_Bitmap*   bm   = &slot->bitmap;

    // 简单居中到目标框内
    int off_x = 0; // 左侧对齐
    int off_y = 0;
    if ((int)h > (int)bm->rows) off_y = (int)(h - bm->rows) / 2;
    for (int yy = 0; yy < (int)bm->rows; yy++) {
        int dy = off_y + yy;
        if (dy < 0 || dy >= (int)h) continue;
        for (int xx = 0; xx < (int)bm->width; xx++) {
            int dx = off_x + xx;
            if (dx < 0 || dx >= (int)w) continue;
            uint8_t val = bm->buffer[yy * bm->pitch + xx];
            if (val > 127) set_bit_1bpp(pBits, w, (uint32_t)dx, (uint32_t)dy);
        }
    }
    uint32_t adv = (uint32_t)((slot->advance.x + 32) >> 6);
    if (adv > w) adv = w;
    return adv ? adv : (uint32_t)bm->width;
}

static inline void set_pix_gray(uint8_t* buf, uint32_t w, uint32_t x, uint32_t y, uint8_t bpp, uint8_t val) {
    if (bpp == 4) {
        uint32_t bytes_per_row = ((w + 7) / 8) * 4; // 等于 ceil(w/2)
        uint32_t byte_index = y * bytes_per_row + (x / 2);
        uint8_t  shift = (uint8_t)((1 - (x % 2)) * 4);
        uint8_t  mask  = (uint8_t)(0x0Fu << shift);
        buf[byte_index] = (uint8_t)((buf[byte_index] & ~mask) | ((uint8_t)(val & 0x0F) << shift));
    } else if (bpp == 2) {
        uint32_t bytes_per_row = ((w + 7) / 8) * 2; // 等于 ceil(w/4)
        uint32_t byte_index = y * bytes_per_row + (x / 4);
        uint8_t  shift = (uint8_t)((3 - (x % 4)) * 2);
        uint8_t  mask  = (uint8_t)(0x03u << shift);
        buf[byte_index] = (uint8_t)((buf[byte_index] & ~mask) | ((uint8_t)(val & 0x03) << shift));
    }
}

unsigned int* get_Font_Gray(unsigned char *pBits,
                            unsigned char sty,
                            unsigned long fontCode,
                            unsigned char fontSize,
                            unsigned char thick) {
    (void)sty; (void)thick;
    static unsigned int re_buff[2];
    if (!pBits || fontSize == 0 || !s_ft_ok) {
        re_buff[0] = 0;
        re_buff[1] = 2;
        return re_buff;
    }
    const uint32_t w = fontSize;
    const uint32_t h = fontSize;

    // 与固件约定：小号用 4 阶，大号用 2 阶
    uint8_t bpp = (fontSize >= 16 && fontSize < 34) ? 4 : 2;
    uint32_t bytes_per_row = ((w + 7) / 8) * bpp;
    memset(pBits, 0, bytes_per_row * h);

    uint32_t codepoint = (uint32_t)fontCode; // 视为 Unicode
    if (FT_Set_Pixel_Sizes(s_ft_face, 0, h)) {
        re_buff[0] = 0; re_buff[1] = bpp; return re_buff;
    }
    if (FT_Load_Char(s_ft_face, codepoint, FT_LOAD_RENDER)) {
        re_buff[0] = 0; re_buff[1] = bpp; return re_buff;
    }
    FT_GlyphSlot slot = s_ft_face->glyph;
    FT_Bitmap*   bm   = &slot->bitmap;

    int off_x = 0;
    int off_y = 0;
    if ((int)h > (int)bm->rows) off_y = (int)(h - bm->rows) / 2;
    for (int yy = 0; yy < (int)bm->rows; yy++) {
        int dy = off_y + yy;
        if (dy < 0 || dy >= (int)h) continue;
        for (int xx = 0; xx < (int)bm->width; xx++) {
            int dx = off_x + xx;
            if (dx < 0 || dx >= (int)w) continue;
            uint8_t val = bm->buffer[yy * bm->pitch + xx]; // 0..255
            if (bpp == 4) {
                uint8_t v4 = (uint8_t)((val * 15 + 127) / 255);
                set_pix_gray(pBits, w, (uint32_t)dx, (uint32_t)dy, 4, v4);
            } else {
                uint8_t v2 = (uint8_t)((val * 3 + 127) / 255);
                set_pix_gray(pBits, w, (uint32_t)dx, (uint32_t)dy, 2, v2);
            }
        }
    }

    uint32_t adv = (uint32_t)((slot->advance.x + 32) >> 6);
    if (adv > w) adv = w;
    re_buff[0] = adv ? adv : (unsigned int)bm->width;
    re_buff[1] = bpp; // 用“阶”表征 bpp（2 或 4）
    return re_buff;
}

void Gray_Process(unsigned char *OutPutData ,int width,int High,unsigned char Grade) {
    (void)OutPutData; (void)width; (void)High; (void)Grade;
    // PC 仿真版：若 get_Font_Gray 已按目标阶写入，可不做任何处理
}

unsigned long U2G(unsigned int unicode) {
    // PC 仿真：直接返回 unicode，实现“恒等映射”，
    // 这样上层的 gt_unicode2gb18030(e) 得到的值仍可被本实现直接识别为 Unicode。
    return (unsigned long)unicode;
}

#endif // LUAT_USE_GUI


