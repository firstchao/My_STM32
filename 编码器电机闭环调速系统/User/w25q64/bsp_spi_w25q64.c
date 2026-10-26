#include "w25q64/bsp_spi_w25q64.h"
#include "TB6612/bsp_tb6612.h"

extern SPI_HandleTypeDef hspi2;

volatile uint8_t Usb_SpeedUpdata_flag = 0;  //串口设置速度模式：速度更新标志位(1:更新; 0:空闲)
volatile int8_t w25q_writeorRead_flag = 0;  //W25Q64JV读写数据标志位(1:写数据; 0:空闲; -1:读数据)
volatile int8_t adc_switch_flah = 0;        //电位器状态切换标志位(1:启用; 0:空闲; -1:禁用)
volatile uint8_t speedupdata_print_flag = 0;//打印更新速度标志位(1:更新; 0:空闲)

volatile uint32_t usb_rx_speedpercent = 0;  //串口命令获取速度百分比(解析命令获取)

extern volatile int8_t motor_direction_rpm;     // 电机实际旋转方向标志位(1: 正转, 0: 反转)

static USB_RX_MotorSpeed Rx_Speed_Buffer;       //USB设置速度缓存区(初始为空)

extern volatile int8_t adc_ban;             //电位器禁用标识(0：启用, 1：禁用)

/**
  * @brief  初始化FLASH片选引脚GPIO
  * @note   配置FLASH_CS引脚为推挽输出，初始为高电平
  * @retval 无
  */
void W25Q64JV_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : FLASH_CS_Pin */
  GPIO_InitStruct.Pin = FLASH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);

}

/**
  * @brief  读取W25Q64闪存制造商和设备ID
  * @retval 16位ID（高8位为制造商ID，低8位为设备ID），读取失败返回0
  */
uint16_t W25Q64JV_ReadDeviceID(void)
{
    uint8_t tx_buf[6] = {0};        // 发送缓存
    uint8_t rx_buf[6] = {0};        // 接收缓存
    uint16_t id;

    tx_buf[0] = W25Q64_MANUFACTURER_DEVICE_ID;// 读取制造商设备ID指令
    tx_buf[1] = (uint8_t)((W25Q64_MANUFACTDEVICEID_ADDRESS >> 16) & 0xFF);  // 地址高8位
    tx_buf[2] = (uint8_t)((W25Q64_MANUFACTDEVICEID_ADDRESS >> 8) & 0xFF);  // 地址中8位
    tx_buf[3] = (uint8_t)(W25Q64_MANUFACTDEVICEID_ADDRESS & 0xFF);          // 地址低8位
    tx_buf[4] = 0x00;  // 哑元字节1
    tx_buf[5] = 0x00;  // 哑元字节2

    FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 6, HAL_MAX_DELAY) != HAL_OK)//发送命令并接收数据
    {
        FLASH_CS_HIGH();//拉高片选，结束通讯
        printf("错误：发送接收制造商/设备ID失败！\r\n");
        return 0;
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯

    id = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];  // 组合接收到的ID高低字节
    return id;
}

/**
  * @brief  读取W25Q64芯片JEDEC ID
  * @retval 24位JEDEC ID，读取失败返回0
  */
uint32_t W25Q64JV_ReadJEDECID(void)
{
    uint8_t tx_buf[4] = {0};        // 发送缓存
    uint8_t rx_buf[4] = {0};        // 接收缓存
    uint32_t id;

    tx_buf[0] = W25Q64_JEDEC_ID;// 读取JEDEC ID 指令
    tx_buf[1] = 0x00;// 哑元字节1
    tx_buf[2] = 0x00;// 哑元字节2
    tx_buf[3] = 0x00;// 哑元字节3

    FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 4, HAL_MAX_DELAY) != HAL_OK)//发送命令并接收数据
    {
        FLASH_CS_HIGH();//拉高片选，结束通讯
        printf("错误：发送接收芯片JEDEC ID失败！\r\n");
        return 0;
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯

    id = ((uint32_t)rx_buf[1] << 16) | ((uint32_t)rx_buf[2] << 8) | rx_buf[3];// 组合JEDEC ID高、中、低字节

    return id;
}

/**
  * @brief  使能写操作，发送写使能命令
  */
 void W25Q64JV_WriteEnable(void)
 {
    uint8_t cmd = W25Q64_WRITE_ENABLE;// 写使能命令

    FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        printf("错误：发送写使能命令失败！\r\n");
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯
 }

/**
  * @brief  等待写操作完成，轮询状态寄存器WIP位
  */
uint8_t W25Q64JV_WaitForWriteEnd(void)
{
    uint8_t cmd = W25Q64_READ_STATUS_REGISTER_1;  // 读取状态寄存器命令
    uint8_t status = 0;

    FLASH_CS_LOW();                                // 拉低片选，开始通信

    if (HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)  // 发送读取状态寄存器命令
    {
        printf("错误：发送读取状态寄存器命令失败！\r\n");
        return 1;
    }

    uint32_t timeout = 0;//程序计时，防止超时
    do {
        if (HAL_SPI_Receive(&hspi2, &status, 1, HAL_MAX_DELAY) != HAL_OK)  // 接收状态寄存器值
        {
            printf("错误：接收状态寄存器失败！\r\n");
            return 1;
        }
    } while ((status & 0x01) == SET && timeout < 100000);                  // 等待写操作完成（WIP位清零）

    FLASH_CS_HIGH();                               // 拉高片选，结束通信
    return 0;
}

 /**
  * @brief  批量擦除整个芯片
  */
 void W25Q64JV_BulkErase(void)
 {
    uint8_t cmd = W25Q64_CHIP_ERASE;// 全片擦除指令

    W25Q64JV_WriteEnable();// 写使能

    FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)// 发送擦除命令
    {
        printf("错误：SPI 发送芯片擦除命令失败\r\n");
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯

    if(W25Q64JV_WaitForWriteEnd())//等待擦除操作完成
    {
      printf("错误：芯片擦除超时\r\n");
      return;
    }
 }

/**
  * @brief  擦除指定扇区（4KB）
  * @param  sector_addr 扇区地址（字节地址）
  */
void W25Q64JV_SectorErase(uint32_t sector_addr)
{
    uint8_t tx_buf[4] = {0};// 发送缓存

    tx_buf[0] = W25Q64_SECTOR_ERASE_4KB;    // 扇区擦除命令
    tx_buf[1] = (sector_addr >> 16) & 0xFF; // 地址高8位
    tx_buf[2] = (sector_addr >>  8) & 0xFF; // 地址中8位
    tx_buf[3] =  sector_addr & 0xFF;        // 地址低8位

    W25Q64JV_WriteEnable();// 写使能

    FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_Transmit(&hspi2, tx_buf, 4, HAL_MAX_DELAY) != HAL_OK)// 发送命令和地址
    {
        printf("错误：SPI 发送扇区擦除命令失败\r\n");
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯

    if(W25Q64JV_WaitForWriteEnd())//等待擦除操作完成
    {
      printf("错误：扇区擦除超时\r\n");
      return;
    }
}

/**
  * @brief  向指定页写入数据，大小不能超过一页
  * @param  write_addr 写入起始地址（必须在同一页内）
  * @param  buffer 待写入的数据指针
  * @param  size 写入数据长度（字节数），不能跨页
  */
void W25Q64JV_WritePage(uint32_t write_addr, uint8_t* buffer, uint16_t size)
{
    // 检查是否跨页，跨页不允许写入
    if(size + (write_addr % W25Q64_SPI_FLASH_PAGESIZE) > W25Q64_SPI_FLASH_PAGESIZE)
        return;

    uint8_t tx_buf[4] = {0};// 发送缓存

    tx_buf[0] = W25Q64_PAGE_PROGRAM;   // 页编程命令
    tx_buf[1] = (write_addr >> 16) & 0xFF; // 地址高8位
    tx_buf[2] = (write_addr >>  8) & 0xFF; // 地址中8位
    tx_buf[3] =  write_addr & 0xFF;        // 地址低8位

    W25Q64JV_WriteEnable();// 写使能

    FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_Transmit(&hspi2, tx_buf, 4, HAL_MAX_DELAY) != HAL_OK)// 发送写命令和地址
    {
        printf("错误：SPI 发送页写入命令失败\r\n");
    }
    if(HAL_SPI_Transmit(&hspi2, buffer, size, HAL_MAX_DELAY) != HAL_OK)// 发送写入数据
    {
        printf("错误：SPI 发送写入数据失败\r\n");
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯

    if(W25Q64JV_WaitForWriteEnd())//等待擦除操作完成
    {
      printf("错误：扇区擦除超时\r\n");
      return;
    }
}

/**
  * @brief  向FLASH写入指定长度的数据，自动处理跨页写入
  * @param  write_addr 写入起始地址
  * @param  buffer 待写入数据指针
  * @param  size 写入数据长度（字节数）
  */
void W25Q64JV_WriteBuffer(uint32_t write_addr, uint8_t* buffer, uint32_t size)
{
    uint32_t write_num = size;                     // 剩余写入字节数
    uint32_t current_addr = write_addr;            // 当前写入地址
    uint32_t offset = write_addr % W25Q64_SPI_FLASH_PAGESIZE;  // 当前地址在页内的偏移量

    // 如果起始地址不是页起始地址，先写入页剩余空间的数据
    if(offset != 0) {
        uint32_t page_remain = W25Q64_SPI_FLASH_PAGESIZE - offset;  // 当前页剩余空间
        if(write_num > page_remain) {
            W25Q64JV_WritePage(current_addr, buffer, page_remain); // 写满当前页剩余空间
            current_addr += page_remain;       // 地址后移
            buffer += page_remain;             // 数据指针后移
            write_num -= page_remain;          // 剩余数据减少
        } else {
            W25Q64JV_WritePage(current_addr, buffer, write_num);   // 数据不足一页，直接写入
            return;
        }
    }

    // 按整页写入剩余数据
    while(write_num > 0) {
        uint16_t write_size = (write_num > W25Q64_SPI_FLASH_PAGESIZE) ? W25Q64_SPI_FLASH_PAGESIZE : write_num;  // 本次写入大小
        W25Q64JV_WritePage(current_addr, buffer, write_size);  // 写入一页数据
        current_addr += write_size;     // 地址后移
        buffer += write_size;           // 数据指针后移
        write_num -= write_size;        // 剩余数据减少
    }
}

/**
  * @brief  从指定地址读取数据到缓冲区
  * @param  read_addr 读取起始地址
  * @param  buffer 存放读取数据的缓冲区指针
  * @param  size 要读取的数据长度（字节数）
  */
void W25Q64JV_ReadBuffer(uint32_t read_addr, uint8_t* buffer, uint32_t size)
{
    uint8_t tx_buf[4] = {0};// 发送缓存
    
    tx_buf[0] = W25Q64_READ_DATA;         // 读取数据指令
    tx_buf[1] = (read_addr >> 16) & 0xFF; // 地址高8位
    tx_buf[2] = (read_addr >>  8) & 0xFF; // 地址中8位
    tx_buf[3] =  read_addr & 0xFF;        // 地址低8位

      FLASH_CS_LOW();//拉低片选，开始通讯
    if(HAL_SPI_Transmit(&hspi2, tx_buf, 4, HAL_MAX_DELAY) != HAL_OK)// 发送写命令和地址
    {
        printf("错误：SPI 发送读取命令失败\r\n");
    }
    if(HAL_SPI_Receive(&hspi2, buffer, size, HAL_MAX_DELAY) != HAL_OK)// 发送写入数据
    {
        printf("错误：SPI 发送读取命令失败\r\n");
    }
    FLASH_CS_HIGH();//拉高片选，结束通讯
} 

/**
  * @brief  唤醒FLASH，退出掉电模式
  */
void W25Q64JV_WakeUp(void)
{
    uint8_t cmd = W25Q64_RELEASE_POWER_DOWN_HPM_DEVICE_ID;  // 唤醒命令
    FLASH_CS_LOW();                                         // 拉低片选，开始通信

    if (HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)  // 发送命令
    {
        printf("SPI 发送唤醒命令失败\r\n");
    }

    FLASH_CS_HIGH();                                        // 拉高片选，结束通信
}

/**
  * @brief  进入掉电模式，降低功耗
  */
void W25Q64JV_PowerDown(void)
{
    uint8_t cmd = W25Q64_POWER_DOWN;  // 掉电命令
    FLASH_CS_LOW();                   // 拉低片选，开始通信

    if (HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)  // 发送命令
    {
        printf("SPI 发送掉电命令失败\r\n");
    }

    FLASH_CS_HIGH();                  // 拉高片选，结束通信
}


/**
  * @brief  向W25Q64JV写入或读取串口设置电机运行参数
  * @note   w25q_writeorRead_flag置1：写入数据
  *         w25q_writeorRead_flag置-1：读取数据并校验
  *         w25q_writeorRead_flag置0：空闲
  */
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
    /*空闲*/
    else
        return 1;
    /*操作完成*/
    w25q_writeorRead_flag = 0;//置0读写标志位
    return 0;//操作完成返回0
}

/**
  * @brief  串口命令控制速度：电位器状态切换保存于读取速度参数
  * @note   写入数据：w25q_writeorRead_flag置1
  *         读取数据并校验：w25q_writeorRead_flag置-1
  *         空闲：w25q_writeorRead_flag置0
  */
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
            printf("电机速度百分比:%d%\r\n", Rx_Speed_Buffer.rx_speepercent);
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
        usb_rx_speedpercent = 0;
    }
    /*置0电位器状态切换标志位*/
    adc_switch_flah = 0;
    
}

/**
  * @brief  串口命令控制速度模式：检测设置速度命令更新
  * @note   该函数在adc_ban = 1时在定时器函数中调用，将串口接收到的速度百分比值换算为供PID算法使用的ADC值
  *         Usb_SpeedUpdata_flag置1：将接收置映射为ADC值并置位数据写入标志位
  *         w25q_writeorRead_flag置0：空闲
  */
uint32_t W25Q_USB_GetValue(void)
{
    uint32_t usb_rx_adcval;
    /*检测到速度更新且电位器已被禁用*/
    if(Usb_SpeedUpdata_flag ==1 && adc_ban == 1)
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

        return usb_rx_speedpercent;
    }

    /*无更新*/
    else
    { 
        usb_rx_speedpercent = Rx_Speed_Buffer.rx_speepercent;//直接读取缓冲区对应值
        Usb_SpeedUpdata_flag = 0;//确保标志位为0
        return usb_rx_speedpercent;
    }
}

/*******END*******/
