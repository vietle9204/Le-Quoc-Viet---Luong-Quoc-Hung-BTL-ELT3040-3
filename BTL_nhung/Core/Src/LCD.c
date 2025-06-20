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
    // Enable GPIOB (bit 1) và I2C1 (bit 21)
    RCC->AHB1ENR |= (1 << 1);       // GPIOBEN
    RCC->APB1ENR |= (1 << 21);      // I2C1EN

    GPIOB->MODER &= ~((0x3 << (6 * 2)) | (0x3 << (7 * 2)));
    GPIOB->MODER |=  ((0x2 << (6 * 2)) | (0x2 << (7 * 2)));

    GPIOB->AFR[0] &= ~((0xF << (6 * 4)) | (0xF << (7 * 4)));
    GPIOB->AFR[0] |=  ((0x4 << (6 * 4)) | (0x4 << (7 * 4)));

    GPIOB->OTYPER |= (1 << 6) | (1 << 7);
    
    GPIOB->PUPDR &= ~((0x3 << (6 * 2)) | (0x3 << (7 * 2)));
    GPIOB->PUPDR |=  ((0x1 << (6 * 2)) | (0x1 << (7 * 2)));

    // Disable I2C1 before configuring
    I2C1->CR1 &= ~(1 << 0);   // PE = 0

    I2C1->CR2 = 40;           // PCLK1 = 40MHz
    I2C1->CCR = 200;          // CCR = Fpclk / (2 * F_SCL)
    I2C1->TRISE = 41;         // TRISE = (1000ns / T_PCLK) + 1

    // Enable I2C1
    I2C1->CR1 |= (1 << 0);    // PE = 1
}

void I2C1_Write(uint8_t addr, uint8_t *data, uint8_t len)
{
    I2C1->CR1 |= (1 << 8);                      // START
    while (!(I2C1->SR1 & (1 << 0)));            // Wait for SB

    I2C1->DR = addr & ~0x01;                    // Write mode (LSB = 0)
    while (!(I2C1->SR1 & (1 << 1)));            // Wait for ADDR
    (void)I2C1->SR2;                            // Clear ADDR

    for (int i = 0; i < len; i++) {
        while (!(I2C1->SR1 & (1 << 7)));        // Wait for TXE
        I2C1->DR = data[i];
    }

    while (!(I2C1->SR1 & (1 << 2)));            // Wait for BTF
    I2C1->CR1 |= (1 << 9);                      // STOP
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
        case 0: address = 0x80 + col; break;  // Line 1
        case 1: address = 0xC0 + col; break;  // Line 2
        default: return;
    }

    lcd_send_cmd(address);
}
