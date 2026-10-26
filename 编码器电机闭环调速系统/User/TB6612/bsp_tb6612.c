#include "TB6612/bsp_tb6612.h"
#include "TIM/bsp_tim.h"
#include "adc/bsp_adc.h"
#include "NVIC/bsp_nvic.h"
#include "PID/bsp_pid.h"
#include "led/bsp_led.h"
#include "w25q64/bsp_spi_w25q64.h"

/*key1,key2中断发生标志位*/
extern volatile uint8_t key1_flag;
extern volatile uint8_t key2_flag;

volatile Motor_State_t motor_state = MOTOR_IDLE;//电机状态标志(默认停止)
static volatile GPIO_PinState AIN1_Stata = GPIO_PIN_RESET;//缓存AIN1电平状态(默认为低)
static volatile GPIO_PinState AIN2_Stata = GPIO_PIN_RESET;//缓存AIN2电平状态(默认为低)

/*key1,key2 非阻塞计时消抖标志位*/
static volatile uint8_t key1_debounce = 0;  // key1 消抖计时
static volatile uint8_t key2_debounce = 0;  // key2 消抖计时
static volatile uint8_t key1_trigger = 0;   // key1 消抖后有效触发标志
static volatile uint8_t key2_trigger = 0;   // key2 消抖后有效触发标志

static volatile uint8_t brake_timer = 0;   // 制动计时器
static volatile uint8_t pending_direction = 0; // 记录制动后需要切换的方向
volatile int8_t motor_direction_theo = 0;      // 电机理论旋转方向标志位(1: 正转, 0: 反转)

volatile int8_t adc_ban = 0;    //电位器禁用标识(0：启用, 1：禁用)
uint32_t adc_val = 0;	        // 存储ADC原始数字值
volatile float speed_percent = 0;// 存储转换后的速度百分比值
//volatile float target_speed = 0;// 存储电位器设置的目标速度

static volatile int16_t curr_encoder_count = 0;
static volatile int16_t last_encoder_count = 0;
int32_t total_encoder_count = 0;            // 累计计数，用于计算位置
volatile int8_t motor_direction_rpm = 0;    // 电机实际旋转方向标志位(1: 正转, 0: 反转)
volatile float motor_speed_rpm = 0.0f;      // 存储电机实际转速

/* 测速窗口累加器：
 * 测速值每 SPEED_SAMPLE_MS 毫秒刷新一次，其余时间保持上一次结果，
 * 这样既提高了测速分辨率，又不会给 PID 引入过大的滞后 */
static volatile int32_t  speed_window_count = 0;   // 窗口内累计脉冲数(带符号)
static volatile uint16_t speed_window_ms    = 0;   // 窗口已累计的时间(ms)
static volatile float    speed_rpm_signed   = 0.0f;// 带符号的滤波后转速(r/min)

volatile uint32_t pwm_duty = PWM_DUTY_LOW;//TB6612 PWM占空比

extern volatile uint8_t speedupdata_print_flag;//打印更新速度标志位(1:更新; 0:空闲)

/**
  * @brief  初始化TB6612
  */
void TB6612_Init(void)
{
    TB6612_PWMA_Init();//初始化PWMA

    TB6612_GPIO_Init();//初始化AIN1,AIN2和STBY

    //HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);////开启PWM输出（PB0-TIM3-CH3）

    HAL_GPIO_WritePin(TB6612_STBY_GPIO_Port , TB6612_STBY_Pin , GPIO_PIN_SET);	//STBY拉高使能电机驱动芯片
}

/**
  * @brief  初始化AIN1,AIN2和STBY的GPIO引脚
  * @note   配置为推挽输出,AIN1,AIN2默认低电平,STBY默认输高电平
  */
void TB6612_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();// 使能GPIOA端口时钟

    /* 配置 GPIO 输出电平 */
    HAL_GPIO_WritePin(GPIOA, TB6612_STBY_Pin, GPIO_PIN_SET);//STBY默认高电平
    HAL_GPIO_WritePin(GPIOA, TB6612_AIN1_Pin|TB6612_AIN2_Pin, GPIO_PIN_RESET);//AIN1,AIN2默认低电平

    /* 配置 GPIO 引脚模式 */
    GPIO_InitStruct.Pin = TB6612_AIN1_Pin|TB6612_AIN2_Pin|TB6612_STBY_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 设置为推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;         // 不上拉不下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;// 低速模式
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}


/**
  * @brief  进行按键消抖，并设置电机状态
  */
void TB6612_SetState(void)
{
    /*1.  按键软件消抖 (20ms)*/
    if(key1_flag)//key1 消抖
    {
        key1_debounce++;
        if(key1_debounce >= 20)// 20ms 消抖确认
        {
            key1_debounce = 0;  //置0结束消抖
            key1_flag = 0;      // 清除中断标志
            key1_trigger = 1;   // 触发有效事件
        }
    }
    else{
        key1_debounce = 0;
    }
    
    if(key2_flag)//key2 消抖
    {
        key2_debounce++;
        if(key2_debounce >= 20)// 20ms 消抖确认
        {
            key2_debounce = 0;  //置0结束消抖
            key2_flag = 0;      // 清除中断标志
            key2_trigger = 1;   // 触发有效事件
        }
    }
    else{
        key2_debounce = 0;
    }

    /* 2. 优先级处理：同时按下时，KEY1(开关) > KEY2(换向) */
    if(key1_trigger)//触发 key1(EXIT0) 中断事件————开关
    {
        key1_trigger = 0; 
        key2_trigger = 0;   //清除转向事件标识
        key2_flag = 0;      //清除底层标志，防止残留
			

        if(motor_state == MOTOR_IDLE)//电机停止
        {
            /*开启电机：默认正转*/
            //__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0); 
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);// 关闭PWM输出,避免干扰

            
            // 电机正转：IN1置高，IN2置低
            HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_SET);    
            HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_RESET); 

            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);//重新开启PWM
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, PWM_DUTY_LOW);//设置占空比为最低速

            motor_state = MOTOR_RUNNING;//设置运行标识
            motor_direction_theo = 1;//设置正转标识
            R_LED_ON();//红灯亮——电机运行 
            LED_DirectSet(motor_direction_theo);//LED显示电机正转
        }
        else if(motor_state == MOTOR_RUNNING || motor_state == MOTOR_BRAKING)//电机运行
        {
            /*关闭电机*/
            //__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0); 
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);// 关闭PWM输出,避免干扰

            // 开始制动：IN1,IN2同时置高
            HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_SET);

            motor_state = MOTOR_BRAKING;
            pending_direction = 0;  // 制动后停止
            brake_timer = 0;        // 重置制动计时器
        }
    }
    else if(key2_trigger && motor_state == MOTOR_RUNNING)//触发 key2(EXIT13) 中断事件————转向
    {
        key2_trigger = 0;

        //缓存AIN1，AIN2电平状态
        AIN1_Stata = HAL_GPIO_ReadPin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin);//读取AIN1状态
        AIN2_Stata = HAL_GPIO_ReadPin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin);//读取AIN2状态

        //__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0); 
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);// 关闭PWM输出,避免干扰

        // 开始制动：IN1,IN2同时置高
        HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_SET);
        motor_state = MOTOR_BRAKING;
        pending_direction = 1;  // 标记制动后需要切换方向
        brake_timer = 0;        // 重置制动计时器
    }
    else{//key2按下但不满足条件：电机运行
        key2_trigger = 0; // 不符合换向条件时清除标志
    }

    /*3. 制动状态机与延时过渡*/
    if(motor_state == MOTOR_BRAKING)
    {
        brake_timer++;
        if(brake_timer >= 100)// 100ms 制动过渡确认
        {
            brake_timer = 0;//置0结束制动
            //__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0); 
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);// 关闭PWM输出,避免干扰

            //判断是否需要转向
            if(pending_direction == 1)
            {
                //交换AIN1,AIN2电平状态
                HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, AIN2_Stata);
                HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, AIN1_Stata);

                HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);//重新开启PWM
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, PWM_DUTY_LOW);//设置占空比为最低速

                motor_state = MOTOR_RUNNING;//设置运行标识
                motor_direction_theo = !motor_direction_theo;//翻转电机旋转方向标识
                LED_DirectSet(motor_direction_theo);//翻转LED蓝绿灯

                /*开启PWM设置占空比*/
            }
            else
            {
                // 制动完成转为停止：IN1,IN2置低
                HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_RESET);
                motor_state = MOTOR_IDLE;//设置停止标识
                RGB_ALL_OFF();//关闭所有LED——电机停止
            }
        }
    }
}

/**
  * @brief  读取电位器ADC值并设置电机转速
  * @note   占空比完全由 PID_Calculate 决定：输出限幅、启动死区补偿
  *         都已在 PID 模块内部按实测模型完成。
  *         原代码在 PID 之后又强制 pwm_duty ≥ PWM_DUTY_LOW(20)，
  *         会把 PID 想要的 0~20% 全部抬到 20%，导致低速无法调节、
  *         电位器旋到最小时电机也停不下来，故此处不再重复限幅。
  * @param  无
  * @retval 无
  */
void TB6612_SetSpeed(void)
{
	if(motor_state == MOTOR_RUNNING)
    {
		/*读取ADC值*/
		if(adc_ban)//电位器禁用，启用串口设置速度模式
		{
			pwm_duty = W25Q_USB_GetValue();//解析命令并直接映射为PWM占空比
			speedupdata_print_flag = 1;//置1打印更新数据
		}
       
    else//未禁用电位器，调用PID算法计算
    {
			adc_val = HAL_ADC_GetValue(&hadc1);//获取电位器ADC值
			speed_percent = (float)adc_val / 4095.0f * 100.0f;//映射为速度百分比（仅用于串口显示）
			
			//获取电机实际速度
			TB6612_GetSpeed();

			//PID 计算PWM占空比
			pwm_duty = PID_Calculate(adc_val, motor_speed_rpm);
    }
		

		// 将计算出的占空比写入寄存器
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm_duty);
	}
}

/**
  * @brief  编码器获取并计算电机速度和转向
  * @note   优化要点：
  *         1) 测速窗口由 1ms 改为 SPEED_SAMPLE_MS(20ms)：1ms 采样时 1 个脉冲
  *            就代表 125 r/min，量化误差极大，是测速剧烈跳变的根源；
  *         2) 滤波改为对"带符号转速"进行，最后再取绝对值：
  *            原代码对滤波结果取 fabs，正转(计数递减)时 0.8*v+0.2*raw 变成
  *            减法运算，滤波器退化为差分器，测速值塌缩到真值的 1/9 左右，
  *            造成正反转测速严重不对称；
  *         3) 电机停止时重新同步计数基准并清零测速状态，避免下次启动速度尖峰。
  * @param  无
  * @retval 无
  */
void TB6612_GetSpeed(void)
{
    if(motor_state == MOTOR_RUNNING)
    {
        int16_t delta;

        /* 1. 读取当前编码器原始计数值 */
        curr_encoder_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);

        /* 2. 计算增量（Delta） */
        // 处理16位计数器溢出问题：无符号相减后强转有符号
        delta = (int16_t)(curr_encoder_count - last_encoder_count);
        last_encoder_count = curr_encoder_count;

        /* 3. 累加总计数（用于计算位置/圈数） */
        total_encoder_count += delta;

        /* 4. 计算方向（判定与标志语义保持原样：计数递减=正转） */
        if (delta > 0) {
            motor_direction_rpm = 0;  // 反转
        } else if (delta < 0) {
            motor_direction_rpm = 1;  // 正转
        }

        /* 5. 把增量累加到测速窗口，窗口满了再统一换算转速 */
        speed_window_count += delta;
        speed_window_ms++;

        if (speed_window_ms >= SPEED_SAMPLE_MS)
        {
            /* 转速(RPM) = (窗口内脉冲数 / 一圈总脉冲数) / 窗口时间(s) * 60 */
            float raw_rpm = ((float)speed_window_count * 60.0f) /
                            (ENCODER_PPR * ((float)SPEED_SAMPLE_MS * 0.001f));

            speed_window_count = 0;
            speed_window_ms    = 0;

            /* 6. 一阶低通滤波：对带符号量滤波，正反转行为一致 */
            speed_rpm_signed += SPEED_FILTER_ALPHA * (raw_rpm - speed_rpm_signed);

            /* 7. motor_speed_rpm 保持原有语义：转速大小(恒为非负) */
            motor_speed_rpm = fabsf(speed_rpm_signed);
        }
    }
    else
    {
        /* 电机未运行：重新同步计数基准，清零测速状态 */
        last_encoder_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim4);
        speed_window_count = 0;
        speed_window_ms    = 0;
        speed_rpm_signed   = 0.0f;
        motor_speed_rpm    = 0.0f;
    }
}




/****end****/
