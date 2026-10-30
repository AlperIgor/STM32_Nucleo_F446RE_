#include "stm32f4xx_hal.h"

void MX_GPIO_Init();
void SystemClock_Config();

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    uint32_t led_timer=0;// Переменная-"будильник" (хранит время, когда нужно выключить LED)
    uint8_t led_active=0;// Флаг состояния: 1 — светодиод сейчас горит, 0 — выключен

    uint32_t button_debounce=0;
    while(1){
        uint32_t currect_time=HAL_GetTick();// Получаем текущее время в миллисекундах
         // 1. ПРОВЕРКА КНОПКИ
         if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET){
             // Если кнопка нажата и прошло больше 200 мс с момента прошлого нажатия
            if(currect_time-button_debounce>200){
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5,GPIO_PIN_SET);// Включаем светодиод

                led_timer=currect_time+3000; // Ставим "будильник" на текущее время + 3000 мс (3 секунды)
                led_active=1; // Поднимаем флаг, что светодиод запущен
                button_debounce=currect_time; // Запоминаем время нажатия
            }

         }
     // 2. НЕБЛОКИРУЮЩАЯ ПРОВЕРКА ТАЙМЕРА
    // Если светодиод горит И текущее время перевалило за отметку "будильника"
     if(led_active==1 && (currect_time>=led_timer)){
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET);
        led_active=0;
     }
    }
}
void MX_GPIO_Init(void){
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // LED (PA5)
    GPIO_InitTypeDef GPIO_InitStruct={0};
    GPIO_InitStruct.Pin=GPIO_PIN_5;
    GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull=GPIO_NOPULL;
    GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA,&GPIO_InitStruct);
    // Кнопка (PC13)
    GPIO_InitStruct.Pin=GPIO_PIN_13;
    GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull=GPIO_PULLUP;
 
    HAL_GPIO_Init(GPIOC,&GPIO_InitStruct);


}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_CLOCKTYPE_PCLK2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}
void SysTick_Handler(void)
{
  HAL_IncTick(); // Увеличиваем счетчик времени HAL на 1
}