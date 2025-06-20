#include "stm32f4xx_hal.h"
#include "stdio.h"
#include "math.h"
#include "string.h"
#include "LCD.h"

int low_gar_value = 50;					//mức gar thấp > 50
int high_gar_value = 150;					// mức gar cao > 150
int warning_gar_value = 300;				// mức gar trên ngưỡng nguy hiểm > 300

#define system_on 1
#define system_off 0

int sys_state = system_on;
//int sys_flag = 1;

int warning_state = 0;
int gar_flag = 0;

int ppm_value = 0;
uint16_t adc_value = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Error_Handler(void);

void OnOffSwitch_Init(void);					// khởi tạo button 1 : on - off.
void ResetSwitch_Init(void);					// khởi tạo button 2 : reset.

void init_timer1_led_RGB(void);                 // khởi tạo timer pwm cho RGB.
void RGB_update(int R, int G, int B, int FREQ); // cập nhật màu sắc và tần số nhấp nháy.

void init_ADC_MQ2(void);					// khởi tạo ADC đọc cảm biến  mq2.
void init_relay(void);							// khởi tạo relay.
void init_buzzer(void);							// khởi tạo buzzer.

/**
 * hàm cấu hình switch 1 : on - off
 * cấu hình pin PA1 : mode input
 * điện trở pull-up ngoài.
 * cấu hình ngắt sườn xuống.
 */
void OnOffSwitch_Init(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	GPIOA->MODER &= ~(3 << (1 * 2));           // Input mode.
	GPIOA->PUPDR &= ~(3 << (1 * 2));           // Clear.

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;		// clock cho ngắt.

	SYSCFG->EXTICR[0] &= ~(0xF << (1 * 4));     // EXTI1 -> PA1.
	EXTI->IMR |= (1 << 1);						// Unmask EXTI1.
	EXTI->FTSR |= (1 << 1);           			// ngắt sườn xuống.
	EXTI->RTSR &= ~(1 << 1);					// không ngắt sườn lên.

	NVIC_EnableIRQ(EXTI1_IRQn);				// Enable EXTI1 interrupt in NVIC.
}

/**
 * hàm cấu hình switch 2 : reset
 * cấu hình pin PA2 : mode input
 * điện trở pull-up ngoài.
 * cấu hình ngắt sườn xuống.
 */
void ResetSwitch_Init(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	GPIOA->MODER &= ~(3 << (2 * 2));           // Input mode.
	GPIOA->PUPDR &= ~(3 << (2 * 2));           // Clear.

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;		// clock cho ngắt.
	SYSCFG->EXTICR[0] &= ~(0xF << (2 * 4));     // EXTI2 -> PA2

	EXTI->IMR |= (1 << 2);						// Unmask EXTI2
	EXTI->FTSR |= (1 << 2);					// ngắt sườn xuống
	EXTI->RTSR &= ~(1 << 2);

	NVIC_EnableIRQ(EXTI2_IRQn);				// Enable EXTI2 interrupt in NVIC
}

/**
 * hàm bật tắt hệ thống.
 * state == 1 : system_on.
 */
void system_on_off(int state) {
	if (state) {
		ADC1->CR2 |= ADC_CR2_ADON;        // bật adc1.
		ADC1->CR2 |= ADC_CR2_SWSTART;
		TIM1->CR1 |= TIM_CR1_CEN;		  // bật timer cho rgb.

		RGB_update(0, 0, 1, 0);		 //set rgb
		sys_state = system_on;

		lcd_gotoxy(0, 0);
		lcd_puts("system: on ");
	} else {
		RGB_update(0, 1, 0, 0);
		sys_state = system_off;

		ADC1->CR2 &= ~ADC_CR2_ADON;  // tắt adc
		TIM1->CR1 &= ~TIM_CR1_CEN;	 // tắt timmer.

		GPIOB->ODR &= ~(1 << 12);	 // tắt relay
		GPIOB->ODR |= (1 << 0);	 // tắt buzzer

		lcd_gotoxy(0, 0);
		lcd_puts("system: off     ");
		lcd_gotoxy(0, 1);
		lcd_puts("gar: -  ppm: ---");
	}

}

/**
 *hàm reset hệ thống.
 *xóa trạng thái hiện tại của ADC và timer đồng thời khởi tạo lại.
 *khởi tạo lại LCD.
 */
void system_reset(void) {
	system_on_off(system_off);

	//NVIC_DisableIRQ(EXTI1_IRQn);  // Vô hiệu hóa switch 1
	NVIC_DisableIRQ(EXTI2_IRQn);  // Vô hiệu hóa switch 2
	NVIC_DisableIRQ(ADC_IRQn);

	warning_state = 0;
	ppm_value = 0;
	adc_value = 0;

	ADC1->SR = 0;
	init_ADC_MQ2();

	TIM1->SR = 0;
	init_timer1_led_RGB();

	(void)I2C1->DR;      //đọc để rest cờ nếu có.

	lcd_init();

	lcd_gotoxy(0, 0);
	lcd_puts("system: rst     ");
	lcd_gotoxy(0, 1);
	lcd_puts("gas: 0  ppm: 0  ");

	while (!(GPIOA->IDR >> 2 & 0x1))// chờ tới khi nút nhấn được nhả
	NVIC_EnableIRQ(EXTI1_IRQn);	// Enable EXTI1 interrupt in NVIC.
	NVIC_EnableIRQ(EXTI2_IRQn);	// Enable EXTI2 interrupt in NVIC.
	NVIC_EnableIRQ(ADC_IRQn); 	// Bật ngắt ADC trong NVIC

	system_on_off(system_on);
}

/**
 * xử lý ngắt PA1: on/off hệ thống.
 */
void EXTI1_IRQHandler(void) {
	if (EXTI->PR & (1 << 1)) {
		sys_state ^= 1;				//đổi trạng thái
		system_on_off(sys_state);	//gọi hàm on-of
		EXTI->PR |= (1U << 1);      // Clear interrupt pending
	}
}

/**
 * xử lý ngắt PA1: reset.
 */
void EXTI2_IRQHandler(void) {
	if (EXTI->PR & (1 << 2)) {
		system_reset();
		EXTI->PR |= (1 << 2); // Clear interrupt pending
	}
}

/**
 * Cấu hình PA0 chế độ analog để đọc ADC.
 * Cấu hình ADC1
 */
void init_ADC_MQ2(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  	// Bật clock GPIOA
	GPIOA->MODER &= ~(3 << (0 * 2));  		// Xóa cấu hình cũ của PA0
	GPIOA->MODER |= (3 << (0 * 2)); // Chọn chế độ analog cho PA0 (MODER00 = 11)

	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;     // Bật clock cho ADC1

	ADC1->CR1 &= ~ADC_CR1_RES_Msk;        	// Xóa cấu hình độ phân giải cũ
	ADC1->CR1 |= ADC_CR1_EOCIE;	// Bật ngắt khi ADC chuyển đổi hoàn tất (EOC)

	ADC1->SMPR2 |= (7 << (0 * 3)); // Cấu hình thời gian lấy mẫu cho kênh 0:  cycles
	ADC1->SQR3 &= ~(0xF << 0);              // Chọn kênh ADC = kênh 0 (PA0)
	ADC1->CR2 |= ADC_CR2_ADON;            	// Bật ADC1 (Enable ADC)

	NVIC_EnableIRQ(ADC_IRQn); 		// Bật ngắt ADC trong NVIC
}

/**
 * hàm tính ppm từ giá trị adc.
 */
int ppm_caculator(uint16_t data) {
	// Tính điện áp từ giá trị ADC đọc được (độ phân giải 8-bit → 256 mức)
//	double voltage = 5 * data / 256.0;
	// Tính điện trở Rs theo điện áp
	double Rs = 1000.0 * ((4095.0 - data) / data);
	// Tỷ số Rs/Ro (Ro = 34123.967 : đo trong không khí sạch)
	double divRsRo = Rs / 15000.0;
	// Tính log10(ppm) từ đường cong đặc trưng
	float log_ppm = -0.47 * log10f(divRsRo) + 1.63;
	// Tính ppm bằng cách mũ hóa cơ số 10
	return (int) powf(10.0, log_ppm);
}

/**
 * hàm xử lý ngắt từ adc.
 * đọc giá trị, tính ppm
 * kiểm tra trạng thái cảnh báo.
 */
void ADC_IRQHandler(void) {
	if (ADC1->SR & (1 << 1)) 	// Kiểm tra cờ EOC (End Of Conversion)
			{
		adc_value = ADC1->DR;   			// Đọc giá trị ADC
		ppm_value = ppm_caculator(adc_value); 	// Tính toán ppm từ giá trị ADC

		if (ppm_value < low_gar_value) {	//kiểm tra mức khí gar
			if (warning_state == 0)				// so sánh trạng thái trước đó
				return;								//cùng trạng thái trả về
			else {
				warning_state = 0;				// khác trạng thái : cập nhật
				gar_flag = 1;						// và set flag
			}
		} else if (ppm_value >= low_gar_value
				&& ppm_value < high_gar_value) {
			if (warning_state == 1)
				return;
			else {
				warning_state = 1;
				gar_flag = 1;
			}
		} else if (ppm_value >= high_gar_value
				&& ppm_value < warning_gar_value) {
			if (warning_state == 2)
				return;
			else {
				warning_state = 2;
				gar_flag = 1;
			}
		} else if (ppm_value >= warning_gar_value) {
			if (warning_state == 3)
				return;
			else {
				warning_state = 3;
				gar_flag = 1;
			}
		}

		ADC1->SR &= ~(1 << 1);  			// Xóa cờ EOC
	}
}

/**
 * khởi tạo timer cho led RGB.
 * cấu hình GPIO CH1-PA8-R, CH2-PA9-G, CH3-PA10-B
 * cấu hình PWM mode2 cho cả 3 kênh.
 * mục đích : tạo xung để nhấp nháy led.
 */
void init_timer1_led_RGB(void) {
	// 1. Bật clock GPIOA và TIM1
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	// 2. Cấu hình các chân PA8, PA9, PA10 làm Alternate Function (AF1 tương ứng với TIM1)
	GPIOA->MODER &= ~((3 << (8 * 2)) | (3 << (9 * 2)) | (3 << (10 * 2))); // Xóa cấu hình cũ
	GPIOA->MODER |= (2 << (8 * 2)) | (2 << (9 * 2)) | (2 << (10 * 2)); // Đặt chế độ AF

	GPIOA->AFR[1] &= ~((0xF << ((8 - 8) * 4)) | (0xF << ((9 - 8) * 4))
			| (0xF << ((10 - 8) * 4)));
	GPIOA->AFR[1] |= (1 << ((8 - 8) * 4)) | (1 << ((9 - 8) * 4))
			| (1 << ((10 - 8) * 4));  // AF1
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

	TIM1->CCMR1 |= (0b111 << TIM_CCMR1_OC1M_Pos);  // PWM mode 2 cho OC1
	TIM1->CCMR1 |= (0b111 << TIM_CCMR1_OC2M_Pos);  // PWM mode 2 cho OC2
	TIM1->CCMR2 |= (0b111 << TIM_CCMR2_OC3M_Pos);  // PWM mode 2 cho OC3

	TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E; // Cho phép xuất tín hiệu trên CH1, CH2, CH3
	// 6. Cho phép output (Main Output Enable)
	TIM1->BDTR |= TIM_BDTR_MOE;
	// 7. Bắt đầu Timer
	TIM1->CR1 |= TIM_CR1_CEN;
}

void RGB_update(int R, int G, int B, int FREQ) {
	TIM1->CR1 &= ~TIM_CR1_CEN;  // Dừng Timer để cập nhật
	
	if (FREQ == 0)  // Nếu không nhấp nháy
			{
		TIM1->ARR = 999;  // Chu kỳ 1 giây
		// Cập nhật duty cycle 100% hoặc 0% tùy màu
		TIM1->CCR1 = (R == 0) ? 0 : 999;
		TIM1->CCR2 = (G == 0) ? 0 : 999;
		TIM1->CCR3 = (B == 0) ? 0 : 999;
	} else  // Nếu có nhấp nháy
	{
		int arr_val = (1000 / FREQ) - 1; // Tính chu kỳ mới theo tần số mong muốn
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
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;  // clock cho gpiob

	GPIOB->MODER &= ~(3 << (12 * 2));   // clear
	GPIOB->MODER |= (1 << (12 * 2));   // PB12: ouput

	GPIOB->OTYPER &= ~(1 << 12);        // Push-pull
	GPIOB->PUPDR &= ~(3 << (12 * 2));	// no pullupp , pulldown

	GPIOB->ODR &= ~(1 << 12);			// set giá trị ban đầu bằng 0.
}

void init_buzzer(void) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;	//  clock cho port B

	GPIOB->MODER &= ~(3 << (0 * 2));   // Clear
	GPIOB->MODER |= (1 << (0 * 2));   // Set PB0 output

	GPIOB->OTYPER &= ~(1 << 0);        // Push-pull
	GPIOB->PUPDR &= ~(3 << (0 * 2));	//No pull-up, no pull-down (PUPDR = 00)

	GPIOB->ODR |= 1 << 0;			//set giá trị ban đầu = 1: buzzer off.
}

void handle_uart_message(char *msg) {
    if (strcmp(msg, "SWITCH_ON") == 0) {
    	sys_state = system_on;
        system_on_off(system_on);

    } else if (strcmp(msg, "SWITCH_OFF") == 0) {
    	sys_state = system_off;
        system_on_off(system_off);

    } else if (strcmp(msg, "RESET") == 0) {
    	system_reset();

    } else if (msg[0] == '<' && msg[strlen(msg) - 1] == '>') {
        // Chuỗi dạng: <v6,v7,v8>

        if (sscanf(msg, "<%d,%d,%d>", &low_gar_value, &high_gar_value, &warning_gar_value) == 3) {

        }
    }
}

int main(void) {
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();

	I2C1_Init();
	lcd_init();
	lcd_gotoxy(0, 0);
	lcd_puts("system: on ");
	lcd_gotoxy(0, 1);
	lcd_puts("gar: 0  ppm: 0");

	init_ADC_MQ2();
	init_timer1_led_RGB();

	init_relay();
	init_buzzer();

	OnOffSwitch_Init();
	ResetSwitch_Init();
	RGB_update(0, 0, 1, 0);

	NVIC_SetPriority(EXTI2_IRQn, 1); 			// Ưu tiên mức 1
	NVIC_SetPriority(EXTI1_IRQn, 2); 			// Ưu tiên mức 2
	NVIC_SetPriority(ADC_IRQn, 3); 			// Ưu tiên mức 3 cho ngắt ADC

	while (1) {
		//khiểm tra hệ thống on.
		if (sys_state) {
			// kiểm tra gar_flag: được set khi trạng thái cảnh báo thay đổi.
			if (gar_flag) {
				switch (warning_state) {
				case 0:
					RGB_update(0, 0, 1, 0);		// cập nhật RGB.

					GPIOB->ODR &= ~(1 << 12);	//relay
					GPIOB->ODR |= (1 << 0);	//buzzer

					lcd_gotoxy(5, 1);
					lcd_puts("0");
					break;

				case 1:
					RGB_update(1, 1, 0, 0);
					GPIOB->ODR &= ~(1 << 12);
					GPIOB->ODR |= (1 << 0);

					lcd_gotoxy(5, 1);
					lcd_puts("1");
					break;

				case 2:
					RGB_update(1, 0, 0, 1);

					GPIOB->ODR |= 1 << 12;
					GPIOB->ODR |= 1 << 0;

					lcd_gotoxy(5, 1);
					lcd_puts("2");
					lcd_puts("1");
					break;

				case 3:
					RGB_update(1, 0, 0, 5);

					GPIOB->ODR |= 1 << 12;
					GPIOB->ODR &= ~(1 << 0);

					lcd_gotoxy(5, 1);
					lcd_puts("3");

					lcd_puts("1");
					break;

				default:
					break;
				}
				gar_flag = 0;
			}
			char str[161];
			sprintf(str, "%d   ", ppm_value);
			lcd_gotoxy(13, 1);
			lcd_puts(str);

			sprintf(str, "%d   ", adc_value);
			lcd_gotoxy(12, 0);
			lcd_puts(str);

			ADC1->CR2 |= ADC_CR2_SWSTART;         // Bắt đầu chuyển đổi
			for (int i = 0; i < 2000000; i++)
				;		//delay
		}

	} /* USER CODE END 3 */
}
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 15;
	RCC_OscInitStruct.PLL.PLLN = 96;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
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

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
