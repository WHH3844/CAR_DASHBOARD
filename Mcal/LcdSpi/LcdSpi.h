#ifndef LCD_SPI_H
#define LCD_SPI_H

#include <stdint.h>

/*
 * 梁山派扩展板 demo 使用 240x280 SPI 小屏。
 * 当前 PCB 没有单独引出 LCD_DC，所以驱动使用 9-bit 串行写法：
 * 第 1 bit 为命令/数据标志，0 表示命令，1 表示数据。
 */
#define LCD_SPI_WIDTH               240u
#define LCD_SPI_HEIGHT              280u

#define LCD_SPI_RGB565_BLACK        0x0000u
#define LCD_SPI_RGB565_WHITE        0xFFFFu
#define LCD_SPI_RGB565_RED          0xF800u
#define LCD_SPI_RGB565_GREEN        0x07E0u
#define LCD_SPI_RGB565_BLUE         0x001Fu
#define LCD_SPI_RGB565_YELLOW       0xFFE0u
#define LCD_SPI_RGB565_CYAN         0x07FFu
#define LCD_SPI_RGB565_MAGENTA      0xF81Fu

void LcdSpi_Init(void);
void LcdSpi_PanelInitOnly(void);
void LcdSpi_BacklightOn(void);
void LcdSpi_BacklightOff(void);
void LcdSpi_FillColor(uint16_t color);
void LcdSpi_DrawColorBars(void);

#endif /* LCD_SPI_H */
