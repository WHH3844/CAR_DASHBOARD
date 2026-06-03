# 知识点梳理：汽车仪表盘项目 HR/技术答辩

## 一句话项目介绍

这是一个基于 GD32F470ZGT6 的汽车仪表盘学习板项目。我先完成 PCB 板级 bring-up，验证了电源、SDRAM、RGB LCD、CAN、I2C、EEPROM、RTC、SHT30、TF 卡、按键和蜂鸣器，然后把测试 Demo 迁移到 Mini AUTOSAR-like 分层架构中，实现了 `MCAL -> BSW -> RTE -> APP` 的主线数据流，并加入了基础 UDS 诊断、DTC 和 EEPROM 参数保存。

## 为什么不用完整 AUTOSAR

完整 AUTOSAR 需要 ARXML、配置工具、RTE 生成器、完整 BSW 栈和复杂工具链。这个项目是学习板，不是量产 ECU，所以采用轻量 AUTOSAR-like：

- 保留分层思想：APP 不直接碰硬件。
- 保留接口思想：信号通过 RTE 读写。
- 保留服务思想：诊断、存储、通信在 BSW。
- 不做形式主义：不引入完整 AUTOSAR 配置和生成体系。

面试回答可以说：我关注的是 AUTOSAR 的工程思想，而不是为了“看起来像”而堆复杂度。

## 分层架构怎么讲

```text
APP：仪表业务、显示、按键、电源、传感器
RTE：信号和服务接口，隔离 APP 与 BSW
BSW：CAN、诊断、故障、存储、LCD 抽象、电源抽象
MCAL：GD32 片上外设驱动，如 CAN/I2C/SDIO/EXMC/TLI/UART
Hardware：真实 PCB 和外设
```

核心数据流：

```text
CAN 0x321
  -> CanIf 接收标准帧
  -> PduR 按 ID 路由
  -> Com 解出车速/转速
  -> RTE 保存信号
  -> App_Dashboard 判断报警
  -> App_Display 刷新 LCD
```

## CAN 知识点

本项目第一版使用 Classic CAN：

- 标准帧 11-bit ID。
- 500K 波特率。
- DLC=8。
- Intel 小端格式。
- `0x321` 是动力总成输入。
- `0x325` 是仪表状态输出。
- `0x700/0x708` 是诊断物理请求/响应。
- `0x7DF` 是功能寻址请求。

能讲的重点：

- `CanIf` 不理解车速，只负责 CAN 帧收发。
- `Com` 才负责把字节转换成物理信号。
- ID 路由放在 `PduR`，这样诊断和普通信号不会混在一起。

## UDS 诊断知识点

已实现第一版教学级 UDS：

- `0x10 DiagnosticSessionControl`：切默认/扩展会话。
- `0x22 ReadDataByIdentifier`：读车速、转速、电压、启动次数、版本等 DID。
- `0x19 ReadDTCInformation`：读故障数量和故障码。
- `0x14 ClearDiagnosticInformation`：清故障。
- `0x3E TesterPresent`：保持诊断会话。

为什么只做单帧：

- 当前 DID 数据都尽量控制在 7 字节以内。
- 单帧足够验证 Dcm、Dem、NvM 和 CAN 诊断闭环。
- 多帧 ISO-TP 后续再做，避免第一版复杂度过高。

## Dem/DTC 怎么讲

Dem 是 Diagnostic Event Manager，负责把系统故障变成可诊断的 DTC。

本项目定义了 20 个 DTC，例如：

- SDRAM 初始化失败。
- LCD 初始化失败。
- CAN Bus-Off。
- CAN 接收超时。
- EEPROM 通信失败。
- EEPROM CRC 失败。
- RTC 通信失败。
- SHT30 通信失败。
- TF 卡挂载失败。
- 看门狗复位记录。

每个 DTC 当前保存：

- 当前状态。
- confirmed 位。
- warning indicator 位。
- 发生次数 occurrence counter。

面试可以说：我没有只做一个错误码数组，而是按 UDS/Dem 的思路做了状态掩码和 DTC 查询入口。

## NvM/EEPROM 怎么讲

NvM 是非易失性数据管理。底层 EEPROM 是 FT24C16A。

当前保存三个块：

- BootInfo：启动次数。
- SystemConfig：背光、蜂鸣器、主题。
- DemStatus：DTC 状态和发生次数。

每个块格式：

```text
magic + version + length + CRC16 + payload
```

为什么要 CRC：

- EEPROM 数据可能因为掉电、写入中断或干扰损坏。
- NvM 读取时先校验 CRC，不通过就加载默认值。
- 这是车载软件里常见的数据可靠性设计。

## LCD/SDRAM 怎么讲

LCD 是 800x480 RGB565 屏，TLI/RGB 输出像素，SDRAM 做 framebuffer。

关键经验：

- 一开始 LCD 花屏不是 CAN/I2C 冲突，而是 SDRAM 参数不适合 TLI 连续读 framebuffer。
- 修改为参考 Demo 的 SDRAM 参数后恢复正常。
- 后续显示刷新避免整屏高频擦写，采用固定框架一次绘制、动态区域局部刷新。

这能体现调试能力：不是“看到花屏就乱改 LCD”，而是从 TLI 数据流和 SDRAM 带宽角度定位。

## 电源管理怎么讲

硬件有船型开关强制上电和软开关自锁。

启动关键点：

- MCU 启动早期必须尽快拉高 `PWR_HOLD`。
- 如果 `PWR_HOLD` 太晚，软开关模式下可能刚启动就断电。

关机流程：

```text
长按 KEY_POWER
  -> App_Power 请求关机
  -> EcuM 进入 SLEEP_PREPARE
  -> 关闭蜂鸣器和背光
  -> 保存 Dem/NvM
  -> 拉低 PWR_HOLD
```

## FreeRTOS 移植怎么讲

当前已经把 FreeRTOS 加入工程，但第一版没有急着拆很多任务，而是先创建一个 `EcuM` 主任务承载原来的 10ms 周期调度。

这样做的原因：

- 前期板级 bring-up 用裸机更容易定位硬件问题，硬件跑通后再加入 RTOS 更稳。
- 第一版单任务能验证 FreeRTOS 移植、tick、中断向量、堆和任务栈，同时保持原有业务时序基本不变。
- LCD framebuffer、I2C、EEPROM、RTE 全局信号还没有加锁，直接拆多任务容易引入并发访问问题。
- 后续可以再拆成 CAN 任务、显示任务、传感器任务、NvM/诊断任务，并用队列、互斥量或事件组处理共享资源。

当前调度：

- FreeRTOS tick：1ms。
- `EcuM` 主任务：每 10ms 运行一次。
- 10ms：按键、CAN、Dashboard、诊断入口。
- 100ms：NvM/Dem 后台处理。
- 500ms：LCD 主界面刷新。
- 1000ms：RTC/SHT30、运行日志。

面试回答重点：

- `SVC_Handler` 用于启动第一个任务。
- `PendSV_Handler` 用于上下文切换。
- `SysTick_Handler` 提供 RTOS tick。
- 本项目使用 `heap_4.c` 做动态内存管理，适合有创建/释放需求且能合并空闲块的场景。
- 配置了 malloc failed hook 和 stack overflow hook，便于发现堆不足和任务栈溢出。
- 如果板上 RTOS 出问题，可以把 `APP_CFG_USE_FREERTOS` 改为 `0u` 回退裸机，对比判断是业务问题还是 RTOS 移植问题。

## HR 可能问：你遇到过什么难点

可以回答：

1. LCD 只亮背光不显示：后来确认最初参考的是 SPI 小屏 Demo，不适合 RGB 屏。
2. LCD 花屏：定位到 SDRAM 参数不适合 TLI framebuffer 连续读取。
3. CAN 联调：先用 PCAN 验证标准帧收发，再把临时 `0x100` Demo 迁移到正式 `0x321` 矩阵。
4. 软件结构：从测试大文件拆成 `CanIf/Com/RTE/App`，解决后续不可维护问题。
5. EEPROM 保存：加入 magic/version/length/CRC，避免直接裸写导致数据不可判断。

## HR 可能问：你负责了什么

可以回答：

- 完成 PCB 回板后的分阶段硬件测试。
- 调通 SDRAM、LCD、CAN、I2C、EEPROM、RTC、SHT30、TF 卡和按键蜂鸣器。
- 设计并实现 Mini AUTOSAR-like 软件分层。
- 实现 CAN 信号解析、RTE 信号接口、Dashboard APP。
- 实现基础 UDS 诊断、Dem 故障管理和 NvM EEPROM 保存。
- 移植 FreeRTOS，并用单 `EcuM` 主任务保持原 10ms 调度节拍。
- 编写测试步骤和调试记录，能用 PCAN/Keil/串口复现。
