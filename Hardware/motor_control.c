#include "motor_control.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "Delay.h"
#include <stdio.h>
// 12路电机STEP/DIR引脚配置（严格对应最终GPIO表）
Motor_GPIO_Config motor_gpio_config[12] = {
    {GPIOA, GPIO_Pin_0,  GPIOA, GPIO_Pin_1},   // MOTOR_1
    {GPIOA, GPIO_Pin_3,  GPIOA, GPIO_Pin_4},   // MOTOR_2
    {GPIOA, GPIO_Pin_5,  GPIOA, GPIO_Pin_6},   // MOTOR_3
    {GPIOA, GPIO_Pin_7,  GPIOA, GPIO_Pin_8},   // MOTOR_4
    {GPIOA, GPIO_Pin_11, GPIOA, GPIO_Pin_12},  // MOTOR_5
    {GPIOB, GPIO_Pin_0,  GPIOB, GPIO_Pin_1},   // MOTOR_6
    {GPIOC, GPIO_Pin_14, GPIOC, GPIO_Pin_15},   // MOTOR_7 
    {GPIOB, GPIO_Pin_7,  GPIOB, GPIO_Pin_8},   // MOTOR_8  
    {GPIOB, GPIO_Pin_5,  GPIOB, GPIO_Pin_6},   // MOTOR_9
    {GPIOB, GPIO_Pin_9,  GPIOB, GPIO_Pin_10},   // MOTOR_10
    {GPIOB, GPIO_Pin_11, GPIOB, GPIO_Pin_12},  // MOTOR_11
    {GPIOB, GPIO_Pin_13, GPIOB, GPIO_Pin_14}   // MOTOR_12
};

// 电机GPIO初始化（STEP/DIR为推挽输出，ENABLE全局使能）
void Motor_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // 使能GPIOA/GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB| RCC_APB2Periph_GPIOC, ENABLE);
    
    // 1. 全局ENABLE引脚初始化（PA2，推挽输出，上拉，50MHz）
    GPIO_InitStruct.GPIO_Pin = MOTOR_ENABLE_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(MOTOR_ENABLE_PORT, &GPIO_InitStruct);
    //	  锁死电机
	  GPIO_SetBits(MOTOR_ENABLE_PORT, MOTOR_ENABLE_PIN); 
	
    
    // 2. 12路电机STEP/DIR引脚初始化
    for (uint8_t i = 0; i < 12; i++)
    {
        // STEP引脚初始化
        GPIO_InitStruct.GPIO_Pin = motor_gpio_config[i].step_pin;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(motor_gpio_config[i].step_port, &GPIO_InitStruct);
        
        // DIR引脚初始化
        GPIO_InitStruct.GPIO_Pin = motor_gpio_config[i].dir_pin;
        GPIO_Init(motor_gpio_config[i].dir_port, &GPIO_InitStruct);
        
        // 默认电平：STEP低、DIR低
        GPIO_ResetBits(motor_gpio_config[i].step_port, motor_gpio_config[i].step_pin);
        GPIO_ResetBits(motor_gpio_config[i].dir_port, motor_gpio_config[i].dir_pin);
    }
}

// USART1初始化（9600bps，8N1，PA9=TX，PA10=RX）
void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    
    // 使能USART1和GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // TX引脚（PA9）：复用推挽输出
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // RX引脚（PA10）：浮空输入
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // USART配置
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStruct);
    
    // 使能USART1
    USART_Cmd(USART1, ENABLE);
}
// 全局电机使能/失能（0=失能/锁死，1=使能/可动）
void Motor_Set_Enable(uint8_t enable)
{
    if (enable)
    {
        // 使能：ENABLE低电平（A4988低电平有效）
        GPIO_ResetBits(MOTOR_ENABLE_PORT, MOTOR_ENABLE_PIN);
    }
    else
    {
        // 失能：ENABLE高电平（锁死所有电机）
        GPIO_SetBits(MOTOR_ENABLE_PORT, MOTOR_ENABLE_PIN);
    }
}

// 设置单电机方向（0=反转，1=正转）
void Motor_Set_Direction(Motor_Num motor, uint8_t dir)
{
    if (dir)
    {
        // 正转：DIR引脚高电平
        GPIO_SetBits(motor_gpio_config[motor].dir_port, motor_gpio_config[motor].dir_pin);
    }
    else
    {
        // 反转：DIR引脚低电平
        GPIO_ResetBits(motor_gpio_config[motor].dir_port, motor_gpio_config[motor].dir_pin);
    }
}

// 单电机走指定步长（4细分适配：脉冲数=步长）
// motor：电机编号，steps：步长（脉冲数），speed：速度（步/秒，范围10-500）
void Motor_Run_Steps(Motor_Num motor, uint32_t steps, uint16_t speed)
{
    if (speed < 10) speed = 10; // 最低速度，避免脉冲过密
    if (speed > 500) speed = 500; // 最高速度，避免丢步
    
    uint32_t step_delay = 1000 / speed; // 每步间隔（ms）
    
    // 1. 前置准备：使能电机
    Motor_Set_Enable(1);
    
    // 2. 发送脉冲（上升沿触发A4988，4细分下1脉冲=1/4步距角）
    for (uint32_t i = 0; i < steps; i++)
    {
        // STEP引脚高电平
        GPIO_SetBits(motor_gpio_config[motor].step_port, motor_gpio_config[motor].step_pin);
        Delay_ms(step_delay/2);
        // STEP引脚低电平
        GPIO_ResetBits(motor_gpio_config[motor].step_port, motor_gpio_config[motor].step_pin);
        // 步间延时（控制速度）
        Delay_ms(step_delay/2); // 扣除脉冲高电平的1ms
    }
    
    // 3. 后置处理：失能电机（锁死）
    Motor_Set_Enable(0);
    
    // 串口回显完成信息
    char resp[32];
    sprintf(resp, "Motor%d Run %d Steps OK\r\n", motor+1, steps);
    USART1_SendString(resp);
}
// 串口发送字符串（标准库）
void USART1_SendString(char* str)
{
    while (*str != '\0')
    {
        // 等待发送寄存器为空
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
    // 等待发送完成
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}



// 串口接收单个字符（轮询方式，无中断）
uint8_t USART1_ReceiveChar(void)
{
    // 等待接收寄存器非空
    while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
    return USART_ReceiveData(USART1);
}
#include <string.h>
#include <stdlib.h>

// 指令缓存（存储串口接收的完整指令）
#define CMD_BUF_LEN 32
char cmd_buf[CMD_BUF_LEN];
uint8_t cmd_buf_idx = 0;

// 解析串口指令（核心函数）
void USART1_Parse_Cmd(void)
{
    // 1. 清空无效指令
    if (cmd_buf_idx >= CMD_BUF_LEN - 1)
    {
			  USART1_SendString("Error: Cmd Too Long\r\n"); // 新增：提示指令过CHANG
        cmd_buf_idx = 0;
        memset(cmd_buf, 0, CMD_BUF_LEN);
        return;
    }
    
    // 2. 读取串口字符，直到换行（\n）触发解析
    uint8_t ch = USART1_ReceiveChar();
    if (ch == '\r') // 换行作为指令结束符
    {
        cmd_buf[cmd_buf_idx] = '\0'; // 字符串结束符
        cmd_buf_idx = 0; // 重置缓存索引
         USART1_SendString("指令已接受\r\n");
        // 3. 解析指令
        // 3.1 紧急停止指令
        if (strcmp(cmd_buf, "STOP") == 0)
        {
            Motor_Set_Enable(0); // 锁死所有电机
            USART1_SendString("All Motors STOP\r\n");
            return;
        }
        
        // 3.2 电机控制指令（M<num>D<dir>S<steps>V<speed>）
        char* m_ptr = strstr(cmd_buf, "M");
        char* d_ptr = strstr(cmd_buf, "D");
        char* s_ptr = strstr(cmd_buf, "S");
        char* v_ptr = strstr(cmd_buf, "V");
        if (m_ptr && d_ptr && s_ptr && v_ptr)
        {
            // 提取参数
            int motor_num = atoi(m_ptr + 1) - 1; // 转为0-11的枚举值
            int dir = atoi(d_ptr + 1);
            int steps = atoi(s_ptr + 1);
            int speed = atoi(v_ptr + 1);
            
            // 参数合法性检查
            if (motor_num < 0 || motor_num > 11)
            {
                USART1_SendString("Error: Motor Num Must 1-12\r\n");
                return;
            }
            if (dir != 0 && dir != 1)
            {
                USART1_SendString("Error: Dir Must 0(Reverse) or 1(Forward)\r\n");
                return;
            }
            if (steps < 1 || steps > 10000)
            {
                USART1_SendString("Error: Steps Must 1-10000\r\n");
                return;
            }
            if (speed < 10 || speed > 500)
            {
                USART1_SendString("Error: Speed Must 10-500\r\n");
                return;
            }
            
            // 执行电机控制
            Motor_Set_Direction((Motor_Num)motor_num, dir);
            Motor_Run_Steps((Motor_Num)motor_num, steps, speed);
        }
        else
        {
            USART1_SendString("Error: Cmd Format Error\r\n");
            USART1_SendString("Correct Format: M<num>D<dir>S<steps>V<speed> (e.g., M1D1S100V50)\r\n");
        }
    }
    else
    {
        // 未到结束符，继续缓存字符
        cmd_buf[cmd_buf_idx++] = ch;
    }
}
