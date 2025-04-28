#include "stm32f4xx_hal.h"
#include <stdio.h> // cho sprintf
#include <string.h> // cho memset

extern I2C_HandleTypeDef hi2c1; // Bạn nhớ đã có cấu hình I2C1
extern const uint8_t font5x8[][5]; // Bảng font chữ 5x8

#define OLED_I2C_ADDR (0x3C << 1)

// Khai báo trước các hàm OLED
void ssd1306_send_cmd(uint8_t cmd);
void ssd1306_send_data(uint8_t *data, uint16_t size);
void ssd1306_set_cursor(uint8_t x, uint8_t page);
void ssd1306_clear(void);
void ssd1306_draw_char(uint8_t x, uint8_t page, char c);
void ssd1306_draw_string(uint8_t x, uint8_t page, const char* str);

// Hàm hiển thị nội dung theo yêu cầu
void oled_display_status(uint16_t gas_ppm, uint8_t system_status, uint8_t gas_alert_status) {
    char buffer[32];

    ssd1306_clear();

    // Hiển thị dòng 1: Nồng độ khí gas
    ssd1306_set_cursor(0, 0);
    sprintf(buffer, "Gas: %d ppm", gas_ppm);
    ssd1306_draw_string(0, 0, buffer);

    // Hiển thị dòng 2: Trạng thái hệ thống
    ssd1306_set_cursor(0, 2);
    sprintf(buffer, "System: %s", (system_status == 1) ? "Running" : "Stopped");
    ssd1306_draw_string(0, 2, buffer);

    // Hiển thị dòng 3: Trạng thái cảnh báo
    ssd1306_set_cursor(0, 4);
    const char *alert_str;
    switch (gas_alert_status) {
        case 0: alert_str = "OK"; break;
        case 1: alert_str = "Low Gas"; break;
        case 2: alert_str = "High Gas"; break;
        case 3: alert_str = "Danger!!!"; break;
        default: alert_str = "Unknown"; break;
    }
    sprintf(buffer, "Alert: %s", alert_str);
    ssd1306_draw_string(0, 4, buffer);
}

// Hàm vẽ chuỗi string ra màn hình
void ssd1306_draw_string(uint8_t x, uint8_t page, const char* str) {
    while (*str) {
        ssd1306_draw_char(x, page, *str);
        x += 6; // 5 pixel font + 1 pixel khoảng cách
        str++;
    }
}
