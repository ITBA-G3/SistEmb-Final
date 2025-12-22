/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "source/drivers/SD/sd.h"
#include <string.h>

static sd_card_t sd_card;
static DSTATUS sd_stat = STA_NOINIT;

/* Buffer alineado para DATPORT (512 bytes) */
static uint32_t sd_bounce[SD_BLOCK_SIZE / 4] __attribute__((aligned(4)));


/* Definitions of physical drive number for each drive */
#define DEV_MMC		0	/* Example: Map MMC/SD card to physical drive 1 */
//#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
//#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv)
{
    if (pdrv != DEV_MMC)
        return STA_NOINIT;

    return sd_stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE pdrv)
{
    if (pdrv != DEV_MMC)
        return STA_NOINIT;

    sd_error_t err = sd_init(&sd_card);
    if (err != SD_OK) {
        sd_stat = STA_NOINIT;
        return sd_stat;
    }

    sd_stat = 0;   // drive ready
    return sd_stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

static volatile sd_error_t g_last_sd_err;
static volatile DWORD g_last_sector;
static volatile UINT g_last_count;

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != DEV_MMC || count == 0) return RES_PARERR;
    if (sd_stat & STA_NOINIT) return RES_NOTRDY;

    g_last_sector = sector;
    g_last_count  = count;

    while (count--)
    {
        sd_error_t e;

        if (((uintptr_t)buff & 0x3u) == 0u) {
            e = sd_read_blocks(&sd_card, sector, (uint32_t*)buff, 1);
        } else {
            e = sd_read_blocks(&sd_card, sector, sd_bounce, 1);
            if (e == SD_OK) memcpy(buff, sd_bounce, 512);
        }

        if (e != SD_OK) {
            g_last_sd_err = e;
            return RES_ERROR;
        }

        buff += 512;
        sector++;
    }

    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,
    const BYTE *buff,
    DWORD sector,
    UINT count
)
{
    if (pdrv != DEV_MMC || !count)
        return RES_PARERR;

    if (sd_stat & STA_NOINIT)
        return RES_NOTRDY;

    while (count--)
    {
        memcpy(sd_bounce, buff, SD_BLOCK_SIZE);

        sd_error_t err = sd_write_blocks(&sd_card, sector, sd_bounce, 1);
        if (err != SD_OK)
            return RES_ERROR;

        buff   += SD_BLOCK_SIZE;
        sector += 1;
    }

    return RES_OK;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,
    BYTE cmd,
    void *buff
)
{
    if (pdrv != DEV_MMC)
        return RES_PARERR;

    if (sd_stat & STA_NOINIT)
        return RES_NOTRDY;

    switch (cmd)
    {
    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_SIZE:
        *(WORD*)buff = SD_BLOCK_SIZE;
        return RES_OK;

    case GET_BLOCK_SIZE:

        *(DWORD*)buff = 1;
        return RES_OK;

    case GET_SECTOR_COUNT:

        return RES_PARERR;
    }

    return RES_PARERR;
}


