/************************************************************************/
/* XMEGA Template Board                                                 */
/*                                                                      */
/* board.h                                                              */
/*                                                                      */
/* Alex Forencich <alex@alexforencich.com>                              */
/*                                                                      */
/* Copyright (c) 2011 Alex Forencich                                    */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation       */
/* files(the "Software"), to deal in the Software without restriction,  */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies or substantial portions of the Software.      */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#ifndef __BOARD_H
#define __BOARD_H

#include <avr/io.h>

// token pasting
#define token_paste2_int(x, y) x ## y
#define token_paste2(x, y) token_paste2_int(x, y)
#define token_paste3_int(x, y, z) x ## y ## z
#define token_paste3(x, y, z) token_paste3_int(x, y, z)

// IO pins
// LEDs
#define LED_PORT                PORTB
#define LED_USR_0_PIN           0
#define LED_USR_1_PIN           1
#define LED_USR_2_PIN           3
#define LED_USR_0_PIN_bm        (1 << LED_USR_0_PIN)
#define LED_USR_1_PIN_bm        (1 << LED_USR_1_PIN)
#define LED_USR_2_PIN_bm        (1 << LED_USR_2_PIN)

// Buttons
#define BTN_PORT                PORTF
#define BTN_PIN                 0
#define BTN_PIN_bm              (1 << BTN_PIN)

// SPI Bus
#define SPI_CS_PORT             PORTD
#define SPI_CS_DEV_PIN          0
#define SPI_CS_DEV_PIN_bm       (1 << SPI_CS_DEV_PIN)

#define SPI_CS_DEV_assert()     SPI_CS_PORT.OUTCLR = SPI_CS_DEV_PIN_bm
#define SPI_CS_DEV_deassert()   SPI_CS_PORT.OUTSET = SPI_CS_DEV_PIN_bm

#define SPI_DEV                 SPIC

// UART
#define UART_BAUD_RATE          115200
#define UART_PORT_NAME          F
#define UART_NUMBER             0
#define UART_TX_PIN             3
#define UART_PORT               token_paste2(PORT, UART_PORT_NAME)
#define UART_DEVICE_PORT        token_paste2(UART_PORT_NAME, UART_NUMBER)
#define UART_DEVICE             token_paste2(USART, UART_DEVICE_PORT)

#define USART_N0_DEVICE_PORT    C0
#define USART_N1_DEVICE_PORT    C1
#define USART_N2_DEVICE_PORT    D0
#define USART_N3_DEVICE_PORT    D1
#define USART_N4_DEVICE_PORT    E0
#define USART_N5_DEVICE_PORT    E1

#define NODE_BAUD_RATE          115200

// I2C
#define I2C_DEV                 TWIE

#define I2C_slave_vect          TWIE_TWIS_vect
#define I2C_master_vect         TWIE_TWIM_vect


#endif // __BOARD_H

