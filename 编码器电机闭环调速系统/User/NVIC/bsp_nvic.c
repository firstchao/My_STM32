#include "NVIC/bsp_nvic.h"
#include "stm32f1xx_it.h"
#include "key/bsp_key.h"
#include "TB6612/bsp_tb6612.h"
#include "TIM/bsp_tim.h"

/*设置key1,key2中断发生标志位*/
volatile uint8_t key1_flag = 0;//未发送中断，置0
volatile uint8_t key2_flag = 0;//未发送中断，置0

/**
  * @brief  检测按键是否按下，并设置标志位
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == KEY1_switch_Pin)
    {
        key1_flag = 1;
        return;
    }

    if(GPIO_Pin == KEY2_reverse_Pin)
    {
        key2_flag = 1;
        return;
    }
   
}


/****end****/
