#include "PowerIf.h"
#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

/*
 * 上电早期还没有启动调度器和正常延时服务。
 * 这里的短忙等延时只用于电源键去抖。
 */
static void PowerIf_DelayMs(uint32_t ms)
{
    uint32_t i;

    while (ms-- != 0u)
    {
        for (i = 0u; i < 7200u; i++)
        {
            __NOP();
        }
    }
}

void PowerIf_Init(void)
{
    rcu_periph_clock_enable(KEY_POWER_GPIO_CLK);
    rcu_periph_clock_enable(PWR_HOLD_GPIO_CLK);

    gpio_mode_set(KEY_POWER_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY_POWER_PIN);

    gpio_mode_set(PWR_HOLD_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, PWR_HOLD_PIN);
    gpio_output_options_set(PWR_HOLD_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PWR_HOLD_PIN);

    PowerIf_HoldOff();
}

void PowerIf_HoldOn(void)
{
    gpio_bit_set(PWR_HOLD_PORT, PWR_HOLD_PIN);
}

void PowerIf_HoldOff(void)
{
    gpio_bit_reset(PWR_HOLD_PORT, PWR_HOLD_PIN);
}

uint8_t PowerIf_KeyIsPressed(void)
{
    return (gpio_input_bit_get(KEY_POWER_PORT, KEY_POWER_PIN) == RESET) ? 1u : 0u;
}

uint8_t PowerIf_BootCheckAndHold(void)
{
    PowerIf_Init();
    PowerIf_DelayMs(30u);

    if (PowerIf_KeyIsPressed() != 0u)
    {
        PowerIf_HoldOn();
        return 1u;
    }

    PowerIf_HoldOff();
    return 0u;
}

uint8_t PowerIf_WaitKeyPressAndHold(uint32_t timeout_ms)
{
    uint32_t waited_ms = 0u;

    while (1)
    {
        if (PowerIf_KeyIsPressed() != 0u)
        {
            PowerIf_DelayMs(30u);
            if (PowerIf_KeyIsPressed() != 0u)
            {
                PowerIf_HoldOn();
                return 1u;
            }
        }

        if ((timeout_ms != 0u) && (waited_ms >= timeout_ms))
        {
            PowerIf_HoldOff();
            return 0u;
        }

        PowerIf_DelayMs(10u);

        if (timeout_ms != 0u)
        {
            waited_ms += 10u;
        }
    }
}

uint8_t PowerIf_WaitKeyRelease(uint32_t timeout_ms)
{
    uint32_t waited_ms = 0u;

    while (PowerIf_KeyIsPressed() != 0u)
    {
        if (waited_ms >= timeout_ms)
        {
            return 0u;
        }

        PowerIf_DelayMs(10u);
        waited_ms += 10u;
    }

    return 1u;
}

uint8_t PowerIf_LongPressShutdownTask(uint32_t long_press_ms, uint32_t sample_ms)
{
    static uint32_t pressed_ms = 0u;
    static uint8_t key_released_seen = 0u;

    if (sample_ms == 0u)
    {
        sample_ms = 1u;
    }

    if (PowerIf_KeyIsPressed() == 0u)
    {
        key_released_seen = 1u;
        pressed_ms = 0u;
        PowerIf_DelayMs(sample_ms);
        return 0u;
    }

    if (key_released_seen == 0u)
    {
        PowerIf_DelayMs(sample_ms);
        return 0u;
    }

    if (pressed_ms >= long_press_ms)
    {
        PowerIf_Shutdown();
        return 1u;
    }

    pressed_ms += sample_ms;
    PowerIf_DelayMs(sample_ms);
    return 0u;
}

void PowerIf_Shutdown(void)
{
    PowerIf_HoldOff();

    while (PowerIf_KeyIsPressed() != 0u)
    {
        PowerIf_DelayMs(10u);
    }

    /*
     * 在真实软开关供电场景下，松开 KEY_POWER 后整板应该掉电。
     * 调试时 ST-LINK 或 USB-TTL 可能反向供 3.3V，让 MCU 继续活着。
     * 这种情况下重新等待下一次电源键；再次按下时先拉高 PWR_HOLD，
     * 再触发软件复位，让下一次开机表现得像冷启动。
     */
    while (1)
    {
        if (PowerIf_KeyIsPressed() != 0u)
        {
            PowerIf_DelayMs(30u);

            if (PowerIf_KeyIsPressed() != 0u)
            {
                PowerIf_HoldOn();
                NVIC_SystemReset();
            }
        }

        PowerIf_DelayMs(10u);
    }
}
