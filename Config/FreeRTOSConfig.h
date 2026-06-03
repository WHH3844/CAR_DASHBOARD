#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "gd32f4xx.h"

/*
 * FreeRTOSConfig.h 是 FreeRTOS 移植的核心配置文件。
 * 本项目第一版目标是“能稳定跑正式 EcuM 主循环”，所以先关闭复杂特性，
 * 后续再按需要打开软件定时器、事件组、多任务拆分等能力。
 */

#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      ((TickType_t)1000)
#define configMAX_PRIORITIES                    5
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                   ((size_t)(24u * 1024u))
/*
 * 目前只创建 EcuM 主任务，24KB heap 主要覆盖任务栈和内核对象。
 * 如果后续拆分显示、CAN、传感器任务，需要同步复核 heap 和各任务栈深度。
 */
#define configMAX_TASK_NAME_LEN                 12
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               4
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     1

#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0

#define configUSE_IDLE_HOOK                     1
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1

#define configUSE_TIMERS                        0
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                5
#define configTIMER_TASK_STACK_DEPTH            256

/*
 * GD32F470 是 Cortex-M4F，NVIC 优先级位数按 CMSIS 的 __NVIC_PRIO_BITS。
 * FreeRTOS 要求 SysTick/PendSV 在最低优先级，能调用 FreeRTOS API 的中断
 * 优先级不能高于 configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY。
 */
#define configPRIO_BITS                         __NVIC_PRIO_BITS
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1

/*
 * FreeRTOS RVDS/ARM_CM4F 端口默认导出下面三个函数名。
 * 启动文件向量表使用 CMSIS 标准名字，所以这里做一次映射。
 */
#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler
#define xPortSysTickHandler                     SysTick_Handler

#define configASSERT(x)                         if ((x) == 0) { __disable_irq(); for (;;) {} }

#endif /* FREERTOS_CONFIG_H */
