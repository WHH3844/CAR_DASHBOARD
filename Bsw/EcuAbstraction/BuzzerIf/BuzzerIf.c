#include "BuzzerIf.h"

#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

void BuzzerIf_Init(void)
{
    rcu_periph_clock_enable(BUZZER_GPIO_CLK);

    gpio_mode_set(BUZZER_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BUZZER_PIN);
    gpio_output_options_set(BUZZER_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BUZZER_PIN);

    BuzzerIf_Off();
}

void BuzzerIf_On(void)
{
    gpio_bit_set(BUZZER_PORT, BUZZER_PIN);
}

void BuzzerIf_Off(void)
{
    gpio_bit_reset(BUZZER_PORT, BUZZER_PIN);
}

void BuzzerIf_Set(uint8_t on)
{
    if (on != 0u)
    {
        BuzzerIf_On();
    }
    else
    {
        BuzzerIf_Off();
    }
}
