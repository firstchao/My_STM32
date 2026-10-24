#include "W25Q/app_w25q.h"
#include "spi/bsp_spi.h"
#include "w25q64/bsp_spi_w25q64.h"
#include "TB6612/bsp_tb6612.h"

// 发送和接收缓存
// static uint8_t tx_buffer[TEST_DATA_SIZE];
// static uint8_t rx_buffer[TEST_DATA_SIZE];

volatile uint8_t Usb_SpeedUpdata_flag = 0;//串口设置速度模式：速度更新标志位(1:更新; 0:默认)
volatile int8_t w25q_writeorRead_flag = 0;//W25Q64JV读写数据标志位(1:写数据; 0:禁用; -1:读数据)
volatile int8_t adc_switch_flah = 0;//电位器禁用/启用标志位(1:启用; 0:未发生; -1:禁用)

static volatile float usb_rx_speedpercent = 0;//串口命令获取速度百分比(解析命令获取)

extern volatile int8_t motor_direction_rpm;    // 电机实际旋转方向标志位(1: 正转, 0: 反转

static USB_RX_MotorSpeed Rx_Speed_Buffer;//USB设置速度缓存区(初始为空)

/**
  * @brief  初始化W25Q64JV，并校验设备ID
  */
void W25Q_Init(void)
{
    uint32_t id_read;

    // 1. 初始化 W25Q64JV SPI 和 CS 引脚
    MX_SPI2_Init();         //初始化硬件SPI
    W25Q64JV_GPIO_Init();   //初始化片选引脚

    // 2. 读取 JEDEC ID，确认设备连接正常
    id_read = W25Q64JV_ReadJEDECID();// 读取 JEDEC ID
    printf("JEDEC ID: 0x%X\r\n", id_read);
    if (id_read != WFLASH_ID)// 与预设 ID 对比判断设备是否在线
    {
        printf("设备ID校验失败，ID=0x%X\r\n", id_read);
        return;
    }

    // 3. 读取 Manufacturer/Device ID，确认设备在线
    id_read = W25Q64JV_ReadDeviceID();// 读取厂商和设备 ID
    printf("Manufacturer/Device ID: 0x%X\r\n", id_read);

    printf("W25Q64JV设备ID校验完成\r\n");
}

uint8_t W25Q_WriteorRead_Speed(void)
{
    /*写入数据*/
    if(w25q_writeorRead_flag == 1)
    {
        // 1. 检查地址是否扇区对齐 (4KB = 0x1000)
        if ((RXSPEED_SECTOR_ADDR % 0x1000) != 0) 
        {
            printf("错误：写入地址未按 4KB 对齐！\r\n");
            w25q_writeorRead_flag = 0;
            return 1;// 地址为对齐返回1
        }

        // 2. 擦除扇区（准备写入）
        W25Q64JV_SectorErase(RXSPEED_SECTOR_ADDR);// 擦除测试扇区（4KB）

        // 3. 写入数据（跨页写入）
        W25Q64JV_WriteBuffer(RXSPEED_SECTOR_ADDR, (uint8_t *)&Rx_Speed_Buffer, sizeof(USB_RX_MotorSpeed));
    }
    /*读取数据*/
    else if(w25q_writeorRead_flag == -1)
    {
        // 1. 读取写入的数据到缓存
        W25Q64JV_ReadBuffer(RXSPEED_SECTOR_ADDR, (uint8_t *)&Rx_Speed_Buffer, sizeof(USB_RX_MotorSpeed));

        // 2. 数据有效性校验 
        if(Rx_Speed_Buffer.magic_number != CONFIG_MAGIC)
        {
            printf("未读取到有效数据!\r\n");
            w25q_writeorRead_flag = 0;
            return 1;// 数据无效返回1
        }
    }
    else
        return 1;
    /*操作完成*/
    w25q_writeorRead_flag = 0;//置0读写标志位
    return 0;//操作完成返回0
}

void W25Q_Inaugural_GetandSave(void)
{
    /*电位器被禁用——读取上次电机运行参数到缓冲区*/
    if(adc_switch_flah == -1)
    {
        /*手动开启一次读取,并校验数据有效性*/ 
        w25q_writeorRead_flag = -1;//置位-1,开启数据读取

        if(W25Q_WriteorRead_Speed())
        {
            //数据无效，速度默认设为0
            printf("已将速度清零...\r\n");

            //重新初始化缓冲区
            Rx_Speed_Buffer.magic_number = CONFIG_MAGIC;
            Rx_Speed_Buffer.rx_speepercent = 0.0f;
            Rx_Speed_Buffer.rx_adcval = 0;
        }   
        else
        {
            printf("已读取上次退出时的速度设置\r\n");
            printf("电机速度百分比:%.2f%\r\n", Rx_Speed_Buffer.rx_speepercent);
        }

        
    }

    /*电位器被启用——将当前电机运行参数保存到*/
    if(adc_switch_flah == 1)
    {
         /*1. 手动开启一次数据写入，保存缓冲区数据*/
        w25q_writeorRead_flag = 1;//置位1,开启W25Q64写入
    
        if(W25Q_WriteorRead_Speed())
        {
            printf("缓冲区数据保存失败...\r\n");
        }

        /*2. 清理缓冲区，释放内存*/
        memset(&Rx_Speed_Buffer, 0, sizeof(USB_RX_MotorSpeed));
    }

    adc_switch_flah = 0;
    
}

uint32_t W25Q_USB_GetValue(void)
{
    uint32_t usb_rx_adcval;
    /*检测到速度更新*/
    if(Usb_SpeedUpdata_flag)
    {
        // 1. 将接收到的电机速度百分比映射为ADC值
        usb_rx_adcval = (uint32_t)(usb_rx_speedpercent / 100.0f) * 4095;

        // 2. 将数据保存到缓冲区
        Rx_Speed_Buffer.rx_speepercent = usb_rx_speedpercent;//保存速度百分比
        Rx_Speed_Buffer.rx_adcval = usb_rx_adcval;//保存映射ADC值
        //Rx_Speed_Buffer->motor_direction = motor_direction_rpm;//保存电机转向
        Rx_Speed_Buffer.magic_number = CONFIG_MAGIC;//写入校验位

        // 3. 将数据写入存储器
        w25q_writeorRead_flag = 1;//置位1,开启W25Q64写入

        return usb_rx_adcval;
    }

    /*无更新*/
    else
    { 
        usb_rx_adcval = Rx_Speed_Buffer.rx_adcval;//直接读取缓冲区对应值
        Usb_SpeedUpdata_flag = 0;//确保标志位为0
        return usb_rx_adcval;
    }
}


/*****end*****/
