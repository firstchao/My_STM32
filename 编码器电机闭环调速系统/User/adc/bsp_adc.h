#ifndef BSP_ADC_H
#define BSP_ADC_H

#include "main.h"

/****ADC引脚定义****/
#define ADC_SIG_Pin GPIO_PIN_6
#define ADC_SIG_GPIO_Port GPIOA

extern ADC_HandleTypeDef hadc1;

/****函数声明****/
void RA1_ADC_Start(void);
void RA1_SIG_Init(void);
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle);
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle);

#endif
