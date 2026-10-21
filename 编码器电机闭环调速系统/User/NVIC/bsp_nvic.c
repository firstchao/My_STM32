#include "NVIC/bsp_nvic.h"
#include "stm32f1xx_it.h"
#include "key/bsp_key.h"
#include "usart/bsp_usart.h"

/*设置key1,key2中断发生标志位*/
volatile uint8_t key1_flag = 0;// 未发送中断，置0
volatile uint8_t key2_flag = 0;// 未发送中断，置0
 
volatile USART_Cmd_t usart_cmd = CMD_BLANK;//串口命令标识，初始为"空"
uint8_t RxBuff[RX_BUF_LEN] = {0};   //接收数据缓冲区(最多接收10个字节)



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
        //接收到 WORK
        if(Size == 4 && strncmp((char *)RxBuff, "WORK", 4) == 0)
        {
            usart_cmd = CMD_WORK;// 串口命令标识置位"运行"
        }
        //接收到 STOP
        else if(Size == 4 && strncmp((char *)RxBuff, "STOP", 4) == 0)
        {
            usart_cmd = CMD_STOP;// 串口命令标识置位"停止"
        }
        //接收到 TOGGLE
        else if(Size == 6 && strncmp((char *)RxBuff, "TOGGLE", 6) == 0)
        {
            usart_cmd = CMD_TOGGLE;// 串口命令标识置位"换向"
        }
        //接收到 POSITIVE
        else if(Size == 8 && strncmp((char *)RxBuff, "POSITIVE", 8) == 0)
        {
            usart_cmd = CMD_POSITIVE;// 串口命令标识置位"正转"
        }
        //接收到 REVERSE
        else if(Size == 7 && strncmp((char *)RxBuff, "REVERSE", 7) == 0)
        {
            usart_cmd = CMD_REVERSE;// 串口命令标识置位"正转"
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
