#ifndef IOHWAB_H
#define IOHWAB_H

#include <stdint.h>

#define IOHWAB_KEY1_MASK    0x01u
#define IOHWAB_KEY2_MASK    0x02u
#define IOHWAB_KEY3_MASK    0x04u

void IoHwAb_KeyInit(void);
uint8_t IoHwAb_ReadUserKeyMask(void);
uint8_t IoHwAb_Key1IsPressed(void);
uint8_t IoHwAb_Key2IsPressed(void);
uint8_t IoHwAb_Key3IsPressed(void);

#endif /* IOHWAB_H */
