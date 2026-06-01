#include "dashboard_demo_test.h"

#include "BuzzerIf.h"
#include "Can.h"
#include "Exmc.h"
#include "IoHwAb.h"
#include "LcdTli.h"
#include "PowerIf.h"
#include "RtcIf.h"
#include "SensorIf.h"
#include "Uart.h"
#include "board_pins.h"

#include <stdint.h>

#define TEST12_CAN_STATUS_STD_ID          0x321u
#define TEST12_CAN_INPUT_STD_ID           0x100u
#define TEST12_CAN_ALT_INPUT_STD_ID       0x123u
#define TEST12_CAN_TX_TIMEOUT_LOOP        200000u

#define TEST12_LOOP_MS                    20u
#define TEST12_DRAW_PERIOD_MS             500u
#define TEST12_SENSOR_PERIOD_MS           1000u
#define TEST12_RTC_PERIOD_MS              1000u
#define TEST12_CAN_STATUS_PERIOD_MS       1000u
#define TEST12_SERIAL_PERIOD_MS           1000u
#define TEST12_SIM_PERIOD_MS              100u
#define TEST12_KEY_DEBOUNCE_MS            40u
#define TEST12_KEY_BEEP_MS                70u

#define TEST12_SPEED_ALARM_X10            1200u
#define TEST12_RPM_ALARM                  5000u
#define TEST12_SPEED_MAX_X10              2200u
#define TEST12_RPM_MAX                    8000u

#define TEST12_RGB565(r, g, b)            ((uint16_t)(((((uint16_t)(r)) & 0xF8u) << 8u) | \
                                                       ((((uint16_t)(g)) & 0xFCu) << 3u) | \
                                                       ((((uint16_t)(b)) & 0xF8u) >> 3u)))

#define TEST12_COLOR_BG                   TEST12_RGB565(10u, 14u, 18u)
#define TEST12_COLOR_PANEL                TEST12_RGB565(22u, 30u, 38u)
#define TEST12_COLOR_PANEL_DARK           TEST12_RGB565(14u, 19u, 24u)
#define TEST12_COLOR_TEXT                 TEST12_RGB565(224u, 232u, 235u)
#define TEST12_COLOR_MUTED                TEST12_RGB565(108u, 122u, 132u)
#define TEST12_COLOR_CYAN                 TEST12_RGB565(0u, 190u, 230u)
#define TEST12_COLOR_GREEN                TEST12_RGB565(80u, 220u, 130u)
#define TEST12_COLOR_YELLOW               TEST12_RGB565(245u, 200u, 60u)
#define TEST12_COLOR_RED                  TEST12_RGB565(235u, 70u, 75u)
#define TEST12_COLOR_BLUE                 TEST12_RGB565(75u, 130u, 245u)

typedef struct
{
    uint32_t framebuffer;
    uint32_t tick_ms;

    uint16_t speed_kph_x10;
    uint16_t rpm;
    int32_t temperature_c_x100;
    int32_t humidity_rh_x100;
    RtcIf_TimeType rtc_time;

    uint8_t lcd_ok;
    uint8_t layout_drawn;
    uint8_t can_ok;
    uint8_t sensor_ok;
    uint8_t rtc_ok;
    uint8_t has_can_value;
    uint8_t sim_mode;
    uint8_t sim_dir_up;
    uint8_t mute_alarm;
    uint8_t alarm_active;
    uint8_t can_tx_counter;

    uint8_t raw_key_mask;
    uint8_t last_raw_key_mask;
    uint8_t stable_key_mask;
    uint32_t key_same_ms;
    uint32_t key_beep_until_ms;

    uint32_t last_can_rx_ms;
    uint32_t next_draw_ms;
    uint32_t next_sensor_ms;
    uint32_t next_rtc_ms;
    uint32_t next_can_status_ms;
    uint32_t next_serial_ms;
    uint32_t next_sim_ms;
} Test12_StateType;

static const uint8_t TEST12_FONT_SPACE[7] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
static const uint8_t TEST12_FONT_DOT[7]   = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x04u, 0x04u};
static const uint8_t TEST12_FONT_COLON[7] = {0x00u, 0x04u, 0x04u, 0x00u, 0x04u, 0x04u, 0x00u};
static const uint8_t TEST12_FONT_MINUS[7] = {0x00u, 0x00u, 0x00u, 0x1Fu, 0x00u, 0x00u, 0x00u};
static const uint8_t TEST12_FONT_SLASH[7] = {0x01u, 0x02u, 0x02u, 0x04u, 0x08u, 0x08u, 0x10u};
static const uint8_t TEST12_FONT_PERCENT[7] = {0x18u, 0x19u, 0x02u, 0x04u, 0x08u, 0x13u, 0x03u};

static const uint8_t TEST12_FONT_DIGITS[10][7] =
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

static const uint8_t TEST12_FONT_LETTERS[26][7] =
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

static void Test12_DelayMs(uint32_t ms)
{
    uint32_t i;

    while (ms-- != 0u)
    {
        for (i = 0u; i < 20000u; i++)
        {
            __NOP();
        }
    }
}

static void Test12_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test12_DelayMs(10u);
        elapsed += 10u;
    }
}

static void Test12_PrintHex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    Uart_DebugPutc(hex[(value >> 4u) & 0x0Fu]);
    Uart_DebugPutc(hex[value & 0x0Fu]);
}

static void Test12_PrintSignedX100(int32_t value)
{
    uint32_t abs_value;

    if (value < 0)
    {
        Uart_DebugPutc('-');
        abs_value = (uint32_t)(-value);
    }
    else
    {
        abs_value = (uint32_t)value;
    }

    Uart_DebugPutDec(abs_value / 100u);
    Uart_DebugPutc('.');
    if ((abs_value % 100u) < 10u)
    {
        Uart_DebugPutc('0');
    }
    Uart_DebugPutDec(abs_value % 100u);
}

static void Test12_PrintSpeedX10(uint16_t speed_x10)
{
    Uart_DebugPutDec((uint32_t)(speed_x10 / 10u));
    Uart_DebugPutc('.');
    Uart_DebugPutDec((uint32_t)(speed_x10 % 10u));
}

static uint16_t Test12_AlarmColor(uint8_t alarm)
{
    return (alarm != 0u) ? TEST12_COLOR_RED : TEST12_COLOR_GREEN;
}

static void Test12_FillRect(uint32_t framebuffer,
                            uint32_t x,
                            uint32_t y,
                            uint32_t width,
                            uint32_t height,
                            uint16_t color)
{
    volatile uint16_t *fb;
    uint32_t row;
    uint32_t col;
    uint32_t max_width;
    uint32_t max_height;

    if ((x >= LCD_TLI_WIDTH) || (y >= LCD_TLI_HEIGHT) || (width == 0u) || (height == 0u))
    {
        return;
    }

    max_width = LCD_TLI_WIDTH - x;
    max_height = LCD_TLI_HEIGHT - y;
    if (width > max_width)
    {
        width = max_width;
    }
    if (height > max_height)
    {
        height = max_height;
    }

    fb = (volatile uint16_t *)framebuffer;
    for (row = 0u; row < height; row++)
    {
        for (col = 0u; col < width; col++)
        {
            fb[((y + row) * LCD_TLI_WIDTH) + x + col] = color;
        }
    }
}

static void Test12_DrawRect(uint32_t framebuffer,
                            uint32_t x,
                            uint32_t y,
                            uint32_t width,
                            uint32_t height,
                            uint16_t color)
{
    Test12_FillRect(framebuffer, x, y, width, 2u, color);
    Test12_FillRect(framebuffer, x, y + height - 2u, width, 2u, color);
    Test12_FillRect(framebuffer, x, y, 2u, height, color);
    Test12_FillRect(framebuffer, x + width - 2u, y, 2u, height, color);
}

static const uint8_t *Test12_GetGlyph(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return TEST12_FONT_DIGITS[(uint8_t)(ch - '0')];
    }

    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch = (char)(ch - 'a' + 'A');
    }

    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return TEST12_FONT_LETTERS[(uint8_t)(ch - 'A')];
    }

    if (ch == '.')
    {
        return TEST12_FONT_DOT;
    }
    if (ch == ':')
    {
        return TEST12_FONT_COLON;
    }
    if (ch == '-')
    {
        return TEST12_FONT_MINUS;
    }
    if (ch == '/')
    {
        return TEST12_FONT_SLASH;
    }
    if (ch == '%')
    {
        return TEST12_FONT_PERCENT;
    }

    return TEST12_FONT_SPACE;
}

static uint32_t Test12_DrawChar(uint32_t framebuffer,
                                uint32_t x,
                                uint32_t y,
                                char ch,
                                uint8_t scale,
                                uint16_t color)
{
    const uint8_t *glyph;
    uint32_t row;
    uint32_t col;

    glyph = Test12_GetGlyph(ch);

    for (row = 0u; row < 7u; row++)
    {
        for (col = 0u; col < 5u; col++)
        {
            if ((glyph[row] & (uint8_t)(1u << (4u - col))) != 0u)
            {
                Test12_FillRect(framebuffer,
                                x + (col * scale),
                                y + (row * scale),
                                scale,
                                scale,
                                color);
            }
        }
    }

    return x + ((uint32_t)scale * 6u);
}

static uint32_t Test12_DrawString(uint32_t framebuffer,
                                  uint32_t x,
                                  uint32_t y,
                                  const char *text,
                                  uint8_t scale,
                                  uint16_t color)
{
    while (*text != '\0')
    {
        x = Test12_DrawChar(framebuffer, x, y, *text, scale, color);
        text++;
    }

    return x;
}

static void Test12_U32ToDec(uint32_t value, char *buffer)
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

static uint32_t Test12_DrawU32(uint32_t framebuffer,
                               uint32_t x,
                               uint32_t y,
                               uint32_t value,
                               uint8_t scale,
                               uint16_t color)
{
    char buffer[12];

    Test12_U32ToDec(value, buffer);
    return Test12_DrawString(framebuffer, x, y, buffer, scale, color);
}

static uint32_t Test12_DrawTwoDigits(uint32_t framebuffer,
                                     uint32_t x,
                                     uint32_t y,
                                     uint8_t value,
                                     uint8_t scale,
                                     uint16_t color)
{
    x = Test12_DrawChar(framebuffer, x, y, (char)('0' + (value / 10u)), scale, color);
    x = Test12_DrawChar(framebuffer, x, y, (char)('0' + (value % 10u)), scale, color);
    return x;
}

static uint32_t Test12_DrawSignedX100(uint32_t framebuffer,
                                      uint32_t x,
                                      uint32_t y,
                                      int32_t value,
                                      uint8_t scale,
                                      uint16_t color)
{
    uint32_t abs_value;

    if (value < 0)
    {
        x = Test12_DrawChar(framebuffer, x, y, '-', scale, color);
        abs_value = (uint32_t)(-value);
    }
    else
    {
        abs_value = (uint32_t)value;
    }

    x = Test12_DrawU32(framebuffer, x, y, abs_value / 100u, scale, color);
    x = Test12_DrawChar(framebuffer, x, y, '.', scale, color);
    x = Test12_DrawTwoDigits(framebuffer, x, y, (uint8_t)(abs_value % 100u), scale, color);

    return x;
}

static void Test12_DrawBar(uint32_t framebuffer,
                           uint32_t x,
                           uint32_t y,
                           uint32_t width,
                           uint32_t height,
                           uint32_t value,
                           uint32_t max_value,
                           uint16_t color)
{
    uint32_t fill_width;

    if (value > max_value)
    {
        value = max_value;
    }

    if (max_value == 0u)
    {
        fill_width = 0u;
    }
    else
    {
        fill_width = ((width - 8u) * value) / max_value;
    }

    Test12_FillRect(framebuffer, x, y, width, height, TEST12_COLOR_PANEL_DARK);
    Test12_DrawRect(framebuffer, x, y, width, height, TEST12_COLOR_MUTED);
    Test12_FillRect(framebuffer, x + 4u, y + 4u, fill_width, height - 8u, color);
}

static void Test12_DrawTime(uint32_t framebuffer,
                            uint32_t x,
                            uint32_t y,
                            const Test12_StateType *state,
                            uint8_t scale,
                            uint16_t color)
{
    if (state->rtc_ok == 0u)
    {
        (void)Test12_DrawString(framebuffer, x, y, "NO RTC", scale, TEST12_COLOR_YELLOW);
        return;
    }

    x = Test12_DrawTwoDigits(framebuffer, x, y, state->rtc_time.hour, scale, color);
    x = Test12_DrawChar(framebuffer, x, y, ':', scale, color);
    x = Test12_DrawTwoDigits(framebuffer, x, y, state->rtc_time.minute, scale, color);
    x = Test12_DrawChar(framebuffer, x, y, ':', scale, color);
    (void)Test12_DrawTwoDigits(framebuffer, x, y, state->rtc_time.second, scale, color);
}

static const char *Test12_CanStatusText(const Test12_StateType *state)
{
    if (state->can_ok == 0u)
    {
        return "CAN FAIL";
    }

    if (state->sim_mode != 0u)
    {
        return "SIM MODE";
    }

    if (state->has_can_value == 0u)
    {
        return "CAN WAIT";
    }

    if ((state->tick_ms - state->last_can_rx_ms) > 3000u)
    {
        return "CAN LOST";
    }

    return "CAN OK";
}

static void Test12_DrawDashboard(Test12_StateType *state)
{
    uint16_t speed_color;
    uint16_t rpm_color;
    uint16_t can_color;
    uint32_t x;

    speed_color = (state->speed_kph_x10 >= TEST12_SPEED_ALARM_X10) ? TEST12_COLOR_RED : TEST12_COLOR_CYAN;
    rpm_color = (state->rpm >= TEST12_RPM_ALARM) ? TEST12_COLOR_RED : TEST12_COLOR_GREEN;
    can_color = ((state->can_ok != 0u) && ((state->sim_mode != 0u) ||
                 ((state->has_can_value != 0u) && ((state->tick_ms - state->last_can_rx_ms) <= 3000u)))) ?
                 TEST12_COLOR_GREEN : TEST12_COLOR_YELLOW;

    if (state->layout_drawn == 0u)
    {
        /*
         * TLI 正在从 SDRAM 取图，运行时不要高频整屏擦除。
         * 固定框架只画一次，后面只刷新数字和状态区域，能明显减轻 SDRAM 读写抢占。
         */
        LcdTli_FillColor(state->framebuffer, TEST12_COLOR_BG);

        Test12_FillRect(state->framebuffer, 0u, 0u, LCD_TLI_WIDTH, 58u, TEST12_COLOR_PANEL);
        (void)Test12_DrawString(state->framebuffer, 24u, 15u, "CAR DASHBOARD DEMO", 4u, TEST12_COLOR_TEXT);

        Test12_FillRect(state->framebuffer, 28u, 76u, 354u, 232u, TEST12_COLOR_PANEL);
        Test12_FillRect(state->framebuffer, 418u, 76u, 354u, 232u, TEST12_COLOR_PANEL);
        Test12_DrawRect(state->framebuffer, 28u, 76u, 354u, 232u, TEST12_COLOR_MUTED);
        Test12_DrawRect(state->framebuffer, 418u, 76u, 354u, 232u, TEST12_COLOR_MUTED);

        (void)Test12_DrawString(state->framebuffer, 48u, 94u, "SPEED", 4u, TEST12_COLOR_TEXT);
        (void)Test12_DrawString(state->framebuffer, 438u, 94u, "RPM", 4u, TEST12_COLOR_TEXT);

        Test12_FillRect(state->framebuffer, 28u, 330u, 744u, 112u, TEST12_COLOR_PANEL);
        Test12_DrawRect(state->framebuffer, 28u, 330u, 744u, 112u, TEST12_COLOR_MUTED);

        (void)Test12_DrawString(state->framebuffer, 48u, 352u, "TIME", 3u, TEST12_COLOR_MUTED);
        (void)Test12_DrawString(state->framebuffer, 390u, 352u, "TEMP", 3u, TEST12_COLOR_MUTED);
        (void)Test12_DrawString(state->framebuffer, 48u, 392u, "HUM", 3u, TEST12_COLOR_MUTED);
        (void)Test12_DrawString(state->framebuffer,
                                48u,
                                452u,
                                "KEY1 SIM  KEY2 MUTE  KEY3 CLEAR",
                                2u,
                                TEST12_COLOR_MUTED);
        state->layout_drawn = 1u;
    }

    Test12_FillRect(state->framebuffer, 570u, 12u, 190u, 34u, TEST12_COLOR_PANEL);
    (void)Test12_DrawString(state->framebuffer, 570u, 18u, Test12_CanStatusText(state), 3u, can_color);

    Test12_FillRect(state->framebuffer, 52u, 138u, 294u, 105u, TEST12_COLOR_PANEL);
    x = Test12_DrawU32(state->framebuffer, 52u, 142u, state->speed_kph_x10 / 10u, 13u, speed_color);
    (void)Test12_DrawString(state->framebuffer, x + 10u, 205u, "KMH", 4u, TEST12_COLOR_MUTED);

    Test12_FillRect(state->framebuffer, 438u, 138u, 300u, 105u, TEST12_COLOR_PANEL);
    x = Test12_DrawU32(state->framebuffer, 438u, 145u, state->rpm, 10u, rpm_color);
    (void)Test12_DrawString(state->framebuffer, x + 10u, 207u, "RPM", 4u, TEST12_COLOR_MUTED);

    Test12_DrawBar(state->framebuffer,
                   50u,
                   268u,
                   310u,
                   24u,
                   state->speed_kph_x10,
                   TEST12_SPEED_MAX_X10,
                   Test12_AlarmColor(state->speed_kph_x10 >= TEST12_SPEED_ALARM_X10));
    Test12_DrawBar(state->framebuffer,
                   440u,
                   268u,
                   310u,
                   24u,
                   state->rpm,
                   TEST12_RPM_MAX,
                   Test12_AlarmColor(state->rpm >= TEST12_RPM_ALARM));

    Test12_FillRect(state->framebuffer, 145u, 350u, 210u, 28u, TEST12_COLOR_PANEL);
    Test12_DrawTime(state->framebuffer, 145u, 352u, state, 3u, TEST12_COLOR_TEXT);

    Test12_FillRect(state->framebuffer, 500u, 350u, 230u, 28u, TEST12_COLOR_PANEL);
    if (state->sensor_ok != 0u)
    {
        x = Test12_DrawSignedX100(state->framebuffer, 500u, 352u, state->temperature_c_x100, 3u, TEST12_COLOR_TEXT);
        (void)Test12_DrawString(state->framebuffer, x + 4u, 352u, "C", 3u, TEST12_COLOR_TEXT);
    }
    else
    {
        (void)Test12_DrawString(state->framebuffer, 500u, 352u, "NO SHT", 3u, TEST12_COLOR_YELLOW);
    }

    Test12_FillRect(state->framebuffer, 145u, 390u, 210u, 28u, TEST12_COLOR_PANEL);
    if (state->sensor_ok != 0u)
    {
        x = Test12_DrawSignedX100(state->framebuffer, 145u, 392u, state->humidity_rh_x100, 3u, TEST12_COLOR_TEXT);
        (void)Test12_DrawString(state->framebuffer, x + 4u, 392u, "%", 3u, TEST12_COLOR_TEXT);
    }
    else
    {
        (void)Test12_DrawString(state->framebuffer, 145u, 392u, "NO SHT", 3u, TEST12_COLOR_YELLOW);
    }

    Test12_FillRect(state->framebuffer, 390u, 390u, 360u, 28u, TEST12_COLOR_PANEL);
    if (state->mute_alarm != 0u)
    {
        (void)Test12_DrawString(state->framebuffer, 390u, 392u, "MUTE", 3u, TEST12_COLOR_YELLOW);
    }
    else
    {
        (void)Test12_DrawString(state->framebuffer, 390u, 392u, "BUZZER ON", 3u, TEST12_COLOR_GREEN);
    }

    if (state->alarm_active != 0u)
    {
        (void)Test12_DrawString(state->framebuffer, 590u, 392u, "ALARM", 3u, TEST12_COLOR_RED);
    }
    else
    {
        (void)Test12_DrawString(state->framebuffer, 590u, 392u, "NORMAL", 3u, TEST12_COLOR_GREEN);
    }
}

static void Test12_PrintCanData(const Can_MessageType *message)
{
    uint8_t index;

    Uart_DebugPuts(" bytes=");
    for (index = 0u; index < message->dlc; index++)
    {
        if (index != 0u)
        {
            Uart_DebugPutc(' ');
        }
        Test12_PrintHex8(message->data[index]);
    }
}

static void Test12_HandleCanRx(Test12_StateType *state)
{
    Can_MessageType message;
    uint16_t speed_x10;
    uint16_t rpm;

    if (state->can_ok == 0u)
    {
        return;
    }

    while (Can1_Read(&message) != 0u)
    {
        if ((message.is_remote != 0u) || (message.is_extended != 0u) || (message.dlc < 4u))
        {
            continue;
        }

        if (message.id == TEST12_CAN_STATUS_STD_ID)
        {
            continue;
        }

        /*
         * 联调阶段放宽接收 ID：推荐 PCAN 发 0x100，之前验证过的 0x123 也接收。
         * 若 PCAN 误填成 0x000，只要数据长度正确，也会先解析出来方便排查。
         */
        if ((message.id != TEST12_CAN_INPUT_STD_ID) &&
            (message.id != TEST12_CAN_ALT_INPUT_STD_ID) &&
            (message.id != 0u))
        {
            Uart_DebugPuts("[CAN RX] ignore std id=");
            Uart_DebugPutHex32(message.id);
            Test12_PrintCanData(&message);
            Uart_DebugPuts("\n");
            continue;
        }

        speed_x10 = (uint16_t)((uint16_t)message.data[0] | ((uint16_t)message.data[1] << 8u));
        rpm = (uint16_t)((uint16_t)message.data[2] | ((uint16_t)message.data[3] << 8u));

        if (speed_x10 > TEST12_SPEED_MAX_X10)
        {
            speed_x10 = TEST12_SPEED_MAX_X10;
        }
        if (rpm > TEST12_RPM_MAX)
        {
            rpm = TEST12_RPM_MAX;
        }

        state->speed_kph_x10 = speed_x10;
        state->rpm = rpm;
        state->sim_mode = 0u;
        state->has_can_value = 1u;
        state->last_can_rx_ms = state->tick_ms;

        Uart_DebugPuts("[CAN RX] dashboard id=");
        Uart_DebugPutHex32(message.id);
        Test12_PrintCanData(&message);
        Uart_DebugPuts(" speed=");
        Test12_PrintSpeedX10(state->speed_kph_x10);
        Uart_DebugPuts("km/h rpm=");
        Uart_DebugPutDec(state->rpm);
        Uart_DebugPuts("\n");
    }
}

static void Test12_UpdateSensor(Test12_StateType *state)
{
    SensorIf_Sht30DataType data;

    if (state->sensor_ok == 0u)
    {
        return;
    }

    if (SensorIf_Sht30Read(&data) != 0u)
    {
        state->temperature_c_x100 = data.temperature_c_x100;
        state->humidity_rh_x100 = data.humidity_rh_x100;
    }
    else
    {
        Uart_DebugPuts("[WARN] SHT30 read failed\n");
    }
}

static void Test12_UpdateRtc(Test12_StateType *state)
{
    if (state->rtc_ok == 0u)
    {
        return;
    }

    if (RtcIf_ReadTime(&state->rtc_time) == 0u)
    {
        Uart_DebugPuts("[WARN] RTC read failed\n");
        state->rtc_ok = 0u;
    }
}

static void Test12_UpdateSim(Test12_StateType *state)
{
    if ((state->sim_mode == 0u) || (state->tick_ms < state->next_sim_ms))
    {
        return;
    }

    state->next_sim_ms = state->tick_ms + TEST12_SIM_PERIOD_MS;

    if (state->sim_dir_up != 0u)
    {
        if (state->speed_kph_x10 < 1600u)
        {
            state->speed_kph_x10 = (uint16_t)(state->speed_kph_x10 + 20u);
        }
        else
        {
            state->sim_dir_up = 0u;
        }
    }
    else
    {
        if (state->speed_kph_x10 > 20u)
        {
            state->speed_kph_x10 = (uint16_t)(state->speed_kph_x10 - 20u);
        }
        else
        {
            state->sim_dir_up = 1u;
        }
    }

    state->rpm = (uint16_t)(800u + ((uint32_t)state->speed_kph_x10 * 3u));
    if (state->rpm > TEST12_RPM_MAX)
    {
        state->rpm = TEST12_RPM_MAX;
    }
}

static void Test12_HandleKeyPress(Test12_StateType *state, uint8_t key_mask)
{
    if (key_mask == IOHWAB_KEY1_MASK)
    {
        state->sim_mode = (state->sim_mode == 0u) ? 1u : 0u;
        state->has_can_value = (state->sim_mode != 0u) ? 1u : state->has_can_value;
        state->next_sim_ms = state->tick_ms;
        state->key_beep_until_ms = state->tick_ms + TEST12_KEY_BEEP_MS;
        Uart_DebugPuts((state->sim_mode != 0u) ? "[KEY] KEY1 sim on\n" : "[KEY] KEY1 sim off\n");
    }
    else if (key_mask == IOHWAB_KEY2_MASK)
    {
        state->mute_alarm = (state->mute_alarm == 0u) ? 1u : 0u;
        state->key_beep_until_ms = state->tick_ms + TEST12_KEY_BEEP_MS;
        Uart_DebugPuts((state->mute_alarm != 0u) ? "[KEY] KEY2 mute on\n" : "[KEY] KEY2 mute off\n");
    }
    else if (key_mask == IOHWAB_KEY3_MASK)
    {
        state->speed_kph_x10 = 0u;
        state->rpm = 0u;
        state->has_can_value = 0u;
        state->sim_mode = 0u;
        state->key_beep_until_ms = state->tick_ms + TEST12_KEY_BEEP_MS;
        Uart_DebugPuts("[KEY] KEY3 clear dashboard values\n");
    }
}

static void Test12_HandleChangedKeys(Test12_StateType *state, uint8_t old_mask, uint8_t new_mask)
{
    uint8_t changed;
    uint8_t key;

    changed = (uint8_t)(old_mask ^ new_mask);

    key = IOHWAB_KEY1_MASK;
    if (((changed & key) != 0u) && ((new_mask & key) != 0u))
    {
        Test12_HandleKeyPress(state, key);
    }

    key = IOHWAB_KEY2_MASK;
    if (((changed & key) != 0u) && ((new_mask & key) != 0u))
    {
        Test12_HandleKeyPress(state, key);
    }

    key = IOHWAB_KEY3_MASK;
    if (((changed & key) != 0u) && ((new_mask & key) != 0u))
    {
        Test12_HandleKeyPress(state, key);
    }
}

static void Test12_UpdateKeys(Test12_StateType *state)
{
    state->raw_key_mask = IoHwAb_ReadUserKeyMask();
    if (state->raw_key_mask == state->last_raw_key_mask)
    {
        if (state->key_same_ms < TEST12_KEY_DEBOUNCE_MS)
        {
            state->key_same_ms += TEST12_LOOP_MS;
        }
    }
    else
    {
        state->last_raw_key_mask = state->raw_key_mask;
        state->key_same_ms = 0u;
    }

    if ((state->key_same_ms >= TEST12_KEY_DEBOUNCE_MS) &&
        (state->stable_key_mask != state->raw_key_mask))
    {
        Test12_HandleChangedKeys(state, state->stable_key_mask, state->raw_key_mask);
        state->stable_key_mask = state->raw_key_mask;
    }
}

static void Test12_UpdateAlarm(Test12_StateType *state)
{
    uint8_t beep_on;

    state->alarm_active = ((state->speed_kph_x10 >= TEST12_SPEED_ALARM_X10) ||
                           (state->rpm >= TEST12_RPM_ALARM)) ? 1u : 0u;

    if (state->tick_ms < state->key_beep_until_ms)
    {
        BuzzerIf_On();
        return;
    }

    if ((state->alarm_active == 0u) || (state->mute_alarm != 0u))
    {
        BuzzerIf_Off();
        return;
    }

    beep_on = ((state->tick_ms % 500u) < 80u) ? 1u : 0u;
    BuzzerIf_Set(beep_on);
}

static void Test12_SendCanStatus(Test12_StateType *state)
{
    uint8_t data[8];

    if ((state->can_ok == 0u) || (state->tick_ms < state->next_can_status_ms))
    {
        return;
    }

    state->next_can_status_ms = state->tick_ms + TEST12_CAN_STATUS_PERIOD_MS;

    data[0] = (uint8_t)(state->speed_kph_x10 & 0xFFu);
    data[1] = (uint8_t)((state->speed_kph_x10 >> 8u) & 0xFFu);
    data[2] = (uint8_t)(state->rpm & 0xFFu);
    data[3] = (uint8_t)((state->rpm >> 8u) & 0xFFu);
    data[4] = (uint8_t)((state->alarm_active != 0u) ? 0x01u : 0x00u);
    if (state->mute_alarm != 0u)
    {
        data[4] |= 0x02u;
    }
    if (state->sim_mode != 0u)
    {
        data[4] |= 0x04u;
    }
    data[5] = (uint8_t)((state->sensor_ok != 0u) ? (state->temperature_c_x100 / 100) : 0u);
    data[6] = (uint8_t)((state->rtc_ok != 0u) ? state->rtc_time.second : 0u);
    data[7] = state->can_tx_counter;

    (void)Can1_SendStd(TEST12_CAN_STATUS_STD_ID, data, 8u, TEST12_CAN_TX_TIMEOUT_LOOP);
    state->can_tx_counter++;
}

static void Test12_PrintSerialStatus(Test12_StateType *state)
{
    if (state->tick_ms < state->next_serial_ms)
    {
        return;
    }

    state->next_serial_ms = state->tick_ms + TEST12_SERIAL_PERIOD_MS;

    Uart_DebugPuts("[DASH] speed=");
    Test12_PrintSpeedX10(state->speed_kph_x10);
    Uart_DebugPuts("km/h rpm=");
    Uart_DebugPutDec(state->rpm);
    Uart_DebugPuts(" temp=");
    Test12_PrintSignedX100(state->temperature_c_x100);
    Uart_DebugPuts("C hum=");
    Test12_PrintSignedX100(state->humidity_rh_x100);
    Uart_DebugPuts("% ");
    Uart_DebugPuts(Test12_CanStatusText(state));
    Uart_DebugPuts(" alarm=");
    Uart_DebugPutDec(state->alarm_active);
    Uart_DebugPuts(" mute=");
    Uart_DebugPutDec(state->mute_alarm);
    Uart_DebugPuts("\n");
}

static void Test12_InitState(Test12_StateType *state)
{
    state->framebuffer = 0u;
    state->tick_ms = 0u;
    state->speed_kph_x10 = 0u;
    state->rpm = 0u;
    state->temperature_c_x100 = 0;
    state->humidity_rh_x100 = 0;
    state->rtc_time.year = 2026u;
    state->rtc_time.month = 6u;
    state->rtc_time.date = 1u;
    state->rtc_time.weekday = 1u;
    state->rtc_time.hour = 0u;
    state->rtc_time.minute = 0u;
    state->rtc_time.second = 0u;
    state->lcd_ok = 0u;
    state->layout_drawn = 0u;
    state->can_ok = 0u;
    state->sensor_ok = 0u;
    state->rtc_ok = 0u;
    state->has_can_value = 0u;
    state->sim_mode = 0u;
    state->sim_dir_up = 1u;
    state->mute_alarm = 0u;
    state->alarm_active = 0u;
    state->can_tx_counter = 0u;
    state->raw_key_mask = 0u;
    state->last_raw_key_mask = 0u;
    state->stable_key_mask = 0u;
    state->key_same_ms = 0u;
    state->key_beep_until_ms = 0u;
    state->last_can_rx_ms = 0u;
    state->next_draw_ms = 0u;
    state->next_sensor_ms = 0u;
    state->next_rtc_ms = 0u;
    state->next_can_status_ms = 1000u;
    state->next_serial_ms = 1000u;
    state->next_sim_ms = 0u;
}

static void Test12_InitPeripherals(Test12_StateType *state)
{
    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 12_dashboard_demo start\n");
    Uart_DebugPuts("[INFO] LCD+SDRAM+CAN+RTC+SHT30+KEY+BUZZER integrated demo\n");

    if (Exmc_SdramInit() == 0u)
    {
        Uart_DebugPuts("[FAIL] SDRAM init for dashboard framebuffer\n");
        while (1)
        {
            Test12_WaitWithPowerCheck(100u);
        }
    }

    state->framebuffer = Exmc_SdramBase();
    LcdTli_FillColor(state->framebuffer, TEST12_COLOR_BG);
    if (LcdTli_Init(state->framebuffer) == 0u)
    {
        Uart_DebugPuts("[FAIL] LCD TLI init\n");
        while (1)
        {
            Test12_WaitWithPowerCheck(100u);
        }
    }

    LcdTli_BacklightOn();
    state->lcd_ok = 1u;
    Uart_DebugPuts("[PASS] LCD framebuffer ready\n");

    IoHwAb_KeyInit();
    BuzzerIf_Init();
    state->raw_key_mask = IoHwAb_ReadUserKeyMask();
    state->last_raw_key_mask = state->raw_key_mask;
    state->stable_key_mask = state->raw_key_mask;
    Uart_DebugPuts("[PASS] KEY/BUZZER init\n");

    if (Can1_Init500K() != 0u)
    {
        state->can_ok = 1u;
        Uart_DebugPuts("[PASS] CAN1 500K init\n");
    }
    else
    {
        Uart_DebugPuts("[WARN] CAN1 init failed, dashboard still runs without CAN\n");
    }

    if (RtcIf_Init() != 0u)
    {
        state->rtc_ok = 1u;
        (void)RtcIf_StartOscillator();
        Test12_UpdateRtc(state);
        Uart_DebugPuts("[PASS] DS3231 init\n");
    }
    else
    {
        Uart_DebugPuts("[WARN] DS3231 init failed\n");
    }

    if (SensorIf_Sht30Init() != 0u)
    {
        state->sensor_ok = 1u;
        Test12_UpdateSensor(state);
        Uart_DebugPuts("[PASS] SHT30 init\n");
    }
    else
    {
        Uart_DebugPuts("[WARN] SHT30 init failed\n");
    }

    Uart_DebugPuts("[INFO] PCAN send standard frame id=0x100 dlc=8\n");
    Uart_DebugPuts("[INFO] data[0..1]=speed_kph_x10 LE, data[2..3]=rpm LE\n");
    Uart_DebugPuts("[INFO] example 80.0km/h 2500rpm: 20 03 C4 09 00 00 00 00\n");
    Uart_DebugPuts("[INFO] KEY1=simulation, KEY2=mute alarm, KEY3=clear values\n");
}

void Test12_DashboardDemo_Run(void)
{
    Test12_StateType state;

    Test12_InitState(&state);
    Test12_InitPeripherals(&state);
    Test12_DrawDashboard(&state);

    while (1)
    {
        Test12_HandleCanRx(&state);
        Test12_UpdateKeys(&state);
        Test12_UpdateSim(&state);

        if (state.tick_ms >= state.next_sensor_ms)
        {
            state.next_sensor_ms = state.tick_ms + TEST12_SENSOR_PERIOD_MS;
            Test12_UpdateSensor(&state);
        }

        if (state.tick_ms >= state.next_rtc_ms)
        {
            state.next_rtc_ms = state.tick_ms + TEST12_RTC_PERIOD_MS;
            Test12_UpdateRtc(&state);
        }

        Test12_UpdateAlarm(&state);
        Test12_SendCanStatus(&state);
        Test12_PrintSerialStatus(&state);

        if (state.tick_ms >= state.next_draw_ms)
        {
            state.next_draw_ms = state.tick_ms + TEST12_DRAW_PERIOD_MS;
            Test12_DrawDashboard(&state);
        }

        Test12_WaitWithPowerCheck(TEST12_LOOP_MS);
        state.tick_ms += TEST12_LOOP_MS;
    }
}
