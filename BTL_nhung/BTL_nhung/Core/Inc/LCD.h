/*
 * LCD.h
 *
 *  Created on: Jun 14, 2025
 *      Author: LEGION
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "stm32f4xx.h"

// Địa chỉ I2C LCD
#define LCD_ADDR (0x27 << 1)

// Prototype
void delay_ms(uint32_t ms);
void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len);

void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_init(void);
void lcd_puts(char *str);
void lcd_gotoxy(uint8_t col, uint8_t row);

#endif /* INC_LCD_H_ */

