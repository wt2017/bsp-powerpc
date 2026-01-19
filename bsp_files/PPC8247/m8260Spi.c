/*SPI interface to serial EEPROM library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
2005-07-15 write by bjm
*/

/* includes */

#include "vxWorks.h"
#include "vme.h"
#include "memLib.h"
#include "cacheLib.h"
#include "sysLib.h"
#include "config.h"
#include "string.h"
#include "intLib.h"
#include "logLib.h"
#include "stdio.h"
#include "taskLib.h"
#include "vxLib.h"
#include "drv/parallel/m8260IOPort.h"
#include "bspCommon.h"

#define M8260_SPI_BASE_REG		(vxImmrGet()+ 0x89FC)  /* I2C's DPRAM pointer */

#define SPI_RX_BD_OFF 			(M8260_SPI_PARAM_OFF+ 0x80)  /* spi RX BD descriptor */
#define SPI_RX_BUF_OFF 			(M8260_SPI_PARAM_OFF+ 0x90)  /* spi RX buffer offset */
#define SPI_TX_BD_OFF 			(M8260_SPI_PARAM_OFF+ 0xb0)  /* spi TX BD descriptor */
#define SPI_TX_BUF_OFF 			(M8260_SPI_PARAM_OFF+ 0xc0)  /* spi TX buffer offset */
#define M8260_SPI_BD_BUF_LEN 		32 /*32 byte date buffer of BD */

#define SPI_TX_BUF_BASE       (vxImmrGet() + SPI_TX_BUF_OFF)
#define SPI_RX_BUF_BASE       (vxImmrGet() + SPI_RX_BUF_OFF)
 
#define M8260_SPI_BD_BUF_LEN 	32 /*32 byte date buffer of BD */

#define SPI_TRANS_RX_BD_LAST  0x0800 /* last in frame */
#define SPI_TRANS_RX_BD_WRAP  0x2000 /* wrap back to first BD */
#define SPI_TRANS_RX_BD_EMPTY 0x8000 /* buffer is empty */
#define SPI_TRANS_TX_BD_LAST  0x0800 /* last in frame */
#define SPI_TRANS_TX_BD_WRAP  0x2000 /* wrap back to first BD */
#define SPI_TRANS_TX_BD_READY 0x8000 /* ready for Tx */

#define MPC8260_SPI_ERR_CYCLE_TIMEOUT	9	
#define MPC8260_SPI_PARAM_ERR			10	
#define MPC8260_SPI_NO_INIT			11	

#define SPI_INTERFACE_PINS 0x0000E000
#define EnSPI		  0x0100
#define DisSPI		  0x1671
#define SPI_STR		  0x80

#define SPI_5321_READ   0x60
#define SPI_5321_WRITE  0x61

#define BCM5321_MAX_BYTE_CNT    8 /*8 BYTE BUFFER*/

#define SPI_5321_SW_TIMEOUT_CNT 32
#define SPI_5321_SW_RETYR_CNT 	16

#define SPI_STS_SPIF   0x80
#define SPI_STS_RACK   0x20

#define SPI_5321   0x0
#define SPI_CPLD   0x1
#define SPI_8510   0x2
#define SPI_LIU1   0x3
#define SPI_LIU2   0x4

typedef struct /* SPI_BUF */
{
	VUINT16 statusMode; /* status and control */
	VUINT16 dataLength; /* length of data buffer in bytes */
	u_char * dataPointer; /* points to data buffer */
} SPI_BD;

typedef struct /* SPI_PARAM */
{
	VUINT16 rbase;  /* Rx buffer descriptor base address */
	VUINT16 tbase;  /* Tx buffer descriptor base address */
	VUINT8 rfcr;    /* Rx function code */
	VUINT8 tfcr;    /* Tx function code */
	VUINT16 mrblr;  /* maximum receive buffer length */
	VUINT32 rstate; /* Rx internal state */
	VUINT32 res1;   /* reserved/internal */
	VUINT16 rbptr;  /* Rx buffer descriptor pointer */
	VUINT16 res2;   /* reserved/internal */
	VUINT32 res3;   /* reserved/internal */
	VUINT32 tstate; /* Tx internal state */
	VUINT32 res4;   /* reserved/internal */
	VUINT16 tbptr;  /* Tx buffer descriptor pointer */
	VUINT16 res5;   /* reserved/internal */
	VUINT32 res6;   /* reserved/internal */
	VUINT32 res7;   /* reserved/internal */
} SPI_PARAM;
typedef SPI_PARAM *pSPI_PARAM;
typedef SPI_BD * pSPI_BD ;

typedef struct spi_dev
{
	BOOL init;
	pSPI_PARAM pPram;
	pSPI_BD pTxBdBase;
	pSPI_BD pRxBdBase;
	unsigned char *txBuffer ;
	unsigned char *rxBuffer ;	
} SPI_DEV;

LOCAL SPI_DEV spiDev ={FALSE, NULL, NULL,NULL, NULL};
LOCAL SEM_ID spiMutexSem = NULL ;

#define	MPC8260_SPIE(base)	((VINT8  *) ((base) + 0x11AA6))
#define	MPC8260_SPIM(base)	((VINT8  *) ((base) + 0x11AAA))
#define	MPC8260_SPCOM(base)	((VINT8  *) ((base) + 0x11AAD))
#define	MPC8260_SPMODE(base)	((VINT16  *) ((base) + 0x11AA0))

#define MAX_SPI_RETRY  10
#define MPC8260_SPI_WRITE	1
#define MPC8260_SPI_READ	2

int g_spiDebugInfo = 0 ;

/* Forward declarations for SPI device. */
extern void bsp_cycle_delay(unsigned int num) ; 

/******************************************************************************
*
* sysHwSpiInit - initializes spi device
*
* This routine initializes spi device to prepare for subsequent reads from or
* writes to spi device.
*
* RETURNS: none
*/
void bsp_spi_init(void)
{
	SPI_DEV* pSpi = &spiDev;
	int immrVal = vxImmrGet();

	/* Set pointer to SPI parameter RAM. */
	*((unsigned short int *)M8260_SPI_BASE_REG) = M8260_SPI_PARAM_OFF;
	pSpi->pPram = (pSPI_PARAM)(immrVal + M8260_SPI_PARAM_OFF);

	/* Clear parameter RAM values. */
	pSpi->pPram->rbase = (UINT16) SPI_RX_BD_OFF;
	pSpi->pPram->tbase = (UINT16) SPI_TX_BD_OFF;
	pSpi->pPram->mrblr = 16; 
	pSpi->pPram->rfcr = 0x18;
	pSpi->pPram->tfcr = 0x18;

	/* Set up SPI dual-ported RAM. */
	pSpi->pRxBdBase = (pSPI_BD) (immrVal + SPI_RX_BD_OFF);
	pSpi->pTxBdBase = (pSPI_BD) (immrVal + SPI_TX_BD_OFF);

	/*
	* Configure port D pins to enable SPIMOSI, SPIMISO and SPICLK pins.
	*/
	*M8260_IOP_PDPAR(immrVal) |= PD16|PD17|PD18;
	*M8260_IOP_PDODR(immrVal) &= ~(PD16|PD17|PD18);
	*M8260_IOP_PDDIR(immrVal) &= ~(PD16|PD17|PD18);
	*M8260_IOP_PDSO(immrVal) |= PD16|PD17|PD18;

	/* Set up the Port D pins. */
	*M8260_IOP_PDPAR(immrVal) &= ~PD19; /* make CS a GP I/O pin */
	*M8260_IOP_PDDIR(immrVal) |= PD19;
	*M8260_IOP_PDDAT(immrVal) |= PD19; /* de-assert for now */

	/* Initialize buffer descriptors. */
	pSpi->pTxBdBase->statusMode = (SPI_TRANS_TX_BD_LAST | SPI_TRANS_TX_BD_WRAP);
	pSpi->pTxBdBase->dataLength = 0;
	pSpi->pTxBdBase->dataPointer = (u_char *)SPI_TX_BUF_BASE;

	pSpi->pRxBdBase->statusMode = (SPI_TRANS_RX_BD_EMPTY | SPI_TRANS_RX_BD_LAST | SPI_TRANS_RX_BD_WRAP);
	pSpi->pRxBdBase->dataLength = 0;
	pSpi->pRxBdBase->dataPointer = (u_char *)SPI_RX_BUF_BASE;

	/* Initialize Rx and Tx parameters for SPI. */
	*M8260_CPCR(immrVal) = 0x25410000;

	/* Wait for initialization to complete. */
	while (*M8260_CPCR(immrVal) & 00010000)
	{;}

	/* Clear events. */
	*MPC8260_SPIE(immrVal) = 0xFF;
	*MPC8260_SPMODE(immrVal) = DisSPI;

	if(spiMutexSem == NULL)
		spiMutexSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);

	pSpi->init = TRUE;
	printf("SPI interface intialize completed\n");

	return;
} /* end of sysHwSpiInit */

/***************************************************************************
*
* mpc8260SpiWaitRx - perform spi receive date wait
*
* This routine is used to perform an I2C receive date in bd
*
* RETURNS: OK, or ERROR if timed out waiting for an receive date .
*/
LOCAL STATUS mpc8260SpiWaitRx(void)
{
	UINT32 	timeOutCount;
	SPI_DEV* pSpi = &spiDev;

	for (timeOutCount = 10; timeOutCount; timeOutCount--)
	{
		if((pSpi->pRxBdBase->statusMode & SPI_TRANS_RX_BD_EMPTY) == 0) // BD data buffer data reived
		{
			break;
		}
		bsp_cycle_delay (1);
	}

	if (timeOutCount == 0)
	{
		return (ERROR);
	}

	return (OK);
}

/***************************************************************************
*
* mpc8260i2cWaittx - perform I2C tx date wait
*
* This routine is used to perform an I2C tx date in bd
*
* RETURNS: OK, or ERROR if timed out waiting for an tx date .
*/
LOCAL STATUS mpc8260SpiWaitTx(void)
{
	UINT32 	timeOutCount;
	SPI_DEV* pSpi = &spiDev;

	for (timeOutCount = 10; timeOutCount; timeOutCount--)
	{
		if ((pSpi->pTxBdBase->statusMode & SPI_TRANS_TX_BD_READY) == 0) //BD buffered data have been send out    
		{
			break;
		}
		bsp_cycle_delay (1);
	}
	if (timeOutCount == 0)
	{
		return (ERROR);
	}

	return (OK);
}

/****************************************************************
 *   
 * bsp_spi_output - This function sets up SPI interface out BD 
 * and control registers ,fit for freescale 8247,8260 ... chips
 * PARAMETERS :
 * buffer :  receive or transmit buffer ,
 * length : length of the buffer(when transmit) , length of date(when receive, date lengthe <=32).
 * option : SPI mode register value, 
 * read_action : == TRUE(1),read action ,== FALSE(0), write action 
(BIT2) CI : Clock invert
 *	0 The inactive state of SPICLK is low.
 *	1 The inactive state of SPICLK is high.
(BIT3) CP Clock phase.
 *	0 SPICLK starts toggling at the middle of the data transfer.
 *	1 SPICLK starts toggling at the beginning of the data transfer.
(BIT5) Reverse data. Determines the receive and transmit character bit order.
 *  0 Reverse data¡ªlsb of the character sent and received first
 *  1 Normal operation¡ªmsb of the character sent and received first
		to see more in chips Reference Manual ........
		
 * RETURNS : OK,if receive or transmit successfully,else return ERROR
 *
 */
STATUS bsp_spi_output(char *buffer, int length , unsigned short option , unsigned char read_action)
{
	int status = OK;	
	SPI_DEV* pSpi = &spiDev;
	int immrVal = vxImmrGet();

	if(pSpi->init == FALSE)
	{
		printf("\nError :SPI interface have not been initialized\n") ;
		return MPC8260_SPI_NO_INIT;		
	}

	if(buffer == NULL || length > M8260_SPI_BD_BUF_LEN)
	{
		printf("\nError : param Error  buffer = 0x%x [not NULL] , length = %d [0:32]\n" ,
			(UINT32)buffer ,length) ;
		return MPC8260_SPI_PARAM_ERR;
	}
	
	semTake (spiMutexSem, WAIT_FOREVER);


	*MPC8260_SPMODE(immrVal) = option ;   

	/* Wait for transmit to finish (first). */
	if ((mpc8260SpiWaitTx ()) != OK)
	{
		printf("\nError : timeout when transmit previous date\n") ;
		status = MPC8260_SPI_ERR_CYCLE_TIMEOUT;
		goto spi_out_exit ;
	} 

	/* Assert SPI chip select (CS). */
	*M8260_IOP_PDDAT(immrVal) &= ~PD19;

	/* Setup TX date buffer*/
	memcpy((void *)SPI_TX_BUF_BASE, buffer, length);

	/* Setup transmit BD buffer */
	pSpi->pTxBdBase->statusMode = ( SPI_TRANS_TX_BD_READY | SPI_TRANS_TX_BD_WRAP | SPI_TRANS_TX_BD_LAST );
	pSpi->pTxBdBase->dataLength = length ;
	pSpi->pTxBdBase->dataPointer = (u_char *)SPI_TX_BUF_BASE;

	/* Set up receive buffer descriptor. */
	pSpi->pRxBdBase->statusMode = ( SPI_TRANS_RX_BD_WRAP | SPI_TRANS_RX_BD_EMPTY );
	pSpi->pRxBdBase->dataLength = 0;
	pSpi->pRxBdBase->dataPointer = (u_char *)SPI_RX_BUF_BASE;

	/* Start transfer. */
	*MPC8260_SPCOM(immrVal) = SPI_STR;

	/* Wait for transmit to finish (first). */
	if ((mpc8260SpiWaitTx ()) != OK)
	{
		printf("\nError : timeout when transmit current date\n") ;
		status = MPC8260_SPI_ERR_CYCLE_TIMEOUT ;
		goto spi_out_exit ;
	}

	if(read_action) //read command , must read date from
	{
		/* Wait for transmit to finish (first). */
		if ((mpc8260SpiWaitRx ()) != OK)
		{
			status = MPC8260_SPI_ERR_CYCLE_TIMEOUT ;
			if(g_spiDebugInfo) printf("\nError : timeout when receive previous date\n") ;
			goto spi_out_exit ;
		}
		else
		{
		    /*read receive data to user buffer*/
			memcpy(buffer , pSpi->pRxBdBase->dataPointer , pSpi->pRxBdBase->dataLength);
	//		length = pSpi->pRxBdBase->dataLength ;
		}
	}


spi_out_exit:
	pSpi->pTxBdBase->statusMode = ( SPI_TRANS_TX_BD_WRAP | SPI_TRANS_TX_BD_LAST );
	pSpi->pTxBdBase->dataLength = 0 ;
	pSpi->pRxBdBase->statusMode = ( SPI_TRANS_RX_BD_WRAP | SPI_TRANS_RX_BD_EMPTY );
	pSpi->pRxBdBase->dataLength = 0;

	/* De-assert 8260 SPI chip select (CS). */
	*M8260_IOP_PDDAT(immrVal) |= PD19;

	/*disable the spi*/
	*MPC8260_SPMODE(immrVal) &= (~EnSPI);  

	semGive(spiMutexSem);

	return status ;
}
void spiDevShow()
{
	char i , buffer[4] ;
	int status , length = 2;
	
	printf("SPI-Dev -Address ,  First-Bye-value\n");
	for(i = 0 ; i < 0x7f ; i++)
	{
		buffer[0] = (i << 1) | 0x1; /*read action*/ 
		buffer[1] = 0 ;
		status = bsp_spi_output(buffer, length , 0x1771, 1) ;

		if(status  ==  0)
			printf("\t0x%x         \t0x%6x \n" , buffer[0] , buffer[1]) ;
			
	}
		
}



