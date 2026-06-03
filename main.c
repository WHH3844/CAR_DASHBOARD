#include "EcuM.h"

int main(void)
{
    /*
     * main.c 只保留 ECU 级入口。
     * 具体初始化顺序、周期调度、关机保存都交给 EcuM，
     * 这样 APP 不会再变成一个巨大的测试 main。
     */
    EcuM_Init();
    EcuM_MainLoop();
}
