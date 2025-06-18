/*
 * UART_ESP32.h
 *
 *  Created on: Jun 18, 2025
 *      Author: LEGION
 */
#include "main.h"
#ifndef INC_UART_ESP32_H_
#define INC_UART_ESP32_H_

void uart6_send_char(char c);
// Gửi chuỗi
void uart6_send_string(const char *s);

void uart6_init(void);

void handle_uart_message(char *msg);

#endif /* INC_UART_ESP32_H_ */
