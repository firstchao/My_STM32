#ifndef __BSP_SPI_H
#define __BSP_SPI_H

#include "main.h"  


extern SPI_HandleTypeDef hspi1;	// 声明SPI1的句柄变量hspi1，定义在别的文件中，方便多个文件访问SPI1接口

#define FLASH_SCK_Pin           GPIO_PIN_5
#define FLASH_SCK_GPIO_Port     GPIOA
#define FLASH_MISO_Pin          GPIO_PIN_6
#define FLASH_MISO_GPIO_Port    GPIOA
#define FLASH_MOSI_Pin          GPIO_PIN_7
#define FLASH_MOSI_GPIO_Port    GPIOA

/************ 函数声明 ************/
void MX_SPI1_Init(void);

#endif /* __BSP_SPI_H */
