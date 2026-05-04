// obd_types.h - OBD-II Tip Tanımları — PID, DTC, servis yapıları

#ifndef OBD_TYPES_H
#define OBD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "project_config.h"

#define OBD_PID_SUPPORTED_01_20     0x00U   
#define OBD_PID_MONITOR_STATUS      0x01U   
#define OBD_PID_COOLANT_TEMP        0x05U   
#define OBD_PID_ENGINE_RPM          0x0CU   
#define OBD_PID_VEHICLE_SPEED       0x0DU   
#define OBD_PID_SUPPORTED_21_40     0x20U   
#define OBD_PID_SUPPORTED_41_60     0x40U   
#define OBD_PID_OIL_TEMPERATURE     0x5CU   

#define OBD_INFOTYPE_VIN_COUNT      0x01U   
#define OBD_INFOTYPE_VIN            0x02U   

// DTC kategorileri (2-byte DTC'nin üst 2 biti)
typedef enum {
    DTC_CAT_POWERTRAIN = 0x00,  
    DTC_CAT_CHASSIS    = 0x40,  
    DTC_CAT_BODY       = 0x80,  
    DTC_CAT_NETWORK    = 0xC0   
} DTC_Category_t;

typedef enum {
    DTC_STATUS_INACTIVE  = 0,   
    DTC_STATUS_PENDING   = 1,   
    DTC_STATUS_CONFIRMED = 2    
} DTC_Status_t;

#define DTC_P0217   0x0217U     
#define DTC_P0196   0x0196U     
#define DTC_P0500   0x0500U     

// Freeze Frame yapısı
typedef struct {
    uint16_t dtc_code;          
    uint16_t rpm;               
    int16_t  coolant_temp_x10;  
    int16_t  oil_temp_x10;      
    uint8_t  vehicle_speed;     
    uint32_t timestamp_ms;      
    bool     valid;             
} FreezeFrame_t;

// DTC girişi — tek bir DTC'nin tüm bilgileri
typedef struct {
    uint16_t      code;         
    DTC_Status_t  status;       
    uint8_t       debounce_cnt; 
    uint32_t      pending_since;
    FreezeFrame_t freeze_frame; 
} DTC_Entry_t;

// OBD application-level mesaj yapısı
typedef struct {
    uint8_t  service_id;        
    uint8_t  data[ISOTP_MAX_PAYLOAD_SIZE - 1]; 
    uint16_t length;            
} OBD_Message_t;

// Sensör snapshot yapısı
typedef struct {
    uint16_t rpm;               
    int16_t  coolant_temp;      
    int16_t  oil_temp;          
    uint8_t  vehicle_speed;     
    bool     engine_running;    
} SensorData_t;

// PID numarasına göre support bitmap'te ilgili bit'i set et
static inline void OBD_SetPidSupported(uint8_t bitmap[4], uint8_t pid_offset)
{
    if ((pid_offset >= 1U) && (pid_offset <= 32U))
    {
        uint8_t zero_idx  = pid_offset - 1U;
        uint8_t byte_idx  = zero_idx / 8U;
        uint8_t bit_pos   = 7U - (zero_idx % 8U);
        bitmap[byte_idx] |= (1U << bit_pos);
    }
}

#ifdef __cplusplus
}
#endif

#endif 
