// app_tasks_scanner.c - OBD Scanner — Ana Task Orchestrator Implementasyonu

#include "app_tasks_scanner.h"
#include "main.h"
#include "scanner_buttons.h"
#include "project_config.h"
#include "mcp2515.h"
#include "mcp2515_regs.h"
#include "can_interface.h"
#include "isotp.h"
#include "obd_types.h"
#include "scanner_comm.h"
#include "scanner_ui.h"
#include "ssd1306.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>

#define SCANNER_STACK_CAN_RX        512U
#define SCANNER_STACK_COMM          512U
#define SCANNER_STACK_BUTTON        256U
#define SCANNER_STACK_UI            256U

#define SCANNER_PRIO_CAN_RX         5U    
#define SCANNER_PRIO_COMM           4U    
#define SCANNER_PRIO_BUTTON         3U    
#define SCANNER_PRIO_UI             2U    

#define SCANNER_CMD_QUEUE_DEPTH     4U

static TaskHandle_t s_hTask_CAN_Rx    = NULL;
static TaskHandle_t s_hTask_Comm      = NULL;
static TaskHandle_t s_hTask_Button    = NULL;
static TaskHandle_t s_hTask_UI        = NULL;

static UART_HandleTypeDef  *s_huart   = NULL;
static I2C_HandleTypeDef   *s_hi2c    = NULL;

static QueueHandle_t s_queueCAN_Rx    = NULL;    
static QueueHandle_t s_queueCmd       = NULL;    

static SemaphoreHandle_t s_mutexSPI   = NULL;    

static MCP2515_Handle_t s_hmcp;

static ISOTP_Session_t s_isotp_session;

static CAN_IF_Config_t s_can_if_config;

// MCP2515 donanımını Scanner modunda başlat ve konfigüre et
static bool init_mcp2515_scanner(SPI_HandleTypeDef *hspi)
{
    extern UART_HandleTypeDef huart2;
    #define MCP_TRACE(msg) HAL_UART_Transmit(&huart2, (uint8_t*)(msg), sizeof(msg)-1, HAL_MAX_DELAY)

    s_hmcp.hspi      = hspi;
    s_hmcp.cs_port   = MCP2515_CS_GPIO_Port;
    s_hmcp.cs_pin    = MCP2515_CS_Pin;
    s_hmcp.spi_mutex = s_mutexSPI;

    MCP_TRACE("  [S1] MCP2515_Init...\r\n");

    MCP2515_Status_t st = MCP2515_Init(&s_hmcp);
    if (st != MCP2515_OK) {
        MCP_TRACE("  [S2] MCP2515_Init FAILED\r\n");
        return false;
    }
    MCP_TRACE("  [S3] MCP2515_Init OK\r\n");

    if (MCP2515_ConfigBitTiming(&s_hmcp,
                                 MCP2515_CNF1_500KBPS,
                                 MCP2515_CNF2_500KBPS,
                                 MCP2515_CNF3_500KBPS) != MCP2515_OK) {
        MCP_TRACE("  [S4] BitTiming FAILED\r\n");
        return false;
    }

    if (MCP2515_EnableInterrupts(&s_hmcp,
                                  MCP2515_CANINTE_RX0IE |
                                  MCP2515_CANINTE_RX1IE) != MCP2515_OK) {
        MCP_TRACE("  [S5] CANINTE FAILED\r\n");
        return false;
    }

    if (MCP2515_ConfigFilter(&s_hmcp,
                              0x7FF, 0x7FF,
                              CAN_ID_OBD_RESPONSE,
                              CAN_ID_OBD_RESPONSE) != MCP2515_OK) {
        MCP_TRACE("  [S6] Filter config FAILED\r\n");
        return false;
    }

    if (MCP2515_SetMode(&s_hmcp, MCP2515_MODE_NORMAL_OP) != MCP2515_OK) {
        MCP_TRACE("  [S7] Normal mode FAILED\r\n");
        return false;
    }

    MCP_TRACE("  [S8] MCP2515 Scanner mode OK (filter=0x7E8)\r\n");
    #undef MCP_TRACE
    return true;
}

void AppTasks_Scanner_Init(SPI_HandleTypeDef *hspi,
                           UART_HandleTypeDef *huart,
                           I2C_HandleTypeDef  *hi2c)
{
    
    s_huart = huart;
    s_hi2c  = hi2c;

    DEBUG_LOG(DEBUG_LEVEL_INFO, "Scanner: Init starting...\r\n");

    s_mutexSPI = xSemaphoreCreateMutex();
    if (s_mutexSPI == NULL) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: SPI Mutex FAILED\r\n");
        return;
    }

    s_queueCAN_Rx = xQueueCreate(QUEUE_DEPTH_CAN_RX, sizeof(CAN_Frame_t));
    if (s_queueCAN_Rx == NULL) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: CAN RX Queue FAILED\r\n");
        return;
    }

    s_queueCmd = xQueueCreate(SCANNER_CMD_QUEUE_DEPTH, sizeof(ScannerCmd_t));
    if (s_queueCmd == NULL) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: CMD Queue FAILED\r\n");
        return;
    }

    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    if (!init_mcp2515_scanner(hspi)) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR,
                  "Scanner: CRITICAL — MCP2515 init failed! (Check Wiring)\r\n");
        
    }

    ISOTP_Init(&s_isotp_session,
               CAN_ID_OBD_FUNCTIONAL_REQ,   
               CAN_ID_OBD_RESPONSE);         

    BaseType_t ret;

    ret = xTaskCreate(CAN_IF_RxTask, "CAN_Rx", SCANNER_STACK_CAN_RX,
                      NULL, SCANNER_PRIO_CAN_RX, &s_hTask_CAN_Rx);
    if (ret != pdPASS) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: CAN_Rx task FAILED\r\n");
        return;
    }

    s_can_if_config.hmcp           = &s_hmcp;
    s_can_if_config.rx_queue       = s_queueCAN_Rx;
    s_can_if_config.rx_task_handle = s_hTask_CAN_Rx;
    CAN_IF_Init(&s_can_if_config);

    ScannerButtons_Init(s_queueCmd);

    ret = xTaskCreate(ScannerButtons_Task, "Button", SCANNER_STACK_BUTTON,
                      NULL, SCANNER_PRIO_BUTTON, &s_hTask_Button);
    if (ret != pdPASS) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: Button task FAILED\r\n");
        return;
    }

    ScannerComm_Init(&s_isotp_session, s_queueCmd);

    ret = xTaskCreate(ScannerComm_Task, "Comm", SCANNER_STACK_COMM,
                      NULL, SCANNER_PRIO_COMM, &s_hTask_Comm);
    if (ret != pdPASS) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: Comm task FAILED\r\n");
        return;
    }

    ScannerUI_Init(s_hi2c);

    ret = xTaskCreate(ScannerUI_Task, "UI", SCANNER_STACK_UI,
                      NULL, SCANNER_PRIO_UI, &s_hTask_UI);
    if (ret != pdPASS) {
        DEBUG_LOG(DEBUG_LEVEL_ERROR, "Scanner: UI task FAILED\r\n");
        return;
    }

    DEBUG_LOG(DEBUG_LEVEL_INFO,
              "Scanner: All tasks created. System Ready.\r\n");
}
