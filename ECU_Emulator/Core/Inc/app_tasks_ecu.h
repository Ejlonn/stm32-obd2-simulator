// app_tasks_ecu.h - ECU Emulator — FreeRTOS Task Implementasyonları

#ifndef APP_TASKS_ECU_H
#define APP_TASKS_ECU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

// Tüm ECU task'larını, queue'ları, mutex'leri oluştur ve başlat
void AppTasks_ECU_Init(SPI_HandleTypeDef *hspi,
                       UART_HandleTypeDef *huart,
                       I2C_HandleTypeDef  *hi2c);

#ifdef __cplusplus
}
#endif

#endif 
