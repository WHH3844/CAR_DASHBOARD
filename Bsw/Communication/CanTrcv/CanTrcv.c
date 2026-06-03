#include "CanTrcv.h"

#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

static CanTrcv_ModeType CanTrcv_Mode;

void CanTrcv_Init(void)
{
    rcu_periph_clock_enable(CAN1_CTRL_GPIO_CLK);
    rcu_periph_clock_enable(CAN1_ERR_N_GPIO_CLK);

    gpio_mode_set(CAN1_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CAN1_EN_PIN);
    gpio_output_options_set(CAN1_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CAN1_EN_PIN);

    gpio_mode_set(CAN1_STB_N_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CAN1_STB_N_PIN);
    gpio_output_options_set(CAN1_STB_N_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, CAN1_STB_N_PIN);

    gpio_mode_set(CAN1_ERR_N_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, CAN1_ERR_N_PIN);

    (void)CanTrcv_SetMode(CANTRCV_MODE_STANDBY);
}

Std_ReturnType CanTrcv_SetMode(CanTrcv_ModeType mode)
{
    if (mode == CANTRCV_MODE_NORMAL)
    {
        /*
         * SIT1043QT：EN 拉高使能收发器，STB_N 拉高退出待机。
         * 这两个脚在硬件测试中已验证，正式工程只封装成 CanTrcv 接口。
         */
        gpio_bit_set(CAN1_EN_PORT, CAN1_EN_PIN);
        gpio_bit_set(CAN1_STB_N_PORT, CAN1_STB_N_PIN);
        CanTrcv_Mode = CANTRCV_MODE_NORMAL;
        return E_OK;
    }

    gpio_bit_set(CAN1_EN_PORT, CAN1_EN_PIN);
    gpio_bit_reset(CAN1_STB_N_PORT, CAN1_STB_N_PIN);
    CanTrcv_Mode = CANTRCV_MODE_STANDBY;
    return E_OK;
}

CanTrcv_ModeType CanTrcv_GetMode(void)
{
    return CanTrcv_Mode;
}

uint8_t CanTrcv_IsErrorAsserted(void)
{
    return (gpio_input_bit_get(CAN1_ERR_N_PORT, CAN1_ERR_N_PIN) == RESET) ? 1u : 0u;
}
