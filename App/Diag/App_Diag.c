#include "App_Diag.h"

void App_Diag_Init(void)
{
    /*
     * 预留应用层诊断初始化。
     * 当前可诊断数据直接来自 RTE/NvM/Dem；后续如果增加应用自检缓存、
     * 运行统计 DID 或诊断 IO 控制，可以在这里初始化对应上下文。
     */
}

void App_Diag_MainFunction(void)
{
    /*
     * 预留周期入口。
     * 保持空实现是有意的：EcuM 可以稳定调度该模块，后续扩展不需要修改
     * 生命周期框架或新增调度点。
     */
}
