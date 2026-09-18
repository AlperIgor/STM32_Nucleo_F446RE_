#include "stm32f4xx.h"

// Простая задержка на цикле
void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 4000; i++) {
        __NOP();
    }
}

// Отправка одного символа в UART2
void uart_putc(char c) {
    while (!(USART2->SR & USART_SR_TXE));  // Ждем, пока освободится буфер передачи
    USART2->DR = (uint8_t)c;               // Записываем символ в регистр данных
}

// Отправка строки текста в UART2
void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s);
        s++;
    }
}

// Ожидание и прием одного символа из UART2
char uart_getc(void) {
    while (!(USART2->SR & USART_SR_RXNE)); // Ждем, пока придут данные (флаг RXNE станет 1)
    return (char)(USART2->DR & 0xFF);      // Читаем байт и возвращаем его
}

int main(void) {
    // 1. Включаем тактирование периферии
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // Тактирование портов GPIOA (пины PA2, PA3, PA5)
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;  // Тактирование модуля USART2

    // 2. Настраиваем пин PA2 (TX) как альтернативную функцию AF7
    GPIOA->MODER &= ~(0b11 << (2*2));
    GPIOA->MODER |=  (0b10 << (2*2));      // Alternate function
    GPIOA->OTYPER &= ~(1 << 2);            // Push-pull
    GPIOA->OSPEEDR |= (0b11 << (2*2));     // Very high speed
    GPIOA->AFR[0] &= ~(0xF << (2*4));
    GPIOA->AFR[0] |=  (7 << (2*4));        // AF7 для PA2

    // 3. Настраиваем пин PA3 (RX) как альтернативную функцию AF7
    GPIOA->MODER &= ~(0b11 << (3*2));
    GPIOA->MODER |=  (0b10 << (3*2));      // Alternate function
    GPIOA->PUPDR &= ~(0b11 << (3*2));
    GPIOA->PUPDR |=  (0b01 << (3*2));      // Включаем подтяжку Pull-up (важно для линии RX)
    GPIOA->AFR[0] &= ~(0xF << (3*4));
    GPIOA->AFR[0] |=  (7 << (3*4));        // AF7 для PA3

    // 4. Настраиваем пин PA5 (зеленый светодиод LD2 на платах Nucleo) на выход
    GPIOA->MODER &= ~(0b11 << (5*2));
    GPIOA->MODER |=  (0b01 << (5*2));      // General purpose output mode

    // 5. Настройка параметров самого USART2 (115200 бод при 16 МГц)
    USART2->BRR = 0x8B;                    // Точное значение делителя для 115200 бод при тактировании 16 МГц
    
    // Включаем: сам модуль (UE), передатчик (TE) и приемник (RE)
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; 

    // Даем виртуальному COM-порту на ПК время определиться
    delay_ms(1000);  
    
    // Стартовое приветствие
    uart_puts("--- STM32 UART Terminal Active ---\r\n");
    uart_puts("Press '1' to turn LED ON, '0' to turn LED OFF.\r\n");

    while (1) {
        // Программа останавливается (ждет) на этой строчке, пока в терминале не нажмут клавишу
        char user_input = uart_getc(); 

        // Механизм ЭХО: отправляем нажатый символ обратно, чтобы он отобразился на экране ПК
        uart_putc(user_input); 

        // Обработка введенной команды
        if (user_input == '1') {
            GPIOA->BSRR = (1 << 5);        // Устанавливаем PA5 в 1 (Светодиод загорается)
            uart_puts(" -> LED is NOW ON!\r\n");
        } 
        else if (user_input == '0') {
            GPIOA->BSRR = (1 << (5 + 16)); // Сбрасываем PA5 в 0 (Светодиод гаснет)
            uart_puts(" -> LED is NOW OFF!\r\n");
        } 
        else if (user_input == '\r' || user_input == '\n') {
            // Игнорируем перевод строки, если в терминале включена отправка Enter
            continue; 
        }
        else {
            uart_puts(" -> Error: unknown command!\r\n");
        }
    }
}