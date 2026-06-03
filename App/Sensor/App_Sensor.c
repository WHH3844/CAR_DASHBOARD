#include "App_Sensor.h"

#include "Dem.h"
#include "RtcIf.h"
#include "Rte_Signal.h"
#include "SensorIf.h"

static uint8_t App_Sensor_RtcOk;
static uint8_t App_Sensor_Sht30Ok;
static uint32_t App_Sensor_NextRtcMs;
static uint32_t App_Sensor_NextShtMs;

void App_Sensor_Init(void)
{
    App_Sensor_RtcOk = RtcIf_Init();
    if (App_Sensor_RtcOk != 0u)
    {
        (void)RtcIf_StartOscillator();
        Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
    }
    else
    {
        Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_FAILED);
    }

    App_Sensor_Sht30Ok = SensorIf_Sht30Init();
    Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED,
                       (App_Sensor_Sht30Ok != 0u) ? DEM_EVENT_STATUS_PASSED : DEM_EVENT_STATUS_FAILED);

    App_Sensor_NextRtcMs = 0u;
    App_Sensor_NextShtMs = 0u;
}

static void App_Sensor_UpdateRtc(void)
{
    RtcIf_TimeType time;

    if (App_Sensor_RtcOk == 0u)
    {
        return;
    }

    if (RtcIf_ReadTime(&time) != 0u)
    {
        if (RtcIf_IsTimeValid(&time) != 0u)
        {
            (void)Rte_Write_RtcTime(&time, 1u);
            Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
            Dem_SetEventStatus(DEM_EVENT_RTC_TIME_INVALID, DEM_EVENT_STATUS_PASSED);
        }
        else
        {
            (void)Rte_Write_RtcTime(&time, 0u);
            Dem_SetEventStatus(DEM_EVENT_RTC_TIME_INVALID, DEM_EVENT_STATUS_FAILED);
        }
    }
    else
    {
        Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_FAILED);
    }
}

static void App_Sensor_UpdateSht30(void)
{
    SensorIf_Sht30DataType sht;
    Rte_EnvironmentDataType env;

    if (App_Sensor_Sht30Ok == 0u)
    {
        return;
    }

    if (SensorIf_Sht30Read(&sht) != 0u)
    {
        env.temperature_c_x100 = sht.temperature_c_x100;
        env.humidity_rh_x100 = sht.humidity_rh_x100;
        env.valid = 1u;
        (void)Rte_Write_Environment(&env);
        Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
    }
    else
    {
        env.temperature_c_x100 = 0;
        env.humidity_rh_x100 = 0;
        env.valid = 0u;
        (void)Rte_Write_Environment(&env);
        Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED, DEM_EVENT_STATUS_FAILED);
    }
}

void App_Sensor_MainFunction(uint32_t tick_ms)
{
    if (tick_ms >= App_Sensor_NextRtcMs)
    {
        App_Sensor_NextRtcMs = tick_ms + 1000u;
        App_Sensor_UpdateRtc();
    }

    if (tick_ms >= App_Sensor_NextShtMs)
    {
        App_Sensor_NextShtMs = tick_ms + 1000u;
        App_Sensor_UpdateSht30();
    }
}
#include "App_Sensor.h"

#include "Dem.h"
#include "RtcIf.h"
#include "Rte_Signal.h"
#include "SensorIf.h"

static uint8_t App_SensorRtcOk;
static uint8_t App_SensorShtOk;

void App_Sensor_Init(void)
{
    App_SensorRtcOk = RtcIf_Init();
    if (App_SensorRtcOk != 0u)
    {
        (void)RtcIf_StartOscillator();
        (void)Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
    }
    else
    {
        (void)Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_FAILED);
    }

    App_SensorShtOk = SensorIf_Sht30Init();
    if (App_SensorShtOk != 0u)
    {
        (void)Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
    }
    else
    {
        (void)Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED, DEM_EVENT_STATUS_FAILED);
    }
}

void App_Sensor_MainFunction(void)
{
    RtcIf_TimeType time;
    SensorIf_Sht30DataType sht30;
    int16_t temp_x10;
    uint16_t hum_x10;

    if (App_SensorRtcOk != 0u)
    {
        if ((RtcIf_ReadTime(&time) != 0u) && (RtcIf_IsTimeValid(&time) != 0u))
        {
            (void)Rte_Write_RtcTime(&time);
            (void)Dem_SetEventStatus(DEM_EVENT_RTC_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
            (void)Dem_SetEventStatus(DEM_EVENT_RTC_TIME_INVALID, DEM_EVENT_STATUS_PASSED);
        }
        else
        {
            (void)Dem_SetEventStatus(DEM_EVENT_RTC_TIME_INVALID, DEM_EVENT_STATUS_FAILED);
        }
    }

    if (App_SensorShtOk != 0u)
    {
        if (SensorIf_Sht30Read(&sht30) != 0u)
        {
            temp_x10 = (int16_t)(sht30.temperature_c_x100 / 10);
            hum_x10 = (uint16_t)(sht30.humidity_rh_x100 / 10);
            (void)Rte_Write_Sht30(temp_x10, hum_x10);
            (void)Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED, DEM_EVENT_STATUS_PASSED);
        }
        else
        {
            (void)Dem_SetEventStatus(DEM_EVENT_SHT30_COMM_FAILED, DEM_EVENT_STATUS_FAILED);
        }
    }
}
