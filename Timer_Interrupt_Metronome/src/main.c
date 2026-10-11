#include "stm32f4xx_hal.h"
#define LED_PIN GPIO_PIN_5
#define LED_GPIO_PORT GPIOA

#define PRESCALER_VAL   44999
#define PERIOD_VAL      999
//Объявление хэндлера управления базовым таймером TIM6
TIM_HandleTypeDef htim6;
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM6_Init(void);

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM6_Init();//Инициализация базового таймера TIM6
     /* 
     * ВАЖНО: Запуск таймера TIM6 в режиме генерации прерываний.
     * Эта функция разрешает аппаратному модулю отправлять сигналы прерываний в ядро NVIC.
     */
     HAL_TIM_Base_Start_IT(&htim6);
    while(1){}
}
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Настройка регулятора напряжения питания для работы на максимальной частоте 
    */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Инициализация осцилляторов RCC и настройка умножителя частоты PLL 
    * Внутренний генератор HSI = 16 МГц. 
    * Частота ядра = (16МГц / 16) * 360 / 2 = 180 МГц.
    */
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

    /** Активация режима Over-Drive для стабильной работы на частоте 180 МГц 
    */
    HAL_PWREx_EnableOverDrive();

    /** Инициализация тактирования шин CPU, AHB и APB 
    * HCLK (Шина AHB) = 180 МГц
    * PCLK1 (Шина APB1) = 45 МГц -> Таймеры на этой шине умножают частоту х2 = 90 МГц
    * PCLK2 (Шина APB2) = 90 МГц
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK

                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    /* Задержка Flash-памяти выставлена в 5 Latency (для работы на 180 МГц при 3.3В) */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}
static void MX_TIM6_Init(void){
    //Включение тактирования аппаратного модуля TIM6
    __HAL_RCC_TIM6_CLK_ENABLE();

    htim6.Instance=TIM6;
    htim6.Init.Prescaler=PRESCALER_VAL; //Наш предделитель
    htim6.Init.CounterMode=TIM_COUNTERMODE_UP;//Режим счета: только вверх
    htim6.Init.Period = PERIOD_VAL; //Период перезагрузки (ARR)
    htim6.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_ENABLE;

    //Применение настроек структуры к регистрам таймера
    HAL_TIM_Base_Init(&htim6);

    HAL_NVIC_SetPriority(TIM6_DAC_IRQn,0,0);//Высокий приоритет выполнения
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);//Разрешение обработки данного вектора 


}
static void MX_GPIO_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct={0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_GPIO_WritePin(LED_GPIO_PORT,LED_PIN,GPIO_PIN_RESET);
    GPIO_InitStruct.Pin=LED_PIN;
    GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull=GPIO_NOPULL;
    GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(LED_GPIO_PORT,&GPIO_InitStruct);

}
//Функция обратного вызова (Callback) по переполнению периода таймера.
//Вызывается библиотекой HAL автоматически внутри прерывания.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    //Обязательно проверяем, какой именно таймер вызвал Callback
    if(htim->Instance==TIM6){
        //Инвертируем состояние пина светодиода (ВКЛ -> ВЫКЛ -> ВКЛ...)
        HAL_GPIO_TogglePin(LED_GPIO_PORT,LED_PIN);
    }
}
//Низкоуровневый аппаратный обработчик прерывания (ISR).
//Имя функции жестко привязано к таблице векторов стартапа микроконтроллера.
void TIM6_DAC_IRQHandler(void){
    /* 
     * Передаем управление внутренней функции библиотеки HAL. 
     * Она очистит флаги прерывания в регистрах и перенаправит нас в Callback-функцию выше.
     */
    HAL_TIM_IRQHandler(&htim6);

}