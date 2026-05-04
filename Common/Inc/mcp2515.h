// mcp2515.h - MCP2515 CAN Controller — Driver API

#ifndef MCP2515_H
#define MCP2515_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MCP2515_OK = 0,             
    MCP2515_ERR_SPI,            
    MCP2515_ERR_MODE,           
    MCP2515_ERR_TIMEOUT,        
    MCP2515_ERR_TX_BUSY,        
    MCP2515_ERR_NO_MSG,         
    MCP2515_ERR_PARAM,          
    MCP2515_ERR_MUTEX           
} MCP2515_Status_t;

typedef enum {
    MCP2515_MODE_NORMAL_OP    = 0x00U,
    MCP2515_MODE_SLEEP_OP     = 0x20U,
    MCP2515_MODE_LOOPBACK_OP  = 0x40U,
    MCP2515_MODE_LISTENONLY_OP = 0x60U,
    MCP2515_MODE_CONFIG_OP    = 0x80U
} MCP2515_OpMode_t;

typedef struct {
    uint32_t id;            
    uint8_t  dlc;           
    uint8_t  data[8];       
    bool     is_extended;   
    bool     is_rtr;        
} CAN_Frame_t;

// MCP2515 handle yapısı
typedef struct {
    
    SPI_HandleTypeDef *hspi;        
    GPIO_TypeDef      *cs_port;     
    uint16_t           cs_pin;      

    SemaphoreHandle_t  spi_mutex;   

    bool               initialized; 
} MCP2515_Handle_t;

// MCP2515 başlatma — reset gönderir ve config mode'a geçer
MCP2515_Status_t MCP2515_Init(MCP2515_Handle_t *hmcp);

// CAN bit timing konfigürasyonu
MCP2515_Status_t MCP2515_ConfigBitTiming(MCP2515_Handle_t *hmcp,
                                          uint8_t cnf1,
                                          uint8_t cnf2,
                                          uint8_t cnf3);

// RX acceptance filter ve mask konfigürasyonu
MCP2515_Status_t MCP2515_ConfigFilter(MCP2515_Handle_t *hmcp,
                                       uint16_t mask0, uint16_t mask1,
                                       uint16_t filter0, uint16_t filter1);

// Çalışma modu değiştirme
MCP2515_Status_t MCP2515_SetMode(MCP2515_Handle_t *hmcp, MCP2515_OpMode_t mode);

// CAN frame gönder
MCP2515_Status_t MCP2515_SendFrame(MCP2515_Handle_t *hmcp, const CAN_Frame_t *frame);

// CAN frame oku (RX buffer'dan)
MCP2515_Status_t MCP2515_ReadFrame(MCP2515_Handle_t *hmcp, CAN_Frame_t *frame);

// Interrupt flag register'ını oku
MCP2515_Status_t MCP2515_GetInterruptFlags(MCP2515_Handle_t *hmcp, uint8_t *flags);

// Belirli interrupt flag'lerini temizle
MCP2515_Status_t MCP2515_ClearInterruptFlags(MCP2515_Handle_t *hmcp, uint8_t flags);

// Hata flag register'ını oku
MCP2515_Status_t MCP2515_GetErrorFlags(MCP2515_Handle_t *hmcp, uint8_t *eflg);

// CANSTAT register'ından mevcut çalışma modunu oku
MCP2515_Status_t MCP2515_GetCurrentMode(MCP2515_Handle_t *hmcp, MCP2515_OpMode_t *mode);

// Interrupt enable register (CANINTE) programla
MCP2515_Status_t MCP2515_EnableInterrupts(MCP2515_Handle_t *hmcp, uint8_t int_mask);

#ifdef __cplusplus
}
#endif

#endif 
