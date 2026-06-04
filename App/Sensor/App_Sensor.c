#include "App_Sensor.h"

#include "Dem.h"
#include "RtcIf.h"
#include "Rte_Signal.h"
#include "SensorIf.h"

static uint8_t App_Sensor_RtcOk;
static uint8_t App_Sensor_Sht30Ok;
static uint32_t App_Sensor_NextRtcMs;
static uint32_t App_Sensor_NextShtMs;

#define APP_SENSOR_DS3231_OSF                    0x80u
#define APP_SENSOR_RTC_BUILD_SYNC_MARGIN_SEC     300u

static uint8_t App_Sensor_Digit(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return (uint8_t)(ch - '0');
    }

    return 0u;
}

static uint8_t App_Sensor_IsLeapYear(uint16_t year)
{
    if ((year % 400u) == 0u)
    {
        return 1u;
    }
    if ((year % 100u) == 0u)
    {
        return 0u;
    }
    return ((year % 4u) == 0u) ? 1u : 0u;
}

static uint16_t App_Sensor_DaysBeforeMonth(uint16_t year, uint8_t month)
{
    static const uint16_t days_before_month[12] =
    {
        0u, 31u, 59u, 90u, 120u, 151u, 181u, 212u, 243u, 273u, 304u, 334u
    };
    uint16_t days;

    if (month < 1u)
    {
        month = 1u;
    }
    else if (month > 12u)
    {
        month = 12u;
    }

    days = days_before_month[month - 1u];
    if ((month > 2u) && (App_Sensor_IsLeapYear(year) != 0u))
    {
        days++;
    }

    return days;
}

static uint32_t App_Sensor_SecondOfYear(const RtcIf_TimeType *time)
{
    uint32_t days;

    days = (uint32_t)App_Sensor_DaysBeforeMonth(time->year, time->month);
    days += (uint32_t)time->date - 1u;

    return (days * 86400u) +
           ((uint32_t)time->hour * 3600u) +
           ((uint32_t)time->minute * 60u) +
           (uint32_t)time->second;
}

static uint8_t App_Sensor_IsBuildTimeNewer(const RtcIf_TimeType *rtc_time,
                                           const RtcIf_TimeType *build_time)
{
    uint32_t rtc_second;
    uint32_t build_second;

    if (build_time->year > rtc_time->year)
    {
        return 1u;
    }
    if (build_time->year < rtc_time->year)
    {
        return 0u;
    }

    rtc_second = App_Sensor_SecondOfYear(rtc_time);
    build_second = App_Sensor_SecondOfYear(build_time);
    return (build_second > (rtc_second + APP_SENSOR_RTC_BUILD_SYNC_MARGIN_SEC)) ? 1u : 0u;
}

static uint8_t App_Sensor_ParseBuildMonth(const char *month)
{
    if ((month[0] == 'J') && (month[1] == 'a')) { return 1u; }
    if ((month[0] == 'F') && (month[1] == 'e')) { return 2u; }
    if ((month[0] == 'M') && (month[1] == 'a') && (month[2] == 'r')) { return 3u; }
    if ((month[0] == 'A') && (month[1] == 'p')) { return 4u; }
    if ((month[0] == 'M') && (month[1] == 'a') && (month[2] == 'y')) { return 5u; }
    if ((month[0] == 'J') && (month[1] == 'u') && (month[2] == 'n')) { return 6u; }
    if ((month[0] == 'J') && (month[1] == 'u') && (month[2] == 'l')) { return 7u; }
    if ((month[0] == 'A') && (month[1] == 'u')) { return 8u; }
    if ((month[0] == 'S') && (month[1] == 'e')) { return 9u; }
    if ((month[0] == 'O') && (month[1] == 'c')) { return 10u; }
    if ((month[0] == 'N') && (month[1] == 'o')) { return 11u; }
    return 12u;
}

static uint8_t App_Sensor_CalcWeekday(uint16_t year, uint8_t month, uint8_t date)
{
    uint16_t y;
    uint8_t m;
    uint16_t k;
    uint16_t j;
    uint16_t h;

    y = year;
    m = month;
    if (m < 3u)
    {
        m = (uint8_t)(m + 12u);
        y--;
    }

    k = y % 100u;
    j = y / 100u;
    h = ((uint16_t)date + ((13u * ((uint16_t)m + 1u)) / 5u) + k + (k / 4u) + (j / 4u) + (5u * j)) % 7u;

    return (uint8_t)(((h + 5u) % 7u) + 1u);
}

static void App_Sensor_GetBuildTime(RtcIf_TimeType *time)
{
    const char *build_date;
    const char *build_clock;

    build_date = __DATE__;
    build_clock = __TIME__;

    time->month = App_Sensor_ParseBuildMonth(build_date);
    time->date = (uint8_t)((App_Sensor_Digit(build_date[4]) * 10u) + App_Sensor_Digit(build_date[5]));
    time->year = (uint16_t)((uint16_t)App_Sensor_Digit(build_date[7]) * 1000u);
    time->year += (uint16_t)((uint16_t)App_Sensor_Digit(build_date[8]) * 100u);
    time->year += (uint16_t)((uint16_t)App_Sensor_Digit(build_date[9]) * 10u);
    time->year += (uint16_t)App_Sensor_Digit(build_date[10]);
    time->hour = (uint8_t)((App_Sensor_Digit(build_clock[0]) * 10u) + App_Sensor_Digit(build_clock[1]));
    time->minute = (uint8_t)((App_Sensor_Digit(build_clock[3]) * 10u) + App_Sensor_Digit(build_clock[4]));
    time->second = (uint8_t)((App_Sensor_Digit(build_clock[6]) * 10u) + App_Sensor_Digit(build_clock[7]));
    time->weekday = App_Sensor_CalcWeekday(time->year, time->month, time->date);
}

static void App_Sensor_SyncRtcToBuildTime(void)
{
    RtcIf_TimeType rtc_time;
    RtcIf_TimeType build_time;
    uint8_t rtc_read_ok;
    uint8_t rtc_status;
    uint8_t need_set;

    App_Sensor_GetBuildTime(&build_time);
    rtc_read_ok = RtcIf_ReadTime(&rtc_time);
    need_set = 0u;

    if ((RtcIf_ReadStatus(&rtc_status) != 0u) && ((rtc_status & APP_SENSOR_DS3231_OSF) != 0u))
    {
        need_set = 1u;
    }
    if ((rtc_read_ok == 0u) || (RtcIf_IsTimeValid(&rtc_time) == 0u))
    {
        need_set = 1u;
    }
    else if (App_Sensor_IsBuildTimeNewer(&rtc_time, &build_time) != 0u)
    {
        need_set = 1u;
    }

    if ((need_set != 0u) && (RtcIf_SetTime(&build_time) != 0u))
    {
        (void)RtcIf_ClearOscStopFlag();
        (void)Rte_Write_RtcTime(&build_time, 1u);
    }
}

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
        App_Sensor_SyncRtcToBuildTime();
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
