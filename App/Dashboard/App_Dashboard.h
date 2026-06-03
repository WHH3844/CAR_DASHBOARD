#ifndef APP_DASHBOARD_H
#define APP_DASHBOARD_H

#include <stdint.h>

/*
 * 仪表业务控制模块。
 *
 * 该模块不直接操作硬件，只通过 RTE 读写仪表数据、蜂鸣器静音状态和模拟模式。
 * 主要职责：
 * 1. 响应按键事件：KEY1 切换模拟数据，KEY2 切换蜂鸣器静音，KEY3 清除车速/转速。
 * 2. 在模拟模式下周期生成车速和转速，方便脱离整车 CAN 报文做 LCD/报警联调。
 * 3. 根据车速、转速阈值计算报警状态，并驱动蜂鸣器提示。
 */
void App_Dashboard_Init(void);

/*
 * 10ms 主调度入口。
 *
 * tick_ms 使用 EcuM/Os 统一的毫秒时间基准，内部用它做按键提示音时长、
 * 模拟数据刷新间隔和报警蜂鸣节拍控制。
 */
void App_Dashboard_MainFunction(uint32_t tick_ms);

#endif /* APP_DASHBOARD_H */
