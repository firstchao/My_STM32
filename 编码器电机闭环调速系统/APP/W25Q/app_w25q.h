#ifndef APP_W251_H
#define APP_W251_H

#include "main.h"

#define RXSPEED_SECTOR_ADDR    0x000000    // 扇区起始地址（4KB扇区）
#define TEST_DATA_SIZE      256         // 测试写入大小，1页大小

#define CONFIG_MAGIC       0xAA55BBCC  // 有效数据标志

/*USB控制速度模式参数结构体*/
typedef struct
{
    uint32_t magic_number;//数据有效性校验位
    float rx_speepercent;//速度百分比
    uint32_t rx_adcval;//转换得到的adc值
    //int8_t motor_direction;//电机转向
}USB_RX_MotorSpeed;


void W25Q_Init(void);

#endif
