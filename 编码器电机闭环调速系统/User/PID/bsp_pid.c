#include "PID/bsp_pid.h"

/* 电机"理论旋转方向"标志位：1 = 正转, 0 = 反转，由 TB6612/bsp_tb6612.c 定义。
 * PID 用它来选择正/反转的方向补偿参数；用理论方向而不是实测方向，
 * 是因为换向过渡(制动 100ms)期间实测方向会短暂失效 */
extern volatile int8_t motor_direction_theo;

/* PID 结构体实例：使用全局变量而不是 static，
 * 便于在 Keil 调试器的 Watch 窗口中在线观察
 * 目标速度、实际速度、偏差、积分量和输出占空比的变化过程 */
PID_TypeDef g_speed_pid;

/* 惰性初始化标志：工程中若未调用 PID_Init()，PID_Calculate() 首次执行时会自动补做一次 */
static uint8_t s_pid_initialized = 0U;

/**
  * @brief  初始化 PID 结构体参数
  * @note   上电后调用一次即可；即使忘记调用，PID_Calculate() 内部也会自动补做一次。
  *         所有参数初值均在 bsp_pid.h 中定义，来源见该文件头部说明。
  * @param  无
  * @retval 无
  */
void PID_Init(void)
{
    /* 运算量清零 */
    g_speed_pid.target      = 0.0f;     /* 目标速度 (r/min)   */
    g_speed_pid.actual      = 0.0f;     /* 实际速度 (r/min)   */
    g_speed_pid.error       = 0.0f;     /* 当前偏差 e(k)      */
    g_speed_pid.last_error  = 0.0f;     /* 上次偏差 e(k-1)    */
    g_speed_pid.integral    = 0.0f;     /* 偏差累加项 Σe(j)   */
    g_speed_pid.feedforward = 0.0f;     /* 前馈项             */
    g_speed_pid.feedback    = 0.0f;     /* 反馈项             */
    g_speed_pid.output      = 0.0f;     /* PID 输出(占空比)   */
    g_speed_pid.output_flt  = 0.0f;     /* 滤波后的输出       */
    g_speed_pid.direction   = 1;        /* 默认正转           */

    /* PID 三个整定参数 */
    g_speed_pid.kp = PID_KP_DEFAULT;
    g_speed_pid.ki = PID_KI_DEFAULT;
    g_speed_pid.kd = PID_KD_DEFAULT;

    /* 输出限幅：占空比取值范围 0~100（对应 TIM3 的 CCR 范围） */
    g_speed_pid.out_max = PID_PWM_DUTY_MAX;
    g_speed_pid.out_min = PID_PWM_DUTY_MIN;

    /* 积分限幅：抗积分饱和，防止电机堵转或目标突变时积分项无限累加 */
    g_speed_pid.integral_max = PID_INTEGRAL_LIMIT;

    s_pid_initialized = 1U;
}

/**
  * @brief  在线修改 PID 三个参数
  * @param  kp: 比例系数  ki: 积分系数  kd: 微分系数
  * @retval 无
  */
void PID_SetParam(float kp, float ki, float kd)
{
    g_speed_pid.kp = kp;
    g_speed_pid.ki = ki;
    g_speed_pid.kd = kd;
}

/**
  * @brief  电位器 ADC 原始值 → 电机目标速度
  * @note   只使用 ADC 的 [PID_ADC_MIN, PID_ADC_MAX] 区间做线性映射，
  *         两端余量用于吸收电位器机械限位、接触抖动和 ADC 噪声；
  *         低于 PID_ADC_MIN 时目标速度为 0，电机停转。
  * @param  adc_val: ADC 原始值(0~4095)
  * @retval 目标速度 speed_target (r/min)
  */
static float PID_AdcToSpeed(uint32_t adc_val)
{
    float adc = (float)adc_val;

    /* 越界保护：ADC 原始值不会超过 12 位满量程 */
    if (adc > PID_ADC_FULL_SCALE)
    {
        adc = PID_ADC_FULL_SCALE;
    }

    /* 低端余量：视为目标速度 0，电机停止 */
    if (adc <= PID_ADC_MIN)
    {
        return 0.0f;
    }

    /* 高端余量：超过上限即按最高目标速度处理 */
    if (adc > PID_ADC_MAX)
    {
        adc = PID_ADC_MAX;
    }

    /* 中间区间线性映射为 0 ~ 目标速度上限 */
    return (adc - PID_ADC_MIN) / (PID_ADC_MAX - PID_ADC_MIN) * PID_SPEED_MAX_RPM;
}

/**
  * @brief  PID 计算，得到 PWM 占空比
  * @note   采用"实测模型前馈 + 离散位置式 PID 反馈 + 方向补偿"的结构：
  *             前馈  u_ff = speed_target / PID_RPM_PER_DUTY
  *             反馈  u_fb = Kp·e(k) + Ki·Σe(j) + Kd·[e(k) - e(k-1)]
  *             合成  u    = u_ff + u_fb
  *             补偿  CCR  = 方向死区 + u × 方向增益
  *         前馈由实测的占空比-转速线性模型直接算出所需占空比，
  *         使电位器全程都能等比例调速；PID 只负责修正模型误差与负载扰动，
  *         因此响应快、超调小。两个方向使用各自的死区与增益，
  *         保证相同电位器位置下正反转转速一致。
  * @param  adc_val:      电位器 ADC 原始值(0~4095)
  * @param  speed_actual: 编码器实测电机转速 (r/min，非负)
  * @retval PWM 占空比，取值 0~100（对应 TIM3 的 CCR，ARR+1 = 100）
  */
uint32_t PID_Calculate(uint32_t adc_val, float speed_actual)
{
    float u;
    float ccr;
    float integral_next;

    /* 0. 惰性初始化：保证本模块被独立调用时参数也是有效的 */
    if (s_pid_initialized == 0U)
    {
        PID_Init();
    }

    /*------------------------------------------------------------------
     * 第一步：将 ADC 原始值换算为电机目标速度 speed_target
     *----------------------------------------------------------------*/
    g_speed_pid.target    = PID_AdcToSpeed(adc_val);
    g_speed_pid.actual    = speed_actual;
    g_speed_pid.direction = motor_direction_theo;

    /* 目标速度为 0（电位器旋到最小端）：直接停机并清除积分与滤波状态，
     * 避免下次启动时残留的积分量造成输出冲击 */
    if (g_speed_pid.target <= 0.0f)
    {
        g_speed_pid.error       = 0.0f - speed_actual;
        g_speed_pid.last_error  = g_speed_pid.error;
        g_speed_pid.integral    = 0.0f;
        g_speed_pid.feedforward = 0.0f;
        g_speed_pid.feedback    = 0.0f;
        g_speed_pid.output      = 0.0f;
        g_speed_pid.output_flt  = 0.0f;

        return 0U;
    }

    /*------------------------------------------------------------------
     * 第二步：前馈——按实测线性模型直接给出所需占空比
     *----------------------------------------------------------------*/
    g_speed_pid.feedforward = g_speed_pid.target / PID_RPM_PER_DUTY;

    /*------------------------------------------------------------------
     * 第三步：位置式 PID 反馈，只修正前馈残差
     *----------------------------------------------------------------*/
    /* 当前偏差 e(k) */
    g_speed_pid.error = g_speed_pid.target - g_speed_pid.actual;

    /* 积分项试探性累加 Σe(j) + e(k)，并做积分限幅（抗积分饱和） */
    integral_next = g_speed_pid.integral + g_speed_pid.error;
    if (integral_next > g_speed_pid.integral_max)
    {
        integral_next = g_speed_pid.integral_max;
    }
    else if (integral_next < -g_speed_pid.integral_max)
    {
        integral_next = -g_speed_pid.integral_max;
    }

    /* 位置式 PID 公式：u(k) = Kp·e(k) + Ki·Σe(j) + Kd·[e(k) - e(k-1)] */
    g_speed_pid.feedback = (g_speed_pid.kp * g_speed_pid.error)
                         + (g_speed_pid.ki * integral_next)
                         + (g_speed_pid.kd * (g_speed_pid.error - g_speed_pid.last_error));

    /* 保存本次偏差，供下一次计算使用 */
    g_speed_pid.last_error = g_speed_pid.error;

    /*------------------------------------------------------------------
     * 第四步：前馈 + 反馈，得到"线性化占空比" u
     *----------------------------------------------------------------*/
    u = g_speed_pid.feedforward + g_speed_pid.feedback;

    /* 输出限幅。若已经饱和且偏差仍指向饱和方向，则撤销本次积分累加
     * （条件积分抗饱和），否则积分会在大偏差期间继续累积，
     * 造成转速冲过头后再缓慢回落，产生明显的超调 */
    if (u > g_speed_pid.out_max)
    {
        u = g_speed_pid.out_max;
        if (g_speed_pid.error > 0.0f)
        {
            integral_next = g_speed_pid.integral;
        }
    }
    else if (u < 0.0f)
    {
        u = 0.0f;
        if (g_speed_pid.error < 0.0f)
        {
            integral_next = g_speed_pid.integral;
        }
    }
    g_speed_pid.integral = integral_next;

    /*------------------------------------------------------------------
     * 第五步：方向补偿——两个方向的启动死区与增益不同，分别补偿
     *----------------------------------------------------------------*/
    if (g_speed_pid.direction != 0)
    {
        ccr = PID_DIR_DZ_FWD + (u * PID_DIR_GAIN_FWD);   /* 正转 */
    }
    else
    {
        ccr = PID_DIR_DZ_REV + (u * PID_DIR_GAIN_REV);   /* 反转 */
    }

    /* 输出限幅：占空比不能超出 CCR 的有效范围 */
    if (ccr > g_speed_pid.out_max)
    {
        ccr = g_speed_pid.out_max;
    }
    else if (ccr < 0.0f)
    {
        ccr = 0.0f;
    }

    /*------------------------------------------------------------------
     * 第六步：输出轻度低通，抑制测速 20ms 刷新带来的台阶
     *----------------------------------------------------------------*/
    g_speed_pid.output_flt += PID_OUTPUT_FILTER_ALPHA * (ccr - g_speed_pid.output_flt);
    g_speed_pid.output = g_speed_pid.output_flt;

    /* 返回可直接写入 CCR 的占空比值 */
    return (uint32_t)g_speed_pid.output;
}

/****end****/
