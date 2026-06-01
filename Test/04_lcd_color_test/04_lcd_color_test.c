#include "lcd_color_test.h"

#include "Exmc.h"
#include "LcdTli.h"
#include "PowerIf.h"
#include "Uart.h"
#include "board_pins.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

#include <stdint.h>

#define TEST04_LCD_PIN_TOGGLE_DIAG       0u

typedef struct
{
    const char *name;
    uint16_t color;
} Test04_ColorStepType;

static void Test04_DelayMs(uint32_t ms)
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

static void Test04_WaitWithPowerCheck(uint32_t ms)
{
    uint32_t elapsed;

    elapsed = 0u;
    while (elapsed < ms)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
        Test04_DelayMs(10u);
        elapsed += 10u;
    }
}

#if TEST04_LCD_PIN_TOGGLE_DIAG
static void Test04_GpioOutputInit(uint32_t gpio_periph, uint32_t pins)
{
    gpio_mode_set(gpio_periph, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pins);
    gpio_output_options_set(gpio_periph, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pins);
}

static void Test04_LcdPinDiagInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);

    Test04_GpioOutputInit(GPIOA, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 |
                                 GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12);
    Test04_GpioOutputInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_8 | GPIO_PIN_9 |
                                 GPIO_PIN_10 | GPIO_PIN_11);
    Test04_GpioOutputInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
    Test04_GpioOutputInit(GPIOD, GPIO_PIN_3 | GPIO_PIN_12 | GPIO_PIN_13);
    Test04_GpioOutputInit(GPIOF, GPIO_PIN_10);
    Test04_GpioOutputInit(GPIOG, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10 |
                                 GPIO_PIN_11 | GPIO_PIN_12);

    /* LCD_RST 和 LCD_BLK 保持高电平，避免诊断时复位屏幕或关闭背光。 */
    gpio_bit_set(GPIOD, GPIO_PIN_12 | GPIO_PIN_13);
}

static void Test04_LcdPinDiagWrite(uint8_t level)
{
    if (level != 0u)
    {
        gpio_bit_set(GPIOA, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 |
                            GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12);
        gpio_bit_set(GPIOB, GPIO_PIN_0 | GPIO_PIN_8 | GPIO_PIN_9 |
                            GPIO_PIN_10 | GPIO_PIN_11);
        gpio_bit_set(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
        gpio_bit_set(GPIOD, GPIO_PIN_3);
        gpio_bit_set(GPIOF, GPIO_PIN_10);
        gpio_bit_set(GPIOG, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10 |
                            GPIO_PIN_11 | GPIO_PIN_12);
    }
    else
    {
        gpio_bit_reset(GPIOA, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 |
                              GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12);
        gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_8 | GPIO_PIN_9 |
                              GPIO_PIN_10 | GPIO_PIN_11);
        gpio_bit_reset(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
        gpio_bit_reset(GPIOD, GPIO_PIN_3);
        gpio_bit_reset(GPIOF, GPIO_PIN_10);
        gpio_bit_reset(GPIOG, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10 |
                              GPIO_PIN_11 | GPIO_PIN_12);
    }

    gpio_bit_set(GPIOD, GPIO_PIN_12 | GPIO_PIN_13);
}

static void Test04_LcdPinDiagRun(void)
{
    uint8_t level;

    level = 0u;
    Uart_DebugPuts("[DIAG] LCD RGB/TLI pins toggle as GPIO\n");
    Uart_DebugPuts("[DIAG] measure: PG7=CLK PF10=DE PC6=HSYNC PA4=VSYNC\n");
    Uart_DebugPuts("[DIAG] also check RGB pins on FPC, level changes every 500ms\n");
    Test04_LcdPinDiagInit();

    while (1)
    {
        Test04_LcdPinDiagWrite(level);
        Uart_DebugPuts((level != 0u) ? "[DIAG] pins high\n" : "[DIAG] pins low\n");
        Test04_WaitWithPowerCheck(500u);
        level = (level == 0u) ? 1u : 0u;
    }
}
#endif

#if !TEST04_LCD_PIN_TOGGLE_DIAG
static void Test04_PrintColor(const char *name)
{
    Uart_DebugPuts("[LCD] show ");
    Uart_DebugPuts(name);
    Uart_DebugPuts("\n");
}
#endif

void Test04_LcdColor_Run(void)
{
#if TEST04_LCD_PIN_TOGGLE_DIAG
    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 04_lcd_pin_diag start\n");
    Test04_LcdPinDiagRun();
#else
    static const Test04_ColorStepType colors[] =
    {
        {"red", LCD_TLI_RGB565_RED},
        {"green", LCD_TLI_RGB565_GREEN},
        {"blue", LCD_TLI_RGB565_BLUE},
        {"white", LCD_TLI_RGB565_WHITE},
        {"black", LCD_TLI_RGB565_BLACK}
    };
    uint32_t framebuffer;
    uint32_t index;

    Uart_DebugInit();
    Uart_DebugPuts("\n[BOOT] 04_lcd_color_test start\n");
    Uart_DebugPuts("[INFO] RGB/TLI LCD 800x480, framebuffer in SDRAM\n");

    if (Exmc_SdramInit() == 0u)
    {
        Uart_DebugPuts("[FAIL] SDRAM init for LCD framebuffer\n");
        while (1)
        {
            Test04_WaitWithPowerCheck(100u);
        }
    }

    framebuffer = Exmc_SdramBase();
    LcdTli_FillColor(framebuffer, LCD_TLI_RGB565_BLACK);

    if (LcdTli_Init(framebuffer) == 0u)
    {
        Uart_DebugPuts("[FAIL] TLI init\n");
        while (1)
        {
            Test04_WaitWithPowerCheck(100u);
        }
    }

    LcdTli_BacklightOn();

    Uart_DebugPuts("[PASS] LCD TLI init\n");
    Uart_DebugPuts("[INFO] size=");
    Uart_DebugPutDec(LCD_TLI_WIDTH);
    Uart_DebugPuts("x");
    Uart_DebugPutDec(LCD_TLI_HEIGHT);
    Uart_DebugPuts(" framebuffer=");
    Uart_DebugPutHex32(framebuffer);
    Uart_DebugPuts("\n");

    while (1)
    {
        for (index = 0u; index < (sizeof(colors) / sizeof(colors[0])); index++)
        {
            LcdTli_FillColor(framebuffer, colors[index].color);
            Test04_PrintColor(colors[index].name);
            Test04_WaitWithPowerCheck(1000u);
        }

        LcdTli_DrawColorBars(framebuffer);
        Test04_PrintColor("color bars");
        Test04_WaitWithPowerCheck(2000u);
    }
#endif
}
