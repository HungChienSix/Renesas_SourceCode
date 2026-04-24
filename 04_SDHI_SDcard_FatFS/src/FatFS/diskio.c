/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

#include "SDHI_SDcard/sdhi_sdcard.h"

#define DEV_FLASH       0
#define DEV_SDCARD      1

#define SDHI_TIMEOUT_us 100000
static uint32_t timeout_us = SDHI_TIMEOUT_us;
sdmmc_device_t my_sdmmc_device = {0};

extern __IO uint32_t g_transfer_complete ;
extern __IO uint32_t g_card_erase_complete ;
#ifdef CARD_INSERT_DETECT
    extern __IO bool g_card_inserted ;
#endif


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
//	int result;

	switch (pdrv) {
	case DEV_FLASH :
	    stat = RES_OK;

		return stat;

	case DEV_SDCARD :
	    stat = RES_OK;

		return stat;

//	case DEV_RAM :
//		result = RAM_disk_status();
//
//		// translate the reslut code here
//
//		return stat;
//
//	case DEV_MMC :
//		result = MMC_disk_status();
//
//		// translate the reslut code here
//
//		return stat;
//
//	case DEV_USB :
//		result = USB_disk_status();
//
//		// translate the reslut code here
//
//		return stat;
	}

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
	case DEV_FLASH :
	    stat = RES_OK;

		return stat;

	case DEV_SDCARD :
        SDCard_Init();
#ifdef CARD_INSERT_DETECT
        /* 检查卡是否插入 */
        err = R_SDHI_StatusGet(&g_sdmmc0_ctrl, &status);
        assert(FSP_SUCCESS == err);
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
        fsp_err_t err = R_SDHI_MediaInit(&g_sdmmc0_ctrl, &my_sdmmc_device);
        assert(FSP_SUCCESS == err);

        stat = RES_OK;
		return stat;

//	case DEV_RAM :
//		result = RAM_disk_initialize();
//
//		// translate the reslut code here
//
//		return stat;
//
//	case DEV_MMC :
//		result = MMC_disk_initialize();
//
//		// translate the reslut code here
//
//		return stat;
//
//	case DEV_USB :
//		result = USB_disk_initialize();
//
//		// translate the reslut code here
//
//		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;


	switch (pdrv) {
	case DEV_FLASH :
	    res = RES_OK;

		return res;

	case DEV_SDCARD :
        R_SDHI_Read(&g_sdmmc0_ctrl, buff, sector , count); //1 sector == 512 bytes

        /* 添加超时机制，避免无限等待 */
        timeout_us = SDHI_TIMEOUT_us;

        while (g_transfer_complete == 0 && timeout_us>0)
        {
            timeout_us--;
            R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
        }

        if (g_transfer_complete == 0)
        {
            /* 超时处理 */
            printf("[FatFS] SD卡读取超时\r\n");
            return RES_ERROR;
        }

        g_transfer_complete = 0;  // 清除标志
        timeout_us = SDHI_TIMEOUT_us;

        res = RES_OK;
        return res;

//	case DEV_RAM :
//		// translate the arguments here
//
//		result = RAM_disk_read(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
//
//	case DEV_MMC :
//		// translate the arguments here
//
//		result = MMC_disk_read(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
//
//	case DEV_USB :
//		// translate the arguments here
//
//		result = USB_disk_read(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_FLASH :

		res = RES_OK;
		return res;

	case DEV_SDCARD :

		R_SDHI_Write(&g_sdmmc0_ctrl, buff, sector , count);

		/* 添加超时机制，避免无限等待 */
		timeout_us = SDHI_TIMEOUT_us;

		while (g_transfer_complete == 0 && timeout_us>0)
		{
			timeout_us--;
			R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MICROSECONDS);
		}

		if (g_transfer_complete == 0)
		{
			/* 超时处理 */
			printf("[FatFS] SD卡写入超时\r\n");
			return RES_ERROR;
		}

		g_transfer_complete = 0;  // 清除标志
		timeout_us = SDHI_TIMEOUT_us;

		res = RES_OK;
		return res;

//	case DEV_RAM :
//		// translate the arguments here
//
//		result = RAM_disk_write(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
//
//	case DEV_MMC :
//		// translate the arguments here
//
//		result = MMC_disk_write(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
//
//	case DEV_USB :
//		// translate the arguments here
//
//		result = USB_disk_write(buff, sector, count);
//
//		// translate the reslut code here
//
//		return res;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
    case DEV_FLASH :
        switch (cmd) {
            case GET_SECTOR_COUNT:      /* 扇区数量：1024*4096/1024/1024 = 4(MB) */
                      *(DWORD *)buff = 1024;
                      break;
                  case GET_SECTOR_SIZE:       /* 扇区大小  */
                      *(WORD *)buff = 4096;
                      break;
                  case GET_BLOCK_SIZE:        /* 同时擦除扇区个数 */
                      *(DWORD *)buff = 1;
                      break;
        }

        res = RES_OK;
        return res;

    case DEV_SDCARD :
        switch (cmd)
        {
            // 扇区大小。仅当FF_MAX_SS > FF_MIN_SS时，才需要此命令。
            case GET_SECTOR_SIZE:       /* 扇区大小：512(Byte)  */
                *(WORD *)buff = (WORD)my_sdmmc_device.sector_size_bytes;
                break;
            // 擦除块大小（以扇区为单位）
            case GET_BLOCK_SIZE:        /* 同时擦除扇区个数 */
                *(DWORD *)buff = my_sdmmc_device.erase_sector_count;
                break;
            // 可用扇区数
            case GET_SECTOR_COUNT:      /* 扇区数量 */
                *(DWORD *)buff = my_sdmmc_device.sector_count;
                break;
        }

        res = RES_OK;
        return res;

//	case DEV_RAM :
//
//		// Process of the command for the RAM drive
//
//		return res;
//
//	case DEV_MMC :
//
//		// Process of the command for the MMC/SD card
//
//		return res;
//
//	case DEV_USB :
//
//		// Process of the command the USB drive
//
//		return res;
	}

	return RES_PARERR;
}

