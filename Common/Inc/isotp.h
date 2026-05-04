// isotp.h - ISO-TP (ISO 15765-2) Transport Protocol Katmanı

#ifndef ISOTP_H
#define ISOTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"
#include "mcp2515.h"        

#define ISOTP_PCI_SF    0x00U   
#define ISOTP_PCI_FF    0x10U   
#define ISOTP_PCI_CF    0x20U   
#define ISOTP_PCI_FC    0x30U   

#define ISOTP_FC_FS_CTS     0x00U   
#define ISOTP_FC_FS_WAIT    0x01U   
#define ISOTP_FC_FS_OVFLW   0x02U   

#define ISOTP_PAD_BYTE      0xCCU

#define ISOTP_SF_MAX_DATA   7U

#define ISOTP_FF_FIRST_DATA 6U

#define ISOTP_CF_DATA       7U

typedef enum {
    ISOTP_OK = 0,               
    ISOTP_COMPLETE,             
    ISOTP_IN_PROGRESS,          
    ISOTP_ERR_TIMEOUT,          
    ISOTP_ERR_SEQUENCE,         
    ISOTP_ERR_OVERFLOW,         
    ISOTP_ERR_FC_OVERFLOW,      
    ISOTP_ERR_INVALID_FRAME,    
    ISOTP_ERR_BUSY,             
    ISOTP_ERR_PARAM             
} ISOTP_Status_t;

typedef enum {
    ISOTP_STATE_IDLE,               
    ISOTP_STATE_TX_WAIT_FC,         
    ISOTP_STATE_TX_SENDING_CF,      
    ISOTP_STATE_RX_RECEIVING_CF     
} ISOTP_State_t;

// ISO-TP session yapısı
typedef struct {
    
    ISOTP_State_t state;

    uint8_t  rx_buffer[ISOTP_MAX_PAYLOAD_SIZE]; 
    uint16_t rx_total_length;       
    uint16_t rx_offset;             
    uint8_t  rx_seq_number;         

    uint8_t  tx_buffer[ISOTP_MAX_PAYLOAD_SIZE]; 
    uint16_t tx_total_length;       
    uint16_t tx_offset;             
    uint8_t  tx_seq_number;         
    uint8_t  tx_block_size;         
    uint8_t  tx_block_count;        
    uint8_t  tx_stmin;              

    uint32_t last_activity_tick;    

    uint16_t tx_can_id;             
    uint16_t rx_can_id;             
} ISOTP_Session_t;

// ISO-TP session başlat
ISOTP_Status_t ISOTP_Init(ISOTP_Session_t *session, uint16_t tx_id, uint16_t rx_id);

// Veri gönder (segmentation otomatik yapılır)
ISOTP_Status_t ISOTP_Send(ISOTP_Session_t *session,
                           const uint8_t *data, uint16_t length);

// Alınan CAN frame'i ISO-TP katmanında işle
ISOTP_Status_t ISOTP_ProcessRxFrame(ISOTP_Session_t *session,
                                     const CAN_Frame_t *frame);

// Tamamlanmış mesajı al
ISOTP_Status_t ISOTP_GetReceivedMessage(ISOTP_Session_t *session,
                                         uint8_t *data, uint16_t *length);

// Periyodik timeout kontrolü
ISOTP_Status_t ISOTP_Tick(ISOTP_Session_t *session);

// Session'ı zorla sıfırla (IDLE'a döndür)
void ISOTP_Reset(ISOTP_Session_t *session);

// Session'ın mevcut durumunu sorgula
ISOTP_State_t ISOTP_GetState(const ISOTP_Session_t *session);

// TX state machine'i ilerlet (non-blocking)
ISOTP_Status_t ISOTP_ProcessTx(ISOTP_Session_t *session);

#ifdef __cplusplus
}
#endif

#endif 
