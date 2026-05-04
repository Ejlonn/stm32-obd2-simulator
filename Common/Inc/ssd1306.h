// ssd1306.h - SSD1306 / SSD1315 OLED I2C Driver — Public API

#ifndef SSD1306_H
#define SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define SSD1306_WIDTH           128U
#define SSD1306_HEIGHT          64U
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8U)   

#define SSD1306_I2C_ADDR        (0x3C << 1)

#define SSD1306_I2C_TIMEOUT     100U

typedef enum {
    SSD1306_BLACK = 0,  
    SSD1306_WHITE = 1   
} SSD1306_Color_t;

// OLED ekranı başlat (init sekansı gönder)
bool SSD1306_Init(I2C_HandleTypeDef *hi2c);

// Framebuffer'ı tamamen temizle (siyah)
void SSD1306_Clear(void);

// Framebuffer'ı belirtilen renkle doldur
void SSD1306_Fill(SSD1306_Color_t color);

// Metin yazma imleç pozisyonunu ayarla
void SSD1306_SetCursor(uint8_t x, uint8_t y);

// Tek karakter yaz (Font_6x8)
void SSD1306_WriteChar(char ch, SSD1306_Color_t color);

// String yaz (null-terminated, Font_6x8)
void SSD1306_WriteString(const char *str, SSD1306_Color_t color);

// Tek piksel çiz
void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_Color_t color);

// Yatay çizgi çiz
void SSD1306_DrawHLine(uint8_t x, uint8_t y, uint8_t w, SSD1306_Color_t color);

// Dolgulu dikdörtgen çiz
void SSD1306_DrawFilledRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_Color_t color);

// Belirtilen dikdörtgen alanı ters çevir (invert)
void SSD1306_InvertRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

// Framebuffer içeriğini I2C üzerinden OLED'e gönder
void SSD1306_UpdateScreen(void);

#ifdef __cplusplus
}
#endif

#endif 
