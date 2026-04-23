/* V1.0 LittleFS Port - SDHI SD卡后端 */

#include "lfs.h"
#include "lfs_port.h"
#include "SDHI_SDcard/sdhi_sdcard.h"
#include "hal_data.h"
#include <stdio.h>

/*
 * LittleFS移植到SD卡的说明：
 *
 * 教程中使用SPI Flash（W25QXX），每个扇区4KB，需要手动擦除后写入。
 * SD卡使用512字节扇区，写入前不需要手动擦除（SD卡内部处理）。
 *
 * 关键差异：
 * 1. SD卡扇区大小固定512字节，使用扇区号寻址（不是字节地址）
 * 2. SD卡写入时自动处理擦除，erase回调可以为空操作
 * 3. SD卡使用DMA异步传输，需要等待传输完成回调
 */

/* 外部变量：SDHI异步传输完成标志（定义在sdhi_sdcard.c的回调中设置） */
extern volatile uint32_t g_transfer_complete;

/* LittleFS配置参数 */
#define LFS_SD_BLOCK_SIZE       512     /* 块大小 = SD卡扇区大小 */
#define LFS_SD_READ_SIZE        512     /* 最小读取大小 */
#define LFS_SD_PROG_SIZE        512     /* 最小编程大小 */
#define LFS_SD_CACHE_SIZE       512     /* 缓存大小 */
#define LFS_SD_LOOKAHEAD_SIZE   8       /* 前瞻缓冲区大小（8字节 = 64块前瞻） */
#define LFS_SD_BLOCK_CYCLES     500     /* 擦写均衡周期 */

/**
 * lfs与底层SD卡读数据接口
 * @param  c      lfs配置
 * @param  block  块编号（对应SD卡扇区号）
 * @param  off    块内偏移地址
 * @param  buffer 用于存储读取到的数据
 * @param  size   要读取的字节数
 * @return LFS_ERR_OK 成功, LFS_ERR_IO IO错误
 */
static int lfs_sd_read(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, void *buffer, lfs_size_t size)
{
    fsp_err_t ferr;
    /* 计算SD卡扇区号和扇区数量 */
    uint32_t sector = block * (c->block_size / SDHI_MAX_BLOCK_SIZE) + off / SDHI_MAX_BLOCK_SIZE;
    uint32_t count  = size / SDHI_MAX_BLOCK_SIZE;

    /* 启动异步读取 */
    g_transfer_complete = 0;
    ferr = R_SDHI_Read(&g_sdmmc0_ctrl, (uint8_t *)buffer, sector, count);
    if (FSP_SUCCESS != ferr)
    {
        return LFS_ERR_IO;
    }

    /* 等待DMA传输完成 */
    while (0 == g_transfer_complete)
    {
        ;
    }

    /* 检查传输结果 */
    if (2 == g_transfer_complete)
    {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

/**
 * lfs与底层SD卡写数据接口
 * @param  c      lfs配置
 * @param  block  块编号（对应SD卡扇区号）
 * @param  off    块内偏移地址
 * @param  buffer 待写入的数据
 * @param  size   待写入数据的大小
 * @return LFS_ERR_OK 成功, LFS_ERR_IO IO错误
 */
static int lfs_sd_prog(const struct lfs_config *c, lfs_block_t block,
                       lfs_off_t off, const void *buffer, lfs_size_t size)
{
    fsp_err_t ferr;
    uint32_t sector = block * (c->block_size / SDHI_MAX_BLOCK_SIZE) + off / SDHI_MAX_BLOCK_SIZE;
    uint32_t count  = size / SDHI_MAX_BLOCK_SIZE;

    /* 启动异步写入 */
    g_transfer_complete = 0;
    ferr = R_SDHI_Write(&g_sdmmc0_ctrl, (const uint8_t *)buffer, sector, count);
    if (FSP_SUCCESS != ferr)
    {
        return LFS_ERR_IO;
    }

    /* 等待DMA传输完成 */
    while (0 == g_transfer_complete)
    {
        ;
    }

    /* 检查传输结果 */
    if (2 == g_transfer_complete)
    {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

/**
 * lfs与底层SD卡擦除接口
 *
 * 注意：SD卡写入时内部自动处理擦除，此函数为空操作。
 * 这与SPI Flash不同（SPI Flash必须先擦除再写入）。
 *
 * @param  c     lfs配置
 * @param  block 块编号
 * @return LFS_ERR_OK
 */
static int lfs_sd_erase(const struct lfs_config *c, lfs_block_t block)
{
    /* SD卡不需要手动擦除，写入时自动处理 */
    (void)c;
    (void)block;
    return LFS_ERR_OK;
}

/**
 * lfs与底层SD卡同步接口
 * @param  c lfs配置
 * @return LFS_ERR_OK
 */
static int lfs_sd_sync(const struct lfs_config *c)
{
    (void)c;
    return LFS_ERR_OK;
}

////////////////////////////////////////////////////////
///
/// 静态内存使用方式必须设定这三个缓存
/// 每个缓存大小必须 >= cache_size
///
BSP_ALIGN_VARIABLE(4) static uint8_t read_buffer[LFS_SD_CACHE_SIZE];
BSP_ALIGN_VARIABLE(4) static uint8_t prog_buffer[LFS_SD_CACHE_SIZE];
static uint8_t lookahead_buffer[LFS_SD_LOOKAHEAD_SIZE];

/* LittleFS 句柄 */
lfs_t lfs_sdcard;

/* LittleFS 文件句柄 */
lfs_file_t lfs_file_sdcard;

/* 文件缓存缓冲区（lfs_file_open内部会调用malloc分配缓存，
 * 嵌入式系统堆空间不足会返回LFS_ERR_NOMEM，
 * 使用lfs_file_opencfg提供静态缓存解决此问题） */
BSP_ALIGN_VARIABLE(4) static uint8_t file_buffer[LFS_SD_CACHE_SIZE];
static struct lfs_file_config file_cfg = {
    .buffer = file_buffer,
    .attrs  = NULL,
    .attr_count = 0,
};

/* LittleFS 配置结构体 */
struct lfs_config lfs_cfg =
{
    /* block device operations */
    .read  = lfs_sd_read,
    .prog  = lfs_sd_prog,
    .erase = lfs_sd_erase,
    .sync  = lfs_sd_sync,

    /* block device configuration */
    .read_size      = LFS_SD_READ_SIZE,
    .prog_size      = LFS_SD_PROG_SIZE,
    .block_size     = LFS_SD_BLOCK_SIZE,
    .block_count    = 0,                    /* 运行时根据SD卡容量动态设置 */
    .cache_size     = LFS_SD_CACHE_SIZE,
    .lookahead_size = LFS_SD_LOOKAHEAD_SIZE,
    .block_cycles   = LFS_SD_BLOCK_CYCLES,

    /* 静态内存必须设置这三个缓存 */
    .read_buffer      = read_buffer,
    .prog_buffer      = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

/**
 * LittleFS移植层初始化
 *
 * 流程：
 * 1. 初始化SDHI外设并打开SD卡
 * 2. 初始化SD卡媒体（获取容量信息）
 * 3. 设置LittleFS的block_count
 * 4. 尝试挂载LittleFS，失败则格式化后重新挂载
 *
 * @return 0 成功, 负数失败
 */
int lfs_port_init(void)
{
    int lfs_err;
    fsp_err_t ferr;
    sdmmc_device_t sd_device;

    /* 1. 初始化SDHI外设并打开SD卡 */
    printf("Initializing SD card...\r\n");
    SDCard_Init();

    /* SD卡通电后需要1ms稳定时间 */
    R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);

    /* 2. 初始化SD卡媒体，获取容量信息 */
    ferr = R_SDHI_MediaInit(&g_sdmmc0_ctrl, &sd_device);
    if (FSP_SUCCESS != ferr)
    {
        printf("SD card MediaInit failed: %d\r\n", (int)ferr);
        return -1;
    }

    printf("SD card info: %d sectors, %d bytes/sector, erase unit: %d sectors\r\n",
           (int)sd_device.sector_count,
           (int)sd_device.sector_size_bytes,
           (int)sd_device.erase_sector_count);

    /* 3. 根据SD卡实际容量设置block_count */
    lfs_cfg.block_count = sd_device.sector_count;

    /* 4. 尝试挂载LittleFS */
    printf("Mounting LittleFS...\r\n");
    lfs_err = lfs_mount(&lfs_sdcard, &lfs_cfg);

    if (lfs_err)
    {
        /* 挂载失败（首次使用或文件系统损坏），格式化后重新挂载 */
        printf("Mount failed (err=%d), formatting...\r\n", lfs_err);
        lfs_err = lfs_format(&lfs_sdcard, &lfs_cfg);
        if (lfs_err)
        {
            printf("Format failed: %d\r\n", lfs_err);
            return lfs_err;
        }
        lfs_err = lfs_mount(&lfs_sdcard, &lfs_cfg);
        if (lfs_err)
        {
            printf("Mount after format failed: %d\r\n", lfs_err);
            return lfs_err;
        }
    }

    printf("LittleFS mounted successfully!\r\n");
    return 0;
}

/**
 * LittleFS 启动计数测试
 *
 * 每次启动读取boot_count文件，计数+1后写回。
 * 掉电后计数不会丢失，验证文件系统读写正常。
 */
void lfs_test(void)
{
    uint32_t boot_count = 0;
    int err;

    printf("\r\n=== LittleFS Boot Count Test ===\r\n");

    /* 打开文件（读写模式，不存在则创建，使用静态文件缓存） */
    err = lfs_file_opencfg(&lfs_sdcard, &lfs_file_sdcard, "boot_count",
                           LFS_O_RDWR | LFS_O_CREAT, &file_cfg);
    if (err)
    {
        printf("Failed to open file: %d\r\n", err);
        return;
    }

    /* 读取当前计数值 */
    lfs_file_read(&lfs_sdcard, &lfs_file_sdcard, &boot_count, sizeof(boot_count));
    printf("Current boot_count: %d\r\n", (int)boot_count);

    /* 计数+1 */
    boot_count += 1;
    lfs_file_rewind(&lfs_sdcard, &lfs_file_sdcard);
    lfs_file_write(&lfs_sdcard, &lfs_file_sdcard, &boot_count, sizeof(boot_count));

    /* 关闭文件（确保数据写入SD卡） */
    lfs_file_close(&lfs_sdcard, &lfs_file_sdcard);

    printf("Updated boot_count: %d\r\n", (int)boot_count);
    printf("=== Test Complete ===\r\n");
}
