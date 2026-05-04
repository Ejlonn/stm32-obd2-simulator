// fault_manager.c - ECU Emulator — Fault Manager — Implementation

#include "fault_manager.h"
#include "sensor_model.h"
#include "project_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include <string.h>

// Fault koşul tanımı — statik tablo
typedef struct {
    FaultId_t  id;
    uint16_t   dtc_code;
    uint8_t    debounce_threshold;
} FaultDefinition_t;

static const FaultDefinition_t s_fault_defs[FAULT_COUNT] = {
    { FAULT_ENGINE_OVERTEMP,     DTC_P0217, FAULT_DEBOUNCE_THRESHOLD },
    { FAULT_OIL_OVERTEMP,        DTC_P0196, FAULT_DEBOUNCE_THRESHOLD },
    { FAULT_SPEED_RPM_MISMATCH,  DTC_P0500, FAULT_DEBOUNCE_THRESHOLD * 2U  },
};

static DTC_Entry_t s_dtc_table[FAULT_COUNT];

static bool s_mil_on = false;

// Belirli bir fault koşulunun aktif olup olmadığını kontrol et
static bool fault_condition_active(FaultId_t id, const SensorData_t *sensor)
{
    switch (id)
    {
        case FAULT_ENGINE_OVERTEMP:
            
            return (sensor->coolant_temp > FAULT_COOLANT_OVER_THRESH) &&
                   (sensor->rpm < FAULT_RPM_LOW_THRESH);

        case FAULT_OIL_OVERTEMP:
            
            return (sensor->oil_temp > FAULT_OIL_OVER_THRESH);

        case FAULT_SPEED_RPM_MISMATCH:
            
            return (sensor->vehicle_speed > FAULT_SPEED_MISMATCH_MIN) &&
                   (sensor->rpm < FAULT_RPM_STALL_THRESH);

        default:
            return false;
    }
}

// Freeze frame yakala
static void capture_freeze_frame(DTC_Entry_t *entry, const SensorData_t *sensor)
{
    entry->freeze_frame.dtc_code        = entry->code;
    entry->freeze_frame.rpm             = sensor->rpm;
    entry->freeze_frame.coolant_temp_x10 = sensor->coolant_temp * 10;
    entry->freeze_frame.oil_temp_x10    = sensor->oil_temp * 10;
    entry->freeze_frame.vehicle_speed   = sensor->vehicle_speed;
    entry->freeze_frame.timestamp_ms    = HAL_GetTick();
    entry->freeze_frame.valid           = true;
}

// MIL durumunu güncelle
static void update_mil(void)
{
    s_mil_on = false;
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        if (s_dtc_table[i].status == DTC_STATUS_CONFIRMED) {
            s_mil_on = true;
            break;
        }
    }
}

void FaultMgr_Init(void)
{
    memset(s_dtc_table, 0, sizeof(s_dtc_table));

    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        s_dtc_table[i].code   = s_fault_defs[i].dtc_code;
        s_dtc_table[i].status = DTC_STATUS_INACTIVE;
    }

    s_mil_on = false;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "FaultMgr: Init OK, %u faults defined\r\n", FAULT_COUNT);
}

void FaultMgr_Evaluate(const SensorData_t *sensor)
{
    if (sensor == NULL) return;

    uint32_t now = HAL_GetTick();

    taskENTER_CRITICAL();

    for (uint8_t i = 0; i < FAULT_COUNT; i++)
    {
        DTC_Entry_t *entry = &s_dtc_table[i];
        bool condition = fault_condition_active((FaultId_t)i, sensor);

        switch (entry->status)
        {
            case DTC_STATUS_INACTIVE:
                if (condition) {
                    entry->debounce_cnt++;
                    if (entry->debounce_cnt >= s_fault_defs[i].debounce_threshold) {
                        
                        entry->status        = DTC_STATUS_PENDING;
                        entry->pending_since = now;
                        entry->debounce_cnt  = 0;

                        capture_freeze_frame(entry, sensor);
                    }
                } else {
                    
                    entry->debounce_cnt = 0;
                }
                break;

            case DTC_STATUS_PENDING:
                if (condition) {
                    
                    uint32_t elapsed = now - entry->pending_since;
                    if (elapsed >= FAULT_CONFIRM_TIME_MS) {
                        
                        entry->status = DTC_STATUS_CONFIRMED;
                        update_mil();
                    }
                } else {
                    entry->status       = DTC_STATUS_INACTIVE;
                    entry->debounce_cnt = 0;
                }
                break;

            case DTC_STATUS_CONFIRMED:
                
                break;
        }
    }

    taskEXIT_CRITICAL();

}

void FaultMgr_ClearAllDTC(void)
{
    taskENTER_CRITICAL();
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        s_dtc_table[i].status       = DTC_STATUS_INACTIVE;
        s_dtc_table[i].debounce_cnt = 0;
        s_dtc_table[i].pending_since = 0;
        s_dtc_table[i].freeze_frame.valid = false;
    }
    s_mil_on = false;
    taskEXIT_CRITICAL();

    DEBUG_LOG(DEBUG_LEVEL_INFO, "FaultMgr: All DTCs cleared, MIL OFF\r\n");
}

bool FaultMgr_IsMilOn(void)
{
    taskENTER_CRITICAL();
    bool val = s_mil_on;
    taskEXIT_CRITICAL();
    return val;
}

uint8_t FaultMgr_GetConfirmedDTCCount(void)
{
    taskENTER_CRITICAL();
    uint8_t count = 0;
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        if (s_dtc_table[i].status == DTC_STATUS_CONFIRMED) {
            count++;
        }
    }
    taskEXIT_CRITICAL();
    return count;
}

uint8_t FaultMgr_GetPendingDTCCount(void)
{
    taskENTER_CRITICAL();
    uint8_t count = 0;
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        if (s_dtc_table[i].status == DTC_STATUS_PENDING) {
            count++;
        }
    }
    taskEXIT_CRITICAL();
    return count;
}

uint8_t FaultMgr_GetConfirmedDTCs(uint16_t *dtc_codes, uint8_t max_count)
{
    taskENTER_CRITICAL();
    uint8_t count = 0;
    for (uint8_t i = 0; i < FAULT_COUNT && count < max_count; i++) {
        if (s_dtc_table[i].status == DTC_STATUS_CONFIRMED) {
            dtc_codes[count++] = s_dtc_table[i].code;
        }
    }
    taskEXIT_CRITICAL();
    return count;
}

uint8_t FaultMgr_GetPendingDTCs(uint16_t *dtc_codes, uint8_t max_count)
{
    taskENTER_CRITICAL();
    uint8_t count = 0;
    for (uint8_t i = 0; i < FAULT_COUNT && count < max_count; i++) {
        if (s_dtc_table[i].status == DTC_STATUS_PENDING) {
            dtc_codes[count++] = s_dtc_table[i].code;
        }
    }
    taskEXIT_CRITICAL();
    return count;
}

bool FaultMgr_GetFreezeFrame(uint8_t frame_number, FreezeFrame_t *ff)
{
    if (ff == NULL) return false;

    uint8_t found = 0;
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        if (s_dtc_table[i].freeze_frame.valid) {
            if (found == frame_number) {
                *ff = s_dtc_table[i].freeze_frame;
                return true;
            }
            found++;
        }
    }

    return false;
}

uint8_t FaultMgr_GetTotalDTCCount(void)
{
    return FaultMgr_GetConfirmedDTCCount() + FaultMgr_GetPendingDTCCount();
}
