#include "MOTOR/app_motor.h"
#include "key/bsp_key.h"
#include "led/bsp_led.h"
#include "TB6612/bsp_tb6612.h"
#include "TIM/bsp_tim.h"
#include "adc/bsp_adc.h"
#include "PID/bsp_pid.h"

extern volatile Motor_State_t motor_state;

void APP_TASK(void)
{
    Key_Config();//初始化按键

    LED_GPIO_Config();//初始化LED

    TB6612_Init();//初始化TB6612

    Encoder_Init();//初始化编码器
	
		motor_state = MOTOR_IDLE;//设置电机状态为停止

    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);//启动TIM1编码器

    RA1_ADC_Start();//初始化 ADC，并开启ADC转换

    PID_Init();
    
}

/****end****/
