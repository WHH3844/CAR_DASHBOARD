#ifndef ECUM_H
#define ECUM_H

#include <stdint.h>

typedef enum
{
    /* 尚未进入启动流程或已经掉电。 */
    ECUM_STATE_OFF = 0u,
    /* 上电保持、基础日志初始化之前的早期启动阶段。 */
    ECUM_STATE_BOOT,
    /* BSW/RTE/硬件抽象初始化和关键外设自检阶段。 */
    ECUM_STATE_SELF_TEST,
    /* 正常运行阶段，EcuM_MainFunction 会调度各周期任务。 */
    ECUM_STATE_RUN,
    /* 收到关机请求后，正在保存 NvM/Dem 并准备断电。 */
    ECUM_STATE_SLEEP_PREPARE,
    /* 关键硬件初始化失败，系统停留在故障态，不进入业务调度。 */
    ECUM_STATE_FAULT
} EcuM_StateType;

/*
 * ECU Manager 生命周期入口。
 *
 * 完成上电自锁、基础服务初始化、SDRAM/LCD/CAN 等硬件路径初始化和 APP 初始化。
 */
void EcuM_Init(void);

/* 主调度函数，由 Os_Start() 创建的 FreeRTOS 任务或裸机 super loop 周期调用。 */
void EcuM_MainFunction(void);

/* 进入 OS 调度；根据 APP_CFG_USE_FREERTOS 选择 FreeRTOS 或裸机循环。 */
void EcuM_MainLoop(void);

/* 裸机模式下推进 EcuM 软件时间，FreeRTOS 模式下时间来自 RTOS tick。 */
void EcuM_AdvanceTick(uint32_t elapsed_ms);

/* 调试/诊断用状态读取接口。 */
EcuM_StateType EcuM_GetState(void);

/* 返回 EcuM 当前毫秒时间基准。 */
uint32_t EcuM_GetTickMs(void);

#endif /* ECUM_H */
