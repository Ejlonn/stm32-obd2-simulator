// scanner_comm.c - OBD Scanner — Haberleşme Motoru Implementasyonu (Paket 3)

#include "scanner_comm.h"
#include "can_interface.h"
#include "obd_types.h"
#include <string.h>

static ISOTP_Session_t      *s_session = NULL;
static QueueHandle_t         s_cmd_q   = NULL;
static ScannerComm_Result_t  s_result;

static const uint8_t s_live_pids[SCOMM_LIVE_PID_COUNT] = {
    OBD_PID_ENGINE_RPM,         
    OBD_PID_VEHICLE_SPEED,      
    OBD_PID_COOLANT_TEMP,       
    OBD_PID_OIL_TEMPERATURE,    
    OBD_PID_MONITOR_STATUS      
};

// CAN RX queue'daki eski frame'leri temizle
static void drain_rx_queue(void)
{
    CAN_Frame_t dummy;
    while (CAN_IF_ReceiveFrame(&dummy, 0) == CAN_IF_OK) {
        
    }
}

// Tek bir OBD isteği gönder ve yanıt bekle
static ScannerComm_Status_t send_and_wait(const uint8_t *req,
                                          uint16_t       req_len,
                                          ScannerComm_RawResponse_t *resp)
{
    resp->valid  = false;
    resp->length = 0;

    ISOTP_Reset(s_session);
    drain_rx_queue();

    ISOTP_Status_t st = ISOTP_Send(s_session, req, req_len);
    if (st != ISOTP_OK && st != ISOTP_IN_PROGRESS) {
        DEBUG_LOG(DEBUG_LEVEL_WARN, "SComm: ISOTP_Send fail=%d\r\n", st);
        return SCOMM_STATUS_ERROR;
    }

    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(SCOMM_REQUEST_TIMEOUT_MS);

    for (;;)
    {
        if (xTaskGetTickCount() >= deadline) {
            DEBUG_LOG(DEBUG_LEVEL_WARN, "SComm: Timeout\r\n");
            return SCOMM_STATUS_TIMEOUT;
        }

        CAN_Frame_t frame;
        if (CAN_IF_ReceiveFrame(&frame, SCOMM_RX_POLL_MS) == CAN_IF_OK)
        {
            st = ISOTP_ProcessRxFrame(s_session, &frame);

            if (st == ISOTP_COMPLETE) {
                
                st = ISOTP_GetReceivedMessage(s_session, resp->data, &resp->length);
                if (st == ISOTP_OK) {
                    resp->valid = true;
                    return SCOMM_STATUS_OK;
                }
                return SCOMM_STATUS_ERROR;
            }
        }

        ISOTP_ProcessTx(s_session);

        if (ISOTP_Tick(s_session) == ISOTP_ERR_TIMEOUT) {
            return SCOMM_STATUS_TIMEOUT;
        }
    }
}

// Live Data döngüsü — 5 PID'i sırayla sorgula
static void execute_live_data(void)
{
    s_result.cmd    = SCANNER_CMD_LIVE_DATA;
    s_result.status = SCOMM_STATUS_OK;
    s_result.ready  = false;

    uint8_t req[2];

    for (uint8_t i = 0; i < SCOMM_LIVE_PID_COUNT; i++)
    {
        req[0] = OBD_SERVICE_01_LIVE_DATA;
        req[1] = s_live_pids[i];

        ScannerComm_Status_t st = send_and_wait(req, 2, &s_result.live[i]);

        if (st != SCOMM_STATUS_OK) {
            s_result.status = st;
            DEBUG_LOG(DEBUG_LEVEL_WARN, "SComm: PID 0x%02X fail\r\n",
                      s_live_pids[i]);
            
        }
    }

    s_result.ready = true;
}

// Tek istek komutu yürüt (DTC Read, DTC Clear, VIN)
static void execute_single(ScannerCmd_t cmd,
                            const uint8_t *req,
                            uint16_t req_len)
{
    s_result.cmd   = cmd;
    s_result.ready = false;

    s_result.status = send_and_wait(req, req_len, &s_result.single);
    s_result.ready  = true;
}

void ScannerComm_Init(ISOTP_Session_t *session, QueueHandle_t cmd_queue)
{
    s_session = session;
    s_cmd_q   = cmd_queue;
    memset(&s_result, 0, sizeof(s_result));
}

void ScannerComm_Task(void *argument)
{
    (void)argument;

    ScannerCmd_t cmd;
    bool live_polling = false;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "ScannerComm: Task started\r\n");

    for (;;)
    {
        
        TickType_t wait = live_polling ? pdMS_TO_TICKS(50) : portMAX_DELAY;

        if (xQueueReceive(s_cmd_q, &cmd, wait) == pdTRUE)
        {
            
            live_polling = (cmd == SCANNER_CMD_LIVE_DATA);
        }
        else if (!live_polling)
        {
            
            continue;
        }

        switch (cmd)
        {
            case SCANNER_CMD_LIVE_DATA:
                execute_live_data();
                break;

            case SCANNER_CMD_READ_DTC:
            {
                uint8_t req[] = { OBD_SERVICE_03_STORED_DTC };
                execute_single(cmd, req, sizeof(req));
                live_polling = false;
                break;
            }

            case SCANNER_CMD_CLEAR_DTC:
            {
                uint8_t req[] = { OBD_SERVICE_04_CLEAR_DTC };
                execute_single(cmd, req, sizeof(req));
                live_polling = false;
                break;
            }

            case SCANNER_CMD_READ_VIN:
            {
                uint8_t req[] = { OBD_SERVICE_09_VEHICLE_INFO, OBD_INFOTYPE_VIN };
                execute_single(cmd, req, sizeof(req));
                live_polling = false;
                break;
            }

            default:
                DEBUG_LOG(DEBUG_LEVEL_WARN, "SComm: Unknown cmd=%u\r\n",
                          (unsigned)cmd);
                live_polling = false;
                break;
        }
    }
}

const ScannerComm_Result_t* ScannerComm_GetResult(void)
{
    return &s_result;
}
