// app_tasks_ecu.c - ECU Emulator — FreeRTOS Task Implementasyonları

#include "app_tasks_ecu.h"
#include "project_config.h"
#include "mcp2515.h"
#include "mcp2515_regs.h"
#include "can_interface.h"
#include "isotp.h"
#include "obd_types.h"
#include "obd_service_ecu.h"
#include "sensor_model.h"
#include "fault_manager.h"
#include "button_handler.h"
#include "ssd1306.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

static TaskHandle_t s_hTask_CAN_Rx       = NULL;
static TaskHandle_t s_hTask_Comm         = NULL;    
static TaskHandle_t s_hTask_Sensor       = NULL;
static TaskHandle_t s_hTask_Fault        = NULL;
static TaskHandle_t s_hTask_Button       = NULL;
static TaskHandle_t s_hTask_Dashboard    = NULL;

static UART_HandleTypeDef  *s_huart_dash = NULL;
static I2C_HandleTypeDef   *s_hi2c_dash  = NULL;
static bool                 s_oled_ok    = false;

static QueueHandle_t s_queueCAN_Rx       = NULL;    

static SemaphoreHandle_t s_mutexSPI      = NULL;     

static MCP2515_Handle_t s_hmcp;

static ISOTP_Session_t s_isotp_session;

static CAN_IF_Config_t s_can_if_config;

// Birleşik CAN Haberleşme Task'ı
static void Task_CAN_CommHandler(void *argument)
{
    (void)argument;

    CAN_Frame_t can_frame;
    OBD_Message_t obd_request;
    OBD_Message_t obd_response;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "Task_Comm: Started (ISOTP+OBD unified)\r\n");

    for (;;)
    {
        
        if (CAN_IF_ReceiveFrame(&can_frame, 50) == CAN_IF_OK)
        {
            
            if (can_frame.id == s_isotp_session.rx_can_id ||
                can_frame.id == CAN_ID_OBD_FUNCTIONAL_REQ)
            {
                ISOTP_Status_t status = ISOTP_ProcessRxFrame(&s_isotp_session, &can_frame);

                if (status == ISOTP_COMPLETE)
                {
                    
                    uint8_t payload[ISOTP_MAX_PAYLOAD_SIZE];
                    uint16_t payload_len;

                    if (ISOTP_GetReceivedMessage(&s_isotp_session, payload, &payload_len) == ISOTP_OK)
                    {
                        if (payload_len >= 1)
                        {
                            obd_request.service_id = payload[0];
                            obd_request.length     = payload_len - 1;
                            if (obd_request.length > 0) {
                                memcpy(obd_request.data, &payload[1], obd_request.length);
                            }

                            if (OBD_ECU_HandleRequest(&obd_request, &obd_response))
                            {
                                uint8_t resp_payload[ISOTP_MAX_PAYLOAD_SIZE];
                                uint16_t resp_len = 0;

                                resp_payload[0] = obd_response.service_id;
                                resp_len = 1;

                                if (obd_response.length > 0) {
                                    memcpy(&resp_payload[1], obd_response.data, obd_response.length);
                                    resp_len += obd_response.length;
                                }

                                ISOTP_Status_t isotp_status = ISOTP_Send(&s_isotp_session,
                                                                          resp_payload, resp_len);

                                if (isotp_status == ISOTP_OK) {
                                    DEBUG_LOG(DEBUG_LEVEL_DEBUG, "Comm: SF sent, SID=0x%02X\r\n",
                                              obd_response.service_id);
                                } else if (isotp_status == ISOTP_IN_PROGRESS) {
                                    DEBUG_LOG(DEBUG_LEVEL_DEBUG, "Comm: MF started, SID=0x%02X len=%u\r\n",
                                              obd_response.service_id, resp_len);
                                } else {
                                    DEBUG_LOG(DEBUG_LEVEL_ERROR, "Comm: ISOTP_Send failed, status=%d\r\n",
                                              isotp_status);
                                }
                            }
                        }
                    }
                }
            }
        }

        ISOTP_ProcessTx(&s_isotp_session);

        ISOTP_Tick(&s_isotp_session);
    }
}

// Sensor Model Task — periyodik sensör güncelleme
static void Task_SensorModel(void *argument)
{
    (void)argument;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    DEBUG_LOG(DEBUG_LEVEL_INFO, "Task_SensorModel: Started\r\n");

    for (;;)
    {
        SensorModel_Update();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_UPDATE_PERIOD_MS));
    }
}

// Fault Manager Task — periyodik arıza değerlendirme
static void Task_FaultManager(void *argument)
{
    (void)argument;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorData_t sensor;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "Task_FaultManager: Started\r\n");

    for (;;)
    {
        SensorModel_GetData(&sensor);
        FaultMgr_Evaluate(&sensor);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(FAULT_EVAL_PERIOD_MS));
    }
}

// MCP2515 donanımını başlat ve konfigüre et
static bool init_mcp2515(SPI_HandleTypeDef *hspi)
{
    
    extern UART_HandleTypeDef huart2;
    #define MCP_TRACE(msg) HAL_UART_Transmit(&huart2, (uint8_t*)(msg), sizeof(msg)-1, HAL_MAX_DELAY)

    s_hmcp.hspi      = hspi;
    s_hmcp.cs_port   = GPIOB;          
    s_hmcp.cs_pin    = GPIO_PIN_6;
    s_hmcp.spi_mutex = s_mutexSPI;

    MCP_TRACE("  [5a] Calling MCP2515_Init...\r\n");

    MCP2515_Status_t st = MCP2515_Init(&s_hmcp);
    if (st != MCP2515_OK) {
        MCP_TRACE("  [5c] MCP2515_Init returned error (normal if not connected)\r\n");
        return false;
    }
    MCP_TRACE("  [5d] MCP2515_Init OK\r\n");

    if (MCP2515_ConfigBitTiming(&s_hmcp,
                                 MCP2515_CNF1_500KBPS,
                                 MCP2515_CNF2_500KBPS,
                                 MCP2515_CNF3_500KBPS) != MCP2515_OK) {
        MCP_TRACE("  [5e] BitTiming FAILED\r\n");
        return false;
    }

    if (MCP2515_EnableInterrupts(&s_hmcp,
                                  MCP2515_CANINTE_RX0IE |
                                  MCP2515_CANINTE_RX1IE) != MCP2515_OK) {
        MCP_TRACE("  [5e2] CANINTE program FAILED\r\n");
        return false;
    }

    if (MCP2515_ConfigFilter(&s_hmcp,
                              0x7FF, 0x7FF,
                              CAN_ID_OBD_FUNCTIONAL_REQ,
                              CAN_ID_OBD_PHYSICAL_REQ) != MCP2515_OK) {
        MCP_TRACE("  [5f1] Filter config FAILED\r\n");
        return false;
    }

    if (MCP2515_SetMode(&s_hmcp, MCP2515_MODE_NORMAL_OP) != MCP2515_OK) {
        MCP_TRACE("  [5g] Normal mode FAILED\r\n");
        return false;
    }

    MCP_TRACE("  [5h] MCP2515 fully configured (CANINTE + filtered)\r\n");
    #undef MCP_TRACE
    return true;
}

#if ENABLE_DASHBOARD

#if ENABLE_OLED
// OLED ekranı güncelle — Gear göstergeli v3.0
static void dashboard_update_oled(const SensorData_t *sensor,
                                   bool mil_on, uint8_t dtc_conf,
                                   uint8_t dtc_pend)
{
    if (!s_oled_ok) return;

    char line[32]; 
    uint8_t gear = SensorModel_GetGear();

    SSD1306_Clear();

    SSD1306_SetCursor(0, 0);
    snprintf(line, sizeof(line), "MIL:%-3s  G:%u  DTC:%u",
             mil_on ? "ON" : "OFF",
             gear,
             dtc_conf + dtc_pend);
    SSD1306_WriteString(line, SSD1306_WHITE);

    if (mil_on) {
        SSD1306_InvertRect(0, 0, 42, 8);
    }

    SSD1306_DrawHLine(0, 9, 128, SSD1306_WHITE);

    SSD1306_SetCursor(0, 12);
    snprintf(line, sizeof(line), "  RPM:%5u  G:%u", sensor->rpm, gear);
    SSD1306_WriteString(line, SSD1306_WHITE);

    SSD1306_SetCursor(0, 22);
    snprintf(line, sizeof(line), "  SPD:  %3u km/h", sensor->vehicle_speed);
    SSD1306_WriteString(line, SSD1306_WHITE);

    SSD1306_DrawHLine(0, 33, 128, SSD1306_WHITE);

    SSD1306_SetCursor(0, 36);
    snprintf(line, sizeof(line), " CLT:%3dC  OIL:%3dC",
             sensor->coolant_temp, sensor->oil_temp);
    SSD1306_WriteString(line, SSD1306_WHITE);

    uint8_t clt_bar = (uint8_t)((sensor->coolant_temp > 0 ?
                       (sensor->coolant_temp > 120 ? 120 : sensor->coolant_temp)
                       : 0) * 60 / 120);
    SSD1306_DrawFilledRect(4, 47, clt_bar, 5, SSD1306_WHITE);
    SSD1306_DrawHLine(4, 46, 60, SSD1306_WHITE);  
    SSD1306_DrawHLine(4, 52, 60, SSD1306_WHITE);  

    uint8_t oil_bar = (uint8_t)((sensor->oil_temp > 0 ?
                       (sensor->oil_temp > 150 ? 150 : sensor->oil_temp)
                       : 0) * 60 / 150);
    SSD1306_DrawFilledRect(68, 47, oil_bar, 5, SSD1306_WHITE);
    SSD1306_DrawHLine(68, 46, 60, SSD1306_WHITE);
    SSD1306_DrawHLine(68, 52, 60, SSD1306_WHITE);

    SSD1306_SetCursor(0, 56);
    snprintf(line, sizeof(line), " %s  ENG:%s",
             SensorModel_GetProfile() == SENSOR_PROFILE_NORMAL ? "NORMAL" : "FAULT ",
             sensor->engine_running ? "RUN" : "OFF");
    SSD1306_WriteString(line, SSD1306_WHITE);

    SSD1306_UpdateScreen();
}
#else  
static void dashboard_update_oled(const SensorData_t *sensor,
                                   bool mil_on, uint8_t dtc_conf,
                                   uint8_t dtc_pend)
{
    (void)sensor; (void)mil_on; (void)dtc_conf; (void)dtc_pend;
}
#endif 

// UART telemetri ekranı bas (VT100 escape kodları ile)
static void dashboard_update_uart(const SensorData_t *sensor,
                                   bool mil_on, uint8_t dtc_conf,
                                   uint8_t dtc_pend)
{
    if (s_huart_dash == NULL) return;

    char buf[96];
    int len;
    uint8_t gear = SensorModel_GetGear();

    #define TX(s,l) HAL_UART_Transmit(s_huart_dash,(uint8_t*)(s),(l),50)
    #define TXS(s)  TX((s), sizeof(s)-1)

    static bool first_run = true;
    if (first_run) {
        TXS("\033[2J");           
        first_run = false;
    }
    TXS("\033[H");               
    TXS("\033[?25l");            

    uint32_t uptime_s  = xTaskGetTickCount() / configTICK_RATE_HZ;
    uint8_t  up_h  = uptime_s / 3600;
    uint8_t  up_m  = (uptime_s % 3600) / 60;
    uint8_t  up_ss = uptime_s % 60;

    TXS("\033[36m+==========================================+\033[0m\r\n");
    TXS("\033[36m|\033[1;37m       I N T E L L I O B D   E C U       \033[36m|\033[0m\r\n");
    TXS("\033[36m|\033[0;37m          Live Telemetry Dashboard        \033[36m|\033[0m\r\n");
    TXS("\033[36m+==========================================+\033[0m\r\n");

    TXS("\033[36m|\033[0m                                          \033[36m|\033[0m\r\n");

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  ENGINE RPM .......... \033[1;32m%5u\033[0m rpm      \033[36m|\033[0m\r\n",
        sensor->rpm);
    TX(buf, len);

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  GEAR ................ \033[1;33m    %u\033[0m          \033[36m|\033[0m\r\n",
        gear);
    TX(buf, len);

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  VEHICLE SPEED ....... \033[1;32m%5u\033[0m km/h     \033[36m|\033[0m\r\n",
        sensor->vehicle_speed);
    TX(buf, len);

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  COOLANT TEMP ........ \033[1;32m%5d\033[0m C        \033[36m|\033[0m\r\n",
        sensor->coolant_temp);
    TX(buf, len);

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  OIL TEMP ............ \033[1;32m%5d\033[0m C        \033[36m|\033[0m\r\n",
        sensor->oil_temp);
    TX(buf, len);

    TXS("\033[36m|\033[0m                                          \033[36m|\033[0m\r\n");
    TXS("\033[36m+==========================================+\033[0m\r\n");

    if (mil_on) {
        TXS("\033[36m|\033[0m  MIL STATUS .......... \033[1;31m[  ON   ]\033[0m       \033[36m|\033[0m\r\n");
    } else {
        TXS("\033[36m|\033[0m  MIL STATUS .......... \033[1;32m[  OFF  ]\033[0m       \033[36m|\033[0m\r\n");
    }

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  CONFIRMED DTC ....... \033[1;33m%5u\033[0m          \033[36m|\033[0m\r\n",
        dtc_conf);
    TX(buf, len);

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  PENDING DTC ......... \033[1;33m%5u\033[0m          \033[36m|\033[0m\r\n",
        dtc_pend);
    TX(buf, len);

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  SENSOR PROFILE ...... \033[1;37m%-6s\033[0m          \033[36m|\033[0m\r\n",
        SensorModel_GetProfile() == SENSOR_PROFILE_NORMAL ? "NORMAL" : "FAULT");
    TX(buf, len);

    TXS("\033[36m|\033[0m                                          \033[36m|\033[0m\r\n");

    len = snprintf(buf, sizeof(buf),
        "\033[36m|\033[0m  UPTIME .............. \033[0;37m%02u:%02u:%02u\033[0m          \033[36m|\033[0m\r\n",
        up_h, up_m, up_ss);
    TX(buf, len);

    TXS("\033[36m+==========================================+\033[0m\r\n");

    #undef TX
    #undef TXS
}

// Dashboard Task — Periyodik OLED + UART telemetri güncellemesi
static void Task_Dashboard(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();
    SensorData_t sensor;

#if ENABLE_OLED
    if (s_hi2c_dash != NULL) {
        s_oled_ok = SSD1306_Init(s_hi2c_dash);
        if (s_oled_ok) {
            DEBUG_LOG(DEBUG_LEVEL_INFO, "Dashboard: OLED init OK\r\n");
        } else {
            DEBUG_LOG(DEBUG_LEVEL_WARN, "Dashboard: OLED not found (UART only)\r\n");
        }
    }
#endif

    DEBUG_LOG(DEBUG_LEVEL_INFO, "Dashboard: Task started (period=%ums)\r\n",
              DASHBOARD_UPDATE_PERIOD_MS);

    for (;;)
    {
        
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DASHBOARD_UPDATE_PERIOD_MS));

        SensorModel_GetData(&sensor);
        bool    mil_on   = FaultMgr_IsMilOn();
        uint8_t dtc_conf = FaultMgr_GetConfirmedDTCCount();
        uint8_t dtc_pend = FaultMgr_GetPendingDTCCount();

        dashboard_update_oled(&sensor, mil_on, dtc_conf, dtc_pend);

        dashboard_update_uart(&sensor, mil_on, dtc_conf, dtc_pend);
    }
}

#endif 

void AppTasks_ECU_Init(SPI_HandleTypeDef *hspi,
                       UART_HandleTypeDef *huart,
                       I2C_HandleTypeDef  *hi2c)
{
    
    s_huart_dash = huart;
    s_hi2c_dash  = hi2c;

    s_mutexSPI = xSemaphoreCreateMutex();
    if (s_mutexSPI == NULL) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: Mutex create FAILED\r\n");
        return;
    }

    s_queueCAN_Rx = xQueueCreate(QUEUE_DEPTH_CAN_RX, sizeof(CAN_Frame_t));
    if (s_queueCAN_Rx == NULL) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: CAN RX Queue create FAILED\r\n");
        return;
    }

    if (!init_mcp2515(hspi)) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: CRITICAL — MCP2515 init failed! (Check Wiring)\r\n");
    }

    SensorModel_Init();
    FaultMgr_Init();

    ISOTP_Init(&s_isotp_session, CAN_ID_OBD_RESPONSE, CAN_ID_OBD_PHYSICAL_REQ);

    BaseType_t ret;

    ret = xTaskCreate(CAN_IF_RxTask, "CAN_Rx", TASK_STACK_CAN_RX,
                      NULL, TASK_PRIO_CAN_RX, &s_hTask_CAN_Rx);
    if (ret != pdPASS) { DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: CAN_Rx task FAILED\r\n"); return; }

    s_can_if_config.hmcp           = &s_hmcp;
    s_can_if_config.rx_queue       = s_queueCAN_Rx;
    s_can_if_config.rx_task_handle = s_hTask_CAN_Rx;
    CAN_IF_Init(&s_can_if_config);

    ret = xTaskCreate(Task_CAN_CommHandler, "Comm", TASK_STACK_ISOTP,
                      NULL, TASK_PRIO_ISOTP, &s_hTask_Comm);
    if (ret != pdPASS) { DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: Comm task FAILED\r\n"); return; }

    ret = xTaskCreate(Task_SensorModel, "Sensor", TASK_STACK_SENSOR,
                      NULL, TASK_PRIO_SENSOR, &s_hTask_Sensor);
    if (ret != pdPASS) { DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: Sensor task FAILED\r\n"); return; }

    ret = xTaskCreate(Task_FaultManager, "FaultMgr", TASK_STACK_FAULT,
                      NULL, TASK_PRIO_FAULT, &s_hTask_Fault);
    if (ret != pdPASS) { DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: FaultMgr task FAILED\r\n"); return; }

    ret = xTaskCreate(ButtonHandler_Task, "Button", TASK_STACK_BUTTON,
                      NULL, TASK_PRIO_BUTTON, &s_hTask_Button);
    if (ret != pdPASS) { DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: Button task FAILED\r\n"); return; }
    
    ButtonHandler_Init(s_hTask_Button);

#if ENABLE_DASHBOARD
    ret = xTaskCreate(Task_Dashboard, "Dash", TASK_STACK_OLED,
                      NULL, TASK_PRIO_OLED, &s_hTask_Dashboard);
    if (ret != pdPASS) { DEBUG_LOG(DEBUG_LEVEL_ERROR, "ECU: Dashboard task FAILED\r\n"); return; }
#endif

    DEBUG_LOG(DEBUG_LEVEL_INFO, "ECU: All tasks created successfully. System Ready.\r\n");
}

