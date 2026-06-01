#include "Exmc.h"

#include "gd32f4xx_exmc.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

#define EXMC_SDRAM_BUSY_TIMEOUT        0x100000u
#define EXMC_SDRAM_MODE_REGISTER       0x0220u
#define EXMC_SDRAM_REFRESH_COUNT       761u

static void Exmc_SdramDelay(volatile uint32_t count)
{
    while (count-- != 0u)
    {
        __NOP();
    }
}

static uint8_t Exmc_SdramWaitReady(void)
{
    uint32_t timeout;

    timeout = EXMC_SDRAM_BUSY_TIMEOUT;
    while (SET == exmc_flag_get(EXMC_SDRAM_DEVICE0, EXMC_SDRAM_FLAG_NREADY))
    {
        if (timeout == 0u)
        {
            return 0u;
        }

        timeout--;
    }

    return 1u;
}

static uint8_t Exmc_SdramCommand(uint32_t command, uint32_t refresh_number, uint32_t mode_register)
{
    exmc_sdram_command_parameter_struct command_init;

    command_init.command = command;
    command_init.bank_select = EXMC_SDRAM_DEVICE0_SELECT;
    command_init.auto_refresh_number = refresh_number;
    command_init.mode_register_content = mode_register;

    exmc_sdram_command_config(&command_init);

    return Exmc_SdramWaitReady();
}

static void Exmc_SdramGpioInit(void)
{
    /* 按原理图连接 SDRAM：PD/PE/PF/PG/PC 都切到 EXMC 复用功能。 */
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_GPIOG);

    /* PC0=NWE，PC2=NE0，PC3=CKE0。 */
    gpio_af_set(GPIOC, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);

    /* PD0/1、PD8/9/10、PD14/15 为 D2/D3/D13/D14/D15/D0/D1。 */
    gpio_af_set(GPIOD, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 |
                                   GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 |
                                                     GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 |
                                                               GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15);

    /* PE0/1 为 NBL0/NBL1，PE7~PE15 为 D4~D12。 */
    gpio_af_set(GPIOE, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
                                   GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                                   GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
                                                     GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                                                     GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 |
                                                               GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                                                               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* PF0~PF5、PF12~PF15 为 A0~A9，PF11 为 NRAS。 */
    gpio_af_set(GPIOF, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                                   GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
                                   GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                                                     GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
                                                     GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                                                               GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
                                                               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    /* PG0~PG2 为 A10~A12，PG4/5 为 BA0/BA1，PG8 为 CLK，PG15 为 NCAS。 */
    gpio_af_set(GPIOG, GPIO_AF_12, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                                   GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                                                     GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                                                               GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15);
}

uint8_t Exmc_SdramInit(void)
{
    exmc_sdram_timing_parameter_struct timing;
    exmc_sdram_parameter_struct init;

    Exmc_SdramGpioInit();
    rcu_periph_clock_enable(RCU_EXMC);

    /*
     * W9825G6KH-6I：16bit 数据宽度、4 个内部 bank、13 行地址、9 列地址。
     * LCD/TLI 连续从 SDRAM 取 framebuffer，参数对齐已实测可显示的 RGB_800x480_DEMO：
     * SDCLK=HCLK/2、CAS=2、打开 burst read、pipeline=1。
     */
    timing.load_mode_register_delay = 2u;
    timing.exit_selfrefresh_delay = 8u;
    timing.row_address_select_delay = 5u;
    timing.auto_refresh_delay = 7u;
    timing.write_recovery_delay = 2u;
    timing.row_precharge_delay = 3u;
    timing.row_to_column_delay = 3u;

    init.sdram_device = EXMC_SDRAM_DEVICE0;
    init.column_address_width = EXMC_SDRAM_COW_ADDRESS_9;
    init.row_address_width = EXMC_SDRAM_ROW_ADDRESS_13;
    init.data_width = EXMC_SDRAM_DATABUS_WIDTH_16B;
    init.internal_bank_number = EXMC_SDRAM_4_INTER_BANK;
    init.cas_latency = EXMC_CAS_LATENCY_2_SDCLK;
    init.write_protection = DISABLE;
    init.sdclock_config = EXMC_SDCLK_PERIODS_2_HCLK;
    init.brust_read_switch = ENABLE;
    init.pipeline_read_delay = EXMC_PIPELINE_DELAY_1_HCLK;
    init.timing = &timing;

    exmc_sdram_deinit(EXMC_SDRAM_DEVICE0);
    exmc_sdram_init(&init);

    if (Exmc_SdramCommand(EXMC_SDRAM_CLOCK_ENABLE, EXMC_SDRAM_AUTO_REFLESH_1_SDCLK, 0u) == 0u)
    {
        return 0u;
    }

    Exmc_SdramDelay(20000u);

    if (Exmc_SdramCommand(EXMC_SDRAM_PRECHARGE_ALL, EXMC_SDRAM_AUTO_REFLESH_1_SDCLK, 0u) == 0u)
    {
        return 0u;
    }

    if (Exmc_SdramCommand(EXMC_SDRAM_AUTO_REFRESH, EXMC_SDRAM_AUTO_REFLESH_8_SDCLK, 0u) == 0u)
    {
        return 0u;
    }

    if (Exmc_SdramCommand(EXMC_SDRAM_LOAD_MODE_REGISTER,
                          EXMC_SDRAM_AUTO_REFLESH_1_SDCLK,
                          EXMC_SDRAM_MODE_REGISTER) == 0u)
    {
        return 0u;
    }

    exmc_sdram_refresh_count_set(EXMC_SDRAM_REFRESH_COUNT);

    return Exmc_SdramWaitReady();
}

uint32_t Exmc_SdramBase(void)
{
    return EXMC_SDRAM_BASE_ADDR;
}

uint32_t Exmc_SdramSize(void)
{
    return EXMC_SDRAM_SIZE_BYTES;
}
