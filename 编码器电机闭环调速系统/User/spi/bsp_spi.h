#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "main.h"  


extern SPI_HandleTypeDef hspi2;	// 声明SPI1的句柄变量hspi2，定义在别的文件中，方便多个文件访问SPI1接口

#define FLASH_SCK_Pin           GPIO_PIN_13
#define FLASH_SCK_GPIO_Port     GPIOB
#define FLASH_MISO_Pin          GPIO_PIN_14
#define FLASH_MISO_GPIO_Port    GPIOB
#define FLASH_MOSI_Pin          GPIO_PIN_15
#define FLASH_MOSI_GPIO_Port    GPIOB

/************ 函数声明 ************/
void MX_SPI2_Init(void);

#endif /* __BSP_SPI_H */
