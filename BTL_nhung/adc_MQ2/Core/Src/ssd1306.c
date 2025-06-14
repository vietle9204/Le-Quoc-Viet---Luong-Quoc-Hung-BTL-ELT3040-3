#include "ssd1306.h"




#include "stm32f4xx.h"
/**
void init_I2C_oled(void) {
    // 1. Bật clock GPIOB và I2C1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;    // Bật clock cho GPIOB
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;     // Bật clock cho I2C1

    // 2. Cấu hình Alternate Function cho PB6 và PB7 (AF4)
    GPIOB->MODER &= ~((3 << (6 * 2)) | (3 << (7 * 2)));  // Clear bits
    GPIOB->MODER |= (2 << (6 * 2)) | (2 << (7 * 2));     // Chế độ AF

    GPIOB->OTYPER |= (1 << 6) | (1 << 7);                // Open-drain

    GPIOB->OSPEEDR |= (3 << (6 * 2)) | (3 << (7 * 2));   // High speed

    GPIOB->PUPDR &= ~((3 << (6 * 2)) | (3 << (7 * 2)));  // No pull-up/down

    GPIOB->AFR[0] |= (4 << (6 * 4)) | (4 << (7 * 4));    // AF4 cho PB6, PB7

    // 3. Reset I2C1 trước khi cấu hình
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    // 4. Cấu hình tần số đồng hồ I2C (input clock = 40MHz)
    I2C1->CR2 = 40;     // Tần số APB1 = 40MHz

    // 5. Cấu hình clock I2C ở chế độ chuẩn (100kHz)
    // Tclock = 40MHz / (2 * CCR) => CCR = 200
    I2C1->CCR = 200;
    I2C1->TRISE = 41;   // = (42MHz/1MHz) + 1 = 43

    // 6. Enable I2C1
    I2C1->CR1 |= I2C_CR1_PE;
}


void I2C1_Write(uint8_t addr, uint8_t *data, uint16_t size) {
    // 1. Start condition
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));               // Đợi SB = 1

    // 2. Gửi địa chỉ (viết)
    (void) I2C1->SR1;
    I2C1->DR = addr << 1;                            // Gửi địa chỉ + Write
    while (!(I2C1->SR1 & I2C_SR1_ADDR));             // Đợi địa chỉ được ACK
    (void) I2C1->SR1; (void) I2C1->SR2;              // Đọc SR1 và SR2 để clear

    // 3. Gửi từng byte
    for (uint16_t i = 0; i < size; i++) {
        while (!(I2C1->SR1 & I2C_SR1_TXE));          // Đợi TXE
        I2C1->DR = data[i];
    }

    // 4. Đợi truyền xong
    while (!(I2C1->SR1 & I2C_SR1_BTF));              // Đợi Byte Transfer Finished

    // 5. Stop condition
    I2C1->CR1 |= I2C_CR1_STOP;
}

void ssd1306_send_cmd(uint8_t cmd) {
    uint8_t buffer[2] = { 0x00, cmd };               // 0x00: control byte cho command
    I2C1_Write(SSD1306_I2C_ADDR, buffer, 2);
}


void ssd1306_send_data(uint8_t *data, uint16_t size) {
    uint8_t buf[size + 1];
    buf[0] = 0x40;                                   // 0x40: control byte cho data
    for (uint16_t i = 0; i < size; i++)
        buf[i + 1] = data[i];

    I2C1_Write(SSD1306_I2C_ADDR, buf, size + 1);
}


void ssd1306_init() {
	HAL_Delay(100);
	ssd1306_send_cmd(0xAE); // Tắt màn hình

	ssd1306_send_cmd(0x20); // Set Memory Addressing Mode
	ssd1306_send_cmd(0x00); // Horizontal addressing mode

	ssd1306_send_cmd(0xB0); // Page start address
	ssd1306_send_cmd(0xC8); // COM Output Scan Direction

	ssd1306_send_cmd(0x00); // Low column address
	ssd1306_send_cmd(0x10); // High column address

	ssd1306_send_cmd(0x40); // Start line address

	ssd1306_send_cmd(0x81); // Contrast
	ssd1306_send_cmd(0x7F);

	ssd1306_send_cmd(0xA1); // Segment re-map
	ssd1306_send_cmd(0xA6); // Normal display

	ssd1306_send_cmd(0xA8); // Multiplex ratio
	ssd1306_send_cmd(0x3F);

	ssd1306_send_cmd(0xA4); // Display entire on
	ssd1306_send_cmd(0xD3); // Display offset
	ssd1306_send_cmd(0x00);

	ssd1306_send_cmd(0xD5); // Display clock divide ratio
	ssd1306_send_cmd(0x80);

	ssd1306_send_cmd(0xD9); // Pre-charge period
	ssd1306_send_cmd(0xF1);

	ssd1306_send_cmd(0xDA); // COM pins hardware config
	ssd1306_send_cmd(0x12);

	ssd1306_send_cmd(0xDB); // VCOMH deselect
	ssd1306_send_cmd(0x40);

	ssd1306_send_cmd(0x8D); // Charge pump
	ssd1306_send_cmd(0x14);

	ssd1306_send_cmd(0xAF); // Bật màn hình
}

void ssd1306_clear(void) {
	uint8_t zero[128] = { 0 };
	for (uint8_t page = 0; page < 8; page++) {
		ssd1306_set_cursor(0, page);
		ssd1306_send_data(zero, 128);
	}
}

void ssd1306_set_cursor(uint8_t x, uint8_t page) {
	ssd1306_send_cmd(0xB0 | page); // page
	ssd1306_send_cmd(0x00 | (x & 0x0F)); // lower column
	ssd1306_send_cmd(0x10 | (x >> 4)); // higher column
}

void ssd1306_draw_char(uint8_t x, uint8_t page, char c) {
	if (c < ' ' || c > '~')
		return;
	ssd1306_set_cursor(x, page);
	ssd1306_send_data((uint8_t*) font5x8[c - ' '], 5);
}

void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str) {
	while (*str) {
		ssd1306_draw_char(x, page, *str++);
		x += 6;
	}
}

*/
