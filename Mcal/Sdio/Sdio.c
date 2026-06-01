#include "Sdio.h"

#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_sdio.h"

#define SDIO_CMD_GO_IDLE_STATE              0u
#define SDIO_CMD_SEND_IF_COND               8u
#define SDIO_CMD_ALL_SEND_CID               2u
#define SDIO_CMD_SET_REL_ADDR               3u
#define SDIO_CMD_SEND_CSD                   9u
#define SDIO_CMD_SELECT_CARD                7u
#define SDIO_CMD_SET_BLOCKLEN               16u
#define SDIO_CMD_READ_SINGLE_BLOCK          17u
#define SDIO_CMD_APP_CMD                    55u
#define SDIO_ACMD_SD_SEND_OP_COND           41u

#define SDIO_CMD_FLAG_ALL                   (SDIO_FLAG_CCRCERR | SDIO_FLAG_CMDTMOUT | \
                                             SDIO_FLAG_CMDRECV | SDIO_FLAG_CMDSEND)
#define SDIO_DATA_FLAG_ALL                  (SDIO_FLAG_DTCRCERR | SDIO_FLAG_DTTMOUT | \
                                             SDIO_FLAG_TXURE | SDIO_FLAG_RXORE | \
                                             SDIO_FLAG_DTEND | SDIO_FLAG_STBITE | \
                                             SDIO_FLAG_DTBLKEND)
#define SDIO_WAIT_LOOP                      2000000u
#define SDIO_DATA_TIMEOUT                   0x00FFFFFFu
#define SDIO_BLOCK_SIZE                     512u
#define SDIO_WORDS_PER_BLOCK                (SDIO_BLOCK_SIZE / 4u)

static Sdio_CardInfoType Sdio_CardInfo;
static Sdio_StatusType Sdio_LastStatus;
static uint32_t Sdio_LastCommand;
static uint32_t Sdio_LastStatusRegister;

static void Sdio_DelayMs(uint32_t ms)
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

static void Sdio_ClearFlags(void)
{
    sdio_flag_clear(SDIO_CMD_FLAG_ALL | SDIO_DATA_FLAG_ALL);
}

static void Sdio_SetInitClock(void)
{
    /*
     * SD 卡上电识别阶段要求 SDIO_CLK 不超过 400kHz。
     * 当前系统 PLLQ 约 44~48MHz，118 分频可以落在安全范围。
     */
    sdio_clock_config(SDIO_SDIOCLKEDGE_RISING,
                      SDIO_CLOCKBYPASS_DISABLE,
                      SDIO_CLOCKPWRSAVE_DISABLE,
                      118u);
    sdio_bus_mode_set(SDIO_BUSMODE_1BIT);
    sdio_clock_enable();
}

static void Sdio_SetTransferClock(void)
{
    /*
     * 第一版硬件测试优先求稳，先用 1bit + 低速读扇区。
     * 读卡稳定后再打开 4bit 和更高 SDIO_CLK。
     */
    sdio_clock_config(SDIO_SDIOCLKEDGE_RISING,
                      SDIO_CLOCKBYPASS_DISABLE,
                      SDIO_CLOCKPWRSAVE_DISABLE,
                      10u);
    sdio_bus_mode_set(SDIO_BUSMODE_1BIT);
    sdio_clock_enable();
}

static Sdio_StatusType Sdio_SendCommand(uint32_t command,
                                        uint32_t argument,
                                        uint32_t response_type,
                                        uint8_t ignore_crc)
{
    uint32_t timeout;

    Sdio_LastCommand = command;
    Sdio_ClearFlags();

    sdio_command_response_config(command, argument, response_type);
    sdio_wait_type_set(SDIO_WAITTYPE_NO);
    sdio_csm_enable();

    timeout = SDIO_WAIT_LOOP;
    while (timeout-- != 0u)
    {
        Sdio_LastStatusRegister = SDIO_STAT;

        if (response_type == SDIO_RESPONSETYPE_NO)
        {
            if (sdio_flag_get(SDIO_FLAG_CMDSEND) == SET)
            {
                sdio_flag_clear(SDIO_CMD_FLAG_ALL);
                Sdio_LastStatus = SDIO_STATUS_OK;
                return SDIO_STATUS_OK;
            }
        }
        else
        {
            if (sdio_flag_get(SDIO_FLAG_CMDRECV) == SET)
            {
                sdio_flag_clear(SDIO_CMD_FLAG_ALL);
                Sdio_LastStatus = SDIO_STATUS_OK;
                return SDIO_STATUS_OK;
            }
        }

        if (sdio_flag_get(SDIO_FLAG_CMDTMOUT) == SET)
        {
            sdio_flag_clear(SDIO_CMD_FLAG_ALL);
            Sdio_LastStatus = SDIO_STATUS_CMD_TIMEOUT;
            return SDIO_STATUS_CMD_TIMEOUT;
        }

        if (sdio_flag_get(SDIO_FLAG_CCRCERR) == SET)
        {
            if (ignore_crc != 0u)
            {
                sdio_flag_clear(SDIO_CMD_FLAG_ALL);
                Sdio_LastStatus = SDIO_STATUS_OK;
                return SDIO_STATUS_OK;
            }

            sdio_flag_clear(SDIO_CMD_FLAG_ALL);
            Sdio_LastStatus = SDIO_STATUS_CMD_CRC;
            return SDIO_STATUS_CMD_CRC;
        }
    }

    Sdio_LastStatus = SDIO_STATUS_TIMEOUT;
    return SDIO_STATUS_TIMEOUT;
}

static Sdio_StatusType Sdio_SendAppCommand(uint32_t command,
                                           uint32_t argument,
                                           uint32_t response_type,
                                           uint8_t ignore_crc)
{
    Sdio_StatusType status;

    status = Sdio_SendCommand(SDIO_CMD_APP_CMD,
                              ((uint32_t)Sdio_CardInfo.rca) << 16u,
                              SDIO_RESPONSETYPE_SHORT,
                              0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }

    return Sdio_SendCommand(command, argument, response_type, ignore_crc);
}

static void Sdio_SaveLongResponse(uint32_t response[4])
{
    response[0] = sdio_response_get(SDIO_RESPONSE0);
    response[1] = sdio_response_get(SDIO_RESPONSE1);
    response[2] = sdio_response_get(SDIO_RESPONSE2);
    response[3] = sdio_response_get(SDIO_RESPONSE3);
}

static uint32_t Sdio_ParseBlockCountFromCsd(const uint32_t csd[4])
{
    uint8_t csd_structure;
    uint32_t c_size;

    /*
     * GD32 的长响应寄存器顺序对应 CSD[127:96]、[95:64]、[63:32]、[31:0]。
     * SDHC/SDXC 的容量字段 C_SIZE 位于 CSD[69:48]。
     */
    csd_structure = (uint8_t)((csd[0] >> 30u) & 0x03u);
    if (csd_structure == 1u)
    {
        c_size = ((csd[1] & 0x0000003Fu) << 16u) |
                 ((csd[2] >> 16u) & 0x0000FFFFu);
        return (c_size + 1u) * 1024u;
    }

    return 0u;
}

void Sdio_InitPinsAndClock(void)
{
    rcu_periph_clock_enable(TF_SDIO_D_GPIO_CLK);
    rcu_periph_clock_enable(TF_SDIO_CMD_GPIO_CLK);
    rcu_periph_clock_enable(TF_SDIO_CLK);

    gpio_af_set(TF_SDIO_D_PORT, TF_SDIO_GPIO_AF,
                TF_SDIO_D0_PIN | TF_SDIO_D1_PIN |
                TF_SDIO_D2_PIN | TF_SDIO_D3_PIN | TF_SDIO_CLK_PIN);
    gpio_af_set(TF_SDIO_CMD_PORT, TF_SDIO_GPIO_AF, TF_SDIO_CMD_PIN);

    gpio_mode_set(TF_SDIO_D_PORT,
                  GPIO_MODE_AF,
                  GPIO_PUPD_PULLUP,
                  TF_SDIO_D0_PIN | TF_SDIO_D1_PIN |
                  TF_SDIO_D2_PIN | TF_SDIO_D3_PIN);
    gpio_output_options_set(TF_SDIO_D_PORT,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_50MHZ,
                            TF_SDIO_D0_PIN | TF_SDIO_D1_PIN |
                            TF_SDIO_D2_PIN | TF_SDIO_D3_PIN);

    gpio_mode_set(TF_SDIO_CLK_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, TF_SDIO_CLK_PIN);
    gpio_output_options_set(TF_SDIO_CLK_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TF_SDIO_CLK_PIN);

    gpio_mode_set(TF_SDIO_CMD_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, TF_SDIO_CMD_PIN);
    gpio_output_options_set(TF_SDIO_CMD_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, TF_SDIO_CMD_PIN);

    sdio_deinit();
    sdio_power_state_set(SDIO_POWER_ON);
    Sdio_SetInitClock();
    Sdio_DelayMs(10u);
}

Sdio_StatusType Sdio_CardInit(void)
{
    Sdio_StatusType status;
    uint32_t retry;
    uint32_t ocr;
    uint32_t response;

    Sdio_CardInfo.card_type = SDIO_CARD_TYPE_UNKNOWN;
    Sdio_CardInfo.rca = 0u;
    Sdio_CardInfo.ocr = 0u;
    Sdio_CardInfo.block_count = 0u;
    Sdio_CardInfo.block_size = SDIO_BLOCK_SIZE;

    Sdio_InitPinsAndClock();

    status = Sdio_SendCommand(SDIO_CMD_GO_IDLE_STATE, 0u, SDIO_RESPONSETYPE_NO, 0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }

    /*
     * CMD8 验证 2.7~3.6V 电压窗口和 SD v2 卡。
     * 目前测试目标是常见 SDHC/SDXC TF 卡，老 SDSC 卡暂不作为主路径。
     */
    status = Sdio_SendCommand(SDIO_CMD_SEND_IF_COND,
                              0x000001AAu,
                              SDIO_RESPONSETYPE_SHORT,
                              0u);
    if (status != SDIO_STATUS_OK)
    {
        return SDIO_STATUS_UNSUPPORTED;
    }

    response = sdio_response_get(SDIO_RESPONSE0);
    if ((response & 0x00000FFFu) != 0x000001AAu)
    {
        Sdio_LastStatus = SDIO_STATUS_UNSUPPORTED;
        return SDIO_STATUS_UNSUPPORTED;
    }

    ocr = 0u;
    for (retry = 0u; retry < 2000u; retry++)
    {
        status = Sdio_SendAppCommand(SDIO_ACMD_SD_SEND_OP_COND,
                                     0x40300000u,
                                     SDIO_RESPONSETYPE_SHORT,
                                     1u);
        if (status != SDIO_STATUS_OK)
        {
            return status;
        }

        ocr = sdio_response_get(SDIO_RESPONSE0);
        if ((ocr & 0x80000000u) != 0u)
        {
            break;
        }

        Sdio_DelayMs(1u);
    }

    if ((ocr & 0x80000000u) == 0u)
    {
        Sdio_LastStatus = SDIO_STATUS_NOT_READY;
        return SDIO_STATUS_NOT_READY;
    }

    Sdio_CardInfo.ocr = ocr;
    if ((ocr & 0x40000000u) != 0u)
    {
        Sdio_CardInfo.card_type = SDIO_CARD_TYPE_SDHC_SDXC;
    }
    else
    {
        Sdio_CardInfo.card_type = SDIO_CARD_TYPE_SDSC;
    }

    status = Sdio_SendCommand(SDIO_CMD_ALL_SEND_CID, 0u, SDIO_RESPONSETYPE_LONG, 0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }
    Sdio_SaveLongResponse(Sdio_CardInfo.cid);

    status = Sdio_SendCommand(SDIO_CMD_SET_REL_ADDR, 0u, SDIO_RESPONSETYPE_SHORT, 0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }
    Sdio_CardInfo.rca = (uint16_t)(sdio_response_get(SDIO_RESPONSE0) >> 16u);

    status = Sdio_SendCommand(SDIO_CMD_SEND_CSD,
                              ((uint32_t)Sdio_CardInfo.rca) << 16u,
                              SDIO_RESPONSETYPE_LONG,
                              0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }
    Sdio_SaveLongResponse(Sdio_CardInfo.csd);
    Sdio_CardInfo.block_count = Sdio_ParseBlockCountFromCsd(Sdio_CardInfo.csd);

    status = Sdio_SendCommand(SDIO_CMD_SELECT_CARD,
                              ((uint32_t)Sdio_CardInfo.rca) << 16u,
                              SDIO_RESPONSETYPE_SHORT,
                              0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }

    status = Sdio_SendCommand(SDIO_CMD_SET_BLOCKLEN,
                              SDIO_BLOCK_SIZE,
                              SDIO_RESPONSETYPE_SHORT,
                              0u);
    if (status != SDIO_STATUS_OK)
    {
        return status;
    }

    Sdio_SetTransferClock();
    Sdio_LastStatus = SDIO_STATUS_OK;
    return SDIO_STATUS_OK;
}

Sdio_StatusType Sdio_ReadBlock(uint32_t block_number, uint8_t *buffer)
{
    Sdio_StatusType status;
    uint32_t argument;
    uint32_t word;
    uint32_t words_read;
    uint32_t timeout;

    if (buffer == 0)
    {
        Sdio_LastStatus = SDIO_STATUS_PARAM;
        return SDIO_STATUS_PARAM;
    }

    if (Sdio_CardInfo.card_type == SDIO_CARD_TYPE_SDSC)
    {
        argument = block_number * SDIO_BLOCK_SIZE;
    }
    else
    {
        argument = block_number;
    }

    Sdio_ClearFlags();
    sdio_data_config(SDIO_DATA_TIMEOUT, SDIO_BLOCK_SIZE, SDIO_DATABLOCKSIZE_512BYTES);
    sdio_data_transfer_config(SDIO_TRANSMODE_BLOCK, SDIO_TRANSDIRECTION_TOSDIO);
    sdio_dsm_enable();

    status = Sdio_SendCommand(SDIO_CMD_READ_SINGLE_BLOCK,
                              argument,
                              SDIO_RESPONSETYPE_SHORT,
                              0u);
    if (status != SDIO_STATUS_OK)
    {
        sdio_dsm_disable();
        return status;
    }

    words_read = 0u;
    timeout = SDIO_WAIT_LOOP * 4u;
    while ((words_read < SDIO_WORDS_PER_BLOCK) && (timeout-- != 0u))
    {
        Sdio_LastStatusRegister = SDIO_STAT;

        if ((sdio_flag_get(SDIO_FLAG_DTCRCERR) == SET) ||
            (sdio_flag_get(SDIO_FLAG_DTTMOUT) == SET) ||
            (sdio_flag_get(SDIO_FLAG_RXORE) == SET) ||
            (sdio_flag_get(SDIO_FLAG_STBITE) == SET))
        {
            sdio_dsm_disable();
            sdio_flag_clear(SDIO_DATA_FLAG_ALL);
            Sdio_LastStatus = SDIO_STATUS_READ_ERROR;
            return SDIO_STATUS_READ_ERROR;
        }

        if (sdio_flag_get(SDIO_FLAG_RXDTVAL) == SET)
        {
            word = sdio_data_read();
            buffer[(words_read * 4u) + 0u] = (uint8_t)(word & 0xFFu);
            buffer[(words_read * 4u) + 1u] = (uint8_t)((word >> 8u) & 0xFFu);
            buffer[(words_read * 4u) + 2u] = (uint8_t)((word >> 16u) & 0xFFu);
            buffer[(words_read * 4u) + 3u] = (uint8_t)((word >> 24u) & 0xFFu);
            words_read++;
        }
    }

    if (words_read != SDIO_WORDS_PER_BLOCK)
    {
        sdio_dsm_disable();
        sdio_flag_clear(SDIO_DATA_FLAG_ALL);
        Sdio_LastStatus = SDIO_STATUS_TIMEOUT;
        return SDIO_STATUS_TIMEOUT;
    }

    timeout = SDIO_WAIT_LOOP;
    while ((sdio_flag_get(SDIO_FLAG_DTEND) == RESET) && (timeout-- != 0u))
    {
    }

    sdio_dsm_disable();
    sdio_flag_clear(SDIO_DATA_FLAG_ALL);

    if (timeout == 0u)
    {
        Sdio_LastStatus = SDIO_STATUS_TIMEOUT;
        return SDIO_STATUS_TIMEOUT;
    }

    Sdio_LastStatus = SDIO_STATUS_OK;
    return SDIO_STATUS_OK;
}

const Sdio_CardInfoType *Sdio_GetCardInfo(void)
{
    return &Sdio_CardInfo;
}

Sdio_StatusType Sdio_GetLastStatus(void)
{
    return Sdio_LastStatus;
}

uint32_t Sdio_GetLastCommand(void)
{
    return Sdio_LastCommand;
}

uint32_t Sdio_GetLastStatusRegister(void)
{
    return Sdio_LastStatusRegister;
}
