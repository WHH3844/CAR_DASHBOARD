# CAR_DASHBOARD

基于 GD32F470ZGT6 / GD32F450ZGT6 的汽车仪表盘学习板工程。

本项目目标不是实现完整商用 AUTOSAR，而是基于 AUTOSAR Classic 的分层思想，搭建一个适合学习、调试、硬件验证和后续扩展的轻量化汽车仪表盘软件架构。

当前项目仍在开发中，软件工程已经建立基础目录和模块骨架，PCB 一版设计已经完成，后续重点是等待板子回来后按模块焊接、上电和验证。

## 项目定位

本项目用于学习和验证以下内容：

- GD32F470 / GD32F450 MCU 工程开发
- 汽车仪表盘硬件 bring-up
- CAN 通信与仪表数据解析
- RGB LCD 显示与 SDRAM framebuffer
- EEPROM / RTC / SHT30 / TF 卡等外设调试
- 基于 AUTOSAR Classic 思想的软件分层
- MCAL / BSW / RTE / APP 的职责划分
- 后续诊断、故障码、参数存储和日志功能扩展

项目当前更偏向教学型 ECU / 学习板，不适合直接用于量产车辆或安全相关场景。

## 当前状态

- 已创建 GD32F470ZGT6 汽车仪表盘工程模板
- 已建立 `App` / `Rte` / `Bsw` / `Mcal` / `Os` / `Config` / `Stub` / `Test` 目录
- 已加入 GD32F4xx CMSIS、标准外设库和 USB 库
- 已创建 Keil MDK 工程文件
- 已预留 12 个分阶段硬件测试程序目录
- `main.c` 当前仍为最小空循环，具体模块代码等待板子验证后逐步填充
- PCB 一版已经完成，等待打样、回板、焊接和分阶段测试

## 硬件概览

第一版硬件规划包含：

| 模块 | 器件 / 功能 |
|---|---|
| MCU | GD32F470ZGT6 / GD32F450ZGT6 |
| 外部存储 | W9825G6KH-6I SDRAM，32MB |
| 显示 | 4.3 寸 RGB565 LCD |
| CAN | SIT1043QT CAN FD 收发器 |
| 电源 | 12V 输入，MP9486A 转 5V / 3.3V |
| 上电控制 | 船型开关强制上电 + 软开关自锁 |
| RTC | DS3231 + CR1220 后备电池 |
| EEPROM | FT24C16A |
| 传感器 | SHT30 温湿度传感器 |
| 存储扩展 | TF / microSD 卡接口 |
| 人机接口 | 用户按键、蜂鸣器、状态指示灯 |
| 调试 | SWD + UART |

## 关键引脚规划

| 功能 | MCU 引脚 |
|---|---|
| KEY_POWER | PC13 |
| PWR_HOLD | PE3 |
| CAN1_TX | PB13 |
| CAN1_RX | PB12 |
| CAN1_EN | PB14 |
| CAN1_STBN | PB15 |
| CAN1_ERR_N | PG3 |
| I2C0_SCL | PB6 |
| I2C0_SDA | PB7 |
| SDIO_D0 | PC8 |
| SDIO_D1 | PC9 |
| SDIO_D2 | PC10 |
| SDIO_D3 | PC11 |
| SDIO_CLK | PC12 |
| SDIO_CMD | PD2 |
| KEY1 | PF6 |
| KEY2 | PF7 |
| KEY3 | PF8 |
| BUZZER_CTRL | PF9 |

## 软件架构

项目采用轻量 AUTOSAR-like 分层设计：

![CAR_DASHBOARD 软件架构图](Docs/software_architecture_imagegen.png)

```text
┌────────────────────────────────────┐
│ APP                                │
│ Dashboard / Display / Power / ...  │
├────────────────────────────────────┤
│ RTE                                │
│ Signal / Service / Event           │
├────────────────────────────────────┤
│ BSW                                │
│ Services / Communication / Memory  │
│ ECU Abstraction                    │
├────────────────────────────────────┤
│ MCAL                               │
│ Mcu / Port / Dio / Can / I2c / ... │
├────────────────────────────────────┤
│ Hardware                           │
│ GD32 + SDRAM + LCD + CAN + I2C     │
└────────────────────────────────────┘
```

核心原则：

- APP 不直接操作寄存器
- APP 不直接访问 CAN、EEPROM、LCD 等底层驱动
- 业务逻辑尽量通过 RTE 读写信号或调用服务
- MCAL 只负责 MCU 外设和硬件资源
- BSW 负责通信、存储、诊断、电源、抽象接口等基础服务
- 前期以裸机 super loop 为主，模块稳定后再考虑 FreeRTOS

## 目录结构

```text
CAR_DASHBOARD/
├── App/                 # 应用层：仪表、显示、电源、按键、传感器、日志等
├── Bsw/                 # 基础软件层
│   ├── Communication/   # CanIf、CanTp、Com、PduR、CanSM、CanTrcv、UartIf
│   ├── EcuAbstraction/  # LcdIf、PowerIf、RtcIf、SensorIf、BuzzerIf 等
│   ├── Memory/          # Eep、MemIf、NvM 相关存储抽象、SD/FATFS、SDRAM
│   └── Services/        # EcuM、BswM、Dem、Dcm、NvM、Crc、LogM、Det
├── Config/              # 模块配置头文件
├── Firmware/            # GD32F4xx CMSIS、标准外设库、USB 库
├── Mcal/                # MCU 抽象层驱动
├── Os/                  # 简单调度或后续 RTOS 适配
├── Project/             # Keil MDK 工程文件
├── Rte/                 # 运行时环境接口
├── Stub/                # 无硬件时的模拟输入/输出
├── Test/                # 分阶段硬件测试程序
├── main.c               # 当前最小入口
└── 进度.md              # 开发记录
```

## 测试程序规划

收到 PCB 后建议按以下顺序逐步焊接和验证，每个测试程序尽量保持独立可运行：

| 顺序 | 测试目录 | 目标 |
|---|---|---|
| 01 | `Test/01_power_hold_test` | 验证 KEY_POWER、PWR_HOLD、船型开关 BYPASS |
| 02 | `Test/02_uart_led_test` | 验证最小系统、时钟、UART、LED |
| 03 | `Test/03_sdram_test` | 验证 EXMC SDRAM 初始化和读写 |
| 04 | `Test/04_lcd_color_test` | 验证 RGB LCD、背光、纯色和色条显示 |
| 05 | `Test/05_can_loop_test` | 验证 CAN 收发器和 USB-CAN 通信 |
| 06 | `Test/06_i2c_scan_test` | 扫描 EEPROM、RTC、SHT30 等 I2C 设备 |
| 07 | `Test/07_eeprom_test` | 验证 EEPROM 读写和掉电保存 |
| 08 | `Test/08_rtc_test` | 验证 DS3231 走时和后备电池 |
| 09 | `Test/09_sht30_test` | 验证温湿度读取 |
| 10 | `Test/10_tf_card_test` | 验证 SDIO、FATFS、文件读写 |
| 11 | `Test/11_key_buzzer_test` | 验证按键、长按检测和蜂鸣器 |
| 12 | `Test/12_dashboard_demo` | 综合仪表 Demo：CAN 数据 + LCD 显示 + 报警 |

## 上电与焊接原则

PCB 回来后不要一次性焊满、一次性上电。推荐流程：

1. 裸板目检和电源网络阻值检查
2. 只焊输入保护与电源接口
3. 焊船型开关和软开关自锁电路
4. 焊 5V DCDC 并限流测试
5. 焊 3.3V DCDC 并限流测试
6. 焊 MCU 最小系统、SWD、UART
7. 焊 SDRAM 并单独测试
8. 焊 LCD 和背光并单独测试
9. 焊 CAN 模块并单独测试
10. 焊 EEPROM / RTC / SHT30 并进行 I2C 扫描
11. 焊 TF 卡、按键、蜂鸣器等扩展模块
12. 最后进行系统联调和长时间稳定性测试

首次上电必须使用可调电源限流，并优先确认 `12V_SYS`、`5V_SYS`、`3V3_SYS` 是否正常。

## 计划中的 CAN 信号

第一版 Demo 可先使用模拟 CAN 报文：

| CAN ID | 周期 | 内容 |
|---|---|---|
| `0x100` | 10ms | 车速、转速、油量、水温、报警标志 |
| `0x101` | 100ms | 电池电压、点火状态、电源模式、背光亮度 |
| `0x7E0` | 按需 | 诊断请求 |
| `0x7E8` | 按需 | 诊断响应 |

后续数据流目标：

```text
CAN 总线
  ↓
MCAL Can
  ↓
CanIf / Com
  ↓
RTE Signal
  ↓
App_Dashboard / App_Display
  ↓
LcdIf / BacklightIf / BuzzerIf
```

## 开发路线

### 阶段 A：硬件 Bring-up

- 电源、PWR_HOLD、UART、LED
- SDRAM
- LCD
- CAN
- I2C 外设

### 阶段 B：基础软件层

- MCAL 基础驱动
- CanIf / CanTrcv / Com 简化版
- LcdIf / PowerIf / RtcIf / SensorIf
- Eep / NvM 简化版
- RTE 信号接口

### 阶段 C：Dashboard Demo

- USB-CAN 模拟车速 / 转速
- LCD 显示车速 / 转速
- 超限蜂鸣器报警
- RTC 显示时间
- SHT30 显示温湿度
- EEPROM 保存配置

### 阶段 D：诊断与故障码

- Dem 故障事件记录
- Dcm 基础 UDS 服务
- CanTp 多帧传输
- 通过 USB-CAN 读取 DID / DTC

### 阶段 E：日志与 TF 卡

- TF 卡挂载 FATFS
- 运行日志和故障日志
- 长时间运行数据记录

## 当前构建方式

当前工程主要面向 Keil MDK：

```text
Project/CAR_DASHBOARD.uvprojx
```

由于大部分模块仍是骨架代码，当前工程更适合作为目录模板和后续 bring-up 的起点。等板子回来后，应优先补齐最小系统、UART、DIO、PWR_HOLD 等底层代码，再逐步接入其他模块。

## 注意事项

- `PWR_HOLD` 必须在启动早期尽快拉高，否则软开关模式下可能刚启动就断电
- LCD 调试前必须先确认 SDRAM 和背光
- CAN 调试前必须确认收发器 `EN` / `STBN` 状态
- I2C 调试建议先做总线扫描，再逐个验证 EEPROM、RTC、SHT30
- EEPROM 不要高频写入，后续应通过 NvM 做延迟写和 CRC 校验
- TF 卡和日志功能不影响第一阶段主功能，可放到后面
- 第一阶段不建议直接上 FreeRTOS，先用裸机循环验证硬件更容易定位问题

## 进度记录

开发进度请继续记录到：

```text
进度.md
```

每次新增模块、调通测试、发现硬件问题或确认第二版 PCB 修改点，都建议追加记录，方便后续复盘。
