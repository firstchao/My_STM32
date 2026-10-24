#include "usart/bsp_usart.h"
#include "NVIC/bsp_nvic.h"
#include "TB6612/bsp_tb6612.h"

UART_HandleTypeDef huart1;  // USART1 结构体句柄



/*key1,key2中断发生标志位*/
extern volatile uint8_t key1_flag;
extern volatile uint8_t key2_flag;

extern volatile Motor_State_t motor_state;//电机状态标志位
extern volatile int8_t motor_direction_theo; // 电机旋转方向标志位(1: 正转, 0: 反转)

extern volatile USART_Cmd_t usart_cmd;//串口命令标识
extern uint8_t RxBuff[RX_BUF_LEN];   //接收数据缓冲区

/**
  * @brief  调试串口 UART 初始化函数
  * @note   配置 USART1 的波特率、数据位、停止位、校验位与工作模式
  * @param  无
  * @retval 无
  */
void Debug_UART_Init(void)
{
    // 设置 USART1 相关参数
    huart1.Instance        = USART1;                   // USART1 外设
    huart1.Init.BaudRate   = 115200;                   // 波特率设置
    huart1.Init.WordLength = UART_WORDLENGTH_8B;       // 8 位数据位
    huart1.Init.StopBits   = UART_STOPBITS_1;          // 1 位停止位
    huart1.Init.Parity     = UART_PARITY_NONE;         // 无奇偶校验
    huart1.Init.Mode       = UART_MODE_TX_RX;          // 打开发送和接收
    huart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;      // 无硬件流控
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;   // 16 倍过采样

    // 初始化 UART
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler(); // 初始化失败，进入错误处理函数
    }

    //开启首次中断接收
    HAL_UART_Receive_IT(&huart1, RxBuff, 1);
}


/**
  * @brief  USART1 外设底层初始化（由 HAL_UART_Init 自动调用）
  * @param  uartHandle：UART 句柄
  * @retval 无
  */
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (uartHandle->Instance == USART1)
    {
        // 1. 使能 USART1 时钟
        __HAL_RCC_USART1_CLK_ENABLE();

        // 2. 使能 GPIOA 时钟（PA9、PA10）
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // 3. 配置 USART1_TX（PA9）为复用推挽输出，高速
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // 4. 配置 USART1_RX（PA10）为浮空输入
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        //5. USART1 中断初始化
        HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);// USART1 中断优先级：3
        HAL_NVIC_EnableIRQ(USART1_IRQn);  // 使能 USART1 中断
    }
}


/**
  * @brief  UART MSP 反初始化（释放资源）
  * @param  uartHandle：UART 句柄
  * @retval 无
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
    if (uartHandle->Instance == USART1)
    {
        /*-------------------- 关闭外设时钟 --------------------*/
        __HAL_RCC_USART1_CLK_DISABLE();

        /*-------------------- 释放 GPIO 引脚 --------------------*/
        // PA9（USART1_TX）、PA10（USART1_RX）解除初始化
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

        //USART1 中断取消初始化
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    }
}

/**
  * @brief  重定向 printf 的输出到串口
  * @param  ch 要发送的字符
  * @param  f  文件指针（标准库要求的参数，一般不使用）
  * @retval 返回发送的字符
  */
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
    return ch;
}

/**
  * @brief  检测串口发送命令控制电机状态
  * @note   命令表：STOP(关闭电机);WORK(开启电机);TOGGLE(电机换向);REVERSE(电机反转);POSITIVE(电机正转)
  */
void USART1_MotorControl(void)
{
    if(usart_cmd != CMD_BLANK)//usart1接收到命令
    {
        /****解析串口接收到的命令****/
        /*命令：STOP(执行条件：电机运行)*/
        if(usart_cmd == CMD_STOP)
        {
            if(motor_state == MOTOR_RUNNING)
            {
                key1_flag = 1;//置位电机开关标志位
                printf("已关闭电机\r\n");
            }
            else
            {
                printf("电机已为关闭状态，命令无效\r\n");
            }
        }

        /*命令：WORK(执行条件：电机停止)*/
        else if(usart_cmd == CMD_WORK) 
        {
            if(motor_state == MOTOR_IDLE)
            {
                key1_flag = 1;//置位电机开关标志位
                printf("已开启电机\r\n");
            }
            else
            {
                printf("电机已为运行状态，命令无效\r\n");
            }
        }

        /*命令：TOGGLE(执行条件：电机运行)*/
        else if(usart_cmd == CMD_TOGGLE)
        {
            if(motor_state == MOTOR_RUNNING)//确保电机运行
            {
                key2_flag = 1;//置位电机换向标志位
                printf("已翻转电机转向\r\n");
            }
             else
            {
                printf("电机未运行，命令无效\r\n");
            }
        }

        /*命令：REVERSE(执行条件：电机正转)*/
        else if(usart_cmd == CMD_REVERSE)
        {
            if(motor_state == MOTOR_RUNNING)//确保电机运行
            {
                if(motor_direction_theo)//电机正转
                {
                    key2_flag = 1;//置位电机换向标志位
                    printf("已设置电机旋转方向为：正转\r\n");
                }
                else//电机反转
                {
                    printf("当前电机旋转方向为：反转，命令无效\r\n");
                }
            }
             else
            {
                printf("电机未运行，命令无效\r\n");
            }
            
        }

        /*命令：POSITIVE(执行条件：电机反转)*/
        else if(usart_cmd == CMD_POSITIVE)
        {
            if(motor_state == MOTOR_RUNNING)//确保电机运行
            {
                if(!motor_direction_theo)//电机反转
                {
                    key2_flag = 1;//置位电机换向标志位
                    printf("已设置电机旋转方向为：反转\r\n");
                }
                else//电机正转
                {
                    printf("当前电机旋转方向为：正转，命令无效\r\n");
                }
            }
             else
            {
                printf("电机未运行，命令无效\r\n");
            }
        }

        else if(usart_cmd == CMD_DEFAULT)
        {
            printf("未知命令\r\n");
        }
        printf("\n\n");
    }
    /*置位为空命令,等待下一次调用*/
    usart_cmd = CMD_BLANK;
}

