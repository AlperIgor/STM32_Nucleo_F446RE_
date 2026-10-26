#include "stm32f4xx_hal.h"
#include <string.h>

// Хэндл для структуры управления UART
UART_HandleTypeDef huart2;

// Глобальные переменные
volatile uint32_t brightness = 50;  // Стартовая яркость 50%
volatile uint32_t pwm_counter = 0; // Счетчик для программного ШИМ

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);

int main(void){
    HAL_Init();               // Инициализация библиотеки HAL
    SystemClock_Config();     // Стабильные 16 МГц от HSI (без конфликтов шин)
    MX_GPIO_Init();           // Настройка портов (Включая bare-metal PA5)
    MX_USART2_UART_Init();    // Настройка UART2 на частоте 16 МГц

    // Отправляем приветствие в терминал (работает мгновенно при подаче питания)
    char msg[] = "\r\n=== ДИММЕР НА NUCLEO-F446RE ГОТОВ ===\r\nВводите символы + / - или цифры 0-9:\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), 200);

    while(1){
        // --- 1. ЖЕЛЕЗОБЕТОННЫЙ ОПРОС UART БЕЗ ПРЕРЫВАНИЙ ---
        // Проверяем аппаратный флаг RXNE (Receive Data Register Not Empty) напрямую в регистре статуса SR
        if (USART2->SR & USART_SR_RXNE) {
            // Считываем принятый байт (чтение регистра DR автоматически сбрасывает флаг RXNE)
            uint8_t rx_data = (uint8_t)(USART2->DR & 0xFF);

            // Обработка логики изменения яркости
            if (rx_data == '+') {
                if (brightness <= 90) brightness += 10;
                else brightness = 100;
            } 
            else if (rx_data == '-') {
                if (brightness >= 10) brightness -= 10;
                else brightness = 0;
            } 
            else if (rx_data >= '0' && rx_data <= '9') {
                brightness = (rx_data - '0') * 11; // Шаги 0, 11, 22 ... 99%
            }

            // Эхо-ответ: мгновенно возвращаем принятый символ назад в терминал
            USART2->DR = rx_data;
            while (!(USART2->SR & USART_SR_TXE)); // Ждем окончания передачи байта
        }

        // --- 2. АВТОМАТИЧЕСКИЙ СБРОС ОШИБОК ПЕРЕПОЛНЕНИЯ (Шумозащита) ---
        if (USART2->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE)) {
            __HAL_UART_CLEAR_OREFLAG(&huart2);
            __HAL_UART_CLEAR_NEFLAG(&huart2);
            __HAL_UART_CLEAR_FEFLAG(&huart2);
        }

        // --- 3. ГЕНЕРАЦИЯ ПРОГРАММНОГО ШИМ ---
        pwm_counter++;
        if (pwm_counter >= 100) {
            pwm_counter = 0;
        }

        if (pwm_counter < brightness && brightness > 0) {
            GPIOA->BSRR = GPIO_PIN_5;        // Включить светодиод
        } else {
            GPIOA->BSRR = (uint32_t)GPIO_PIN_5 << 16; // Выключить светодиод
        }

        // Микросекундная пауза, чтобы ШИМ работал плавно и не мерцал
        for(volatile int i = 0; i < 20; i++);
    }
}

// Настройка портов ввода-вывода
void MX_GPIO_Init(void){
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Настройка пинов PA2 (TX) и PA3 (RX) под виртуальный COM-порт
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2; 
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // BARE-METAL ОЧИСТКА И НАСТРОЙКА PA5 (Светодиод) во избежание блокировок со стороны MCO2
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5); 
    GPIOA->MODER &= ~(3UL << (5 * 2));
    GPIOA->MODER |=  (1UL << (5 * 2)); // Режим Output
    GPIOA->PUPDR &= ~(3UL << (5 * 2));
    GPIOA->OSPEEDR |= (3UL << (5 * 2));
}

// Настройка модуля UART2 (Обычный режим опроса регистров)
void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE(); 

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;                  
    huart2.Init.WordLength = UART_WORDLENGTH_8B;     
    huart2.Init.StopBits = UART_STOPBITS_1;         
    huart2.Init.Parity = UART_PARITY_NONE;          
    huart2.Init.Mode = UART_MODE_TX_RX;             
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;    
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
    
    // Прерывания NVIC для UART не включаем! Опрос идет напрямую в цикле while(1).
}

// Безопасное тактирование напрямую от внутреннего HSI (16 МГц) — убирает любые зависания PLL
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; 
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
