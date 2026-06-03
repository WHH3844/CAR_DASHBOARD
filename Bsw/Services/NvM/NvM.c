#include "NvM.h"

#include "Crc.h"
#include "Eep.h"
#include "LogM.h"
#include "NvM_Cfg.h"

#define NVM_HEADER_SIZE             7u
#define NVM_MAX_PAYLOAD_SIZE        128u
#define NVM_CRC_START_VALUE         0xFFFFu

/*
 * NvM 块格式：
 * [0..1] magic，用于识别块类型；
 * [2]    version，用于后续结构体升级；
 * [3..4] payload length；
 * [5..6] CRC16，覆盖 payload。
 */
typedef struct
{
    NvM_BlockIdType id;
    uint16_t address;
    uint16_t magic;
    uint16_t max_length;
} NvM_BlockConfigType;

static const NvM_BlockConfigType NvM_BlockConfigs[] =
{
    {NVM_BLOCK_BOOT_INFO,     NVM_CFG_ADDR_BOOT_INFO,     NVM_CFG_MAGIC_BOOT_INFO,     sizeof(NvM_BootInfoType)},
    {NVM_BLOCK_SYSTEM_CONFIG, NVM_CFG_ADDR_SYSTEM_CONFIG, NVM_CFG_MAGIC_SYSTEM_CONFIG, sizeof(NvM_SystemConfigType)},
    {NVM_BLOCK_DEM_STATUS,    NVM_CFG_ADDR_DEM_STATUS,    NVM_CFG_MAGIC_DEM_STATUS,    NVM_MAX_PAYLOAD_SIZE}
};

static NvM_BootInfoType NvM_BootInfo;
static NvM_SystemConfigType NvM_SystemConfig;
static uint8_t NvM_Initialized;

static const NvM_BlockConfigType *NvM_FindConfig(NvM_BlockIdType block_id)
{
    uint8_t index;

    for (index = 0u; index < (sizeof(NvM_BlockConfigs) / sizeof(NvM_BlockConfigs[0])); index++)
    {
        if (NvM_BlockConfigs[index].id == block_id)
        {
            return &NvM_BlockConfigs[index];
        }
    }

    return 0;
}

static uint8_t NvM_ReadByte(uint16_t address, uint8_t *data)
{
    return Eep_ReadByte(address, data);
}

static uint8_t NvM_WriteByte(uint16_t address, uint8_t data)
{
    return Eep_WriteByte(address, data);
}

static uint16_t NvM_MakeU16(uint8_t low, uint8_t high)
{
    /* EEPROM 按 little-endian 存储 16-bit 字段，便于在十六进制 dump 中低字节先看见。 */
    return (uint16_t)((uint16_t)low | ((uint16_t)high << 8u));
}

static void NvM_PutU16(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFu);
    buffer[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

Std_ReturnType NvM_ReadBlock(NvM_BlockIdType block_id, uint8_t *data, uint16_t length)
{
    const NvM_BlockConfigType *config;
    uint8_t header[NVM_HEADER_SIZE];
    uint8_t payload[NVM_MAX_PAYLOAD_SIZE];
    uint16_t stored_magic;
    uint16_t stored_length;
    uint16_t stored_crc;
    uint16_t calc_crc;
    uint16_t index;

    if ((data == 0) || (length == 0u) || (length > NVM_MAX_PAYLOAD_SIZE))
    {
        /* NvM 不接受空指针、空长度和超过临时 payload 缓冲区的读请求。 */
        return E_NOT_OK;
    }

    config = NvM_FindConfig(block_id);
    if ((config == 0) || (length > config->max_length))
    {
        return E_NOT_OK;
    }

    for (index = 0u; index < NVM_HEADER_SIZE; index++)
    {
        if (NvM_ReadByte((uint16_t)(config->address + index), &header[index]) == 0u)
        {
            return E_NOT_OK;
        }
    }

    stored_magic = NvM_MakeU16(header[0], header[1]);
    stored_length = NvM_MakeU16(header[3], header[4]);
    stored_crc = NvM_MakeU16(header[5], header[6]);

    if ((stored_magic != config->magic) ||
        (header[2] != NVM_CFG_VERSION) ||
        (stored_length != length))
    {
        /*
         * magic/version/length 任一不匹配都认为块无效。
         * 这样结构体大小变化后不会把旧 EEPROM 数据误解释成新结构。
         */
        return E_NOT_OK;
    }

    for (index = 0u; index < stored_length; index++)
    {
        if (NvM_ReadByte((uint16_t)(config->address + NVM_HEADER_SIZE + index), &payload[index]) == 0u)
        {
            return E_NOT_OK;
        }
    }

    calc_crc = Crc_CalculateCrc16(payload, stored_length, NVM_CRC_START_VALUE);
    if (calc_crc != stored_crc)
    {
        /* CRC 不一致说明 EEPROM 数据损坏或写入中断，调用方会走默认值路径。 */
        return E_NOT_OK;
    }

    for (index = 0u; index < stored_length; index++)
    {
        data[index] = payload[index];
    }

    return E_OK;
}

Std_ReturnType NvM_WriteBlock(NvM_BlockIdType block_id, const uint8_t *data, uint16_t length)
{
    const NvM_BlockConfigType *config;
    uint8_t header[NVM_HEADER_SIZE];
    uint16_t crc;
    uint16_t index;

    if ((data == 0) || (length == 0u) || (length > NVM_MAX_PAYLOAD_SIZE))
    {
        return E_NOT_OK;
    }

    config = NvM_FindConfig(block_id);
    if ((config == 0) || (length > config->max_length))
    {
        return E_NOT_OK;
    }

    crc = Crc_CalculateCrc16(data, length, NVM_CRC_START_VALUE);
    /*
     * 头部在 RAM 中一次性拼好，再逐字节写入 EEPROM。
     * 当前 Eep 接口是字节粒度，后续若换页写，也只需要调整 NvM_WriteByte 封装。
     */
    NvM_PutU16(&header[0], config->magic);
    header[2] = NVM_CFG_VERSION;
    NvM_PutU16(&header[3], length);
    NvM_PutU16(&header[5], crc);

    for (index = 0u; index < NVM_HEADER_SIZE; index++)
    {
        if (NvM_WriteByte((uint16_t)(config->address + index), header[index]) == 0u)
        {
            return E_NOT_OK;
        }
    }

    for (index = 0u; index < length; index++)
    {
        if (NvM_WriteByte((uint16_t)(config->address + NVM_HEADER_SIZE + index), data[index]) == 0u)
        {
            return E_NOT_OK;
        }
    }

    return E_OK;
}

static void NvM_LoadOrDefaultBootInfo(void)
{
    if (NvM_ReadBlock(NVM_BLOCK_BOOT_INFO,
                      (uint8_t *)&NvM_BootInfo,
                      sizeof(NvM_BootInfo)) != E_OK)
    {
        /*
         * 首次上电或 EEPROM 校验失败时使用安全默认值。
         * reserved 清零，避免未来字段扩展时读到随机数据。
         */
        NvM_BootInfo.boot_counter = 0u;
        NvM_BootInfo.last_reset_reason = 0u;
        NvM_BootInfo.reserved[0] = 0u;
        NvM_BootInfo.reserved[1] = 0u;
        NvM_BootInfo.reserved[2] = 0u;
    }

    /* boot_counter 每次初始化递增，并立即写回，便于诊断 DID 观察重启次数。 */
    NvM_BootInfo.boot_counter++;
    (void)NvM_WriteBlock(NVM_BLOCK_BOOT_INFO,
                         (const uint8_t *)&NvM_BootInfo,
                         sizeof(NvM_BootInfo));
}

static void NvM_LoadOrDefaultSystemConfig(void)
{
    if (NvM_ReadBlock(NVM_BLOCK_SYSTEM_CONFIG,
                      (uint8_t *)&NvM_SystemConfig,
                      sizeof(NvM_SystemConfig)) != E_OK)
    {
        /*
         * 系统配置读不到时写入默认配置。
         * 这样后续 App_Dashboard_Init() 可以始终拿到一份完整配置。
         */
        NvM_SystemConfig.backlight_level = NVM_CFG_SYSTEM_BACKLIGHT_DEFAULT;
        NvM_SystemConfig.buzzer_enable = NVM_CFG_SYSTEM_BUZZER_DEFAULT;
        NvM_SystemConfig.theme = NVM_CFG_SYSTEM_THEME_DEFAULT;
        NvM_SystemConfig.reserved = 0u;

        (void)NvM_WriteBlock(NVM_BLOCK_SYSTEM_CONFIG,
                             (const uint8_t *)&NvM_SystemConfig,
                             sizeof(NvM_SystemConfig));
    }
}

void NvM_Init(void)
{
    Eep_Init();
    NvM_LoadOrDefaultBootInfo();
    NvM_LoadOrDefaultSystemConfig();
    NvM_Initialized = 1u;
    LogM_Info("NvM init ok");
}

void NvM_MainFunction(uint32_t tick_ms)
{
    (void)tick_ms;
}

const NvM_BootInfoType *NvM_GetBootInfo(void)
{
    return &NvM_BootInfo;
}

const NvM_SystemConfigType *NvM_GetSystemConfig(void)
{
    return &NvM_SystemConfig;
}

Std_ReturnType NvM_SetSystemConfig(const NvM_SystemConfigType *config)
{
    if ((config == 0) || (NvM_Initialized == 0u))
    {
        return E_NOT_OK;
    }

    NvM_SystemConfig = *config;
    return NvM_WriteBlock(NVM_BLOCK_SYSTEM_CONFIG,
                          (const uint8_t *)&NvM_SystemConfig,
                          sizeof(NvM_SystemConfig));
}

void NvM_WriteAll(void)
{
    if (NvM_Initialized == 0u)
    {
        /* 初始化未完成时不写 EEPROM，避免把未准备好的 RAM 镜像覆盖到持久区。 */
        return;
    }

    (void)NvM_WriteBlock(NVM_BLOCK_BOOT_INFO,
                         (const uint8_t *)&NvM_BootInfo,
                         sizeof(NvM_BootInfo));
    (void)NvM_WriteBlock(NVM_BLOCK_SYSTEM_CONFIG,
                         (const uint8_t *)&NvM_SystemConfig,
                         sizeof(NvM_SystemConfig));
}
