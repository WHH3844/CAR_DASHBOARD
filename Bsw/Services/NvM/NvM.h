#ifndef NVM_H
#define NVM_H

#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    /* 启动计数、最近复位原因等小体积启动信息。 */
    NVM_BLOCK_BOOT_INFO = 0u,
    /* 用户配置：背光、蜂鸣器、主题等。 */
    NVM_BLOCK_SYSTEM_CONFIG,
    /* Dem 事件状态和发生次数持久化区。 */
    NVM_BLOCK_DEM_STATUS
} NvM_BlockIdType;

typedef struct
{
    /* 每次 NvM_Init 成功读取/默认化后加 1，可通过 DID 查看启动次数。 */
    uint32_t boot_counter;
    /* 预留给后续复位原因映射，例如看门狗、软件复位、低压复位。 */
    uint8_t last_reset_reason;
    uint8_t reserved[3];
} NvM_BootInfoType;

typedef struct
{
    /* 背光百分比 0~100，启动时由 App_Dashboard 写入 RTE/BacklightIf。 */
    uint8_t backlight_level;
    /* 0 表示默认静音，非 0 表示允许报警蜂鸣。 */
    uint8_t buzzer_enable;
    /* UI 主题预留字段，当前显示层暂未使用。 */
    uint8_t theme;
    uint8_t reserved;
} NvM_SystemConfigType;

/*
 * Non-volatile Memory 简化管理器。
 *
 * 每个块使用固定地址 + 7 字节头部 + payload 的格式保存到 EEPROM：
 * magic(2) + version(1) + length(2) + crc16(2)。
 */
void NvM_Init(void);

/* 预留周期任务入口；当前块写入是同步完成的。 */
void NvM_MainFunction(uint32_t tick_ms);

/* 读取指定 NvM 块，校验 magic/version/length/crc 后才拷贝 payload。 */
Std_ReturnType NvM_ReadBlock(NvM_BlockIdType block_id, uint8_t *data, uint16_t length);

/* 写入指定 NvM 块，先写头部再写 payload，长度不能超过块配置上限。 */
Std_ReturnType NvM_WriteBlock(NvM_BlockIdType block_id, const uint8_t *data, uint16_t length);

/* 返回 RAM 中的启动信息快照。指针归 NvM 所有，调用方不要修改。 */
const NvM_BootInfoType *NvM_GetBootInfo(void);

/* 返回 RAM 中的系统配置快照。指针归 NvM 所有，调用方不要修改。 */
const NvM_SystemConfigType *NvM_GetSystemConfig(void);

/* 更新系统配置并立即写入 EEPROM。 */
Std_ReturnType NvM_SetSystemConfig(const NvM_SystemConfigType *config);

/* 关机前保存所有需要持久化的 NvM RAM 镜像。 */
void NvM_WriteAll(void);

#endif /* NVM_H */
