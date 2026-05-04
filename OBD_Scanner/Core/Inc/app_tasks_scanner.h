// app_tasks_scanner.h - OBD Scanner — Ana Task Orchestrator API

#ifndef APP_TASKS_SCANNER_H
#define APP_TASKS_SCANNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

// Scanner alt-sistem init — tüm task'ları oluşturur
void AppTasks_Scanner_Init(SPI_HandleTypeDef *hspi,
                           UART_HandleTypeDef *huart,
                           I2C_HandleTypeDef  *hi2c);

#ifdef __cplusplus
}
#endif

#endif 
