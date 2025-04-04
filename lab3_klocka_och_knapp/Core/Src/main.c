/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "quad_sseg.h"
#include <stdio.h>
#include <string.h>

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
TIM_HandleTypeDef htim9;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM9_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define BOUNCE_DELAY_MS 20			//number of ms before the signal curve stabilizes in oscilloscope

uint16_t button_exti_count;			//number of times the interrupt/callback is run
uint16_t button_debounced_count;	//number of times the user has pressed the button

uint32_t unhandled_exti;
uint32_t current_tick_value;
uint32_t last_exti_tick;

uint32_t hours = 0;
uint32_t minutes = 0;
uint32_t seconds = 0;

/* use for testing
uint32_t hours = 23;
uint32_t minutes = 59;
uint32_t seconds = 45;
*/


uint32_t counter_1s = 0;

volatile uint8_t colon_state = 1;	//initializes the colon state to 1 (on)



int uart_get_menu_choice()
{
	char str[1] = { '\0' };
	uint16_t str_len = 1;
	HAL_UART_Receive(&huart2, (uint8_t *) str, str_len, HAL_MAX_DELAY);
	int ret = -1;
	sscanf(str, "%d", &ret);
	return ret;
}


void uart_print_menu()
{
	char menu_title[] = "Choose an option:\n\r";
	char op1[] = "1. Clock mode.\n\r";
	char op2[] = "2. Button mode.\n\r";

	HAL_UART_Transmit(&huart2, (uint8_t*)menu_title, strlen(menu_title), 2000);
	HAL_UART_Transmit(&huart2, (uint8_t*)op1, strlen(op1), 1000);
	HAL_UART_Transmit(&huart2, (uint8_t*)op2, strlen(op2), 1000);
}


void uart_print_bad_choice()
{
	char wrong_inp[] = "Wrong input\n\r";
	HAL_UART_Transmit(&huart2, (uint8_t*)wrong_inp, strlen(wrong_inp), 1000);
}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	//my clock config (prescaler = 999, period = 41999) interrupts the timer every 0.5 seconds.
	//to make the display update every 1 seconds I use the counter_1s

	if (htim->Instance == TIM9) 	//if the current timer is TIM9 then execute the code within the if block
	{
		counter_1s++;
		if (counter_1s >= 2)
		{
			counter_1s = 0;

			seconds++;
			if (seconds >= 60)
			{
				seconds = 0;
				minutes++;
			}

			if (minutes >= 60)
			{
				minutes = 0;
				hours++;
			}

			if (hours >= 24)
			{
				hours = 0;
			}
		}

		//the colon should blink every 0.5 s
		colon_state = !colon_state;

	}
}



void clock_mode()
{
	while(1)
	{
		HAL_TIM_Base_Start_IT(&htim9);

		int b1_pressed;

		uint8_t hour1 = hours / 10;
		uint8_t hour2 = hours % 10;
		uint8_t min1 = minutes / 10;
		uint8_t min2 = minutes % 10;
		uint8_t sec1 = seconds / 10;
		uint8_t sec2 = seconds % 10;



		// check b1 button (active low)
		b1_pressed = GPIO_PIN_RESET == HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);


		//if blue button is pressed then display HHMM, otherwise display MMSS
		if (b1_pressed)
		{
			qs_put_digits(hour1, hour2, min1, min2, colon_state);
		}
		else
		{
			qs_put_digits(min1, min2, sec1, sec2, colon_state);
		}
	}
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == MY_BTN_Pin)
	{
		button_exti_count++;
		unhandled_exti = 1;
		last_exti_tick = HAL_GetTick();
	}
}



void now_counter()
{
	current_tick_value = HAL_GetTick();
}



void button_mode()
{
	int b1_pressed;
	int my_btn_pressed;

	while(1)
	{
		/* deal with debouncing the button */
		if(unhandled_exti)
		{

			//now räknar varje HAL_GetTick, last_flank_causing_exti räknar varje callback när my_btn trycks.
			//BOUNCE_DELAY är antalet ms innan signalen blir stabil.
			current_tick_value = HAL_GetTick();

			if ((current_tick_value - last_exti_tick) >= BOUNCE_DELAY_MS)
			{
				my_btn_pressed = GPIO_PIN_RESET == HAL_GPIO_ReadPin(MY_BTN_GPIO_Port, MY_BTN_Pin);

				if (my_btn_pressed)
				{
					button_debounced_count++;
				}
				unhandled_exti = 0;
			}
		}

		// check b1 button (active low)
		b1_pressed = GPIO_PIN_RESET == HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);

		//if b1_pressed is true then display button_exti_count, otherwise display button_debounced_count
		qs_put_big_num(b1_pressed ? button_exti_count : button_debounced_count);
	}
}






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
  MX_USART2_UART_Init();
  MX_TIM9_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */



  while (1)
  {
	  uart_print_menu();

	  int menu_choice = uart_get_menu_choice();
	  {
		  switch (menu_choice)
		  {
		  case 1:			   clock_mode();	break;
		  case 2:			  button_mode();	break;
		  default:	uart_print_bad_choice(); 	break;
		  }
	  }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 999;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 41999;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|SEG_DIO_Pin|SEG_CLK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : B1_Pin MY_BTN_Pin */
  GPIO_InitStruct.Pin = B1_Pin|MY_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin SEG_CLK_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|SEG_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SEG_DIO_Pin */
  GPIO_InitStruct.Pin = SEG_DIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SEG_DIO_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
