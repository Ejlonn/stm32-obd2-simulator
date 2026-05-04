// obd_service_ecu.c - ECU Emulator — OBD-II Service Handler — Implementation

#include "obd_service_ecu.h"
#include "obd_pid_encode.h"
#include "sensor_model.h"
#include "fault_manager.h"
#include "project_config.h"
#include <string.h>

// Mode 01 — Live Data PID handler
static bool handle_service_01(const OBD_Message_t *req, OBD_Message_t *resp)
{
    if (req->length < 1) {
        return false;   
    }

    uint8_t pid = req->data[0];

    resp->service_id = OBD_SERVICE_01_LIVE_DATA + OBD_RESPONSE_OFFSET;   
    resp->data[0]    = pid;

    SensorData_t sensor;
    SensorModel_GetData(&sensor);

    switch (pid)
    {
        case OBD_PID_SUPPORTED_01_20:   
        case OBD_PID_SUPPORTED_21_40:   
        case OBD_PID_SUPPORTED_41_60:   
        {
            uint8_t bitmap[4];
            OBD_BuildSupportedPidBitmap(pid, bitmap);
            memcpy(&resp->data[1], bitmap, 4);
            resp->length = 5;   
            return true;
        }

        case OBD_PID_MONITOR_STATUS:    
        {
            bool mil_on = FaultMgr_IsMilOn();
            uint8_t dtc_count = FaultMgr_GetConfirmedDTCCount();
            uint8_t status_bytes[4];
            OBD_EncodeMonitorStatus(mil_on, dtc_count, status_bytes);
            memcpy(&resp->data[1], status_bytes, 4);
            resp->length = 5;
            return true;
        }

        case OBD_PID_COOLANT_TEMP:      
        {
            resp->data[1] = OBD_EncodeCoolantTemp(sensor.coolant_temp);
            resp->length = 2;
            return true;
        }

        case OBD_PID_ENGINE_RPM:        
        {
            OBD_EncodeRPM(sensor.rpm, &resp->data[1], &resp->data[2]);
            resp->length = 3;
            return true;
        }

        case OBD_PID_VEHICLE_SPEED:     
        {
            resp->data[1] = OBD_EncodeVehicleSpeed(sensor.vehicle_speed);
            resp->length = 2;
            return true;
        }

        case OBD_PID_OIL_TEMPERATURE:   
        {
            resp->data[1] = OBD_EncodeOilTemp(sensor.oil_temp);
            resp->length = 2;
            return true;
        }

        default:
            
            return false;
    }
}

// Mode 02 — Freeze Frame handler
static bool handle_service_02(const OBD_Message_t *req, OBD_Message_t *resp)
{
    if (req->length < 2) {
        return false;
    }

    uint8_t pid          = req->data[0];
    uint8_t frame_number = req->data[1];

    FreezeFrame_t ff;
    if (!FaultMgr_GetFreezeFrame(frame_number, &ff)) {
        return false;   
    }

    resp->service_id = OBD_SERVICE_02_FREEZE_FRAME + OBD_RESPONSE_OFFSET;  
    resp->data[0]    = pid;
    resp->data[1]    = frame_number;

    switch (pid)
    {
        case OBD_PID_COOLANT_TEMP:
            resp->data[2] = OBD_EncodeCoolantTemp((int16_t)(ff.coolant_temp_x10 / 10));
            resp->length = 3;
            return true;

        case OBD_PID_ENGINE_RPM:
            OBD_EncodeRPM(ff.rpm, &resp->data[2], &resp->data[3]);
            resp->length = 4;
            return true;

        case OBD_PID_VEHICLE_SPEED:
            resp->data[2] = OBD_EncodeVehicleSpeed(ff.vehicle_speed);
            resp->length = 3;
            return true;

        case OBD_PID_OIL_TEMPERATURE:
            resp->data[2] = OBD_EncodeOilTemp((int16_t)(ff.oil_temp_x10 / 10));
            resp->length = 3;
            return true;

        default:
            return false;
    }
}

// Mode 03 — Stored/Confirmed DTC handler
static bool handle_service_03(const OBD_Message_t *req, OBD_Message_t *resp)
{
    (void)req;

    uint16_t dtc_codes[DTC_MAX_COUNT];
    uint8_t count = FaultMgr_GetConfirmedDTCs(dtc_codes, DTC_MAX_COUNT);

    resp->service_id = OBD_SERVICE_03_STORED_DTC + OBD_RESPONSE_OFFSET;    
    resp->data[0]    = count;

    for (uint8_t i = 0; i < count; i++) {
        resp->data[1 + i * 2]     = (uint8_t)(dtc_codes[i] >> 8);      
        resp->data[1 + i * 2 + 1] = (uint8_t)(dtc_codes[i] & 0xFF);   
    }

    resp->length = 1 + (count * 2);   
    return true;
}

// Mode 04 — Clear DTC handler
static bool handle_service_04(const OBD_Message_t *req, OBD_Message_t *resp)
{
    (void)req;

    FaultMgr_ClearAllDTC();

    resp->service_id = OBD_SERVICE_04_CLEAR_DTC + OBD_RESPONSE_OFFSET;     
    resp->length     = 0;   
    return true;
}

// Mode 07 — Pending DTC handler
static bool handle_service_07(const OBD_Message_t *req, OBD_Message_t *resp)
{
    (void)req;

    uint16_t dtc_codes[DTC_MAX_COUNT];
    uint8_t count = FaultMgr_GetPendingDTCs(dtc_codes, DTC_MAX_COUNT);

    resp->service_id = OBD_SERVICE_07_PENDING_DTC + OBD_RESPONSE_OFFSET;   
    resp->data[0]    = count;

    for (uint8_t i = 0; i < count; i++) {
        resp->data[1 + i * 2]     = (uint8_t)(dtc_codes[i] >> 8);
        resp->data[1 + i * 2 + 1] = (uint8_t)(dtc_codes[i] & 0xFF);
    }

    resp->length = 1 + (count * 2);
    return true;
}

// Mode 09 — Vehicle Info handler
static bool handle_service_09(const OBD_Message_t *req, OBD_Message_t *resp)
{
    if (req->length < 1) {
        return false;
    }

    uint8_t infotype = req->data[0];

    resp->service_id = OBD_SERVICE_09_VEHICLE_INFO + OBD_RESPONSE_OFFSET;  
    resp->data[0]    = infotype;

    switch (infotype)
    {
        case OBD_INFOTYPE_VIN_COUNT:    
            resp->data[1] = 1;          
            resp->length  = 2;
            return true;

        case OBD_INFOTYPE_VIN:          
        {
            
            resp->data[1] = 0x01;   
            memcpy(&resp->data[2], SIM_VIN_STRING, SIM_VIN_LENGTH);
            resp->length = 2 + SIM_VIN_LENGTH;   
            return true;
        }

        default:
            return false;
    }
}

// Negative Response üret
static bool build_negative_response(uint8_t service_id, uint8_t nrc, OBD_Message_t *resp)
{
    resp->service_id = OBD_NEGATIVE_RESPONSE_SID;   
    resp->data[0]    = service_id;                   
    resp->data[1]    = nrc;                          
    resp->length     = 2;
    return true;
}

bool OBD_ECU_HandleRequest(const OBD_Message_t *request, OBD_Message_t *response)
{
    if (request == NULL || response == NULL) {
        return false;
    }

    memset(response, 0, sizeof(OBD_Message_t));

    bool handled = false;

    switch (request->service_id)
    {
        case OBD_SERVICE_01_LIVE_DATA:
            handled = handle_service_01(request, response);
            break;

        case OBD_SERVICE_02_FREEZE_FRAME:
            handled = handle_service_02(request, response);
            break;

        case OBD_SERVICE_03_STORED_DTC:
            handled = handle_service_03(request, response);
            break;

        case OBD_SERVICE_04_CLEAR_DTC:
            handled = handle_service_04(request, response);
            break;

        case OBD_SERVICE_07_PENDING_DTC:
            handled = handle_service_07(request, response);
            break;

        case OBD_SERVICE_09_VEHICLE_INFO:
            handled = handle_service_09(request, response);
            break;

        default:
            
            return build_negative_response(request->service_id,
                                            OBD_NRC_SERVICE_NOT_SUPPORTED,
                                            response);
    }

    if (!handled) {
        
        return build_negative_response(request->service_id,
                                        OBD_NRC_SUBFUNCTION_NOT_SUPPORTED,
                                        response);
    }

    return true;
}
