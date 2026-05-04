// scanner_ui.h - OBD Scanner — OLED UI Yöneticisi API (Paket 5)

#ifndef SCANNER_UI_H
#define SCANNER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

// UI modülünü başlat
void ScannerUI_Init(I2C_HandleTypeDef *hi2c);

// UI FreeRTOS task fonksiyonu
void ScannerUI_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif 
