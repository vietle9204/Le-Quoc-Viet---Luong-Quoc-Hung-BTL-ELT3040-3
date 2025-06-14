/*
 * usart_esp32.c
 *
 *  Created on: Apr 26, 2025
 *      Author: LEGION
 */


#include "stm32f4xx.h"
#include "string.h"


char receiver[20];
int receiver_index;

void init_usart6(void)         //PA11,PA12
{
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;       // cấp clock cho port A
	GPIOA->MODER &= ~(0xF << 11*2);
	GPIOA->MODER |= (2 << 11*2) | (2 << 12*2);     //chọn chế độ AF cho PA11, PA12
	GPIOA->AFR[1] |= 8 << (11-8)*4 | 8 << (12-8)*4; // chọn AF8

	RCC->APB2ENR |= RCC_APB2ENR_USART6EN;      //cấp clock cho usart6
	USART6->BRR = 43 << 4 | 6;                         //baudrate = 115200 , fck = 80MHz
	USART6->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;   // bật ngắt RX, bật TX, RX, usart

	//cấu hình ưu tiên ngắt
	//bật ngắt cho usart
}

void send_sys_state(char* msg)      //send sys_state, ppm, mode
{
	while(msg != NULL)
	{
		while(USART6->SR & USART_SR_TXE)
		{
			USART6->DR = *msg++;
		}
	}
}

void USART6_IRQHandler(void)
{
    if (USART6->SR & USART_SR_RXNE)  // Kiểm tra cờ RXNE
    {
        uint8_t data = (uint8_t)(USART6->DR);  // Đọc dữ liệu nhận được
        if(data != '\n')
        {
        	receiver[receiver_index] = data;
        	receiver_index++;
        }
        else
        {
        	*receiver = '\0';

        }


    }
}

