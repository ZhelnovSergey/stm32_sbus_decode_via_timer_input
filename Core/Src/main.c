#include "main.h"


// Default
void SystemClock_Config(void);
void MX_GPIO_Init(void);

// sbus.h
extern uint8_t	g_fl_convert;

int				read_sbus(uint16_t* sbus_channels);



// local
uint16_t		g_sbus_channels		[50];


int main(void)
{
	HAL_Init();
	SystemClock_Config();

	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	TIM2->PSC = 63;					// divider is [PSC+1], PSC is 7     => 8MHz / [PSC+1] = 1MHz.   One tick of TIM2 is 1us.
	TIM2->ARR = 0xffff;					// max value

	// CH1 - input

	TIM2->CCMR1  = 0;
	TIM2->CCMR2  = 0;
	TIM2->CCMR2 |= TIM_CCMR2_CC4S_0;	// Input is TI4
	TIM2->CCMR2 |= TIM_CCMR2_CC3S_1;	// Input is TI4

	TIM2->CCER   = 0;
	TIM2->CCER  |= TIM_CCER_CC3E;		// Enable input 3
	TIM2->CCER  |= TIM_CCER_CC4E;		// Enable input 4

	TIM2->CCER  &=~TIM_CCER_CC3P;		// Rising  edge
	TIM2->CCER  |= TIM_CCER_CC4P;		// Falling edge


	TIM2->DIER  |= TIM_DIER_CC3IE;		// Generate event after input 1		Falling
	TIM2->DIER  |= TIM_DIER_CC4IE;		// Generate event after input 2		Rising

	TIM2->CR1   |= TIM_CR1_CEN;		// Do not Start

	HAL_NVIC_EnableIRQ(TIM2_IRQn);

	//HAL_Delay(100);



	while(1)
	{
		if (TIM2->CNT > 600)
			if (0 == g_fl_convert)
			{
				read_sbus(g_sbus_channels);
				g_fl_convert = 1;
			}

	}
}



/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct =
	{ 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct =
	{ 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
void MX_GPIO_Init(void)
{



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
