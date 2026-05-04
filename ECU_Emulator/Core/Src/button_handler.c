// button_handler.c - ECU Emulator — Button Handler — Implementation

#include "button_handler.h"
#include "sensor_model.h"
#include "project_config.h"
#include "stm32f4xx_hal.h"

static TaskHandle_t s_task_handle = NULL;

static volatile bool s_fault_active = false;

#define BUTTON_DEBOUNCE_MS  50U

#ifndef B1_Pin
#define B1_Pin          GPIO_PIN_13
#endif
#ifndef B1_GPIO_Port
#define B1_GPIO_Port    GPIOC
#endif

void ButtonHandler_Init(TaskHandle_t task_handle)
{
    s_task_handle  = task_handle;
    s_fault_active = false;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "ButtonHandler: Init OK\r\n");
}

void ButtonHandler_Task(void *argument)
{
    (void)argument;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "ButtonHandler: Task started\r\n");

    for (;;)
    {
        
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));

        if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
        {
            
            s_fault_active = !s_fault_active;

            SensorModel_SetFaultInjection(s_fault_active);

            DEBUG_LOG(DEBUG_LEVEL_INFO, "Button: Fault injection %s\r\n",
                      s_fault_active ? "ON" : "OFF");

            while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));
        }
    }
}

void ButtonHandler_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == B1_Pin && s_task_handle != NULL)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(s_task_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

bool ButtonHandler_IsFaultActive(void)
{
    return s_fault_active;
}
