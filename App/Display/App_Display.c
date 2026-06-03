#include "App_Display.h"

#include "LcdIf.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

static uint8_t App_Display_LayoutDrawn;

void App_Display_Init(void)
{
    uint8_t level;

    App_Display_LayoutDrawn = 0u;
    if (Rte_Read_BacklightLevel(&level) == E_OK)
    {
        Rte_Call_Backlight_Set(level);
    }
}

static void App_Display_DrawLayout(void)
{
    if (LcdIf_IsReady() == 0u)
    {
        return;
    }

    /*
     * 固定框架只画一次，运行中只擦小区域刷新。
     * 这是前面 dashboard_demo 解决 LCD 花屏/撕裂后保留下来的经验。
     */
    LcdIf_Clear(0x0842u);
    LcdIf_FillRect(0u, 0u, 800u, 58u, 0x18E3u);
    LcdIf_DrawText(24u, 16u, "CAR DASHBOARD", 4u, LCDIF_COLOR_WHITE);

    LcdIf_DrawText(48u, 92u, "SPEED", 4u, LCDIF_COLOR_GRAY);
    LcdIf_DrawText(430u, 92u, "RPM", 4u, LCDIF_COLOR_GRAY);
    LcdIf_DrawText(48u, 330u, "TIME", 3u, LCDIF_COLOR_GRAY);
    LcdIf_DrawText(300u, 330u, "TEMP", 3u, LCDIF_COLOR_GRAY);
    LcdIf_DrawText(540u, 330u, "HUM", 3u, LCDIF_COLOR_GRAY);
    LcdIf_DrawText(48u, 440u, "KEY1 SIM  KEY2 MUTE  KEY3 CLEAR", 2u, LCDIF_COLOR_GRAY);
    App_Display_LayoutDrawn = 1u;
}

static void App_Display_DrawTime(uint32_t x, uint32_t y)
{
    RtcIf_TimeType time;
    uint8_t valid;

    (void)Rte_Read_RtcTime(&time, &valid);
    LcdIf_FillRect(x, y, 210u, 28u, 0x0842u);

    if (valid == 0u)
    {
        LcdIf_DrawText(x, y, "NO RTC", 3u, LCDIF_COLOR_YELLOW);
        return;
    }

    LcdIf_DrawU32(x, y, time.hour, 3u, LCDIF_COLOR_WHITE);
    LcdIf_DrawText(x + 38u, y, ":", 3u, LCDIF_COLOR_WHITE);
    if (time.minute < 10u)
    {
        LcdIf_DrawText(x + 62u, y, "0", 3u, LCDIF_COLOR_WHITE);
        LcdIf_DrawU32(x + 80u, y, time.minute, 3u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawU32(x + 62u, y, time.minute, 3u, LCDIF_COLOR_WHITE);
    }
    LcdIf_DrawText(x + 120u, y, ":", 3u, LCDIF_COLOR_WHITE);
    if (time.second < 10u)
    {
        LcdIf_DrawText(x + 144u, y, "0", 3u, LCDIF_COLOR_WHITE);
        LcdIf_DrawU32(x + 162u, y, time.second, 3u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawU32(x + 144u, y, time.second, 3u, LCDIF_COLOR_WHITE);
    }
}

static void App_Display_DrawEnvironment(void)
{
    Rte_EnvironmentDataType env;

    (void)Rte_Read_Environment(&env);

    LcdIf_FillRect(300u, 365u, 190u, 28u, 0x0842u);
    LcdIf_FillRect(540u, 365u, 190u, 28u, 0x0842u);

    if (env.valid == 0u)
    {
        LcdIf_DrawText(300u, 365u, "NO SHT", 3u, LCDIF_COLOR_YELLOW);
        LcdIf_DrawText(540u, 365u, "NO SHT", 3u, LCDIF_COLOR_YELLOW);
        return;
    }

    LcdIf_DrawSignedX100(300u, 365u, env.temperature_c_x100, 3u, LCDIF_COLOR_WHITE);
    LcdIf_DrawText(425u, 365u, "C", 3u, LCDIF_COLOR_WHITE);
    LcdIf_DrawSignedX100(540u, 365u, env.humidity_rh_x100, 3u, LCDIF_COLOR_WHITE);
    LcdIf_DrawText(665u, 365u, "%", 3u, LCDIF_COLOR_WHITE);
}

void App_Display_MainFunction(uint32_t tick_ms)
{
    Rte_DashboardDataType data;
    uint16_t speed_color;
    uint16_t rpm_color;

    (void)tick_ms;

    if (LcdIf_IsReady() == 0u)
    {
        return;
    }

    if (App_Display_LayoutDrawn == 0u)
    {
        App_Display_DrawLayout();
    }

    (void)Rte_Read_DashboardData(&data);
    speed_color = (data.alarm_active != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_CYAN;
    rpm_color = (data.alarm_active != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_GREEN;

    LcdIf_FillRect(48u, 145u, 300u, 100u, 0x0842u);
    LcdIf_DrawU32(48u, 145u, data.vehicle_speed_kph_x10 / 10u, 12u, speed_color);
    LcdIf_DrawText(245u, 210u, "KMH", 4u, LCDIF_COLOR_GRAY);

    LcdIf_FillRect(430u, 145u, 320u, 100u, 0x0842u);
    LcdIf_DrawU32(430u, 145u, data.engine_rpm, 9u, rpm_color);
    LcdIf_DrawText(650u, 210u, "RPM", 4u, LCDIF_COLOR_GRAY);

    LcdIf_FillRect(570u, 16u, 210u, 30u, 0x18E3u);
    if (data.simulation_mode != 0u)
    {
        LcdIf_DrawText(570u, 18u, "SIM MODE", 3u, LCDIF_COLOR_YELLOW);
    }
    else if (data.can_ems_valid != 0u)
    {
        LcdIf_DrawText(570u, 18u, "CAN OK", 3u, LCDIF_COLOR_GREEN);
    }
    else
    {
        LcdIf_DrawText(570u, 18u, "CAN LOST", 3u, LCDIF_COLOR_YELLOW);
    }

    LcdIf_FillRect(48u, 365u, 210u, 28u, 0x0842u);
    App_Display_DrawTime(48u, 365u);
    App_Display_DrawEnvironment();

    LcdIf_FillRect(390u, 405u, 330u, 28u, 0x0842u);
    if (data.buzzer_muted != 0u)
    {
        LcdIf_DrawText(390u, 405u, "MUTE", 3u, LCDIF_COLOR_YELLOW);
    }
    else if (data.alarm_active != 0u)
    {
        LcdIf_DrawText(390u, 405u, "ALARM", 3u, LCDIF_COLOR_RED);
    }
    else
    {
        LcdIf_DrawText(390u, 405u, "NORMAL", 3u, LCDIF_COLOR_GREEN);
    }
}
#include "App_Display.h"

#include "BacklightIf.h"
#include "LcdIf.h"
#include "Rte_Signal.h"

static uint8_t App_DisplayLayoutDrawn;

static void App_Display_DrawLayout(void)
{
    LcdIf_Clear(LCDIF_COLOR_DARK);
    LcdIf_FillRect(0u, 0u, 800u, 58u, LCDIF_COLOR_PANEL);
    LcdIf_DrawString(24u, 15u, "CAR DASHBOARD", 4u, LCDIF_COLOR_WHITE);

    LcdIf_FillRect(28u, 78u, 354u, 220u, LCDIF_COLOR_BLACK);
    LcdIf_FillRect(418u, 78u, 354u, 220u, LCDIF_COLOR_BLACK);
    LcdIf_DrawString(48u, 96u, "SPEED", 4u, LCDIF_COLOR_WHITE);
    LcdIf_DrawString(438u, 96u, "RPM", 4u, LCDIF_COLOR_WHITE);

    LcdIf_FillRect(28u, 330u, 744u, 112u, LCDIF_COLOR_PANEL);
    LcdIf_DrawString(48u, 352u, "TIME", 3u, LCDIF_COLOR_CYAN);
    LcdIf_DrawString(390u, 352u, "TEMP", 3u, LCDIF_COLOR_CYAN);
    LcdIf_DrawString(48u, 392u, "HUM", 3u, LCDIF_COLOR_CYAN);
    LcdIf_DrawString(390u, 392u, "CAN", 3u, LCDIF_COLOR_CYAN);

    App_DisplayLayoutDrawn = 1u;
}

static void App_Display_DrawTwoDigits(uint32_t x, uint32_t y, uint8_t value)
{
    if (value < 10u)
    {
        LcdIf_DrawU32(x, y, 0u, 3u, LCDIF_COLOR_WHITE);
        LcdIf_DrawU32(x + 18u, y, value, 3u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawU32(x, y, value, 3u, LCDIF_COLOR_WHITE);
    }
}

static void App_Display_DrawTime(const Rte_DashboardDataType *data)
{
    if (data->rtc_valid == 0u)
    {
        LcdIf_DrawString(145u, 352u, "NO RTC", 3u, LCDIF_COLOR_YELLOW);
        return;
    }

    App_Display_DrawTwoDigits(145u, 352u, data->rtc_time.hour);
    LcdIf_DrawString(181u, 352u, ":", 3u, LCDIF_COLOR_WHITE);
    App_Display_DrawTwoDigits(199u, 352u, data->rtc_time.minute);
    LcdIf_DrawString(235u, 352u, ":", 3u, LCDIF_COLOR_WHITE);
    App_Display_DrawTwoDigits(253u, 352u, data->rtc_time.second);
}

static void App_Display_DrawX10(uint32_t x, uint32_t y, uint32_t value_x10, uint16_t color)
{
    LcdIf_DrawU32(x, y, value_x10 / 10u, 3u, color);
    LcdIf_DrawString(x + 54u, y, ".", 3u, color);
    LcdIf_DrawU32(x + 72u, y, value_x10 % 10u, 3u, color);
}

void App_Display_Init(void)
{
    App_DisplayLayoutDrawn = 0u;

    if (LcdIf_Init() == E_OK)
    {
        BacklightIf_Init();
        BacklightIf_SetLevel(100u);
        App_Display_DrawLayout();
    }
}

void App_Display_MainFunction(void)
{
    Rte_DashboardDataType data;
    uint16_t speed_color;
    uint16_t rpm_color;

    if (LcdIf_IsReady() == 0u)
    {
        return;
    }

    if (App_DisplayLayoutDrawn == 0u)
    {
        App_Display_DrawLayout();
    }

    if (Rte_Read_DashboardData(&data) != E_OK)
    {
        return;
    }

    speed_color = (data.speed_kph_x10 >= 1200u) ? LCDIF_COLOR_RED : LCDIF_COLOR_CYAN;
    rpm_color = (data.engine_rpm >= 5000u) ? LCDIF_COLOR_RED : LCDIF_COLOR_GREEN;

    /*
     * 只刷新数字区域，不高频整屏清屏。
     * 这是前面 LCD 花屏问题复盘后的经验：TLI 正在读 SDRAM，软件整屏重画会抢带宽。
     */
    LcdIf_FillRect(52u, 145u, 290u, 100u, LCDIF_COLOR_BLACK);
    LcdIf_DrawU32(52u, 145u, data.speed_kph_x10 / 10u, 12u, speed_color);
    LcdIf_DrawString(250u, 208u, "KMH", 4u, LCDIF_COLOR_WHITE);

    LcdIf_FillRect(438u, 145u, 300u, 100u, LCDIF_COLOR_BLACK);
    LcdIf_DrawU32(438u, 145u, data.engine_rpm, 10u, rpm_color);

    LcdIf_FillRect(145u, 350u, 210u, 30u, LCDIF_COLOR_PANEL);
    App_Display_DrawTime(&data);

    LcdIf_FillRect(500u, 350u, 230u, 30u, LCDIF_COLOR_PANEL);
    if (data.sensor_valid != 0u)
    {
        App_Display_DrawX10(500u, 352u, (uint32_t)data.sht30_temp_c_x10, LCDIF_COLOR_WHITE);
        LcdIf_DrawString(590u, 352u, "C", 3u, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawString(500u, 352u, "NO SHT", 3u, LCDIF_COLOR_YELLOW);
    }

    LcdIf_FillRect(145u, 390u, 210u, 30u, LCDIF_COLOR_PANEL);
    if (data.sensor_valid != 0u)
    {
        App_Display_DrawX10(145u, 392u, data.sht30_humidity_x10, LCDIF_COLOR_WHITE);
    }
    else
    {
        LcdIf_DrawString(145u, 392u, "NO SHT", 3u, LCDIF_COLOR_YELLOW);
    }

    LcdIf_FillRect(455u, 390u, 180u, 30u, LCDIF_COLOR_PANEL);
    if (data.can_ems_valid != 0u)
    {
        LcdIf_DrawString(455u, 392u, "CAN OK", 3u, LCDIF_COLOR_GREEN);
    }
    else
    {
        LcdIf_DrawString(455u, 392u, "CAN LOST", 3u, LCDIF_COLOR_YELLOW);
    }
}
