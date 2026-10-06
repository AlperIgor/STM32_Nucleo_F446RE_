#include "stm32f4xx_hal.h"

#define LED_PIN GPIO_PIN_5
#define LED_GPIO_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_13
#define BUTTON_GPIO_PORT GPIOC
// Глобальный флаг аварии. Должен быть volatile, так как меняется внутри ИСР
volatile uint8_t emergency_mode = 0;
void SystemClock_Config(void);
void MX_GPIO_Init(void);
int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    while (1)
    {
       if(!emergency_mode){
        // Обычный режим: медленное мигание светодиодом
        HAL_GPIO_TogglePin(LED_GPIO_PORT,LED_PIN);
        HAL_Delay(1000);
       }else{
        // Аварийный режим: мгновенно выключаем светодиод
            HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
       }
    }
   
}
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Включение тактирования портов GPIOA и GPIOC
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // 1. Настройка пина светодиода (PA5) на выход
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    // 2. Настройка пина кнопки (PC13) на работу с прерыванием
    // Исползуем спад импульса (Falling Edge), так как кнопка при нажатии замыкается на GND
    GPIO_InitStruct.Pin = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; 
    GPIO_InitStruct.Pull = GPIO_NOPULL; // На Nucleo внешняя подтяжка уже есть на плате
    HAL_GPIO_Init(BUTTON_GPIO_PORT, &GPIO_InitStruct);

    // 3. Настройка контроллера прерываний (NVIC) для EXTI15_10
    // Линия PC13 относится к вектору прерываний EXTI15_10_IRQn
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0); // Приоритет 2
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);        // Разрешаем прерывание
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // Проверяем, что прерывание пришло именно от нашей кнопки
    if (GPIO_Pin == BUTTON_PIN) {
        // Мгновенное действие: активируем аварийный режим и гасим диод
        emergency_mode = 1;
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);
    }
}
void EXTI15_10_IRQHandler(void){
    HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);
}
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Включение тактирования модуля управления питанием (PWR)
    __HAL_RCC_PWR_CLK_ENABLE();
    

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    // Настройка источника тактования: внутренний HSI
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Ошибка инициализации тактирования
        while(1);
    }
RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_SYSCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        // Ошибка конфигурации шин
        while(1);
    }
}
void SysTick_Handler(void) {
    HAL_IncTick();
}