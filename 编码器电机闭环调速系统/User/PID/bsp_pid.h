#ifndef BSP_PID_H
#define BSP_PID_H

#include "main.h"

/*==============================================================================
 *  编码器电机闭环调速系统 —— PID 算法模块
 *
 *  功能：
 *      1) 把电位器的 ADC 原始值(0~4095)换算成电机目标速度 speed_target(r/min)；
 *      2) 以 speed_target 与编码器实测速度 speed_actual 为输入，按离散位置式
 *         PID 公式运算，得到所需的 PWM 占空比；
 *      3) 返回值为 uint32_t 占空比，可直接作为
 *         __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty) 的比较值使用。
 *
 *  参数来源（Doc 目录下的规格手册 + 实测标定）：
 *      [1] Doc\CF4042（规格书）.pdf .............................. 直流减速电机参数
 *      [2] Doc\野火小智电位器模块规格手册_20240422.pdf ............ 电位器(ADC)参数
 *      [3] Doc\野火小智TB6612FNG电机驱动模块规格手册_20240629.pdf . 驱动芯片与 PWM 参数
 *      [4] 实测：DAP 烧录开环扫描固件，串口记录占空比-转速曲线（见各宏注释）
 *
 *  使用方法（控制周期与测速周期一致，SysTick 中 1ms 调用一次）：
 *      uint32_t duty = PID_Calculate(adc_val, motor_speed_rpm);
 *      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty);
 *============================================================================*/

/*---------------------- 1. 电位器(ADC)参数 [2] ----------------------*/
/* 《野火小智电位器模块规格手册》：总阻值 10KΩ、旋转角度 280°±5°、线性输出；
 * 推荐工作电压 3.3V~5V，SIG 脚输出 0~3.3V。
 * 本工程 STM32F103 使用 12 位 ADC(PA6 / ADC1_IN6)，采集范围 0~3.3V，
 * 因此 SIG 输出电压 0~3.3V 对应 ADC 原始值 0~4095，
 * 电位器旋钮的旋转角度与 ADC 原始值之间近似为线性关系。 */
#define PID_ADC_FULL_SCALE        4095.0f   /* 12 位 ADC 满量程原始值              */

/* ADC 有效区间：两端各留约 2% 余量。
 * 电位器两端存在机械限位与接触抖动、ADC 采样也带噪声，
 * 若直接用 0~4095 映射，旋钮旋到底时速度会在 0 与最大值之间来回跳；
 * 留出余量后：ADC < PID_ADC_MIN 时目标速度为 0(电机停转)，
 *             ADC > PID_ADC_MAX 时目标速度为上限，
 *             中间区间做线性映射，保证 0~4095 全程速度均匀可控 */
#define PID_ADC_MIN               80.0f     /* 低端余量(约 2%)，低于此值停机     */
#define PID_ADC_MAX               4000.0f   /* 高端余量(约 2.3%)                 */

/*---------------------- 2. 直流减速电机参数 [1] ----------------------*/
/* 《CF4042（规格书）》：额定电压 12V、减速比 1:30、
 *                       空载转速 366±10% r/min、负载转速 330±10% r/min */
#define PID_MOTOR_RATED_VOLTAGE   12.0f     /* 电机额定电压 (V)                  */
#define PID_MOTOR_GEAR_RATIO      30.0f     /* 减速箱减速比 1:30                 */
#define PID_MOTOR_NO_LOAD_RPM     366.0f    /* 额定电压下的空载转速 (r/min)      */
#define PID_MOTOR_LOAD_RPM        330.0f    /* 额定电压下的负载转速 (r/min)      */

/* 目标速度上限：取额定负载转速 330 r/min。
 * 实测 12V 满占空比空载转速约 360~367 r/min，取 330 可在最高速段留出约 10%
 * 的调节余量，避免电位器旋到底时 PID 长期饱和、速度顶到极限而失去闭环调节能力 */
#define PID_SPEED_MAX_RPM         PID_MOTOR_LOAD_RPM

/*---------------------- 3. 编码器测速参数 ----------------------*/
/* 编码器每圈(输出轴)总计数 480 = 电机轴 16 计数/圈 × 减速比 30（实测标定，
 * 见 bsp_tb6612.h 中 ENCODER_PPR 的说明），测速窗口 20ms */
#define PID_ENCODER_PPR           480.0f    /* 编码器转一圈的总计数              */
#define PID_CONTROL_PERIOD_S      0.001f    /* 控制(SysTick)周期 1ms             */
#define PID_SPEED_SAMPLE_MS       20.0f     /* 测速窗口 20ms                     */
/* 测速分辨率：窗口内 1 个脉冲 = 60/(480×0.02) = 6.25 r/min */
#define PID_SPEED_PER_COUNT       6.25f

/*---------------------- 4. PWM 参数 [3] ----------------------*/
/* 《TB6612FNG 电机驱动模块规格手册》：PWM 占空比 Duty = CCR/(ARR+1)，分辨率 1%；
 * 本工程 TIM3：PSC = 72-1，ARR = 100-1
 *   → PWM 频率 = 72MHz / 72 / 100 = 10kHz（低于手册上限 100kHz）
 *   → 占空比分辨率 = 1/(ARR+1) = 1%
 *   → 比较寄存器 CCR 的取值范围为 0~100，即 PID 输出的占空比范围为 0~100 */
#define PID_PWM_DUTY_MAX          100.0f    /* 最大占空比(CCR 上限)              */
#define PID_PWM_DUTY_MIN          0.0f      /* 最小占空比；死区由方向补偿负责    */

/*---------------------- 5. 电机实测模型（前馈与方向补偿） [4] ----------------------*/
/* 实测方法：开环给定占空比，20ms 窗口测速，每点稳定 800ms 后测 500ms。
 * 实测结果（12V 供电）：
 *      正转：转速 ≈ 3.748 × (占空比 - 2.2)     r/min
 *      反转：转速 ≈ 3.662 × (占空比 - 2.0)     r/min
 * 可见：(a) 死区只有约 2% 占空比，远小于原代码强制的最低 20%；
 *       (b) 两个方向的斜率相差约 2.3%，反转略弱，需要做方向补偿；
 *       (c) 转速与占空比基本线性，因此可以用"前馈 + PID 微调"的结构，
 *           让电位器全程都能近似等比例地调速。 */
#define PID_RPM_PER_DUTY          3.70f     /* 线性化模型斜率(r/min 每 1% 占空比) */

/* 方向补偿：把 PID 求出的"线性化占空比" u 还原成实际 CCR
 *     实际占空比 = 方向死区 + u × 方向增益
 * 使两个方向在相同 u 下输出相同的转速 */
#define PID_DIR_DZ_FWD            2.2f      /* 正转启动死区(%占空比)             */
#define PID_DIR_GAIN_FWD          0.987f    /* 正转增益(3.70/3.748)              */
#define PID_DIR_DZ_REV            2.0f      /* 反转启动死区(%占空比)             */
#define PID_DIR_GAIN_REV          1.010f    /* 反转增益(3.70/3.662)              */

/*---------------------- 6. PID 整定参数 ----------------------*/
/* 离散位置式 PID：u(k) = Kp·e(k) + Ki·Σe(j) + Kd·[e(k) - e(k-1)]
 * 控制周期 T = 1ms，Ki、Kd 已把 T 折算在内，整定值均按 1ms 周期给出。
 * 最终占空比 = 前馈 u_ff + PID 反馈 u_fb，再做方向补偿。
 *
 * 前馈已经承担了主要占空比，PID 只需修正残差，因此增益必须取得较小：
 * 实测被控对象为 τ≈120ms 的一阶惯性环节，测速又引入约 40~70ms 等效滞后，
 * 环路增益过大时会在 2Hz 附近产生极限环（实测 Kp=0.5/Ki=0.004 时
 * 转速在 87~220 r/min 之间来回摆动、占空比反复顶到 100）。
 * 按 τ=120ms + 40~70ms 滞后做仿真扫参后取：
 *   Kp = 0.12  ：偏差 1 r/min 对应 0.12% 占空比
 *   Ki = 0.0003：约 6% 占空比/秒 的积分修正速度，用于消除模型误差与负载扰动
 *   Kd = 0.0   ：测速每 20ms 才刷新一次，1ms 控制周期下微分项会在刷新时刻
 *                产生周期性冲击并放大量化噪声，默认关闭；确有需要可小幅增大
 * 该组参数在附加 40ms 与 70ms 滞后的两种极端模型下超调均 < 11%、稳态纹波 < 4 r/min。 */
#define PID_KP_DEFAULT            0.12f     /* 比例系数 (占空比% / r/min)        */
#define PID_KI_DEFAULT            0.0003f   /* 积分系数 (占空比% / (r/min·次))   */
#define PID_KD_DEFAULT            0.0f      /* 微分系数 (占空比%·次 / r/min)     */

/* 积分限幅（抗积分饱和）：限幅值取 Ki × 限幅 = 30% 占空比。
 * 前馈已提供绝大部分占空比，积分只需修正残差，限幅取 30% 已足够，
 * 同时可避免积分在大偏差下过度累积造成超调 */
#define PID_INTEGRAL_LIMIT        (30.0f / PID_KI_DEFAULT)

/*---------------------- 7. 输出一阶低通 ----------------------*/
/* 对最终占空比再做一次轻度平滑，抑制测速刷新带来的台阶 */
#define PID_OUTPUT_FILTER_ALPHA   0.6f      /* 新值权重，1.0 = 不滤波            */

/*---------------------- PID 结构体定义 ----------------------*/
typedef struct
{
    float target;       /* 目标速度 speed_target (r/min)      */
    float actual;       /* 实际速度 speed_actual (r/min)      */
    float error;        /* 当前偏差 e(k) = target - actual    */
    float last_error;   /* 上一次偏差 e(k-1)                  */
    float integral;     /* 偏差累加项 Σe(j)                   */
    float feedforward;  /* 前馈项(线性化占空比)               */
    float feedback;     /* PID 反馈项                         */
    float output;       /* 最终输出，即 PWM 占空比 (0~100)    */
    float output_flt;   /* 低通滤波后的输出                   */

    float kp;           /* 比例系数                            */
    float ki;           /* 积分系数                            */
    float kd;           /* 微分系数                            */

    float out_max;      /* 输出上限                            */
    float out_min;      /* 输出下限                            */
    float integral_max; /* 积分限幅值                          */

    int8_t direction;   /* 本周期使用的旋转方向(1:正转 0:反转) */
} PID_TypeDef;

extern PID_TypeDef g_speed_pid;   /* PID 结构体实例，可在调试器中在线观察 */

/*---------------------- 函数声明 ----------------------*/
void     PID_Init(void);                                      /* 初始化 PID 结构体参数           */
void     PID_SetParam(float kp, float ki, float kd);          /* 在线修改 PID 三个参数           */
uint32_t PID_Calculate(uint32_t adc_val, float speed_actual); /* PID 计算，返回 PWM 占空比(0~100) */

#endif
