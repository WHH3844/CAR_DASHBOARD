# CAR_DASHBOARD C 语言语法学习

这份笔记只围绕本工程真实用到的 C 语言写法整理，适合边看代码边学。推荐对照这些目录阅读：

- `main.c`
- `Config/*.h`
- `Bsw/Services/EcuM/EcuM.c`
- `Os/Os.c`
- `Rte/Rte_Signal.c`
- `Bsw/Communication/Com/Com.c`
- `Bsw/Services/Dcm/Dcm.c`
- `Bsw/Services/NvM/NvM.c`
- `App/*/*.c`

## 1. C 文件和头文件

本工程通常一个模块有一个 `.c` 和一个 `.h`：

```text
App/Display/App_Display.c
App/Display/App_Display.h
```

常见分工：

- `.h` 放类型定义、宏定义、函数声明。
- `.c` 放变量、静态函数、函数实现。
- 其它模块只 `#include` 头文件，不直接包含 `.c` 文件。

头文件保护：

```c
#ifndef ECUM_H
#define ECUM_H

void EcuM_Init(void);

#endif /* ECUM_H */
```

作用：防止同一个头文件被重复包含后出现重复定义。

## 2. `#include`

`#include` 用来引入其它头文件：

```c
#include "EcuM.h"
#include "App_Cfg.h"
#include <stdint.h>
```

区别：

- `"xxx.h"`：优先在工程 include 路径或当前模块路径查找，常用于项目自己的头文件。
- `<stdint.h>`：标准库头文件，常用于 `uint8_t`、`uint16_t`、`uint32_t` 等固定宽度整数类型。

## 3. 宏定义 `#define`

工程里大量配置放在 `Config/*.h`：

```c
#define APP_CFG_USE_FREERTOS         1u
#define APP_CFG_MAIN_LOOP_MS         10u
#define CAN_ID_EMS_POWERTRAIN_20MS   0x321u
```

要点：

- `1u` 的 `u` 表示 unsigned，无符号整数。
- `0x321u` 是十六进制无符号整数，常用于 CAN ID、寄存器位、magic 值。
- 宏没有类型检查，使用时要注意单位和取值范围。

本工程常见宏分类：

| 类型 | 示例 | 用途 |
|---|---|---|
| 功能开关 | `APP_CFG_USE_FREERTOS` | 编译期选择 RTOS 或裸机 |
| 周期配置 | `APP_CFG_DISPLAY_PERIOD_MS` | 控制任务调度周期 |
| CAN ID | `CAN_ID_ICM_STATUS_100MS` | 固化 CAN 矩阵 |
| 换算参数 | `COM_CFG_SPEED_RAW_TO_X10_NUM` | CAN 原始值转物理值 |
| NvM 地址 | `NVM_CFG_ADDR_BOOT_INFO` | EEPROM 块布局 |

## 4. 条件编译 `#if/#else/#endif`

本工程用条件编译在同一份代码里支持 FreeRTOS 和裸机模式：

```c
#if APP_CFG_USE_FREERTOS != 0u
    vTaskDelay(pdMS_TO_TICKS(ms));
#else
    Os_BusyDelayMs(ms);
#endif
```

特点：

- 条件编译发生在编译前，不满足条件的代码不会参与编译。
- 适合做平台选择、功能裁剪、调试回退。
- 不适合滥用，否则代码路径太多会难维护。

本工程当前关键开关：

```c
#define APP_CFG_USE_FREERTOS          1u
#define APP_CFG_FREERTOS_SPLIT_TASKS  1u
```

含义：

- `APP_CFG_USE_FREERTOS=1u`：启用 FreeRTOS。
- `APP_CFG_FREERTOS_SPLIT_TASKS=1u`：启用 CAN、应用、显示、传感器、NvM、日志、EcuM 生命周期拆分任务。

## 5. 固定宽度整数类型

嵌入式工程不常直接用 `int`，而是用固定宽度类型：

```c
uint8_t  level;
uint16_t rpm;
uint32_t tick_ms;
int16_t  coolant_temp_c;
```

含义：

| 类型 | 位宽 | 常见用途 |
|---|---:|---|
| `uint8_t` | 8 bit | CAN 字节、标志位、百分比 |
| `uint16_t` | 16 bit | rpm、ADC 值、EEPROM 地址 |
| `uint32_t` | 32 bit | tick、计数器、DTC |
| `int16_t` | 16 bit 有符号 | 温度、带正负的物理量 |

为什么这样写：

- MCU 寄存器、CAN 字节、EEPROM 地址都有明确位宽。
- 可移植性更好，不依赖编译器里 `int` 的具体大小。
- 做位操作和通信打包时更安全。

## 6. 函数定义和函数声明

函数声明通常放在 `.h`：

```c
void EcuM_Init(void);
void EcuM_MainFunction(void);
```

函数实现放在 `.c`：

```c
void EcuM_Init(void)
{
    Os_Init();
    LogM_Init();
}
```

函数格式：

```text
返回类型 函数名(参数列表)
{
    函数体
}
```

`void` 的两种含义：

- `void EcuM_Init(void)` 前面的 `void`：函数不返回值。
- 括号里的 `void`：函数没有参数。

## 7. `static` 的用法

### 7.1 静态全局变量

```c
static EcuM_StateType EcuM_State;
static uint32_t EcuM_TickMs;
```

作用：

- 变量只在当前 `.c` 文件可见。
- 其它模块不能直接访问，只能通过函数接口读取或修改。

这是嵌入式模块封装的核心写法。

### 7.2 静态函数

```c
static void EcuM_Shutdown(void)
{
    NvM_WriteAll();
    PowerIf_Shutdown();
}
```

作用：

- 函数只在当前 `.c` 文件内部使用。
- 不暴露给其它模块，减少接口污染。

### 7.3 函数内静态变量

```c
static uint32_t last_save_ms;
```

特点：

- 只初始化一次。
- 函数返回后值仍然保留。
- 常用于周期调度、缓存、上一次状态记录。

## 8. 枚举 `enum`

`EcuM_StateType` 用枚举表示系统状态：

```c
typedef enum
{
    ECUM_STATE_OFF = 0u,
    ECUM_STATE_BOOT,
    ECUM_STATE_SELF_TEST,
    ECUM_STATE_RUN,
    ECUM_STATE_SLEEP_PREPARE,
    ECUM_STATE_FAULT
} EcuM_StateType;
```

优点：

- 比直接写数字更清楚。
- 调试器里更容易看状态含义。
- 适合状态机、事件 ID、模式选择。

注意：

- 枚举底层通常是 `int`，但具体大小由编译器决定。
- 如果要精确控制通信字节，通常还是转成 `uint8_t`。

## 9. 结构体 `struct`

结构体用于把相关数据打包：

```c
typedef struct
{
    uint16_t vehicle_speed_kph_x10;
    uint16_t engine_rpm;
    uint8_t fuel_percent;
    uint8_t can_ems_valid;
} Rte_DashboardDataType;
```

使用方式：

```c
Rte_DashboardDataType data;
Rte_Read_DashboardData(&data);
```

访问成员：

```c
data.engine_rpm
data.can_ems_valid
```

如果变量是结构体指针，用 `->`：

```c
data->vehicle_speed_kph_x10
```

`.` 和 `->` 的区别：

| 写法 | 对象 |
|---|---|
| `data.member` | `data` 是结构体变量 |
| `ptr->member` | `ptr` 是结构体指针 |

## 10. `typedef`

`typedef` 给类型起别名：

```c
typedef enum { ... } EcuM_StateType;
typedef struct { ... } Rte_ConfigDataType;
```

好处：

- 代码更像工程接口，而不是到处写 `struct xxx`。
- 模块之间传递类型时更简洁。

本工程风格：

- 枚举类型以 `Type` 结尾。
- 结构体类型也以 `Type` 结尾。
- 模块名前缀放在类型名前，例如 `Rte_`、`NvM_`、`Dem_`。

## 11. 指针和地址

### 11.1 取地址 `&`

```c
Rte_DashboardDataType data;
Rte_Read_DashboardData(&data);
```

`&data` 表示把变量地址传给函数。函数可以通过这个地址把结果写回调用者的变量。

### 11.2 指针参数

```c
Std_ReturnType Rte_Read_DashboardData(Rte_DashboardDataType *data)
{
    if (data == 0)
    {
        return E_NOT_OK;
    }

    *data = Rte_DashboardData;
    return E_OK;
}
```

要点：

- `Rte_DashboardDataType *data` 表示 `data` 是指向结构体的指针。
- `data == 0` 是空指针检查。
- `*data = ...` 表示把内容写到指针指向的位置。

### 11.3 `const` 指针

```c
Std_ReturnType NvM_SetSystemConfig(const NvM_SystemConfigType *config)
```

含义：函数只读取 `config` 指向的数据，不应该修改它。

## 12. 数组

CAN 数据区常用数组：

```c
uint8_t frame[8];
```

访问下标：

```c
frame[0] = 0x04u;
frame[7] = counter;
```

注意：

- C 数组下标从 0 开始。
- `frame[8]` 的合法下标是 `0~7`。
- 越界访问不会自动报错，但会破坏内存，是嵌入式常见严重 bug。

结构体数组：

```c
static const Dem_EventConfigType Dem_EventConfigs[DEM_EVENT_COUNT] =
{
    {DEM_EVENT_LOW_SUPPLY_VOLTAGE, 0x010001u, 3u, 1u},
    {DEM_EVENT_CAN_RX_TIMEOUT,     0x040002u, 2u, 1u}
};
```

用途：建立事件 ID、DTC、等级、报警灯之间的配置表。

## 13. 字符串

字符串本质是以 `'\0'` 结尾的字符数组：

```c
LogM_Info("runtime heartbeat");
```

显示层也用字符串：

```c
LcdIf_DrawText(640u, 16u, "CAN OK", 3u, color);
```

注意：

- C 字符串必须以 `'\0'` 结束。
- 字符串字面量通常不可修改。
- 统计字符串长度时要遍历到 `'\0'`。

## 14. 控制语句

### 14.1 `if / else`

```c
if (level > 100u)
{
    level = 100u;
}
```

用途：范围钳位、状态判断、错误处理。

### 14.2 `switch / case`

```c
switch (pdu->id)
{
case CAN_ID_EMS_POWERTRAIN_20MS:
    Com_DecodePowertrain(pdu, tick_ms);
    break;

default:
    break;
}
```

用途：按 CAN ID、UDS SID、DID、状态等选择处理函数。

注意：

- 每个 `case` 后通常要写 `break`。
- 忘记 `break` 会继续执行下一个 `case`，除非这是有意设计。

### 14.3 `while`

```c
while (1)
{
    EcuM_MainFunction();
}
```

用途：

- 裸机 super loop。
- FreeRTOS 任务函数内的永久循环。
- 初始化失败后的停机调试点。

### 14.4 `for`

```c
for (index = 0u; index < 8u; index++)
{
    frame[index] = 0u;
}
```

用途：数组清零、拷贝 CAN 数据、遍历 DTC 表。

## 15. 位操作

嵌入式和通信代码里位操作非常常见。

### 15.1 与 `&`

```c
ignition = pdu->data[0] & 0x03u;
```

作用：取低 2 bit。

### 15.2 右移 `>>`

```c
gear = (pdu->data[0] >> 2u) & 0x0Fu;
```

作用：先把 bit2~bit5 移到低位，再用 `0x0F` 取出。

### 15.3 或 `|`

```c
frame[4] |= 0x02u;
```

作用：把某个 bit 置 1，不影响其它 bit。

### 15.4 取反 `~`

```c
status &= (uint8_t)(~DEM_STATUS_TEST_FAILED);
```

作用：清除某个位。

常见口诀：

- 置位：`value |= mask`
- 清位：`value &= ~mask`
- 判断：`if ((value & mask) != 0u)`
- 取字段：`(value >> shift) & mask`

## 16. 大端、小端和字节拼接

CAN 业务信号使用 little-endian：

```c
return (uint16_t)(data[0] | ((uint16_t)data[1] << 8u));
```

含义：

- `data[0]` 是低字节。
- `data[1]` 是高字节。
- 两个字节拼成 `uint16_t`。

写回 little-endian：

```c
data[0] = (uint8_t)(value & 0xFFu);
data[1] = (uint8_t)((value >> 8u) & 0xFFu);
```

UDS DID 请求通常用 big-endian：

```c
did = ((uint16_t)data[0] << 8u) | data[1];
```

学习重点：

- 通信协议必须明确字节序。
- 不要直接把结构体强转成字节数组发 CAN，因为结构体可能有 padding。

## 17. 强制类型转换

本工程常见转换：

```c
(uint8_t)(value & 0xFFu)
(uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u))
(void)tick_ms;
```

作用：

- 明确截断到目标位宽。
- 避免整数提升导致告警。
- `(void)tick_ms` 表示参数有意未使用，避免编译器 warning。

注意：

- 强转不是万能修复，可能隐藏溢出。
- 做乘法前通常先转到更宽类型，例如 `(uint32_t)speed_raw * 625u`。

## 18. `sizeof`

`sizeof` 获取类型或对象大小：

```c
sizeof(NvM_BootInfoType)
sizeof(payload)
sizeof(response)
```

用途：

- 写 NvM 块长度。
- 清数组或拷贝数组时避免手写长度。
- CAN 响应长度固定时可用 `sizeof(response)`。

注意：

- 对数组形参使用 `sizeof` 得到的是指针大小，不是原数组长度。
- 在函数内如果参数是 `uint8_t *data`，不能用 `sizeof(data)` 当数据长度。

## 19. 返回值和错误处理

工程里常用 `Std_ReturnType`：

```c
return E_OK;
return E_NOT_OK;
```

典型写法：

```c
if (NvM_ReadBlock(...) != E_OK)
{
    load_default();
}
```

优点：

- 调用方能判断是否成功。
- 不用异常机制，适合嵌入式 C。
- 错误路径清晰，方便打日志和上报 Dem。

## 20. 定点数

嵌入式里经常不用浮点，而用整数表达小数。

本工程例子：

| 信号 | 内部单位 | 示例 |
|---|---|---|
| 车速 | `0.1 km/h` | `800` 表示 `80.0 km/h` |
| 温湿度 | `x100` | `2575` 表示 `25.75 C` |
| 胎压 | `x100 bar` | `275` 表示 `2.75 bar` |
| 电压 | `mV` | `12000` 表示 `12.0 V` |

换算例子：

```c
speed_x10 = (uint16_t)(((uint32_t)speed_raw * 625u) / 1000u);
```

这样可以避免浮点库开销，也更适合 CAN 打包。

## 21. 时间差和无符号回绕

工程里常见写法：

```c
if ((tick_ms - last_tick_ms) >= 1000u)
{
    last_tick_ms = tick_ms;
}
```

为什么不直接写 `tick_ms >= last_tick_ms + 1000u`：

- `uint32_t` tick 最终会回绕。
- 无符号减法在 C 里按模运算，回绕后仍能得到正确的时间差语义。

这是嵌入式周期调度常用写法。

## 22. 临界区和 mutex

语法上只是函数调用，但背后是并发保护：

```c
Os_EnterCritical();
Rte_DashboardData = data;
Os_ExitCritical();
```

用途：

- 保护短时间内的全局变量读写。
- 防止多任务或中断打断一组内存操作。

I2C0 和 NvM 使用 mutex：

```c
if (Os_NvMLock() != 0u)
{
    NvM_MainFunction(tick_ms);
    Dem_MainFunction(tick_ms);
    Os_NvMUnlock();
}
```

区别：

- 临界区适合很短的内存操作。
- mutex 适合 I2C、EEPROM 这类可能等待的资源。

## 23. FreeRTOS 任务函数写法

任务函数原型：

```c
static void Os_CanTask(void *argument)
```

特点：

- 参数类型固定为 `void *`。
- 通常内部是 `while (1)`。
- 使用 `vTaskDelayUntil()` 保持周期。

典型结构：

```c
static void Task(void *argument)
{
    TickType_t last_wake;

    (void)argument;
    last_wake = xTaskGetTickCount();

    while (1)
    {
        DoWork();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10u));
    }
}
```

## 24. 编译器内置宏

`App_Sensor.c` 用到：

```c
__DATE__
__TIME__
```

含义：

- `__DATE__`：编译日期字符串，例如 `"Jun  5 2026"`。
- `__TIME__`：编译时间字符串，例如 `"14:30:00"`。

用途：RTC 时间无效时，用编译时间自动校准一个合理时间。

## 25. 嵌入式寄存器和外设库写法

MCAL 和 Firmware 层会出现寄存器、外设库、CMSIS 写法，例如：

```c
__disable_irq();
__enable_irq();
__NOP();
```

含义：

- `__disable_irq()`：关闭全局中断。
- `__enable_irq()`：打开全局中断。
- `__NOP()`：空操作指令，常用于粗略延时或等待。

学习时注意：

- APP 层不应该直接调用寄存器接口。
- 直接操作硬件的代码应收敛在 MCAL 或 ECU Abstraction。

## 26. 模块命名风格

本工程常见命名：

```text
模块_动作_对象
Rte_Write_Powertrain()
Rte_Read_DashboardData()
Com_DecodePowertrain()
App_Display_DrawSpeedGauge()
```

好处：

- 一眼看出函数属于哪个模块。
- 读调用链时容易判断层级。
- 避免不同模块函数重名。

## 27. 阅读代码建议顺序

推荐按调用链学习：

1. `main.c`：看工程入口。
2. `Bsw/Services/EcuM/EcuM.c`：看启动、初始化、调度、关机。
3. `Os/Os.c`：看 FreeRTOS 任务和裸机回退。
4. `Rte/Rte_Signal.c`：看全局信号怎么被读写。
5. `Bsw/Communication/Com/Com.c`：看 CAN 字节如何转成物理量。
6. `App/Dashboard/App_Dashboard.c`：看业务逻辑。
7. `App/Display/App_Display.c`：看显示刷新和缓存。
8. `Bsw/Services/Dcm/Dcm.c`：看 UDS 服务解析。
9. `Bsw/Services/Dem/Dem.c` 和 `Bsw/Services/NvM/NvM.c`：看故障和 EEPROM 保存。

## 28. 本工程最应该掌握的 C 语法清单

- `#include`、`#define`、`#if/#else/#endif`
- 头文件保护
- `uint8_t/uint16_t/uint32_t/int16_t`
- 函数声明和函数定义
- `static` 全局变量、`static` 函数、函数内 `static`
- `typedef enum`、`typedef struct`
- 指针、取地址 `&`、解引用 `*`、结构体指针 `->`
- 数组和下标
- 字符串和 `'\0'`
- `if/else`、`switch/case`、`for`、`while`
- 位操作：`&`、`|`、`~`、`<<`、`>>`
- 强制类型转换
- `sizeof`
- `const`
- 返回值错误处理
- 定点数换算
- 无符号 tick 时间差
- 临界区和 mutex 的使用习惯

把这些语法掌握后，再看本工程的 APP/RTE/BSW/MCAL 分层代码会顺很多。
