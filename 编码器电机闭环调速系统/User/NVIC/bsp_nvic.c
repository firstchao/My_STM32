#include "NVIC/bsp_nvic.h"
#include "stm32f1xx_it.h"
#include "key/bsp_key.h"
#include "usart/bsp_usart.h"

/*设置key1,key2中断发生标志位*/
volatile uint8_t key1_flag = 0;// 未发送中断，置0
volatile uint8_t key2_flag = 0;// 未发送中断，置0
 
volatile USART_Cmd_t usart_cmd = CMD_BLANK;//串口命令标识，初始为"空"
uint8_t RxBuff[RX_BUF_LEN] = {0};   //接收数据缓冲区(最多接收10个字节)

extern volatile uint32_t usb_rx_speedpercent;  //串口命令获取速度百分比(解析命令获取)
extern volatile uint8_t Usb_SpeedUpdata_flag;  //串口设置速度模式：速度更新标志位(1:更新; 0:默认)

/**
  * @brief  检测按键是否按下，并设置标志位
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == KEY1_switch_Pin)//key1按下
    {
        key1_flag = 1;
        return;
    }

    if(GPIO_Pin == KEY2_reverse_Pin)//key2按下
    {
        key2_flag = 1;
        return;
    }
   
}

/**
  * @brief  检测串口是否接收到数据，并设置标志位
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart -> Instance == USART1)//USART1接收数据
    {
        /*1.添加字符串结束符*/
        RxBuff[Size] = '\0';

        /*2.解析数据，置位命令标识*/
        /*电机按键控制模拟*/
        //接收到 MOTOR_WORK
        if(Size == 10 && strncmp((char *)RxBuff, "MOTOR_WORK", 10) == 0)
        {
            usart_cmd = CMD_WORK;// 串口命令标识置位"运行"
        }
        //接收到 STOP
        else if(Size == 10 && strncmp((char *)RxBuff, "MOTOR_STOP", 10) == 0)
        {
            usart_cmd = CMD_STOP;// 串口命令标识置位"停止"
        }
        //接收到 TOGGLE
        else if(Size == 12 && strncmp((char *)RxBuff, "MOTOR_TOGGLE", 12) == 0)
        {
            usart_cmd = CMD_TOGGLE;// 串口命令标识置位"换向"
        }
        //接收到 POSITIVE
        else if(Size == 14 && strncmp((char *)RxBuff, "MOTOR_POSITIVE", 14) == 0)
        {
            usart_cmd = CMD_POSITIVE;// 串口命令标识置位"正转"
        }
        //接收到 REVERSE
        else if(Size == 13 && strncmp((char *)RxBuff, "MOTOR_REVERSE", 13) == 0)
        {
            usart_cmd = CMD_REVERSE;// 串口命令标识置位"正转"
        }
        /*串口控制电机速度*/
        //接收到 ADC_BAN
        else if(Size == 7 && strncmp((char *)RxBuff, "ADC_BAN", 7) == 0)
        {
            usart_cmd = CMD_ADC_BAN;// 串口命令标识置位"禁用电位器"
        }
        //接收到 ADC_ALLOW
        else if(Size == 9 && strncmp((char *)RxBuff, "ADC_ALLOW", 9) == 0)
        {
            usart_cmd = CMD_ADC_ALLOW;// 串口命令标识置位"启用电位器"
        }
        //接收到 MOTOR_SPEED:XXX(XXX数据长度为1~3位)
        else if(Size > 12 && Size <= 15  && strstr((char *)RxBuff, "MOTOR_SPEED:") != NULL)
        {
            if(sscanf((char *)RxBuff, "MOTOR_SPEED:%d", &usb_rx_speedpercent) == 1)//读取速度百分比
            {
                usart_cmd = CMD_SPEEDSET;// 串口命令标识置位"设置速度"
            }
            else
            {
                usart_cmd = CMD_SPEEDSET_err;// 串口命令标识置位"设置速度(错误)"
            }
        }
        //接收区已满且命令未知
        else if(Size > 0) 
        {
            usart_cmd = CMD_DEFAULT;// 串口命令标识置位"未定义命令"
        }
        /*3.清空缓冲区，防止上一帧残留数据干扰*/
        memset(RxBuff, 0, sizeof(RxBuff));//清空接收缓冲区
    }
    /*4. 重新开启 DMA + IDLE 接收，准备接收下一帧*/
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RxBuff, RX_BUF_LEN);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 再次关掉半满中断
}

/****end****/
