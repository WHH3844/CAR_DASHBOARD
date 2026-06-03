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
    /*
     * RTC 和 SHT30 是独立外设：一个失败不阻塞另一个。
     * 初始化结果保存在本地标志位中，周期任务据此决定是否继续访问设备。
     */
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
        /*
         * I2C 通信成功不等于时间可信。
         * RtcIf_IsTimeValid() 会检查年月日时分秒范围，避免显示无意义时间。
         */
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
        /*
         * SensorIf 已经把 SHT30 原始值转换为 x100 定点数。
         * RTE 保留同样单位，显示层负责格式化为带两位小数的文本。
         */
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
        /* RTC 秒级刷新即可，过高频率会增加 I2C 总线占用但显示收益很小。 */
        App_Sensor_NextRtcMs = tick_ms + 1000u;
        App_Sensor_UpdateRtc();
    }

    if (tick_ms >= App_Sensor_NextShtMs)
    {
        /* SHT30 温湿度变化较慢，1s 周期兼顾响应速度和总线负载。 */
        App_Sensor_NextShtMs = tick_ms + 1000u;
        App_Sensor_UpdateSht30();
    }
}
