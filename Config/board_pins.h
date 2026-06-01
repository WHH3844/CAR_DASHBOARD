#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "gd32f4xx.h"

/* 电源按键和自锁保持引脚。
 * KEY_POWER 低电平表示按下，PWR_HOLD 高电平保持供电。
 */
#define KEY_POWER_GPIO_CLK        RCU_GPIOC
#define KEY_POWER_PORT            GPIOC
#define KEY_POWER_PIN             GPIO_PIN_13

#define PWR_HOLD_GPIO_CLK         RCU_GPIOE
#define PWR_HOLD_PORT             GPIOE
#define PWR_HOLD_PIN              GPIO_PIN_3

/* 1：如果 MCU 不是由 KEY_POWER 按下启动，则一直等待按键再自锁。
 * 0：不等待按键，直接继续运行，适合 SW1/BYPASS 强制上电调试。
 */
#define POWERIF_WAIT_KEY_WHEN_NOT_PRESSED    1u
#define POWERIF_SHUTDOWN_LONG_PRESS_MS       1500u
#define POWERIF_SHUTDOWN_SAMPLE_MS           10u

/* RGB 状态灯。
 * XL-3528RGBW-HM 公共端接 3V3_SYS，因此三路颜色都是低电平点亮：
 * GPIO 输出低电平 = 点亮，GPIO 输出高电平 = 熄灭。
 */
#define STATUS_LED_R_GPIO_CLK      RCU_GPIOG
#define STATUS_LED_R_PORT          GPIOG
#define STATUS_LED_R_PIN           GPIO_PIN_9

#define STATUS_LED_G_GPIO_CLK      RCU_GPIOD
#define STATUS_LED_G_PORT          GPIOD
#define STATUS_LED_G_PIN           GPIO_PIN_6

#define STATUS_LED_B_GPIO_CLK      RCU_GPIOD
#define STATUS_LED_B_PORT          GPIOD
#define STATUS_LED_B_PIN           GPIO_PIN_7

/* LCD FPC 控制脚。
 * SPI3_NSS/SCK/MISO/MOSI 分别连接 PE4/PE2/PE5/PE6；
 * LCD_RST 为 PD12，LCD_BLK 为 PD13，背光默认按高电平点亮处理。
 */
#define LCD_SPI_GPIO_CLK           RCU_GPIOE
#define LCD_SPI_SCK_PORT           GPIOE
#define LCD_SPI_SCK_PIN            GPIO_PIN_2
#define LCD_SPI_NSS_PORT           GPIOE
#define LCD_SPI_NSS_PIN            GPIO_PIN_4
#define LCD_SPI_MISO_PORT          GPIOE
#define LCD_SPI_MISO_PIN           GPIO_PIN_5
#define LCD_SPI_MOSI_PORT          GPIOE
#define LCD_SPI_MOSI_PIN           GPIO_PIN_6

#define LCD_RST_GPIO_CLK           RCU_GPIOD
#define LCD_RST_PORT               GPIOD
#define LCD_RST_PIN                GPIO_PIN_12

#define LCD_BLK_GPIO_CLK           RCU_GPIOD
#define LCD_BLK_PORT               GPIOD
#define LCD_BLK_PIN                GPIO_PIN_13

/* 调试串口默认使用 USART0：TX PA9，RX PA10。
 * 如果原理图使用了其他串口，只需要修改下面这些宏。
 */
/* CAN1 接 SIT1043QT 收发器。
 * PB13/PB12 为 CAN1_TX/CAN1_RX，PB14 使能收发器，PB15 退出待机。
 * CAN1_ERR_N 为低有效错误指示，外部已经有上拉，软件按普通输入读取。
 */
#define CAN1_GPIO_CLK              RCU_GPIOB
#define CAN1_TX_PORT               GPIOB
#define CAN1_TX_PIN                GPIO_PIN_13
#define CAN1_RX_PORT               GPIOB
#define CAN1_RX_PIN                GPIO_PIN_12
#define CAN1_GPIO_AF               GPIO_AF_9

#define CAN1_CTRL_GPIO_CLK         RCU_GPIOB
#define CAN1_EN_PORT               GPIOB
#define CAN1_EN_PIN                GPIO_PIN_14
#define CAN1_STB_N_PORT            GPIOB
#define CAN1_STB_N_PIN             GPIO_PIN_15

#define CAN1_ERR_N_GPIO_CLK        RCU_GPIOG
#define CAN1_ERR_N_PORT            GPIOG
#define CAN1_ERR_N_PIN             GPIO_PIN_3

#define CAN1_TEST_BAUDRATE         500000u

/* I2C0 总线。
 * PB6/PB7 连接 EEPROM、RTC、SHT30，也可能连接 LCD 触摸控制器。
 * I2C 引脚使用开漏输出，依赖板上的上拉电阻保持空闲高电平。
 */
#define I2C0_BUS                    I2C0
#define I2C0_BUS_CLK                RCU_I2C0
#define I2C0_GPIO_CLK               RCU_GPIOB
#define I2C0_SCL_PORT               GPIOB
#define I2C0_SCL_PIN                GPIO_PIN_6
#define I2C0_SDA_PORT               GPIOB
#define I2C0_SDA_PIN                GPIO_PIN_7
#define I2C0_GPIO_AF                GPIO_AF_4
#define I2C0_BUS_SPEED              100000u

/* TF / microSD 卡 SDIO 接口。
 * PC8~PC11 为 DAT0~DAT3，PC12 为 CLK，PD2 为 CMD。
 * 当前原理图未接卡检测脚，测试程序默认卡已经插入。
 */
#define TF_SDIO_BUS                 SDIO
#define TF_SDIO_CLK                 RCU_SDIO
#define TF_SDIO_D_GPIO_CLK          RCU_GPIOC
#define TF_SDIO_CMD_GPIO_CLK        RCU_GPIOD
#define TF_SDIO_D_PORT              GPIOC
#define TF_SDIO_D0_PIN              GPIO_PIN_8
#define TF_SDIO_D1_PIN              GPIO_PIN_9
#define TF_SDIO_D2_PIN              GPIO_PIN_10
#define TF_SDIO_D3_PIN              GPIO_PIN_11
#define TF_SDIO_CLK_PORT            GPIOC
#define TF_SDIO_CLK_PIN             GPIO_PIN_12
#define TF_SDIO_CMD_PORT            GPIOD
#define TF_SDIO_CMD_PIN             GPIO_PIN_2
#define TF_SDIO_GPIO_AF             GPIO_AF_12

/* 用户按键与蜂鸣器。
 * KEY1/KEY2/KEY3 外部 10k 上拉到 3V3_SYS，按下接 GND，低电平有效。
 * BUZZER_CTRL 通过 S8050 驱动 5V 有源蜂鸣器，高电平响、低电平停。
 */
#define USER_KEY_GPIO_CLK           RCU_GPIOF
#define USER_KEY1_PORT              GPIOF
#define USER_KEY1_PIN               GPIO_PIN_6
#define USER_KEY2_PORT              GPIOF
#define USER_KEY2_PIN               GPIO_PIN_7
#define USER_KEY3_PORT              GPIOF
#define USER_KEY3_PIN               GPIO_PIN_8

#define BUZZER_GPIO_CLK             RCU_GPIOF
#define BUZZER_PORT                 GPIOF
#define BUZZER_PIN                  GPIO_PIN_9

#define DEBUG_UART                 USART0
#define DEBUG_UART_CLK             RCU_USART0
#define DEBUG_UART_TX_GPIO_CLK     RCU_GPIOA
#define DEBUG_UART_RX_GPIO_CLK     RCU_GPIOA
#define DEBUG_UART_TX_PORT         GPIOA
#define DEBUG_UART_RX_PORT         GPIOA
#define DEBUG_UART_TX_PIN          GPIO_PIN_9
#define DEBUG_UART_RX_PIN          GPIO_PIN_10
#define DEBUG_UART_GPIO_AF         GPIO_AF_7
#define DEBUG_UART_BAUDRATE        115200u

#endif /* BOARD_PINS_H */
