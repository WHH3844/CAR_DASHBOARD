#include "RtcIf.h"

#include "I2c.h"

#define RTCIF_DS3231_ADDR             0x68u
#define RTCIF_REG_SECONDS             0x00u
#define RTCIF_REG_CONTROL             0x0Eu
#define RTCIF_REG_STATUS              0x0Fu
#define RTCIF_CONTROL_EOSC            0x80u
#define RTCIF_STATUS_OSF              0x80u
#define RTCIF_TIMEOUT_LOOP            50000u

static uint8_t RtcIf_BcdToBin(uint8_t bcd)
{
    return (uint8_t)(((bcd >> 4u) * 10u) + (bcd & 0x0Fu));
}

static uint8_t RtcIf_BinToBcd(uint8_t bin)
{
    return (uint8_t)(((bin / 10u) << 4u) | (bin % 10u));
}

static uint8_t RtcIf_ReadReg(uint8_t reg, uint8_t *data)
{
    return I2c0_ReadByteAfterWriteByte(RTCIF_DS3231_ADDR,
                                       reg,
                                       data,
                                       RTCIF_TIMEOUT_LOOP);
}

static uint8_t RtcIf_WriteReg(uint8_t reg, uint8_t data)
{
    uint8_t buffer[2];

    buffer[0] = reg;
    buffer[1] = data;
    return I2c0_WriteBytes(RTCIF_DS3231_ADDR,
                           buffer,
                           2u,
                           RTCIF_TIMEOUT_LOOP);
}

uint8_t RtcIf_Init(void)
{
    I2c0_Init100K();
    return I2c0_ProbeAddress(RTCIF_DS3231_ADDR, RTCIF_TIMEOUT_LOOP);
}

uint8_t RtcIf_StartOscillator(void)
{
    uint8_t control;

    if (RtcIf_ReadReg(RTCIF_REG_CONTROL, &control) == 0u)
    {
        return 0u;
    }

    control = (uint8_t)(control & (uint8_t)(~RTCIF_CONTROL_EOSC));
    return RtcIf_WriteReg(RTCIF_REG_CONTROL, control);
}

uint8_t RtcIf_ReadStatus(uint8_t *status)
{
    if (status == 0)
    {
        return 0u;
    }

    return RtcIf_ReadReg(RTCIF_REG_STATUS, status);
}

uint8_t RtcIf_ClearOscStopFlag(void)
{
    uint8_t status;

    if (RtcIf_ReadStatus(&status) == 0u)
    {
        return 0u;
    }

    status = (uint8_t)(status & (uint8_t)(~RTCIF_STATUS_OSF));
    return RtcIf_WriteReg(RTCIF_REG_STATUS, status);
}

uint8_t RtcIf_ReadTime(RtcIf_TimeType *time)
{
    uint8_t reg[7];
    uint8_t index;
    uint8_t hour_reg;

    if (time == 0)
    {
        return 0u;
    }

    for (index = 0u; index < 7u; index++)
    {
        if (RtcIf_ReadReg((uint8_t)(RTCIF_REG_SECONDS + index), &reg[index]) == 0u)
        {
            return 0u;
        }
    }

    time->second = RtcIf_BcdToBin((uint8_t)(reg[0] & 0x7Fu));
    time->minute = RtcIf_BcdToBin((uint8_t)(reg[1] & 0x7Fu));

    hour_reg = reg[2];
    if ((hour_reg & 0x40u) != 0u)
    {
        time->hour = RtcIf_BcdToBin((uint8_t)(hour_reg & 0x1Fu));
        if ((hour_reg & 0x20u) != 0u)
        {
            if (time->hour != 12u)
            {
                time->hour = (uint8_t)(time->hour + 12u);
            }
        }
        else if (time->hour == 12u)
        {
            time->hour = 0u;
        }
    }
    else
    {
        time->hour = RtcIf_BcdToBin((uint8_t)(hour_reg & 0x3Fu));
    }

    time->weekday = RtcIf_BcdToBin((uint8_t)(reg[3] & 0x07u));
    time->date = RtcIf_BcdToBin((uint8_t)(reg[4] & 0x3Fu));
    time->month = RtcIf_BcdToBin((uint8_t)(reg[5] & 0x1Fu));
    time->year = (uint16_t)(2000u + RtcIf_BcdToBin(reg[6]));

    return 1u;
}

uint8_t RtcIf_SetTime(const RtcIf_TimeType *time)
{
    uint8_t buffer[8];

    if ((time == 0) || (RtcIf_IsTimeValid(time) == 0u))
    {
        return 0u;
    }

    buffer[0] = RTCIF_REG_SECONDS;
    buffer[1] = RtcIf_BinToBcd(time->second);
    buffer[2] = RtcIf_BinToBcd(time->minute);
    buffer[3] = RtcIf_BinToBcd(time->hour);
    buffer[4] = RtcIf_BinToBcd(time->weekday);
    buffer[5] = RtcIf_BinToBcd(time->date);
    buffer[6] = RtcIf_BinToBcd(time->month);
    buffer[7] = RtcIf_BinToBcd((uint8_t)(time->year - 2000u));

    return I2c0_WriteBytes(RTCIF_DS3231_ADDR,
                           buffer,
                           8u,
                           RTCIF_TIMEOUT_LOOP);
}

uint8_t RtcIf_IsTimeValid(const RtcIf_TimeType *time)
{
    if (time == 0)
    {
        return 0u;
    }

    if ((time->year < 2024u) || (time->year > 2099u))
    {
        return 0u;
    }

    if ((time->month < 1u) || (time->month > 12u))
    {
        return 0u;
    }

    if ((time->date < 1u) || (time->date > 31u))
    {
        return 0u;
    }

    if ((time->weekday < 1u) || (time->weekday > 7u))
    {
        return 0u;
    }

    if ((time->hour > 23u) || (time->minute > 59u) || (time->second > 59u))
    {
        return 0u;
    }

    return 1u;
}
