/* V1.0 SDHI_SDcard */

#include "SDHI_SDcard/sdhi_sdcard.h"
#include "sys_time/sys_time.h"
#include "stdio.h"

sdmmc_device_t my_sdmmc_device = {0};

/* SDHI SD卡初始化函数 */
fsp_err_t SDCard_Init(void)
{
    fsp_err_t err;

    err = R_SDHI_Open(&g_sdmmc0_ctrl, &g_sdmmc0_cfg);
    if(FSP_SUCCESS != err){
        return err;
    }

#ifdef CARD_INSERT_DETECT
        /* 检查卡是否插入 */
        err = R_SDHI_StatusGet(&g_sdmmc0_ctrl, &status);
        if(FSP_SUCCESS != err){
            return err;
        }
        if (!status.card_inserted)
        {
            /* 等待卡插入中断 */
            while (!g_card_inserted)
            {
                printf("\r\n请插入SD卡\r\n");
                R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
            }
            printf("\r\n检测到卡已插入\r\n");
        }
#endif

    /* 设备应在检测到VDD最小值后1ms内准备好接受第一个命令。
        参考SD物理层简化规范6.00版第6.4.1.1节“卡的通电时间”。
    */
    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);

    /* 初始化SD卡。在为SD设备插入卡之前，不应执行此操作。 */
    err = R_SDHI_MediaInit(&g_sdmmc0_ctrl, &my_sdmmc_device);
    return err;
}

__IO uint32_t g_transfer_complete = 0;
__IO uint32_t g_card_erase_complete = 0;

#ifdef CARD_INSERT_DETECT
    __IO bool g_card_inserted = false;
#endif

/* 如果启用了卡检测中断，则在发生卡检测事件时调用回调。 */
void sdhi_callback(sdmmc_callback_args_t *p_args)
{
#ifdef CARD_INSERT_DETECT
    if (SDMMC_EVENT_CARD_INSERTED == p_args->event)  //卡插入中断
    {
        g_card_inserted = true;
    }
    if (SDMMC_EVENT_CARD_REMOVED == p_args->event)   //卡拔出中断
    {
        g_card_inserted = false;
    }
#endif
    if (SDMMC_EVENT_ERASE_COMPLETE == p_args->event)  //擦除完成
    {
        g_card_erase_complete = 1;
    }
    if (SDMMC_EVENT_ERASE_BUSY == p_args->event)  //擦除超时
    {
        g_card_erase_complete = 2;
    }
    if (SDMMC_EVENT_TRANSFER_COMPLETE == p_args->event)  //读取或写入完成
    {
        g_transfer_complete = 1;
    }
    if(SDMMC_EVENT_TRANSFER_ERROR ==  p_args->event)
    {
        g_transfer_complete = 2;
    }

}

uint8_t g_dest[SDHI_MAX_BLOCK_SIZE] BSP_ALIGN_VARIABLE(4);  //4字节对齐
uint8_t g_src[SDHI_MAX_BLOCK_SIZE]  BSP_ALIGN_VARIABLE(4);

SDCard_ERROR_t SDCard_Read(uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us)
{
    g_transfer_complete = 0;
    fsp_err_t err = R_SDHI_Read(&g_sdmmc0_ctrl, buff, sector, count);
    if(FSP_SUCCESS != err){
        return SDHI_Read_Err;
    }

    while (g_transfer_complete == 0 && timeout_us > 0)
    {
        timeout_us--;
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
    }

    if (g_transfer_complete != 1)
    {
        return SDHI_Read_Timeout;
    }

    return SDHI_OK;
}

uint8_t SDCard_Write(const uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us)
{
    g_transfer_complete = 0;
    fsp_err_t err = R_SDHI_Write(&g_sdmmc0_ctrl, buff, sector, count);
    if(FSP_SUCCESS != err){
        return SDHI_Write_Err;
    }

    while (g_transfer_complete == 0 && timeout_us > 0)
    {
        timeout_us--;
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
    }

    if (g_transfer_complete != 1)
    {
        return SDHI_Write_Timeout;
    }

    return SDHI_OK;
}

void SDCard_ioctl(uint8_t cmd, void *buff)
{
    switch (cmd)
    {
        case 1:     /* GET_SECTOR_COUNT */
            *(uint32_t *)buff = my_sdmmc_device.sector_count;
            break;
        case 2:     /* GET_SECTOR_SIZE */
            *(uint16_t *)buff = (uint16_t)my_sdmmc_device.sector_size_bytes;
            break;
        case 3:     /* GET_BLOCK_SIZE */
            *(uint32_t *)buff = my_sdmmc_device.erase_sector_count;
            break;
    }
}

void SDCard_Operation(void)
{
    fsp_err_t err;
    uint32_t i;

    /* 初始化要传输到SD卡内的源数组 */
    for (i = 0; i < SDHI_MAX_BLOCK_SIZE; i++)
    {
        g_src[i] = (uint8_t)('A' + (uint8_t)(i % 26));
    }

    /* 写入数据 */
    err = R_SDHI_Write(&g_sdmmc0_ctrl, g_src, 1, 1);
    assert(FSP_SUCCESS == err);
    while (g_transfer_complete == 0);
    g_transfer_complete = 0;

    /* 读出数据 */
    err = R_SDHI_Read(&g_sdmmc0_ctrl, g_dest, 1, 1);
    assert(FSP_SUCCESS == err);
    while (g_transfer_complete == 0);
    g_transfer_complete = 0;

    /* 打印数据 */
    for (i = 0; i < SDHI_MAX_BLOCK_SIZE; i++)
    {
        if (i % 26 == 0)
            printf(" ");
        printf("%c", g_dest[i]);
    }

    /* 对比数据 */
    if (strncmp((char*)&g_dest[0], (char*)&g_src[0], SDHI_MAX_BLOCK_SIZE) == 0)
    {
        printf("\r\nSD卡读写数据成功!\r\n");
    }
    else
    {
        printf("\r\nSD卡读写数据错误!\r\n");
    }
}

/* SD卡读取速度测量函数 - 使用系统时钟计算时间 */
uint32_t SDCard_MeasureReadSpeed(void)
{
    fsp_err_t err;
    uint32_t start_time, end_time;
    uint32_t elapsed_us;
    float elapsed_sec;
    uint32_t speed_kbps;
    uint32_t total_bytes;
    uint32_t num_blocks;
    uint32_t start_block;

    /* 测试配置：读取256个块，每个512字节，共128KB */
    num_blocks = 256;
    start_block = 2;  // 从第2个扇区开始（避开可能的FAT表）
    total_bytes = num_blocks * SDHI_MAX_BLOCK_SIZE;  // 512 * 256 = 131072 字节 = 128 KB

    printf("\r\n正在测量SD卡读取速度...\r\n");
    printf("读取数据量: %d KB\r\n", total_bytes / 1024);

    /* 开始计时 */
    start_time = SysTime_Get_us();

    /* 连续读取多个数据块 */
    for (uint32_t i = 0; i < num_blocks; i++)
    {
        g_transfer_complete = 0;
        err = R_SDHI_Read(&g_sdmmc0_ctrl, g_dest, start_block + i, 1);
        if (err != FSP_SUCCESS)
        {
            printf("读取错误! 块号: %d, 错误码: %d\r\n", start_block + i, err);
            return 0;
        }

        /* 等待读取完成 */
        while (g_transfer_complete == 0)
        {
            /* 等待传输完成 */
        }

        if (g_transfer_complete == 2)
        {
            printf("传输错误! 块号: %d\r\n", start_block + i);
            return 0;
        }
    }

    /* 结束计时 */
    end_time = SysTime_Get_us();
    elapsed_us = SysTime_Elapsed_us(start_time, end_time);

    /* 计算耗时（秒） */
    elapsed_sec = (float)elapsed_us / 1000000.0f;

    /* 计算速度 (KB/s) */
    speed_kbps = (uint32_t)(total_bytes / elapsed_sec / 1024);

    printf("读取完成! 耗时: %d us\r\n", elapsed_us);
    printf("读取速度: %d KB/s\r\n", speed_kbps);

    return speed_kbps;
}

