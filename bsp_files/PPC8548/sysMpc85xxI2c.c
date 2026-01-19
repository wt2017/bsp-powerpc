/* sysMpc85xxI2c.c - I2C Driver Source Module */

/* Copyright (c) 2005-2006 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,27jan06,dtr  Fix EepromShow and tidyup.
01b,04apr05,mdo  Documentation fixes for apigen
01a,28feb05,dtr  created.
*/

/*
DESCRIPTION

I2C Driver Source (Low Level Routines) Module
Mpc85xx Memory Controller (PowerPlus Architecture)

Notes:
   1. GREAT care must be made when sending commands to the 
      MPC85XX(Mpc85xx).  Several "i2cCycleMpc85xxDelay" calls are made
      in order to not overwhelm the I2C interface.  If commands
      are sent too fast, the I2C interface will lockup and the I2C
      bus may become unusable without a powercycle.  Generally, if
      you cause a I2C bus cycle, you should wait for "a while".
      A possible cause is that the caches were turned on when this
      driver was written.

INCLUDE FILES:   sysMpc85xxI2c.h
*/

/* includes */

#include <vxWorks.h>        /* vxWorks generics */
#include "config.h"
#include "sysMotI2c.h"
#include "sysMpc85xxI2c.h"  /* low level definitions, specific */
#include <stdio.h>
#include <string.h>
#include <logLib.h>
#include <stdlib.h>
#include <taskLib.h>
#include <arch/ppc/vxPpcLib.h>

/* externals */
IMPORT I2C_DRV_CTRL * pI2cDrvCtrl[2] ;
IMPORT void sysMsDelay(UINT);
IMPORT SEM_ID i2cAckSynBSem;
IMPORT SEM_ID i2cReadSynBSem;

/* forward declarations */

void sysMpc85xxMsDelay (UINT mSeconds);
void i2cCycleMpc85xxDelay (int mSeconds);
void i2cInit();

/* debug declarations */

#define SPD_SIZE        256
#define REAL_SPD_SIZE   256
char defaultSPD[SPD_SIZE];


/******************************************************************************
* 
* i2cCycleMpc85xxStart - perform I2C "start" cycle
* 
* This function's purpose is to perform an I2C start cycle.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/
int i2cCycleMpc85xxStart 
    (
    int unit
    ) 
    {
    unsigned int timeOutCount;
    UINT8 statusReg = 0;

    if (pI2cDrvCtrl[unit] == NULL)
        return -1 ;

     /* 
     * wait until the I2C bus is free.  if it doesn't become free
     * within a *reasonable* amount of time, exit with an error.
     */

    for ( timeOutCount = 10000; timeOutCount; timeOutCount-- )
    {
        statusReg = i2cIoctl(I2C_IOCTL_RD, 
                            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_STATUS_REG), 0, 0);

        if ( !(statusReg & MPC85XX_I2C_STATUS_REG_MBB) )
        {
            break;
        }
    }

    if ( !timeOutCount )
        {
        return(-1); 
        }

    /*
     * since this is the first time through, generate a START(MSTA) and
     * place the I2C interface into a master transmitter mode(MTX).
     */

    i2cIoctl(I2C_IOCTL_RMW_OR, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
            (MPC85XX_I2C_CONTROL_REG_MTX |
             MPC85XX_I2C_CONTROL_REG_MSTA),0);

    return(0);

    }

void i2cForceMpc85xxStop (int unit)
{
    if(TRUE == g_BspI2cDebug)
    printf("i2cForceMpc85xxStop\r\n");

    /*实验做出来的,freescal技术并没有给出说明,他们说没办法*/
    i2cIoctl(I2C_IOCTL_WR, 
        (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
         0xc8 , 0);
    i2cIoctl(I2C_IOCTL_WR, 
        (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
         0x0 , 0);
    i2cIoctl(I2C_IOCTL_WR, 
        (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
         0xe0 , 0);
    i2cIoctl(I2C_IOCTL_WR, 
        (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
         0xc8 , 0);

    sysUsDelay(10);

    i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
         (UINT32)(pI2cDrvCtrl[unit]->baseAdrs + MPC85XX_I2C_CONTROL_REG),
         ((UINT8)~MPC85XX_I2C_CONTROL_REG_MEN), 
         MPC85XX_I2C_CONTROL_REG_MSTA);
    
    i2cIoctl(I2C_IOCTL_RMW_AND, 
         (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG),
         ((UINT8)~MPC85XX_I2C_CONTROL_REG_MSTA), 0);
    
    i2cIoctl(I2C_IOCTL_RMW_OR, 
         (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
         (MPC85XX_I2C_CONTROL_REG_MEN | MPC85XX_I2C_CONTROL_REG_MIEN), 0);
         
    return ;
}

/******************************************************************************
* 
* i2cCycleMpc85xxStop - perform I2C "stop" cycle
* 
* This function's purpose is to perform an I2C stop cycle.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/
int i2cCycleMpc85xxStop 
    (
    int unit
    )
    {
    unsigned long statusReg ;
    STATUS status;
    UINT   timeOutCount;
    if (pI2cDrvCtrl[unit] == NULL)
        return -1 ;

    /*
     * turn off MSTA bit(which will generate a STOP bus cycle)
     * turn off MTX bit(which places the MPC85XX interface into receive mode
     * turn off TXAK bit(which allows 9th clock cycle acknowledges)
     */
    i2cIoctl(I2C_IOCTL_RMW_AND, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG), 
            ((UINT8)~(MPC85XX_I2C_CONTROL_REG_MTX  |
               MPC85XX_I2C_CONTROL_REG_MSTA |
               MPC85XX_I2C_CONTROL_REG_TXAK)), 0);

    for ( timeOutCount = 3000; timeOutCount; timeOutCount-- )
    {
        statusReg = i2cIoctl(I2C_IOCTL_RD, 
                           (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_STATUS_REG), 
                           0, 0);
        if (((statusReg & MPC85XX_I2C_STATUS_REG_MBB) == 0) /*idle*/
            && ((statusReg & MPC85XX_I2C_STATUS_REG_MAL) == 0) )
        {
            status = OK;
            return(0);
        }
    }
    
    if (((statusReg & MPC85XX_I2C_STATUS_REG_MBB) != 0)
        || ((statusReg & MPC85XX_I2C_STATUS_REG_MAL) != 0))
	{
        if(TRUE == g_BspI2cDebug)		
        {
            printf("%s(line %d) mpc8548i2cCycleStop fail , i2c bus have been reset\r\n",
                    __FUNCTION__, __LINE__);
        }
        for ( timeOutCount = 20; timeOutCount; timeOutCount-- )
        {
        	i2cForceMpc85xxStop(unit);
            sysUsDelay(10);
            statusReg = i2cIoctl(I2C_IOCTL_RD, 
                               (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_STATUS_REG), 
                               0, 0);
                               
            if(TRUE == g_BspI2cDebug)       
            {
                printf("statusReg = 0x%x\r\n", statusReg);
            }
            if (((statusReg & MPC85XX_I2C_STATUS_REG_MBB) == 0) /*idle*/
                && ((statusReg & MPC85XX_I2C_STATUS_REG_MAL) == 0) )
            {
                status = OK;
                return(0);
            }
        }
	}

    return(-1);

    }


/******************************************************************************
* 
* i2cCycleMpc85xxRead - perform I2C "read" cycle
* 
* This function's purpose is to perform an I2C read cycle.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/
int i2cCycleMpc85xxRead
    (
    int unit,
    unsigned char *pReadDataBuf,    /* pointer to read data buffer */
    int ack    /* 0 = don't ack, 1 = ack */
    )
{

    if (pI2cDrvCtrl[unit] == NULL)
        return -1 ;
    
    /*
     * place the I2C interface into receive mode(MTX=0) and set the interface
     * to NOT acknowledge(TXAK=1) the incoming data on the 9th clock cycle.
     * this is required when doing random reads of a I2C device.
     */
#if 0
    if (!ack)
        {
        /* Don't send master ack. */
        i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG),
            ((UINT8)~MPC85XX_I2C_CONTROL_REG_MTX),
              MPC85XX_I2C_CONTROL_REG_TXAK);
        }
    else
        {
        /* Send master ack. */
        i2cIoctl(I2C_IOCTL_RMW_AND_OR, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_CONTROL_REG),
            ((UINT8)~(MPC85XX_I2C_CONTROL_REG_MTX|MPC85XX_I2C_CONTROL_REG_TXAK)),
              0);
        }
#endif


	if( semTake(i2cReadSynBSem , 2) == ERROR)
    {
        if(TRUE == g_BspI2cDebug)
      		printf("semTake i2cReadSynBSem err\r\n") ;
   		return ERROR ;
    }
    else
    {
        /* now do the actual read, make this one count */
        *pReadDataBuf = i2cIoctl(I2C_IOCTL_RD, 
                           (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_DATA_REG), 
                           0, 0);


        if(TRUE == g_BspI2cDebug)
        {
            printf("%s(line %d) readData = 0x%x\r\n", __FUNCTION__, __LINE__, *pReadDataBuf);
        }        
        return(0);
    }
}


/******************************************************************************
* 
* i2cCycleMpc85xxWrite - perform I2C "write" cycle
* 
* This function's purpose is to perform an I2C write cycle.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/
int i2cCycleMpc85xxWrite
    (
    int             unit,
    unsigned char   writeData   /* character to write */
    )
    {
    if (pI2cDrvCtrl[unit] == NULL)
        return -1 ;

    /*
     * write the requested data to the data register, which will cause
     * it to be transmitted on the I2C bus.
     */

    i2cIoctl(I2C_IOCTL_WR, 
            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_DATA_REG), 
            (UINT32)writeData, 0);

    return(0);
    }


/******************************************************************************
* 
* i2cCycleMpc85xxAckIn - perform I2C "acknowledge-in" cycle
* 
* This function's purpose is to perform an I2C acknowledge-in cycle.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/
int i2cCycleMpc85xxAckIn 
    (
    int unit
    )
{
    if (pI2cDrvCtrl[unit] == NULL)
        return -1 ;

    /*
     * wait until an *internal* device interrupt has been generated, then
     * clear it.  if it is not received, return with an error.
     * we are polling, so NO processor interrupt is generated.
     */

	if( semTake(i2cAckSynBSem , 2) == ERROR)
	{
        if(TRUE == g_BspI2cDebug)
        {
    		printf("bsp_i2c_ack_delay\r\n") ;
        }
		return ERROR ;
	}
    return(0);
}


/******************************************************************************
* 
* i2cCycleMpc85xxAckOut - perform I2C "acknowledge-out" cycle
* 
* This function's purpose is to perform an I2C acknowledge-out cycle.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/
int i2cCycleMpc85xxAckOut 
    (
    int unit
    )
    {
    if (pI2cDrvCtrl[unit] == NULL)
        return -1 ;

    return(0);
    }


/******************************************************************************
* 
* i2cCycleMpc85xxKnownState - initialize the I2C bus to a known state
* 
* This function's purpose is to initialize the I2C bus to a known state.
* 
* RETURNS:
*   zero        = operation successful
*   non-zero    = operation failed
*
* ERRNO: N/A
*/

int i2cCycleMpc85xxKnownState 
    (
    int unit
    )
    {
    STATUS status;
    UINT   timeOutCount;
    UINT   statusReg;

    status = OK;


    if (pI2cDrvCtrl[unit] == NULL)
        return ERROR ;
    
    /*
     * wait until the I2C bus is free.  if it doesn't become free
     * within a *reasonable* amount of time, exit with an error.
     */

    for ( timeOutCount = 3000; timeOutCount; timeOutCount-- )
        {
        statusReg = i2cIoctl(I2C_IOCTL_RD, 
                            (UINT32)(pI2cDrvCtrl[unit]->baseAdrs+MPC85XX_I2C_STATUS_REG), 0, 0);

        if ( !(statusReg & MPC85XX_I2C_STATUS_REG_MBB) )
            {
            status = OK;
            break;
            }

        /* 
         * re-initialize the I2C if the BUS BUSY does not clear
         * after trying half the *reasonable* amount of reads of the 
         * status register.
         */

        if ( !(timeOutCount % 50) )
            {
            status = i2cDrvInit(unit, I2C_DRV_TYPE);
            if ( status == OK )
                break;
            else
                return ERROR ;
            }
        }

    if ( !timeOutCount )
        status = ERROR;

    return(status);
    }


/******************************************************************************
* 
* i2cCycleMpc85xxDelay - perform interface's I2C delay routine 
* 
* This function's purpose is to perform whatever delay required for the device.
* 
* RETURNS:  N/A
*
* ERRNO: N/A
*/
void i2cCycleMpc85xxDelay
    (
    int mSeconds    /* time to delay in milliseconds */
    )
    {

    sysMpc85xxMsDelay(mSeconds);

    }


/******************************************************************************
*
* sysMpc85xxMsDelay - delay for the specified amount of time (MS)
*
* This routine will delay for the specified amount of time by counting
* decrementer ticks.
*
* This routine is not dependent on a particular rollover value for
* the decrementer, it should work no matter what the rollover
* value is.
*
* A small amount of count may be lost at the rollover point resulting in
* the sysMpc85xxMsDelay() causing a slightly longer delay than requested.
*
* This routine will produce incorrect results if the delay time requested
* requires a count larger than 0xffffffff to hold the decrementer
* elapsed tick count.  For a System Bus Speed of 67 MHz this amounts to
* about 258 seconds.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
void sysMpc85xxMsDelay
    (
    UINT        delay            /* length of time in MS to delay */
    )
    {
    sysMsDelay(delay);
    }

