// project_config.h - IntelliOBD Simulator — Proje geneli sabitler ve konfigürasyon

#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define PROJECT_NAME                "IntelliOBD_Simulator"
#define PROJECT_VERSION_MAJOR       1U
#define PROJECT_VERSION_MINOR       0U
#define PROJECT_VERSION_PATCH       0U

#define CAN_ID_OBD_FUNCTIONAL_REQ   0x7DFU  
#define CAN_ID_OBD_PHYSICAL_REQ     0x7E0U  
#define CAN_ID_OBD_RESPONSE         0x7E8U  

#define CAN_BITRATE_KBPS            500U

#define MCP2515_OSC_FREQ_HZ         8000000UL

#define MCP2515_CNF1_500KBPS        0x00U
#define MCP2515_CNF2_500KBPS        0x91U
#define MCP2515_CNF3_500KBPS        0x01U

#define ISOTP_MAX_PAYLOAD_SIZE      64U

#define ISOTP_TIMEOUT_MS            1000U

#define ISOTP_FC_BLOCK_SIZE         0U      
#define ISOTP_FC_STMIN_MS           0U      

#define ISOTP_CAN_DL               8U

#define OBD_SERVICE_01_LIVE_DATA        0x01U   
#define OBD_SERVICE_02_FREEZE_FRAME     0x02U   
#define OBD_SERVICE_03_STORED_DTC       0x03U   
#define OBD_SERVICE_04_CLEAR_DTC        0x04U   
#define OBD_SERVICE_07_PENDING_DTC      0x07U   
#define OBD_SERVICE_09_VEHICLE_INFO     0x09U   

#define OBD_RESPONSE_OFFSET             0x40U

#define OBD_NRC_SERVICE_NOT_SUPPORTED       0x11U
#define OBD_NRC_SUBFUNCTION_NOT_SUPPORTED   0x12U
#define OBD_NRC_GENERAL_REJECT              0x10U

#define OBD_NEGATIVE_RESPONSE_SID       0x7FU

#define TASK_STACK_CAN_RX           512U
#define TASK_STACK_ISOTP            512U
#define TASK_STACK_OBD              512U
#define TASK_STACK_SENSOR           256U
#define TASK_STACK_FAULT            256U
#define TASK_STACK_BUTTON           256U
#define TASK_STACK_OLED             256U
#define TASK_STACK_LOGGER           256U

#define TASK_PRIO_CAN_RX            5U    
#define TASK_PRIO_ISOTP             4U    
#define TASK_PRIO_OBD               4U    
#define TASK_PRIO_SENSOR            3U    
#define TASK_PRIO_FAULT             3U    
#define TASK_PRIO_BUTTON            2U    
#define TASK_PRIO_OLED              2U    
#define TASK_PRIO_LOGGER            2U    

#define QUEUE_DEPTH_CAN_RX          8U
#define QUEUE_DEPTH_OBD_MSG         4U

#define DASHBOARD_UPDATE_PERIOD_MS  500U

#ifndef ENABLE_DASHBOARD
#define ENABLE_DASHBOARD            1
#endif
#ifndef ENABLE_OLED
#define ENABLE_OLED                 1
#endif

#ifndef ENABLE_CSV_LOGGING
#define ENABLE_CSV_LOGGING          1
#endif

#define SENSOR_UPDATE_PERIOD_MS     100U

#define SENSOR_RPM_NORMAL_MIN       800U
#define SENSOR_RPM_NORMAL_MAX       3500U
#define SENSOR_COOLANT_NORMAL_C     90      
#define SENSOR_OIL_NORMAL_C         95      
#define SENSOR_SPEED_NORMAL_MAX     120U    

#define FAULT_EVAL_PERIOD_MS        200U

#define FAULT_DEBOUNCE_THRESHOLD    5U

#define FAULT_CONFIRM_TIME_MS       5000U

#define DTC_MAX_COUNT               8U

#define FREEZE_FRAME_MAX_COUNT      4U

#define FAULT_COOLANT_OVER_THRESH   110     
#define FAULT_RPM_LOW_THRESH        700U    

#define FAULT_OIL_OVER_THRESH       135     

#define FAULT_SPEED_MISMATCH_MIN    20U     
#define FAULT_RPM_STALL_THRESH      500U    

#define LOGGER_PERIOD_MS            1000U

#define DEBUG_LEVEL_NONE    0U
#define DEBUG_LEVEL_ERROR   1U
#define DEBUG_LEVEL_WARN    2U
#define DEBUG_LEVEL_INFO    3U
#define DEBUG_LEVEL_DEBUG   4U

#define DEBUG_ACTIVE_LEVEL  DEBUG_LEVEL_INFO

#ifndef ENABLE_DEBUG_LOG
#define ENABLE_DEBUG_LOG  1
#endif

#if (ENABLE_DEBUG_LOG && DEBUG_ACTIVE_LEVEL > DEBUG_LEVEL_NONE)
  #include <stdio.h>
  #include "stm32f4xx_hal.h"
  #define DEBUG_LOG(level, fmt, ...) \
      do { \
          if ((level) <= DEBUG_ACTIVE_LEVEL) { \
              printf("[%lu] " fmt, (unsigned long)HAL_GetTick(), ##__VA_ARGS__); \
          } \
      } while (0)
#else
  #define DEBUG_LOG(level, fmt, ...)  ((void)0)
#endif

#define SIM_VIN_STRING              "WBA3A5C55FK123456"
#define SIM_VIN_LENGTH              17U

#ifdef __cplusplus
}
#endif

#endif 
