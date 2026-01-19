/* sysMotI2c.c - I2C Driver Source Module */

/* Copyright (c) 2000-2005 Wind River Systems, Inc. */
/* Copyright 1996-2000 Motorola, Inc. All Rights Reserved */

/*
modification history
--------------------
01e,27jan06,dtr  Tidy up.
01d,10nov05,mdo  Documentation fixes for apigen
01c,21may02,gtf  modified for Raytheon NetFires bsp.
01b,10mar00,rhk  changed over to use sysCalcBusSpd, removed 100MHz speed.
01a,28feb00,rhk  created from version 01d, MV2100 BSP.
*/

/*
DESCRIPTION

This file contains generic functions to read/write an I2C device.
Currently the file only supports the MPC8548 I2C interface.
However, additional I2C bus controllers can be easily be added
as required. This dirver doesn't support mutiple threads/tasks and 
therfore the user should control access to driver via mutex if that 
is reqd.

INCLUDE FILES: sysMotI2c.h
*/


/* includes */

#include <vxWorks.h>		/* vxWorks generics */
#include "config.h"
#include <ioLib.h>			/* input/output generics */
#include <blkIo.h>			/* block input/output specifics */
#include <semLib.h>			/* semaphore operations */
#include <cacheLib.h>			/* cache control */
#include <intLib.h>			/* interrupt control */
#include <semLib.h>
#include "sysMotI2c.h"		/* driver specifics */
#include "sysMpc85xxI2c.h"	/* Mpc85xx I2C Driver Header Module */
#include <stdio.h>
#include <logLib.h>
#include <stdlib.h>
#include "sysEpic.h"

#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif /* defined PRJ_BUILD */

extern int g_BspI2cDebug;
extern void i2cForceMpc85xxStop (int unit);

#ifdef  INCLUDE_RTP
STATUS i2cFileDevCreate 
    (
    char * name,		/* device name		    */
    int    length		/* MAX buffer number of bytes supprot by i2c dev  */
    ) ;
#endif /*INCLUDE_RTP*/

/* defines */
#undef I2C_DRIVER_DEBUG

/* externals */

#ifdef I2C_DRIVER_TESTS
IMPORT int printf();		/* formatted print */
#endif

IMPORT void sysMpc85xxMsDelay (UINT mSeconds);

/* locals */

LOCAL UINT8 i2cInByte (UINT32);
LOCAL void i2cOutByte (UINT32,UINT8);
LOCAL int i2cDoOp  (  
	int    unit,
	UINT32 deviceAddress,		/* device I2C bus address */
	i2cCmdPckt_t *pI2cCmdPacket		/* pointer to command packet */
    	) ;


/* Driver/controller routines table for the Mpc85xx device. */
i2cDrvRoutines_t i2cDrvRoutinesTableMpc85xx = 
{
	(int(*)())i2cCycleMpc85xxStart,
	(int(*)())i2cCycleMpc85xxStop,
	(int(*)())i2cCycleMpc85xxRead,
	(int(*)())i2cCycleMpc85xxWrite,
	(int(*)())i2cCycleMpc85xxAckIn,
	(int(*)())i2cCycleMpc85xxAckOut,
	(int(*)())i2cCycleMpc85xxKnownState,
	(void(*)())sysMpc85xxMsDelay
};

/* driver/controller routines table, indexed by the "I2C_DRV_TYPE". */

i2cDrvRoutines_t *i2cDrvRoutinesTables[] = 
{
	(i2cDrvRoutines_t *)&i2cDrvRoutinesTableMpc85xx	/* index 0 */
};

I2C_DRV_CTRL i2C1DrvCtrl ;
I2C_DRV_CTRL i2C2DrvCtrl ;
I2C_DRV_CTRL * pI2cDrvCtrl[2] = { NULL, NULL } ;
SEM_ID i2cAckSynBSem = NULL ;
SEM_ID i2cReadSynBSem = NULL ;

LOCAL SEM_ID i2cMutexSem = NULL ;

int g_BspI2cReadMutex = FALSE;
/*iic调试打印信息开关*/
int g_BspI2cDebug = FALSE;
/**************************************************************************
* 
* i2cIoctl - perform I2C read/write operations
*
* The purpose of this function is to perform i2cIn/OutByte operations
* with synchronization.
*
* RETURNS: data written or data read
*
* ERRNO: N/A
*/
UINT8 i2cIoctl
    (
    UINT32 ioctlflg,  /* input/output control flag */
		      /* 0, write */
		      /* 1, read */
		      /* 2, read/modify/write (ORing) */
		      /* 3, read/modify/write (ANDing) */
		      /* 4, read/modify/write (AND/ORing) */
    UINT32  address,	  /* address of device register to be operated upon */
    UINT8   bdata1,	  /* data item 1 for read/write operation */
    UINT8   bdata2	  /* data item 2 for read/write operation */
    )
{
    UINT8 u8temp;

#ifdef I2C_DRIVER_DEBUG
    logMsg("i2cIoctl: adrs - 0x%x.\n", address,2,3,4,5,6);
#endif

    if ( ioctlflg == I2C_IOCTL_WR )	/* write */
	{
        i2cOutByte(address, bdata1);
   	}
    else if ( ioctlflg == I2C_IOCTL_RD ) /* read */
	{
        bdata1 = i2cInByte(address);
	}
    else if ( ioctlflg == I2C_IOCTL_RMW_OR ) /* ORing */
	{
        u8temp = i2cInByte(address);
        u8temp |= bdata1;
        i2cOutByte(address, u8temp);
	}
    else if ( ioctlflg == I2C_IOCTL_RMW_AND ) /* ANDing */
	{
        u8temp = i2cInByte(address);
        u8temp &= bdata1;
        i2cOutByte(address, u8temp);
	}
    else if ( ioctlflg == I2C_IOCTL_RMW_AND_OR ) /* AND/ORing */
	{
        u8temp = i2cInByte(address);
        u8temp &= bdata1;
        u8temp |= bdata2;
        i2cOutByte(address, u8temp);
	}
    return(bdata1);
}

/***************************************************************************
*
* i2cDrvInit - initialize I2C device controller
*
* This function initializes the I2C device controller device for operation.
* This function should only be executed once during system initialization 
* time.
*
* NOTE: no printf or logMsg statements should be called during this routine
* since it is called in sysHwInit(). If output is desired a polled or debug 
* dump routine should be used.
*
* RETURNS: OK
*
* ERRNO: N/A
*/

STATUS i2cDrvInit
    (
    int unit,
    int i2cControllerType		/* I2C controller type */
    )
    {
    /*
     * Check for unknown controller type, and initialize I2C controller
     * for operation (if needed).  Note: a switch statement does not work here if
     * executing from ROM due to branch history table creation.
     */

    if (pI2cDrvCtrl[unit] == NULL)
    {
        if (unit == 0)
        {
            pI2cDrvCtrl[unit] = &i2C1DrvCtrl ;
            pI2cDrvCtrl[unit]->baseAdrs = M85XX_I2C1_BASE ;
        }
        else if (unit == 1)
        {
            pI2cDrvCtrl[unit] = &i2C2DrvCtrl ;
            pI2cDrvCtrl[unit]->baseAdrs = M85XX_I2C2_BASE ;
        }

        pI2cDrvCtrl[unit]->baseAdrs += CCSBAR ;
    }

    /* disable the I2C module, set the device to Master Mode */
    i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
	     (UINT32)(pI2cDrvCtrl[unit]->baseAdrs + MPC85XX_I2C_CONTROL_REG),
	     ((UINT8)~MPC85XX_I2C_CONTROL_REG_MEN), 
	     MPC85XX_I2C_CONTROL_REG_MSTA);

    taskDelay(1);
    /* initialize and enable the I2C interface */
    UINT8 divider ;
//  divider = 0x2c;   /* 266000/1280 约 200k */    
//  divider = 0x2a;   /* 266000/896  约 300k */    
    divider = 0x31;   /* 266000/3072 约 86k  */        
    i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
	     (UINT32)(pI2cDrvCtrl[unit]->baseAdrs + MPC85XX_I2C_FREQ_DIV_REG),
	     ((UINT8)~MPC85XX_I2C_FREQ_DIV_REG_MASK), 
	     divider); 
    
    /*
     * set the device to slave mode.  This is required for
     * clearing a BUS BUSY lockup condition.
     */
    i2cIoctl(I2C_IOCTL_RMW_AND, 
	     (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG),
	     ((UINT8)~MPC85XX_I2C_CONTROL_REG_MSTA), 0);
   
    /* enable the interface */
    /*按器件手册的说明，这个似乎应该放在前面，实际测试还是放在这里比较好*/
    i2cIoctl(I2C_IOCTL_RMW_OR, 
	     (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
	     (MPC85XX_I2C_CONTROL_REG_MEN | MPC85XX_I2C_CONTROL_REG_MIEN), 0);

    return(OK);
    }


/******************************************************************************
*
* _i2cRead_action - read blocks from I2C
*
* This function's purpose is to read the specified number of
* blocks from the specified device.
*
* RETURNS: OK, or ERROR on a bad request
*
* ERRNO: N/A
*/

static int _i2cRead_action
    (
    int          unit,
    UINT32       deviceAddress,	/* Device's I2C bus address */
    unsigned int numBlks,	    /* number of blocks to read or single/double byte register */
    char *       pBuf			/* pointer to buffer to receive data */
    )
    {
    int localStatus;		/* local status variable */
    i2cCmdPckt_t i2cCmdPacket;	/* command packet */

    /* Check to see if the driver's been installed */

    if(pI2cDrvCtrl[unit]->baseAdrs == 0)
    {
        if(TRUE == g_BspI2cDebug)
        {
            printf("%s(%d) I2C driver for unit %d not initialized.\r\n",
                    __FUNCTION__, __LINE__,unit);
        }
        return ERROR ;
    }
    /* Check for a NOP request. */
    if (( !numBlks ) || (NULL == pBuf))
	{
        if(TRUE == g_BspI2cDebug)
        {
            printf("%s(%d)pBuf = NULL! or numBlks = 0x%x < 2\r\n",
                __FUNCTION__, __LINE__, numBlks);
        }
    	return(ERROR);
	}

    /* Build command packet. */
    i2cCmdPacket.command = I2C_READOP;
    i2cCmdPacket.status = 0;
    i2cCmdPacket.memoryAddress = (unsigned int)pBuf;
    i2cCmdPacket.nBlocks = numBlks;
    i2cCmdPacket.eCount = numBlks;

//    taskDelay(2);/*每次操作时需要延迟*/

	semTake (i2cMutexSem, WAIT_FOREVER);
    localStatus = i2cDoOp(unit, deviceAddress, (i2cCmdPckt_t *)&i2cCmdPacket);
	semGive (i2cMutexSem);

#ifdef I2C_DRIVER_DEBUG
    logMsg("command          =%08X\r\n", i2cCmdPacket.command,2,3,4,5,6);
    logMsg("status           =%08X\r\n", i2cCmdPacket.status,2,3,4,5,6);
    logMsg("memory address   =%08X\r\n", i2cCmdPacket.memoryAddress,2,3,4,5,6);
    logMsg("number of blocks =%08X\r\n", i2cCmdPacket.nBlocks,2,3,4,5,6);
    logMsg("expected count   =%08X\r\n", i2cCmdPacket.eCount,2,3,4,5,6);
#endif

    /* Return the appropriate status. */
    if ( i2cCmdPacket.status != 0 )
	{
	    if(g_BspI2cDebug)
	    {
            printf("%s(line %d) i2cCmdPacket.status - 0x%x, deviceAddress = 0x%x\r\n",
               __FUNCTION__,__LINE__, i2cCmdPacket.status, deviceAddress);
        }
       	localStatus = ERROR ;
	}
    else
    	localStatus	= OK ;

    return(localStatus);
    }


/***************************************************************************
*
* _i2cWrite_action - read blocks from I2C
*
* This function purpose is to write the specified number of
* blocks to the specified device.
*
* RETURNS: OK, or ERROR
*
* ERRNO: N/A
*/

static int _i2cWrite_action
    (
    int          unit,
    UINT32       deviceAddress,	/* Device's I2C bus address */
    unsigned int numBlks,	/* number of blocks to write */
    char *       pBuf			/* pointer to buffer of send data */
    )
    {
    int localStatus;		/* local status variable */

    i2cCmdPckt_t i2cCmdPacket;	/* command packet */

    /* Check to see if the driver's been installed */
    if(pI2cDrvCtrl[unit]->baseAdrs == 0)
    {
        if(TRUE == g_BspI2cDebug)
        {
            printf("%s(%d) I2C driver for unit %d not initialized.\r\n",
                    __FUNCTION__, __LINE__,unit);
        }
        return ERROR ;
    }

    /* Check for a NOP request. */
    if (( !numBlks ) || (NULL == pBuf))
	{
        if(TRUE == g_BspI2cDebug)
        {
            printf("%s(%d)pBuf = NULL! or numBlks = 0x%x < 2\r\n",
                __FUNCTION__, __LINE__, numBlks);
        }
    	return(ERROR);
	}

    /* Build command packet. */

    i2cCmdPacket.command = I2C_WRITOP;
    i2cCmdPacket.status = 0;
    i2cCmdPacket.memoryAddress = (unsigned int)pBuf;
    i2cCmdPacket.nBlocks = numBlks;
    i2cCmdPacket.eCount = numBlks;

//    taskDelay(2);/*每次操作时需要延迟*/

	semTake (i2cMutexSem, WAIT_FOREVER);
    localStatus = i2cDoOp(unit, deviceAddress, (i2cCmdPckt_t *)&i2cCmdPacket);
	semGive (i2cMutexSem);

#ifdef I2C_DRIVER_DEBUG
    logMsg("command          =%08X\r\n", i2cCmdPacket.command,2,3,4,5,6);
    logMsg("status           =%08X\r\n", i2cCmdPacket.status,2,3,4,5,6);
    logMsg("memory address   =%08X\r\n", i2cCmdPacket.memoryAddress,2,3,4,5,6);
    logMsg("number of blocks =%08X\r\n", i2cCmdPacket.nBlocks,2,3,4,5,6);
    logMsg("expected count   =%08X\r\n", i2cCmdPacket.eCount,2,3,4,5,6);
#endif

    /* Return the appropriate status. */
    if ( i2cCmdPacket.status != 0 )
	{
	    if(g_BspI2cDebug)
	    {
            printf("%s(line %d) i2cCmdPacket.status - 0x%x, deviceAddress = 0x%x\r\n",
               __FUNCTION__,__LINE__, i2cCmdPacket.status,deviceAddress);
        }
    	localStatus = ERROR ;
	}
    else
	localStatus	= OK ;

    return localStatus ;
    }

/******************************************************************************
*
* i2cDoOp - execute commands 
*
* This function executes the command operations specified by the command 
* packet.
*
* RETURNS: OK, ERROR if bad request
*
* ERRNO: N/A
*/

static int i2cDoOp
    (
    int    unit,
    UINT32 deviceAddress,		/* device I2C bus address */
    i2cCmdPckt_t *pI2cCmdPacket		/* pointer to command packet */
    )
    {
    i2cDrvRoutines_t *pRoutine;	/* low level routines table pointer */
    int byteCount;		/* byte counter */
    int statusVariable;		/* local status variable */
    unsigned char *pWriteData;	/* pointer to write data buffer */
 //   unsigned int ultmp = 0;
    /* Initialize pointer to driver routines table. */
    pRoutine = i2cDrvRoutinesTables[I2C_DRV_TYPE];

    /* single byte read/write */
    if ( pI2cCmdPacket->command == I2C_READOP )
	{
	statusVariable = 0;
	if ( I2C_KNOWN_STATE(unit) )
	    {
	    statusVariable = I2C_ERROR_KNOWN_STATE; 
	    byteCount = pI2cCmdPacket->nBlocks;
	    goto errorEnd ;
	    }
	if ( I2C_CYCLE_START(unit) )
	    {
	    statusVariable = I2C_ERROR_CYCLE_START; 
	    goto errorEnd ;
	    }
	/* device address */
	if ( I2C_CYCLE_WRITE(unit,i2cAddressMunge(deviceAddress)|0x1) )
	    {
	    statusVariable = I2C_ERROR_CYCLE_WRITE; 
	    goto errorEnd ;
	    }

	if ( I2C_CYCLE_ACKIN(unit) )
	    {
	    statusVariable = I2C_ERROR_CYCLE_ACKIN; 
	    goto errorEnd ;
	    }

    /* Send master ack. */
    i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
        (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG),
        ((UINT8)~(MPC85XX_I2C_CONTROL_REG_MTX|MPC85XX_I2C_CONTROL_REG_TXAK)),
          0);

    g_BspI2cReadMutex = TRUE;
    /*random reads*/        
    i2cIoctl(I2C_IOCTL_RD, (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_DATA_REG), 0, 0);
 
    for (byteCount = 0; byteCount < pI2cCmdPacket->nBlocks; byteCount++)
    {
        /*last READ byte , disable ack singal on SDA*/
        if(byteCount == (pI2cCmdPacket->nBlocks-1))  
        {
            i2cIoctl(I2C_IOCTL_RMW_OR, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
            MPC85XX_I2C_CONTROL_REG_TXAK, 0);

            /*最后一个字节需要先输出stop信号，再读取数据，
            否则从设备会继续向总线输出数据，影响STOP的信号质量*/
            if( semTake(i2cReadSynBSem , 2) == ERROR)
            {
                if(TRUE == g_BspI2cDebug)
                    printf("last byte semTake i2cReadSynBSem err\r\n") ;
            }
            I2C_CYCLE_STOP(unit);     
            /* now do the actual read, make this one count */
            *((unsigned char *)pI2cCmdPacket->memoryAddress+byteCount) = i2cIoctl(I2C_IOCTL_RD, 
                               (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_DATA_REG), 
                               0, 0);
        }    
        else if(0 != I2C_CYCLE_READ(unit,(unsigned char *)pI2cCmdPacket->memoryAddress+byteCount,1))
        {
            statusVariable = I2C_ERROR_CYCLE_READ; 
            break;
        }
    }

    g_BspI2cReadMutex = FALSE;    
    
	}
    else if ( pI2cCmdPacket->command == I2C_WRITOP )
	{
	/* Initialize pointer to caller's write data buffer. */
	pWriteData = (unsigned char *)pI2cCmdPacket->memoryAddress;

	statusVariable = 0;
	if ( I2C_KNOWN_STATE(unit) )
	    {
	    statusVariable = I2C_ERROR_KNOWN_STATE; 
	    byteCount = pI2cCmdPacket->nBlocks;
	    goto errorEnd ;
	    }
	if ( I2C_CYCLE_START(unit) )
	    {
	    statusVariable = I2C_ERROR_CYCLE_START; 
	    goto errorEnd ;
	    }
	/* device address */
	if ( I2C_CYCLE_WRITE(unit,i2cAddressMunge(deviceAddress)) )
	    {
	    statusVariable = I2C_ERROR_CYCLE_WRITE; 
        goto errorEnd ;
	    }
	if ( I2C_CYCLE_ACKIN(unit) )
	    {
	    statusVariable = I2C_ERROR_CYCLE_ACKIN; 
	    goto errorEnd ;
	    }

    for (byteCount = 0; byteCount < pI2cCmdPacket->nBlocks; byteCount++)
        {
        /* Write the data */
    	I2C_CYCLE_WRITE(unit,*(pWriteData+byteCount)) ;
        if ( I2C_CYCLE_ACKIN(unit) )
            {
            statusVariable = I2C_ERROR_CYCLE_ACKIN; 
            goto errorEnd ;
            }
        }
	/*
	 * update the caller's command packet with status of
	 * the operation
	 */
	pI2cCmdPacket->status = statusVariable;
	}
    else
	{
	I2C_KNOWN_STATE(unit);
	return ERROR ;
	}

    errorEnd:
    if ( I2C_CYCLE_STOP(unit))
    {
        statusVariable = I2C_ERROR_CYCLE_STOP; 
    }
    
    /* Leave the I2C bus in a known state. */
    I2C_KNOWN_STATE(unit);

    /*
     * update the caller's command packet with status of
     * the operation
     */
    pI2cCmdPacket->status = statusVariable;
    return OK ;
    }
/******************************************************************************
*
* i2cAddressMunge - initialize the i2c device driver
*
* This function's purpose is to munge/modify the I2C device address
* based upon the byte offset into the device.  This only applies to
* EEPROM type devices.  Dependent upon the size of the device, A0-A2
* address lines are utilized as 256 byte size page offsets.
*
* RETURNS: munged address
*
* ERRNO: N/A
*/

unsigned int i2cAddressMunge
    (
    unsigned int deviceAddress
    )
    {
    return((deviceAddress)&0xfe) ;
    }

/*****************************************************************************
*
* i2cInByte - reads a byte from I2C Regs
*
* This function reads a byte from an I2C register.
*
* RETURNS: value of byte read
*
* ERRNO: N/A
*/

LOCAL UINT8 i2cInByte 
    (
    UINT32 dataPtr
    )
    {
    UINT8 *ptr = (UINT8*)dataPtr ;
    return(*ptr);
    }
/*****************************************************************************
*
* i2cOutByte - writes a byte to an I2C register
*
* This function writes a byte to an I2C register.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void i2cOutByte
    (
    UINT32 dataPtr, 
    UINT8 data
    )
    {
    UINT8 * ptr = (UINT8*)dataPtr ;
    *ptr = data ;
    }

INT32 bsp_i2c_read
    (
    UINT32 	deviceAddress,	/* Device I2C  bus address */
    UINT32 	numBytes,	/* number of Bytes to read */
    char *	pBuf ,		/* pointer to buffer to receive data */
    UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
    UINT32   stop          /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    )
{
    return _i2cRead_action(0, deviceAddress, numBytes, pBuf);
}
INT32 bsp_i2c_write
    (
    UINT32 	deviceAddress,	/* Devices I2C bus address */
    UINT32 	numBytes,	/* number of bytes to write */
    char *	pBuf ,		/* pointer to buffer of send data */
	UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
	UINT32   stop            /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    )
{
    return _i2cWrite_action(0, deviceAddress, numBytes, pBuf);
}

INT32 bsp_i2c_output
    (
    char 		*pBuf ,		/* pointer to buffer to receive data */
    UINT32 	numBytes	/* number of Bytes to read */
    )
{
    if((NULL == pBuf) || (numBytes < 2))
    {
        printf("%s(%d)pBuf = NULL! or numBytes = 0x%x < 2\r\n",
                __FUNCTION__, __LINE__, numBytes);
        return ERROR;
    }

    if(pBuf[0] & 0x01)
    {
        return _i2cRead_action(0, pBuf[0], numBytes-1, (&(pBuf[1])));
    }
    else
    {
        return _i2cWrite_action(0, pBuf[0], numBytes-1, (&(pBuf[1])));
    }
}

UINT32 bsp_i2c_scl_freq(UINT32   num_of_1k)
{
    UINT8 divider ;

    if(38 == num_of_1k) /*只支持这种速率*/
    {
        divider = 0x3A;   /* 266000/14336 约 18k  */        
    }
	else
	{
	    divider = 0x31;   /* 266000/3072 约 86k  */        
	}

    i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
        (UINT32)(pI2cDrvCtrl[0]->baseAdrs + MPC85XX_I2C_FREQ_DIV_REG),
        ((UINT8)~MPC85XX_I2C_FREQ_DIV_REG_MASK), divider); 

	return 0;
}

/*********************************************************
PPC8548的IIC中断处理函数
注意:这里读写，总裁丢失都用同一个中断处理函数。
其中读写操作完成时MPC85XX_I2C_STATUS_REG_MCF位都会置1，
写操作完成时MPC85XX_I2C_STATUS_REG_RXAK位会置0；
为了识别读操作的中断，增加了一个全局变量g_BspI2cReadMutex
*********************************************************/
static STATUS BspI2cIntHandle()
{
       unsigned char statusReg = 0;
       unsigned int unit = 0;

    statusReg = i2cIoctl(I2C_IOCTL_RD, 
                        (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_STATUS_REG), 0, 0);
    if(TRUE == g_BspI2cDebug)
    {    
        logMsg("statusReg = 0x%x\n", statusReg,0,0,0,0,0);
    }        

    i2cIoctl(I2C_IOCTL_RMW_AND, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_STATUS_REG), 
            ((UINT8)~MPC85XX_I2C_STATUS_REG_MIF), 0);
    

    if(0x10 ==( statusReg & MPC85XX_I2C_STATUS_REG_MAL))
    {
        if(TRUE == g_BspI2cDebug)
        {        
            logMsg("i2c Arbitration is lost\n", 0,0,0,0,0,0);
        }            
    }
    
    else if(0x80 ==( statusReg & MPC85XX_I2C_STATUS_REG_MCF))
    {
        if(TRUE == g_BspI2cReadMutex)/*读操作*/
        {
            if(TRUE == g_BspI2cDebug)
            {
                logMsg("semGive(i2cReadSynBSem)\n", 0,0,0,0,0,0);
            }   
            semGive(i2cReadSynBSem) ; 
            return OK ;
        }
        else
        {
            if((0 ==( statusReg & MPC85XX_I2C_STATUS_REG_RXAK)))/*写操作*/
            {
                if(TRUE == g_BspI2cDebug)
                {            
                    logMsg("semGive(i2cAckSynBSem)\n", 0,0,0,0,0,0);
                }                
                
                semGive(i2cAckSynBSem) ; 
                return OK ;
            }
        }
    }
    return ERROR ;
}

void BspI2cInit()
{
	i2cDrvInit(0,0);

#ifdef  INCLUDE_RTP
	i2cFileDevCreate("/i2c" , 256) ;
#endif /* INCLUDE_RTP*/

	if(i2cAckSynBSem == NULL)
   	{
   		i2cAckSynBSem = semBCreate (SEM_Q_FIFO , SEM_EMPTY);
	}	
	
	if(i2cReadSynBSem == NULL)
   	{
   		i2cReadSynBSem = semBCreate (SEM_Q_FIFO , SEM_EMPTY);
	}	 

	if(i2cMutexSem== NULL)
   	{
   		i2cMutexSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
	}	 

	intConnect((VOIDFUNCPTR *)EPIC_I2C_INT_VEC, (VOIDFUNCPTR) BspI2cIntHandle, 0);
	intEnable(EPIC_I2C_INT_VEC) ;
}

int BspI2cTest()
{

    unsigned long ucValWrite = 0x123456;    
    unsigned long ucValRead = 0;    
    
    *CPLD_REG(0x31) = 0x10;
    
    _i2cWrite_action(0,0xa4,4,(char *)&ucValWrite);
    ucValWrite = 0;
    _i2cWrite_action(0,0xa4,1,(char *)&ucValWrite);
    _i2cRead_action(0,0xa4,3,(char *)&ucValRead);
    if(ucValRead != 0x12345600)
    {
        printf("\r\nucValRead = 0x%x", ucValRead);
        return -1;
    }
    return 0;
}

#ifdef  INCLUDE_RTP

typedef struct		  /* I2C_FILE_DEV - memory device descriptor */
    {
    DEV_HDR devHdr;
    int mode;		  /* O_RDONLY, O_WRONLY, O_RDWR */
    int maxBufLen ;
    } I2C_FILE_DEV;

/*******************************************************************************
*
* memDrv - install a i2c driver
*/

LOCAL int i2cFileDrvNum = 0;	  /* driver number of i2c driver */

LOCAL I2C_FILE_DEV *pI2cfileDrv = NULL;



/*******************************************************************************
*
* memOpen - open a  i2cFile file
*
* RETURNS: The file descriptor number, or ERROR if the name is not a
* valid number.
*
* ERRNO: EINVAL
*/

LOCAL I2C_FILE_DEV *i2cFileOpen ( )

{
	if (pI2cfileDrv != NULL)
		return pI2cfileDrv;
	else
	{
		errnoSet (EINVAL);
		return (I2C_FILE_DEV *) ERROR;
	}
}

/*******************************************************************************
*
* i2cFileRead - read from a i2cFile file
*
* Perform a read operation on an open i2cFile.
*
* RETURNS: ERROR if the file open mode is O_WRONLY; otherwise, the number of
* bytes read (may be less than the amount requested), or 0 if the
* file is at EOF.
*
* ERRNO
* \is
* \i EINVAL
* \i EISDIR
* \ie
*/

LOCAL int i2cFileRead
    (
    I2C_FILE_DEV *pfd,	/* file descriptor of file to read */
    char *buffer,	/* buffer to receive data */
    int maxbytes	/* max bytes to read in to buffer */
    )
    {
   int status = ERROR ;
   
    /* Fail if the mode is invalid, or if it's a directory. */
    if (pfd->mode == O_WRONLY)
	{
	errnoSet (EINVAL);
	return (ERROR);
	}
	
    if(pfd != pI2cfileDrv)
	{
	errnoSet (EINVAL);
	return (ERROR);
	}
	
    if (maxbytes > 0)
        {
        status = bsp_i2c_read (buffer[0] | 0x1 , maxbytes -1 , &(buffer[1]) , 0 , 1) ;
        }
    if(status == OK)  
	return (maxbytes);
    else 
	return status ;
  
    }


/*******************************************************************************
*
* i2cFileWrite - write to a i2cFile file
*
* Perform a write operation to an open i2cFile.
*
* RETURNS: The number of bytes written, or ERROR if past the end of memory or
* is O_RDONLY only.
*
* ERRNO
* \is
* \i EINVAL
* \i EISDIR
* \ie
*/

LOCAL int i2cFileWrite
    (
    I2C_FILE_DEV *pfd,	/* file descriptor of file to write */
    char *buffer,	/* buffer to be written */
    int nbytes		/* number of bytes to write from buffer */
    )
    {
   int status = ERROR ;

    /* Fail if the mode is invalid, or if it's a directory. */
    if (pfd->mode == O_RDONLY)
	{
	errnoSet (EINVAL);
	return (ERROR);
	}
    if(pfd  != pI2cfileDrv)
	{
	errnoSet (EINVAL);
	return (ERROR);
	}
	
    if (nbytes > 0)
        {
       status = bsp_i2c_write(buffer[0] & 0xFE , nbytes -1 , &(buffer[1])  , 0 , 1);
        }

    if(status == OK)  
       return (nbytes);
    else 
	return status ;
    }


/*******************************************************************************
*
* i2cFileIoctl - do device specific control function
*
*
* RETURNS: OK, or ERROR if seeking passed the end of memory.
*
* ERRNO
* \is
* \i EINVAL
* \i S_ioLib_UNKNOWN_REQUEST
* \ie
*/

LOCAL STATUS i2cFileIoctl
    (
    I2C_FILE_DEV* pfd,	/* descriptor to control */
    int function,	/* function code */
    int	arg		/* some argument */
    )
    {
    if(pfd  != pI2cfileDrv)
	{
	errnoSet (EINVAL);
	return (ERROR);
	}
    return (OK);
    }


/*******************************************************************************
*
* i2cFileClose - close a i2cFile
*
* RETURNS: OK, or ERROR if file couldn't be flushed or entry couldn't 
* be found.
*
* ERRNO: N/A.
*/

LOCAL STATUS i2cFileClose
    (
    I2C_FILE_DEV* pfd	/* file descriptor of file to close */
    )
    {
    return OK;
    }

/*******************************************************************************
*
* i2cFileDevCreate - create a i2cFile device
*
* This routine initializes the i2cFile driver. It is called in i2c Iniatlize when RTP is ENABLE.
*
* RETURNS: OK, or ERROR if memory is insufficient or the I/O system cannot add
* the device.
*
* ERRNO: S_ioLib_NO_DRIVER
*
*/

STATUS i2cFileDevCreate 
    (
    char * name,		/* device name		    */
    int    length		/* MAX buffer number of bytes supprot by i2c dev  */
    )
    {
    STATUS status;

    if (i2cFileDrvNum > 0)
	return (OK);	/* driver already installed */

     i2cFileDrvNum = iosDrvInstall ((FUNCPTR) i2cFileOpen, (FUNCPTR) NULL,
			       (FUNCPTR) i2cFileOpen, i2cFileClose,
			       i2cFileRead, i2cFileWrite, i2cFileIoctl);
	
    if (i2cFileDrvNum < 1)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    if ((pI2cfileDrv = (I2C_FILE_DEV *) calloc (1, sizeof (I2C_FILE_DEV))) == NULL)
	return (ERROR);

    pI2cfileDrv->mode  = O_RDWR;
    pI2cfileDrv->maxBufLen = length ;

    status = iosDevAdd ((DEV_HDR *) pI2cfileDrv, name, i2cFileDrvNum);
		
    if (status == ERROR)
	{
		free ((char *) pI2cfileDrv);
	        pI2cfileDrv = NULL ;
    	}
    return (status);
    }

/*******************************************************************************
*
* i2cFileDevDelete - delete a i2cFile device
*
* RETURNS: OK, or ERROR if the device doesn't exist.
*
* ERRNO: N/A.
*/

STATUS i2cFileDevDelete
    (
    char * name			/* device name */
    )
    {
    DEV_HDR * pDevHdr;

    /* get the device pointer corresponding to the device name */
 
    if ((pDevHdr = iosDevFind (name, NULL)) == NULL)
        return (ERROR);

    /* delete the device from the I/O system */

    iosDevDelete (pDevHdr);

    /* free the device pointer */

    free ((I2C_FILE_DEV *) pDevHdr);

    return (OK);
    }

#endif /* INCLUDE_RTP*/

