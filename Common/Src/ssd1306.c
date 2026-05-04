// ssd1306.c - SSD1306 / SSD1315 OLED I2C Driver — Implementation

#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <string.h>

static uint8_t s_buffer[SSD1306_BUFFER_SIZE];

static I2C_HandleTypeDef *s_hi2c = NULL;

static bool s_initialized = false;

static uint8_t s_cursor_x = 0;
static uint8_t s_cursor_y = 0;

// SSD1306'ya tek komut gönder
static bool ssd1306_write_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { 0x00, cmd };   
    return (HAL_I2C_Master_Transmit(s_hi2c, SSD1306_I2C_ADDR,
                                     buf, 2, SSD1306_I2C_TIMEOUT) == HAL_OK);
}

// SSD1306'ya veri bloğu gönder (framebuffer transfer)
static bool ssd1306_write_data(const uint8_t *data, uint16_t len)
{
    
    return (HAL_I2C_Mem_Write(s_hi2c, SSD1306_I2C_ADDR, 0x40,
                               I2C_MEMADD_SIZE_8BIT,
                               (uint8_t *)data, len,
                               SSD1306_I2C_TIMEOUT) == HAL_OK);
}

bool SSD1306_Init(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == NULL) {
        return false;
    }

    s_hi2c = hi2c;

    if (HAL_I2C_IsDeviceReady(s_hi2c, SSD1306_I2C_ADDR, 3, 100) != HAL_OK) {
        return false;
    }

    static const uint8_t init_cmds[] = {
        0xAE,       
        0xD5, 0x80, 
        0xA8, 0x3F, 
        0xD3, 0x00, 
        0x40,       
        0x8D, 0x14, 
        0x20, 0x00, 
        0xA1,       
        0xC8,       
        0xDA, 0x12, 
        0x81, 0x7F, 
        0xD9, 0xF1, 
        0xDB, 0x40, 
        0xA4,       
        0xA6,       
        0xAF,       
    };

    for (uint8_t i = 0; i < sizeof(init_cmds); i++) {
        if (!ssd1306_write_cmd(init_cmds[i])) {
            return false;
        }
    }

    SSD1306_Clear();
    SSD1306_UpdateScreen();

    s_initialized = true;
    return true;
}

void SSD1306_Clear(void)
{
    memset(s_buffer, 0x00, sizeof(s_buffer));
    s_cursor_x = 0;
    s_cursor_y = 0;
}

void SSD1306_Fill(SSD1306_Color_t color)
{
    memset(s_buffer, (color == SSD1306_WHITE) ? 0xFF : 0x00, sizeof(s_buffer));
}

void SSD1306_SetCursor(uint8_t x, uint8_t y)
{
    s_cursor_x = x;
    s_cursor_y = y;
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_Color_t color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return;
    }

    uint16_t idx = (y / 8U) * SSD1306_WIDTH + x;

    if (color == SSD1306_WHITE) {
        s_buffer[idx] |=  (1U << (y & 7U));
    } else {
        s_buffer[idx] &= ~(1U << (y & 7U));
    }
}

void SSD1306_WriteChar(char ch, SSD1306_Color_t color)
{
    
    if (ch < 32 || ch > 126) {
        ch = ' ';
    }

    if (s_cursor_x + Font_6x8.width > SSD1306_WIDTH) {
        s_cursor_x = 0;
        s_cursor_y += Font_6x8.height;
    }
    if (s_cursor_y + Font_6x8.height > SSD1306_HEIGHT) {
        return; 
    }

    uint16_t offset = (uint16_t)(ch - 32) * Font_6x8.width;

    if ((s_cursor_y & 7U) == 0) {
        
        uint8_t page = s_cursor_y / 8U;
        uint16_t buf_idx = page * SSD1306_WIDTH + s_cursor_x;

        for (uint8_t col = 0; col < Font_6x8.width; col++) {
            uint8_t col_data = Font_6x8.data[offset + col];
            if (color == SSD1306_WHITE) {
                s_buffer[buf_idx + col] |= col_data;
            } else {
                s_buffer[buf_idx + col] &= ~col_data;
            }
        }
    } else {
        
        for (uint8_t col = 0; col < Font_6x8.width; col++) {
            uint8_t col_data = Font_6x8.data[offset + col];
            for (uint8_t row = 0; row < Font_6x8.height; row++) {
                if (col_data & (1U << row)) {
                    SSD1306_DrawPixel(s_cursor_x + col,
                                       s_cursor_y + row, color);
                }
            }
        }
    }

    s_cursor_x += Font_6x8.width;
}

void SSD1306_WriteString(const char *str, SSD1306_Color_t color)
{
    if (str == NULL) return;

    while (*str) {
        SSD1306_WriteChar(*str++, color);
    }
}

void SSD1306_DrawHLine(uint8_t x, uint8_t y, uint8_t w, SSD1306_Color_t color)
{
    for (uint8_t i = 0; i < w && (x + i) < SSD1306_WIDTH; i++) {
        SSD1306_DrawPixel(x + i, y, color);
    }
}

void SSD1306_DrawFilledRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_Color_t color)
{
    for (uint8_t row = 0; row < h; row++) {
        SSD1306_DrawHLine(x, y + row, w, color);
    }
}

void SSD1306_InvertRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    for (uint8_t row = y; row < y + h && row < SSD1306_HEIGHT; row++) {
        for (uint8_t col = x; col < x + w && col < SSD1306_WIDTH; col++) {
            uint16_t idx = (row / 8U) * SSD1306_WIDTH + col;
            s_buffer[idx] ^= (1U << (row & 7U));
        }
    }
}

void SSD1306_UpdateScreen(void)
{
    if (!s_initialized || s_hi2c == NULL) {
        return;
    }

    ssd1306_write_cmd(0x21);    
    ssd1306_write_cmd(0x00);    
    ssd1306_write_cmd(0x7F);    

    ssd1306_write_cmd(0x22);    
    ssd1306_write_cmd(0x00);    
    ssd1306_write_cmd(0x07);    

    ssd1306_write_data(s_buffer, SSD1306_BUFFER_SIZE);
}
