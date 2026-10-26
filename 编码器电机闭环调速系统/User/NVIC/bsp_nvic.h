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
    CMD_ADC_BAN,//禁用电位器
    CMD_ADC_ALLOW,//启用电位器
    CMD_SPEEDSET,//设置速度
    CMD_SPEEDSET_err,//设置速度(命令格式错误)
    CMD_DEFAULT//未定义命令
}USART_Cmd_t;

#define RX_BUF_LEN    32   //接收数据缓冲区 

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);


#endif
