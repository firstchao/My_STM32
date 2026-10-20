#include "TIM/bsp_tim.h"

/*TIM3，TIM4结构体句柄*/
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

/**
  * @brief  初始化 TB6612 PWM 输出（TIM3_CH3）
  * @note   PWM 频率 50Hz（20ms），初始占空比 0
  */
void TB6612_PWMA_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72-1;                                // 定时器预分频
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;                // 向上计数模式
    htim3.Init.Period = 100-1;                                  // 自动重装载值
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;          // 时钟分频
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;// 使能 ARR 预装载
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief  初始化 TIM4 编码模式（TIM4_CH1,CH2）
  * @note   
  */
void Encoder_Init(void)
{
  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 4-1;// 定时器预分频
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;// 向上计数模式
  htim4.Init.Period = 65535;// 自动重装载值
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;// 时钟分频
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;// 失能 ARR 预装载
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;// 编码器模式3
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;// 
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim4, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

/*TIM3 PWM模式 HAL函数*/
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle)
{
    if(tim_pwmHandle->Instance==TIM3)
    {
        /* TIM3 端口时钟使能 */
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(timHandle->Instance==TIM3)
    {
        /* GPIOB 时钟使能 */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        
        /* 配置 TIM3 通道3 引脚 PB0 */
        GPIO_InitStruct.Pin = TB6612_PWMA_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(TB6612_PWMA_GPIO_Port, &GPIO_InitStruct);
    }

}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* tim_pwmHandle)
{
    if(tim_pwmHandle->Instance==TIM3)
    {
        /* 禁用 TIM3 时钟 */
        __HAL_RCC_TIM3_CLK_DISABLE();

    }
}

/*TIM1 编码器模式 HAL函数*/
void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef* tim_encoderHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(tim_encoderHandle->Instance==TIM4)
  {
    /* TIM4 端口时钟使能 */
    __HAL_RCC_TIM4_CLK_ENABLE();

    /* GPIOB 时钟使能 */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /* 配置 TIM4 通道1 引脚 PB6, 通道2 引脚 PB7*/
    GPIO_InitStruct.Pin = Encoder_CH1_Pin|Encoder_CH2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  }
}

void HAL_TIM_Encoder_MspDeInit(TIM_HandleTypeDef* tim_encoderHandle)
{

  if(tim_encoderHandle->Instance==TIM4)
  {
    /* 禁用 TIM4 时钟 */
    __HAL_RCC_TIM4_CLK_DISABLE();

    /* 配置 TIM4 通道1 引脚 PB6, 通道2 引脚 PB7*/
    HAL_GPIO_DeInit(GPIOB, Encoder_CH1_Pin|Encoder_CH2_Pin);
  }
}

/***end****/
