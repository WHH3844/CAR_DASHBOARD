#ifndef BUZZERIF_H
#define BUZZERIF_H

#include <stdint.h>

void BuzzerIf_Init(void);
void BuzzerIf_On(void);
void BuzzerIf_Off(void);
void BuzzerIf_Set(uint8_t on);

#endif /* BUZZERIF_H */
