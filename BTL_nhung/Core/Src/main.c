/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f4xx_hal.h"
#include "OLED_SSD1306.c"
#include "string.h"
#include "math.h"

#define STATE_STOPPED  0


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


void init_Switch(void);
void init_led(void);
void init_buzzer(void);


void init_ADC_MQ2(void);
void init_timer1_led_RGB(void);
void init_relay(void);

void RGB_update(int R, int G, int B, int FREQ);

//void init_UART_ESP32(void);
//void send_ESP32(char *str);



int main(void)
{
  SystemClock_Config();

  init_Switch();
  init_led();
  init_buzzer();
//  init_I2C_oled();

  init_ADC_MQ2();
  init_timer1_led_RGB();
  init_relay();

  ssd1306_init();

  while (1)

  {
    /* USER CODE END WHILE */
	 ADC1->CR2 |= ADC_CR2_SWSTART;         // Bắt đầu chuyển đổi
	  HAL_Delay(1000);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * SW1: khai báo PA1 (stop-start) input (interrupt)
 * SW2: khai báo PA2 (reset) input
 */
void init_Switch(void){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
}

void init_led(void){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
}

void init_buzzer(void){
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
}


void init_ADC_MQ2(void)
{
	// Cấu hình GPIOA chế độ analog để đọc ADC
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  	// Bật clock GPIOA
	GPIOA->MODER &= ~(3 << (0 * 2));  		// Xóa cấu hình cũ của PA0
	GPIOA->MODER  |= (3 << (0 * 2));  		// Chọn chế độ analog cho PA0 (MODER00 = 11)

	// Cấu hình bộ ADC
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;     // Bật clock cho ADC1

	ADC1->CR1 &= ~ADC_CR1_RES_Msk;        	// Xóa cấu hình độ phân giải cũ
	ADC1->CR1 |= 2 << ADC_CR1_RES_Pos;    	// Chọn độ phân giải 8 bit (10 = 8-bit resolution)
	ADC1->CR1 |= ADC_CR1_EOCIE;				// Bật ngắt khi ADC chuyển đổi hoàn tất (EOC)

	ADC1->SMPR2 |= (3 << (0 * 3));          // Cấu hình thời gian lấy mẫu cho kênh 0: 56 cycles

	ADC1->SQR3 &= ~(0xF << 0);              // Chọn kênh ADC = kênh 0 (PA0)

	ADC1->CR2 |= ADC_CR2_ADON;            	// Bật ADC1 (Enable ADC)

	NVIC_EnableIRQ(ADC_IRQn); 				// Bật ngắt ADC trong NVIC
	NVIC_SetPriority(ADC_IRQn, 1); 			// Ưu tiên mức 1 cho ngắt ADC
}

int ppm_caculator(uint16_t data)
{
    // Tính điện áp từ giá trị ADC đọc được (độ phân giải 8-bit → 256 mức)
    double voltage = 3.3 * data / 256.0;
    // Tính điện trở Rs theo điện áp
    double Rs = 1 * ((3.3 - voltage) / voltage);
    // Tỷ số Rs/Ro (Ro = 10 ohm, giả sử)
    double divRsRo = Rs / 10;
    // Tính log10(ppm) từ đường cong đặc trưng
    float log_ppm = -0.45 * log10f(divRsRo) + 1.25;
    // Tính ppm bằng cách mũ hóa cơ số 10
    return (int) powf(10.0, log_ppm);
}

void ADC_IRQHandler(void)
{
	uint16_t adc_value;
	uint16_t ppm_value;
	if (ADC1->SR & (1 << 1)) 	// Kiểm tra cờ EOC (End Of Conversion)
	{
	    adc_value = ADC1->DR;   			// Đọc giá trị ADC
	    ppm_value = ppm_caculator(adc_value); 	// Tính toán ppm từ giá trị ADC

	    // Cập nhật hành vi theo ppm
	    if(ppm_value < 20)
	    {




	    	// bật led xanh dương khi không có khí gar .
	    	RGB_update(0, 0, 1, 0);
	    }

	    if(ppm_value >= 20 && ppm_value <150)
	    {



	    	// bật ledvàng khi khí gar thấp.
	    	RGB_update(1, 1, 0, 0);

	    }

	    if(ppm_value >= 150 && ppm_value < 400)
	    {



	    	// bật led đỏ nháy 1Hz.
	    	RGB_update(1, 0, 0, 1);

	    	// kích hoạt relay
	    	GPIOB->ODR |= 1 << 12;
	    }

	    if(ppm_value >= 400)
	    {



	    	// bật led đỏ nháy 5Hz.
	    	RGB_update(1, 0, 0, 5);

	    	// kích hoạt relay
	    	GPIOB->ODR |= 1 << 12;
	    }

	    ADC1->SR &= ~(1 << 1);  			// Xóa cờ EOC
	}
}

void init_timer1_led_RGB(void)
{
    // 1. Bật clock GPIOA và TIM1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // 2. Cấu hình các chân PA8, PA9, PA10 làm Alternate Function (AF1 tương ứng với TIM1)
    GPIOA->MODER &= ~( (3 << (8 * 2)) | (3 << (9 * 2)) | (3 << (10 * 2)) );  // Xóa cấu hình cũ
    GPIOA->MODER |=  (2 << (8 * 2)) | (2 << (9 * 2)) | (2 << (10 * 2));      // Đặt chế độ AF

    GPIOA->AFR[1] &= ~((0xF << ((8 - 8) * 4)) | (0xF << ((9 - 8) * 4)) | (0xF << ((10 - 8) * 4)));
    GPIOA->AFR[1] |=  (1 << ((8 - 8) * 4)) | (1 << ((9 - 8) * 4)) | (1 << ((10 - 8) * 4));  // AF1

    // 3. Cấu hình Timer1
    TIM1->PSC = 79999;        // Prescaler: giảm từ 80 MHz xuống 1 kHz
    TIM1->ARR = 999;          // Auto-reload: 1s chu kỳ (1 tick = 1ms)

    // 4. Đặt giá trị so sánh ban đầu cho 3 kênh PWM (CCR)
    TIM1->CCR1 = 0;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = 0;

    // 5. Cấu hình chế độ PWM cho các kênh (OC1, OC2, OC3)
    TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M);  // Xóa mode cũ
    TIM1->CCMR2 &= ~(TIM_CCMR2_OC3M);                   // Xóa mode cũ

    TIM1->CCMR1 |= (0b110 << TIM_CCMR1_OC1M_Pos);  // PWM mode 1 cho OC1
    TIM1->CCMR1 |= (0b110 << TIM_CCMR1_OC2M_Pos);  // PWM mode 1 cho OC2
    TIM1->CCMR2 |= (0b110 << TIM_CCMR2_OC3M_Pos);  // PWM mode 1 cho OC3

    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E; // Cho phép xuất tín hiệu trên CH1, CH2, CH3

    // 6. Cho phép output (Main Output Enable)
    TIM1->BDTR |= TIM_BDTR_MOE;

    // 7. Bắt đầu Timer
    TIM1->CR1 |= TIM_CR1_CEN;
}

void RGB_update(int R, int G, int B, int FREQ)
{
	TIM1->CR1 &= ~TIM_CR1_CEN;  // Dừng Timer để cập nhật

	if(FREQ == 0)  // Nếu không nhấp nháy
	{
        TIM1->ARR = 999;  // Chu kỳ 1 giây
        // Cập nhật duty cycle 100% hoặc 0% tùy màu
        TIM1->CCR1 = (R == 0) ? 0 : 999;
        TIM1->CCR2 = (G == 0) ? 0 : 999;
        TIM1->CCR3 = (B == 0) ? 0 : 999;
	}
	else  // Nếu có nhấp nháy
	{
		int arr_val = 1000 / FREQ - 1;       	// Tính chu kỳ mới theo tần số mong muốn
		int duty = arr_val / 2;				// Duty cycle 50%

		TIM1->ARR = arr_val;				// Cập nhật chu kỳ mới
		TIM1->CCR1 = (R == 0) ? 0 : duty;
		TIM1->CCR2 = (G == 0) ? 0 : duty;
		TIM1->CCR3 = (B == 0) ? 0 : duty;
	}

	TIM1->EGR |= TIM_EGR_UG;  				// Tạo sự kiện cập nhật
	TIM1->CR1 |= TIM_CR1_CEN; 				// Khởi động lại Timer
}


void init_relay(void) {
    // 1. Bật clock cho port B
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    // 2. Cấu hình PB12 là output (MODER = 01)
    GPIOB->MODER &= ~(3 << (12 * 2));   // Clear 2 bit MODER12
    GPIOB->MODER |=  (1 << (12 * 2));   // Set bit 12 thành 01 (output)
    // 3. Output type: push-pull (OTYPER = 0)
    GPIOB->OTYPER &= ~(1 << 12);        // Push-pull
    // 5. No pull-up, no pull-down (PUPDR = 00)
    GPIOB->PUPDR &= ~(3 << (12 * 2));
    // 6. Đặt mức logic ban đầu (LOW): relay off
    GPIOB->ODR &= ~(1 << 12);
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
