#include "adc/bsp_adc.h"

/*初始化ADC1结构体句柄*/
ADC_HandleTypeDef hadc1;

/**
  * @brief  初始化 ADC，并开启ADC转换
  */
void RA1_ADC_Start(void)
{
    //初始化SIG引脚
    RA1_SIG_Init();

    //启动ADC校准
    HAL_ADCEx_Calibration_Start(&hadc1);

    //开启ADC转换
    HAL_ADC_Start(&hadc1);
}

/**
  * @brief  初始化 ADC1，用于读取电位器模拟输出
  * @note   配置单通道、软件触发、右对齐数据
  */
void RA1_SIG_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* ADC1 配置 */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;         // 单通道扫描
    hadc1.Init.ContinuousConvMode = ENABLE;             // 连续转换
    hadc1.Init.DiscontinuousConvMode = DISABLE;         // 不使用不连续模式
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;   // 软件触发转换
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;         // 数据右对齐
    hadc1.Init.NbrOfConversion = 1;                     // 转换通道数 1
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();// 初始化失败处理
    }

    /* 配置 ADC 通道 */
    sConfig.Channel = ADC_CHANNEL_6;                    // ADC1 通道 6 (PA6)
    sConfig.Rank = ADC_REGULAR_RANK_1;                  // 常规通道序列 1
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;   // 采样时间
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();// 通道配置失败处理
    }

}

/**
  * @brief  ADC MSP 初始化，配置 ADC1 GPIO 和 DMA
  * @param  adcHandle: ADC 句柄指针
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance==ADC1)
    {
        /* ADC1 时钟使能 */
        __HAL_RCC_ADC1_CLK_ENABLE();

        /* 配置 ADC1 输入引脚 PA6 */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        GPIO_InitStruct.Pin = ADC_SIG_Pin;// 选择 PA6
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;// 模拟模式
        HAL_GPIO_Init(ADC_SIG_GPIO_Port, &GPIO_InitStruct);
    }
}

/**
  * @brief  ADC MSP 反初始化，释放 ADC1 GPIO 和 DMA
  * @param  adcHandle: ADC 句柄指针
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
    if(adcHandle->Instance==ADC1)
    {
        /* 禁用 ADC1 时钟 */
        __HAL_RCC_ADC1_CLK_DISABLE();

        /* 反初始化 ADC1 引脚 PA6 */
        HAL_GPIO_DeInit(ADC_SIG_GPIO_Port, ADC_SIG_Pin); 
    }
}



/****end****/
