// obd_pid_encode.c - OBD-II PID Encode / Decode — Implementation

#include "obd_pid_encode.h"
#include "obd_types.h"
#include <string.h>

uint8_t OBD_EncodeCoolantTemp(int16_t temp_c)
{
    int16_t encoded = temp_c + 40;
    if (encoded < 0)   encoded = 0;
    if (encoded > 255) encoded = 255;
    return (uint8_t)encoded;
}

int16_t OBD_DecodeCoolantTemp(uint8_t byte_a)
{
    return (int16_t)byte_a - 40;
}

void OBD_EncodeRPM(uint16_t rpm, uint8_t *byte_a, uint8_t *byte_b)
{
    uint16_t val = rpm * 4U;
    *byte_a = (uint8_t)(val >> 8);
    *byte_b = (uint8_t)(val & 0xFFU);
}

uint16_t OBD_DecodeRPM(uint8_t byte_a, uint8_t byte_b)
{
    return ((uint16_t)byte_a * 256U + (uint16_t)byte_b) / 4U;
}

uint8_t OBD_EncodeVehicleSpeed(uint8_t speed_kph)
{
    return speed_kph;   
}

uint8_t OBD_DecodeVehicleSpeed(uint8_t byte_a)
{
    return byte_a;
}

uint8_t OBD_EncodeOilTemp(int16_t temp_c)
{
    int16_t encoded = temp_c + 40;
    if (encoded < 0)   encoded = 0;
    if (encoded > 255) encoded = 255;
    return (uint8_t)encoded;
}

int16_t OBD_DecodeOilTemp(uint8_t byte_a)
{
    return (int16_t)byte_a - 40;
}

void OBD_EncodeMonitorStatus(bool mil_on, uint8_t dtc_count, uint8_t out[4])
{
    
    out[0] = (mil_on ? 0x80U : 0x00U) | (dtc_count & 0x7FU);
    out[1] = 0x07U;    
    out[2] = 0x65U;    
    out[3] = 0x00U;
}

void OBD_DecodeMonitorStatus(const uint8_t data[4], bool *mil_on, uint8_t *dtc_count)
{
    if (mil_on != NULL) {
        *mil_on = (data[0] & 0x80U) ? true : false;
    }
    if (dtc_count != NULL) {
        *dtc_count = data[0] & 0x7FU;
    }
}

void OBD_DTCToString(uint16_t dtc_code, char str[6])
{
    
    static const char category_chars[] = "PCBU";

    uint8_t high_byte = (uint8_t)(dtc_code >> 8);
    uint8_t low_byte  = (uint8_t)(dtc_code & 0xFF);

    uint8_t cat   = (high_byte >> 6) & 0x03U;
    uint8_t digit2 = (high_byte >> 4) & 0x03U;
    uint8_t digit3 = high_byte & 0x0FU;
    uint8_t digit4 = (low_byte >> 4) & 0x0FU;
    uint8_t digit5 = low_byte & 0x0FU;

    str[0] = category_chars[cat];
    str[1] = '0' + digit2;
    str[2] = (digit3 < 10) ? ('0' + digit3) : ('A' + digit3 - 10);
    str[3] = (digit4 < 10) ? ('0' + digit4) : ('A' + digit4 - 10);
    str[4] = (digit5 < 10) ? ('0' + digit5) : ('A' + digit5 - 10);
    str[5] = '\0';
}

uint16_t OBD_StringToDTC(const char str[5])
{
    uint8_t cat = 0;
    switch (str[0]) {
        case 'P': cat = 0; break;
        case 'C': cat = 1; break;
        case 'B': cat = 2; break;
        case 'U': cat = 3; break;
        default:  cat = 0; break;
    }

    uint8_t d2 = (uint8_t)(str[1] - '0') & 0x03U;
    uint8_t d3 = 0, d4 = 0, d5 = 0;

    for (uint8_t i = 2; i <= 4; i++) {
        uint8_t val;
        if (str[i] >= '0' && str[i] <= '9') {
            val = (uint8_t)(str[i] - '0');
        } else if (str[i] >= 'A' && str[i] <= 'F') {
            val = (uint8_t)(str[i] - 'A' + 10);
        } else if (str[i] >= 'a' && str[i] <= 'f') {
            val = (uint8_t)(str[i] - 'a' + 10);
        } else {
            val = 0;
        }

        switch (i) {
            case 2: d3 = val; break;
            case 3: d4 = val; break;
            case 4: d5 = val; break;
        }
    }

    uint8_t high_byte = (cat << 6) | (d2 << 4) | d3;
    uint8_t low_byte  = (d4 << 4) | d5;

    return ((uint16_t)high_byte << 8) | low_byte;
}

void OBD_BuildSupportedPidBitmap(uint8_t base_pid, uint8_t bitmap[4])
{
    memset(bitmap, 0, 4);

    switch (base_pid)
    {
        case 0x00:
            
            OBD_SetPidSupported(bitmap, OBD_PID_MONITOR_STATUS);   
            OBD_SetPidSupported(bitmap, OBD_PID_COOLANT_TEMP);     
            OBD_SetPidSupported(bitmap, OBD_PID_ENGINE_RPM);       
            OBD_SetPidSupported(bitmap, OBD_PID_VEHICLE_SPEED);    
            OBD_SetPidSupported(bitmap, OBD_PID_SUPPORTED_21_40);  
            break;

        case 0x20:

            OBD_SetPidSupported(bitmap, 32);   
            break;

        case 0x40:

            OBD_SetPidSupported(bitmap, 28);   
            break;

        default:
            
            break;
    }
}
