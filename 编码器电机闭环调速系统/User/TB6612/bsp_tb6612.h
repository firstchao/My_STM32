#ifndef BSP_TB6612_H
#define BSP_TB6612_H

#include "main.h"

/****引脚定义****/
#define TB6612_AIN1_Pin GPIO_PIN_4
#define TB6612_AIN1_GPIO_Port GPIOA
#define TB6612_AIN2_Pin GPIO_PIN_5
#define TB6612_AIN2_GPIO_Port GPIOA
#define TB6612_STBY_Pin GPIO_PIN_7
#define TB6612_STBY_GPIO_Port GPIOA

/* 电机状态枚举 */
typedef enum {
    MOTOR_IDLE = 0,    // 停止状态
    MOTOR_RUNNING,     // 运行状态
    MOTOR_BRAKING      // 制动过渡状态
} Motor_State_t;


#define CONTROL_PERIOD_S 0.001f // 控制周期(1ms)

/* 编码器每圈(输出轴)总计数：
 * 实测标定——12V 满占空比空载时编码器脉冲率为 2950(正转)/2866(反转) 个/s，
 * 规格书 CF4042 给出该电压下空载转速 366 r/min，
 * 故每圈计数 = 脉冲率 ÷ (转速/60) ≈ 470~484，取整为 480
 * （= 电机轴 16 计数/圈 × 减速比 30）。
 * 原值 128 使测速结果被放大约 3.75 倍，是造成转速显示虚高、
 * PID 正反转表现不一致的直接原因 */
#define ENCODER_PPR 480.0f

/* 测速窗口(ms)：1ms 采样时 1 个脉冲就代表 125 r/min，直接换算会使测速值剧烈跳变；
 * 累计 SPEED_SAMPLE_MS 毫秒后再换算，分辨率提高 SPEED_SAMPLE_MS 倍
 * （20ms 窗口下 1 个脉冲 ≈ 6.25 r/min） */
#define SPEED_SAMPLE_MS 20

/* 测速一阶低通滤波系数(新值权重)，0~1，越小越平滑但滞后越大。
 * 取 0.65 时等效时间常数约 17ms，在抑制量化跳变与减少环路滞后之间折中 */
#define SPEED_FILTER_ALPHA 0.65f

#define PWM_DUTY_LOW 20//电机最低速度，占空比20

/****函数声明****/
void TB6612_Init(void);
void TB6612_GPIO_Init(void);
void TB6612_SetState(void);
void TB6612_SetSpeed(void);
void TB6612_GetSpeed(void);


#endif
