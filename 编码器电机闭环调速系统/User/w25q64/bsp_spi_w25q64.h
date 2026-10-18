#ifndef __BSP_SPI_W25Q64_H__
#define __BSP_SPI_W25Q64_H__

#include "main.h"
#include "spi/bsp_spi.h"

/*SPI引脚定义*/
#define FLASH_CS_Pin        GPIO_PIN_4
#define FLASH_CS_GPIO_Port  GPIOA

/*******************************宏定义*******************************/
#define FLASH_CS_LOW()          HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)  // 拉低片选，开始通信
#define FLASH_CS_HIGH()         HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET)    // 拉高片选，结束通信

#define W25Q64_MANUFACTDEVICEID_ADDRESS     0x000000    // 芯片制造商及设备ID地址

#define WFLASH_ID   0xEF4017    // W25Q64芯片ID

#define W25Q64_SPI_FLASH_PAGESIZE       256             // W25Q64页大小，单位字


/********************************W25Q64命令定义*******************************/
#define W25Q64_MANUFACTURER_DEVICE_ID           0x90    // 读取制造商和设备ID
#define W25Q64_JEDEC_ID                         0x9F    // 读取JEDEC ID
#define W25Q64_WRITE_ENABLE                     0x06    // 写使能指令
#define W25Q64_READ_STATUS_REGISTER_1           0x05    // 读状态寄存器1
#define W25Q64_READ_STATUS_REGISTER_2           0x35    // 读状态寄存器2
#define W25Q64_SECTOR_ERASE_4KB                 0x20    // 擦除4KB扇区
#define W25Q64_BLOCK_ERASE_64KB                 0xD8    // 擦除64KB块
#define W25Q64_BLOCK_ERASE_32KB                 0x52    // 擦除32KB块
#define W25Q64_CHIP_ERASE                       0xC7    // 全片擦除
#define W25Q64_PAGE_PROGRAM                     0x02    // 页编程指令
#define W25Q64_READ_DATA                        0x03    // 读取数据指令
#define W25Q64_RELEASE_POWER_DOWN_HPM_DEVICE_ID 0xAB    // 释放掉电/高性能模式，并读取设备ID
#define W25Q64_POWER_DOWN                       0xB9    // 进入掉电模式

/********************************函数声明*******************************/
void W25Q64JV_GPIO_Init(void);                      // 初始化FLASH片选引脚
uint16_t W25Q64JV_ReadDeviceID(void);               // 读取W25Q64闪存制造商和设备ID
uint32_t W25Q64JV_ReadJEDECID(void);                // 读取W25Q64芯片JEDEC ID
void W25Q64JV_WriteEnable(void);                    // 发送写使能命令
uint8_t W25Q64JV_WaitForWriteEnd(void);             // 等待写操作完成
void W25Q64JV_BulkErase(void);                      // 批量擦除整个芯片
void W25Q64JV_SectorErase(uint32_t sector_addr);    // 擦除指定扇区（4KB）
void W25Q64JV_WritePage(uint32_t write_addr, uint8_t* buffer, uint16_t size);   //向指定页写入数据
void W25Q64JV_WriteBuffer(uint32_t write_addr, uint8_t* buffer, uint32_t size); //向FLASH写入指定长度的数据
void W25Q64JV_ReadBuffer(uint32_t read_addr, uint8_t* buffer, uint32_t size);   //从指定地址读取数据到缓冲区
void W25Q64JV_PowerDown(void);                      // 进入掉电模式，降低功耗
void W25Q64JV_WakeUp(void);                         // 唤醒FLASH

#endif /*__ GPIO_H__ */
