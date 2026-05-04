// obd_pid_encode.h - OBD-II PID Encode / Decode Fonksiyonları

#ifndef OBD_PID_ENCODE_H
#define OBD_PID_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Coolant temperature → OBD byte encode
uint8_t OBD_EncodeCoolantTemp(int16_t temp_c);

// OBD byte → coolant temperature decode
int16_t OBD_DecodeCoolantTemp(uint8_t byte_a);

// RPM → OBD bytes encode
void OBD_EncodeRPM(uint16_t rpm, uint8_t *byte_a, uint8_t *byte_b);

// OBD bytes → RPM decode
uint16_t OBD_DecodeRPM(uint8_t byte_a, uint8_t byte_b);

// Vehicle speed → OBD byte encode
uint8_t OBD_EncodeVehicleSpeed(uint8_t speed_kph);

// OBD byte → vehicle speed decode
uint8_t OBD_DecodeVehicleSpeed(uint8_t byte_a);

// Oil temperature → OBD byte encode
uint8_t OBD_EncodeOilTemp(int16_t temp_c);

// OBD byte → oil temperature decode
int16_t OBD_DecodeOilTemp(uint8_t byte_a);

// MIL durumu ve DTC sayısı → Mode 01 PID 0x01 encode
void OBD_EncodeMonitorStatus(bool mil_on, uint8_t dtc_count, uint8_t out[4]);

// Mode 01 PID 0x01 → MIL durumu ve DTC sayısı decode
void OBD_DecodeMonitorStatus(const uint8_t data[4], bool *mil_on, uint8_t *dtc_count);

// DTC raw code → string format (ör: "P0217")
void OBD_DTCToString(uint16_t dtc_code, char str[6]);

// DTC string → raw code
uint16_t OBD_StringToDTC(const char str[5]);

// Projede desteklenen PID'ler için support bitmap üret
void OBD_BuildSupportedPidBitmap(uint8_t base_pid, uint8_t bitmap[4]);

#ifdef __cplusplus
}
#endif

#endif 
