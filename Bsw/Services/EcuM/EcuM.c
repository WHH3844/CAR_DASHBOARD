#include "EcuM.h"

#include "App_Cfg.h"
#include "App_Dashboard.h"
#include "App_Diag.h"
#include "App_Display.h"
#include "App_Key.h"
#include "App_Logger.h"
#include "App_Power.h"
#include "App_Sensor.h"
#include "BacklightIf.h"
#include "BuzzerIf.h"
#include "CanIf.h"
#include "CanSM.h"
#include "CanTp.h"
#include "Com.h"
#include "Dcm.h"
#include "Dem.h"
#include "LcdIf.h"
#include "LogM.h"
#include "NvM.h"
#include "Os.h"
#include "PowerIf.h"
#include "Rte.h"
#include "SdramIf.h"
#include "board_pins.h"

static EcuM_StateType EcuM_State;
static uint32_t EcuM_TickMs;
static uint32_t EcuM_NextDisplayMs;
static uint32_t EcuM_NextNvMMs;
static uint32_t EcuM_NextLoggerMs;

static void EcuM_BootPowerHold(void)
{
#if POWERIF_WAIT_KEY_WHEN_NOT_PRESSED
    if (PowerIf_BootCheckAndHold() == 0u)
    {
        if (PowerIf_WaitKeyPressAndHold(0u) == 0u)
        {
            while (1)
            {
            }
        }
    }
#else
    (void)PowerIf_BootCheckAndHold();
#endif

    (void)PowerIf_WaitKeyRelease(1500u);
}

static void EcuM_InitHardwarePath(void)
{
    /*
     * 初始化顺序来自 README/开发文档：
     * 先保证电源保持和日志，再初始化 EEPROM/NvM/Dem，
     * 然后 SDRAM -> LCD，最后通信与 APP。
     */
    Rte_Init();
    NvM_Init();
    Dem_Init();

    if (SdramIf_Init() != E_OK)
    {
        EcuM_State = ECUM_STATE_FAULT;
        return;
    }

    if (LcdIf_Init(SdramIf_GetFrameBuffer()) != E_OK)
    {
        EcuM_State = ECUM_STATE_FAULT;
        return;
    }

    BacklightIf_Init();
    BuzzerIf_Init();
    Dcm_Init();
    CanTp_Init();
    Com_Init();
    (void)CanIf_Init();
    CanSM_Init();
}

static void EcuM_InitApps(void)
{
    App_Power_Init();
    App_Key_Init();
    App_Dashboard_Init();
    App_Display_Init();
    App_Sensor_Init();
    App_Diag_Init();
    App_Logger_Init();
}

void EcuM_Init(void)
{
    EcuM_State = ECUM_STATE_BOOT;
    EcuM_TickMs = 0u;
    EcuM_NextDisplayMs = 0u;
    EcuM_NextNvMMs = 0u;
    EcuM_NextLoggerMs = 0u;

    EcuM_BootPowerHold();
    LogM_Init();
    LogM_Info("CAR_DASHBOARD architecture main start");

    EcuM_State = ECUM_STATE_SELF_TEST;
    EcuM_InitHardwarePath();
    if (EcuM_State == ECUM_STATE_FAULT)
    {
        LogM_Error("EcuM self test failed");
        return;
    }

    EcuM_InitApps();
    EcuM_State = ECUM_STATE_RUN;
    LogM_Info("EcuM enter RUN");
}

static void EcuM_Shutdown(void)
{
    EcuM_State = ECUM_STATE_SLEEP_PREPARE;
    LogM_Info("shutdown requested, saving NvM and powering off");

    Rte_Call_Buzzer_Set(0u);
    Rte_Call_Backlight_Set(0u);
    Dem_SaveNow();
    NvM_WriteAll();

    PowerIf_Shutdown();
}

void EcuM_MainFunction(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        return;
    }

    CanIf_MainFunction(EcuM_TickMs);
    CanSM_MainFunction(EcuM_TickMs);
    Com_MainFunction(EcuM_TickMs);
    Dcm_MainFunction(EcuM_TickMs);

    App_Power_MainFunction(EcuM_TickMs);
    App_Key_MainFunction(EcuM_TickMs);
    App_Dashboard_MainFunction(EcuM_TickMs);
    App_Sensor_MainFunction(EcuM_TickMs);
    App_Diag_MainFunction();

    if (EcuM_TickMs >= EcuM_NextDisplayMs)
    {
        EcuM_NextDisplayMs = EcuM_TickMs + APP_CFG_DISPLAY_PERIOD_MS;
        App_Display_MainFunction(EcuM_TickMs);
    }

    if (EcuM_TickMs >= EcuM_NextNvMMs)
    {
        EcuM_NextNvMMs = EcuM_TickMs + APP_CFG_NVM_PERIOD_MS;
        NvM_MainFunction(EcuM_TickMs);
        Dem_MainFunction(EcuM_TickMs);
    }

    if (EcuM_TickMs >= EcuM_NextLoggerMs)
    {
        EcuM_NextLoggerMs = EcuM_TickMs + 1000u;
        App_Logger_MainFunction(EcuM_TickMs);
    }

    if (App_Power_IsShutdownRequested() != 0u)
    {
        App_Power_ClearShutdownRequest();
        EcuM_Shutdown();
    }
}

void EcuM_MainLoop(void)
{
    while (1)
    {
        EcuM_MainFunction();
        Os_DelayMs(APP_CFG_MAIN_LOOP_MS);
        EcuM_TickMs += APP_CFG_MAIN_LOOP_MS;
    }
}

EcuM_StateType EcuM_GetState(void)
{
    return EcuM_State;
}

uint32_t EcuM_GetTickMs(void)
{
    return EcuM_TickMs;
}
#include "EcuM.h"

#include "App_Cfg.h"
#include "App_Dashboard.h"
#include "App_Diag.h"
#include "App_Display.h"
#include "App_Key.h"
#include "App_Logger.h"
#include "App_Power.h"
#include "App_Sensor.h"
#include "BswM.h"
#include "CanIf.h"
#include "CanSM.h"
#include "CanTp.h"
#include "Com.h"
#include "Dem.h"
#include "Det.h"
#include "FatFsIf.h"
#include "LogM.h"
#include "NvM.h"
#include "PduR.h"
#include "PowerIf.h"
#include "Rte.h"
#include "Rte_Signal.h"
#include "SdramIf.h"
#include "board_pins.h"

static EcuM_StateType EcuM_State;
static uint16_t EcuM_Timer100Ms;
static uint16_t EcuM_Timer500Ms;
static uint16_t EcuM_Timer1000Ms;

static void EcuM_DelayMs(uint32_t ms)
{
    uint32_t i;

    while (ms-- != 0u)
    {
        for (i = 0u; i < 20000u; i++)
        {
            __NOP();
        }
    }
}

static void EcuM_ApplyNvMConfig(void)
{
    const NvM_SystemConfigType *config;

    config = NvM_GetSystemConfig();
    (void)Rte_Write_BacklightLevel(config->backlight_level);
    (void)Rte_Write_BuzzerEnable(config->buzzer_enable);
    (void)Rte_Write_ConfigTheme(config->theme);
}

void EcuM_Init(void)
{
    EcuM_State = ECUM_STATE_BOOT;

#if POWERIF_WAIT_KEY_WHEN_NOT_PRESSED
    if (PowerIf_BootCheckAndHold() == 0u)
    {
        if (PowerIf_WaitKeyPressAndHold(0u) == 0u)
        {
            while (1)
            {
            }
        }
    }
#else
    (void)PowerIf_BootCheckAndHold();
    PowerIf_HoldOn();
#endif

    (void)PowerIf_WaitKeyRelease(1500u);

    LogM_Init();
    LogM_Info("CAR_DASHBOARD EcuM boot");

    Det_Init();
    Dem_Init();
    Rte_Init();
    NvM_Init();
    EcuM_ApplyNvMConfig();

    BswM_Init();
    PduR_Init();
    CanTp_Init();
    Com_Init();
    CanSM_Init();
    FatFsIf_Init();

    EcuM_State = ECUM_STATE_SELF_TEST;
    if (SdramIf_Init() != E_OK)
    {
        EcuM_State = ECUM_STATE_FAULT;
        LogM_Error("SDRAM init failed");
        return;
    }

    App_Display_Init();
    App_Key_Init();
    App_Power_Init();
    App_Dashboard_Init();
    App_Diag_Init();
    App_Sensor_Init();
    App_Logger_Init();

    (void)CanIf_Init();

    EcuM_Timer100Ms = 0u;
    EcuM_Timer500Ms = 0u;
    EcuM_Timer1000Ms = 0u;
    EcuM_State = ECUM_STATE_RUN;
    LogM_Info("EcuM entered RUN");
}

void EcuM_RunSystem10ms(void)
{
    Rte_MainFunction(APP_CFG_MAIN_LOOP_MS);
    App_Power_MainFunction(APP_CFG_MAIN_LOOP_MS);
    App_Diag_MainFunction(APP_CFG_MAIN_LOOP_MS);
}

void EcuM_RunCan10ms(void)
{
    CanIf_MainFunctionRx();
    CanIf_MainFunctionBusOff();
    CanSM_MainFunction();
    Com_MainFunction(APP_CFG_MAIN_LOOP_MS);
}

void EcuM_RunApp10ms(void)
{
    App_Key_MainFunction(APP_CFG_MAIN_LOOP_MS);
    App_Dashboard_MainFunction(APP_CFG_MAIN_LOOP_MS);
}

void EcuM_RunDisplay500ms(void)
{
    App_Display_MainFunction();
}

void EcuM_RunDiagNvM100ms(void)
{
    NvM_MainFunction(100u);
    BswM_MainFunction();
}

void EcuM_MainFunction(void)
{
    if (EcuM_State != ECUM_STATE_RUN)
    {
        EcuM_DelayMs(APP_CFG_MAIN_LOOP_MS);
        return;
    }

    EcuM_RunSystem10ms();
    EcuM_RunCan10ms();
    EcuM_RunApp10ms();

    EcuM_Timer100Ms = (uint16_t)(EcuM_Timer100Ms + APP_CFG_MAIN_LOOP_MS);
    EcuM_Timer500Ms = (uint16_t)(EcuM_Timer500Ms + APP_CFG_MAIN_LOOP_MS);
    EcuM_Timer1000Ms = (uint16_t)(EcuM_Timer1000Ms + APP_CFG_MAIN_LOOP_MS);

    if (EcuM_Timer100Ms >= 100u)
    {
        EcuM_Timer100Ms = 0u;
        EcuM_RunDiagNvM100ms();
    }

    if (EcuM_Timer500Ms >= APP_CFG_DISPLAY_PERIOD_MS)
    {
        EcuM_Timer500Ms = 0u;
        EcuM_RunDisplay500ms();
    }

    if (EcuM_Timer1000Ms >= APP_CFG_SENSOR_PERIOD_MS)
    {
        EcuM_Timer1000Ms = 0u;
        App_Sensor_MainFunction();
        App_Logger_MainFunction(1000u);
    }

    EcuM_DelayMs(APP_CFG_MAIN_LOOP_MS);
}

EcuM_StateType EcuM_GetState(void)
{
    return EcuM_State;
}
