#include "W25Q/app_w25q.h"
#include "spi/bsp_spi.h"
#include "w25q64/bsp_spi_w25q64.h"
#include "TB6612/bsp_tb6612.h"


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






/*****end*****/
