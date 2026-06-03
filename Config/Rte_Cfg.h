#ifndef RTE_CFG_H
#define RTE_CFG_H

/*
 * RTE 是 APP 与 BSW 的边界。这里放默认值和信号超时策略。
 */
#define RTE_CFG_DEFAULT_BACKLIGHT_LEVEL     100u
#define RTE_CFG_DEFAULT_BUZZER_ENABLE       1u
#define RTE_CFG_SIGNAL_AGE_SATURATION_MS    60000u

#endif /* RTE_CFG_H */
