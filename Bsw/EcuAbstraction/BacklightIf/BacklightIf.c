#include "BacklightIf.h"

#include "LcdTli.h"

static uint8_t BacklightIf_Level;

void BacklightIf_Init(void)
{
    BacklightIf_Level = 0u;
    LcdTli_BacklightOff();
}

void BacklightIf_SetLevel(uint8_t level)
{
    if (level > 100u)
    {
        level = 100u;
    }

    BacklightIf_Level = level;

    /*
     * 当前硬件测试阶段背光只验证了 GPIO 开关。
     * 先把 0 视为关闭，1~100 视为打开；后续接 PWM 时保持接口不变。
     */
    if (level == 0u)
    {
        LcdTli_BacklightOff();
    }
    else
    {
        LcdTli_BacklightOn();
    }
}

uint8_t BacklightIf_GetLevel(void)
{
    return BacklightIf_Level;
}
#include "BacklightIf.h"

#include "LcdTli.h"

static uint8_t BacklightIf_Level;

void BacklightIf_Init(void)
{
    BacklightIf_Level = 0u;
    LcdTli_BacklightOff();
}

void BacklightIf_SetLevel(uint8_t level)
{
    /*
     * 当前硬件第一版背光只按 GPIO 开关控制。
     * 这里仍保留 0~100 的亮度接口，后续换成 PWM 时 APP 不需要改。
     */
    BacklightIf_Level = (level > 100u) ? 100u : level;

    if (BacklightIf_Level == 0u)
    {
        LcdTli_BacklightOff();
    }
    else
    {
        LcdTli_BacklightOn();
    }
}

uint8_t BacklightIf_GetLevel(void)
{
    return BacklightIf_Level;
}

void BacklightIf_Off(void)
{
    BacklightIf_SetLevel(0u);
}
