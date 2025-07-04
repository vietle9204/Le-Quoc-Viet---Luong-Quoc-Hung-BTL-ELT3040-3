#include "stm32f4xx.h"
#include <stdint.h>
#include <string.h>
#include "ssd1306.h"
#include "font.h"



/**
void I2C1_Write(uint8_t addr, uint8_t *data, uint16_t size) {
    while (I2C1->SR2 & I2C_SR2_BUSY);                       // Chờ I2C sẵn sàng
    I2C1->CR1 |= I2C_CR1_START;                             // Start condition
    while (!(I2C1->SR1 & I2C_SR1_SB));                      // Chờ start

    (void)I2C1->SR1;
    I2C1->DR = addr;                                        // Gửi địa chỉ
    while (!(I2C1->SR1 & I2C_SR1_ADDR));                    // Chờ ACK
    (void)I2C1->SR2;

    for (uint16_t i = 0; i < size; i++) {
        while (!(I2C1->SR1 & I2C_SR1_TXE));
        I2C1->DR = data[i];
    }

    while (!(I2C1->SR1 & I2C_SR1_BTF));                     // Chờ truyền xong
    I2C1->CR1 |= I2C_CR1_STOP;                              // Stop condition
}

void I2C1_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER |= (0x02 << (6 * 2)) | (0x02 << (7 * 2));  // PB6, PB7 = AF
    GPIOB->AFR[0] |= (4 << (6 * 4)) | (4 << (7 * 4));       // AF4 = I2C
    GPIOB->OTYPER |= (1 << 6) | (1 << 7);                   // Open-drain
    GPIOB->OSPEEDR |= (3 << (6 * 2)) | (3 << (7 * 2));      // High speed
    GPIOB->PUPDR |= (1 << (6 * 2)) | (1 << (7 * 2));        // Pull-up

    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR2 = 40;                  // APB1 = 40MHz
    I2C1->CCR = 200;                 // 100kHz
    I2C1->TRISE = 41;
    I2C1->CR1 |= I2C_CR1_PE;
}

// ===== SSD1306 LOW-LEVEL =====
void ssd1306_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = { 0x00, cmd };  // Control byte = 0x00
    I2C1_Write(SSD1306_I2C_ADDR, buf, 2);
}

void ssd1306_send_data(uint8_t *data, uint16_t size) {
    uint8_t buf[129];
    buf[0] = 0x40;                   // Control byte = 0x40 (data)
    memcpy(&buf[1], data, size);
    I2C1_Write(SSD1306_I2C_ADDR, buf, size + 1);
}

// ===== SSD1306 HIGH-LEVEL =====
void ssd1306_init(void) {
    I2C1_Init();

    const uint8_t init_cmds[] = {
        0xAE, 0x20, 0x00, 0xB0,
        0xC8, 0x00, 0x10, 0x40,
        0x81, 0x7F, 0xA1, 0xA6,
        0xA8, 0x3F, 0xA4, 0xD3,
        0x00, 0xD5, 0x80, 0xD9,
        0xF1, 0xDA, 0x12, 0xDB,
        0x40, 0x8D, 0x14, 0xAF
    };

    for (uint8_t i = 0; i < sizeof(init_cmds); i++) {
        ssd1306_send_cmd(init_cmds[i]);
    }
}

void ssd1306_goto(uint8_t x, uint8_t y) {
    ssd1306_send_cmd(0xB0 + y);                     // Page (y)
    ssd1306_send_cmd(0x00 + (x & 0x0F));            // Lower col
    ssd1306_send_cmd(0x10 + ((x >> 4) & 0x0F));     // Upper col
}

void ssd1306_clear(void) {
    uint8_t zero[SSD1306_WIDTH] = {0};
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        ssd1306_goto(0, page);
        ssd1306_send_data(zero, SSD1306_WIDTH);
    }
}

void ssd1306_put_char(char c) {
    if (c < 32 || c > 126) c = '?';
    uint8_t index = c - 32;
    uint8_t data[6];
    memcpy(data, font5x8[index], 5);
    data[5] = 0x00;  // Space between characters
    ssd1306_send_data(data, 6);
}

void ssd1306_put_string(char *str) {
    uint8_t buf[6 * 128]; // buffer lưu font cho tối đa 128 ký tự, mỗi ký tự 6 bytes
    uint16_t len = 0;

    while (*str && len < sizeof(buf) / 6) {
        char c = *str++;
        if (c < 32 || c > 126) c = '?';
        uint8_t index = c - 32;
        memcpy(&buf[len * 6], font5x8[index], 5);
        buf[len * 6 + 5] = 0x00;  // khoảng cách giữa ký tự
        len++;
    }

    if (len > 0) {
        ssd1306_send_data(buf, len * 6);
    }
}
*/
