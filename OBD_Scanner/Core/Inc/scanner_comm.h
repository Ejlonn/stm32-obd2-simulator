// scanner_comm.h - OBD Scanner — Haberleşme Motoru API (Paket 3)

#ifndef SCANNER_COMM_H
#define SCANNER_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "isotp.h"
#include "scanner_buttons.h"   
#include "project_config.h"

#define SCOMM_LIVE_PID_COUNT        5U

#define SCOMM_REQUEST_TIMEOUT_MS    1000U

#define SCOMM_RX_POLL_MS            10U

typedef enum {
    SCOMM_STATUS_NONE = 0,      
    SCOMM_STATUS_OK,            
    SCOMM_STATUS_TIMEOUT,       
    SCOMM_STATUS_ERROR          
} ScannerComm_Status_t;

typedef struct {
    uint8_t  data[ISOTP_MAX_PAYLOAD_SIZE];  
    uint16_t length;                        
    bool     valid;                         
} ScannerComm_RawResponse_t;

// Comm katmanı sonuç yapısı
typedef struct {
    ScannerCmd_t              cmd;      
    ScannerComm_Status_t      status;   

    ScannerComm_RawResponse_t single;

    ScannerComm_RawResponse_t live[SCOMM_LIVE_PID_COUNT];

    volatile bool             ready;    
} ScannerComm_Result_t;

// Haberleşme motorunu başlat
void ScannerComm_Init(ISOTP_Session_t *session, QueueHandle_t cmd_queue);

// Haberleşme motoru FreeRTOS task fonksiyonu
void ScannerComm_Task(void *argument);

// Son tamamlanan sonucun pointer'ını döndür
const ScannerComm_Result_t* ScannerComm_GetResult(void);

#ifdef __cplusplus
}
#endif

#endif 
