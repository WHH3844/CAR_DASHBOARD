#ifndef COM_CFG_H
#define COM_CFG_H

/*
 * Com 层负责把 CAN 原始字节转换成 RTE 信号。
 * 这里集中放物理值换算参数，避免业务代码里散落“魔法数字”。
 */

/* 0x321 VehicleSpeed：Raw * 0.0625 km/h，RTE 内部保存为 0.1 km/h。 */
#define COM_CFG_SPEED_RAW_TO_X10_NUM        625u
#define COM_CFG_SPEED_RAW_TO_X10_DEN        1000u

/* 0x321 EngineSpeed：Raw * 0.25 rpm。 */
#define COM_CFG_RPM_RAW_TO_RPM_NUM          1u
#define COM_CFG_RPM_RAW_TO_RPM_DEN          4u

/* 0x321 FuelPercent：Raw * 0.4%。 */
#define COM_CFG_FUEL_RAW_TO_PERCENT_NUM     4u
#define COM_CFG_FUEL_RAW_TO_PERCENT_DEN     10u

/* 0x321 温度类信号：Raw - 40，单位摄氏度。 */
#define COM_CFG_TEMP_OFFSET_C               40

/* 0x321 电池电压暂按 0.1V/bit 转换，后续接真实 DBC 时只改这里。 */
#define COM_CFG_BATTERY_RAW_TO_MV           100u

/* v1.0 CAN 矩阵无效值。必须先判断无效值，再做物理量换算。 */
#define COM_CFG_SPEED_RAW_MASK              0x1FFFu
#define COM_CFG_SPEED_RAW_INVALID           0x1FFFu
#define COM_CFG_U16_RAW_INVALID             0xFFFFu
#define COM_CFG_U8_RAW_INVALID              0xFFu

/* TPMS 本机显示报警阈值；只对有效的轮位信号生效。 */
#define COM_CFG_TPMS_LOW_PRESSURE_X100      200u
#define COM_CFG_TPMS_HIGH_PRESSURE_X100     350u
#define COM_CFG_TPMS_HIGH_TEMPERATURE_C     85

#endif /* COM_CFG_H */
