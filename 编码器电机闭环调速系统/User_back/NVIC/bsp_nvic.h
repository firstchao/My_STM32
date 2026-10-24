#ifndef BSP_NVIC_H
#define BSP_NVIC_H

#include "main.h"

/*串口命令枚举*/
typedef enum
{
    CMD_BLANK = 0,//空命令,未接收到命令时置位
    CMD_WORK,//运行
    CMD_STOP,//停止
    CMD_TOGGLE,//换向
    CMD_POSITIVE,//正转
    CMD_REVERSE,//反转
    CMD_DEFAULT//未定义命令
}USART_Cmd_t;

#define RX_BUF_LEN    10   //接收数据缓冲区 

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);


#endif
