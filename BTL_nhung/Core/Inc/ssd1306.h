#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "main.h"

#define SSD1306_I2C_ADDR     0x78   // 0x3C << 1
#define SSD1306_WIDTH        128
#define SSD1306_HEIGHT       64
#define SSD1306_PAGES        (SSD1306_HEIGHT / 8)


/**
void init_I2C_oled(void);
void ssd1306_init();
void ssd1306_send_cmd(uint8_t cmd);
void ssd1306_send_data(uint8_t *data, uint16_t size);
void ssd1306_set_cursor(uint8_t x, uint8_t page);
void ssd1306_clear(void);
void ssd1306_draw_char(uint8_t x, uint8_t page, char c);
void ssd1306_draw_string(uint8_t x, uint8_t page, const char *str);
*/

void I2C1_Write(uint8_t addr, uint8_t *data, uint16_t size);
void ssd1306_send_cmd(uint8_t cmd);
void ssd1306_send_data(uint8_t *data, uint16_t size);
void ssd1306_init(void);
void ssd1306_goto(uint8_t x, uint8_t y);
void ssd1306_clear(void);
void ssd1306_put_char(char c);
void ssd1306_put_string(char *str);


#endif
