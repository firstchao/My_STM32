#include "key/bsp_key.h"

/**
  * @brief  初始化按键key1和key2的GPIO引脚
  * @note   配置为下拉输入，上升沿触发中断,并配置中断优先级
  */
void Key_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

     /* GPIO端口时钟使能 */
    __HAL_RCC_GPIOC_CLK_ENABLE();//使能GPIOC端口时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();//使能GPIOA端口时钟

    /*引脚模式配置*/
    //配置 key2 引脚模式
    GPIO_InitStruct.Pin = KEY2_reverse_Pin;//选择PC13
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;//上升沿触发中断
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;//下拉
    HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);
    //配置 key1 引脚模式
    GPIO_InitStruct.Pin = KEY1_switch_Pin;//选择PA0
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;//上升沿触发中断
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;//下拉
    HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);
		
    /*退出中断初始化*/
    //配置key1退出中断
    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);//EXTI0抢占优先级：1
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    //配置key2退出中断
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);//EXTI13抢占优先级：2
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/********end******** */
