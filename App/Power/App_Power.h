#ifndef APP_POWER_H
#define APP_POWER_H

#include <stdint.h>

void App_Power_Init(void);
void App_Power_MainFunction(uint32_t tick_ms);
uint8_t App_Power_IsShutdownRequested(void);
void App_Power_ClearShutdownRequest(void);

#endif /* APP_POWER_H */
#ifndef APP_POWER_H
#define APP_POWER_H

#include <stdint.h>

void App_Power_Init(void);
void App_Power_MainFunction(uint16_t elapsed_ms);

#endif /* APP_POWER_H */
