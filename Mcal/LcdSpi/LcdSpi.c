#include "LcdSpi.h"

#include "board_pins.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

#define LCD_SPI_Y_OFFSET            20u
#define LCD_SPI_CMD                 0u
#define LCD_SPI_DATA                1u

static void LcdSpi_DelayMs(uint32_t ms)
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

static void LcdSpi_BitDelay(void)
{
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static void LcdSpi_GpioInit(void)
{
    rcu_periph_clock_enable(LCD_SPI_GPIO_CLK);
    rcu_periph_clock_enable(LCD_RST_GPIO_CLK);
    rcu_periph_clock_enable(LCD_BLK_GPIO_CLK);

    gpio_mode_set(LCD_SPI_SCK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_SPI_SCK_PIN);
    gpio_output_options_set(LCD_SPI_SCK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_SPI_SCK_PIN);

    gpio_mode_set(LCD_SPI_MOSI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_SPI_MOSI_PIN);
    gpio_output_options_set(LCD_SPI_MOSI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_SPI_MOSI_PIN);

    gpio_mode_set(LCD_SPI_NSS_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_SPI_NSS_PIN);
    gpio_output_options_set(LCD_SPI_NSS_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_SPI_NSS_PIN);

    gpio_mode_set(LCD_SPI_MISO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, LCD_SPI_MISO_PIN);

    gpio_mode_set(LCD_RST_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_RST_PIN);
    gpio_output_options_set(LCD_RST_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_RST_PIN);

    gpio_mode_set(LCD_BLK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_BLK_PIN);
    gpio_output_options_set(LCD_BLK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_BLK_PIN);

    gpio_bit_set(LCD_SPI_NSS_PORT, LCD_SPI_NSS_PIN);
    gpio_bit_set(LCD_SPI_SCK_PORT, LCD_SPI_SCK_PIN);
    gpio_bit_reset(LCD_SPI_MOSI_PORT, LCD_SPI_MOSI_PIN);
    gpio_bit_set(LCD_RST_PORT, LCD_RST_PIN);
    LcdSpi_BacklightOff();
}

static void LcdSpi_Select(void)
{
    gpio_bit_reset(LCD_SPI_NSS_PORT, LCD_SPI_NSS_PIN);
}

static void LcdSpi_Unselect(void)
{
    gpio_bit_set(LCD_SPI_NSS_PORT, LCD_SPI_NSS_PIN);
}

static void LcdSpi_WriteBit(uint8_t bit)
{
    gpio_bit_reset(LCD_SPI_SCK_PORT, LCD_SPI_SCK_PIN);

    if (bit != 0u)
    {
        gpio_bit_set(LCD_SPI_MOSI_PORT, LCD_SPI_MOSI_PIN);
    }
    else
    {
        gpio_bit_reset(LCD_SPI_MOSI_PORT, LCD_SPI_MOSI_PIN);
    }

    LcdSpi_BitDelay();
    gpio_bit_set(LCD_SPI_SCK_PORT, LCD_SPI_SCK_PIN);
    LcdSpi_BitDelay();
}

static void LcdSpi_Write9(uint8_t dc, uint8_t data)
{
    uint8_t mask;

    LcdSpi_WriteBit(dc);

    mask = 0x80u;
    while (mask != 0u)
    {
        LcdSpi_WriteBit((data & mask) ? 1u : 0u);
        mask >>= 1;
    }
}

static void LcdSpi_WriteCommand(uint8_t command)
{
    LcdSpi_Select();
    LcdSpi_Write9(LCD_SPI_CMD, command);
    LcdSpi_Unselect();
}

static void LcdSpi_WriteData8(uint8_t data)
{
    LcdSpi_Select();
    LcdSpi_Write9(LCD_SPI_DATA, data);
    LcdSpi_Unselect();
}

static void LcdSpi_WriteData16(uint16_t data)
{
    LcdSpi_WriteData8((uint8_t)(data >> 8));
    LcdSpi_WriteData8((uint8_t)data);
}

static void LcdSpi_ResetPanel(void)
{
    gpio_bit_reset(LCD_RST_PORT, LCD_RST_PIN);
    LcdSpi_DelayMs(100u);
    gpio_bit_set(LCD_RST_PORT, LCD_RST_PIN);
    LcdSpi_DelayMs(100u);
}

static void LcdSpi_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /*
     * 参考 demo 的 240x280 屏在竖屏方向有 20 行偏移。
     * 写窗口时补上偏移，应用层仍按 0..239、0..279 坐标使用。
     */
    y1 = (uint16_t)(y1 + LCD_SPI_Y_OFFSET);
    y2 = (uint16_t)(y2 + LCD_SPI_Y_OFFSET);

    LcdSpi_WriteCommand(0x2Au);
    LcdSpi_WriteData16(x1);
    LcdSpi_WriteData16(x2);

    LcdSpi_WriteCommand(0x2Bu);
    LcdSpi_WriteData16(y1);
    LcdSpi_WriteData16(y2);

    LcdSpi_WriteCommand(0x2Cu);
}

static void LcdSpi_WriteColorStream(uint16_t color, uint32_t pixels)
{
    uint8_t high;
    uint8_t low;
    uint32_t index;

    high = (uint8_t)(color >> 8);
    low = (uint8_t)color;

    LcdSpi_Select();
    for (index = 0u; index < pixels; index++)
    {
        LcdSpi_Write9(LCD_SPI_DATA, high);
        LcdSpi_Write9(LCD_SPI_DATA, low);
    }
    LcdSpi_Unselect();
}

static void LcdSpi_InitSequence(void)
{
    LcdSpi_WriteCommand(0x11u);
    LcdSpi_DelayMs(120u);

    LcdSpi_WriteCommand(0x36u);
    LcdSpi_WriteData8(0x00u);

    LcdSpi_WriteCommand(0x3Au);
    LcdSpi_WriteData8(0x05u);

    LcdSpi_WriteCommand(0xB2u);
    LcdSpi_WriteData8(0x0Cu);
    LcdSpi_WriteData8(0x0Cu);
    LcdSpi_WriteData8(0x00u);
    LcdSpi_WriteData8(0x33u);
    LcdSpi_WriteData8(0x33u);

    LcdSpi_WriteCommand(0xB7u);
    LcdSpi_WriteData8(0x35u);

    LcdSpi_WriteCommand(0xBBu);
    LcdSpi_WriteData8(0x32u);

    LcdSpi_WriteCommand(0xC2u);
    LcdSpi_WriteData8(0x01u);

    LcdSpi_WriteCommand(0xC3u);
    LcdSpi_WriteData8(0x15u);

    LcdSpi_WriteCommand(0xC4u);
    LcdSpi_WriteData8(0x20u);

    LcdSpi_WriteCommand(0xC6u);
    LcdSpi_WriteData8(0x0Fu);

    LcdSpi_WriteCommand(0xD0u);
    LcdSpi_WriteData8(0xA4u);
    LcdSpi_WriteData8(0xA1u);

    LcdSpi_WriteCommand(0xE0u);
    LcdSpi_WriteData8(0xD0u);
    LcdSpi_WriteData8(0x08u);
    LcdSpi_WriteData8(0x0Eu);
    LcdSpi_WriteData8(0x09u);
    LcdSpi_WriteData8(0x09u);
    LcdSpi_WriteData8(0x05u);
    LcdSpi_WriteData8(0x31u);
    LcdSpi_WriteData8(0x33u);
    LcdSpi_WriteData8(0x48u);
    LcdSpi_WriteData8(0x17u);
    LcdSpi_WriteData8(0x14u);
    LcdSpi_WriteData8(0x15u);
    LcdSpi_WriteData8(0x31u);
    LcdSpi_WriteData8(0x34u);

    LcdSpi_WriteCommand(0xE1u);
    LcdSpi_WriteData8(0xD0u);
    LcdSpi_WriteData8(0x08u);
    LcdSpi_WriteData8(0x0Eu);
    LcdSpi_WriteData8(0x09u);
    LcdSpi_WriteData8(0x09u);
    LcdSpi_WriteData8(0x15u);
    LcdSpi_WriteData8(0x31u);
    LcdSpi_WriteData8(0x33u);
    LcdSpi_WriteData8(0x48u);
    LcdSpi_WriteData8(0x17u);
    LcdSpi_WriteData8(0x14u);
    LcdSpi_WriteData8(0x15u);
    LcdSpi_WriteData8(0x31u);
    LcdSpi_WriteData8(0x34u);

    LcdSpi_WriteCommand(0x21u);
    LcdSpi_WriteCommand(0x29u);
    LcdSpi_DelayMs(120u);
}

void LcdSpi_Init(void)
{
    LcdSpi_GpioInit();
    LcdSpi_ResetPanel();
    LcdSpi_InitSequence();
    LcdSpi_BacklightOn();
}

void LcdSpi_PanelInitOnly(void)
{
    LcdSpi_GpioInit();
    LcdSpi_InitSequence();
}

void LcdSpi_BacklightOn(void)
{
    gpio_bit_set(LCD_BLK_PORT, LCD_BLK_PIN);
}

void LcdSpi_BacklightOff(void)
{
    gpio_bit_reset(LCD_BLK_PORT, LCD_BLK_PIN);
}

void LcdSpi_FillColor(uint16_t color)
{
    LcdSpi_SetWindow(0u, 0u, (uint16_t)(LCD_SPI_WIDTH - 1u), (uint16_t)(LCD_SPI_HEIGHT - 1u));
    LcdSpi_WriteColorStream(color, LCD_SPI_WIDTH * LCD_SPI_HEIGHT);
}

void LcdSpi_DrawColorBars(void)
{
    static const uint16_t colors[] =
    {
        LCD_SPI_RGB565_RED,
        LCD_SPI_RGB565_GREEN,
        LCD_SPI_RGB565_BLUE,
        LCD_SPI_RGB565_WHITE,
        LCD_SPI_RGB565_YELLOW,
        LCD_SPI_RGB565_CYAN,
        LCD_SPI_RGB565_MAGENTA,
        LCD_SPI_RGB565_BLACK
    };
    uint32_t x;
    uint32_t y;
    uint32_t bar;
    uint32_t bar_width;
    uint32_t color_count;

    color_count = sizeof(colors) / sizeof(colors[0]);
    bar_width = LCD_SPI_WIDTH / color_count;

    LcdSpi_SetWindow(0u, 0u, (uint16_t)(LCD_SPI_WIDTH - 1u), (uint16_t)(LCD_SPI_HEIGHT - 1u));
    LcdSpi_Select();
    for (y = 0u; y < LCD_SPI_HEIGHT; y++)
    {
        for (x = 0u; x < LCD_SPI_WIDTH; x++)
        {
            bar = x / bar_width;
            if (bar >= color_count)
            {
                bar = color_count - 1u;
            }

            LcdSpi_Write9(LCD_SPI_DATA, (uint8_t)(colors[bar] >> 8));
            LcdSpi_Write9(LCD_SPI_DATA, (uint8_t)colors[bar]);
        }
    }
    LcdSpi_Unselect();
}
