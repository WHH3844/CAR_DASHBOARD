#ifndef BACKLIGHTIF_H
#define BACKLIGHTIF_H

#include <stdint.h>

void BacklightIf_Init(void);
void BacklightIf_SetLevel(uint8_t level);
uint8_t BacklightIf_GetLevel(void);

#endif /* BACKLIGHTIF_H */
#ifndef BACKLIGHTIF_H
#define BACKLIGHTIF_H

#include "Std_Types.h"

#include <stdint.h>

void BacklightIf_Init(void);
void BacklightIf_SetLevel(uint8_t level);
uint8_t BacklightIf_GetLevel(void);
void BacklightIf_Off(void);

#endif /* BACKLIGHTIF_H */
