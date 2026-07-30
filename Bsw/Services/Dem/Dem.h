#ifndef DEM_H
#define DEM_H

#include "Dem_Cfg.h"
#include "Std_Types.h"

#include <stdint.h>

typedef enum
{
    /* 测试项当前通过，Dem 会清除 testFailed 位，但保留 confirmed 历史。 */
    DEM_EVENT_STATUS_PASSED = 0u,
    /* 测试项当前失败，Dem 会置位 testFailed/confirmed，并按配置点亮 warning 位。 */
    DEM_EVENT_STATUS_FAILED = 1u
} Dem_EventStatusType;

typedef struct
{
    /* 24-bit UDS DTC 编号，存放在 uint32_t 低 24 位。 */
    uint32_t dtc;
    /* UDS DTC statusOfDTC 位图，例如 testFailed、confirmed、warningIndicator。 */
    uint8_t status;
    /* 从 passed 到 failed 的边沿累计次数，持续故障不会每个周期累加。 */
    uint16_t occurrence_counter;
} Dem_DtcRecordType;

/*
 * Diagnostic Event Manager 简化实现。
 *
 * 管理项目内所有故障事件的状态、发生次数和 NvM 持久化数据，
 * 为 Dcm 的 0x19/0x14 服务提供 DTC 查询和清除能力。
 */
void Dem_Init(void);

/* 周期保存脏数据，避免故障抖动时高频写 EEPROM。 */
void Dem_MainFunction(uint32_t tick_ms);

/* 更新事件状态，event_id 必须小于 DEM_EVENT_COUNT。 */
void Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status);

/* 按 UDS status mask 统计匹配的 DTC 数量。 */
uint8_t Dem_GetDtcCountByStatusMask(uint8_t status_mask);

/* 返回第一个匹配 status mask 的 DTC 记录，适合单帧诊断演示。 */
Std_ReturnType Dem_GetFirstDtcByStatusMask(uint8_t status_mask, Dem_DtcRecordType *record);

/* 统计 confirmedDTC 位已经置位的故障数量。 */
uint8_t Dem_GetConfirmedDtcCount(void);

/* 返回最近一次确认的本地 FaultId（1..DEM_EVENT_COUNT），无故障时为 0xFFFF。 */
uint16_t Dem_GetLastFaultId(void);

/* 返回指定事件的发生次数或当前 testFailed 状态。 */
uint16_t Dem_GetOccurrenceCounter(Dem_EventIdType event_id);
uint8_t Dem_IsEventFailed(Dem_EventIdType event_id);

/* 返回 Dem 是否存在待持久化状态，供 0x326 NvMDirty 使用。 */
uint8_t Dem_IsDirty(void);

/* 清除所有 DTC 状态和发生次数，并立即写回 NvM。 */
void Dem_ClearAllDtc(void);

/* 关机前强制保存一次，确保最近故障不会因为周期未到而丢失。 */
void Dem_SaveNow(void);

#endif /* DEM_H */
