/* V1.0 SDHI_SDcard */

#include "SDHI_SDcard/sdhi_sdcard.h"
#include "sys_time/sys_time.h"

sdmmc_device_t my_sdmmc_device = {0};

/* SDHI SD卡初始化函数 */
fsp_err_t SDCard_Init(void)
{
    fsp_err_t err;

    err = R_SDHI_Open(&SDHI_SDCARD_CTRL, &SDHI_SDCARD_CFG);
    if(FSP_SUCCESS != err){
        return err;
    }

#if CARD_INSERT_DETECT_ENABLE
    sdmmc_status_t status;
        /* 检查卡是否插入 */
        err = R_SDHI_StatusGet(&SDHI_SDCARD_CTRL, &status);
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
    err = R_SDHI_MediaInit(&SDHI_SDCARD_CTRL, &my_sdmmc_device);
    return err;
}

__IO uint32_t g_transfer_complete = 0;
__IO uint32_t g_card_erase_complete = 0;

#if CARD_INSERT_DETECT_ENABLE
__IO bool g_card_inserted = false;
#endif

/* 如果启用了卡检测中断，则在发生卡检测事件时调用回调。 */
void sdhi_callback(sdmmc_callback_args_t *p_args)
{
#if CARD_INSERT_DETECT_ENABLE
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
    fsp_err_t err = R_SDHI_Read(&SDHI_SDCARD_CTRL, buff, sector, count);
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

SDCard_ERROR_t SDCard_Write(const uint8_t *buff, uint32_t sector, uint32_t count, uint32_t timeout_us)
{
    g_transfer_complete = 0;
    fsp_err_t err = R_SDHI_Write(&SDHI_SDCARD_CTRL, buff, sector, count);
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

/* SD卡读取速度测量函数 */
uint32_t SDCard_MeasureReadSpeed(void)
{
    uint32_t start_time = SysTime_Get_us();

    /* 读取256块 (128KB) */
    for (uint32_t i = 0; i < 256; i++)
    {
        if (SDCard_Read(g_dest, 2 + i, 1, 1000000) != SDHI_OK)
        {
            return 0;
        }
    }

    uint32_t elapsed_us = SysTime_Elapsed_us(start_time, SysTime_Get_us());
    uint32_t speed_kbps = (128 * 1000000) / elapsed_us;

    // printf("耗时: %lu us, 速度: %lu KB/s\r\n", elapsed_us, speed_kbps);
    return speed_kbps;
}

