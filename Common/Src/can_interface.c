// can_interface.c - CAN Arayüz Soyutlama Katmanı — Implementation

#include "can_interface.h"
#include "mcp2515.h"
#include "mcp2515_regs.h"
#include "project_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static CAN_IF_Config_t s_config;

static bool s_initialized = false;

CAN_IF_Status_t CAN_IF_Init(const CAN_IF_Config_t *config)
{
    if (config == NULL || config->hmcp == NULL || config->rx_queue == NULL) {
        return CAN_IF_ERR_INIT;
    }

    s_config = *config;
    s_initialized = true;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "CAN_IF: Initialized\r\n");
    return CAN_IF_OK;
}

CAN_IF_Status_t CAN_IF_SendFrame(const CAN_Frame_t *frame)
{
    if (!s_initialized || frame == NULL) {
        return CAN_IF_ERR_SEND;
    }

    MCP2515_Status_t mcp_status = MCP2515_SendFrame(s_config.hmcp, frame);

    if (mcp_status == MCP2515_OK) {
        return CAN_IF_OK;
    }

    DEBUG_LOG(DEBUG_LEVEL_WARN, "CAN_IF: SendFrame failed, MCP status=%d\r\n", mcp_status);
    return CAN_IF_ERR_SEND;
}

void CAN_IF_ISR_Callback(void)
{
    
    if (!s_initialized || s_config.rx_task_handle == NULL) {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(s_config.rx_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void CAN_IF_RxTask(void *argument)
{
    (void)argument;

    CAN_Frame_t frame;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "CAN_IF: RxTask started\r\n");

    for (;;)
    {
        
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (MCP2515_ReadFrame(s_config.hmcp, &frame) == MCP2515_OK)
        {
            
            if (xQueueSend(s_config.rx_queue, &frame, 0) != pdTRUE) {
                DEBUG_LOG(DEBUG_LEVEL_WARN, "CAN_IF: RX queue full, frame dropped ID=0x%03lX\r\n",
                          frame.id);
            }
        }
    }
}

CAN_IF_Status_t CAN_IF_ReceiveFrame(CAN_Frame_t *frame, uint32_t timeout_ms)
{
    if (!s_initialized || frame == NULL) {
        return CAN_IF_ERR_TIMEOUT;
    }

    TickType_t wait_ticks = (timeout_ms == portMAX_DELAY)
                            ? portMAX_DELAY
                            : pdMS_TO_TICKS(timeout_ms);

    if (xQueueReceive(s_config.rx_queue, frame, wait_ticks) == pdTRUE) {
        return CAN_IF_OK;
    }

    return CAN_IF_ERR_TIMEOUT;
}
