// usart_esp32.h
#ifndef USART_ESP32_H
#define USART_ESP32_H

#include "stm32f4xx.h"

// Khai báo các biến toàn cục
extern char receiver[20];
extern int receiver_index;

// Khai báo các hàm
void init_usart6(void);             // Khởi tạo USART6 (PA11, PA12)
void send_sys_state(char* msg);     // Gửi trạng thái hệ thống
void USART6_IRQHandler(void);       // Xử lý ngắt USART6

#endif // USART_ESP32_H
