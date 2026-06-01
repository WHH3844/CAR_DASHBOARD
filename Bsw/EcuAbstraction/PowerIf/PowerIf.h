#ifndef POWERIF_H
#define POWERIF_H

#include <stdint.h>

void PowerIf_Init(void);
void PowerIf_HoldOn(void);
void PowerIf_HoldOff(void);
uint8_t PowerIf_KeyIsPressed(void);
uint8_t PowerIf_BootCheckAndHold(void);
uint8_t PowerIf_WaitKeyPressAndHold(uint32_t timeout_ms);
uint8_t PowerIf_WaitKeyRelease(uint32_t timeout_ms);
uint8_t PowerIf_LongPressShutdownTask(uint32_t long_press_ms, uint32_t sample_ms);
void PowerIf_Shutdown(void);

#endif /* POWERIF_H */
