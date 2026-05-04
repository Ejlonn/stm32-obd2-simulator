// can_interface.h - CAN Arayüz Soyutlama Katmanı

#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mcp2515.h"
#include "FreeRTOS.h"
#include "queue.h"

typedef enum {
    CAN_IF_OK = 0,
    CAN_IF_ERR_INIT,
    CAN_IF_ERR_SEND,
    CAN_IF_ERR_QUEUE_FULL,
    CAN_IF_ERR_TIMEOUT
} CAN_IF_Status_t;

// CAN Interface konfigürasyon yapısı
typedef struct {
    MCP2515_Handle_t *hmcp;             
    QueueHandle_t     rx_queue;         
    TaskHandle_t      rx_task_handle;   
} CAN_IF_Config_t;

// CAN Interface'i başlat
CAN_IF_Status_t CAN_IF_Init(const CAN_IF_Config_t *config);

// CAN frame gönder
CAN_IF_Status_t CAN_IF_SendFrame(const CAN_Frame_t *frame);

// MCP2515 INT# ISR callback'i
void CAN_IF_ISR_Callback(void);

// CAN RX handler task fonksiyonu
void CAN_IF_RxTask(void *argument);

// RX queue'dan frame al
CAN_IF_Status_t CAN_IF_ReceiveFrame(CAN_Frame_t *frame, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif 
