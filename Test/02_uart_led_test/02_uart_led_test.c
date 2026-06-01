#include "uart_led_test.h"

#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

static void Test02_DelayMs(uint32_t ms)
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

static void Test02_Led_Init(void)
{
    rcu_periph_clock_enable(STATUS_LED_R_GPIO_CLK);
    rcu_periph_clock_enable(STATUS_LED_G_GPIO_CLK);
    rcu_periph_clock_enable(STATUS_LED_B_GPIO_CLK);

    gpio_mode_set(STATUS_LED_R_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STATUS_LED_R_PIN);
    gpio_mode_set(STATUS_LED_G_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STATUS_LED_G_PIN);
    gpio_mode_set(STATUS_LED_B_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STATUS_LED_B_PIN);

    gpio_output_options_set(STATUS_LED_R_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, STATUS_LED_R_PIN);
    gpio_output_options_set(STATUS_LED_G_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, STATUS_LED_G_PIN);
    gpio_output_options_set(STATUS_LED_B_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, STATUS_LED_B_PIN);

    gpio_bit_set(STATUS_LED_R_PORT, STATUS_LED_R_PIN);
    gpio_bit_set(STATUS_LED_G_PORT, STATUS_LED_G_PIN);
    gpio_bit_set(STATUS_LED_B_PORT, STATUS_LED_B_PIN);
}

static void Test02_Led_Set(uint8_t red_on, uint8_t green_on, uint8_t blue_on)
{
    if (red_on != 0u)
    {
        gpio_bit_reset(STATUS_LED_R_PORT, STATUS_LED_R_PIN);
    }
    else
    {
        gpio_bit_set(STATUS_LED_R_PORT, STATUS_LED_R_PIN);
    }

    if (green_on != 0u)
    {
        gpio_bit_reset(STATUS_LED_G_PORT, STATUS_LED_G_PIN);
    }
    else
    {
        gpio_bit_set(STATUS_LED_G_PORT, STATUS_LED_G_PIN);
    }

    if (blue_on != 0u)
    {
        gpio_bit_reset(STATUS_LED_B_PORT, STATUS_LED_B_PIN);
    }
    else
    {
        gpio_bit_set(STATUS_LED_B_PORT, STATUS_LED_B_PIN);
    }
}

static void Test02_RunStep(const char *message, uint8_t red_on, uint8_t green_on, uint8_t blue_on)
{
    uint32_t i;

    Uart_DebugPuts(message);
    Test02_Led_Set(red_on, green_on, blue_on);

    for (i = 0u; i < 50u; i++)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test02_DelayMs(10u);
    }
}

void Test02_UartLed_Run(void)
{
    Uart_DebugInit();
    Test02_Led_Init();

    Uart_DebugPuts("\n[BOOT] 02_uart_led_test start\n");
    Uart_DebugPuts("[BOOT] PWR_HOLD on\n");
    Uart_DebugPuts("[INFO] UART 115200 8N1, RGB LED active low\n");

    while (1)
    {
        Test02_RunStep("[BOOT] boot ok, LED red\n", 1u, 0u, 0u);
        Test02_RunStep("[BOOT] boot ok, LED green\n", 0u, 1u, 0u);
        Test02_RunStep("[BOOT] boot ok, LED blue\n", 0u, 0u, 1u);
        Test02_RunStep("[BOOT] boot ok, LED white\n", 1u, 1u, 1u);
        Test02_RunStep("[BOOT] boot ok, LED off\n", 0u, 0u, 0u);
    }
}
