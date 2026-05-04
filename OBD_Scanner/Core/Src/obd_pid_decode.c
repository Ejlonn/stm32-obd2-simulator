// obd_pid_decode.c - OBD Scanner — Response Decoder Implementasyonu (Paket 4)

#include "obd_pid_decode.h"
#include "obd_pid_encode.h"
#include "obd_types.h"
#include <string.h>

// Yanıtın beklenen SID'e sahip olup olmadığını kontrol et
static bool validate_response(const ScannerComm_RawResponse_t *resp,
                               uint8_t expected_sid,
                               uint16_t min_len)
{
    if (!resp->valid || resp->length < min_len) {
        return false;
    }
    return (resp->data[0] == expected_sid);
}

bool OBD_DecodeLiveData(const ScannerComm_Result_t *result, OBD_LiveData_t *out)
{
    memset(out, 0, sizeof(*out));

    uint8_t decoded_count = 0;
    const uint8_t resp_sid = OBD_SERVICE_01_LIVE_DATA + OBD_RESPONSE_OFFSET; 

    if (validate_response(&result->live[0], resp_sid, 4)) {
        out->rpm = OBD_DecodeRPM(result->live[0].data[2], result->live[0].data[3]);
        decoded_count++;
    }

    if (validate_response(&result->live[1], resp_sid, 3)) {
        out->speed_kmh = OBD_DecodeVehicleSpeed(result->live[1].data[2]);
        decoded_count++;
    }

    if (validate_response(&result->live[2], resp_sid, 3)) {
        out->coolant_temp = OBD_DecodeCoolantTemp(result->live[2].data[2]);
        decoded_count++;
    }

    if (validate_response(&result->live[3], resp_sid, 3)) {
        out->oil_temp = OBD_DecodeOilTemp(result->live[3].data[2]);
        decoded_count++;
    }

    if (validate_response(&result->live[4], resp_sid, 6)) {
        OBD_DecodeMonitorStatus(&result->live[4].data[2],
                                &out->mil_on, &out->dtc_count);
        decoded_count++;
    }

    out->valid = (decoded_count > 0);
    return out->valid;
}

bool OBD_DecodeDtcList(const ScannerComm_RawResponse_t *resp, OBD_DtcList_t *out)
{
    memset(out, 0, sizeof(*out));

    const uint8_t resp_sid = OBD_SERVICE_03_STORED_DTC + OBD_RESPONSE_OFFSET; 
    if (!validate_response(resp, resp_sid, 2)) {
        return false;
    }

    uint8_t reported_count = resp->data[1];
    
    uint8_t available_pairs = (resp->length - 2) / 2;
    uint8_t count = (reported_count < available_pairs) ? reported_count : available_pairs;
    if (count > DTC_MAX_COUNT) {
        count = DTC_MAX_COUNT;
    }

    for (uint8_t i = 0; i < count; i++) {
        uint8_t h = resp->data[2 + i * 2];
        uint8_t l = resp->data[3 + i * 2];
        out->codes[i] = ((uint16_t)h << 8) | l;
        OBD_DTCToString(out->codes[i], out->strings[i]);
    }

    out->count = count;
    out->valid = true;
    return true;
}

bool OBD_DecodeClearDtc(const ScannerComm_RawResponse_t *resp)
{
    const uint8_t resp_sid = OBD_SERVICE_04_CLEAR_DTC + OBD_RESPONSE_OFFSET; 
    return validate_response(resp, resp_sid, 1);
}

bool OBD_DecodeVin(const ScannerComm_RawResponse_t *resp, OBD_Vin_t *out)
{
    memset(out, 0, sizeof(*out));

    const uint8_t resp_sid = OBD_SERVICE_09_VEHICLE_INFO + OBD_RESPONSE_OFFSET; 
    if (!validate_response(resp, resp_sid, 3 + SIM_VIN_LENGTH)) {
        return false;
    }

    memcpy(out->vin, &resp->data[3], SIM_VIN_LENGTH);
    out->vin[SIM_VIN_LENGTH] = '\0';
    out->valid = true;
    return true;
}
