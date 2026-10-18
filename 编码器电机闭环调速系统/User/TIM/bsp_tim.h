#ifndef BSP_TIM_H
#define BSP_TIM_H

#include "main.h"

/****TIM引脚定义****/
//TIM3
#define TB6612_PWMA_Pin GPIO_PIN_0
#define TB6612_PWMA_GPIO_Port GPIOB
//TIM4
#define Encoder_CH1_Pin GPIO_PIN_6
#define Encoder_CH1_GPIO_Port GPIOB
#define Encoder_CH2_Pin GPIO_PIN_7
#define Encoder_CH2_GPIO_Port GPIOB

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/****函数声明****/
//TIM3
void TB6612_PWMA_Init(void);
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle);
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* tim_pwmHandle);
//TIM4
void Encoder_Init(void);
void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef* tim_encoderHandle);
void HAL_TIM_Encoder_MspDeInit(TIM_HandleTypeDef* tim_encoderHandle);


#endif
