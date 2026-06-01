#include "LcdTli.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_tli.h"

#define LCD_TLI_BYTES_PER_PIXEL         2u
#define LCD_TLI_LINE_BYTES              (LCD_TLI_WIDTH * LCD_TLI_BYTES_PER_PIXEL)

#define LCD_RST_PORT                    GPIOD
#define LCD_RST_PIN                     GPIO_PIN_12
#define LCD_BLK_PORT                    GPIOD
#define LCD_BLK_PIN                     GPIO_PIN_13

#define LCD_TLI_PLLSAI_N                192u
#define LCD_TLI_PLLSAI_P                2u
#define LCD_TLI_PLLSAI_R                3u
#define LCD_TLI_PLLSAIR_DIV             RCU_PLLSAIR_DIV2

static void LcdTli_DelayMs(uint32_t ms)
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

static void LcdTli_GpioAfConfig(uint32_t gpio_periph, uint32_t af, uint32_t pins)
{
    gpio_af_set(gpio_periph, af, pins);
    gpio_mode_set(gpio_periph, GPIO_MODE_AF, GPIO_PUPD_NONE, pins);
    gpio_output_options_set(gpio_periph, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pins);
}

static void LcdTli_GpioInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);

    /*
     * RGB565 与同步信号按原理图连接到 TLI。
     * 少数 TLI 信号在 GD32F470 上使用 AF9，其余使用 AF14。
     */
    LcdTli_GpioAfConfig(GPIOA, GPIO_AF_14, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 |
                                           GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12);
    LcdTli_GpioAfConfig(GPIOB, GPIO_AF_14, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);
    LcdTli_GpioAfConfig(GPIOB, GPIO_AF_9, GPIO_PIN_0);
    LcdTli_GpioAfConfig(GPIOC, GPIO_AF_14, GPIO_PIN_6 | GPIO_PIN_7);
    LcdTli_GpioAfConfig(GPIOD, GPIO_AF_14, GPIO_PIN_3);
    LcdTli_GpioAfConfig(GPIOF, GPIO_AF_14, GPIO_PIN_10);
    LcdTli_GpioAfConfig(GPIOG, GPIO_AF_14, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_11);
    LcdTli_GpioAfConfig(GPIOG, GPIO_AF_9, GPIO_PIN_10 | GPIO_PIN_12);

    /* LCD_RST 和 LCD_BLK 先按普通 GPIO 控制，首测阶段背光只做开关。 */
    gpio_mode_set(LCD_RST_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_RST_PIN);
    gpio_output_options_set(LCD_RST_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_RST_PIN);
    gpio_bit_set(LCD_RST_PORT, LCD_RST_PIN);

    gpio_mode_set(LCD_BLK_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_BLK_PIN);
    gpio_output_options_set(LCD_BLK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_BLK_PIN);
    gpio_bit_reset(LCD_BLK_PORT, LCD_BLK_PIN);
}

static uint8_t LcdTli_ClockInit(void)
{
    /*
     * 这里沿用已验证 RGB_800x480_DEMO 的 PLLSAI 配置。
     * 该配置配合 800x480 消隐参数可以稳定点亮当前 RGB 屏。
     */
    if (SUCCESS != rcu_pllsai_config(LCD_TLI_PLLSAI_N, LCD_TLI_PLLSAI_P, LCD_TLI_PLLSAI_R))
    {
        return 0u;
    }

    rcu_tli_clock_div_config(LCD_TLI_PLLSAIR_DIV);
    rcu_osci_on(RCU_PLLSAI_CK);

    if (SUCCESS != rcu_osci_stab_wait(RCU_PLLSAI_CK))
    {
        return 0u;
    }

    rcu_periph_clock_enable(RCU_TLI);
    return 1u;
}

static void LcdTli_ResetPanel(void)
{
    gpio_bit_reset(LCD_RST_PORT, LCD_RST_PIN);
    LcdTli_DelayMs(20u);
    gpio_bit_set(LCD_RST_PORT, LCD_RST_PIN);
    LcdTli_DelayMs(120u);
}

uint8_t LcdTli_Init(uint32_t framebuffer)
{
    tli_parameter_struct tli_init_struct;
    tli_layer_parameter_struct layer_init_struct;
    uint32_t accumulated_hbp;
    uint32_t accumulated_vbp;

    LcdTli_GpioInit();
    LcdTli_BacklightOff();
    LcdTli_ResetPanel();

    if (LcdTli_ClockInit() == 0u)
    {
        return 0u;
    }

    tli_deinit();

    accumulated_hbp = LCD_TLI_HSYNC + LCD_TLI_HBP;
    accumulated_vbp = LCD_TLI_VSYNC + LCD_TLI_VBP;

    tli_struct_para_init(&tli_init_struct);
    tli_init_struct.synpsz_hpsz = (uint16_t)(LCD_TLI_HSYNC - 1u);
    tli_init_struct.synpsz_vpsz = (uint16_t)(LCD_TLI_VSYNC - 1u);
    tli_init_struct.backpsz_hbpsz = (uint16_t)(accumulated_hbp - 1u);
    tli_init_struct.backpsz_vbpsz = (uint16_t)(accumulated_vbp - 1u);
    tli_init_struct.activesz_hasz = accumulated_hbp + LCD_TLI_WIDTH - 1u;
    tli_init_struct.activesz_vasz = accumulated_vbp + LCD_TLI_HEIGHT - 1u;
    tli_init_struct.totalsz_htsz = accumulated_hbp + LCD_TLI_WIDTH + LCD_TLI_HFP - 1u;
    tli_init_struct.totalsz_vtsz = accumulated_vbp + LCD_TLI_HEIGHT + LCD_TLI_VFP - 1u;
    tli_init_struct.backcolor_red = 0u;
    tli_init_struct.backcolor_green = 0u;
    tli_init_struct.backcolor_blue = 0u;
    tli_init_struct.signalpolarity_hs = TLI_HSYN_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_vs = TLI_VSYN_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_de = TLI_DE_ACTLIVE_LOW;
    tli_init_struct.signalpolarity_pixelck = TLI_PIXEL_CLOCK_TLI;
    tli_init(&tli_init_struct);

    tli_layer_struct_para_init(&layer_init_struct);
    layer_init_struct.layer_window_leftpos = (uint16_t)accumulated_hbp;
    layer_init_struct.layer_window_rightpos = (uint16_t)(accumulated_hbp + LCD_TLI_WIDTH - 1u);
    layer_init_struct.layer_window_toppos = (uint16_t)accumulated_vbp;
    layer_init_struct.layer_window_bottompos = (uint16_t)(accumulated_vbp + LCD_TLI_HEIGHT - 1u);
    layer_init_struct.layer_ppf = LAYER_PPF_RGB565;
    layer_init_struct.layer_sa = 255u;
    layer_init_struct.layer_default_alpha = 0u;
    layer_init_struct.layer_default_red = 0u;
    layer_init_struct.layer_default_green = 0u;
    layer_init_struct.layer_default_blue = 0u;
    layer_init_struct.layer_acf1 = LAYER_ACF1_SA;
    layer_init_struct.layer_acf2 = LAYER_ACF2_SA;
    layer_init_struct.layer_frame_bufaddr = framebuffer;
    layer_init_struct.layer_frame_line_length = (uint16_t)(LCD_TLI_LINE_BYTES + 3u);
    layer_init_struct.layer_frame_buf_stride_offset = (uint16_t)LCD_TLI_LINE_BYTES;
    layer_init_struct.layer_frame_total_line_number = (uint16_t)LCD_TLI_HEIGHT;
    tli_layer_init(LAYER0, &layer_init_struct);
    tli_dither_config(TLI_DITHER_ENABLE);

    tli_layer_enable(LAYER0);
    tli_reload_config(TLI_FRAME_BLANK_RELOAD_EN);
    tli_enable();

    return 1u;
}

void LcdTli_BacklightOn(void)
{
    gpio_bit_set(LCD_BLK_PORT, LCD_BLK_PIN);
}

void LcdTli_BacklightOff(void)
{
    gpio_bit_reset(LCD_BLK_PORT, LCD_BLK_PIN);
}

void LcdTli_FillColor(uint32_t framebuffer, uint16_t color)
{
    volatile uint16_t *fb;
    uint32_t index;
    uint32_t pixels;

    fb = (volatile uint16_t *)framebuffer;
    pixels = LCD_TLI_WIDTH * LCD_TLI_HEIGHT;

    for (index = 0u; index < pixels; index++)
    {
        fb[index] = color;
    }
}

void LcdTli_DrawColorBars(uint32_t framebuffer)
{
    static const uint16_t colors[] =
    {
        LCD_TLI_RGB565_RED,
        LCD_TLI_RGB565_GREEN,
        LCD_TLI_RGB565_BLUE,
        LCD_TLI_RGB565_WHITE,
        LCD_TLI_RGB565_YELLOW,
        LCD_TLI_RGB565_CYAN,
        LCD_TLI_RGB565_MAGENTA,
        LCD_TLI_RGB565_BLACK
    };
    volatile uint16_t *fb;
    uint32_t x;
    uint32_t y;
    uint32_t bar;
    uint32_t bar_width;

    fb = (volatile uint16_t *)framebuffer;
    bar_width = LCD_TLI_WIDTH / (sizeof(colors) / sizeof(colors[0]));

    for (y = 0u; y < LCD_TLI_HEIGHT; y++)
    {
        for (x = 0u; x < LCD_TLI_WIDTH; x++)
        {
            bar = x / bar_width;
            if (bar >= (sizeof(colors) / sizeof(colors[0])))
            {
                bar = (sizeof(colors) / sizeof(colors[0])) - 1u;
            }

            fb[(y * LCD_TLI_WIDTH) + x] = colors[bar];
        }
    }
}

uint32_t LcdTli_GetFrameBufferBytes(void)
{
    return LCD_TLI_WIDTH * LCD_TLI_HEIGHT * LCD_TLI_BYTES_PER_PIXEL;
}
