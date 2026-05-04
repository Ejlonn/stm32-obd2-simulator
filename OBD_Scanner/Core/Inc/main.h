/* USER CODE BEGIN Header */
// : - : Header for main.c file.
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* USER CODE BEGIN EM */

/* USER CODE END EM */

void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define BTN_K1_Pin GPIO_PIN_0
#define BTN_K1_GPIO_Port GPIOC
#define BTN_K2_Pin GPIO_PIN_1
#define BTN_K2_GPIO_Port GPIOC
#define BTN_K3_Pin GPIO_PIN_2
#define BTN_K3_GPIO_Port GPIOC
#define BTN_K4_Pin GPIO_PIN_3
#define BTN_K4_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define MCP2515_INT_Pin GPIO_PIN_0
#define MCP2515_INT_GPIO_Port GPIOB
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define MCP2515_CS_Pin GPIO_PIN_6
#define MCP2515_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif 
