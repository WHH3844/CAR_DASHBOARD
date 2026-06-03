#ifndef LCDIF_H
#define LCDIF_H

#include "Std_Types.h"

#include <stdint.h>

#define LCDIF_COLOR_BLACK       0x0000u
#define LCDIF_COLOR_WHITE       0xFFFFu
#define LCDIF_COLOR_RED         0xF800u
#define LCDIF_COLOR_GREEN       0x07E0u
#define LCDIF_COLOR_BLUE        0x001Fu
#define LCDIF_COLOR_YELLOW      0xFFE0u
#define LCDIF_COLOR_CYAN        0x07FFu
#define LCDIF_COLOR_GRAY        0x8410u

/*
 * LCD 抽象层。
 *
 * 对上提供 framebuffer 绘图能力，对下封装 TLI/LTDC 初始化和 RGB565 像素写入。
 * 坐标单位是像素，颜色为 RGB565。所有绘图函数都会在 LCD 未就绪时静默返回，
 * 这样上层任务可以保持周期调用，不必到处判断硬件状态。
 */
Std_ReturnType LcdIf_Init(uint32_t framebuffer);

/* 用指定 RGB565 颜色清满整屏 framebuffer。 */
void LcdIf_Clear(uint16_t color);

/* 填充矩形区域；超出屏幕的部分会被裁剪，不会写越界。 */
void LcdIf_FillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint16_t color);

/* 使用内置 5x7 点阵字体绘制 ASCII 文本，scale 为像素放大倍数。 */
void LcdIf_DrawText(uint32_t x, uint32_t y, const char *text, uint8_t scale, uint16_t color);

/* 绘制无符号十进制整数，用于车速、转速等仪表数字。 */
void LcdIf_DrawU32(uint32_t x, uint32_t y, uint32_t value, uint8_t scale, uint16_t color);

/* 绘制保留两位小数的定点数，例如温湿度 x100 格式。 */
void LcdIf_DrawSignedX100(uint32_t x, uint32_t y, int32_t value, uint8_t scale, uint16_t color);

/* 返回 LCD/TLI 初始化是否成功。 */
uint8_t LcdIf_IsReady(void);

#endif /* LCDIF_H */
