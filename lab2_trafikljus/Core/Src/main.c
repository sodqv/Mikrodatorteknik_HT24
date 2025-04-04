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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int is_button_pressed()
{
	uint32_t register_reading = GPIOC->IDR;
	uint16_t		b1_pin_nbr	= 13;
	uint16_t		b1_pin		= 0x01 << b1_pin_nbr;

	return (register_reading & b1_pin) == 0;
}


enum event
{
	ev_none,
	ev_button_push,
	ev_state_timeout,
	ev_error = -99
};


#define EVQ_SIZE 10			//evq = event queue

enum event evq[EVQ_SIZE];	//declares an array evq of type enum event. The array can hold EVQ_SIZE elements.

int evq_count = 0;
int evq_front_ix = 0;		//front index of the queue
int evq_rear_ix = 0;		//rear index of the queue


enum state
{
	s_init,
	s_car_go,
	s_pushed_wait,
	s_cars_stopping,
	s_cars_stand_still,
	s_walk,
	s_stop_walking,
	s_cars_yellow
};


uint32_t ticks_left_in_state;

void my_systick_handler()
{
	 if (ticks_left_in_state > 0)
	 {
		 ticks_left_in_state--;

		 if (ticks_left_in_state == 0)
		 {
			 evq_push_back(ev_state_timeout);
		 }
	 }
}



//function that fills the queue with ev_error
void evq_init()
{
	for (int i = 0; i < EVQ_SIZE; i++)
	{
		evq[i] = ev_error;
	}
}



void evq_push_back(enum event e)
{
	if (evq_count < EVQ_SIZE)
	{
		evq[evq_rear_ix] = e;
		evq_rear_ix = (evq_rear_ix + 1) % EVQ_SIZE;
		evq_count++;
	}
}


enum event evq_pop_front()
{
	enum event e = ev_none;

	if (evq_count > 0)
	{
		e = evq[evq_front_ix];
		evq_front_ix = (evq_front_ix + 1) % EVQ_SIZE;
		evq_count--;
	}
	return e;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == B1_Pin)
	{
		evq_push_back(ev_button_push);
	}
}



void set_traffic_lights(enum state s)
{
	switch(s) {
		case s_init:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_SET);   //1

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_SET); 	 //1
			break;


		case s_car_go:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_SET); 	 //1

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_RESET); //0
			break;


		case s_pushed_wait:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_SET);   //1

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET); 	 //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_RESET); //0
			break;


		case s_cars_stopping:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_RESET); //0

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_RESET); //0
			break;


		case s_cars_stand_still:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_RESET); //0

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_RESET); //0
			break;


		case s_walk:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_RESET); //0

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_SET);   //1
			break;


		case s_stop_walking:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_RESET); //0
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_RESET); //0

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_RESET); //0
			break;


		case s_cars_yellow:
			HAL_GPIO_WritePin(GPIOA, A_Pin, GPIO_PIN_SET); 	 //1
			HAL_GPIO_WritePin(GPIOA, B_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, C_Pin, GPIO_PIN_RESET); //0

			HAL_GPIO_WritePin(GPIOA, D_Pin, GPIO_PIN_SET);   //1
			HAL_GPIO_WritePin(GPIOA, E_Pin, GPIO_PIN_RESET); //0
			break;
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  enum state st = s_init;
  enum event ev = ev_none;




  evq_init();
  set_traffic_lights(st);


  while (1)
  {

	 ev = evq_pop_front();

	  switch(st) {
	  		case s_init:
	  			if(ev == ev_button_push)
	  			{
					ticks_left_in_state = 2500;
					st = s_cars_stand_still;
					set_traffic_lights(st);
	  			}
	  			break;


	  		case s_cars_stand_still:
	  			if(ev == ev_state_timeout)
	  			{
	  				ticks_left_in_state = 2500;
	  				st = s_walk;
	  				set_traffic_lights(st);
	  			}
	  			break;


	  		case s_walk:
	  			if(ev == ev_state_timeout)
	  			{
	  				ticks_left_in_state = 1000;
	  				st = s_stop_walking;
	  				set_traffic_lights(st);
	  			}
	  			break;


	  		case s_stop_walking:
	  			if(ev == ev_state_timeout)
	  			{
	  				ticks_left_in_state = 1000;
	  				st = s_cars_yellow;
	  				set_traffic_lights(st);
	  			}
	  			break;


	  		case s_cars_yellow:
	  			if(ev == ev_state_timeout)
	  			{
	  				ticks_left_in_state = 0;
	  				st = s_car_go;
	  				set_traffic_lights(st);
	  			}
	  			break;


	  		case s_car_go:
	  			if(ev == ev_button_push)
	  			{
	  				ticks_left_in_state = 2000;
	  				st = s_pushed_wait;
	  				set_traffic_lights(st);
	  			}
	  			break;


	  		case s_pushed_wait:
	  			if(ev == ev_state_timeout)
	  			{
	  				ticks_left_in_state = 1000;
	  				st = s_cars_stopping;
	  				set_traffic_lights(st);
	  			}
	  			break;


	  		case s_cars_stopping:
	  			if(ev == ev_state_timeout)
	  			{
	  				ticks_left_in_state = 1000; 				// set next timeout
	  				st = s_cars_stand_still;					// set next state
	  				set_traffic_lights(s_cars_stand_still); 	// set output
	  			}
	  			break;

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
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|A_Pin|B_Pin|C_Pin
                          |D_Pin|E_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin A_Pin B_Pin C_Pin
                           D_Pin E_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|A_Pin|B_Pin|C_Pin
                          |D_Pin|E_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
