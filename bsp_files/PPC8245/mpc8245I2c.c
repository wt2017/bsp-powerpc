/* mpc8245I2c.c - MPC8245 I2C support */

/* Copyright 1996-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,15sep01,dat  Use of vxDecGet routine
01c,11sep00,rcs  fix include paths
01b,22jun00,bri  updated to WindRiver coding standards.
01a,18jun00,bri  created - based on  Kahlua I2C driver.
*/

/*
DESCRIPTION
This module contains routines for the I2C  interface of MPC8245. 
this driver codes are change from mpc107I2c.c wiritten in tornado2.2 system

The I2C interface is a two-wire, bidirectional serial bus that provides
a simple, efficient way to exchange data between integrated circuit(IC) devices.
The I2C interface allows the MPC8245 to exchange data with other I 2 C devices
such as microcontrollers, EEPROMs, real-time clock devices, A/D converters,
and LCDs.

This module provides the routines  for writing/reading  data to/from
the I2C interface with MPC8245 as a "master"  device .

The routine mpc8245i2cWrite() is used for writing  data to an EEPROM type
slave device .
The routine mpc8245i2cRead() is used for reading data from an EEPROM type
slave device .

Support for other I2C slave devices can be added easily by using the
routines provided in this module .

Reading/writing data from/to the I2C interface with MPC8245 as a "slave" device
are not supported in this module .

However, additional I2C bus controllers can be easily be added as required.

.SH INITIALIZATION
The routine mpc8245I2cInit() should be called to  initialize the
IIC interface.

The user has to specify the the following parameters for
programming the I2C Frequency Divider Register (I2CFDR):
.IP DFFSR
Specifies the Digital filter frequency sampling rate of the I2C interface.
DFFSR can be changed by changing MPC8245_DFFSR_DATA in the header file.
.IP FDR
Specifies the Frequency divider ratio.FDR can be changed by changing
MPC8245_FDR_DATA in the header file .
.LP

The user has to specify the the following parameters for
programming the I2C Address Register (I2CADR) :
.IP ADDR
Specifies the  Slave address used by the I2C interface when MPC8245 is configured
as a slave.ADDR can be changed by changing MPC8245_ADR_DATA in the header file
.LP

Refer to MPC8245 users manual for further details about the values
to be programmed in the I2C registers .
*/

/* includes */

#include "vxWorks.h"
#include "sysLib.h"
#include "stdlib.h"
#include "mpc8245I2c.h"
#include "Epic.h"
#include "semLib.h"
#include "vxLib.h"

/* globals  */

/* static file scope locals */

int  g_i2cDebugInfo = TRUE;    /* debug infor*/
LOCAL SEM_ID i2cMutexSem = NULL ;

/* forward Declarations */

LOCAL INT32	mpc8245i2cCycleStart (UINT32 repeatStart);
LOCAL void	mpc8245i2cCycleStop (void);
LOCAL void 	mpc8245i2cCycleRead (UCHAR* data);
LOCAL void 	mpc8245i2cCycleWrite (UCHAR data);
LOCAL STATUS mpc8245i2cCycleAck (void);
LOCAL UINT32 	mpc8245GetDec (void);
LOCAL UINT32 	mpc8245I2cRegRdWrMod (UINT32 operation , UINT32 address ,
                                     UINT32 data1, UINT32 data2);
LOCAL VOID 	mpc8245I2cDoOperation (UINT32 deviceAddress,
                                      MPC8245_I2C_CMD_PCKT * pI2cCmdPacket);

/***************************************************************************
*
* mpc8245I2cInit - initialize the IIC interface
*
* This routine initializes the IIC Interface.
*
* The  "Frequency Divider Register" is initialized depending on
* the clock frequency.The slave address is also programmed. The IIC
* interface is programmed to be in Master/Receive Mode with interrupts disabled.
* After all the initialization is done  the IIC interface is enabled.

* This routine must be called before the user can use the IIC interface to
* read /write data.
*
* RETURNS: N/A
*/

void bsp_i2c_init (void)
    {

    /* Update the frequency divider register */

    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CFDR,
                           0 , 0);
    SYNC;

    /* Enable the I2C interface     */

    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CCR,
                        MPC8245_I2CCR_MEN,0);
    SYNC;

	if(i2cMutexSem == NULL)
   		i2cMutexSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);

    }


/***************************************************************************
*
* mpc8245i2cRead -  read  from the slave device on IIC bus
*
* This routine  reads the specified number of bytes from the specified slave
* device with MPC8245 as the master  .
*
* RETURNS: OK, or Error on a bad request
*/

INT32 bsp_i2c_read
    (
    UINT32 	deviceAddress,	    /* Device I2C  bus address */
    UINT32 	numBytes,         	/* number of Bytes to read */
    char *	pBuf ,   	        /* pointer to buffer of send data */
    UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
    UINT32   stop           /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    )

	{
	MPC8245_I2C_CMD_PCKT 	i2cCmdPacket;	/* command packet */

	/* Check for a bad request. */

	if (numBytes == 0)
		{
		return (ERROR);
		}

	/* Build command packet. */

	i2cCmdPacket.command = MPC8245_I2C_READOP;
	i2cCmdPacket.status = 0;
	i2cCmdPacket.memoryAddress = (UINT32)pBuf;
	i2cCmdPacket.nBytes = numBytes;
	i2cCmdPacket.repeatStart = (UINT32)repeatStart;
	i2cCmdPacket.stop = stop;

	semTake (i2cMutexSem, WAIT_FOREVER);
	mpc8245I2cDoOperation(deviceAddress, (MPC8245_I2C_CMD_PCKT *)&i2cCmdPacket);
	semGive (i2cMutexSem);

	/* Return the appropriate status. */

	return (i2cCmdPacket.status);
	}


/***************************************************************************
*
* mpc8245i2cWrite - write to the slave device on IIC bus
*
* This routine is used to write the specified number of
* bytes to the specified slave device with  MPC 8245 as a master
*
* RETURNS: OK, or ERROR on a bad request.
*/

INT32 bsp_i2c_write
    (
    UINT32 	deviceAddress,	 /* Devices I2C bus address */
    UINT32 	numBytes,	     /* number of bytes to write */
    char *	pBuf ,   	     /* pointer to buffer of send data */
    UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
    UINT32   stop           /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    )
	{
	MPC8245_I2C_CMD_PCKT 	i2cCmdPacket;	/* command packet */

	/* Check for a NOP request. */

	if (numBytes == 0)
		{
		return (ERROR);
		}

	/* Build command packet. */

	i2cCmdPacket.command = MPC8245_I2C_WRITOP;
	i2cCmdPacket.status = 0;
	i2cCmdPacket.memoryAddress = (UINT32)pBuf;
	i2cCmdPacket.nBytes = numBytes;
	i2cCmdPacket.repeatStart = (UINT32)repeatStart;
	i2cCmdPacket.stop = stop;
	
	/* Take ownership, call driver, release ownership. */

	semTake (i2cMutexSem, WAIT_FOREVER);
	mpc8245I2cDoOperation(deviceAddress, (MPC8245_I2C_CMD_PCKT *)&i2cCmdPacket);
	semGive (i2cMutexSem);

	/* Return the appropriate status. */

	return (i2cCmdPacket.status);

	}



/***************************************************************************
*
* mpc8245i2cCycleRead - perform I2C "read" cycle
*
* This routine is used to  perform a read from the I2C bus .
*
* RETURNS: N/A
*/

LOCAL void mpc8245i2cCycleRead
    (
    UCHAR *	pReadDataBuf	/* pointer to read data buffer */
    )
    {
    UINT32 readData = 0;

    bsp_cycle_delay (1);

    /* Perform the actual read */

    readData = mpc8245I2cRegRdWrMod(MPC8245_I2C_READ, MPC8245_I2C_I2CDR, 0, 0);
    SYNC;

    *pReadDataBuf = (UCHAR)readData; /* copy the read data */

    return;

    }


/***************************************************************************
*
* mpc8245i2cCycleWrite - perform I2C "write" cycle
*
* This routine is used is to perform a write to the I2C bus.
* Data is written to the I2C interface .
*
* RETURNS: N/A
*/

LOCAL void mpc8245i2cCycleWrite
    (
    UCHAR writeData	/* character to write */
    )
    {

    bsp_cycle_delay (1);

    /*
     * Write the requested data to the data register, which will cause
     * it to be transmitted on the I2C bus.
     */

    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CDR, writeData, 0);
    SYNC;

    bsp_cycle_delay (1);

    return;

    }


/***************************************************************************
*
* mpc8245i2cCycleAck - perform I2C "acknowledge-in" cycle
*
* This routine is used to perform an I2C acknowledge-in  on the bus.
*
* RETURNS: OK, or ERROR if timed out waiting for an ACK .
*/

LOCAL STATUS mpc8245i2cCycleAck (void)
    {
    UINT32 	statusReg = 0;
    UINT32 	timeOutCount;

    for (timeOutCount = 100; timeOutCount; timeOutCount--)
        {
        bsp_cycle_delay (1);

        statusReg = mpc8245I2cRegRdWrMod(MPC8245_I2C_READ, MPC8245_I2C_I2CSR,0,0);
        SYNC;
        if (((statusReg & MPC8245_I2CSR_MCF) == MPC8245_I2CSR_MCF)	
			&&((statusReg & MPC8245_I2CSR_MIF) == MPC8245_I2CSR_MIF))
            {
			mpc8245I2cRegRdWrMod(MPC8245_I2C_READ_AND_WRITE,
                                MPC8245_I2C_I2CSR,~(MPC8245_I2CSR_MIF),0);
			
            SYNC;			
            break;             
            }
        }

    if (timeOutCount == 0)
        {
        return (ERROR);
        }

    return (OK);

    }



/***************************************************************************
*
* mpc8245i2cCycleStart - perform I2C "start" cycle
*
* This routine is used is to perform an I2C start cycle.
*
* RETURNS: OK, or ERROR if timed out waiting for the IIC bus to be free.
*/

LOCAL STATUS mpc8245i2cCycleStart (UINT32 repeatStart)
    {
    UINT32 	timeOutCount;
    UINT32 	statusReg = 0;

    /*
     * if this is a repeat start, then set the required bits and return.
     * this driver ONLY supports one repeat start between the start cycle and
     * stop cycles.
     */

    if(repeatStart)  /* If a repeat start */
        {
//       bsp_cycle_delay (1);

        mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CCR,
                    (MPC8245_I2CCR_MEN | MPC8245_I2CCR_RSTA | MPC8245_I2CCR_MSTA|
                     MPC8245_I2CCR_MTX),0);
        SYNC;
//       bsp_cycle_delay (1);
        return(OK);
        }
	
    /*
     * wait until the I2C bus is free.  if it doesn't become free
     * within a *reasonable* amount of time, exit with an error.
     */

    for (timeOutCount = 10; timeOutCount; timeOutCount--)
        {
        bsp_cycle_delay (1);

        statusReg = mpc8245I2cRegRdWrMod(MPC8245_I2C_READ,
                                        MPC8245_I2C_I2CSR, 0, 0);
        SYNC;

        if ((statusReg & MPC8245_I2CSR_MBB) == 0)
            {
            break;
            }
        }

    if (timeOutCount == 0)
        {
        return (ERROR);
        }

    /*
     * since this is the first time through, generate a START(MSTA) and
     * place the I2C interface into a master transmitter mode(MTX).
     */

    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CCR,
                       (MPC8245_I2CCR_MEN | MPC8245_I2CCR_MTX | MPC8245_I2CCR_MSTA ),0);
    SYNC;
 // bsp_cycle_delay (1);

    return (OK);

    }

/***************************************************************************
*
* mpc8245i2cCycleStop - perform I2C Force stop cycle
* This routine is used is to perform an I2C Force stop cycle.
* RETURNS: N/A
*/

LOCAL void mpc8245i2cForceStop (void)
    {

	/*Disable the I2C and set the master bit*/
    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CCR,
                         MPC8245_I2CCR_MSTA, 0);

	SYNC;

	/*Enable the I2C*/
    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CCR,
                         MPC8245_I2CCR_MSTA | MPC8245_I2CCR_MEN, 0);

	SYNC;
 // bsp_cycle_delay (1);
    /*Read the I2CDR.*/
    mpc8245I2cRegRdWrMod(MPC8245_I2C_READ, MPC8245_I2C_I2CDR, 0, 0) ;

	SYNC;
    bsp_cycle_delay (1);
	/*Return the MPC8245 to slave mode by setting*/
    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE, MPC8245_I2C_I2CCR,
                         MPC8245_I2CCR_MEN, 0);

	SYNC;
   // bsp_cycle_delay (1);
    return ;

    }

/***************************************************************************
*
* mpc8245i2cCycleStop - perform I2C "stop" cycle
*
* This routine is used is to perform an I2C stop cycle.
*
* RETURNS: N/A
*/

LOCAL void mpc8245i2cCycleStop (void)
    {
    UINT32 statusReg ;
    bsp_cycle_delay (1);

    /*
     * turn off MSTA bit(which will generate a STOP bus cycle)
     * turn off MTX bit(which places the I2C interface into receive mode
     * turn off TXAK bit(which allows 9th clock cycle acknowledges)
     */

    mpc8245I2cRegRdWrMod(MPC8245_I2C_READ_AND_WRITE, MPC8245_I2C_I2CCR,
                       (~(MPC8245_I2CCR_MTX  | MPC8245_I2CCR_MSTA |
                          MPC8245_I2CCR_TXAK)), 0);
    SYNC;
	bsp_cycle_delay (1);

	statusReg = mpc8245I2cRegRdWrMod(MPC8245_I2C_READ, MPC8245_I2C_I2CSR, 0, 0);
	SYNC;

	if ((statusReg & MPC8245_I2CSR_MBB) != 0)
		{
		if(g_i2cDebugInfo)
			printf("mpc8245i2cCycleStop fail , i2c bus have been reset\n");
		mpc8245i2cForceStop();
		}
	
    return ;

    }

/***************************************************************************
*
* mpc8245i2cRegRdWrMod - i2c Registers In/OutLong and/or-ing wrapper.
*
* The purpose of this routine is to perform AND, OR, and
* AND/OR  or In/Out operations with syncronization.
*
* RETURNS: The data read for read operations.
*/

LOCAL UINT32 mpc8245I2cRegRdWrMod
    (
    UINT32 	ioControlFlag,  /* input/ouput control flag         */
                                /* 0, write                         */
                                /* 1, read                          */
                                /* 2, read/modify/write (ORing)     */
                                /* 3, read/modify/write (ANDing)    */
                                /* 4, read/modify/write (AND/ORing) */
    UINT32 	address,        /* address of device register       */
    UINT32 	wdata1,         /* data item 1 for read/write operation */
    UINT32 	wdata2          /* data item 2 for read/write operation */
    )
    {
    UINT32 i2cTempData = 0;

    if (ioControlFlag == MPC8245_I2C_WRITE)  /* Write */
        {

        /*
         * Data <wdata1> is to be written in
         * the register specified by <address>
         */

       sysEUMBBARWrite(address, wdata1);


        }
    else if (ioControlFlag == MPC8245_I2C_READ) /* Read */
        {

        /* Data is read from the register  specified by <address> */

        i2cTempData =sysEUMBBARRead(address);


        }
    else if (ioControlFlag == MPC8245_I2C_READ_OR_WRITE) /* Read OR Write */
        {

        /*
         * Data <wdata1> is bitwise ORed with the data read
         * from the register specified by <address>  and the
         * resultant  data is written to the register
         */

        i2cTempData =sysEUMBBARRead(address) | wdata1 ;
        SYNC;
        sysEUMBBARWrite(address, i2cTempData);
        }

    else if (ioControlFlag == MPC8245_I2C_READ_AND_WRITE)  /* Read AND Write */
        {

        /*
         * Data <wdata1> is bitwise ANDed with the data read
         * from the register specified by address  and the
         * resultant  data is written to the  register
         */

        i2cTempData =sysEUMBBARRead(address) & wdata1;
        SYNC;
        sysEUMBBARWrite(address, i2cTempData);

        }

    else if (ioControlFlag == MPC8245_I2C_READ_ANDOR_WRITE)/* Read ANDOR write */
        {

        /*
         * Data <wdata1> is bitwise ANDed with the data read
         * from the register specified by <address> and  data
         * <wdata2> is bitwise ORed, and the resultant  data
         * is written to the register
         */


        i2cTempData =sysEUMBBARRead(address);
        SYNC;
        i2cTempData &= wdata1;
        i2cTempData |= wdata2;
        sysEUMBBARWrite(address, i2cTempData);

        }

    SYNC;

    return (i2cTempData);

    }


/***************************************************************************
*
* mpc8245I2cDoOperation - i2c do operation
*
* The  purpose of this routine is to execute the operation as specified
* by the passed command packet.
*
* RETURNS: N/A
*/

LOCAL VOID mpc8245I2cDoOperation
    (
    UINT32 		deviceAddress,	/* device I2C bus address */
    MPC8245_I2C_CMD_PCKT * pI2cCmdPacket /* pointer to command packet */
    )
    {
    INT32 	byteCount = 0 ;	/* byte counter */
    INT32 	statusVariable = 0 ;	/* local status variable */
    UCHAR *	pWriteData = NULL;	/* pointer to write data buffer */

    /* NORMAL start --Command interface to stop.  This is for previous error exits. */
    if(pI2cCmdPacket->repeatStart == 0)  
		mpc8245i2cCycleStop ();

    if (pI2cCmdPacket->command == MPC8245_I2C_READOP) /* read operation  */
		{

		statusVariable = 0;

		/* Start cycle */
		if ((mpc8245i2cCycleStart (pI2cCmdPacket->repeatStart)) != 0)
			{
			statusVariable = MPC8245_I2C_ERR_CYCLE_START;
			if(g_i2cDebugInfo) 
				printf("i2c Read (Address = 0x%x)Start fail \n" , deviceAddress);
			goto operation_exit;
			}

		/* Write the device address */
		mpc8245i2cCycleWrite (deviceAddress | pI2cCmdPacket->command);		
		if ((mpc8245i2cCycleAck ()) != OK )
			{
			statusVariable = MPC8245_I2C_ERR_CYCLE_ACKIN;
			if(g_i2cDebugInfo) 
				printf("i2c Read (write date = 0x%x) no ACK \n" , deviceAddress );
			goto operation_exit;
			}

	    /*
	     * Configure the I2C interface into receive mode(MTX=0) and set
	     * the interface  to NOT acknowledge(TXAK=1) the incoming data on
	     * the 9th clock cycle.
	     * This is required when doing random reads of a I2C device.
	     */
	    mpc8245I2cRegRdWrMod(MPC8245_I2C_WRITE,
	                        MPC8245_I2C_I2CCR,(MPC8245_I2CCR_MSTA|MPC8245_I2CCR_MEN), 0);
	    SYNC;		
        /*random reads*/		
		mpc8245i2cCycleRead((UCHAR *)(pI2cCmdPacket->memoryAddress)) ;
	    bsp_cycle_delay (1);  /*must delay for some type of slower chips */
		
		for (byteCount = 0; byteCount < pI2cCmdPacket->nBytes; byteCount++)
			{
			/*last READ byte , disable ack singal on SDA*/
			if(byteCount == (pI2cCmdPacket->nBytes-1))	
				{
				mpc8245I2cRegRdWrMod(MPC8245_I2C_READ_OR_WRITE,
					MPC8245_I2C_I2CCR , MPC8245_I2CCR_TXAK , 0);
				
				}			
			/* Read the data from the IIC interface */
			mpc8245i2cCycleRead ((UCHAR *)(pI2cCmdPacket->memoryAddress + byteCount));

			if (mpc8245i2cCycleAck() != OK) 
				{
				statusVariable = MPC8245_I2C_ERR_CYCLE_ACKIN;
				if(g_i2cDebugInfo) 
					printf("i2c Read (read date = 0x%x) no ACK \n" ,byteCount);
				goto operation_exit;
				}

			}

		/* if stop flag != 0 , then Generate the Stop Cycle */
		if(pI2cCmdPacket->stop) 
			mpc8245i2cCycleStop () ;
			
		}

    /*
     * write operation for each byte
     * perform the byte write operation, a delay must be
     * exercised following each byte write
     */

	if (pI2cCmdPacket->command == MPC8245_I2C_WRITOP)
		{

		/* Initialize pointer to caller's write data buffer. */

		pWriteData = (UCHAR *)pI2cCmdPacket->memoryAddress;

		/* Write the specified number of bytes from the EEPROM. */

		statusVariable = 0;

		/* Generate the Start Cycle */

		if ((mpc8245i2cCycleStart (pI2cCmdPacket->repeatStart)) != 0)
			{
			statusVariable = MPC8245_I2C_ERR_CYCLE_START;
			if(g_i2cDebugInfo)  
				printf("i2c write (Address = 0x%x)Start fail \n" , deviceAddress);
			goto operation_exit;
			}

		/* Write the device address */

		mpc8245i2cCycleWrite (deviceAddress | pI2cCmdPacket->command);

		if ((mpc8245i2cCycleAck ()) != OK )
			{
			statusVariable = MPC8245_I2C_ERR_CYCLE_ACKIN;
			if(g_i2cDebugInfo)  
				printf("i2c write (write data = 0x%x) no ACK\n" , deviceAddress);
			goto operation_exit;
			}

		for (byteCount = 0; byteCount < pI2cCmdPacket->nBytes; byteCount++)
			{

			/* Write the data */
			mpc8245i2cCycleWrite (*(pWriteData + byteCount));

			if ((mpc8245i2cCycleAck ()) != OK )
				{
				statusVariable = MPC8245_I2C_ERR_CYCLE_ACKIN;
				if(g_i2cDebugInfo)  
					printf("i2c write (write date = 0x%x) no ACK\n" , (*(pWriteData + byteCount)));
				break;
				}
			}
		
		/* Generate the Stop Cycle */
		if(pI2cCmdPacket->stop) 
			mpc8245i2cCycleStop ();		
		}

operation_exit:
	/* update the caller's command packet with status of the operation */
	pI2cCmdPacket->status = statusVariable;
	if(pI2cCmdPacket->status != 0 )
		{
		if(g_i2cDebugInfo)  
			printf("i2c mpc8245I2cDoOperation error (cmd = %d ) , error code = %d \n" , pI2cCmdPacket->command, pI2cCmdPacket->status);
		mpc8245i2cCycleStop() ;
		}
    }

/***************************************************************************
*
* mpc8245GetDec - read from the Decrementer register SPR22.
*
* This routine will read the contents the decrementer (SPR22)
*
* From a C point of view, the routine is defined as follows:
*
* UINT32 mpc8245GetDec()
*
* RETURNS: value of SPR22 (in r3)
*/

LOCAL UINT32 mpc8245GetDec(void)
    {
    return vxDecGet ();
    }

#define M8260_I2C_BD_BUF_LEN  128

void i2cDbgRead(unsigned char deviceId , unsigned char offset ,unsigned char len)
	{
	unsigned char buffer[M8260_I2C_BD_BUF_LEN] ; 
	unsigned int device_addr = deviceId;

	
	buffer[0] = offset ;
	
	bsp_i2c_write(device_addr , 1 , buffer , FALSE , TRUE) ;
	bsp_cycle_delay (5);
	bsp_i2c_read(device_addr , len , buffer, FALSE , TRUE);
	
	printf("buffer(0~4Byte) = 0x%x , 0x%x ,0x%x ,0x%x \n" , buffer[0], buffer[1], buffer[2], buffer[3]) ;
	}

/***************************************************************************/
 void i2cDbgWrite(unsigned char deviceId , unsigned char offset ,unsigned char len ,char *value)
	{ 
	unsigned char buffer[M8260_I2C_BD_BUF_LEN] ;
	unsigned int device_addr = deviceId;

	buffer[0] = offset ;
	memcpy(&(buffer[1]) ,  value , len);	
	bsp_i2c_write(device_addr , len , buffer, FALSE , TRUE) ;
	}



