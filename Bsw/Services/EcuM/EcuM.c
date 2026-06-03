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
    /*
     * 上电早期必须先保持电源自锁，否则用户松开电源键后整机可能掉电。
     * 不同硬件调试阶段可通过 POWERIF_WAIT_KEY_WHEN_NOT_PRESSED 选择是否等待按键。
     */
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

    /*
     * 等待用户松开电源键，给 App_Power 的运行期长按关机逻辑一个干净起点。
     * 超时后继续启动，避免按键异常导致系统永久卡在启动阶段。
     */
    (void)PowerIf_WaitKeyRelease(1500u);
}

static void EcuM_InitHardwarePath(void)
{
    /*
     * 初始化顺序来自前期 bring-up 经验：
     * 先保证 RTE/NvM/Dem 基础服务可用，再初始化 SDRAM 和 LCD，
     * 最后再启动 CAN、诊断和 APP。LCD framebuffer 必须等 SDRAM 成功后才能用。
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
    /*
     * 通信栈初始化顺序从上层服务到下层控制器都可以工作；
     * 这里先初始化 Dcm/CanTp/Com 的 RAM 状态，再启动 CanIf/CanSM 监控硬件。
     */
    Dcm_Init();
    CanTp_Init();
    Com_Init();
    (void)CanIf_Init();
    CanSM_Init();
}

static void EcuM_InitApps(void)
{
    /*
     * APP 初始化放在 BSW/RTE/硬件抽象之后。
     * 例如 App_Display 需要 LCD ready，App_Dashboard 需要 NvM 配置已经读入。
     */
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

#if APP_CFG_USE_FREERTOS != 0u
    LogM_Info("EcuM enter RUN, FreeRTOS enabled");
#else
    LogM_Info("EcuM enter RUN, bare loop enabled");
#endif
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
#if APP_CFG_USE_FREERTOS != 0u
    /*
     * FreeRTOS 模式下系统时间来自 RTOS tick。
     * 裸机模式下由 Os_Start() 在每次 busy-wait 后调用 EcuM_AdvanceTick()。
     */
    EcuM_TickMs = Os_GetTickMs();
#endif

    if (EcuM_State != ECUM_STATE_RUN)
    {
        /* 非 RUN 状态不调度业务，避免初始化失败后继续访问未就绪硬件。 */
        return;
    }

    /*
     * 先跑通信/诊断，再跑应用层。
     * 这样本周期刚收到的 CAN/UDS 数据可以尽快写入 RTE 并被 APP 使用。
     */
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
        /*
         * LCD 刷新成本明显高于普通逻辑任务，按较低频率调度。
         * RTE 中的数据仍然每 10ms 更新，显示只取最近快照。
         */
        EcuM_NextDisplayMs = EcuM_TickMs + APP_CFG_DISPLAY_PERIOD_MS;
        App_Display_MainFunction(EcuM_TickMs);
    }

    if (EcuM_TickMs >= EcuM_NextNvMMs)
    {
        /* NvM/Dem 放在同一个较慢周期，减少 EEPROM 访问和主循环抖动。 */
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
        /* 关机请求只由 EcuM 执行，保证保存和断电顺序集中在一个地方。 */
        App_Power_ClearShutdownRequest();
        EcuM_Shutdown();
    }
}

void EcuM_MainLoop(void)
{
    /*
     * EcuM 只负责 ECU 生命周期，真正“怎么调度”交给 Os。
     * APP_CFG_USE_FREERTOS=1 时 Os_Start() 会创建 FreeRTOS 任务；
     * APP_CFG_USE_FREERTOS=0 时 Os_Start() 会回退到裸机 while(1)。
     */
    Os_Start();
}

void EcuM_AdvanceTick(uint32_t elapsed_ms)
{
    EcuM_TickMs += elapsed_ms;
}

EcuM_StateType EcuM_GetState(void)
{
    return EcuM_State;
}

uint32_t EcuM_GetTickMs(void)
{
    return EcuM_TickMs;
}
