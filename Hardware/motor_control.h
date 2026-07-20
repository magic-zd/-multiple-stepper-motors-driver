#ifndef __MOTOR_CONTROL_H
#define __MOTOR_CONTROL_H

#include "stm32f10x.h"
// 新增：指令解析依赖的库头文件
#include <string.h>
#include <stdlib.h>

// 电机编号枚举
typedef enum {
    MOTOR_1 = 0,
    MOTOR_2,
    MOTOR_3,
    MOTOR_4,
    MOTOR_5,
    MOTOR_6,
    MOTOR_7,
    MOTOR_8,
    MOTOR_9,
    MOTOR_10,
    MOTOR_11,
    MOTOR_12
} Motor_Num;

// GPIO宏定义（对应最终确认的引脚）
#define MOTOR_ENABLE_PIN    GPIO_Pin_2
#define MOTOR_ENABLE_PORT   GPIOA

// 12路电机STEP/DIR引脚表
typedef struct {
    GPIO_TypeDef* step_port;
    uint16_t      step_pin;
    GPIO_TypeDef* dir_port;
    uint16_t      dir_pin;
} Motor_GPIO_Config;

// 全局变量声明
extern Motor_GPIO_Config motor_gpio_config[12];

// 函数声明（补充指令解析函数）
void Motor_GPIO_Init(void);   // 电机GPIO初始化
void USART1_Init(uint32_t baudrate); // 串口1初始化
//void Delay_ms(uint32_t ms);   // 简易毫秒延时（适配4细分脉冲）
void Motor_Set_Enable(uint8_t enable);          // 全局电机使能/失能
void Motor_Set_Direction(Motor_Num motor, uint8_t dir); // 设置单电机方向
void Motor_Run_Steps(Motor_Num motor, uint32_t steps, uint16_t speed); // 走指定步长
void USART1_SendString(char* str);              // 串口发送字符串
uint8_t USART1_ReceiveChar(void);               // 串口接收单个字符
void USART1_Parse_Cmd(void);                    // 新增：串口指令解析函数

#endif
