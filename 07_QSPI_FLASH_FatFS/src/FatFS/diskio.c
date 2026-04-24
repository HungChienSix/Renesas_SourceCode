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

#define DEV_FLASH       0
#define DEV_SDCARD      1

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case DEV_FLASH :
	    stat = RES_OK;

		return stat;

	case DEV_SDCARD :
	    stat = RES_OK;

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

	switch (pdrv) {
	case DEV_FLASH :
	    stat = RES_OK;

		return stat;

	case DEV_SDCARD :
        SDCard_Init();

        stat = RES_OK;
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


	switch (pdrv) {
	case DEV_FLASH :
	    res = RES_OK;

		return res;

	case DEV_SDCARD :
	    if (SDCard_Read(buff, sector, count, 100000) != 0)
	        return RES_ERROR;

        res = RES_OK;
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

	switch (pdrv) {
	case DEV_FLASH :

		res = RES_OK;
		return res;

	case DEV_SDCARD :
		if (SDCard_Write(buff, sector, count, 100000) != 0)
		    return RES_ERROR;

		res = RES_OK;
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
        SDCard_ioctl(cmd, buff);

        res = RES_OK;
        return res;
	}

	return RES_PARERR;
}
