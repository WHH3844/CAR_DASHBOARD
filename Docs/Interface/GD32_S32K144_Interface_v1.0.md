# GD32F470 / S32K144 CAN / UDS 接口协议 v1.0

状态：Released  
发布日期：2026-07-29  
接口版本字节：`0x10`

## 1. 适用范围

本协议定义 GD32F470 仪表域（ICM）与 S32K144 控制域（CDM）之间的 CAN 接口，以及 ICM 的 UDS 诊断接口。信号级细节以同目录两份 v1.0 Excel 为准。

## 2. 通用约定

- CAN：标准 11-bit ID，DLC=8，500 kbit/s。
- 普通 CAN 多字节信号：Intel/Little-Endian。
- UDS 多字节数值：Big-Endian。
- 周期超时通常为 3 倍周期；`0x321` 为 100 ms 关键超时。
- 无效值必须在物理量换算之前识别。
- 接口版本不兼容时不得继续按当前布局解析。

## 3. 报文清单

| ID | 报文 | 方向 | 周期/类型 | Timeout |
|---|---|---|---|---|
| 0x321 | PowertrainStatus | CDM/仿真 -> ICM | 20 ms | 100 ms |
| 0x322 | BodyStatus | CDM/仿真 -> ICM | 100 ms | 500 ms |
| 0x323 | TpmsStatus | CDM/仿真 -> ICM | 1000 ms | 3000 ms |
| 0x324 | DisplayConfig | CDM/仿真 -> ICM | 1000 ms | 3000 ms |
| 0x325 | IcmStatus | ICM -> CDM | 100 ms | 500 ms |
| 0x326 | IcmDiagnosticSummary | ICM -> CDM | 1000 ms | 3000 ms |
| 0x327 | IcmKeyEvent | ICM -> CDM | Event | 1000 ms |
| 0x328 | IcmLogStatus | ICM -> CDM | 1000 ms | 3000 ms |
| 0x329 | CdmStatus | CDM -> ICM | 500 ms | 1500 ms |
| 0x440 | IcmNm | ICM -> CDM | 1000 ms | 3000 ms |
| 0x441 | CdmNm | CDM -> ICM | 1000 ms | 3000 ms |
| 0x700 | UDS Physical Request | Tester -> ICM | Event | N/A |
| 0x708 | UDS Physical Response | ICM -> Tester | Event | N/A |
| 0x7DF | UDS Functional Request | Tester -> ICM | Event | N/A |

## 4. 0x329 CdmStatus

| Byte | 定义 |
|---|---|
| B0 | InterfaceVersion=`0x10` |
| B1 | InputMode[1:0]、PowerMode[4:2]、Health[6:5]、RemoteFaultPresent[7] |
| B2 | RemoteDtcCount；`0xFF` 无效 |
| B3 | CAN1_OK、CAN2_OK、LIN1_OK、LIN2_OK、ADC_VALID、KeyOverride、SelfTestPassed、Reserved |
| B4..5 | LastRemoteFaultId，LE；`0xFFFF` 无效 |
| B6 | AliveCounter，uint8 自然回卷 |
| B7 | CRC8/J1850，覆盖 `29 03` + B0..B6 |

CRC 参数：poly=0x1D，init=0xFF，refin=false，refout=false，xorout=0xFF。

## 5. 0x325 IcmStatus

| Byte/Bit | 信号 | 无效/说明 |
|---|---|---|
| B0.0..2 | ICM_Mode | 0=Init,1=Normal,2=SleepPrep,3=Sleep,4=Fault |
| B0.3..5 | DisplayPage | 0=Boot,1=Main,2=Diag,3=Settings |
| B1.0..1 | BuzzerStatus | 0=Off,1=Active,2=Muted |
| B2 | BacklightActual | 0..100，`0xFF` 无效 |
| B3..4 | TripDistance | LE，0.1 km；无可靠源时 `0xFFFF` |
| B5..6 | DriveTime | LE，分钟；饱和到 `0xFFFF` |
| B7.0 | ShutdownRequest | 1=请求关机 |
| B7.1 | FaultLampRequest | 1=请求故障灯 |

## 6. 0x326 IcmDiagnosticSummary

| Byte/Bit | 信号 |
|---|---|
| B0 | DtcCount，0..20 |
| B1..2 | LastFaultId，LE；`0xFFFF` 表示无 |
| B3..4 | PowerOnCount，LE，低 16 位 |
| B5.0..4 | SDRAM/LCD/RTC/EEPROM/TF 自检通过 |
| B5.5 | CAN online |
| B6 | CanBusOffCounter，饱和到 255 |
| B7.0..3 | ResetReason 低 4 bit |
| B7.4..5 | DiagnosticSession |
| B7.6 | NvMDirty |
| B7.7 | SelfTestPassed |

## 7. NM

- ICM 节点地址：`0x40`；CDM 节点地址：`0x41`。
- `0x440/0x441` 只表达网络管理状态。
- 应用健康必须使用 `0x329`，不得仅根据 NM 在线推断所有传感器和子总线正常。

## 8. UDS

### 地址

- Physical request: `0x700`
- Physical response: `0x708`
- Functional request: `0x7DF`

### v1.0 支持

- `0x10` DiagnosticSessionControl：仅物理。
- `0x22` ReadDataByIdentifier：物理和功能只读。
- `0x19` ReadDTCInformation 子功能 01/02：物理和功能只读，受单帧长度限制。
- `0x14` ClearDiagnosticInformation：仅物理。
- `0x3E` TesterPresent：物理和功能。

### DID

| DID | 名称 |
|---|---|
| 0xF191 | HardwareVersion |
| 0xF193 | SoftwareVersion |
| 0xF200 | BootCounter |
| 0xF201 | VehicleSpeed |
| 0xF202 | EngineSpeed |
| 0xF203 | BatteryVoltage |
| 0xF204 | RtcTime |
| 0xF240 | SdramTestResult |

### 未支持

`0x11/0x27/0x2E/0x2F/0x31/0x85` 和 ISO-TP 多帧在 v1.0 中为 Planned。调用时返回标准 NRC，不得执行隐式功能。

## 9. Dem 与 NvM

- 本地最多 20 个 Dem 事件。
- 失败确认采用事件配置阈值；Confirmed 保留到物理 `0x14` 清除。
- `0x326` 只发送本地 DTC 摘要；S32 远端故障只来自 `0x329`。
- 当前持久块：BootInfo@0x700、SystemConfig@0x720、DemStatus@0x740。
- Snapshot 和 ExtendedData 尚未持久化，矩阵状态为 Planned。

## 10. 降级行为

- `0x321` 超时：动力显示无效，仪表本地功能保持。
- `0x329` 超时/CRC 错/版本错：控制域应用状态无效或降级。
- `0x441` 超时：控制域 NM 离线。
- 控制域离线不能阻断本地 LCD、按键、RTC、SHT30、UDS 和已保存配置。

## 11. RevisionRecords

| 版本 | 日期 | 内容 |
|---|---|---|
| v1.0 | 2026-07-29 | CAN/UDS/Dem/NvM 首次正式审计定版；新增 0x329；DID 命名空间、UDS BE 和权限规则定版 |
