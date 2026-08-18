/*******************************************************************************
 * Size: 25 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef ARROW_ROTATE_RIGHT
#define ARROW_ROTATE_RIGHT 1
#endif

#if ARROW_ROTATE_RIGHT

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+F01E "飥? */
    0x0, 0x0, 0x0, 0x1, 0xfc, 0x1c, 0xf, 0xfe,
    0x38, 0x3f, 0xff, 0x70, 0xf0, 0x1f, 0xe3, 0xc0,
    0x1f, 0xcf, 0x0, 0x1f, 0x9c, 0x1, 0xff, 0x70,
    0x3, 0xfe, 0xe0, 0x7, 0xfd, 0xc0, 0x0, 0x3,
    0x80, 0x0, 0x7, 0x0, 0x0, 0xe, 0x0, 0x0,
    0x1c, 0x0, 0x0, 0x1c, 0x0, 0x0, 0x3c, 0x0,
    0x0, 0x3c, 0x1, 0xc0, 0x3c, 0x7, 0x80, 0x3f,
    0xff, 0x0, 0x3f, 0xf8, 0x0, 0x1f, 0xc0, 0x0,
    0x0, 0x0, 0x0,

    /* U+F0E2 "飪? */
    0x0, 0x0, 0x1, 0xc1, 0xfc, 0x3, 0x8f, 0xfe,
    0x7, 0x7f, 0xfe, 0xf, 0xf0, 0x1e, 0x1f, 0xc0,
    0x1e, 0x3f, 0x0, 0x1e, 0x7f, 0xc0, 0x1c, 0xff,
    0x80, 0x1d, 0xff, 0x0, 0x38, 0x0, 0x0, 0x70,
    0x0, 0x0, 0xe0, 0x0, 0x1, 0xc0, 0x0, 0x3,
    0x80, 0x0, 0x7, 0x0, 0x0, 0x1c, 0x0, 0x0,
    0x78, 0x3c, 0x1, 0xe0, 0x7c, 0x7, 0x80, 0x7f,
    0xfe, 0x0, 0x3f, 0xf8, 0x0, 0x1f, 0x80, 0x0,
    0x0, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 400, .box_w = 23, .box_h = 23, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 67, .adv_w = 400, .box_w = 23, .box_h = 23, .ofs_x = 1, .ofs_y = -2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xc4
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 61470, .range_length = 197, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t arrow_rotate_right = {
#else
lv_font_t arrow_rotate_right = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 23,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if ARROW_ROTATE_RIGHT*/
