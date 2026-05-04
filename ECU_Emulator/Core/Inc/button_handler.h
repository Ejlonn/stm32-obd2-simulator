// button_handler.h - ECU Emulator — Button Handler (Fault Injection Tetikleme)

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

// Button handler'ı başlat
void ButtonHandler_Init(TaskHandle_t task_handle);

// Button handler task fonksiyonu
void ButtonHandler_Task(void *argument);

// EXTI ISR callback — HAL_GPIO_EXTI_Callback içinden çağrılır
void ButtonHandler_EXTI_Callback(uint16_t gpio_pin);

// Fault injection aktif mi
bool ButtonHandler_IsFaultActive(void);

#ifdef __cplusplus
}
#endif

#endif 
