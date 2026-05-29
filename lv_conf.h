/* clang-format off */
#if 1

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (512 * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_DEF_REFR_PERIOD 200  /* ms - slow refresh fine for e-ink */
#define LV_DPI_DEF 300

/*=====================
 * FEATURE CONFIGURATION
 *====================*/
#define LV_DRAW_BUF_ALIGN 4
#define LV_DRAW_BUF_STRIDE_ALIGN 1

/*=======================
 * FONT USAGE
 *=======================*/
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_24

/*=================
 * DRIVERS
 *=================*/
#define LV_USE_LINUX_FBDEV 1

/*==================
 * LOGGING
 *==================*/
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/*=================
 * WIDGETS
 *=================*/
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_OBJ 1
#define LV_USE_LINE 1

#endif /* LV_CONF_H */
#endif /* #if 1 */
