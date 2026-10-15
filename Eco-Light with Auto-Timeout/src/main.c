#include "stm32f4xx_hal.h"

TIM_HandleTypeDef htim6;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);

int main(void){
    HAL_Init();
    
    SystemClock_Config(); 
    
    MX_GPIO_Init();
    MX_TIM6_Init();
    
    while(1){
        // Основной цикл свободен
    }
}

void SystemClock_Config(void){
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    HAL_PWREx_EnableOverDrive();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK

                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

static void MX_GPIO_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // Зеленый светодиод LD2 на Nucleo (PA5)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Синяя кнопка USER на Nucleo (PC13)
    // ИСПРАВЛЕНО: Кнопка Nucleo замыкается на Землю, поэтому нужен IT_FALLING и PULLUP
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; 
    GPIO_InitStruct.Pull = GPIO_PULLUP; 
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // ИСПРАВЛЕНО: Для 13-го пина используется вектор EXTI15_10_IRQn
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0); 
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void MX_TIM6_Init(void){
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 53999; 
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 4999;     // Ровно 3 секунды      
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&htim6);
    
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 2, 0); 
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn); 
}

// Переопределяем Callback для внешних прерываний
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // ИСПРАВЛЕНО: Проверяем нажатие именно 13-го пина
    if (GPIO_Pin == GPIO_PIN_13) 
    {
        // 1. Включаем зеленый светодиод
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        
        // 2. Сбрасываем счетчик в ноль
        __HAL_TIM_SET_COUNTER(&htim6, 0);
        
        // 3. Запускаем таймер
        HAL_TIM_Base_Start_IT(&htim6);
    }
}

// Переопределяем Callback для прерываний таймера
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        // 1. Время вышло — гасим светодиод
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        
        // 2. Выключаем таймер
        HAL_TIM_Base_Stop_IT(&htim6);
    }
}

// ИСПРАВЛЕНО: Название обработчика для группы пинов 10-15
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}
