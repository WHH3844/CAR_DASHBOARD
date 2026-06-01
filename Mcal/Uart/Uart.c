#include "Uart.h"

#include "board_pins.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_usart.h"

void Uart_DebugInit(void)
{
    rcu_periph_clock_enable(DEBUG_UART_TX_GPIO_CLK);
    rcu_periph_clock_enable(DEBUG_UART_RX_GPIO_CLK);
    rcu_periph_clock_enable(DEBUG_UART_CLK);

    gpio_af_set(DEBUG_UART_TX_PORT, DEBUG_UART_GPIO_AF, DEBUG_UART_TX_PIN);
    gpio_af_set(DEBUG_UART_RX_PORT, DEBUG_UART_GPIO_AF, DEBUG_UART_RX_PIN);

    gpio_mode_set(DEBUG_UART_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_UART_TX_PIN);
    gpio_output_options_set(DEBUG_UART_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DEBUG_UART_TX_PIN);

    gpio_mode_set(DEBUG_UART_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, DEBUG_UART_RX_PIN);
    gpio_output_options_set(DEBUG_UART_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, DEBUG_UART_RX_PIN);

    usart_deinit(DEBUG_UART);
    usart_baudrate_set(DEBUG_UART, DEBUG_UART_BAUDRATE);
    usart_transmit_config(DEBUG_UART, USART_TRANSMIT_ENABLE);
    usart_receive_config(DEBUG_UART, USART_RECEIVE_ENABLE);
    usart_enable(DEBUG_UART);
}

void Uart_DebugPutc(char ch)
{
    usart_data_transmit(DEBUG_UART, (uint32_t)ch);

    while (RESET == usart_flag_get(DEBUG_UART, USART_FLAG_TBE))
    {
    }
}

void Uart_DebugPuts(const char *str)
{
    while (*str != '\0')
    {
        if (*str == '\n')
        {
            Uart_DebugPutc('\r');
        }

        Uart_DebugPutc(*str);
        str++;
    }
}

void Uart_DebugPutHex32(uint32_t value)
{
    uint32_t shift;
    uint8_t nibble;

    Uart_DebugPuts("0x");

    for (shift = 28u; shift <= 28u; shift -= 4u)
    {
        nibble = (uint8_t)((value >> shift) & 0x0Fu);
        Uart_DebugPutc((char)((nibble < 10u) ? ('0' + nibble) : ('A' + nibble - 10u)));

        if (shift == 0u)
        {
            break;
        }
    }
}

void Uart_DebugPutDec(uint32_t value)
{
    char buffer[11];
    uint32_t index;

    if (value == 0u)
    {
        Uart_DebugPutc('0');
        return;
    }

    index = 0u;
    while ((value != 0u) && (index < sizeof(buffer)))
    {
        buffer[index] = (char)('0' + (value % 10u));
        value /= 10u;
        index++;
    }

    while (index != 0u)
    {
        index--;
        Uart_DebugPutc(buffer[index]);
    }
}
