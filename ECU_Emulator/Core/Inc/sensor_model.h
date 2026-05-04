// sensor_model.h - ECU Emulator — Fizik Tabanlı Sensör Simülasyon Modeli

#ifndef SENSOR_MODEL_H
#define SENSOR_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "obd_types.h"
#include <stdbool.h>

typedef enum {
    SENSOR_PROFILE_NORMAL,      
    SENSOR_PROFILE_FAULT        
} SensorProfile_t;

// Sensör modelini başlangıç değerleriyle başlat
void SensorModel_Init(void);

// Sensör değerlerini güncelle (her 100ms çağrılır)
void SensorModel_Update(void);

// Aktif profili değiştir
void SensorModel_SetProfile(SensorProfile_t profile);

// Mevcut profili al
SensorProfile_t SensorModel_GetProfile(void);

// Güncel sensör verilerinin thread-safe kopyasını al
void SensorModel_GetData(SensorData_t *data);

// Aktif vites numarasını al (1-6)
uint8_t SensorModel_GetGear(void);

// Fault injection aktifleştir/deaktifleştir
void SensorModel_SetFaultInjection(bool active);

// Fault injection aktif mi
bool SensorModel_IsFaultActive(void);

#ifdef __cplusplus
}
#endif

#endif 
