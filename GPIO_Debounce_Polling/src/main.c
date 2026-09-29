#include "stm32f4xx_hal.h"
//Определения режимов светодиода
#define MODE_OFF 0
#define MODE_LOW 1
#define MODE_MEDIUM 2
#define MODE_HIGH 3
//Привязка к аппаратной периферии
#define BUTTON_PIN GPIO_PIN_13 //Синяя кнопка на плате
#define BUTTON_PORT GPIOC
#define LED_PIN GPIO_PIN_5 // Зеленый светодиод на плате
#define LED_PORT GPIOA

uint8_t current_mode=MODE_OFF;

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Настройка внутренней/внешней конфигурации осцилляторов 
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS; // Сигнал идет в обход встроенного генератора
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;             // 8 МГц / 8 = 1 МГц
  RCC_OscInitStruct.PLL.PLLN = 360;           // 1 МГц * 360 = 360 МГц
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 360 МГц / 2 = 180 МГц (Максимум процессора!)
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
   
    while(1);
  }

 
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    while(1);
  }


  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;     // AHB = 180 МГц
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;      // APB1 = 45 МГц (макс)
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;      // APB2 = 90 МГц (макс)

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    while(1);
  }
}
static void MX_GPIO_Init(void){

    GPIO_InitTypeDef GPIO_InitStruct={0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    // Конфигурация пина светодиода PA5
    GPIO_InitStruct.Pin=LED_PIN;
    GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull=GPIO_NOPULL;
    HAL_GPIO_Init(LED_PORT,&GPIO_InitStruct);
    //Конфигурация пина кнопки PC13
    GPIO_InitStruct.Pin=BUTTON_PIN;
    GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull=GPIO_NOPULL;
    HAL_GPIO_Init(BUTTON_PIN,&GPIO_InitStruct);
}
//Управление режимами 
void Set_LED_Brightness(uint8_t mode){
    switch(mode){
        case MODE_OFF:
        HAL_GPIO_WritePin(LED_PORT,LED_PIN,GPIO_PIN_RESET);
        break;
        case MODE_LOW:
        // Короткая вспышка, долгая пауза — светодиод горит тускло
        HAL_GPIO_WritePin(LED_PORT,LED_PIN,GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(LED_PORT,LED_PIN,GPIO_PIN_RESET);
        HAL_Delay(9);
        break;
        case MODE_MEDIUM:
        // Горит половину времени — средняя яркость
        HAL_GPIO_WritePin(LED_PORT,LED_PIN,GPIO_PIN_SET);
        HAL_Delay(5);
        HAL_GPIO_WritePin(LED_PORT,LED_PIN,GPIO_PIN_RESET);
        HAL_Delay(5);
        break;
         case MODE_HIGH:
         // Горит постоянно — максимальная яркость
         HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        break;
    }
}
int main(void){
    HAL_Init();
    MX_GPIO_Init();
    SystemClock_Config();
    while(1){
        //1. Опрос кнопки . Синяя кнопка при нажатии прижимается к GND (0)
         if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET){
            HAL_Delay(40);
         //2. Проверяем повторно. Если всё еще 0 — это уверенное нажатие
         if(HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET){
           //Циклически меняем режим (0 -> 1 -> 2 -> 3 -> 0)
            current_mode++;
        if (current_mode > MODE_HIGH)
        {
          current_mode = MODE_OFF;
        }
         Set_LED_Brightness(current_mode);

          //3Блокирующий цикл: ждем, пока пользователь ОТПУСТИТ кнопку.
          // Если этого не сделать, режимы будут бешено крутиться, пока кнопка зажата 
        while (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
        {
          /* Если выбран режим LOW или MEDIUM, продолжаем моргать светодиодом, 
             даже пока кнопка удерживается нажатой */
          if (current_mode == MODE_LOW || current_mode == MODE_MEDIUM)
          {
            Set_LED_Brightness(current_mode);
          }
         }    
         }
    // Если кнопка не нажата, просто поддерживаем текущее состояние светодиода */
    Set_LED_Brightness(current_mode);
    }
}
}
void SysTick_Handler(void)
{
  HAL_IncTick();
}
