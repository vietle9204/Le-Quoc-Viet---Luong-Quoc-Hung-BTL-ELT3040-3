/*
 * UART_ESP32.c
 *
 *  Created on: Jun 18, 2025
 *      Author: LEGION
 */


#include "stm32f4xx.h"
#include <string.h>
#include "stdio.h"
#include "UART_ESP32.h"
#include "main.h"


// Gửi 1 byte
void uart6_send_char(char c) {
	while (!(USART6->SR & USART_SR_TXE))
		;  // Đợi TXE = 1
	USART6->DR = c;
}

// Gửi chuỗi
void uart6_send_string(const char *s) {
	while (*s) {
		uart6_send_char(*s++);
	}
}


void uart6_init(void) {
	// 1. Enable clock GPIOA & USART6
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART6EN;

	// 2. Cấu hình PA11 và PA12 thành alternate function (AF8)
	GPIOA->MODER &= ~((3 << (11 * 2)) | (3 << (12 * 2)));   // clear
	GPIOA->MODER |= (2 << (11 * 2)) | (2 << (12 * 2));      // AF mode

	GPIOA->AFR[1] &= ~((0xF << ((11 - 8) * 4)) | (0xF << ((12 - 8) * 4))); // clear
	GPIOA->AFR[1] |= (8 << ((11 - 8) * 4)) | (8 << ((12 - 8) * 4));       // AF8

	// 3. Cấu hình USART6
	USART6->CR1 = 0; // Tắt USART trong khi cấu hình

	// Tính toán Baudrate (USARTDIV)
	// USARTDIV = Fck / (16 * BaudRate) = 80MHz / (16 * 115200) ≈ 43.4
	// Mantissa = 43, Fraction = 0.4 * 16 ≈ 6 → BRR = 43 << 4 | 6 = 0x02B6
	USART6->BRR = (43 << 4) | 6;

	// Enable RX interrupt, RX, TX, and USART
	USART6->CR1 |= USART_CR1_RXNEIE;             // bật ngắt
	USART6->CR1 |= USART_CR1_TE | USART_CR1_RE;  // Enable TX, RX
	USART6->CR1 |= USART_CR1_UE;                 // Enable USART

	// Enable NVIC interrupt for USART6
	NVIC_EnableIRQ(USART6_IRQn);
}

#define UART_BUFFER_SIZE 64
char uart_buf[UART_BUFFER_SIZE];
uint8_t uart_idx = 0;




void USART6_IRQHandler(void) {
    if (USART6->SR & USART_SR_RXNE) {
        char c = USART6->DR;

        if (c == '\n') {
            uart_buf[uart_idx] = '\0';
            handle_uart_message((char*)uart_buf);  // xử lý thông điệp
            uart_idx = 0;
        } else {
            if (uart_idx < UART_BUFFER_SIZE - 1) {
                uart_buf[uart_idx++] = c;
            } else {
                uart_idx = 0;  // Tràn bộ đệm, reset
            }
        }
    }
}
