#include "stm32f4xx_hal.h"
void GPIO_init(void){
   GPIO_InitTypeDef GPIO_InitStuct={0};
   __HAL_RCC_GPIOA_CLK_ENABLE(); // Для пинов, начинающихся на PA...
__HAL_RCC_GPIOB_CLK_ENABLE(); // Для пинов, начинающихся на PB...
__HAL_RCC_GPIOC_CLK_ENABLE(); // Для пинов, начинающихся на PC...
 GPIO_InitStuct.Pin=GPIO_PIN_0;
 GPIO_InitStuct.Mode=GPIO_MODE_OUTPUT_PP;
 GPIO_InitStuct.Pull=GPIO_NOPULL;
 GPIO_InitStuct.Speed=GPIO_SPEED_FREQ_LOW;
 HAL_GPIO_Init(GPIOA, &GPIO_InitStuct);

 GPIO_InitStuct.Pin=GPIO_PIN_3;
 GPIO_InitStuct.Mode=GPIO_MODE_OUTPUT_PP;
 GPIO_InitStuct.Pull=GPIO_NOPULL;
 GPIO_InitStuct.Speed=GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStuct);

GPIO_InitStuct.Pin=GPIO_PIN_7;
 GPIO_InitStuct.Mode=GPIO_MODE_OUTPUT_PP;
 GPIO_InitStuct.Pull=GPIO_NOPULL;
 GPIO_InitStuct.Speed=GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStuct);


}
void SysTick_Handler(void){
    HAL_IncTick();
}
void SystemClock_Config(void){}
int main(void){
    HAL_Init();
    SystemClock_Config();
    SysTick_Handler();
    GPIO_init();
    while(1){
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_RESET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_RESET);
         HAL_Delay(1000);
        HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC,GPIO_PIN_7,GPIO_PIN_SET);
        HAL_Delay(1000);
    } 
}