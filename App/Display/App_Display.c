#include "App_Display.h"

#include "LcdIf.h"
#include "Rte_Service.h"
#include "Rte_Signal.h"

/*
 * 记录静态布局是否已经绘制。
 * LCD framebuffer 位于外部 SDRAM，整屏刷新成本较高，所以运行期尽量只重绘变化区域。
 */
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
    /*
     * 先擦除固定宽度区域，再绘制新时间，避免分钟/秒从两位变一位时残留旧像素。
     */
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
        /* LcdIf_DrawU32 不会自动补零，时间显示需要手动补齐两位。 */
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
        /* 秒字段同样补零，保持 HH:MM:SS 的固定视觉宽度。 */
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

    /*
     * 温度和湿度各自擦除固定区域。
     * SHT30 数据用 x100 定点数保存，显示层负责补小数点和单位。
     */
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
    /*
     * 报警状态直接影响核心数字颜色，让驾驶员无需读文字也能看到超限。
     * 当前只对车速/转速变色，底部状态文字再解释是 MUTE/ALARM/NORMAL。
     */
    speed_color = (data.alarm_active != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_CYAN;
    rpm_color = (data.alarm_active != 0u) ? LCDIF_COLOR_RED : LCDIF_COLOR_GREEN;

    /* 车速和转速是最频繁变化的区域，每次刷新只擦除对应数字框。 */
    LcdIf_FillRect(48u, 145u, 300u, 100u, 0x0842u);
    LcdIf_DrawU32(48u, 145u, data.vehicle_speed_kph_x10 / 10u, 12u, speed_color);
    LcdIf_DrawText(245u, 210u, "KMH", 4u, LCDIF_COLOR_GRAY);

    LcdIf_FillRect(430u, 145u, 320u, 100u, 0x0842u);
    LcdIf_DrawU32(430u, 145u, data.engine_rpm, 9u, rpm_color);
    LcdIf_DrawText(650u, 210u, "RPM", 4u, LCDIF_COLOR_GRAY);

    /*
     * 顶部状态优先级：SIM MODE > CAN OK > CAN LOST。
     * 模拟模式下即使 CAN 丢失也不显示 CAN LOST，避免误导调试人员。
     */
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

    /* 底部状态区表达蜂鸣器和报警关系，静音优先于报警文字。 */
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
