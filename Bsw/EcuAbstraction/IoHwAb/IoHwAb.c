#include "IoHwAb.h"

#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

void IoHwAb_KeyInit(void)
{
    rcu_periph_clock_enable(USER_KEY_GPIO_CLK);

    gpio_mode_set(USER_KEY1_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, USER_KEY1_PIN);
    gpio_mode_set(USER_KEY2_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, USER_KEY2_PIN);
    gpio_mode_set(USER_KEY3_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, USER_KEY3_PIN);
}

uint8_t IoHwAb_Key1IsPressed(void)
{
    return (gpio_input_bit_get(USER_KEY1_PORT, USER_KEY1_PIN) == RESET) ? 1u : 0u;
}

uint8_t IoHwAb_Key2IsPressed(void)
{
    return (gpio_input_bit_get(USER_KEY2_PORT, USER_KEY2_PIN) == RESET) ? 1u : 0u;
}

uint8_t IoHwAb_Key3IsPressed(void)
{
    return (gpio_input_bit_get(USER_KEY3_PORT, USER_KEY3_PIN) == RESET) ? 1u : 0u;
}

uint8_t IoHwAb_ReadUserKeyMask(void)
{
    uint8_t mask;

    mask = 0u;
    if (IoHwAb_Key1IsPressed() != 0u)
    {
        mask |= IOHWAB_KEY1_MASK;
    }

    if (IoHwAb_Key2IsPressed() != 0u)
    {
        mask |= IOHWAB_KEY2_MASK;
    }

    if (IoHwAb_Key3IsPressed() != 0u)
    {
        mask |= IOHWAB_KEY3_MASK;
    }

    return mask;
}
