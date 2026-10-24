#include "spi/bsp_spi.h"
	
SPI_HandleTypeDef hspi2;  // SPI2 句柄结构体

/**
  * @brief  SPI2 初始化函数
  * @param  无
  * @retval 无
  */
void MX_SPI2_Init(void)
{
  hspi2.Instance = SPI2;                                // 选择 SPI2 外设
  hspi2.Init.Mode = SPI_MODE_MASTER;                    // 主机模式
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;          // 全双工（2线）模式
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;              // 数据帧长度：8位
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;            // 时钟空闲时为低电平
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;                // 第1个时钟边沿采样
  hspi2.Init.NSS = SPI_NSS_SOFT;                        // 软件管理 NSS 信号
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; // 时钟预分频 = fPCLK / 4
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;               // 数据高位先传输
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;               // 禁用 TI 模式
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // 禁用 CRC 校验
  hspi2.Init.CRCPolynomial = 10;                        // CRC 多项式值（未使用）
  
  if (HAL_SPI_Init(&hspi2) != HAL_OK)                   // 初始化 SPI2
  {
    Error_Handler();                                    // 错误处理
  }
}



/**
  * @brief  SPI2 硬件底层初始化
  * @param  spiHandle: SPI 句柄
  * @retval 无
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if(spiHandle->Instance == SPI2)
  {
    /* 1. 使能 SPI2 时钟 */
   __HAL_RCC_SPI2_CLK_ENABLE();

    /* 2. 使能 GPIOB 时钟 */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /**
      * SPI2 引脚配置：
      * PB13  -> SPI1_SCK
      * PB14  -> SPI1_MISO
      * PB15  -> SPI1_MOSI
      */
    GPIO_InitStruct.Pin = FLASH_SCK_Pin | FLASH_MOSI_Pin; // SCK、MOSI
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;               // 复用推挽输出
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;         // 高速
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = FLASH_MISO_Pin;                 // MISO
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;               // 输入模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;                   // 无上下拉
    HAL_GPIO_Init(FLASH_MISO_GPIO_Port, &GPIO_InitStruct);
  }
}

/**
  * @brief  SPI2 硬件底层反初始化
  * @param  spiHandle: SPI 句柄
  * @retval 无
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{
  if(spiHandle->Instance == SPI2)
  {
    /* 1. 关闭 SPI2 外设时钟 */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /* 2. 释放 GPIO 引脚配置 */
    HAL_GPIO_DeInit(GPIOA, FLASH_SCK_Pin | FLASH_MISO_Pin | FLASH_MOSI_Pin);
  }
}
