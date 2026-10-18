#include "USB/app_usb.h"
#include "TB6612/bsp_tb6612.h"

extern volatile Motor_State_t motor_state;//电机状态标志

extern volatile int8_t motor_direction_theo;// 电机理论旋转方向标志位
extern volatile int8_t motor_direction_rpm;// 电机实际旋转方向标志位

extern volatile float speed_percent;// 速度百分比值
extern volatile float motor_speed_rpm; // 电机实际转速

extern volatile uint32_t pwm_duty;//TB6612 PWM占空比

/**
  * @brief  打印电机运行参数
  */
void USB_Data_Print(void)
{
    if(motor_state == MOTOR_IDLE)
    {
        printf("电机未工作\r\n");
        
    }
    else
    {
        //打印按键设置转向
        if(motor_direction_theo)
            printf("目标电机转向：正\r\n");
        else
            printf("目标电机转向：反\r\n");

        //打印电机实际转向
        if(motor_direction_rpm)
            printf("当前电机转向：正\r\n");
        else
            printf("当前电机转向：反\r\n");

        //打印电机速度
        printf("电位器设置速度百分比：%.2f%\r\n",speed_percent);

        printf("电机测速：%.2f r/min\r\n",motor_speed_rpm);

        printf("PWM占空比：%d\r\n",pwm_duty);
        
    }
		printf("\n");
    HAL_Delay(3000);//每隔3s显示一次
}

/****end****/
