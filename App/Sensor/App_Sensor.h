#ifndef APP_SENSOR_H
#define APP_SENSOR_H

#include <stdint.h>

/*
 * 传感器采集应用层。
 *
 * 负责 RTC 和 SHT30 的初始化、周期采样和故障上报。
 * 采样结果写入 RTE，显示和诊断模块只从 RTE 取数据，不直接访问 I2C 设备。
 */
void App_Sensor_Init(void);

/*
 * 传感器周期任务。
 *
 * tick_ms 用于分别调度 RTC 和 SHT30 的 1s 采样节拍；若某个设备初始化失败，
 * 对应采样函数会保持退出，并通过 Dem 保留故障状态。
 */
void App_Sensor_MainFunction(uint32_t tick_ms);

#endif /* APP_SENSOR_H */
