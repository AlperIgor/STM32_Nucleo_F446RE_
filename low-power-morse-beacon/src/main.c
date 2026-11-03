#include "stm32f4xx_hal.h"

#define LED_PIN GPIO_PIN_5
#define LED_PORT GPIOA
#define BUTTON_PIN GPIO_PIN_13
#define BUTTON_PORT GPIOC

// Длительность сигналов азбуки Морзе (в миллисекундах)
#define DOT_TIME 200
#define DASH_TIME (DOT_TIME*3)
#define PAUSE_TIME DOT_TIME
#define LETTER_PAUSE (DOT_TIME*3)

// Флаг прерывания
volatile uint8_t g_wakeup_flag = 0;

void MX_GPIO_Init(void);
void SystemClock_Config(void);
void Enter_Stop_Mode(void);
void Flash_Morse_Element(uint32_t duration);
void Transmit_SOS(void);

int main(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    while(1){
        if(g_wakeup_flag){
            g_wakeup_flag = 0;
            // Воспроизводим сигнал SOS
            Transmit_SOS();
        }
        
        // После выполнения (или если проснулись ложно) снова уходим в глубокий сон
        Enter_Stop_Mode();
    }
}

// Перевод микроконтроллера в режим STOP
void Enter_Stop_Mode(void){
    // Гарантируем, что диод выключен перед сном
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    // Приостанавливаем прерывания SysTick перед входом в сон, 
    // чтобы системный таймер не будил процессор каждую миллисекунду
    HAL_SuspendTick();

    // Очищаем флаг пробуждения (Wakeup Flag) в контроллере питания
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    // Входим в STOP режим
    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

    /* --- ЗДЕСЬ МИКРОКОНТРОЛЛЕР СПИТ И ЖДЕТ НАЖАТИЯ КНОПКИ --- */

    // Первым делом после пробуждения полностью восстанавливаем тактовую частоту системы (180 МГц)
    SystemClock_Config();

    // ВОТ ОН КЛЮЧЕВОЙ МОМЕНТ: возобновляем работу системного таймера SysTick, иначе HAL_Delay зависнет!
    HAL_ResumeTick();
}

// Вспомогательная функция для вспышки светодиода заданной длительности
void Flash_Morse_Element(uint32_t duration){
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    HAL_Delay(duration);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    HAL_Delay(PAUSE_TIME);
}

// Отправка сигнала SOS
void Transmit_SOS(void){
    // Буква 'S' (... )
    Flash_Morse_Element(DOT_TIME);
    Flash_Morse_Element(DOT_TIME);
    Flash_Morse_Element(DOT_TIME);
    HAL_Delay(LETTER_PAUSE - PAUSE_TIME);

    // Буква 'O' (---)
    Flash_Morse_Element(DASH_TIME);
    Flash_Morse_Element(DASH_TIME);
    Flash_Morse_Element(DASH_TIME);
    HAL_Delay(LETTER_PAUSE - PAUSE_TIME);

    // Буква 'S' (... )
    Flash_Morse_Element(DOT_TIME);
    Flash_Morse_Element(DOT_TIME);
    Flash_Morse_Element(DOT_TIME);
    
    // Пауза после всей серии SOS перед возможным следующим циклом
    HAL_Delay(1000); 
}

void MX_GPIO_Init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Настройка светодиода
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Настройка кнопки: обязательно включаем внутренний Pull-Up (GPIO_PULLUP)
    GPIO_InitStruct.Pin = BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP; 
    HAL_GPIO_Init(BUTTON_PORT, &GPIO_InitStruct);
    
    // Настройка приоритетов прерываний кнопки
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON; 
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    HAL_PWREx_EnableOverDrive();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

// Колбэк прерывания EXTI 
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin == BUTTON_PIN){
        g_wakeup_flag = 1;
    }
}

// Системный обработчик прерывания для нашей кнопки
void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(BUTTON_PIN);
}
void SysTick_Handler(void)
{
    HAL_IncTick();
}