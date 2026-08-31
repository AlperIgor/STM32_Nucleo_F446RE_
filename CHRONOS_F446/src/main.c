#include "stm32f4xx_hal.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include  <stdio.h>
#include <string.h>
#include <stdlib.h>

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

void MX_I2C1_Init(void);
void MX_USART2_UART_Init(void);
void MX_GPIO_Init(void);
void Draw_SciFi_Interface(void);

//Переменные для хранения времени 
uint8_t hours = 21;
uint8_t minutes = 0;
uint8_t seconds = 0;
// Буферы для работы с UART
uint8_t rx_byte;
char rx_buffer[32];
uint8_t rx_index=0;

int main(void) {
    HAL_Init();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    // Даем дисплею 1 секунду на включение питания
    HAL_Delay(1000); 

    //  Принудительно меняем адрес в библиотеке на 0x3С (со сдвигом влево это 0x7A)
    
    #ifdef SSD1306_I2C_ADDR
    #undef SSD1306_I2C_ADDR
    #endif
    #define SSD1306_I2C_ADDR (0x3C << 1)
    // Инициализация дисплея
    ssd1306_Init();
    // Включаем прерывания UART на прием первого байта
    HAL_UART_Receive_IT(&huart2, &rx_byte,1);
    uint32_t last_time=HAL_GetTick();
    
    while (1) {
     // Проверяем: прошла ли ровно 1 секунда (1000 миллисекунд)?
     if(HAL_GetTick() - last_time>=1000){
        last_time=HAL_GetTick();//сбрасываем таймер на текущую отметку
        seconds++;
        
        if(seconds>=60){
            seconds=0;
            minutes++;
            if(minutes>=60){
                minutes=0;
                hours++;
                if(hours>=24){
                    hours=0;
                }
            }
        }
           // Отрисовка киберпанк интерфейса
            Draw_SciFi_Interface();
            
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
     }  

    }
}

void Draw_SciFi_Interface(void){
    char time_str[16];
    char sec_str[8];
    ssd1306_Fill(Black);
    // Верхняя часть: Прогрес секунд(шкала)
    ssd1306_DrawRectangle(4,2,124,8,White);//рамка полосы
    uint8_t bar_width=(seconds*118)/59;
    ssd1306_FillRectangle(6,4,6+bar_width,6,White);//заполнение 
    //Центральная часть: время крупным шрифтом
    sprintf(time_str, "%02d:%02d",hours, minutes);
    ssd1306_SetCursor(18,18);
    ssd1306_WriteString(time_str,Font_11x18,White);
    //Мелкие секунды отдельно справа вверху от времени
    sprintf(sec_str, "%02d",seconds);
    ssd1306_SetCursor(85,18);
    ssd1306_WriteString(sec_str, Font_7x10,White);
    // Технический статус "SYS OK"
    ssd1306_SetCursor(85,29);
    ssd1306_WriteString("OK",Font_6x8,White);
    //Нижняя часть: Декоративная рамка
    ssd1306_Line(0,48,127,48,White);
    ssd1306_SetCursor(6,52);
    ssd1306_WriteString("MODE: CHRONOS_F446", Font_6x8, White);
    
    ssd1306_Line(120, 63, 127, 56, White);
    ssd1306_Line(124, 63, 127, 60, White);

    ssd1306_UpdateScreen();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if(huart->Instance==USART2){
        if(rx_byte=='\n'|| rx_byte=='\r'){
            if(rx_index>0){
                rx_buffer[rx_index]='\0';
                int h=0,m=0,s=0;
                if(sscanf(rx_buffer,"%d:%d:%d",&h, &m, &s)==3){
                   hours = (uint8_t)h;
                    minutes = (uint8_t)m;
                    seconds = (uint8_t)s;  
                }
                rx_index=0;
            }
        }else{
           // Записываем байт в буфер, если есть место
            if (rx_index < sizeof(rx_buffer) - 1) {
                rx_buffer[rx_index++] = rx_byte;
            } 
        }
         // Снова разрешаем прерывание на прием следующего байта
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}
// Настройка USART2 (Виртуальный COM-порт через USB ST-LINK)
void MX_USART2_UART_Init(void){
     __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct={0};
    GPIO_InitStruct.Pin=GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull=GPIO_PULLUP;
    GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate=GPIO_AF7_USART2;
     HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200; // Скорость подключения
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);

    // Включаем прерывания в контроллере NVIC для USART2
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void MX_I2C1_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}
// Обработчик прерывания USART2 
void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}
void SysTick_Handler(void) {
    HAL_IncTick();
}


