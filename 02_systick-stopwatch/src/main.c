#include "stm32f446xx.h"
#include <stdio.h>

// 1. Объявление типов данных
typedef enum {
    STOPPED = 0,
    RUNNING = 1
} stopwatch_state_t;

// 2. Глобальные переменные проекта
static volatile uint32_t ms_ticks = 0;       
static volatile stopwatch_state_t state = STOPPED;
static volatile uint32_t elapsed_time = 0;  

// 3. Базовые функции работы с UART (должны идти ДО их вызова в прерываниях)
void uart2_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    __NOP();

    // Настраиваем PA2 и PA3 на альтернативную функцию AF7 (USART2)
    GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2))); 
    
    GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
    GPIOA->AFR[0] |=  ((0x7U << (2 * 4)) | (0x7U << (3 * 4))); 

    // Скорость 115200 при частоте шины APB1 в 45 МГц
    USART2->BRR = 0x8B;
    
    // Включаем передатчик и сам модуль USART
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void uart2_send_char(char c) {
    while (!(USART2->SR & USART_SR_TXE)); 
    USART2->DR = (c & 0xFF);
}

void uart2_send_string(const char* str) {
    while (*str) {
        uart2_send_char(*str++);
    }
}

// 4. Системный таймер (1 мс)
void SysTick_Handler(void) {
    if (state == RUNNING) {
        ms_ticks++;
    }
}

void systick_init(void) {
    SysTick->LOAD = (16000UL)-1UL;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | 
                    SysTick_CTRL_TICKINT_Msk | 
                    SysTick_CTRL_CLKSOURCE_Msk;
}

// 5. Обработчик прерывания кнопки на PC13 (теперь видит функцию отправки строки)
void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR13) {
        if (state == STOPPED) {
            ms_ticks = 0; // Сбрасываем счетчик при старте
            state = RUNNING;
            uart2_send_string("\r\n[StopWatch] Started...\r\n");
        } else {
            state = STOPPED;
            elapsed_time = ms_ticks; 
            
            // Буфер для форматирования строки (выделяем 64 байта в стеке)
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "[StopWatch] Stopped! Time: %lu ms\r\n", elapsed_time);
            uart2_send_string(buffer);
        }
        EXTI->PR = EXTI_PR_PR13; // Обязательный сброс флага прерывания
    }
}

void exti13_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    __NOP();

    GPIOC->MODER &= ~(3U << (13 * 2)); 
    GPIOC->PUPDR &= ~(3U << (13 * 2));
    GPIOC->PUPDR |=  (1U << (13 * 2)); // Подтяжка Pull-up

    // Исправленный массив EXTICR (используем индекс [3] для 13-й линии)
    SYSCFG->EXTICR[3] &= ~(0xFU << (1 * 4)); 
    SYSCFG->EXTICR[3] |=  (0x2U << (1 * 4));

    EXTI->FTSR |= EXTI_FTSR_TR13;  // Спадающий фронт
    EXTI->IMR |= EXTI_IMR_MR13;    

    NVIC_SetPriority(EXTI15_10_IRQn, 2);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// 6. Главная точка входа
int main(void) {
    systick_init();
    uart2_init();
    exti13_init();
    __enable_irq(); 

    uart2_send_string("STM32F446RE Bare-Metal Stopwatch Ready.\r\nPress B1 (PC13) to Start/Stop.\r\n");

    while (1) {
         // Режим низкого потребления энергии до клика кнопки
    }
}
