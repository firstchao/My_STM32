#include "oled/app_oled.h"
 #include "dwt/bsp_dwt.h" 
#include "fonts/bsp_fonts.h"
#include "i2c/bsp_i2c.h"
#include "oled/bsp_i2c_oled.h"
#include "TB6612/bsp_tb6612.h"

extern volatile Motor_State_t motor_state;//电机状态标志
extern volatile int8_t motor_direction_theo;//电机旋转方向标志位
extern volatile float speed_percent;// 存储转换后的速度百分比值

void OLED_DataShow(void)
{
    /*清屏，准备新一轮刷新显示*/
    
    OLED_CLS();
    /*第一行：显示"电机运行参数"*/
    OLED_ShowChinese_F16X16(0, 1, 0);// 第0行 第1列：显示“电(0)”
    OLED_ShowChinese_F16X16(0, 2, 1);// 第0行 第2列：显示“机(1)”
    OLED_ShowChinese_F16X16(0, 3, 2);// 第0行 第3列：显示“运(2)”
    OLED_ShowChinese_F16X16(0, 4, 3);// 第0行 第4列：显示“行(3)”
    OLED_ShowChinese_F16X16(0, 5, 4);// 第0行 第5列：显示“参(4)”
    OLED_ShowChinese_F16X16(0, 6, 5);// 第0行 第6列：显示“数(5)”

    /*第二，三行：显示电机运行状态和速度*/
    //状态标识
    OLED_ShowChinese_F16X16(1, 0, 6);// 第1行 第0列：显示“状(6)”
    OLED_ShowChinese_F16X16(1, 1, 7);// 第1行 第1列：显示“态(7)”
    OLED_ShowString_F8X16(1, 4, (uint8_t *)":");// 显示冒号“:”
    //速度标识
    OLED_ShowChinese_F16X16(2, 0, 13);// 第2行 第0列：显示“速(13)”
    OLED_ShowChinese_F16X16(2, 1, 14);// 第2行 第1列：显示“度(14)”
    OLED_ShowString_F8X16(2, 4, (uint8_t *)":");// 显示冒号“:”

    //显示参数
    if(motor_state == MOTOR_IDLE || motor_state == MOTOR_BRAKING)//电机停止
    {
        //显示状态"停机"
        OLED_ShowChinese_F16X16(1, 3, 8);// 第1行 第3列：显示“停(8)”
        OLED_ShowChinese_F16X16(1, 4, 9);// 第1行 第4列：显示“机(9)”
        //显示速度0
        OLED_ShowString_F8X16(2, 6, (uint8_t *)"0");// 显示冒号“0”
    }
    
    else if(motor_state == MOTOR_RUNNING)
    {
        if(motor_direction_theo)//电机正转
        {
            //显示状态"正转"
            OLED_ShowChinese_F16X16(1, 3, 10);// 第1行 第0列：显示“正(10)”
            OLED_ShowChinese_F16X16(1, 4, 12);// 第1行 第0列：显示“转(12)”
        }
        else//电机反转
        {
            //显示状态"反转"
            OLED_ShowChinese_F16X16(1, 3, 11);// 第1行 第0列：显示“反(11)”
            OLED_ShowChinese_F16X16(1, 4, 12);// 第1行 第0列：显示“转(12)”
        }

        char speed_str[8];
        sprintf(speed_str, "%.2f%%",speed_percent);// 格式化速度百分比字符串
        OLED_ShowString_F8X16(2, 6, (uint8_t *)speed_str);// 显示速度百分比

    }

    // /*延时控制（确保采样周期大于2秒）*/
    // DWT_DelayS(2);  // 延时 2 秒

}

/****end****/
