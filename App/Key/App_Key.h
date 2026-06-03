#ifndef APP_KEY_H
#define APP_KEY_H

#include <stdint.h>

/*
 * 用户按键应用层。
 *
 * 负责把 IoHwAb 读取到的原始按键电平转换为稳定的 RTE 按键事件。
 * 当前只发布短按的“按下沿”，释放沿不进入业务层，避免业务模块同时处理按下/释放
 * 两类状态造成重复动作。
 */
void App_Key_Init(void);

/*
 * 按键扫描入口，建议按 APP_CFG_KEY_PERIOD_MS 周期调用。
 *
 * tick_ms 用于软件去抖：原始电平变化后必须保持 APP_KEY_DEBOUNCE_MS，
 * 才会更新稳定状态并发布按键事件。
 */
void App_Key_MainFunction(uint32_t tick_ms);

#endif /* APP_KEY_H */
