#include "NvM.h"

#include "Crc.h"
#include "Eep.h"
#include "LogM.h"
#include "NvM_Cfg.h"

#define NVM_HEADER_SIZE             7u
#define NVM_MAX_PAYLOAD_SIZE        128u
#define NVM_CRC_START_VALUE         0xFFFFu

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
        NvM_BootInfo.boot_counter = 0u;
        NvM_BootInfo.last_reset_reason = 0u;
        NvM_BootInfo.reserved[0] = 0u;
        NvM_BootInfo.reserved[1] = 0u;
        NvM_BootInfo.reserved[2] = 0u;
    }

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
        return;
    }

    (void)NvM_WriteBlock(NVM_BLOCK_BOOT_INFO,
                         (const uint8_t *)&NvM_BootInfo,
                         sizeof(NvM_BootInfo));
    (void)NvM_WriteBlock(NVM_BLOCK_SYSTEM_CONFIG,
                         (const uint8_t *)&NvM_SystemConfig,
                         sizeof(NvM_SystemConfig));
}
#include "NvM.h"

#include "Crc.h"
#include "Dem.h"
#include "Eep.h"
#include "NvM_Cfg.h"

#define NVM_BOOT_PAYLOAD_SIZE       4u
#define NVM_SYSTEM_PAYLOAD_SIZE     4u
#define NVM_DEM_PAYLOAD_SIZE        (DEM_EVENT_COUNT * 3u)
#define NVM_HEADER_SIZE             4u
#define NVM_WRITE_DELAY_MS          1000u

static uint16_t NvM_BootCounter;
static NvM_SystemConfigType NvM_SystemConfig;
static uint8_t NvM_BootDirty;
static uint8_t NvM_SystemDirty;
static uint8_t NvM_DemDirty;
static uint16_t NvM_DirtyAgeMs;

static uint8_t NvM_ReadByteArray(uint16_t address, uint8_t *data, uint16_t length)
{
    uint16_t index;

    for (index = 0u; index < length; index++)
    {
        if (Eep_ReadByte((uint16_t)(address + index), &data[index]) == 0u)
        {
            return 0u;
        }
    }

    return 1u;
}

static uint8_t NvM_WriteByteArray(uint16_t address, const uint8_t *data, uint16_t length)
{
    uint16_t index;

    for (index = 0u; index < length; index++)
    {
        if (Eep_WriteByte((uint16_t)(address + index), data[index]) == 0u)
        {
            return 0u;
        }
    }

    return 1u;
}

static uint8_t NvM_ReadBlock(uint16_t address,
                             uint16_t magic,
                             uint8_t *payload,
                             uint16_t payload_length)
{
    uint8_t header[NVM_HEADER_SIZE];
    uint16_t stored_magic;
    uint16_t stored_crc;
    uint16_t calc_crc;

    if (NvM_ReadByteArray(address, header, sizeof(header)) == 0u)
    {
        return 0u;
    }

    stored_magic = (uint16_t)((uint16_t)header[0] | ((uint16_t)header[1] << 8u));
    stored_crc = (uint16_t)((uint16_t)header[2] | ((uint16_t)header[3] << 8u));
    if (stored_magic != magic)
    {
        return 0u;
    }

    if (NvM_ReadByteArray((uint16_t)(address + NVM_HEADER_SIZE), payload, payload_length) == 0u)
    {
        return 0u;
    }

    calc_crc = Crc_CalculateCrc16(payload, payload_length, 0xFFFFu);
    return (calc_crc == stored_crc) ? 1u : 0u;
}

static uint8_t NvM_WriteBlock(uint16_t address,
                              uint16_t magic,
                              const uint8_t *payload,
                              uint16_t payload_length)
{
    uint8_t header[NVM_HEADER_SIZE];
    uint16_t crc;

    crc = Crc_CalculateCrc16(payload, payload_length, 0xFFFFu);
    header[0] = (uint8_t)(magic & 0xFFu);
    header[1] = (uint8_t)((magic >> 8u) & 0xFFu);
    header[2] = (uint8_t)(crc & 0xFFu);
    header[3] = (uint8_t)((crc >> 8u) & 0xFFu);

    if (NvM_WriteByteArray(address, header, sizeof(header)) == 0u)
    {
        return 0u;
    }

    return NvM_WriteByteArray((uint16_t)(address + NVM_HEADER_SIZE),
                              payload,
                              payload_length);
}

static void NvM_LoadBootInfo(void)
{
    uint8_t payload[NVM_BOOT_PAYLOAD_SIZE];

    if (NvM_ReadBlock(NVM_CFG_ADDR_BOOT_INFO,
                      NVM_CFG_MAGIC_BOOT_INFO,
                      payload,
                      sizeof(payload)) != 0u)
    {
        NvM_BootCounter = (uint16_t)((uint16_t)payload[0] | ((uint16_t)payload[1] << 8u));
    }
    else
    {
        NvM_BootCounter = 0u;
    }

    if (NvM_BootCounter < 0xFFFFu)
    {
        NvM_BootCounter++;
    }
    NvM_BootDirty = 1u;
}

static void NvM_LoadSystemConfig(void)
{
    uint8_t payload[NVM_SYSTEM_PAYLOAD_SIZE];

    if (NvM_ReadBlock(NVM_CFG_ADDR_SYSTEM_CONFIG,
                      NVM_CFG_MAGIC_SYSTEM_CONFIG,
                      payload,
                      sizeof(payload)) != 0u)
    {
        NvM_SystemConfig.theme = payload[0];
        NvM_SystemConfig.backlight_level = payload[1];
        NvM_SystemConfig.buzzer_enable = payload[2];
        NvM_SystemConfig.can_baudrate_index = payload[3];
    }
    else
    {
        NvM_SystemConfig.theme = NVM_CFG_SYSTEM_THEME_DEFAULT;
        NvM_SystemConfig.backlight_level = NVM_CFG_SYSTEM_BACKLIGHT_DEFAULT;
        NvM_SystemConfig.buzzer_enable = NVM_CFG_SYSTEM_BUZZER_DEFAULT;
        NvM_SystemConfig.can_baudrate_index = 0u;
        NvM_SystemDirty = 1u;
    }
}

static void NvM_LoadDemStatus(void)
{
    uint8_t payload[NVM_DEM_PAYLOAD_SIZE];

    if (NvM_ReadBlock(NVM_CFG_ADDR_DEM_STATUS,
                      NVM_CFG_MAGIC_DEM_STATUS,
                      payload,
                      sizeof(payload)) != 0u)
    {
        Dem_LoadNvMData(payload, sizeof(payload));
    }
    else
    {
        NvM_DemDirty = 1u;
    }
}

static Std_ReturnType NvM_WriteBootInfo(void)
{
    uint8_t payload[NVM_BOOT_PAYLOAD_SIZE];

    payload[0] = (uint8_t)(NvM_BootCounter & 0xFFu);
    payload[1] = (uint8_t)((NvM_BootCounter >> 8u) & 0xFFu);
    payload[2] = NVM_CFG_VERSION;
    payload[3] = 0u;

    return (NvM_WriteBlock(NVM_CFG_ADDR_BOOT_INFO,
                           NVM_CFG_MAGIC_BOOT_INFO,
                           payload,
                           sizeof(payload)) != 0u) ? E_OK : E_NOT_OK;
}

static Std_ReturnType NvM_WriteSystemConfig(void)
{
    uint8_t payload[NVM_SYSTEM_PAYLOAD_SIZE];

    payload[0] = NvM_SystemConfig.theme;
    payload[1] = NvM_SystemConfig.backlight_level;
    payload[2] = NvM_SystemConfig.buzzer_enable;
    payload[3] = NvM_SystemConfig.can_baudrate_index;

    return (NvM_WriteBlock(NVM_CFG_ADDR_SYSTEM_CONFIG,
                           NVM_CFG_MAGIC_SYSTEM_CONFIG,
                           payload,
                           sizeof(payload)) != 0u) ? E_OK : E_NOT_OK;
}

static Std_ReturnType NvM_WriteDemStatus(void)
{
    uint8_t payload[NVM_DEM_PAYLOAD_SIZE];

    if (Dem_GetNvMData(payload, sizeof(payload)) == 0u)
    {
        return E_NOT_OK;
    }

    return (NvM_WriteBlock(NVM_CFG_ADDR_DEM_STATUS,
                           NVM_CFG_MAGIC_DEM_STATUS,
                           payload,
                           sizeof(payload)) != 0u) ? E_OK : E_NOT_OK;
}

void NvM_Init(void)
{
    Eep_Init();
    NvM_BootDirty = 0u;
    NvM_SystemDirty = 0u;
    NvM_DemDirty = 0u;
    NvM_DirtyAgeMs = 0u;

    NvM_LoadBootInfo();
    NvM_LoadSystemConfig();
    NvM_LoadDemStatus();
}

void NvM_MainFunction(uint16_t elapsed_ms)
{
    if ((NvM_BootDirty == 0u) && (NvM_SystemDirty == 0u) && (NvM_DemDirty == 0u))
    {
        NvM_DirtyAgeMs = 0u;
        return;
    }

    if (NvM_DirtyAgeMs < NVM_WRITE_DELAY_MS)
    {
        NvM_DirtyAgeMs = (uint16_t)(NvM_DirtyAgeMs + elapsed_ms);
        return;
    }

    (void)NvM_WriteAll();
}

Std_ReturnType NvM_WriteAll(void)
{
    Std_ReturnType result;

    result = E_OK;

    if (NvM_BootDirty != 0u)
    {
        if (NvM_WriteBootInfo() == E_OK)
        {
            NvM_BootDirty = 0u;
        }
        else
        {
            result = E_NOT_OK;
        }
    }

    if (NvM_SystemDirty != 0u)
    {
        if (NvM_WriteSystemConfig() == E_OK)
        {
            NvM_SystemDirty = 0u;
        }
        else
        {
            result = E_NOT_OK;
        }
    }

    if (NvM_DemDirty != 0u)
    {
        if (NvM_WriteDemStatus() == E_OK)
        {
            NvM_DemDirty = 0u;
        }
        else
        {
            result = E_NOT_OK;
        }
    }

    NvM_DirtyAgeMs = 0u;
    return result;
}

void NvM_MarkDemDirty(void)
{
    NvM_DemDirty = 1u;
}

void NvM_MarkSystemConfigDirty(void)
{
    NvM_SystemDirty = 1u;
}

uint16_t NvM_GetBootCounter(void)
{
    return NvM_BootCounter;
}

const NvM_SystemConfigType *NvM_GetSystemConfig(void)
{
    return &NvM_SystemConfig;
}

void NvM_SetSystemConfig(const NvM_SystemConfigType *config)
{
    if (config == 0)
    {
        return;
    }

    NvM_SystemConfig = *config;
    if (NvM_SystemConfig.backlight_level > 100u)
    {
        NvM_SystemConfig.backlight_level = 100u;
    }
    NvM_SystemConfig.buzzer_enable = (NvM_SystemConfig.buzzer_enable != 0u) ? 1u : 0u;
    NvM_MarkSystemConfigDirty();
}
