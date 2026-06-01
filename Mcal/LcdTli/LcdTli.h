#ifndef LCD_TLI_H
#define LCD_TLI_H

#include <stdint.h>

/*
 * 800x480 RGB 屏参数来自可正常显示的 RGB_800x480_DEMO。
 * 该屏由 TLI/RGB 输出像素数据，SDRAM 作为 RGB565 帧缓冲。
 */
#define LCD_TLI_WIDTH               800u
#define LCD_TLI_HEIGHT              480u

#define LCD_TLI_HSYNC               10u
#define LCD_TLI_HBP                 150u
#define LCD_TLI_HFP                 15u
#define LCD_TLI_VSYNC               10u
#define LCD_TLI_VBP                 140u
#define LCD_TLI_VFP                 40u

#define LCD_TLI_RGB565_BLACK        0x0000u
#define LCD_TLI_RGB565_WHITE        0xFFFFu
#define LCD_TLI_RGB565_RED          0xF800u
#define LCD_TLI_RGB565_GREEN        0x07E0u
#define LCD_TLI_RGB565_BLUE         0x001Fu
#define LCD_TLI_RGB565_YELLOW       0xFFE0u
#define LCD_TLI_RGB565_CYAN         0x07FFu
#define LCD_TLI_RGB565_MAGENTA      0xF81Fu

uint8_t LcdTli_Init(uint32_t framebuffer);
void LcdTli_BacklightOn(void);
void LcdTli_BacklightOff(void);
void LcdTli_FillColor(uint32_t framebuffer, uint16_t color);
void LcdTli_DrawColorBars(uint32_t framebuffer);
uint32_t LcdTli_GetFrameBufferBytes(void);

#endif /* LCD_TLI_H */
