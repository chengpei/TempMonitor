/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "ds18b20.h"
#include "esp01s.h"
#include "string.h"
#include "oled.h"
#include <stdio.h>
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void rom_to_hex_str(const uint8_t rom[8], char hexRom[8 * 2 + 1]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t uart_rx_byte;
char tcp_tx_buf[64];
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  DS18B20_Init();
  HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
  // ESP01S_Init(&huart1,
  //             "WIFI-3",
  //             "dsJChangxin.",
  //             "192.168.3.141",
  //             37775);
  OLED_Init();
  OLED_Clear();
  OLED_ShowBitmap16x16(0, 0, zhongwen16x16[0]);
  OLED_ShowBitmap16x16(16, 0, zhongwen16x16[2]);
  OLED_ShowBitmap16x16(72, 0, zhongwen16x16[1]);
  OLED_ShowBitmap16x16(88, 0, zhongwen16x16[2]);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
  int16_t temperature;
  char oled_buf[16];
  // 读取多个探头的ROM保存起来，28FF8833B5160350/28FF7CFA611603C4
  uint8_t roms[MAX_DEVICES][8] = {
    {0x28, 0xFF, 0x88, 0x33, 0xB5, 0x16, 0x03, 0x50}, {0x28, 0xFF, 0x7C, 0xFA, 0x61, 0x16, 0x03, 0xC4}
  };
  uint8_t count = MAX_DEVICES;
  char hexRom[8 * 2 + 1];
  // if(DS18B20_SearchROM(roms, &count))
  // {
    for (uint8_t i = 0; i < count; i++)
    {
      DS18B20_Config12Bit_ByROM(roms[i]);
      rom_to_hex_str(roms[i], hexRom);
      OLED_ShowString(0, i*2+4, hexRom);
    }
  // }
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    for (uint8_t i = 0; i < count; i++) {
      if (DS18B20_ReadTemperatureByROM(roms[i], &temperature))
      {
        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        // HAL_Delay(20);
        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        // 组包发到串口：temp|xx\r\n
        int len = snprintf(tcp_tx_buf,
                           sizeof(tcp_tx_buf),
                           "temp%d|%d\r\n", i+1,
                           temperature);
        ESP01S_SendData((uint8_t *)tcp_tx_buf, len);

        int16_t t_int  = temperature / 100;        // 整数位
        int16_t t_frac = temperature % 100;        // 小数位
        snprintf(oled_buf, sizeof(oled_buf), "%d.%02d", t_int, t_frac);
        OLED_ShowString(i * 72, 2, oled_buf);
        OLED_ShowBitmap16x16(i * 72 + 40, 2, zhongwen16x16[3]);
      }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    ESP01S_RxHandler(uart_rx_byte);
    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
  }
}

void rom_to_hex_str(const uint8_t rom[8], char hexRom[8 * 2 + 1]) {
  for(int i = 0; i < 8; i++) {
    sprintf(&hexRom[i*2], "%02X", rom[i]);
  }
  hexRom[16] = '\0'; // 末尾加 '\0'
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
