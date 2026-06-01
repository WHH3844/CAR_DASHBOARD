#include "PowerIf.h"
#include "board_pins.h"
#include "dashboard_demo_test.h"

int main(void)
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

    /* 当前阶段运行 12_dashboard_demo，进入整板联调小闭环。 */
    Test12_DashboardDemo_Run();

    while (1)
    {
        (void)PowerIf_LongPressShutdownTask(POWERIF_SHUTDOWN_LONG_PRESS_MS,
                                            POWERIF_SHUTDOWN_SAMPLE_MS);
    }
}
