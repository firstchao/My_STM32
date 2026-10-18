#ifndef __BSP_I2C_H__
#define __BSP_I2C_H__

#include "main.h"

extern I2C_HandleTypeDef hi2c1;  // 声明I2C1的句柄变量hi2c1，定义在别的文件中，方便多个文件访问I2C1接口


/************ 函数声明 ************/
void MX_I2C1_Init(void);  


#endif /* __I2C_H__ */

