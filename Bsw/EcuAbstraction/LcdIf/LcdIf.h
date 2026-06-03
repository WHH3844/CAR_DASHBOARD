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

Std_ReturnType LcdIf_Init(uint32_t framebuffer);
void LcdIf_Clear(uint16_t color);
void LcdIf_FillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint16_t color);
void LcdIf_DrawText(uint32_t x, uint32_t y, const char *text, uint8_t scale, uint16_t color);
void LcdIf_DrawU32(uint32_t x, uint32_t y, uint32_t value, uint8_t scale, uint16_t color);
void LcdIf_DrawSignedX100(uint32_t x, uint32_t y, int32_t value, uint8_t scale, uint16_t color);
uint8_t LcdIf_IsReady(void);

#endif /* LCDIF_H */
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
#define LCDIF_COLOR_DARK        0x1082u
#define LCDIF_COLOR_PANEL       0x2104u

Std_ReturnType LcdIf_Init(void);
void LcdIf_Clear(uint16_t color);
void LcdIf_FillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint16_t color);
void LcdIf_DrawString(uint32_t x, uint32_t y, const char *text, uint8_t scale, uint16_t color);
void LcdIf_DrawU32(uint32_t x, uint32_t y, uint32_t value, uint8_t scale, uint16_t color);
void LcdIf_DrawSigned(uint32_t x, uint32_t y, int32_t value, uint8_t scale, uint16_t color);
uint8_t LcdIf_IsReady(void);

#endif /* LCDIF_H */
