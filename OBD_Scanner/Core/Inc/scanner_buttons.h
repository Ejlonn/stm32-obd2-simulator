// scanner_buttons.h - OBD Scanner — 4-Buton Yöneticisi API

#ifndef SCANNER_BUTTONS_H
#define SCANNER_BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

// Scanner komut tipleri — butonlardan ScannerComm'e gönderilir
typedef enum {
    SCANNER_CMD_NONE       = 0,
    SCANNER_CMD_LIVE_DATA  = 1,   
    SCANNER_CMD_READ_DTC   = 2,   
    SCANNER_CMD_CLEAR_DTC  = 3,   
    SCANNER_CMD_READ_VIN   = 4    
} ScannerCmd_t;

// Buton yöneticisini başlat
void ScannerButtons_Init(QueueHandle_t cmd_queue);

// Buton handler FreeRTOS task fonksiyonu
void ScannerButtons_Task(void *argument);

#ifdef __cplusplus
}
#endif

#endif 
