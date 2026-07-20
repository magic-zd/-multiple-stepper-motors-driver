#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"

#include "motor_control.h"

int main(void)
{
    // 初始化
    Motor_GPIO_Init();      // 电机GPIO初始化
    USART1_Init(9600);      // 串口1初始化（9600bps）
    
    USART1_SendString("STM32 Motor Control Ready\r\n");
    USART1_SendString("Cmd Format: M<num>D<dir>S<steps>V<speed> (e.g., M1D1S100V50)\r\n");
    USART1_SendString("STOP: All Motors Lock\r\n");
    
    while (1)
    {
        // 轮询解析串口指令（无中断，简单易调试）
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
        {
          USART1_Parse_Cmd();
        }
    }
}
