// fault_manager.h - ECU Emulator — Fault Manager

#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "obd_types.h"
#include <stdbool.h>

typedef enum {
    FAULT_ENGINE_OVERTEMP = 0,      
    FAULT_OIL_OVERTEMP,             
    FAULT_SPEED_RPM_MISMATCH,       
    FAULT_COUNT                     
} FaultId_t;

// Fault manager'ı başlat — tüm DTC'leri temizle
void FaultMgr_Init(void);

// Periyodik fault değerlendirme (her 200ms çağrılır)
void FaultMgr_Evaluate(const SensorData_t *sensor);

// Tüm DTC'leri temizle (Mode 04 handler tarafından çağrılır)
void FaultMgr_ClearAllDTC(void);

// MIL (Malfunction Indicator Lamp) durumunu al
bool FaultMgr_IsMilOn(void);

// Confirmed DTC sayısını al
uint8_t FaultMgr_GetConfirmedDTCCount(void);

// Pending DTC sayısını al
uint8_t FaultMgr_GetPendingDTCCount(void);

// Confirmed DTC listesini al (Mode 03 yanıtı için)
uint8_t FaultMgr_GetConfirmedDTCs(uint16_t *dtc_codes, uint8_t max_count);

// Pending DTC listesini al (Mode 07 yanıtı için)
uint8_t FaultMgr_GetPendingDTCs(uint16_t *dtc_codes, uint8_t max_count);

// Belirli bir DTC'nin freeze frame'ini al (Mode 02 yanıtı için)
bool FaultMgr_GetFreezeFrame(uint8_t frame_number, FreezeFrame_t *ff);

// Toplam DTC sayısı (confirmed + pending)
uint8_t FaultMgr_GetTotalDTCCount(void);

#ifdef __cplusplus
}
#endif

#endif 
