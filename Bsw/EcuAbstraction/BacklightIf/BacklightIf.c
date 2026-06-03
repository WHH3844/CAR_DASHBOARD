#include "BacklightIf.h"

#include "LcdTli.h"

/* 保存逻辑背光百分比，方便上层查询最近设置值。 */
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
     * 这样 NvM 仍可保存百分比配置，未来升级 PWM 不需要迁移配置格式。
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
