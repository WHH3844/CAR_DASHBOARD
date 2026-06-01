#ifndef RTCIF_H
#define RTCIF_H

#include <stdint.h>

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RtcIf_TimeType;

uint8_t RtcIf_Init(void);
uint8_t RtcIf_StartOscillator(void);
uint8_t RtcIf_ReadTime(RtcIf_TimeType *time);
uint8_t RtcIf_SetTime(const RtcIf_TimeType *time);
uint8_t RtcIf_ReadStatus(uint8_t *status);
uint8_t RtcIf_ClearOscStopFlag(void);
uint8_t RtcIf_IsTimeValid(const RtcIf_TimeType *time);

#endif /* RTCIF_H */
