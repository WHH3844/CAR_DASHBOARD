#include "App_Display.h"

#include "LcdIf.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

#define APP_DISPLAY_COLOR_BG             0x1084u
#define APP_DISPLAY_COLOR_PANEL          0x18C6u
#define APP_DISPLAY_COLOR_PANEL_DARK     0x0842u
#define APP_DISPLAY_COLOR_DIM            0x4208u
#define APP_DISPLAY_COLOR_BLUE           0x04FFu
#define APP_DISPLAY_COLOR_ORANGE         0xFD20u
#define APP_DISPLAY_COLOR_BAR_BG         0x294Au

#define APP_DISPLAY_GEAR_P               0u
#define APP_DISPLAY_GEAR_R               1u
#define APP_DISPLAY_GEAR_N               2u
#define APP_DISPLAY_GEAR_D               3u
#define APP_DISPLAY_GEAR_S               4u
#define APP_DISPLAY_GEAR_M               5u

typedef struct
{
    int16_t x;
    int16_t y;
} App_Display_PointType;

typedef struct
{
    uint8_t valid;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} App_Display_TimeSnapshotType;

typedef enum
{
    APP_DISPLAY_ICON_ENGINE = 0u,
    APP_DISPLAY_ICON_ABS,
    APP_DISPLAY_ICON_AIRBAG,
    APP_DISPLAY_ICON_BRAKE,
    APP_DISPLAY_ICON_TPMS,
    APP_DISPLAY_ICON_DOOR,
    APP_DISPLAY_ICON_SEATBELT,
    APP_DISPLAY_ICON_WASHER,
    APP_DISPLAY_ICON_HIGH_BEAM,
    APP_DISPLAY_ICON_LOW_BEAM,
    APP_DISPLAY_ICON_TURN
} App_Display_WarningIconType;

static const App_Display_PointType App_Display_SpeedArc[13] =
{
    {-87,  50}, {-98,  17}, {-98, -17}, {-87, -50}, {-64, -77}, {-34, -94}, {0, -100},
    { 34, -94}, { 64, -77}, { 87, -50}, { 98, -17}, { 98,  17}, {87, 50}
};

static const App_Display_PointType App_Display_RpmArc[9] =
{
    {-87, 50}, {-100, 0}, {-87, -50}, {-50, -87}, {0, -100},
    {50, -87}, {87, -50}, {100, 0}, {87, 50}
};

static uint8_t App_Display_AppliedBacklightLevel;
static uint8_t App_Display_StaticDrawn;
static uint8_t App_Display_CenterAreaCleared;

static void App_Display_DrawRectOutline(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint16_t color);

void App_Display_Init(void)
{
    uint8_t level;

    App_Display_StaticDrawn = 0u;
    App_Display_CenterAreaCleared = 0u;
    if (Rte_Read_BacklightLevel(&level) == E_OK)
    {
        Rte_Call_Backlight_Set(level);
        App_Display_AppliedBacklightLevel = level;
    }
    else
    {
        App_Display_AppliedBacklightLevel = 0xFFu;
    }
}

static void App_Display_DrawStaticBackground(void)
{
    /*
     * Clear the full framebuffer only once after the LCD becomes ready.
     * Later refreshes redraw small regions, which avoids a visible full-screen blink.
     */
    LcdIf_Clear(APP_DISPLAY_COLOR_BG);
    LcdIf_FillRect(30u, 344u, 740u, 116u, APP_DISPLAY_COLOR_PANEL_DARK);
    App_Display_DrawRectOutline(30u, 344u, 740u, 116u, APP_DISPLAY_COLOR_DIM);
    App_Display_StaticDrawn = 1u;
}

static void App_Display_UpdateBacklight(void)
{
    uint8_t level;

    if (Rte_Read_BacklightLevel(&level) != E_OK)
    {
        return;
    }

    if (level != App_Display_AppliedBacklightLevel)
    {
        Rte_Call_Backlight_Set(level);
        App_Display_AppliedBacklightLevel = level;
    }
}

static uint32_t App_Display_TextWidth(const char *text, uint8_t scale)
{
    uint32_t count;

    count = 0u;
    if (text == 0)
    {
        return 0u;
    }

    while (*text != '\0')
    {
        count++;
        text++;
    }

    return count * 6u * (uint32_t)scale;
}

static uint8_t App_Display_DecDigits(uint32_t value)
{
    uint8_t digits;

    digits = 1u;
    while (value >= 10u)
    {
        value /= 10u;
        digits++;
    }

    return digits;
}

static void App_Display_DrawCenteredText(int32_t center_x, uint32_t y, const char *text, uint8_t scale, uint16_t color)
{
    int32_t x;
    uint32_t width;

    width = App_Display_TextWidth(text, scale);
    x = center_x - (int32_t)(width / 2u);
    if (x < 0)
    {
        x = 0;
    }

    LcdIf_DrawText((uint32_t)x, y, text, scale, color);
}

static void App_Display_DrawCenteredU32(int32_t center_x, uint32_t y, uint32_t value, uint8_t scale, uint16_t color)
{
    int32_t x;
    uint32_t width;

    width = (uint32_t)App_Display_DecDigits(value) * 6u * (uint32_t)scale;
    x = center_x - (int32_t)(width / 2u);
    if (x < 0)
    {
        x = 0;
    }

    LcdIf_DrawU32((uint32_t)x, y, value, scale, color);
}

static void App_Display_DrawRectOutline(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint16_t color)
{
    LcdIf_DrawLine((int32_t)x, (int32_t)y, (int32_t)(x + width - 1u), (int32_t)y, color);
    LcdIf_DrawLine((int32_t)x, (int32_t)(y + height - 1u), (int32_t)(x + width - 1u), (int32_t)(y + height - 1u), color);
    LcdIf_DrawLine((int32_t)x, (int32_t)y, (int32_t)x, (int32_t)(y + height - 1u), color);
    LcdIf_DrawLine((int32_t)(x + width - 1u), (int32_t)y, (int32_t)(x + width - 1u), (int32_t)(y + height - 1u), color);
}

static void App_Display_DrawBoldLine(int32_t x0,
                                     int32_t y0,
                                     int32_t x1,
                                     int32_t y1,
                                     uint16_t color)
{
    LcdIf_DrawLine(x0, y0, x1, y1, color);
    LcdIf_DrawLine(x0, y0 + 1, x1, y1 + 1, color);
}

static void App_Display_DrawBoldCircle(int32_t center_x,
                                       int32_t center_y,
                                       int32_t radius,
                                       uint16_t color)
{
    LcdIf_DrawCircle(center_x, center_y, radius, color);
    if (radius > 1)
    {
        LcdIf_DrawCircle(center_x, center_y, radius - 1, color);
    }
}

static void App_Display_DrawWarningIcon(uint32_t *next_x,
                                        App_Display_WarningIconType icon,
                                        uint16_t color)
{
    int32_t x;
    int32_t y;

    if ((next_x == 0) || (*next_x > 512u))
    {
        return;
    }

    x = (int32_t)(*next_x);
    y = 14;

    if (icon == APP_DISPLAY_ICON_ENGINE)
    {
        App_Display_DrawRectOutline((uint32_t)(x + 5), (uint32_t)(y + 8), 18u, 14u, color);
        App_Display_DrawRectOutline((uint32_t)(x + 6), (uint32_t)(y + 9), 16u, 12u, color);
        App_Display_DrawBoldLine(x + 10, y + 5, x + 18, y + 5, color);
        App_Display_DrawBoldLine(x + 14, y + 5, x + 14, y + 8, color);
        App_Display_DrawBoldLine(x + 2, y + 12, x + 5, y + 12, color);
        App_Display_DrawBoldLine(x + 2, y + 12, x + 2, y + 17, color);
        App_Display_DrawBoldLine(x + 23, y + 11, x + 26, y + 11, color);
        App_Display_DrawBoldLine(x + 26, y + 11, x + 26, y + 17, color);
        App_Display_DrawBoldLine(x + 9, y + 22, x + 9, y + 25, color);
        App_Display_DrawBoldLine(x + 9, y + 25, x + 16, y + 25, color);
    }
    else if (icon == APP_DISPLAY_ICON_ABS)
    {
        App_Display_DrawBoldCircle(x + 14, y + 14, 13, color);
        LcdIf_DrawText((uint32_t)(x + 5), (uint32_t)(y + 10), "ABS", 1u, color);
    }
    else if (icon == APP_DISPLAY_ICON_AIRBAG)
    {
        App_Display_DrawBoldCircle(x + 7, y + 6, 3, color);
        App_Display_DrawBoldLine(x + 7, y + 10, x + 12, y + 17, color);
        App_Display_DrawBoldLine(x + 12, y + 17, x + 8, y + 23, color);
        App_Display_DrawBoldLine(x + 3, y + 12, x + 3, y + 24, color);
        App_Display_DrawBoldLine(x + 3, y + 24, x + 14, y + 24, color);
        App_Display_DrawBoldCircle(x + 21, y + 11, 6, color);
        App_Display_DrawBoldLine(x + 13, y + 15, x + 16, y + 13, color);
    }
    else if (icon == APP_DISPLAY_ICON_BRAKE)
    {
        App_Display_DrawBoldCircle(x + 14, y + 14, 13, color);
        App_Display_DrawBoldCircle(x + 14, y + 14, 10, color);
        App_Display_DrawBoldLine(x + 14, y + 7, x + 14, y + 17, color);
        App_Display_DrawBoldCircle(x + 14, y + 21, 1, color);
    }
    else if (icon == APP_DISPLAY_ICON_TPMS)
    {
        App_Display_DrawBoldLine(x + 4, y + 6, x + 2, y + 17, color);
        App_Display_DrawBoldLine(x + 2, y + 17, x + 7, y + 25, color);
        App_Display_DrawBoldLine(x + 7, y + 25, x + 21, y + 25, color);
        App_Display_DrawBoldLine(x + 21, y + 25, x + 26, y + 17, color);
        App_Display_DrawBoldLine(x + 26, y + 17, x + 24, y + 6, color);
        App_Display_DrawBoldLine(x + 14, y + 7, x + 14, y + 17, color);
        App_Display_DrawBoldCircle(x + 14, y + 21, 1, color);
    }
    else if (icon == APP_DISPLAY_ICON_DOOR)
    {
        App_Display_DrawRectOutline((uint32_t)(x + 9), (uint32_t)(y + 4), 10u, 21u, color);
        App_Display_DrawBoldLine(x + 9, y + 9, x + 3, y + 4, color);
        App_Display_DrawBoldLine(x + 9, y + 18, x + 3, y + 25, color);
        App_Display_DrawBoldLine(x + 18, y + 9, x + 24, y + 4, color);
        App_Display_DrawBoldLine(x + 18, y + 18, x + 24, y + 25, color);
    }
    else if (icon == APP_DISPLAY_ICON_SEATBELT)
    {
        App_Display_DrawBoldCircle(x + 7, y + 5, 3, color);
        App_Display_DrawBoldLine(x + 7, y + 9, x + 12, y + 17, color);
        App_Display_DrawBoldLine(x + 12, y + 17, x + 9, y + 24, color);
        App_Display_DrawBoldLine(x + 3, y + 12, x + 3, y + 25, color);
        App_Display_DrawBoldLine(x + 3, y + 25, x + 17, y + 25, color);
        App_Display_DrawBoldLine(x + 4, y + 9, x + 19, y + 24, color);
        App_Display_DrawBoldLine(x + 18, y + 7, x + 18, y + 24, color);
    }
    else if (icon == APP_DISPLAY_ICON_WASHER)
    {
        App_Display_DrawBoldLine(x + 4, y + 20, x + 24, y + 20, color);
        App_Display_DrawBoldLine(x + 4, y + 20, x + 7, y + 10, color);
        App_Display_DrawBoldLine(x + 24, y + 20, x + 21, y + 10, color);
        App_Display_DrawBoldLine(x + 8, y + 8, x + 10, y + 5, color);
        App_Display_DrawBoldLine(x + 14, y + 8, x + 14, y + 4, color);
        App_Display_DrawBoldLine(x + 20, y + 8, x + 18, y + 5, color);
    }
    else if ((icon == APP_DISPLAY_ICON_HIGH_BEAM) ||
             (icon == APP_DISPLAY_ICON_LOW_BEAM))
    {
        App_Display_DrawBoldLine(x + 4, y + 8, x + 11, y + 5, color);
        App_Display_DrawBoldLine(x + 11, y + 5, x + 11, y + 23, color);
        App_Display_DrawBoldLine(x + 11, y + 23, x + 4, y + 20, color);
        App_Display_DrawBoldLine(x + 4, y + 8, x + 4, y + 20, color);
        if (icon == APP_DISPLAY_ICON_HIGH_BEAM)
        {
            App_Display_DrawBoldLine(x + 14, y + 8, x + 27, y + 8, color);
            App_Display_DrawBoldLine(x + 14, y + 14, x + 27, y + 14, color);
            App_Display_DrawBoldLine(x + 14, y + 20, x + 27, y + 20, color);
        }
        else
        {
            App_Display_DrawBoldLine(x + 14, y + 8, x + 27, y + 12, color);
            App_Display_DrawBoldLine(x + 14, y + 14, x + 27, y + 18, color);
            App_Display_DrawBoldLine(x + 14, y + 20, x + 27, y + 24, color);
        }
    }
    else
    {
        App_Display_DrawBoldLine(x + 2, y + 14, x + 11, y + 6, color);
        App_Display_DrawBoldLine(x + 2, y + 14, x + 11, y + 22, color);
        App_Display_DrawBoldLine(x + 2, y + 14, x + 25, y + 14, color);
        App_Display_DrawBoldLine(x + 17, y + 6, x + 26, y + 14, color);
        App_Display_DrawBoldLine(x + 26, y + 14, x + 17, y + 22, color);
    }

    *next_x += 32u;
}

static void App_Display_ArcPoint(const App_Display_PointType *table,
                                 uint8_t point_count,
                                 uint32_t max_value,
                                 uint32_t value,
                                 int32_t center_x,
                                 int32_t center_y,
                                 int32_t radius,
                                 int32_t *out_x,
                                 int32_t *out_y)
{
    uint32_t pos;
    uint32_t index;
    uint32_t rem;
    int32_t x;
    int32_t y;

    if (value >= max_value)
    {
        x = table[point_count - 1u].x;
        y = table[point_count - 1u].y;
    }
    else
    {
        pos = value * (uint32_t)(point_count - 1u);
        index = pos / max_value;
        rem = pos - (index * max_value);
        x = (((int32_t)table[index].x * (int32_t)(max_value - rem)) +
             ((int32_t)table[index + 1u].x * (int32_t)rem)) / (int32_t)max_value;
        y = (((int32_t)table[index].y * (int32_t)(max_value - rem)) +
             ((int32_t)table[index + 1u].y * (int32_t)rem)) / (int32_t)max_value;
    }

    *out_x = center_x + ((x * radius) / 100);
    *out_y = center_y + ((y * radius) / 100);
}

static void App_Display_DrawGaugeArc(const App_Display_PointType *table,
                                      uint8_t point_count,
                                      int32_t center_x,
                                      int32_t center_y,
                                      int32_t radius,
                                      uint16_t color)
{
    uint8_t index;
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;

    for (index = 0u; index < (uint8_t)(point_count - 1u); index++)
    {
        x0 = center_x + (((int32_t)table[index].x * radius) / 100);
        y0 = center_y + (((int32_t)table[index].y * radius) / 100);
        x1 = center_x + (((int32_t)table[index + 1u].x * radius) / 100);
        y1 = center_y + (((int32_t)table[index + 1u].y * radius) / 100);
        LcdIf_DrawLine(x0, y0, x1, y1, color);
        LcdIf_DrawLine(x0 + 1, y0, x1 + 1, y1, color);
        LcdIf_DrawLine(x0, y0 + 1, x1, y1 + 1, color);
    }
}

static void App_Display_DrawNeedle(const App_Display_PointType *table,
                                   uint8_t point_count,
                                   uint32_t max_value,
                                   uint32_t value,
                                   int32_t center_x,
                                   int32_t center_y,
                                   int32_t radius,
                                   uint16_t color)
{
    int32_t x;
    int32_t y;

    App_Display_ArcPoint(table, point_count, max_value, value, center_x, center_y, radius, &x, &y);
    LcdIf_DrawLine(center_x, center_y, x, y, color);
    LcdIf_DrawLine(center_x + 1, center_y, x + 1, y, color);
    LcdIf_DrawLine(center_x - 1, center_y, x - 1, y, color);
    LcdIf_DrawLine(center_x, center_y + 1, x, y + 1, color);
    LcdIf_FillRect((uint32_t)(center_x - 5), (uint32_t)(center_y - 5), 10u, 10u, color);
}

static void App_Display_DrawGaugeTicks(const App_Display_PointType *table,
                                       uint8_t point_count,
                                       uint32_t label_step,
                                       uint8_t label_scale,
                                       int32_t center_x,
                                       int32_t center_y,
                                       int32_t radius,
                                       uint16_t color)
{
    uint8_t index;
    uint32_t label;
    int32_t xo;
    int32_t yo;
    int32_t xi;
    int32_t yi;
    int32_t xl;
    int32_t yl;

    for (index = 0u; index < point_count; index++)
    {
        xo = center_x + (((int32_t)table[index].x * radius) / 100);
        yo = center_y + (((int32_t)table[index].y * radius) / 100);
        xi = center_x + (((int32_t)table[index].x * (radius - 15)) / 100);
        yi = center_y + (((int32_t)table[index].y * (radius - 15)) / 100);
        xl = center_x + (((int32_t)table[index].x * (radius - 33)) / 100);
        yl = center_y + (((int32_t)table[index].y * (radius - 33)) / 100);

        LcdIf_DrawLine(xi, yi, xo, yo, color);
        LcdIf_DrawLine(xi + 1, yi, xo + 1, yo, color);

        label = (uint32_t)index * label_step;
        App_Display_DrawCenteredU32(xl, (uint32_t)(yl - 7), label, label_scale, LCDIF_COLOR_WHITE);
    }
}

static void App_Display_DrawRpmGauge(uint16_t rpm)
{
    static uint8_t needle_valid;
    static uint16_t previous_rpm;
    static uint16_t previous_text_rpm;
    uint32_t rpm_x1000;
    uint16_t needle_color;
    uint8_t value_changed;

    if (rpm > 8000u)
    {
        rpm = 8000u;
    }

    value_changed = ((needle_valid == 0u) || (previous_rpm != rpm)) ? 1u : 0u;
    rpm_x1000 = rpm;
    needle_color = (rpm >= 5000u) ? LCDIF_COLOR_RED : APP_DISPLAY_COLOR_ORANGE;

    if ((value_changed == 0u) && (App_Display_CenterAreaCleared == 0u))
    {
        return;
    }

    if ((needle_valid != 0u) && (previous_rpm != rpm))
    {
        App_Display_DrawNeedle(App_Display_RpmArc, 9u, 8000u, previous_rpm, 178, 228, 105, APP_DISPLAY_COLOR_BG);
    }

    App_Display_DrawGaugeArc(App_Display_RpmArc, 9u, 178, 228, 135, APP_DISPLAY_COLOR_BLUE);
    App_Display_DrawGaugeTicks(App_Display_RpmArc, 9u, 1u, 2u, 178, 228, 135, LCDIF_COLOR_GRAY);
    App_Display_DrawCenteredText(178, 62u, "RPM", 2u, LCDIF_COLOR_YELLOW);
    App_Display_DrawCenteredText(178, 79u, "X1000", 1u, LCDIF_COLOR_GRAY);

    if ((value_changed != 0u) || (previous_text_rpm != rpm))
    {
        LcdIf_FillRect(82u, 246u, 192u, 42u, APP_DISPLAY_COLOR_BG);
        App_Display_DrawCenteredU32(178, 250u, rpm, 4u, LCDIF_COLOR_WHITE);
        previous_text_rpm = rpm;
    }

    App_Display_DrawNeedle(App_Display_RpmArc, 9u, 8000u, rpm_x1000, 178, 228, 105, needle_color);
    if (value_changed != 0u)
    {
        previous_rpm = rpm;
        needle_valid = 1u;
    }
}

static void App_Display_DrawSpeedGauge(uint16_t speed_kph_x10)
{
    static uint8_t needle_valid;
    static uint32_t previous_speed_kph;
    static uint32_t previous_text_speed_kph;
    uint32_t speed_kph;
    uint16_t needle_color;
    uint8_t value_changed;

    speed_kph = speed_kph_x10 / 10u;
    if (speed_kph > 240u)
    {
        speed_kph = 240u;
    }

    value_changed = ((needle_valid == 0u) || (previous_speed_kph != speed_kph)) ? 1u : 0u;
    needle_color = (speed_kph_x10 >= 1200u) ? LCDIF_COLOR_RED : LCDIF_COLOR_WHITE;

    if ((value_changed == 0u) && (App_Display_CenterAreaCleared == 0u))
    {
        return;
    }

    if ((needle_valid != 0u) && (previous_speed_kph != speed_kph))
    {
        App_Display_DrawNeedle(App_Display_SpeedArc, 13u, 240u, previous_speed_kph, 622, 228, 105, APP_DISPLAY_COLOR_BG);
    }

    App_Display_DrawGaugeArc(App_Display_SpeedArc, 13u, 622, 228, 135, APP_DISPLAY_COLOR_BLUE);
    App_Display_DrawGaugeTicks(App_Display_SpeedArc, 13u, 20u, 1u, 622, 228, 135, LCDIF_COLOR_GRAY);
    App_Display_DrawCenteredText(622, 66u, "KM/H", 2u, LCDIF_COLOR_YELLOW);

    if ((value_changed != 0u) || (previous_text_speed_kph != speed_kph))
    {
        LcdIf_FillRect(572u, 246u, 100u, 42u, APP_DISPLAY_COLOR_BG);
        App_Display_DrawCenteredU32(622, 250u, speed_kph, 4u, LCDIF_COLOR_WHITE);
        previous_text_speed_kph = speed_kph;
    }

    App_Display_DrawNeedle(App_Display_SpeedArc, 13u, 240u, speed_kph, 622, 228, 105, needle_color);
    if (value_changed != 0u)
    {
        previous_speed_kph = speed_kph;
        needle_valid = 1u;
    }
}

static void App_Display_DrawTopStatus(const Rte_DashboardDataType *data,
                                      const Rte_TpmsDataType *tpms,
                                      const Rte_ControlDomainStatusType *domain)
{
    static uint8_t cache_valid;
    static uint8_t previous_warning_flags;
    static uint8_t previous_can_ems_valid;
    static uint8_t previous_simulation_mode;
    static uint8_t previous_tpms_valid;
    static uint8_t previous_tpms_warning;
    static uint8_t previous_body_summary;
    static uint8_t previous_light_summary;
    static uint8_t previous_domain_online;
    static uint8_t previous_domain_valid;
    static uint8_t previous_domain_health;
    static uint8_t previous_domain_fault;
    static uint8_t previous_domain_stalled;
    uint16_t can_color;
    uint16_t domain_color;
    uint8_t body_summary;
    uint8_t light_summary;
    uint32_t x;

    body_summary = (uint8_t)(data->door_open_mask |
                             (uint8_t)(data->driver_seatbelt << 4u) |
                             (uint8_t)(data->parking_brake << 5u) |
                             (uint8_t)(data->washer_fluid_low << 6u));
    light_summary = (uint8_t)(data->low_beam |
                              (uint8_t)(data->high_beam << 1u) |
                              (uint8_t)(data->turn_signal << 2u));
    if ((cache_valid != 0u) &&
        (previous_warning_flags == data->warning_flags) &&
        (previous_can_ems_valid == data->can_ems_valid) &&
        (previous_simulation_mode == data->simulation_mode) &&
        (previous_tpms_valid == tpms->valid) &&
        (previous_tpms_warning == tpms->warning_mask) &&
        (previous_body_summary == body_summary) &&
        (previous_light_summary == light_summary) &&
        (previous_domain_online == domain->nm_online) &&
        (previous_domain_valid == domain->status_valid) &&
        (previous_domain_health == domain->health_state) &&
        (previous_domain_fault == domain->remote_fault_present) &&
        (previous_domain_stalled == domain->alive_stalled))
    {
        return;
    }

    LcdIf_FillRect(0u, 0u, 800u, 56u, APP_DISPLAY_COLOR_PANEL);
    LcdIf_DrawText(24u, 16u, "ICM DASH", 4u, LCDIF_COLOR_WHITE);

    x = 224u;
    if ((data->warning_flags & 0x01u) != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_ENGINE, LCDIF_COLOR_RED);
    }
    if ((data->warning_flags & 0x02u) != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_ABS, LCDIF_COLOR_YELLOW);
    }
    if ((data->warning_flags & 0x04u) != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_AIRBAG, LCDIF_COLOR_RED);
    }
    if (((data->warning_flags & 0x08u) != 0u) || (data->parking_brake != 0u))
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_BRAKE, LCDIF_COLOR_RED);
    }
    if (tpms->valid == 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_TPMS, LCDIF_COLOR_YELLOW);
    }
    else if (tpms->warning_mask != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_TPMS, LCDIF_COLOR_RED);
    }
    if (data->door_open_mask != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_DOOR, LCDIF_COLOR_RED);
    }
    if ((data->driver_seatbelt == 0u) && (data->can_body_valid != 0u))
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_SEATBELT, LCDIF_COLOR_YELLOW);
    }
    if (data->washer_fluid_low != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_WASHER, LCDIF_COLOR_YELLOW);
    }
    if (data->high_beam != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_HIGH_BEAM, LCDIF_COLOR_BLUE);
    }
    else if (data->low_beam != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_LOW_BEAM, LCDIF_COLOR_GREEN);
    }
    if (data->turn_signal != 0u)
    {
        App_Display_DrawWarningIcon(&x, APP_DISPLAY_ICON_TURN, LCDIF_COLOR_GREEN);
    }

    can_color = (data->can_ems_valid != 0u) ? LCDIF_COLOR_GREEN : LCDIF_COLOR_YELLOW;
    LcdIf_DrawText(566u, 8u, "ICM", 2u, LCDIF_COLOR_GRAY);
    if (data->simulation_mode != 0u)
    {
        LcdIf_DrawText(608u, 8u, "SIM", 2u, LCDIF_COLOR_YELLOW);
    }
    else if (data->can_ems_valid != 0u)
    {
        LcdIf_DrawText(608u, 8u, "OK", 2u, can_color);
    }
    else
    {
        LcdIf_DrawText(608u, 8u, "LOST", 2u, can_color);
    }

    LcdIf_DrawText(676u, 8u, "CDM", 2u, LCDIF_COLOR_GRAY);
    domain_color = LCDIF_COLOR_GREEN;
    if (domain->nm_online == 0u)
    {
        domain_color = LCDIF_COLOR_YELLOW;
        LcdIf_DrawText(720u, 8u, "OFF", 2u, domain_color);
    }
    else if (domain->status_valid == 0u)
    {
        domain_color = LCDIF_COLOR_YELLOW;
        LcdIf_DrawText(720u, 8u, "STALE", 2u, domain_color);
    }
    else if ((domain->health_state == 2u) || (domain->remote_fault_present != 0u))
    {
        domain_color = LCDIF_COLOR_RED;
        LcdIf_DrawText(720u, 8u, "FLT", 2u, domain_color);
    }
    else if ((domain->health_state == 1u) || (domain->alive_stalled != 0u))
    {
        domain_color = LCDIF_COLOR_YELLOW;
        LcdIf_DrawText(720u, 8u, "DEG", 2u, domain_color);
    }
    else
    {
        LcdIf_DrawText(720u, 8u, "OK", 2u, domain_color);
    }

    previous_warning_flags = data->warning_flags;
    previous_can_ems_valid = data->can_ems_valid;
    previous_simulation_mode = data->simulation_mode;
    previous_tpms_valid = tpms->valid;
    previous_tpms_warning = tpms->warning_mask;
    previous_body_summary = body_summary;
    previous_light_summary = light_summary;
    previous_domain_online = domain->nm_online;
    previous_domain_valid = domain->status_valid;
    previous_domain_health = domain->health_state;
    previous_domain_fault = domain->remote_fault_present;
    previous_domain_stalled = domain->alive_stalled;
    cache_valid = 1u;
}

static void App_Display_DrawCenterSpeed(const Rte_DashboardDataType *data)
{
    static uint8_t cache_valid;
    static uint32_t previous_speed_kph;
    static uint16_t previous_color;
    static uint8_t previous_valid;
    uint32_t speed_kph;
    uint16_t color;

    speed_kph = (data->vehicle_speed_valid != 0u) ?
                (data->vehicle_speed_kph_x10 / 10u) : 0u;
    color = (data->vehicle_speed_valid == 0u) ? LCDIF_COLOR_YELLOW :
            ((data->alarm_active != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_WHITE);

    if ((cache_valid != 0u) &&
        (previous_speed_kph == speed_kph) &&
        (previous_color == color) &&
        (previous_valid == data->vehicle_speed_valid))
    {
        return;
    }

    LcdIf_FillRect(302u, 96u, 196u, 128u, APP_DISPLAY_COLOR_BG);
    App_Display_CenterAreaCleared = 1u;
    if (data->vehicle_speed_valid != 0u)
    {
        App_Display_DrawCenteredU32(400, 112u, speed_kph, 10u, color);
    }
    else
    {
        App_Display_DrawCenteredText(400, 112u, "--", 10u, color);
    }
    App_Display_DrawCenteredText(400, 198u, "KM/H", 2u, LCDIF_COLOR_GRAY);

    previous_speed_kph = speed_kph;
    previous_color = color;
    previous_valid = data->vehicle_speed_valid;
    cache_valid = 1u;
}

static uint8_t App_Display_GetDisplayGear(const Rte_DashboardDataType *data)
{
    if (data->can_body_valid != 0u)
    {
        /*
         * 0x322 Byte0=0x09 means N in the matrix. For a dashboard demo, infer D when
         * the car is already moving but BCM still reports N; other gears stay as commanded.
         */
        if ((data->gear_position == APP_DISPLAY_GEAR_N) && (data->vehicle_speed_kph_x10 > 0u))
        {
            return APP_DISPLAY_GEAR_D;
        }

        return data->gear_position;
    }

    /*
     * If only 0x321 is being tested, speed can move while 0x322 is absent.
     * Infer D while moving and N while stopped for display only; RTE/CAN are not rewritten.
     */
    if (data->vehicle_speed_kph_x10 > 0u)
    {
        return APP_DISPLAY_GEAR_D;
    }

    return APP_DISPLAY_GEAR_N;
}

static void App_Display_DrawGear(const Rte_DashboardDataType *data)
{
    static uint8_t cache_valid;
    static uint8_t previous_gear;
    static uint16_t previous_color;
    uint8_t gear;
    uint16_t color;

    gear = App_Display_GetDisplayGear(data);
    color = (data->can_body_valid != 0u) ? LCDIF_COLOR_CYAN : LCDIF_COLOR_YELLOW;

    if ((cache_valid != 0u) &&
        (previous_gear == gear) &&
        (previous_color == color))
    {
        return;
    }

    LcdIf_FillRect(350u, 228u, 100u, 42u, APP_DISPLAY_COLOR_BG);
    if (gear == APP_DISPLAY_GEAR_P)
    {
        App_Display_DrawCenteredText(400, 236u, "P", 4u, color);
    }
    else if (gear == APP_DISPLAY_GEAR_R)
    {
        App_Display_DrawCenteredText(400, 236u, "R", 4u, color);
    }
    else if (gear == APP_DISPLAY_GEAR_N)
    {
        App_Display_DrawCenteredText(400, 236u, "N", 4u, color);
    }
    else if (gear == APP_DISPLAY_GEAR_D)
    {
        App_Display_DrawCenteredText(400, 236u, "D", 4u, color);
    }
    else if (gear == APP_DISPLAY_GEAR_S)
    {
        App_Display_DrawCenteredText(400, 236u, "S", 4u, color);
    }
    else if (gear == APP_DISPLAY_GEAR_M)
    {
        App_Display_DrawCenteredText(400, 236u, "M", 4u, color);
    }
    else
    {
        App_Display_DrawCenteredText(400, 236u, "?", 4u, LCDIF_COLOR_YELLOW);
    }

    previous_gear = gear;
    previous_color = color;
    cache_valid = 1u;
}

static void App_Display_DrawBar(uint32_t x,
                                uint32_t y,
                                uint32_t width,
                                uint32_t value,
                                uint32_t max_value,
                                uint16_t fill_color)
{
    uint32_t fill_width;

    if (value > max_value)
    {
        value = max_value;
    }

    fill_width = (value * (width - 4u)) / max_value;
    LcdIf_FillRect(x, y, width, 16u, APP_DISPLAY_COLOR_BAR_BG);
    LcdIf_FillRect(x + 2u, y + 2u, fill_width, 12u, fill_color);
    App_Display_DrawRectOutline(x, y, width, 16u, APP_DISPLAY_COLOR_DIM);
}

static uint32_t App_Display_ClampTempToBar(int16_t temp_c)
{
    int32_t value;

    value = (int32_t)temp_c + 40;
    if (value < 0)
    {
        value = 0;
    }
    if (value > 160)
    {
        value = 160;
    }

    return (uint32_t)value;
}

static uint32_t App_Display_ClampBatteryToBar(uint16_t battery_mv)
{
    if (battery_mv <= 9000u)
    {
        return 0u;
    }
    if (battery_mv >= 15000u)
    {
        return 6000u;
    }

    return (uint32_t)battery_mv - 9000u;
}

static void App_Display_GetTimeSnapshot(const Rte_ConfigDataType *config, App_Display_TimeSnapshotType *snapshot)
{
    RtcIf_TimeType time;
    uint8_t valid;

    snapshot->valid = 0u;
    snapshot->hour = 0u;
    snapshot->minute = 0u;
    snapshot->second = 0u;

    (void)Rte_Read_RtcTime(&time, &valid);
    if (valid != 0u)
    {
        snapshot->valid = 1u;
        snapshot->hour = time.hour;
        snapshot->minute = time.minute;
        snapshot->second = time.second;
    }
    else if (config->datetime_valid != 0u)
    {
        snapshot->valid = 1u;
        snapshot->hour = config->time_hour;
        snapshot->minute = config->time_minute;
        snapshot->second = 0u;
    }
}

static uint8_t App_Display_IsSameTime(const App_Display_TimeSnapshotType *left,
                                      const App_Display_TimeSnapshotType *right)
{
    return ((left->valid == right->valid) &&
            (left->hour == right->hour) &&
            (left->minute == right->minute) &&
            (left->second == right->second)) ? 1u : 0u;
}

static void App_Display_DrawTime(uint32_t x, uint32_t y, const App_Display_TimeSnapshotType *snapshot)
{
    if (snapshot->valid == 0u)
    {
        LcdIf_DrawText(x, y, "--:--:--", 2u, LCDIF_COLOR_YELLOW);
        return;
    }

    if (snapshot->hour < 10u)
    {
        LcdIf_DrawText(x, y, "0", 2u, LCDIF_COLOR_WHITE);
        LcdIf_DrawU32(x + 14u, y, snapshot->hour, 2u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawU32(x, y, snapshot->hour, 2u, LCDIF_COLOR_WHITE);
    }
    LcdIf_DrawText(x + 28u, y, ":", 2u, LCDIF_COLOR_WHITE);
    if (snapshot->minute < 10u)
    {
        LcdIf_DrawText(x + 42u, y, "0", 2u, LCDIF_COLOR_WHITE);
        LcdIf_DrawU32(x + 56u, y, snapshot->minute, 2u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawU32(x + 42u, y, snapshot->minute, 2u, LCDIF_COLOR_WHITE);
    }
    LcdIf_DrawText(x + 70u, y, ":", 2u, LCDIF_COLOR_WHITE);
    if (snapshot->second < 10u)
    {
        LcdIf_DrawText(x + 84u, y, "0", 2u, LCDIF_COLOR_WHITE);
        LcdIf_DrawU32(x + 98u, y, snapshot->second, 2u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawU32(x + 84u, y, snapshot->second, 2u, LCDIF_COLOR_WHITE);
    }
}

static void App_Display_DrawTpms(const Rte_TpmsDataType *tpms)
{
    LcdIf_DrawText(592u, 374u, "TPMS", 2u, LCDIF_COLOR_YELLOW);
    if (tpms->valid == 0u)
    {
        LcdIf_DrawText(592u, 402u, "-- --", 2u, LCDIF_COLOR_GRAY);
        LcdIf_DrawText(592u, 430u, "-- --", 2u, LCDIF_COLOR_GRAY);
        return;
    }

    LcdIf_DrawText(592u, 402u, "LF", 1u, LCDIF_COLOR_GRAY);
    if (tpms->pressure_valid[0] != 0u)
    {
        LcdIf_DrawSignedX100(616u, 398u, tpms->pressure_bar_x100[0], 2u,
                            ((tpms->warning_mask & 0x01u) != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawText(616u, 402u, "--", 2u, LCDIF_COLOR_GRAY);
    }
    LcdIf_DrawText(700u, 402u, "RF", 1u, LCDIF_COLOR_GRAY);
    if (tpms->pressure_valid[1] != 0u)
    {
        LcdIf_DrawSignedX100(724u, 398u, tpms->pressure_bar_x100[1], 2u,
                            ((tpms->warning_mask & 0x02u) != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawText(724u, 402u, "--", 2u, LCDIF_COLOR_GRAY);
    }

    LcdIf_DrawText(592u, 430u, "LR", 1u, LCDIF_COLOR_GRAY);
    if (tpms->pressure_valid[2] != 0u)
    {
        LcdIf_DrawSignedX100(616u, 426u, tpms->pressure_bar_x100[2], 2u,
                            ((tpms->warning_mask & 0x04u) != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawText(616u, 430u, "--", 2u, LCDIF_COLOR_GRAY);
    }
    LcdIf_DrawText(700u, 430u, "RR", 1u, LCDIF_COLOR_GRAY);
    if (tpms->pressure_valid[3] != 0u)
    {
        LcdIf_DrawSignedX100(724u, 426u, tpms->pressure_bar_x100[3], 2u,
                            ((tpms->warning_mask & 0x08u) != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawText(724u, 430u, "--", 2u, LCDIF_COLOR_GRAY);
    }
}

static void App_Display_DrawControlDomainDetails(const Rte_ControlDomainStatusType *domain)
{
    static uint8_t cache_valid;
    static uint8_t previous_status_valid;
    static uint8_t previous_input_mode;
    static uint8_t previous_power_mode;
    static uint8_t previous_fault;
    static uint8_t previous_dtc_count;

    if ((cache_valid != 0u) &&
        (previous_status_valid == domain->status_valid) &&
        (previous_input_mode == domain->input_mode) &&
        (previous_power_mode == domain->power_mode) &&
        (previous_fault == domain->remote_fault_present) &&
        (previous_dtc_count == domain->remote_dtc_count))
    {
        return;
    }

    LcdIf_FillRect(492u, 296u, 286u, 42u, APP_DISPLAY_COLOR_BG);
    LcdIf_DrawText(492u, 300u, "IN", 2u, LCDIF_COLOR_GRAY);
    if (domain->status_valid == 0u)
    {
        LcdIf_DrawText(528u, 300u, "--", 2u, LCDIF_COLOR_YELLOW);
    }
    else if (domain->input_mode == 0u)
    {
        LcdIf_DrawText(528u, 300u, "SIM", 2u, LCDIF_COLOR_CYAN);
    }
    else if (domain->input_mode == 1u)
    {
        LcdIf_DrawText(528u, 300u, "ADC", 2u, LCDIF_COLOR_CYAN);
    }
    else if (domain->input_mode == 2u)
    {
        LcdIf_DrawText(528u, 300u, "FIX", 2u, LCDIF_COLOR_CYAN);
    }
    else
    {
        LcdIf_DrawText(528u, 300u, "INV", 2u, LCDIF_COLOR_YELLOW);
    }

    LcdIf_DrawText(590u, 300u, "PWR", 2u, LCDIF_COLOR_GRAY);
    if (domain->status_valid == 0u)
    {
        LcdIf_DrawText(638u, 300u, "--", 2u, LCDIF_COLOR_YELLOW);
    }
    else if (domain->power_mode == 0u)
    {
        LcdIf_DrawText(638u, 300u, "OFF", 2u, LCDIF_COLOR_WHITE);
    }
    else if (domain->power_mode == 1u)
    {
        LcdIf_DrawText(638u, 300u, "ACC", 2u, LCDIF_COLOR_WHITE);
    }
    else if (domain->power_mode == 2u)
    {
        LcdIf_DrawText(638u, 300u, "RUN", 2u, LCDIF_COLOR_GREEN);
    }
    else if (domain->power_mode == 3u)
    {
        LcdIf_DrawText(638u, 300u, "START", 2u, LCDIF_COLOR_YELLOW);
    }
    else
    {
        LcdIf_DrawText(638u, 300u, "INV", 2u, LCDIF_COLOR_YELLOW);
    }

    LcdIf_DrawText(718u, 300u, "RF", 2u, LCDIF_COLOR_GRAY);
    if ((domain->status_valid != 0u) && (domain->remote_fault_present != 0u))
    {
        LcdIf_DrawU32(748u, 298u, domain->remote_dtc_count, 2u, LCDIF_COLOR_RED);
    }
    else
    {
        LcdIf_DrawText(748u, 300u, "0", 2u,
                       (domain->status_valid != 0u) ? LCDIF_COLOR_GREEN : LCDIF_COLOR_YELLOW);
    }

    previous_status_valid = domain->status_valid;
    previous_input_mode = domain->input_mode;
    previous_power_mode = domain->power_mode;
    previous_fault = domain->remote_fault_present;
    previous_dtc_count = domain->remote_dtc_count;
    cache_valid = 1u;
}

static void App_Display_DrawBottomInfo(const Rte_DashboardDataType *data,
                                       const Rte_EnvironmentDataType *env,
                                       const Rte_TpmsDataType *tpms,
                                       const Rte_ConfigDataType *config)
{
    static uint8_t cache_valid;
    static uint8_t previous_fuel_percent;
    static uint8_t previous_fuel_valid;
    static int16_t previous_coolant_temp_c;
    static uint8_t previous_coolant_valid;
    static uint16_t previous_battery_mv;
    static uint8_t previous_battery_valid;
    static uint8_t previous_status_mode;
    static Rte_EnvironmentDataType previous_env;
    static Rte_TpmsDataType previous_tpms;
    static App_Display_TimeSnapshotType previous_time;
    App_Display_TimeSnapshotType time;
    uint32_t battery_bar;
    uint16_t status_color;
    uint8_t index;
    uint8_t tpms_changed;
    uint8_t status_mode;

    App_Display_GetTimeSnapshot(config, &time);
    status_mode = 0u;
    if (data->buzzer_muted != 0u)
    {
        status_mode = 1u;
    }
    else if (data->alarm_active != 0u)
    {
        status_mode = 2u;
    }

    tpms_changed = (cache_valid == 0u) || (previous_tpms.valid != tpms->valid);
    for (index = 0u; index < 4u; index++)
    {
        if ((previous_tpms.pressure_bar_x100[index] != tpms->pressure_bar_x100[index]) ||
            (previous_tpms.temperature_c[index] != tpms->temperature_c[index]))
        {
            tpms_changed = 1u;
        }
    }

    if ((cache_valid == 0u) ||
        (previous_fuel_percent != data->fuel_percent) ||
        (previous_fuel_valid != data->fuel_percent_valid))
    {
        LcdIf_FillRect(54u, 364u, 224u, 64u, APP_DISPLAY_COLOR_PANEL_DARK);
        LcdIf_DrawText(54u, 364u, "FUEL", 2u, LCDIF_COLOR_YELLOW);
        if (data->fuel_percent_valid != 0u)
        {
            App_Display_DrawBar(54u, 392u, 150u, data->fuel_percent, 100u, LCDIF_COLOR_GREEN);
            LcdIf_DrawU32(214u, 386u, data->fuel_percent, 3u, LCDIF_COLOR_WHITE);
            LcdIf_DrawText(268u, 402u, "%", 2u, LCDIF_COLOR_GRAY);
        }
        else
        {
            App_Display_DrawBar(54u, 392u, 150u, 0u, 100u, LCDIF_COLOR_GRAY);
            LcdIf_DrawText(214u, 390u, "--", 3u, LCDIF_COLOR_GRAY);
        }
        previous_fuel_percent = data->fuel_percent;
        previous_fuel_valid = data->fuel_percent_valid;
    }

    if ((cache_valid == 0u) || (App_Display_IsSameTime(&previous_time, &time) == 0u))
    {
        LcdIf_FillRect(54u, 424u, 212u, 24u, APP_DISPLAY_COLOR_PANEL_DARK);
        LcdIf_DrawText(54u, 424u, "TIME", 2u, LCDIF_COLOR_YELLOW);
        App_Display_DrawTime(128u, 424u, &time);
        previous_time = time;
    }

    if ((cache_valid == 0u) ||
        (previous_coolant_temp_c != data->coolant_temp_c) ||
        (previous_coolant_valid != data->coolant_temp_valid))
    {
        LcdIf_FillRect(312u, 364u, 258u, 56u, APP_DISPLAY_COLOR_PANEL_DARK);
        LcdIf_DrawText(312u, 364u, "COOL", 2u, LCDIF_COLOR_YELLOW);
        if (data->coolant_temp_valid != 0u)
        {
            App_Display_DrawBar(312u, 392u, 150u, App_Display_ClampTempToBar(data->coolant_temp_c), 160u,
                                (data->coolant_temp_c >= 100) ? LCDIF_COLOR_RED : APP_DISPLAY_COLOR_ORANGE);
            if (data->coolant_temp_c < 0)
            {
                LcdIf_DrawText(472u, 390u, "-", 3u, LCDIF_COLOR_WHITE);
                LcdIf_DrawU32(490u, 386u, (uint32_t)(-data->coolant_temp_c), 3u, LCDIF_COLOR_WHITE);
            }
            else
            {
                LcdIf_DrawU32(472u, 386u, (uint32_t)data->coolant_temp_c, 3u, LCDIF_COLOR_WHITE);
            }
            LcdIf_DrawText(528u, 402u, "C", 2u, LCDIF_COLOR_GRAY);
        }
        else
        {
            App_Display_DrawBar(312u, 392u, 150u, 0u, 160u, LCDIF_COLOR_GRAY);
            LcdIf_DrawText(472u, 390u, "--", 3u, LCDIF_COLOR_GRAY);
        }
        previous_coolant_temp_c = data->coolant_temp_c;
        previous_coolant_valid = data->coolant_temp_valid;
    }

    if ((cache_valid == 0u) ||
        (previous_battery_mv != data->battery_mv) ||
        (previous_battery_valid != data->battery_voltage_valid))
    {
        LcdIf_FillRect(312u, 424u, 268u, 26u, APP_DISPLAY_COLOR_PANEL_DARK);
        LcdIf_DrawText(312u, 424u, "BATT", 2u, LCDIF_COLOR_YELLOW);
        if (data->battery_voltage_valid != 0u)
        {
            battery_bar = App_Display_ClampBatteryToBar(data->battery_mv);
            App_Display_DrawBar(386u, 426u, 110u, battery_bar, 6000u,
                                (data->battery_mv < 11500u) ? LCDIF_COLOR_RED : LCDIF_COLOR_CYAN);
            LcdIf_DrawU32(510u, 420u, data->battery_mv / 1000u, 2u, LCDIF_COLOR_WHITE);
            LcdIf_DrawText(538u, 420u, ".", 2u, LCDIF_COLOR_WHITE);
            LcdIf_DrawU32(552u, 420u, (data->battery_mv / 100u) % 10u, 2u, LCDIF_COLOR_WHITE);
            LcdIf_DrawText(570u, 420u, "V", 2u, LCDIF_COLOR_GRAY);
        }
        else
        {
            App_Display_DrawBar(386u, 426u, 110u, 0u, 6000u, LCDIF_COLOR_GRAY);
            LcdIf_DrawText(510u, 420u, "--", 2u, LCDIF_COLOR_GRAY);
        }
        previous_battery_mv = data->battery_mv;
        previous_battery_valid = data->battery_voltage_valid;
    }

    if (tpms_changed != 0u)
    {
        LcdIf_FillRect(592u, 374u, 170u, 72u, APP_DISPLAY_COLOR_PANEL_DARK);
        App_Display_DrawTpms(tpms);
        previous_tpms = *tpms;
    }

    if ((cache_valid == 0u) ||
        (previous_env.valid != env->valid) ||
        (previous_env.temperature_c_x100 != env->temperature_c_x100) ||
        (previous_env.humidity_rh_x100 != env->humidity_rh_x100))
    {
        LcdIf_FillRect(54u, 314u, 280u, 24u, APP_DISPLAY_COLOR_BG);
        if (env->valid != 0u)
        {
            LcdIf_DrawText(54u, 318u, "CABIN", 2u, LCDIF_COLOR_GRAY);
            LcdIf_DrawSignedX100(132u, 314u, env->temperature_c_x100, 2u, LCDIF_COLOR_WHITE);
            LcdIf_DrawText(200u, 318u, "C", 2u, LCDIF_COLOR_GRAY);
            LcdIf_DrawSignedX100(246u, 314u, env->humidity_rh_x100, 2u, LCDIF_COLOR_WHITE);
            LcdIf_DrawText(314u, 318u, "%", 2u, LCDIF_COLOR_GRAY);
        }
        previous_env = *env;
    }

    if ((cache_valid == 0u) || (previous_status_mode != status_mode))
    {
        LcdIf_FillRect(316u, 296u, 168u, 42u, APP_DISPLAY_COLOR_BG);
        if (status_mode == 1u)
        {
            status_color = LCDIF_COLOR_YELLOW;
            App_Display_DrawCenteredText(400, 304u, "MUTE", 3u, status_color);
        }
        else if (status_mode == 2u)
        {
            status_color = LCDIF_COLOR_RED;
            App_Display_DrawCenteredText(400, 304u, "ALARM", 3u, status_color);
        }
        else
        {
            status_color = LCDIF_COLOR_GREEN;
            App_Display_DrawCenteredText(400, 304u, "NORMAL", 3u, status_color);
        }
        previous_status_mode = status_mode;
    }

    cache_valid = 1u;
}

void App_Display_MainFunction(uint32_t tick_ms)
{
    Rte_DashboardDataType data;
    Rte_EnvironmentDataType env;
    Rte_TpmsDataType tpms;
    Rte_ConfigDataType config;
    Rte_ControlDomainStatusType domain;

    (void)tick_ms;
    App_Display_UpdateBacklight();

    if (LcdIf_IsReady() == 0u)
    {
        return;
    }

    (void)Rte_Read_DashboardData(&data);
    (void)Rte_Read_Environment(&env);
    (void)Rte_Read_TpmsStatus(&tpms);
    (void)Rte_Read_ConfigData(&config);
    (void)Rte_Read_ControlDomainStatus(&domain);

    if (App_Display_StaticDrawn == 0u)
    {
        App_Display_DrawStaticBackground();
    }

    App_Display_DrawTopStatus(&data, &tpms, &domain);
    App_Display_DrawCenterSpeed(&data);
    App_Display_DrawRpmGauge((data.engine_rpm_valid != 0u) ? data.engine_rpm : 0u);
    App_Display_DrawSpeedGauge((data.vehicle_speed_valid != 0u) ?
                               data.vehicle_speed_kph_x10 : 0u);
    App_Display_CenterAreaCleared = 0u;
    App_Display_DrawGear(&data);
    App_Display_DrawBottomInfo(&data, &env, &tpms, &config);
    App_Display_DrawControlDomainDetails(&domain);
}
