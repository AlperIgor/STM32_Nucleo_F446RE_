#include "stm32f4xx.h"



int main(void) {
    // ШАГ 1: Включаем тактирование порта GPIOA
    // Светодиод (PA5) и сенсор (PA10) находятся на одном порту.
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN);

    // Настраиваем пин PA5 на выход
    GPIOA->MODER &= ~GPIO_MODER_MODER5_Msk;
    GPIOA->MODER |= (1 << GPIO_MODER_MODER5_Pos);
    //Настраиваем пин PA13-сенсор на вход
    GPIOC->MODER &= ~GPIO_MODER_MODER13_Msk;
    
    while(1) {
        // Читаем регистр IDR (входные данные) порта C.
        // С помощью знака '&' проверяем состояние только 13-го бита.
        // Если результат равен 0 — значит, кнопка прижата к земле (нажата).
        if((GPIOC->IDR & GPIO_IDR_ID13)==0){
            // Кнопка нажата: включаем светодиод.
            // Устанавливаем 5-й бит регистра ODR в единицу.
            GPIOA->ODR |=GPIO_ODR_OD5;
        }
        else{
            GPIOA->ODR &=~GPIO_ODR_OD5;
        }
    }

}
