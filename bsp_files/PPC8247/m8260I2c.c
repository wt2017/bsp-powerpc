/*修改时间: 2007-11-05
  修改人  : zhengchangwen
  修改历史: 把IIC的工作方式由原来的查询方式改为中断方式
  注意事项: 中断产生后不能立刻读取BD表的状态，而要先延迟一段时间，
            否则BD表中的数据会时对时错。
延迟的时间: 10us～15us之间是临界值，在10～12us时基本上不能用，很快就出错，
            15us时测试半小时左右未发现错误。（IIC总线为200k，即5us一个时钟）
            考虑到一定的余量，采用20us的延迟，对光模块和Inventory器件进行长期测试。   
            测试时间共16个小时的疲劳测试，未发现错误。
*/

/* includes */ 
#include "vxWorks.h"
#include "vme.h"
#include "memLib.h"
#include "cacheLib.h"
#include "sysLib.h"
#include "string.h"
#include "intLib.h"
#include "logLib.h"
#include "stdio.h"
#include "taskLib.h"
#include "vxLib.h"
#include "m8260i2c.h"
#include "drv/parallel/m8260IOPort.h"
#include "drv/sio/m8260Cp.h"
#include "drv/mem/m8260Memc.h"
#include "intLib.h"
#include "drv/intrCtl/m8260IntrCtl.h"


//bsp提供的函数


extern	unsigned int vxImmrGet ();
extern     void sysMsDelay (unsigned int delay) ;  /* length of time in MS to delay */
extern 	void sysUsDelay (unsigned int delay) ;  /* length of time in Us to delay */

#define M8260_I2C_BASE_REG		(vxImmrGet()+ 0x8AFC)  /* I2C's DPRAM pointer */

#define I2C_RX_BD_OFF 			(M8260_I2C_PARAM_OFF+0x80)  /* i2c RX BD descriptor offset addr in dpram */
#define I2C_RX_BUF_OFF 			(M8260_I2C_PARAM_OFF+0x90)  /* i2c RX buffer offset offset ,len = 0x90 */

#define I2C_TX_BD_OFF 			(M8260_I2C_PARAM_OFF+0x120)  /* i2c TX BD descriptor offset addr in dpram */
#define I2C_TX_BUF_OFF 			(M8260_I2C_PARAM_OFF+0x130)  /* i2c TX buffer offset offset ,len = 0x90 */

#define I2C_TX_BUF_ADRS 		(vxImmrGet() + I2C_TX_BUF_OFF)
#define I2C_RX_BUF_ADRS 		(vxImmrGet() + I2C_RX_BUF_OFF)
#define M8260_I2C_BD_BUF_LEN 	0x90    	/* 0x90 byte date buffer of BD */
	
#define M8260_I2C_BASE_REG		(vxImmrGet()+ 0x8AFC)  /* I2C's DPRAM pointer */
 

typedef struct /* I2c_BD */
{
	VUINT16 statusMode; /* status and control */
	VUINT16 dataLength; /* length of data buffer in bytes */
	u_char * dataPointer; /* points to data buffer */
} I2C_BD;

typedef struct /* STRU_I2C_PARAM */
{
	VUINT16 rbase;  /* Rx buffer descriptor base address */
	VUINT16 tbase;  /* Tx buffer descriptor base address */
	VUINT8 rfcr;    /* Rx function code */
	VUINT8 tfcr;    /* Tx function code */
	VUINT16 mrblr;  /* maximum receive buffer length */
	VUINT32 rstate; /* Rx internal state */
	VUINT32 rptr;   /* reserved/internal */
	VUINT16 rbptr;  /* Rx buffer descriptor pointer */
	VUINT16 rcount;   /* reserved/internal */
	VUINT32 rtemp;   /* reserved/internal */
	VUINT32 tstate; /* Tx internal state */
	VUINT32 tptr;   /* reserved/internal */
	VUINT16 tbptr;  /* Tx buffer descriptor pointer */
	VUINT16 tcount;   /* reserved/internal */
	VUINT32 ttemp;   /* reserved/internal */
	VUINT32 sdmatmp;   /* reserved/internal */
} STRU_I2C_PARAM;

typedef STRU_I2C_PARAM *pI2C_PARAM;
typedef I2C_BD *pI2C_BD ;

typedef struct i2c_dev
{
	BOOL init;
	pI2C_PARAM pPram;
	pI2C_BD pTxBdBase;
	UINT16 txBdBaseOffset;
	int txBdNext;
	pI2C_BD pRxBdBase;
	UINT16 rxBdBaseOffset;
	int rxBdNext;
} I2C_DEV;

LOCAL I2C_DEV i2cDev ={FALSE , NULL, NULL, 0, 0, NULL, 0, 0};
LOCAL SEM_ID i2cMutexSem = NULL ;
LOCAL SEM_ID i2cTxSynBSem = NULL ;
LOCAL SEM_ID i2cRxSynBSem = NULL ;
unsigned int g_i2cDebugInfo = 0 ;
static void bsp_i2c_interrupt_handle() ;
/******************************************************************************
*
* initI2C - initializes i2c device
*
* This routine initializes i2c device to prepare for subsequent reads from or
* writes to i2c device.
*
* RETURNS: none
*/

void bsp_i2c_init(void)
{
	I2C_DEV* pI2c;
	unsigned short int *I2c_Base;
	unsigned int immrVal = vxImmrGet();

	pI2c = &i2cDev;
	I2c_Base = (unsigned short int *)M8260_I2C_BASE_REG;/*i2c base pointer in DPRAM*/

	/* i2c initialization cannot occur more than once. */
	if (pI2c->init == TRUE)
	{
		/* Initialize Rx and Tx parameters for I2C. */
		*M8260_CPCR(immrVal) = 0x29610000;

		/* Wait for initialization to complete. */
		while (*M8260_CPCR(immrVal) & 00010000)
			;
	
		printf("I2C has been re-initialed...\n");
		return ;
	}
	
	if(i2cMutexSem == NULL)
   	{
   		i2cMutexSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
	}
	if(i2cRxSynBSem == NULL)
   	{
   		i2cRxSynBSem = semBCreate (SEM_Q_FIFO , SEM_EMPTY);
	}
	if(i2cTxSynBSem == NULL)
   	{
   		i2cTxSynBSem = semBCreate (SEM_Q_FIFO , SEM_EMPTY);
	}	

	/* Set pointer to i2c parameter RAM. */
	*I2c_Base  = M8260_I2C_PARAM_OFF ;
	pI2c->pPram = (pI2C_PARAM)(immrVal + *I2c_Base);

	/* Clear parameter RAM values. */
	memset((char *)pI2c->pPram , 0 , sizeof(pI2c->pPram)) ;
	
	/* Set up I2C BD DESCIPTOR. */
	pI2c->pPram->rbase = (UINT16) I2C_RX_BD_OFF;
	pI2c->pPram->tbase = (UINT16) I2C_TX_BD_OFF;

	pI2c->pRxBdBase = (pI2C_BD) (immrVal + I2C_RX_BD_OFF);
	pI2c->pTxBdBase = (pI2C_BD) (immrVal + I2C_TX_BD_OFF);

	/*
	 * Configure port D pins to enable I2C pins.
	 * PBPAR and PBDIR bits 14,15 ones;
	 * PBODR bits 14,15 zeros.
	 */
	*M8260_IOP_PDPAR(immrVal) |= I2C_INTERFACE_PINS;
	*M8260_IOP_PDDIR(immrVal) &= ~I2C_INTERFACE_PINS;
	*M8260_IOP_PDODR(immrVal) |= I2C_INTERFACE_PINS;
	*M8260_IOP_PDSO(immrVal) |= I2C_INTERFACE_PINS;

	/*I2CBRG REG init*/
	*MPC8260_I2BRG(immrVal) = 15 ;

	/* Initialize Rx and Tx parameters for I2C. */
	*M8260_CPCR(immrVal) = 0x29610000;

	/* Wait for initialization to complete. */
	while (*M8260_CPCR(immrVal) & 00010000)
		;

	/* Write MRBLR with the maximum # of bytes per receive. */
	/* SPI_RX_BUF_SZ is 0x100. */
	pI2c->pPram->mrblr = M8260_I2C_BD_BUF_LEN;

	/* Write RFCR and TFCR for normal operation. */
	/* MPC860_RFCR_MOT_BE is 0x18. */
	pI2c->pPram->rfcr = 0x18;
	pI2c->pPram->tfcr = 0x18;

	/* Initialize buffer descriptors. */
	pI2c->pTxBdBase->statusMode = (I2C_TRANS_TX_BD_LAST | I2C_TRANS_TX_BD_WRAP |I2C_TRANS_TX_BD_INT);
	pI2c->pTxBdBase->dataLength = 0;
	pI2c->pTxBdBase->dataPointer = (u_char *)I2C_TX_BUF_ADRS;

	pI2c->pRxBdBase->statusMode = (I2C_TRANS_RX_BD_EMPTY | I2C_TRANS_RX_BD_LAST | I2C_TRANS_RX_BD_WRAP | I2C_TRANS_RX_BD_INT);
	pI2c->pRxBdBase->dataLength = 0;
	pI2c->pRxBdBase->dataPointer = (u_char *)I2C_RX_BUF_ADRS;

	/* Clear events. */
	*MPC8260_I2CE(immrVal) = 0xFF;

	pI2c->init = TRUE;

	*MPC8260_I2MOD(immrVal) = M8260_MOD_I2C_DIS;
	*MPC8260_I2CM(immrVal) = M8260_RX_TX_INT_ENABLE ;

	intConnect (INUM_TO_IVEC(INUM_I2C), 
		      (VOIDFUNCPTR) bsp_i2c_interrupt_handle, 0);
	
	intEnable(INUM_I2C) ;
	sysMsDelay(1) ;
	printf("I2C has been initialed...\n");	
	return ;
}


/*等待中断函数，即等待中断释放的信号量*/
int  bsp_i2c_Tx_int_delay  (UINT  delay)
{
	if( semTake(i2cTxSynBSem , delay) == ERROR)
	{
		if(g_i2cDebugInfo)  printf("bsp_i2c_Tx_int_delay : %d0 MS timeout \n" , delay) ;
		return ERROR ;
	}
	return OK ;	
}

int  bsp_i2c_Rx_int_delay  (UINT  delay)
{
	if( semTake(i2cRxSynBSem , delay) == ERROR)
	{
		if(g_i2cDebugInfo)  printf("bsp_i2c_Rx_int_delay : %d0 MS timeout \n" , delay) ;
		return ERROR ;
	}
	return OK ;	
}

/*中断处理函数，功能:释放信号量，清中断*/
int i2c_interrupt_cnt = 0 ;
static void bsp_i2c_interrupt_handle()
{
	unsigned char ucIntVal = 0;
	i2c_interrupt_cnt++ ;
	ucIntVal = *MPC8260_I2CE(vxImmrGet());

	if(ucIntVal & 0x02)
	{
		ucIntVal |= 0x02;
		semGive(i2cTxSynBSem) ; 
	}


	if(ucIntVal & 0x01)
	{
		ucIntVal |= 0x01;
		semGive(i2cRxSynBSem) ; 
	}

	*MPC8260_I2CE(vxImmrGet()) = ucIntVal;	
}
	
/***************************************************************************
*
* mpc8260i2cWaitRx - perform I2C receive date wait
*
* This routine is used to perform an I2C receive date in bd
*
* RETURNS: OK, or ERROR if timed out waiting for an receive date .
*/

LOCAL STATUS mpc8260i2cWaitRx(void)
    {
	I2C_DEV* pI2c = &i2cDev ;
	unsigned short status ;


	//	bsp_cycle_delay (1);
	if( bsp_i2c_Rx_int_delay (2) == ERROR)
		return -1 ; /*timeout*/
		
	sysUsDelay(20);   /*注意这里要延迟至少20us以上，否则BD表不稳定*/
	status = pI2c->pRxBdBase->statusMode ;
	if (status & I2C_TRANS_RX_BD_OV)
	{
		if(g_i2cDebugInfo) printf("Rx error : status = 0x%x" , status) ;
		return -2 ;
	}
	else  if ((pI2c->pRxBdBase->statusMode & I2C_TRANS_RX_BD_EMPTY) == 0) 
	{
		return OK ;
	}

	else
	{
		if(g_i2cDebugInfo) printf("Rx Error interrupt no source(status = 0x%x) ---????\n" , status) ;
		return (-3);
	}
}

/***************************************************************************
*
* mpc8260i2cWaittx - perform I2C tx date wait
*
* This routine is used to perform an I2C tx date in bd
*
* RETURNS: OK, or ERROR if timed out waiting for an tx date .
*/

LOCAL STATUS mpc8260i2cWaitTx(void)
{
	I2C_DEV* pI2c = &i2cDev ;
	unsigned short status ;

	//bsp_cycle_delay (1);
	if( bsp_i2c_Tx_int_delay (2) == ERROR)
		return -1 ; /*timeout*/

    sysUsDelay(20);	  /*注意这里要延迟至少20us以上，否则BD表不稳定*/	
	status = pI2c->pTxBdBase->statusMode ;
	if ((status & I2C_TRANS_TX_BD_NAK)
		|| (status & I2C_TRANS_TX_BD_UN)
		|| (status& I2C_TRANS_TX_BD_CL))
	{
		if(g_i2cDebugInfo) printf("Tx error : status = 0x%x" , status) ;
		return -2 ;
	}
	else if ((status & I2C_TRANS_TX_BD_READY)== 0) 
	{
		return OK ;
	}
	else
	{
		if(g_i2cDebugInfo) printf("Tx Error interrupt no source(status = 0x%x) ---????\n" , status) ;
		return (-3);
	}

    }
/***************************************************************************
*
* mpc8260i2cDoOperation - i2c do operation
*
* The  purpose of this routine is to execute the operation as specified
* by the passed command packet.
*
* RETURNS: N/A
*/

LOCAL void  mpc8260i2cDoOperation
    (
    UINT32 		deviceAddress,	/* device I2C bus address */
    MPC8260_I2C_CMD_PCKT * pI2cCmdPacket	/* pointer to command packet */
    
    )
    {
	I2C_DEV* pI2c;
	int immrVal = vxImmrGet();


	if(pI2cCmdPacket->nBytes > (M8260_I2C_BD_BUF_LEN -1)) 
	{
		pI2cCmdPacket->status = MPC8260_I2C_ERR_CYCLE_TIMEOUT;
		return;
	}

	
	*MPC8260_I2MOD(immrVal) = M8260_MOD_I2C_EN;   
	pI2c = &i2cDev;
	
	/* Write the device address */
	*(char *)(I2C_TX_BUF_ADRS) = (deviceAddress&0xfe) | pI2cCmdPacket->command ; 
	
    /*the other to BD buffer*/ 
	memcpy((char *)(I2C_TX_BUF_ADRS + 1) , (char *)pI2cCmdPacket->memoryAddress , pI2cCmdPacket->nBytes);
	
	/* Set up transmit buffer descriptor. */
	pI2c->pTxBdBase->statusMode = ( I2C_TRANS_TX_BD_READY | I2C_TRANS_TX_BD_WRAP | I2C_TRANS_TX_BD_LAST | I2C_TRANS_TX_BD_INT);
	pI2c->pTxBdBase->dataLength = pI2cCmdPacket->nBytes + 1 ;
	pI2c->pTxBdBase->dataPointer = (u_char *)I2C_TX_BUF_ADRS;

	/* Set up receive buffer descriptor. */
	pI2c->pRxBdBase->statusMode = (I2C_TRANS_RX_BD_WRAP |I2C_TRANS_RX_BD_EMPTY | I2C_TRANS_RX_BD_LAST | I2C_TRANS_RX_BD_INT );
	pI2c->pRxBdBase->dataLength = 0;
	pI2c->pRxBdBase->dataPointer = (u_char *)I2C_RX_BUF_ADRS;
     
	/* Start transfer. */
	*MPC8260_I2COM(immrVal) = M8260_COM_I2C_S | M8260_COM_I2C_M;
   
	if (pI2cCmdPacket->command == MPC8260_I2C_READ) /* read operation  */
	{
		if ((mpc8260i2cWaitTx ()) != OK)
		{
			pI2cCmdPacket->status = MPC8260_I2C_ERR_CYCLE_TIMEOUT;
			if(g_i2cDebugInfo) printf("\nError : I2C master write (adrs = 0x%x) can not complete in time\n", deviceAddress) ;
			*MPC8260_I2MOD(immrVal) = M8260_MOD_I2C_DIS;
			return ;
		}
		
		if ((mpc8260i2cWaitRx ()) != OK)
		{
			pI2cCmdPacket->status = MPC8260_I2C_ERR_CYCLE_TIMEOUT;
			if(g_i2cDebugInfo) printf("\nError : I2C master read (adrs = 0x%x) did not receive ACK in time\n" , deviceAddress) ;
			*MPC8260_I2MOD(immrVal) = M8260_MOD_I2C_DIS;
			return ;
		}

	  	/* Read the data from the IIC interface */
		if(pI2c->pRxBdBase->dataLength <= M8260_I2C_BD_BUF_LEN)
		{
			if(pI2c->pRxBdBase->dataLength == pI2cCmdPacket->nBytes)
				memcpy((char *)pI2cCmdPacket->memoryAddress , (char *)I2C_RX_BUF_ADRS  , pI2c->pRxBdBase->dataLength);
			else if(pI2c->pRxBdBase->dataLength == (pI2cCmdPacket->nBytes +1)) /*looks like a chip  bug ,....need confirm*/
				memcpy((char *)pI2cCmdPacket->memoryAddress , (char *)(I2C_RX_BUF_ADRS+1)  , pI2c->pRxBdBase->dataLength -1);	
			else
				;  /*do nothing*/
		}
	}

	if (pI2cCmdPacket->command == MPC8260_I2C_WRITE) /* read operation  */
	{
		if ((mpc8260i2cWaitTx ()) != OK)
		{
			pI2cCmdPacket->status = MPC8260_I2C_ERR_CYCLE_TIMEOUT;
			if(g_i2cDebugInfo) printf("\nError : I2C master write (adrs = 0x%x) can not complete in time\n", deviceAddress) ;
			*MPC8260_I2MOD(immrVal) = M8260_MOD_I2C_DIS;
			return ;
		}
	}	
	*MPC8260_I2MOD(immrVal) = M8260_MOD_I2C_DIS;
	sysMsDelay(1) ; //读写完成后延迟一段时间，防止不同芯片读写之间的干扰
	return;

}


/***************************************************************************
*
* bsp_i2c_read -  read  from the slave device on IIC bus
*
* This routine  reads the specified number of bytes from the specified slave
* device with MPC8260 as the master  .
*
* RETURNS: OK, or Error on a bad request
*/

INT32 bsp_i2c_read
    (
    UINT32 	deviceAddress,	/* Device I2C  bus address */
    UINT32 	numBytes,	/* number of Bytes to read */
    char *	pBuf ,		/* pointer to buffer to receive data */
    UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
    UINT32   stop          /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    )

    {
	MPC8260_I2C_CMD_PCKT 	i2cCmdPacket;	/* command packet */

	/* Check for a bad request. */

	if (numBytes == 0 )
	{
		return (ERROR);
	}

	/* Build command packet. */

	i2cCmdPacket.command = MPC8260_I2C_READ;
	i2cCmdPacket.status = 0;
	i2cCmdPacket.memoryAddress = (UINT32)pBuf;
	i2cCmdPacket.nBytes = numBytes ;
	i2cCmdPacket.repeatStart = repeatStart;
	i2cCmdPacket.stop = stop;

	semTake (i2cMutexSem, WAIT_FOREVER);
	mpc8260i2cDoOperation(deviceAddress, (MPC8260_I2C_CMD_PCKT *)&i2cCmdPacket);
	semGive (i2cMutexSem);

	/* Return the appropriate status. */

	return (i2cCmdPacket.status);
    }


/***************************************************************************
*
* bsp_i2c_write - write to the slave device on IIC bus
*
* This routine is used to write the specified number of
* bytes to the specified slave device with  MPC 107 as a master
*
* RETURNS: OK, or ERROR on a bad request.
*/

INT32 bsp_i2c_write
    (
    UINT32 	deviceAddress,	/* Devices I2C bus address */
    UINT32 	numBytes,	/* number of bytes to write */
    char *	pBuf ,		/* pointer to buffer of send data */
	UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
	UINT32   stop            /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    )
    {
	MPC8260_I2C_CMD_PCKT 	i2cCmdPacket;	/* command packet */

	/* Check for a NOP request. */

	if (numBytes == 0)
	{
		return (ERROR);
	}

	    /* Build command packet. */

	i2cCmdPacket.command = MPC8260_I2C_WRITE;
	i2cCmdPacket.status = 0;
	i2cCmdPacket.memoryAddress = (UINT32)pBuf;
	i2cCmdPacket.nBytes = numBytes ;
	i2cCmdPacket.repeatStart = repeatStart;
	i2cCmdPacket.stop = stop;

	semTake (i2cMutexSem, WAIT_FOREVER);
	/* Take ownership, call driver, release ownership. */
	mpc8260i2cDoOperation(deviceAddress, (MPC8260_I2C_CMD_PCKT *)&i2cCmdPacket);
	semGive (i2cMutexSem);

	/* Return the appropriate status. */

	return (i2cCmdPacket.status);

    }

/***************************************************************************
*
* bsp_i2c_write_read -  write and read  from the slave device on IIC bus
*
* This routine  reads the specified number of bytes from the specified slave
* device with MPC8260 as the master  .
*
* RETURNS: OK, or Error on a bad request
*/

INT32 bsp_i2c_write_read
    (
    UINT32 	deviceAddress,	/* Device I2C  bus address */
    UINT32 	writeBytes,	/* number of Bytes to read */
    char *	pWriteBuf ,		/* pointer to buffer to receive data */
	UINT32  delay_ms , /*delay between write and read action*/
    UINT32 	readBytes,	/* number of Bytes to read */
    char *	pReadBuf 		/* pointer to buffer to receive data */
    )

    {
	MPC8260_I2C_CMD_PCKT 	i2cCmdPacket;	/* command packet */

	/* Check for a bad request. */
	semTake (i2cMutexSem, WAIT_FOREVER);
	
	if (writeBytes != 0 )
	{
		
		/* Build command packet. */
		
		i2cCmdPacket.command = MPC8260_I2C_WRITE;
		i2cCmdPacket.status = MPC8260_I2C_OK;
		i2cCmdPacket.memoryAddress = (UINT32)pWriteBuf;
		i2cCmdPacket.nBytes = writeBytes;
		i2cCmdPacket.repeatStart = 0;
		i2cCmdPacket.stop = 1;
		
		mpc8260i2cDoOperation(deviceAddress, (MPC8260_I2C_CMD_PCKT *)&i2cCmdPacket);
		
	}
	if(delay_ms)
		sysMsDelay(delay_ms);	
	/* Check for a bad request. */
	
	if (readBytes != 0 && i2cCmdPacket.status == MPC8260_I2C_OK)
	{
		
		/* Build command packet. */
		
		i2cCmdPacket.command = MPC8260_I2C_READ;
		i2cCmdPacket.status = MPC8260_I2C_OK;
		i2cCmdPacket.memoryAddress = (UINT32)pReadBuf;
		i2cCmdPacket.nBytes = readBytes;
		i2cCmdPacket.repeatStart = 0;
		i2cCmdPacket.stop = 1;
		
		mpc8260i2cDoOperation(deviceAddress, (MPC8260_I2C_CMD_PCKT *)&i2cCmdPacket);
	}
	
	/* Return the appropriate status. */
	semGive (i2cMutexSem);

	return (i2cCmdPacket.status);
    }

INT32 bsp_i2c_output
    (
    char 		*pBuf ,		/* pointer to buffer to receive data */
    UINT32 	numBytes	/* number of Bytes to read */
    )
	{
	MPC8260_I2C_CMD_PCKT 	i2cCmdPacket;	/* command packet */

	/* Check for a bad request. */

	if (numBytes == 0 )
		{
		return (ERROR);
		}

	/* Build command packet. */

	i2cCmdPacket.command = (pBuf[0] & MPC8260_I2C_READ);
	i2cCmdPacket.status = 0;
	i2cCmdPacket.memoryAddress = (UINT32)(&(pBuf[1]));
	i2cCmdPacket.nBytes = numBytes - 1;
	i2cCmdPacket.repeatStart = FALSE;
	i2cCmdPacket.stop = TRUE;

	semTake (i2cMutexSem, WAIT_FOREVER);
	mpc8260i2cDoOperation(pBuf[0], (MPC8260_I2C_CMD_PCKT *)&i2cCmdPacket);
	semGive (i2cMutexSem);

	/* Return the appropriate status. */

	return (i2cCmdPacket.status);
	}
/*BRGCLK now in 81bctl is 200M*2/16 = 25M ---SCCR[DFBGR=0b01]*/	
/*I2CMOD[PDIV] = 10   : BRGCLK/8 =25M/8 = 3125K*/
/*2*([DIV0-DIV7] + 3 + (2 * I2MOD[FLT](=1))) , so X = 3125/(num_of_1k*2)- 5*/
/*
	设置I2C 时钟频率:	最大值195k, 最小6k
*/
#define  MAX_CLK_FEQ  195
#define  MIN_CLK_FEQ  6

UINT32 bsp_i2c_scl_freq
    (
	UINT32   num_of_1k            
    )
{
	unsigned int immrVal = vxImmrGet() , i2cBrg; 

	if(num_of_1k > MAX_CLK_FEQ)
		num_of_1k = MAX_CLK_FEQ ;
	if(num_of_1k < MIN_CLK_FEQ)
		num_of_1k = MIN_CLK_FEQ ;
		
	i2cBrg = (sysBaudClkFreq()/8/1000)/(num_of_1k*2) -5 ;
	/*i2cBrg = (sysBaudClkFreq()/8/1000 -10*num_of_1k)/(2*num_of_1k) ;*/
	
	*MPC8260_I2BRG(immrVal) = i2cBrg;
	return  i2cBrg ;	
}
