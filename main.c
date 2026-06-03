#include "EcuM.h"

int main(void)
{
    /*
     * main.c 只保留系统入口：
     * - EcuM_Init()：完成上电自锁、BSW/RTE/APP 初始化和自检
     * - EcuM_MainLoop()：把运行调度交给 Os，进入裸机 super loop 或 FreeRTOS
     */
    EcuM_Init();
    EcuM_MainLoop();

    /*
     * 正常情况下 EcuM_MainLoop() 不会返回：
     * FreeRTOS 模式进入调度器，裸机模式进入 super loop。
     * 保留死循环用于异常返回时把 CPU 留在可调试位置。
     */
    while (1)
    {
    }
}
