#include "LcdIf.h"

#include "Dem.h"
#include "LcdTli.h"
#include "LogM.h"

static uint32_t LcdIf_FrameBuffer;
static uint8_t LcdIf_Ready;

/*
 * 内置 5x7 ASCII 点阵字体。
 * 为了让裸机 bring-up 阶段不依赖字库文件或文件系统，先把仪表需要的数字、
 * 大写字母和少量符号直接放在代码里。
 */
static const uint8_t LcdIf_FontSpace[7] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
static const uint8_t LcdIf_FontDot[7]   = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x04u, 0x04u};
static const uint8_t LcdIf_FontColon[7] = {0x00u, 0x04u, 0x04u, 0x00u, 0x04u, 0x04u, 0x00u};
static const uint8_t LcdIf_FontMinus[7] = {0x00u, 0x00u, 0x00u, 0x1Fu, 0x00u, 0x00u, 0x00u};
static const uint8_t LcdIf_FontSlash[7] = {0x01u, 0x02u, 0x02u, 0x04u, 0x08u, 0x08u, 0x10u};
static const uint8_t LcdIf_FontPercent[7] = {0x18u, 0x19u, 0x02u, 0x04u, 0x08u, 0x13u, 0x03u};

static const uint8_t LcdIf_FontDigits[10][7] =
{
    {0x0Eu, 0x11u, 0x13u, 0x15u, 0x19u, 0x11u, 0x0Eu},
    {0x04u, 0x0Cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu},
    {0x0Eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1Fu},
    {0x1Eu, 0x01u, 0x01u, 0x0Eu, 0x01u, 0x01u, 0x1Eu},
    {0x02u, 0x06u, 0x0Au, 0x12u, 0x1Fu, 0x02u, 0x02u},
    {0x1Fu, 0x10u, 0x1Eu, 0x01u, 0x01u, 0x11u, 0x0Eu},
    {0x06u, 0x08u, 0x10u, 0x1Eu, 0x11u, 0x11u, 0x0Eu},
    {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u},
    {0x0Eu, 0x11u, 0x11u, 0x0Eu, 0x11u, 0x11u, 0x0Eu},
    {0x0Eu, 0x11u, 0x11u, 0x0Fu, 0x01u, 0x02u, 0x0Cu}
};

static const uint8_t LcdIf_FontLetters[26][7] =
{
    {0x0Eu, 0x11u, 0x11u, 0x1Fu, 0x11u, 0x11u, 0x11u},
    {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x11u, 0x11u, 0x1Eu},
    {0x0Eu, 0x11u, 0x10u, 0x10u, 0x10u, 0x11u, 0x0Eu},
    {0x1Eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x1Eu},
    {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x1Fu},
    {0x1Fu, 0x10u, 0x10u, 0x1Eu, 0x10u, 0x10u, 0x10u},
    {0x0Eu, 0x11u, 0x10u, 0x17u, 0x11u, 0x11u, 0x0Fu},
    {0x11u, 0x11u, 0x11u, 0x1Fu, 0x11u, 0x11u, 0x11u},
    {0x0Eu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x0Eu},
    {0x01u, 0x01u, 0x01u, 0x01u, 0x11u, 0x11u, 0x0Eu},
    {0x11u, 0x12u, 0x14u, 0x18u, 0x14u, 0x12u, 0x11u},
    {0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x1Fu},
    {0x11u, 0x1Bu, 0x15u, 0x15u, 0x11u, 0x11u, 0x11u},
    {0x11u, 0x19u, 0x15u, 0x13u, 0x11u, 0x11u, 0x11u},
    {0x0Eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Eu},
    {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x10u, 0x10u, 0x10u},
    {0x0Eu, 0x11u, 0x11u, 0x11u, 0x15u, 0x12u, 0x0Du},
    {0x1Eu, 0x11u, 0x11u, 0x1Eu, 0x14u, 0x12u, 0x11u},
    {0x0Fu, 0x10u, 0x10u, 0x0Eu, 0x01u, 0x01u, 0x1Eu},
    {0x1Fu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u},
    {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Eu},
    {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0Au, 0x04u},
    {0x11u, 0x11u, 0x11u, 0x15u, 0x15u, 0x15u, 0x0Au},
    {0x11u, 0x11u, 0x0Au, 0x04u, 0x0Au, 0x11u, 0x11u},
    {0x11u, 0x11u, 0x0Au, 0x04u, 0x04u, 0x04u, 0x04u},
    {0x1Fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x1Fu}
};

static const uint8_t *LcdIf_GetGlyph(char ch)
{
    /*
     * 字符查表只覆盖当前仪表 UI 用到的 ASCII 范围。
     * 未支持字符返回空格，避免显示异常字符时访问越界。
     */
    if ((ch >= '0') && (ch <= '9'))
    {
        return LcdIf_FontDigits[(uint8_t)(ch - '0')];
    }

    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch = (char)(ch - 'a' + 'A');
    }

    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return LcdIf_FontLetters[(uint8_t)(ch - 'A')];
    }

    if (ch == '.')
    {
        return LcdIf_FontDot;
    }
    if (ch == ':')
    {
        return LcdIf_FontColon;
    }
    if (ch == '-')
    {
        return LcdIf_FontMinus;
    }
    if (ch == '/')
    {
        return LcdIf_FontSlash;
    }
    if (ch == '%')
    {
        return LcdIf_FontPercent;
    }

    return LcdIf_FontSpace;
}

Std_ReturnType LcdIf_Init(uint32_t framebuffer)
{
    /*
     * framebuffer 由 SdramMgr 分配。LcdIf 保存地址后交给 LcdTli 初始化硬件图层，
     * 后续所有 Fill/Draw 操作都直接写这块 RGB565 显存。
     */
    LcdIf_FrameBuffer = framebuffer;

    if (LcdTli_Init(framebuffer) == 0u)
    {
        LcdIf_Ready = 0u;
        Dem_SetEventStatus(DEM_EVENT_LCD_INIT_FAILED, DEM_EVENT_STATUS_FAILED);
        LogM_Error("LCD TLI init failed");
        return E_NOT_OK;
    }

    LcdIf_Ready = 1u;
    Dem_SetEventStatus(DEM_EVENT_LCD_INIT_FAILED, DEM_EVENT_STATUS_PASSED);
    LogM_Info("LCD TLI init ok");
    return E_OK;
}

void LcdIf_Clear(uint16_t color)
{
    if (LcdIf_Ready != 0u)
    {
        LcdTli_FillColor(LcdIf_FrameBuffer, color);
    }
}

void LcdIf_FillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint16_t color)
{
    volatile uint16_t *fb;
    uint32_t row;
    uint32_t col;

    if ((LcdIf_Ready == 0u) ||
        (x >= LCD_TLI_WIDTH) ||
        (y >= LCD_TLI_HEIGHT) ||
        (width == 0u) ||
        (height == 0u))
    {
        return;
    }

    if ((x + width) > LCD_TLI_WIDTH)
    {
        /* 对右边界做裁剪，保证后面的 framebuffer 线性写入不会越过屏幕宽度。 */
        width = LCD_TLI_WIDTH - x;
    }
    if ((y + height) > LCD_TLI_HEIGHT)
    {
        /* 对下边界做裁剪，调用方可以放心传入贴边区域。 */
        height = LCD_TLI_HEIGHT - y;
    }

    fb = (volatile uint16_t *)LcdIf_FrameBuffer;
    for (row = 0u; row < height; row++)
    {
        for (col = 0u; col < width; col++)
        {
            fb[((y + row) * LCD_TLI_WIDTH) + x + col] = color;
        }
    }
}

static uint32_t LcdIf_DrawChar(uint32_t x, uint32_t y, char ch, uint8_t scale, uint16_t color)
{
    const uint8_t *glyph;
    uint32_t row;
    uint32_t col;

    glyph = LcdIf_GetGlyph(ch);
    /*
     * 每个点阵 bit 被放大成 scale x scale 的实心矩形。
     * 返回值是下一个字符的 x 坐标，额外 1 列间距也乘以 scale。
     */
    for (row = 0u; row < 7u; row++)
    {
        for (col = 0u; col < 5u; col++)
        {
            if ((glyph[row] & (uint8_t)(1u << (4u - col))) != 0u)
            {
                LcdIf_FillRect(x + (col * scale), y + (row * scale), scale, scale, color);
            }
        }
    }

    return x + ((uint32_t)scale * 6u);
}

void LcdIf_DrawText(uint32_t x, uint32_t y, const char *text, uint8_t scale, uint16_t color)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        x = LcdIf_DrawChar(x, y, *text, scale, color);
        text++;
    }
}

static void LcdIf_U32ToDec(uint32_t value, char *buffer)
{
    char temp[11];
    uint8_t count;
    uint8_t index;

    if (value == 0u)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    count = 0u;
    /*
     * 先反向取余得到数字，再倒序拷贝到输出缓冲。
     * 避免引入 sprintf，减小嵌入式工程的库依赖和栈开销。
     */
    while ((value != 0u) && (count < (uint8_t)sizeof(temp)))
    {
        temp[count] = (char)('0' + (value % 10u));
        value /= 10u;
        count++;
    }

    for (index = 0u; index < count; index++)
    {
        buffer[index] = temp[count - 1u - index];
    }
    buffer[count] = '\0';
}

void LcdIf_DrawU32(uint32_t x, uint32_t y, uint32_t value, uint8_t scale, uint16_t color)
{
    char buffer[12];

    LcdIf_U32ToDec(value, buffer);
    LcdIf_DrawText(x, y, buffer, scale, color);
}

void LcdIf_DrawSignedX100(uint32_t x, uint32_t y, int32_t value, uint8_t scale, uint16_t color)
{
    uint32_t abs_value;
    char buffer[16];
    uint8_t pos;

    pos = 0u;
    if (value < 0)
    {
        /* 定点数允许负温度，先记录符号，再按绝对值绘制整数和小数部分。 */
        buffer[pos++] = '-';
        abs_value = (uint32_t)(-value);
    }
    else
    {
        abs_value = (uint32_t)value;
    }

    LcdIf_U32ToDec(abs_value / 100u, &buffer[pos]);
    while (buffer[pos] != '\0')
    {
        pos++;
    }
    buffer[pos++] = '.';
    buffer[pos++] = (char)('0' + ((abs_value / 10u) % 10u));
    buffer[pos++] = (char)('0' + (abs_value % 10u));
    buffer[pos] = '\0';

    LcdIf_DrawText(x, y, buffer, scale, color);
}

uint8_t LcdIf_IsReady(void)
{
    return LcdIf_Ready;
}
