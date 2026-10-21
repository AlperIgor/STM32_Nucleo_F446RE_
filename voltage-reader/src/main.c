#include "stm32f4xx_hal.h"
#include <stdio.h>
// Хендлеры для управления периферией
ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART2_UART_Init(void);
void MX_ADC1_Init(void);

int main(void){
    HAL_Init();
    //Настройка тактирования микроконтроллера на максимальную частоту
   SystemClock_Config();
    //Инициализация портов ввода-вывода, UART2 и ADC1
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();

    uint32_t adc_value=0;
    char msg[64];
    
    while(1){
        //Начинаем преобразование АЦП
        HAL_ADC_Start(&hadc1);

        // Ждем окончания измерения (таймаут 10 мс)
        if(HAL_ADC_PollForConversion(&hadc1,10)==HAL_OK){
            // Считываем 12-битное значение (0...4095)
            adc_value = HAL_ADC_GetValue(&hadc1);
            // Формируем строку и отправляем в консоль через встроенный в Nucleo COM-порт
            int len = sprintf(msg, "ADC Value (PA0): %lu\r\n", adc_value);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 50); 
        }
        // Останавливаем АЦП до следующего цикла
        HAL_ADC_Stop(&hadc1);
        HAL_Delay(200);
    }
}
// --- Функция инициализации модуля АЦП №1 (ADC1) на пине PA0 ---
void MX_ADC1_Init(void){
      // Структура для настройки конкретного измерительного канала АЦП
    ADC_ChannelConfTypeDef sConfig = {0};


    // 1. ВКЛЮЧЕНИЕ ТАКТИРОВАНИЯ ПЕРИФЕРИИ
    // Активируем подачу тактового сигнала на модуль ADC1 (шина APB2)
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Активируем тактирование порта GPIOA (шина AHB1), так как пин PA0 находится на нем
    __HAL_RCC_GPIOA_CLK_ENABLE();

     // 2. НАСТРОЙКА ФИЗИЧЕСКОГО ПИНА МИКРОКОНТРОЛЛЕРА
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;          // Выбираем нулевой пин PA0 
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;    // Переводим пин в аналоговый режим (отключаем цифровые буферы)
    GPIO_InitStruct.Pull = GPIO_NOPULL;         // Отключаем подтяжки к VCC/GND, чтобы не искажать измеряемый сигнал
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);     // Применяем настройки к порту GPIOA

     // 3. НАСТРОЙКА ОБЩИХ ПАРАМЕТРОВ МОДУЛЯ АЦП
    hadc1.Instance = ADC1;                                      // Указываем, что настраиваем именно первый блок АЦП
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;       // Делим частоту шины PCLK на 4. АЦП не должен работать слишком быстро, иначе упадет точность
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;                 // Разрешение 12 бит: результат измерения будет числом от 0 до 4095
    hadc1.Init.ScanConvMode = DISABLE;                          // Сканирование отключено (измеряем только один канал, а не группу по очереди)
    hadc1.Init.ContinuousConvMode = DISABLE;                    // Непрерывный режим отключен: сделали замер — АЦП ждет следующей команды
    hadc1.Init.DiscontinuousConvMode = DISABLE;                 // Прерывистый режим отключен
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; // Запуск от внешних событий (например, таймеров) отключен
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;           // АЦП будет запускаться программно (командой из кода HAL_ADC_Start)
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;                 // Выравнивание битов в регистре данных по правому краю (удобно читать обычный int)
    hadc1.Init.NbrOfConversion = 1;                             // Указываем, сколько каналов будет участвовать в одном цикле преобразования (всего 1)
    hadc1.Init.DMAContinuousRequests = DISABLE;                 // Прямой доступ к памяти (DMA) отключен, данные забираем вручную
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;              // Флаг окончания преобразования (End Of Conversion) взводится после каждого замера
    
    // Инициализируем АЦП с базовыми параметрами. Если что-то пойдет не так, HAL вернет ошибку.
    HAL_ADC_Init(&hadc1);

    // 4. КОНФИГУРАЦИЯ КОНКРЕТНОГО КАНАЛА (ADC1_IN0)
    sConfig.Channel = ADC_CHANNEL_0;               // Привязываем логический Канал 0 (пин PA0 жестко соединен с IN0)
    sConfig.Rank = 1;                              // Задаем очередь замера. Этот канал будет опрашиваться самым первым (и единственным)
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES; // Время выборки (конденсатор АЦП заряжается 56 тактов). Чем больше значение, тем точнее замер
    
    // Применяем настройки канала к нашему модулю АЦП
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

// --- Функция инициализации модуля USART2 для связи с ПК ---
void MX_USART2_UART_Init(void)
{
    // 1. ВКЛЮЧЕНИЕ ТАКТИРОВАНИЯ
    // Активируем тактовый сигнал для модуля USART2 (шина APB1)
    __HAL_RCC_USART2_CLK_ENABLE();
    // Активируем тактирование порта GPIOA (шина AHB1)
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 2. НАСТРОЙКА ПИНОВ ПОД АЛЬТЕРНАТИВНУЮ ФУНКЦИЮ (UART)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // На плате Nucleo пины PA2 и PA3 аппаратно соединены с чипом программатора ST-LINK, который создает виртуальный COM-порт
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3; // PA2 -> линия передачи (TX), PA3 -> линия приема (RX)
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;        // Режим: Альтернативная функция, двухтактный выход (Alternate Function Push-Pull)
    GPIO_InitStruct.Pull = GPIO_NOPULL;           // Отключаем внутренние подтяжки, уровни обеспечивает передатчик
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // Максимальная скорость работы пинов для четких фронтов сигнала
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;   // Переключаем мультиплексор пинов на функцию №7 (именно она отвечает за USART2)
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);       // Применяем настройки геометрии пинов

    // 3. НАСТРОЙКА ПАРАМЕТРОВ ПЕРЕДАЧИ ДАННЫХ UART
    huart2.Instance = USART2;                     // Указываем, что настраиваем второй модуль USART
    huart2.Init.BaudRate = 115200;                // Скорость передачи данных: 115 200 бит/секунду
    huart2.Init.WordLength = UART_WORDLENGTH_8B;  // Длина пакета данных: 8 бит (1 байт — стандарт для текста)
    huart2.Init.StopBits = UART_STOPBITS_1;       // Количество стоп-бит: 1 (сигнализирует о конце посылки)
    huart2.Init.Parity = UART_PARITY_NONE;        // Бит четности (проверка ошибок) отключен
    huart2.Init.Mode = UART_MODE_TX_RX;           // Включаем оба направления: и передачу (TX), и прием (RX)
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;  // Аппаратное управление потоком данных (RTS/CTS) отключено
    huart2.Init.OverSampling = UART_OVERSAMPLING_16; // 16-кратная передискретизация для более надежного подавления помех в линии
    
    // Инициализируем модуль UART2 с заданными параметрами
    HAL_UART_Init(&huart2);
}

void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

// Системное тактирование (настройка тактовой частоты по умолчанию)
void SystemClock_Config(void)
{
  
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Включаем внутренний генератор HSI
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE; // Отключаем сложный множитель PLL
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  // Настраиваем шины процессора напрямую на частоту HSI (16 МГц)
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

// Прерывание SysTick для работы функций HAL_Delay()
void SysTick_Handler(void)
{
    HAL_IncTick();
}






