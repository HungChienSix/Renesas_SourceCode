/* V1.0 SDHI_SDcard */

#include <SDHI_SDcard/sdhi_sdcard.h>
#include "stdio.h"
#include <FatFs/ff.h>
#include "stdlib.h"
#include "r_timer_api.h"

#include "../TIM_Clock/tim_clock.h"

/* SDHI SD卡初始化函数 */
void SDCard_Init(void)
{
    fsp_err_t err;

    err = R_SDHI_Open(&g_sdmmc0_ctrl, &g_sdmmc0_cfg);
    assert(FSP_SUCCESS == err);
    printf("[SDHI] SDHI_Open\r\n");
}

__IO uint32_t g_transfer_complete = 0;
__IO uint32_t g_card_erase_complete = 0;
// __IO bool g_card_inserted = false;


/* 如果启用了卡检测中断，则在发生卡检测事件时调用回调。 */
void SDHI_Callback(sdmmc_callback_args_t *p_args)
{
//    if (SDMMC_EVENT_CARD_INSERTED == p_args->event)  //卡插入中断
//    {
//        g_card_inserted = true;
//    }
//    if (SDMMC_EVENT_CARD_REMOVED == p_args->event)   //卡拔出中断
//    {
//        g_card_inserted = false;
//    }
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

uint8_t g_dest[SDHI_MAX_BLOCK_SIZE] BSP_ALIGN_VARIABLE(32);  //4字节对齐
uint8_t g_src[SDHI_MAX_BLOCK_SIZE]  BSP_ALIGN_VARIABLE(32);

/* 这是内置的一个测试函数 */
void SDCard_Test(void)
{
    fsp_err_t err;
    sdmmc_device_t my_sdmmc_device = {0};
    uint32_t i;


    /* 初始化要传输到SD卡内的源数组 */
    for (i = 0; i < SDHI_MAX_BLOCK_SIZE; i++)
    {
        g_src[i] = (uint8_t)('A' + (uint8_t)(i % 26));
    }

    /* 检查卡是否插入 */
//    err = R_SDHI_StatusGet(&g_sdmmc0_ctrl, &status);
//    assert(FSP_SUCCESS == err);
//    if (!status.card_inserted)
//    {
//        /* 等待卡插入中断 */
//        while (!g_card_inserted)
//        {
//            printf("[SDHI] 请插入SD卡\r\n");
//            R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
//        }
//        printf("[SDHI] 检测到卡已插入\r\n");
//    }

    /* 设备应在检测到VDD最小值后1ms内准备好接受第一个命令。
       参考SD物理层简化规范6.00版第6.4.1.1节“卡的通电时间”。
    */
    R_BSP_SoftwareDelay(20U, BSP_DELAY_UNITS_MILLISECONDS);

    /* 初始化SD卡。在为SD设备插入卡之前，不应执行此操作。 */
    err = R_SDHI_MediaInit(&g_sdmmc0_ctrl, &my_sdmmc_device);
    assert(FSP_SUCCESS == err);

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

    /* 对比数据 */
    if (strncmp((char*)&g_dest[0], (char*)&g_src[0], SDHI_MAX_BLOCK_SIZE) == 0)
    {
        printf("[SDHI_Test] SD卡读写数据测试成功!\r\n");
    }
    else
    {
        printf("[SDHI_Test] SD卡读写数据测试错误!\r\n");
    }
}

/**
 * @brief 测量SD卡文件读取速度
 * @param filepath 文件路径（如 "1:test.wav"）
 * @param buffer_size 每次读取的缓冲区大小（字节），0表示一次性读取整个文件
 * @return 读取速度（KB/s），失败返回0
 */
uint32_t SDCard_MeasureReadSpeed(const char *filepath, uint32_t buffer_size)
{
    FIL file;
    FRESULT fres;
    UINT bytes_read;
    uint8_t *read_buffer = NULL;
    uint32_t total_bytes = 0;
    uint32_t start_count = 0;
    uint32_t end_count = 0;
    uint32_t elapsed_ms = 0;
    uint32_t speed_kbps = 0;
    fsp_err_t err;
    timer_status_t timer_status;
    timer_info_t timer_info;

    // 打开文件
    fres = f_open(&file, filepath, FA_READ);
    if (fres != FR_OK)
    {
        printf("[SDHI_Speed] 文件打开失败 (错误: %d, 路径: %s)\r\n", fres, filepath);
        return 0;
    }

    // 获取文件大小
    uint32_t file_size = f_size(&file);
    printf("[SDHI_Speed] 文件大小: %lu 字节\r\n", file_size);

    // 如果buffer_size为0，则设置为文件大小
    if (buffer_size == 0 || buffer_size > file_size)
    {
        buffer_size = file_size;
    }

    // 分配缓冲区
    read_buffer = (uint8_t *)malloc(buffer_size);
    if (read_buffer == NULL)
    {
        printf("[SDHI_Speed] 缓冲区分配失败 (请求: %lu 字节)\r\n", buffer_size);
        f_close(&file);
        return 0;
    }

    start_count = TIM_Clock_GetTime();
    printf("[SDHI_Speed] 步骤10: 起始计数值=%lu, 状态=%d\r\n", start_count, timer_status.state);

    // 读取文件
    printf("[SDHI_Speed] 步骤11: 开始读取文件 (大小: %lu 字节)...\r\n", file_size);
    total_bytes = 0;
    uint32_t read_count = 0;
    while (total_bytes < file_size)
    {
        uint32_t bytes_to_read = buffer_size;
        if (total_bytes + bytes_to_read > file_size)
        {
            bytes_to_read = file_size - total_bytes;
        }

        fres = f_read(&file, read_buffer, bytes_to_read, &bytes_read);
        if (fres != FR_OK)
        {
            printf("[SDHI_Speed] 读取失败 (错误: %d)\r\n", fres);
            R_GPT_Stop(&g_timer1_ctrl);
            R_GPT_Close(&g_timer1_ctrl);
            free(read_buffer);
            f_close(&file);
            return 0;
        }

        if (bytes_read == 0)
        {
            printf("[SDHI_Speed] 读取到0字节，退出循环\r\n");
            break;  // 文件读取完毕
        }

        total_bytes += bytes_read;
        read_count++;
    }
    printf("[SDHI_Speed] 步骤12: 文件读取完成, 总共读取 %lu 次\r\n", read_count);

    end_count =TIM_Clock_GetTime();
    printf("[SDHI_Speed] 步骤14: 结束计数值=%lu, 状态=%d\r\n", end_count, timer_status.state);

    // 停止并关闭定时器
    printf("[SDHI_Speed] 步骤15: 停止定时器...\r\n");
    R_GPT_Stop(&g_timer1_ctrl);
    printf("[SDHI_Speed] 步骤16: 关闭定时器...\r\n");
    R_GPT_Close(&g_timer1_ctrl);
    printf("[SDHI_Speed] 步骤17: 定时器已关闭\r\n");

    // 关闭文件
    printf("[SDHI_Speed] 步骤18: 关闭文件...\r\n");
    f_close(&file);
    free(read_buffer);
    printf("[SDHI_Speed] 步骤19: 缓冲区已释放\r\n");

    // 计算经过的计数
    uint32_t elapsed_counts = 0;
    if (end_count >= start_count)
    {
        elapsed_counts = end_count - start_count;
    }
    else
    {
        // 发生了计数器溢出
        elapsed_counts = (0xFFFFFFFF - start_count) + end_count + 1;
    }
    printf("[SDHI_Speed] 步骤20: 经过计数=%lu\r\n", elapsed_counts);

    // 计算经过的时间（毫秒）
    // elapsed_ms = elapsed_counts * 1000 / timer_frequency
    elapsed_ms = elapsed_counts;

    // 计算速度
    if (elapsed_ms > 0)
    {
        speed_kbps = (total_bytes * 1000) / elapsed_ms;  // Bytes/s
    }

    printf("[SDHI_Speed] 测试完成!\r\n");
    printf("[SDHI_Speed] 预期读取: %lu 字节\r\n", file_size);
    printf("[SDHI_Speed] 实际读取: %lu 字节\r\n", total_bytes);
    printf("[SDHI_Speed] 读取匹配: %s\r\n", (total_bytes == file_size) ? "YES" : "NO");
    printf("[SDHI_Speed] 耗时: %lu 毫秒\r\n", elapsed_ms);
    printf("[SDHI_Speed] 读取速度: %lu.%03lu MB/s\r\n",
           speed_kbps / (1024 * 1024),
           (speed_kbps % (1024 * 1024)) / 1024);
    printf("[SDHI_Speed] 读取速度: %lu KB/s\r\n", speed_kbps / 1024);

    return speed_kbps / 1024;  // 返回KB/s
}
