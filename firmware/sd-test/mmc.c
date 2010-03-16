/*-----------------------------------------------------------------------*/
/* MMC/SDC (in SPI mode) control module  (C)ChaN, 2006                   */
/*-----------------------------------------------------------------------*/
/* Only rcvr_spi(), xmit_spi(), disk_timerproc(), disk_initialize () and */
/* some macros are platform dependent.                                   */
/*-----------------------------------------------------------------------*/


#include "diskio.h"

#include "spike_hw.h"

/* MMC/SD command (in SPI) */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(0x40+41)	/* SEND_OP_COND (ACMD) */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


/* Control signals (Platform dependent) */
#define SELECT()	spi0->cs = 0		/* MMC CS = L */
#define	DESELECT()	spi0->cs = 1		/* MMC CS = H */

static volatile
DSTATUS Stat = STA_NOINIT;	/* Disk status */

static volatile
BYTE Timer1, Timer2;		/* 100Hz decrement timer */

//#define xmit_spi(dat) 	spi0->data=(dat); while(spi0->status & 0x01)



/*---------------------------------*/
/* Receive a byte from MMC via SPI */
/* (Platform dent)            */

static
BYTE rcvr_spi (void)
{
    spi0->data=0xff;	

    while(spi0->status & 0x01);
    
        uart_putstr("read spi data=0x");
        uart_puthex8(spi0->data);
        uart_putstr(" status=0x");
        uart_puthex8(spi0->status);
        uart_putstr("\n\r");

    return spi0->data;
}

static
void  xmit_spi (BYTE value)
{
    spi0->data=value;	

    while(spi0->status & 0x01);
    
        uart_putstr("write spi data=0x");
        uart_puthex8(value);
        uart_putstr(" status=0x");
        uart_puthex8(spi0->status);
        uart_putstr("\n\r");
}


/* Alternative macro to receive data fast */
#define rcvr_spi_m(dst)	spi0->data=0xff;	while(spi0->status & 0x01); *(dst)=spi0->data

/*---------------------*/
/* Wait for card ready */

static
BYTE wait_ready (void)
{
	BYTE res;
    uart_putstr("wait_ready: ");
	Timer2 = 50;	/* Wait for ready in timeout of 500ms */
	rcvr_spi();
    do
		res = rcvr_spi();
	while ((res != 0xFF) && Timer2--);

    if (res != 0xff)
        uart_putstr("is not ready\r\n");
    else
        uart_putstr("\r\n");
	return res;
}



/*--------------------------------*/
/* Receive a data packet from MMC */

static
BOOL rcvr_datablock (
	BYTE *buff,			/* Data buffer to store received data */
	BYTE wc				/* Word count (0 means 256 words) */
)
{
	BYTE token;
	Timer1 = 10;
	do {							/* Wait for data packet in timeout of 100ms */
		token = rcvr_spi();
	} while ((token == 0xFF) && Timer1);
	if(token != 0xFE) return FALSE;	/* If not valid data token, retutn with error */

	do {							/* Receive the data block into buffer */
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
	} while (--wc);
	rcvr_spi();						/* Discard CRC */
	rcvr_spi();

	return TRUE;					/* Return with success */
}



/*---------------------------*/
/* Send a data packet to MMC */

#ifndef _READONLY
static
BOOL xmit_datablock (
	const BYTE *buff,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data/Stop token */
)
{
	BYTE resp, wc = 0;
	if (wait_ready() != 0xFF) return FALSE;

	xmit_spi(token);					/* Xmit data token */
	if (token != 0xFD) {	/* Is data token */
		do {							/* Xmit the 512 byte data block to MMC */
			xmit_spi(*buff++);
			xmit_spi(*buff++);
		} while (--wc);
		xmit_spi(0xFF);					/* CRC (Dummy) */
		xmit_spi(0xFF);
		resp = rcvr_spi();				/* Reveive data response */
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return FALSE;
	}

	return TRUE;
}
#endif



/*------------------------------*/
/* Send a command packet to MMC */

static
BYTE send_cmd (
	BYTE cmd,		/* Command byte */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;
	
    if (wait_ready() != 0xFF) return 0xFF;
	
    /* Send command packet */
	xmit_spi(cmd);						/* Command */
	xmit_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((BYTE)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((BYTE)arg);				/* Argument[7..0] */
	xmit_spi(0x95);						/* CRC (valid for only CMD0) */

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi();		/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do {
		res = rcvr_spi();
    } while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}




DSTATUS disk_initialize (void)
{
	BYTE n, f;

	uart_putstr("deselect\r\n");
	DESELECT();
    Stat = 0;//XXX: only for testing
	
	f = 0;
	if (!(Stat & STA_NODISK)) {
		n = 10;						            /* Dummy clock */
	    uart_putstr("clock\r\n");
        do
			rcvr_spi();
		while (--n);
	    uart_putstr("select\r\n");
		SELECT();			                    /* CS = L */
        uart_putstr("CMD0\r\n");
		if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
			Timer1 = 50;						/* Initialization timeout of 500 msec */
	        uart_putstr("CMD1\r\n");
            while (Timer1-- && send_cmd(CMD1, 0))	  /* Initialize with CMD1 */
			    uart_putstr("CMD1\r\n");
            if (Timer1) {
				f = 1;							/* When device goes ready, break */
	            uart_putstr("driver ready\r\n");
			} else {
				Timer1 = 100;
				while (Timer1) {				/* Retry initialization with ACMD41 */
	                uart_putstr("CMD55\r\n");
					if (send_cmd(CMD55, 0) & 0xFE) continue;
					if (send_cmd(CMD41, 0) == 0) {
						f = 1; break;			/* When device goes ready, break */
					}
				}
			}
		}

	    uart_putstr("CMD16\r\n");
		if (f && (send_cmd(CMD16, 512) == 0))	/* Select R/W block length */
			f = 2;
	    uart_putstr("deselect\r\n");
		DESELECT();			/* CS = H */
		rcvr_spi();			/* Idle (Release DO) */
	}

	if (f == 2){
		Stat &= ~STA_NOINIT;	/* When initialization succeeded, clear STA_NOINIT */
	    uart_putstr("init ok\r\n");
    } else {
		disk_shutdown();		/* Otherwise force uninitialized */
	    uart_putstr("disk shutdown\r\n");
    }
	return Stat;
}



/*-----------------------*/
/* Shutdown              */
/* (Platform dependent)  */


DSTATUS disk_shutdown (void)
{
	Stat |= STA_NOINIT;

	return Stat;
}



/*--------------------*/
/* Return Disk Status */

DSTATUS disk_status (void)
{
	return Stat;
}



/*----------------*/
/* Read Sector(s) */

DRESULT disk_read (
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (!count) return RES_PARERR;

	sector *= 512;		/* LBA --> byte address */

	SELECT();			/* CS = L */

	if (count == 1) {	/* Single block read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, (BYTE)(512/2)))
			count = 0;
	}
	else {				/* Multiple block read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, (BYTE)(512/2))) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}

	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	return count ? RES_ERROR : RES_OK;
}



/*-----------------*/
/* Write Sector(s) */

#ifndef _READONLY
DRESULT disk_write (
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..255) */
)
{
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;
	if (!count) return RES_PARERR;
	sector *= 512;		/* LBA --> byte address */

	SELECT();			/* CS = L */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}

	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	return count ? RES_ERROR : RES_OK;
}
#endif



/*--------------------------*/
/* Miscellaneous Functions  */

DRESULT disk_ioctl (
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive data block */
)
{
	DRESULT res;
	BYTE n, csd[16], *ptr = buff;
	WORD csm, csize;


	if (Stat & STA_NOINIT) return RES_NOTRDY;

	SELECT();		/* CS = L */

	res = RES_ERROR;
	switch (ctrl) {
		case GET_SECTORS :	/* Get number of sectors on the disk (unsigned long) */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16/2)) {
				/* Calculate disk size */
				csm = 1 << (((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2);
				csize = ((WORD)(csd[8] & 3) >> 6) + (WORD)(csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(DWORD*)ptr = (DWORD)csize * csm;
				res = RES_OK;
			}
			break;

		case MMC_GET_CSD :	/* Receive CSD as a data block (16 bytes) */
			if ((send_cmd(CMD9, 0) == 0)	/* READ_CSD */
				&& rcvr_datablock(ptr, 16/2))
				res = RES_OK;
			break;

		case MMC_GET_CID :	/* Receive CID as a data block (16 bytes) */
			if ((send_cmd(CMD10, 0) == 0)	/* READ_CID */
				&& rcvr_datablock(ptr, 16/2))
				res = RES_OK;
			break;

		case MMC_GET_OCR :	/* Receive OCR as an R3 resp (4 bytes) */
			if (send_cmd(CMD58, 0) == 0) {	/* READ_OCR */
				for (n = 0; n < 4; n++)
					*ptr++ = rcvr_spi();
				res = RES_OK;
			}
			break;

		default:
			res = RES_PARERR;
	}

	DESELECT();			/* CS = H */
	rcvr_spi();			/* Idle (Release DO) */

	return res;
}



/*---------------------------------------*/
/* Device timer interrupt procedure      */
/* This must be called in period of 10ms */
/* (Platform dependent)                  */
#if 0
void disk_timerproc (void)
{
	static BYTE pv;
	BYTE n, s;


	n = Timer1;						/* 100Hz decrement timer */
	if (n) Timer1 = --n;
	n = Timer2;
	if (n) Timer2 = --n;

	n = pv;
	pv = SOCKPORT & (SOCKWP | SOCKINS);	/* Sample socket switch */

	if (n == pv) {					/* Have contacts stabled? */
		s = Stat;

		if (pv & SOCKWP)			/* WP is H (write protected) */
			s |= STA_PROTECT;
		else						/* WP is L (write enabled) */
			s &= ~STA_PROTECT;

		if (pv & SOCKINS)			/* INS = H (Socket empty) */
			s |= (STA_NODISK | STA_NOINIT);
		else						/* INS = L (Card inserted) */
			s &= ~STA_NODISK;

		Stat = s;
	}
}
#endif
