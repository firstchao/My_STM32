#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "main.h"

/****按键引脚定义****/
//key1引脚定义
#define KEY1_switch_Pin     GPIO_PIN_0
#define KEY1_GPIO_Port      GPIOA
//key2引脚定义
#define KEY2_reverse_Pin    GPIO_PIN_13 
#define KEY2_GPIO_Port      GPIOC

//函数声明
void Key_Config(void);//初始化按键key1和key2

#endif
