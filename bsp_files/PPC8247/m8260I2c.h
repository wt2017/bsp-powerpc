
#ifndef __INCm8260I2C
#define __INCm8260I2C


#ifdef __cplusplus
extern "C" {
#endif

//bus speed is 66M
#define MPC8260_DELTA(a,b)              (abs((int)a - (int)b))


#define	MPC8260_I2MOD(base)	((VINT8*) ((base) + 0x11860))
#define	MPC8260_I2ADD(base)	((VINT8*) ((base) + 0x11864))
#define	MPC8260_I2BRG(base)	((VINT8*) ((base) + 0x11868))
#define	MPC8260_I2COM(base)	((VINT8*) ((base) + 0x1186C))
#define	MPC8260_I2CE(base)	((VINT8*) ((base) + 0x11870))
#define	MPC8260_I2CM(base)	((VINT8*) ((base) + 0x11874))

/* mode define for I2C */
#define M8260_MOD_I2C_EN   0x1d
#define M8260_MOD_I2C_DIS  0x1c

#define M8260_RX_TX_INT_ENABLE  0x3 

/* command identifiers for I2C */

#define MPC8260_I2C_READOP	0       /* read operation */
#define MPC8260_I2C_WRITOP	1	/* write operation */

#define M8260_COM_I2C_S 0x80
#define M8260_COM_I2C_M 0x01

/* General definitions */

#define MPC8260_I2C_WRITE            0x0 	/* Write */
#define MPC8260_I2C_READ             0x1         /* Read  */
#define MPC8260_I2C_READ_OR_WRITE    0x2         /* Read OR  write */
#define MPC8260_I2C_READ_AND_WRITE   0x3         /* Read AND write */
#define MPC8260_I2C_READ_ANDOR_WRITE 0x4         /* Read AND OR write */


/* error codes */
#define MPC8260_I2C_OK           	0	/* start cycle */	
#define MPC8260_I2C_ERR_CYCLE_START	1	/* start cycle */
#define MPC8260_I2C_ERR_CYCLE_STOP	2	/* stop cycle */
#define MPC8260_I2C_ERR_CYCLE_READ	3	/* read cycle */
#define MPC8260_I2C_ERR_CYCLE_WRITE	4	/* write cycle */
#define MPC8260_I2C_ERR_CYCLE_ACKIN	5	/* acknowledge in cycle */
#define MPC8260_I2C_ERR_CYCLE_ACKOUT	6	/* acknowledge out cycle */
#define MPC8260_I2C_ERR_KNOWN_STATE	7	/* known state */
#define MPC8260_I2C_ERR_BUF_OVERFLOW 8	/* date buffer overflow */
#define MPC8260_I2C_ERR_CYCLE_TIMEOUT	9	/* cycle timed out */


/* Definitions for I2C. */
#define I2C_TRANS_RX_BD_LAST 0x0800 /* last in frame */
#define I2C_TRANS_RX_BD_WRAP 0x2000 /* wrap back to first BD */
#define I2C_TRANS_RX_BD_EMPTY 0x8000 /* buffer is empty */
#define I2C_TRANS_RX_BD_INT  0x1000 /* INTERRUPT */
#define I2C_TRANS_RX_BD_OV  0x0002 /* error OVERrun*/

#define I2C_TRANS_TX_BD_LAST  0x0800 /* last in frame */
#define I2C_TRANS_TX_BD_WRAP  0x2000 /* wrap back to first BD */
#define I2C_TRANS_TX_BD_READY 0x8000 /* ready for Tx */
#define I2C_TRANS_TX_BD_INT  0x1000 /* INTERRUPT */
#define I2C_TRANS_TX_BD_NAK  0x0004 /* error no ACK */
#define I2C_TRANS_TX_BD_UN  0x0002 /* error UNDER run */
#define I2C_TRANS_TX_BD_CL 0x0001 /* error collision */
#define I2C_INTERFACE_PINS    0x00030000


/* I2C UART Receive Buffer Descriptor definitions */

#define M8260_I2C_RX_BD_EMPTY	0x8000	/* buffer is empty */
#define M8260_I2C_RX_BD_WRAP	0x2000	/* last BD in chain */
#define M8260_I2C_RX_BD_LAST	0x0800	/* last in frame */

/* I2C UART Transmit Buffer Descriptor definitions */
#define M8260_I2C_TX_BD_READY	  0x8000	/* Transmit ready/busy bit */
#define M8260_I2C_TX_BD_WRAP	  0x2000	/* last BD in chain */
#define M8260_I2C_TX_BD_LAST	0x0800	   /* last in frame */


/* driver command packet */

typedef struct mpc8260I2cCmdPckt
    {
    INT32 	command;	/* command identifier */
    INT32 	status;		/* status (error code) */
    UINT32 	memoryAddress;  /* memory address */
    UINT32 	nBytes;	   	/* number of bytes to transfer */
	UINT32   repeatStart ;   /* repeatStart == 0 :i2c normal start , else repeat start*/
	UINT32   stop ;           /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    }MPC8260_I2C_CMD_PCKT;



/* function declarations */

IMPORT  void  bsp_i2c_init(void) ;
IMPORT INT32 	bsp_i2c_read  (UINT32 deviceAddress, UINT32 numBytes,
						char *pBuf ,
						UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
						UINT32   stop           /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
						);
IMPORT INT32 	bsp_i2c_write (UINT32 deviceAddress, UINT32 numBytes,
						char *pBuf ,                              
						UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
						UINT32   stop           /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
						);

#ifdef __cplusplus
}
#endif

#endif  //__INCm8260I2C

