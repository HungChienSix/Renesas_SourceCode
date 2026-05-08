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
// #include "QSPI_Flash/bsp_qspi_flash.h"
#include "SDHI_SDcard/sdhi_sdcard.h"

#define DEV_FLASH        0
#define DEV_SDCARD       1

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
    stat = RES_OK;
    
	switch (pdrv) {
	case DEV_FLASH :
#ifdef FLASH_FatFS
		if(FSP_SUCCESS != QSPI_Flash_WaitForWriteEnd()){ //等待Flash芯片内部操作完成
		    stat = RES_ERROR;
		}
#endif
		return stat;

	case DEV_SDCARD :
		return stat;
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
	stat = RES_OK;
	
	switch (pdrv) {
	case DEV_FLASH :
#ifdef FLASH_FatFS
		if(FSP_SUCCESS != QSPI_Flash_Init()){
		    stat = STA_NOINIT;
		}
#endif
		return stat;

	case DEV_SDCARD :
#if SD_CARD_ENABLE
        if(FSP_SUCCESS != SDCard_Init()){
            stat = STA_NOINIT;
        }
#endif
		return stat;
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
    res = RES_OK;

	switch (pdrv) {
	case DEV_FLASH :
#ifdef FLASH_FatFS
		if(FSP_SUCCESS != QSPI_Flash_BufferRead(buff, sector<<12, count<<12)){ //1 sector == 4096 bytes
		    res = RES_ERROR;
		}
#endif
		return res;

	case DEV_SDCARD :
#if SD_CARD_ENABLE
        if(SDHI_OK != SDCard_Read(buff, sector, count, 100000)){
            res = RES_ERROR;
        }
#endif
        return res;
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
    res = RES_OK;

	switch (pdrv) {
	case DEV_FLASH :
#ifdef FLASH_FatFS
	    uint32_t write_addr = sector << 12;
		QSPI_Flash_SectorErase(write_addr);
		QSPI_Flash_BufferWrite(buff, write_addr, count<<12);
#endif
		return res;

	case DEV_SDCARD :
#if SD_CARD_ENABLE
        if(SDCard_Write(buff, sector, count, 100000) == 0){
            res = RES_OK;
        }
        else{
            res = RES_ERROR;
        }
#endif
		return res;
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
    res = RES_OK;

	switch (pdrv) {
    case DEV_FLASH :
#ifdef FLASH_FatFS
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
#endif

        return res;

    case DEV_SDCARD :
#if SD_CARD_ENABLE
        SDCard_ioctl(cmd, buff);
#endif
        return res;
	}

	return RES_PARERR;
}
