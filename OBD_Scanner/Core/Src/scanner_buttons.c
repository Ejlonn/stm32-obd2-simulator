// scanner_buttons.c - OBD Scanner — 4-Buton Yöneticisi Implementasyonu

#include "scanner_buttons.h"
#include "main.h"
#include "project_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <string.h>
#include <stdbool.h>

typedef struct {
    GPIO_TypeDef *port;         
    uint16_t      pin;          
    ScannerCmd_t  command;      
    const char   *name;         
} ButtonDef_t;

typedef struct {
    uint8_t  stable_count;      
    bool     was_pressed;       
} ButtonState_t;

#define BUTTON_COUNT            4U

#define BUTTON_POLL_PERIOD_MS   20U

#define BUTTON_DEBOUNCE_COUNT   3U

static const ButtonDef_t s_button_defs[BUTTON_COUNT] = {
    { BTN_K1_GPIO_Port, BTN_K1_Pin, SCANNER_CMD_LIVE_DATA, "K1-LiveData" },
    { BTN_K2_GPIO_Port, BTN_K2_Pin, SCANNER_CMD_READ_DTC,  "K2-ReadDTC"  },
    { BTN_K3_GPIO_Port, BTN_K3_Pin, SCANNER_CMD_CLEAR_DTC, "K3-ClearDTC" },
    { BTN_K4_GPIO_Port, BTN_K4_Pin, SCANNER_CMD_READ_VIN,  "K4-ReadVIN"  },
};

static QueueHandle_t s_cmd_queue = NULL;

static ButtonState_t s_button_state[BUTTON_COUNT];

void ScannerButtons_Init(QueueHandle_t cmd_queue)
{
    s_cmd_queue = cmd_queue;
    memset(s_button_state, 0, sizeof(s_button_state));
}

void ScannerButtons_Task(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();

    DEBUG_LOG(DEBUG_LEVEL_INFO, "ScannerButtons: Task started (poll=%ums)\r\n",
              BUTTON_POLL_PERIOD_MS);

    for (;;)
    {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(BUTTON_POLL_PERIOD_MS));

        for (uint8_t i = 0; i < BUTTON_COUNT; i++)
        {
            
            GPIO_PinState pin_state = HAL_GPIO_ReadPin(
                s_button_defs[i].port, s_button_defs[i].pin);

            bool is_pressed = (pin_state == GPIO_PIN_RESET);

            if (is_pressed)
            {
                
                if (s_button_state[i].stable_count < BUTTON_DEBOUNCE_COUNT)
                {
                    s_button_state[i].stable_count++;
                }

                if (s_button_state[i].stable_count >= BUTTON_DEBOUNCE_COUNT &&
                    !s_button_state[i].was_pressed)
                {
                    s_button_state[i].was_pressed = true;

                    if (s_cmd_queue != NULL)
                    {
                        ScannerCmd_t cmd = s_button_defs[i].command;
                        if (xQueueSend(s_cmd_queue, &cmd, 0) == pdTRUE)
                        {
                            DEBUG_LOG(DEBUG_LEVEL_INFO,
                                      "BTN: %s pressed → CMD=%u\r\n",
                                      s_button_defs[i].name,
                                      (unsigned)cmd);
                        }
                        else
                        {
                            DEBUG_LOG(DEBUG_LEVEL_WARN,
                                      "BTN: Queue full, %s dropped\r\n",
                                      s_button_defs[i].name);
                        }
                    }
                }
            }
            else
            {
                
                s_button_state[i].stable_count = 0;
                s_button_state[i].was_pressed  = false;
            }
        }
    }
}
