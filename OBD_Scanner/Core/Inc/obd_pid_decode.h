// obd_pid_decode.h - OBD Scanner — Response Decoder API (Paket 4)

#ifndef OBD_PID_DECODE_H
#define OBD_PID_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "scanner_comm.h"
#include "project_config.h"

// Live Data — decode edilmiş canlı veriler
typedef struct {
    uint16_t rpm;               
    uint8_t  speed_kmh;         
    int16_t  coolant_temp;      
    int16_t  oil_temp;          
    bool     mil_on;            
    uint8_t  dtc_count;         
    bool     valid;             
} OBD_LiveData_t;

// DTC Listesi — decode edilmiş hata kodları
typedef struct {
    uint8_t  count;                         
    uint16_t codes[DTC_MAX_COUNT];          
    char     strings[DTC_MAX_COUNT][6];     
    bool     valid;                         
} OBD_DtcList_t;

// VIN — araç kimlik numarası
typedef struct {
    char vin[18];               
    bool valid;                 
} OBD_Vin_t;

// Live Data ham yanıtlarını decode et
bool OBD_DecodeLiveData(const ScannerComm_Result_t *result, OBD_LiveData_t *out);

// Service 03 (Read DTC) yanıtını decode et
bool OBD_DecodeDtcList(const ScannerComm_RawResponse_t *resp, OBD_DtcList_t *out);

// Service 04 (Clear DTC) yanıtını doğrula
bool OBD_DecodeClearDtc(const ScannerComm_RawResponse_t *resp);

// Service 09 (VIN) yanıtını decode et
bool OBD_DecodeVin(const ScannerComm_RawResponse_t *resp, OBD_Vin_t *out);

#ifdef __cplusplus
}
#endif

#endif 
