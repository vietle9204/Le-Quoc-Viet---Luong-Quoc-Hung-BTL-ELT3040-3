<<<<<<< HEAD
/*
 * LCD.c
 *
 *  Created on: Jun 14, 2025
 *      Author: LEGION
 */

#include "stm32f4xx.h"

#define LCD_ADDR (0x27 << 1)

void delay_ms(uint32_t ms);
void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_init(void);
void lcd_puts(char *str);
void lcd_gotoxy(uint8_t col, uint8_t row);



void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 4000; i++) __NOP();
}

void I2C1_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
    GPIOB->MODER |= (2 << GPIO_MODER_MODE6_Pos) | (2 << GPIO_MODER_MODE7_Pos); // Alternate function

    GPIOB->AFR[0] |= (4 << GPIO_AFRL_AFSEL6_Pos) | (4 << GPIO_AFRL_AFSEL7_Pos); // AF4 for I2C1
    GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6_Msk | GPIO_PUPDR_PUPD7_Msk);
    GPIOB->PUPDR |= (1 << GPIO_PUPDR_PUPD6_Pos) | (1 << GPIO_PUPDR_PUPD7_Pos); // Pull-up

    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR2 = 40;            // APB1 = 42 MHz
    I2C1->CCR = 200;           // Chu kỳ = 42MHz / (2*CCR) = 100kHz
    I2C1->TRISE = 41;          // = (1000ns / T) + 1 = (1000ns / 23.81ns) + 1
    I2C1->CR1 |= I2C_CR1_PE;
}


void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len)
{
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr & ~0x01;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    for (int i = 0; i < len; i++) {
        while (!(I2C1->SR1 & I2C_SR1_TXE));
        I2C1->DR = data[i];
    }

    while (!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
}

void lcd_send_cmd(char cmd)
{
    char u = cmd & 0xF0;
    char l = (cmd << 4) & 0xF0;
    uint8_t data[4] = {
        u | 0x0C, u | 0x08,
        l | 0x0C, l | 0x08
    };
    I2C1_Write(LCD_ADDR, data, 4);
}

void lcd_send_data(char data_char)
{
    char u = data_char & 0xF0;
    char l = (data_char << 4) & 0xF0;
    uint8_t data[4] = {
        u | 0x0D, u | 0x09,
        l | 0x0D, l | 0x09
    };
    I2C1_Write(LCD_ADDR, data, 4);
}

void lcd_init(void)
{
    delay_ms(50);
    lcd_send_cmd(0x30); delay_ms(5);
    lcd_send_cmd(0x30); delay_ms(1);
    lcd_send_cmd(0x30); delay_ms(10);
    lcd_send_cmd(0x20); delay_ms(10);

    lcd_send_cmd(0x28); delay_ms(1);  // Function set
    lcd_send_cmd(0x08); delay_ms(1);  // Display off
    lcd_send_cmd(0x01); delay_ms(2);  // Clear
    lcd_send_cmd(0x06); delay_ms(1);  // Entry mode
    lcd_send_cmd(0x0C); delay_ms(1);  // Display on
}

void lcd_puts(char *str)
{
    while (*str) {
        lcd_send_data(*str++);
    }
}

void lcd_gotoxy(uint8_t col, uint8_t row)
{
    uint8_t address;

    switch (row)
    {
        case 0: address = 0x80 + col; break;  // Dòng 1
        case 1: address = 0xC0 + col; break;  // Dòng 2
case 2: address = 0x94 + col; break;  // Dòng 3
        case 3: address = 0xD4 + col; break;  // Dòng 4
        default: return;  // Sai row thì không gửi lệnh
    }

    lcd_send_cmd(address);  // Gửi lệnh set DDRAM address
}
=======
/*
 * LCD.c
 *
 *  Created on: Jun 14, 2025
 *      Author: LEGION
 */

#include "stm32f4xx.h"

#define LCD_ADDR (0x27 << 1)

void delay_ms(uint32_t ms);
void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_init(void);
void lcd_puts(char *str);
void lcd_gotoxy(uint8_t col, uint8_t row);



void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 4000; i++) __NOP();
}

void I2C1_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER &= ~(GPIO_MODER_MODE6_Msk | GPIO_MODER_MODE7_Msk);
    GPIOB->MODER |= (2 << GPIO_MODER_MODE6_Pos) | (2 << GPIO_MODER_MODE7_Pos); // Alternate function

    GPIOB->AFR[0] |= (4 << GPIO_AFRL_AFSEL6_Pos) | (4 << GPIO_AFRL_AFSEL7_Pos); // AF4 for I2C1
    GPIOB->OTYPER |= GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7;
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6_Msk | GPIO_PUPDR_PUPD7_Msk);
    GPIOB->PUPDR |= (1 << GPIO_PUPDR_PUPD6_Pos) | (1 << GPIO_PUPDR_PUPD7_Pos); // Pull-up

    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR2 = 40;            // APB1 = 42 MHz
    I2C1->CCR = 200;           // Chu kỳ = 42MHz / (2*CCR) = 100kHz
    I2C1->TRISE = 41;          // = (1000ns / T) + 1 = (1000ns / 23.81ns) + 1
    I2C1->CR1 |= I2C_CR1_PE;
}


void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len)
{
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr & ~0x01;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    for (int i = 0; i < len; i++) {
        while (!(I2C1->SR1 & I2C_SR1_TXE));
        I2C1->DR = data[i];
    }

    while (!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
}

void lcd_send_cmd(char cmd)
{
    char u = cmd & 0xF0;
    char l = (cmd << 4) & 0xF0;
    uint8_t data[4] = {
        u | 0x0C, u | 0x08,
        l | 0x0C, l | 0x08
    };
    I2C1_Write(LCD_ADDR, data, 4);
}

void lcd_send_data(char data_char)
{
    char u = data_char & 0xF0;
    char l = (data_char << 4) & 0xF0;
    uint8_t data[4] = {
        u | 0x0D, u | 0x09,
        l | 0x0D, l | 0x09
    };
    I2C1_Write(LCD_ADDR, data, 4);
}

void lcd_init(void)
{
    delay_ms(50);
    lcd_send_cmd(0x30); delay_ms(5);
    lcd_send_cmd(0x30); delay_ms(1);
    lcd_send_cmd(0x30); delay_ms(10);
    lcd_send_cmd(0x20); delay_ms(10);

    lcd_send_cmd(0x28); delay_ms(1);  // Function set
    lcd_send_cmd(0x08); delay_ms(1);  // Display off
    lcd_send_cmd(0x01); delay_ms(2);  // Clear
    lcd_send_cmd(0x06); delay_ms(1);  // Entry mode
    lcd_send_cmd(0x0C); delay_ms(1);  // Display on
}

void lcd_puts(char *str)
{
    while (*str) {
        lcd_send_data(*str++);
    }
}

void lcd_gotoxy(uint8_t col, uint8_t row)
{
    uint8_t address;

    switch (row)
    {
        case 0: address = 0x80 + col; break;  // Dòng 1
        case 1: address = 0xC0 + col; break;  // Dòng 2
case 2: address = 0x94 + col; break;  // Dòng 3
        case 3: address = 0xD4 + col; break;  // Dòng 4
        default: return;  // Sai row thì không gửi lệnh
    }

    lcd_send_cmd(address);  // Gửi lệnh set DDRAM address
}
>>>>>>> 0154b88dc1880da5581ef7c9717fefcf7b3f84b5
