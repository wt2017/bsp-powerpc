/* MotFcc2End.h - Second Generation Motorola FCC Ethernet network interface.*/

/* Copyright 1990-1998 Wind River Systems, Inc. */
/*
modification history
--------------------
01a,16jan03,gjc  SPR#85164 :Second Generation Motorola FCC END header.
*/

#ifndef __INCmotFcc2Endh
#define __INCmotFcc2Endh

/* includes */

#ifdef __cplusplus
extern "C" {
#endif

#include "etherLib.h"
#include "miiLib.h"

/* defines */

#ifndef M8260ABBREVIATIONS
#define M8260ABBREVIATIONS

#ifdef  _ASMLANGUAGE
#define CAST(x)
#else /* _ASMLANGUAGE */
typedef volatile UCHAR VCHAR;   /* shorthand for volatile UCHAR */
typedef volatile INT32 VINT32; /* volatile unsigned word */
typedef volatile INT16 VINT16; /* volatile unsigned halfword */
typedef volatile INT8 VINT8;   /* volatile unsigned byte */
typedef volatile UINT32 VUINT32; /* volatile unsigned word */
typedef volatile UINT16 VUINT16; /* volatile unsigned halfword */
typedef volatile UINT8 VUINT8;   /* volatile unsigned byte */
#define CAST(x) (x)
#endif  /* _ASMLANGUAGE */

#endif /* M8260ABBREVIATIONS */

/*
 * redefine the macro below in the bsp if you need to access the device
 * registers/descriptors in a more suitable way.
 */
#ifndef MOT_FCC_LONG_WR
#define MOT_FCC_LONG_WR(addr, value)                                        \
    (* ((UINT32 *) addr) = ((UINT32) (value)))
#endif /* MOT_FCC_LONG_WR */

#ifndef MOT_FCC_WORD_WR
#define MOT_FCC_WORD_WR(addr, value)                                        \
    (* (addr) = ((UINT16) (value)))
#endif /* MOT_FCC_WORD_WR */

#ifndef MOT_FCC_BYTE_WR
#define MOT_FCC_BYTE_WR(addr, value)                                        \
    (* (addr) = ((UINT8) (value)))
#endif /* MOT_FCC_BYTE_WR */

#ifndef MOT_FCC_LONG_RD
#define MOT_FCC_LONG_RD(addr, value)                                        \
    ((value) = (* (UINT32 *) (addr)))
#endif /* MOT_FCC_LONG_RD */

#ifndef MOT_FCC_WORD_RD
#define MOT_FCC_WORD_RD(addr, value)                                        \
    ((value) = (* (UINT16 *) (addr)))
#endif /* MOT_FCC_WORD_RD */

#ifndef MOT_FCC_BYTE_RD
#define MOT_FCC_BYTE_RD(addr, value)                                        \
    ((value) = (* (UINT8 *) (addr)))
#endif /* MOT_FCC_BYTE_RD */

/*
 * Default macro definitions for BSP interface.
 * These macros can be redefined in a wrapper file, to generate
 * a new module with an optimized interface.
 */

#define MOT_FCC_INUM(pDrvCtrl)                      \
    ((int) INUM_FCC1 + ((pDrvCtrl)->fccNum - 1))

#define MOT_FCC_IVEC(pDrvCtrl)                      \
    INUM_TO_IVEC (MOT_FCC_INUM (pDrvCtrl))

#ifndef SYS_FCC_INT_CONNECT
#define SYS_FCC_INT_CONNECT(pDrvCtrl, pFunc, arg, ret)                  \
{                                                                       \
IMPORT STATUS intConnect (VOIDFUNCPTR *, VOIDFUNCPTR, int);     \
ret = OK;                                                               \
                                                                        \
    pDrvCtrl->intrConnect = TRUE;                                       \
    ret = (intConnect) ((VOIDFUNCPTR*) MOT_FCC_IVEC (pDrvCtrl),     \
            (pFunc), (int) (arg));                          \
}
#endif /* SYS_FCC_INT_CONNECT */

#ifndef SYS_FCC_INT_DISCONNECT
#define SYS_FCC_INT_DISCONNECT(pDrvCtrl, pFunc, arg, ret)                   \
{                                                                           \
ret = OK;                                                                   \
                                                                            \
if (MOT_FCC_IVEC (pDrvCtrl) && (pDrvCtrl->intDiscFunc != NULL))               \
    {                                                                       \
    pDrvCtrl->intrConnect = FALSE;                                          \
    ret = (*pDrvCtrl->intDiscFunc) ((VOIDFUNCPTR*) MOT_FCC_IVEC (pDrvCtrl),   \
                (pFunc));                                       \
    }                                                                       \
}
#endif /* SYS_FCC_INT_DISCONNECT */

#ifndef SYS_FCC_INT_ENABLE
#define SYS_FCC_INT_ENABLE(pDrvCtrl, ret)               \
{                                                                       \
IMPORT int intEnable (int);                     \
ret = OK;                                                               \
                                                                        \
if (MOT_FCC_INUM (pDrvCtrl))                                            \
    ret = intEnable ((int) (MOT_FCC_INUM (pDrvCtrl)));                  \
}
#endif /* SYS_FCC_INT_ENABLE */

#ifndef SYS_FCC_INT_DISABLE
#define SYS_FCC_INT_DISABLE(pDrvCtrl, ret)              \
{                                                                       \
IMPORT int intDisable (int);                        \
ret = OK;                                                               \
                                                                        \
if (MOT_FCC_INUM (pDrvCtrl))                                            \
    ret = intDisable ((int) (MOT_FCC_INUM (pDrvCtrl)));                  \
}
#endif /* SYS_FCC_INT_DISABLE */

#ifndef SYS_FCC_INT_ACK
#define SYS_FCC_INT_ACK(pDrvCtrl, ret)                  \
{                                                                       \
ret = OK;                                                               \
}
#endif /* SYS_FCC_INT_ACK */

#define SYS_FCC_ENET_ADDR_GET(address)                                      \
if (sysFccEnetAddrGet != NULL)                                          \
    if (sysFccEnetAddrGet (pDrvCtrl->unit, (address)) == ERROR)     \
    {                               \
    errnoSet (S_iosLib_INVALID_ETHERNET_ADDRESS);           \
    return (NULL);                          \
    }

#define SYS_FCC_ENET_ENABLE                                             \
if (sysFccEnetEnable != NULL)                                           \
    if (sysFccEnetEnable (pDrvCtrl->immrVal, pDrvCtrl->fccNum)      \
    == ERROR)                                   \
        return (ERROR);

#define SYS_FCC_ENET_DISABLE                                            \
if (sysFccEnetDisable != NULL)                                          \
    if (sysFccEnetDisable (pDrvCtrl->immrVal, pDrvCtrl->fccNum)     \
    == ERROR)                                   \
        return (ERROR);

#define SYS_FCC_MII_BIT_RD(bit)                     \
if (sysFccMiiBitRd != NULL)                     \
    if (sysFccMiiBitRd (pDrvCtrl->immrVal, pDrvCtrl->fccNum, (bit)) \
    == ERROR)                                   \
        return (ERROR);

#define SYS_FCC_MII_BIT_WR(bit)                     \
if (sysFccMiiBitWr != NULL)                     \
    if (sysFccMiiBitWr (pDrvCtrl->immrVal, pDrvCtrl->fccNum, (bit)) \
    == ERROR)                                   \
        return (ERROR);

#define MOT_FCC_MII_WR(data, len)                   \
    {                                   \
    UINT8 i = len;                          \
                                    \
    while (i--)                             \
    SYS_FCC_MII_BIT_WR (((data) >> i) & 0x1);           \
    }

#define MOT_FCC_MII_RD(data, len)                   \
    {                                   \
    UINT8 i = len;                          \
    UINT8 bitVal = 0;                           \
                                    \
    while (i--)                             \
    {                               \
    (data) <<= 1;                           \
    SYS_FCC_MII_BIT_RD (&bitVal);                   \
    (data) |= bitVal & 0x1;                     \
    }                               \
    }


//#define MOT_FCC_LOOP_NS 10

#define MOT_FCC_DEV_NAME        "motfcc"
#define MOT_FCC_DEV_NAME_LEN    7
#define MOT_FCC_TBD_DEF_NUM     128      /* default number of TBDs */
#define MOT_FCC_RBD_DEF_NUM     128      /* default number of RBDs */
#define MOT_FCC_TX_CL_NUM       64      /* number of tx clusters */
#define MOT_FCC_BD_LOAN_NUM     128      /* loaned BDs */
#define MOT_FCC_TX_POLL_NUM      1          /* one TBD for poll operation */
#define MOT_FCC_TBD_MAX         128         /* max number of TBDs */
#define MOT_FCC_RBD_MAX         128         /* max number of RBDs */
#define MOT_FCC_WAIT_MAX        0xf0000000  /* max delay after sending */
#define MOT_FCC_MIN_TX_PKT_SZ   100     /* the smallest packet we send */

#define MOT_FCC_ADDR_LEN        6               /* ethernet address length */

#define MOT_FCC_RES_VAL         0x0000          /* reserved field value */
#define MOT_FCC_FCR_DEF_VAL     M8260_FCR_BO_BE /* def value for the FCR */
#define MOT_FCC_C_MASK_VAL      0xdebb20e3      /* recommended value */
#define MOT_FCC_C_PRES_VAL      0xffffffff      /* recommended value */
#define MOT_FCC_CLEAR_VAL       0x00000000      /* clear this field */
#define MOT_FCC_RET_LIM_VAL     0xf         /* recommended value */
#define MOT_FCC_MINFLR_VAL      0x40        /* recommended value */
#define MOT_FCC_PAD_VAL         MOT_FCC_TIPTR_VAL   /* padding value */
#define MOT_FCC_MAXD_VAL        1524        /* recommended value used to be 1520*/
#define MOT_FCC_DSR_VAL         0xD555          /* recommended value */
#define MOT_FCC_FCCE_VAL        0xffff          /* clear all events */

/* rx/tx buffer descriptors definitions */

#define MOT_FCC_RBD_SZ          8       /* RBD size in byte */
#define MOT_FCC_TBD_SZ          8       /* TBD size in byte */
#define MOT_FCC_TBD_MIN        16       /* min number of TBDs */
#define MOT_FCC_RBD_MIN        16       /* min number of RBDs */
#define CL_OVERHEAD             4       /* prepended cluster overhead */
#define CL_ALIGNMENT            0x20       /* cluster required alignment */
#define MBLK_ALIGNMENT          4       /* mBlks required alignment */
#define MOT_FCC_BD_ALIGN        0x8     /* required alignment for BDs */
#define MOT_FCC_BUF_ALIGN       0x20    /* required alignment for data buffer */
#define MOT_FCC_BD_STAT_OFF     0       /* offset of the status word */
#define MOT_FCC_BD_LEN_OFF      2       /* offset of the data length =
word */
#define MOT_FCC_BD_ADDR_OFF     4       /* offset of the data pointer =
word */

#define MOT_FCC_RBD_ERR         (M8260_FETH_RBD_LG  |                      \
                                 M8260_FETH_RBD_NO  |                      \
                                 M8260_FETH_RBD_SH  |                      \
                                 M8260_FETH_RBD_CR  |                      \
                                 M8260_FETH_RBD_CL  |                      \
                                 M8260_FETH_RBD_OV)

/* allowed PHY's speeds */

#define MOT_FCC_100MBS      100000000       /* bits per sec */
#define MOT_FCC_10MBS       10000000        /* bits per sec */

/*
 * user flags: full duplex mode, loopback mode, serial interface etc.
 * the user may configure some of this options according to his needs
 * by setting the related bits in the <userFlags> field of the load string.
 */

#define MOT_FCC_USR_PHY_NO_AN   0x00000001  /* do not auto-negotiate */
#define MOT_FCC_USR_PHY_TBL     0x00000002  /* use negotiation table */
#define MOT_FCC_USR_PHY_NO_FD   0x00000004  /* do not use full duplex */
#define MOT_FCC_USR_PHY_NO_100  0x00000008  /* do not use 100Mbit speed */
#define MOT_FCC_USR_PHY_NO_HD   0x00000010  /* do not use half duplex */
#define MOT_FCC_USR_PHY_NO_10   0x00000020  /* do not use 10Mbit speed */
#define MOT_FCC_USR_PHY_MON     0x00000040  /* use PHY Monitor */
#define MOT_FCC_USR_PHY_ISO     0x00000100  /* isolate a PHY */
#define MOT_FCC_USR_RMON        0x00000200  /* enable RMON support */
#define MOT_FCC_USR_LOOP        0x00000400  /* external loopback mode */
                                            /* only use it for testing */
#define MOT_FCC_USR_HBC         0x00000800  /* perform heartbeat control */
#define MOT_FCC_USR_BUF_LBUS    0x00001000  /* buffers are on the local */
                                            /* bus */
#define MOT_FCC_USR_BD_LBUS     0x00002000  /* BDs are on the local bus */
#define MOT_FCC_USR_NO_ZCOPY    0x00004000  /* inhibit zcopy mode */
          /* required if bufs are on local bus, optional otherwise */
#define MOT_FCC_USR_DPRAM_ALOC  0x00008000  /* Using DPAM auto allocation */

#define MOT_FCC_TBD_OK      0x1     /* the TBD is a good one */
#define MOT_FCC_TBD_BUSY    0x2     /* the TBD has not been used */
#define MOT_FCC_TBD_ERROR   0x4     /* the TBD is errored */

/* Driver State Variable */
#define MOT_FCC_STATE_INIT          0x00
#define MOT_FCC_STATE_NOT_LOADED    0x00
#define MOT_FCC_STATE_LOADED        0x01
#define MOT_FCC_STATE_NOT_RUNNING   0x00
#define MOT_FCC_STATE_RUNNING       0x02

/* frame descriptors definitions */

typedef char *          MOT_FCC_BD_ID;
typedef MOT_FCC_BD_ID       MOT_FCC_TBD_ID;
typedef MOT_FCC_BD_ID       MOT_FCC_RBD_ID;

/*
 * this table may be customized by the user in configNet.h
 */

IMPORT INT16 motFccPhyAnOrderTbl [];

typedef struct {
    UINT32              numInts;
    UINT32              numZcopySends;
    UINT32              numNonZcopySends;


    UINT32              numTXBInts;


    UINT32              numRXFInts;
    UINT32              numRXBInts;
    UINT32              numGRAInts;
    UINT32              numRXCInts;
    UINT32              numTXCInts;
    UINT32              numTXEInts;

    UINT32              numRXFHandlerEntries;
    UINT32              numRXFHandlerErrQuits;
    UINT32              numRXFHandlerFramesProcessed;
    UINT32              numRXFHandlerFramesRejected;
    UINT32              numRXFHandlerNetBufAllocErrors;
    UINT32              numRXFHandlerNetCblkAllocErrors;
    UINT32              numRXFHandlerNetMblkAllocErrors;
    UINT32              numRXFHandlerFramesCollisions;
    UINT32              numRXFHandlerFramesCrcErrors;
    UINT32              numRXFHandlerFramesLong;
    UINT32              numRXFExceedBurstLimit;
    UINT32              numStallsEntered;
    UINT32              numStallsCleared;
    UINT32              numLSCHandlerEntries;
    UINT32              numLSCHandlerExits;

    UINT32              motFccTxErr;
    UINT32              motFccHbFailErr;
    UINT32              motFccTxLcErr;
    UINT32              motFccTxUrErr;
    UINT32              motFccTxCslErr;
    UINT32              motFccTxRlErr;
    UINT32              motFccTxDefErr;

    UINT32              motFccRxBsyErr;
    UINT32              motFccRxLgErr;
    UINT32              motFccRxNoErr;
    UINT32              motFccRxCrcErr;
    UINT32              motFccRxOvErr;
    UINT32              motFccRxShErr;
    UINT32              motFccRxLcErr;
    UINT32              motFccRxMemErr;
}FCC_DRIVER_STATS;


/*---------------------------------------------------------------------*/
/*       F C C   E T H E R N E T   P A R A M E T E R S                 */
/*---------------------------------------------------------------------*/
typedef volatile unsigned long VULONG;
typedef volatile unsigned short VUSHORT;
typedef volatile unsigned char VUCHAR;

#pragma pack(1)

typedef struct {
    VULONG    stat_bus;       /* Internal use buffer. */
    VULONG    cam_ptr;        /* CAM address. */
    VULONG    c_mask;         /* CRC constant mask*/
    VULONG    c_pres;       /* CRC preset */
    VULONG    crcec;          /* CRC error counter */
    VULONG    alec;           /* alignment error counter */
    VULONG    disfc;          /* discarded frame counter */
    VUSHORT   ret_lim;        /* Retry limit threshold. */
    VUSHORT   ret_cnt;        /* Retry limit counter. */
    VUSHORT   p_per;          /* persistence */
    VUSHORT   boff_cnt;       /* back-off counter */
    VULONG    gaddr_h;        /* group address filter, high */
    VULONG    gaddr_l;        /* group address filter, low */
    VUSHORT   tfcstat;        /* out of sequece Tx BD staus. */
    VUSHORT   tfclen;         /* out of sequece Tx BD length. */
    VULONG    tfcptr;         /* out of sequece Tx BD data pointer. */
    VUSHORT   maxflr;         /* maximum frame length reg */
    VUSHORT   paddr_h;        /* physical address (MSB) */
    VUSHORT   paddr_m;        /* physical address */
    VUSHORT   paddr_l;        /* physical address (LSB) */
    VUSHORT   ibd_cnt;        /* internal BD counter. */
    VUSHORT   ibd_start;      /* internal BD start pointer. */
    VUSHORT   ibd_end;        /* internal BD end pointer. */
    VUSHORT   tx_len;         /* tx frame length counter */
    VUCHAR    ibd_base[0x20]; /* internal micro code usage. */
    VULONG    iaddr_h;        /* individual address filter, high */
    VULONG    iaddr_l;        /* individual address filter, low */
    VUSHORT   minflr;         /* minimum frame length reg */
    VUSHORT   taddr_h;        /* temp address (MSB) */
    VUSHORT   taddr_m;        /* temp address */
    VUSHORT   taddr_l;        /* temp address (LSB) */
    VUSHORT   pad_ptr;        /* pad_ptr. */
    VUSHORT   cf_type;        /* flow control frame type coding. */
    VUSHORT   cf_range;       /* flow control frame range. */
    VUSHORT   max_b;          /* max buffer descriptor byte count. */
    VUSHORT   maxd1;          /* max DMA1 length register. */
    VUSHORT   maxd2;          /* max DMA2 length register. */
    VUSHORT   maxd;           /* Rx max DMA. */
    VUSHORT   dma_cnt;        /* Rx DMA counter. */


    /* counter: */
    VULONG    octc;           /* received octets counter. */
    VULONG    colc;           /* estimated number of collisions */
    VULONG    broc;           /* received good packets of broadcast address */
    VULONG    mulc;           /* received good packets of multicast address */
    VULONG    uspc;           /* received packets shorter then 64 octets. */
    VULONG    frgc;           /* as uspc + bad packets */
    VULONG    ospc;           /* received packets longer then 1518 =octets. */
    VULONG    jbrc;           /* as ospc + bad packets  */
    VULONG    p64c;           /* received packets of 64 octets.. */
    VULONG    p65c;           /* received packets of 65-128 octets.. */
    VULONG    p128c;          /* received packets of 128-255 octets.. */
    VULONG    p256c;          /* received packets of 256-511 octets.. */
    VULONG    p512c;          /* received packets of 512-1023 octets.. */
    VULONG    p1024c;          /* received packets of 1024-1518 octets.. */
    VULONG    cam_buf;        /* cam respond internal buffer. */
    VUSHORT   rfthr;          /* received frames threshold */
    VUSHORT   rfcnt;          /* received frames count */
} FCC_ETH_PARAM_T;
#pragma pack()


/*---------------------------------------------------------------------*/
/*               F C C     P A R A M E T E R S                         */
/*---------------------------------------------------------------------*/


#pragma pack(1)
typedef struct {
    VUSHORT   riptr;      /* Rx internal temporary data pointer. */
    VUSHORT   tiptr;      /* Tx internal temporary data pointer. */
    VUSHORT   RESERVED0;  /* Reserved */
    VUSHORT   mrblr;      /* Rx buffer length */
    VULONG    rstate;     /* Rx internal state */
    VULONG    rbase;      /* RX BD base address */
    VUSHORT   rbdstat;    /* Rx BD status and control */
    VUSHORT   rbdlen;     /* Rx BD data length */
    VULONG    rdptr;      /* rx BD data pointer */
    VULONG    tstate;     /* Tx internal state */
    VULONG    tbase;      /* TX BD base address */
    VUSHORT   tbdstat;    /* Tx BD status and control */
    VUSHORT   tbdlen;     /* Tx BD data length */
    VULONG    tdptr;      /* Tx  data pointer */
    VULONG    rbptr;      /* rx BD pointer */
    VULONG    tbptr;      /* Tx BD pointer */
    VULONG    rcrc;       /* Temp receive CRC */
    VULONG    tcrc;       /* Temp transmit CRC */
  VULONG foo;

/*-----------------------------------------------------------------*/
/*                   Protocol Specific Parameters                  */
/*-----------------------------------------------------------------*/
    union {

    /* Force this union to be 256 bytes - 0x34 bytes big.
        The 0x34 is the size of the protocol independent part
        of the structure.
    */
        UCHAR       pads[0xc4];
        FCC_ETH_PARAM_T     e;

    } prot;

} FCC_PARAM_T;
#pragma pack()


/*-----------------------------------------------------------------*/
/*                   FCC registers                                 */
/*-----------------------------------------------------------------*/
#pragma pack(1)
typedef struct {
    VULONG  fcc_gfmr;        /* General Mode Register */
    VULONG  fcc_psmr;        /* Protocol Specific Mode Reg */
    VUSHORT fcc_todr;        /* Transmit On Demand Reg */
    VUCHAR  reserved22[0x2];
    VUSHORT fcc_dsr;         /* Data Sync Register */
    VUCHAR  reserved23[0x2];
    VUSHORT fcc_fcce;        /* Event Register */
    VUSHORT unused1;         
    VUSHORT fcc_fccm;        /* Mask Register */
    VUSHORT unused2;         
    VUCHAR  fcc_fccs;        /* Status Register */
    VUCHAR  reserved24[0x3];
    VULONG  fcc_tirr;        /* Transmit Partial Rate Reg */
} FCC_REG_T;

#pragma pack()

typedef struct {
    volatile UINT16 bdStat;
    volatile UINT16 bdLen;
    volatile UINT32 bdAddr;
}FCC_BD;

/* 
 * This is the bsp boot information structure for the Mii/Phy, Dpram
 * and misc funtions. It contains bsp specific call backs for the driver.
 * A pointer to the stucture is passed in the load string.
 */

typedef struct 
    {
    FUNCPTR miiPhyInit;       /* bsp MiiPhy init function */
    FUNCPTR miiPhyInt;        /* Driver function for BSP to call */
    FUNCPTR miiPhyBitRead;    /* Bit Read funtion */
    FUNCPTR miiPhyBitWrite;   /* Bit Write function */
    FUNCPTR miiPhyDuplex;     /* duplex status call back */
    FUNCPTR miiPhySpeed;      /* speed status call back */
    FUNCPTR hbFail;           /* heart beat fail */
    FUNCPTR intDisc;          /* disconnect Function */
    FUNCPTR dpramFree;
    FUNCPTR dpramFccMalloc;
    FUNCPTR dpramFccFree;
    FUNCPTR reserved[4];      /* future use */
    } FCC_END_FUNCS;


/* The definition of the driver control structure */

typedef struct drv_ctrl
    {
    END_OBJ             endObj;         /* base class */
    int                 unit;           /* unit number */
#ifdef MOT_FCC_DBG
    FCC_DRIVER_STATS   * Stats;
#endif
    FCC_REG_T          * fccReg;
    FCC_ETH_PARAM_T    * fccEthPar;
    FCC_PARAM_T        * fccPar;

    UINT8       fccNum;         /* FCC being used */
    UINT32      immrVal;        /* internal RAM base address */
    UINT32      fifoTxBase;     /* address of Tx FIFO in internal RAM */
    UINT32      fifoRxBase;     /* address of Rx FIFO in internal RAM */
    char      * pBufBase;       /* FCC memory pool base */
    char      * pBufOrig;       /* FCC memory pool base */
    UINT32      bufSize;        /* FCC memory pool size */
    char      * pBdOrig;        /* FCC BDs base */
    char      * pBdBase;        /* FCC BDs base */
    UINT32      bdSize;         /* FCC BDs size */
    void      * riPtr;          /* save for free in unload */
    void      * tiPtr;          /* save for free in unload */
    void      * padPtr;         /* save for free in unload */
/* receive buffer descripter management */

    UINT16      rbdNum;         /* number of TX bd's */
    FCC_BD    * pRbdNext;       /* RBD next to rx */
    FCC_BD    * pRbdLast;       /* RBD last to replenish */
    UINT16      rbdCnt;         /* number of rbds full */
    FCC_BD    * pRbdBase;       /* RBD ring */
    char     ** rBufList;       /* allocated clusters */
    UINT16      rxUnStallThresh; /* rx reclaim threshold */
    UINT16      rxStallThresh;   /* rx low water threshold */

    volatile BOOL rxStall;       /* rx handler stalled - no Tbd */

/* transmit buffer descripter management */

    UINT16      tbdNum;         /* number of TX bds */
    UINT16      tbdCnt;         /* number of TX bds in use  */
    FCC_BD    * pTbdNext;       /* TBD index */
    FCC_BD    * pTbdLast;       /* TBD index */
    UINT16      tbdFree;
    FCC_BD    * pTbdBase;        /* TBD ring base */
    M_BLK    ** tBufList;        /* allocated clusters */
    UINT16      txUnStallThresh; /* tx reclaim threshold */
    UINT16      txStallThresh;   /* tx low water threshold */

    volatile BOOL txStall;       /* tx handler stalled - no Tbd */

    UINT32      mblkMult; 
    UINT32      clMult;

    ULONG       userFlags;      /* some user flags */
    BOOL        zeroBufFlag;
    INT8        flags;          /* driver state */
    UINT32      state;          /* driver state including load flag */
    BOOL        intrConnect;    /* interrupt has been connected */
    UINT32      intMask;        /* interrupt mask register */
    UINT32      fccIramAddr;    /* base addr of this fcc */
    UINT32      fccPramAddr;    /* parameter RAM addr of this fcc */
    UCHAR *     pTxPollBuf;     /* cluster pointer for poll mode */

    SEM_ID      graSem;         /* synch semaphore for graceful */
    /* transmit command */
    char *       pClBlkArea;     /* cluster block pointer */
    UINT32       clBlkSize;      /* clusters block memory size */
    char *       pMBlkArea;      /* mBlock area pointer */
    UINT32       mBlkSize;       /* mBlocks area memory size */
    CACHE_FUNCS  bufCacheFuncs;  /* cache descriptor */
    CL_POOL_ID   pClPoolId;      /* cluster pool identifier */
    PHY_INFO    *phyInfo;        /* info on a MII-compliant PHY */

    BOOL         lscHandling;

    /* function pointers to support debugging */
    FUNCPTR     netJobAdd;
    FUNCPTR     muxTxRestart; 
    FUNCPTR     muxError;     

    /* Bsp specific funtions and call backs passed in the load string */
    FCC_END_FUNCS * motFccFuncs;

    FUNCPTR   hbFailFunc;    
    FUNCPTR   intDiscFunc;     /* assign a proper disc routine */
    FUNCPTR   phyInitFunc;     /* BSP Phy Init */
    FUNCPTR   phyDuplexFunc;   /* duplex function */
    FUNCPTR   phySpeedFunc;    /* speed function */

    FUNCPTR   dpramFreeFunc;      
    FUNCPTR   dpramFccMallocFunc; 
    FUNCPTR   dpramFccFreeFunc;   

    BOOL      rxJobPending;
    BOOL      txJobPending;
    } DRV_CTRL;


/*
 * this cache functions descriptors is used to flush/invalidate
 * the FCC's data buffers. They are set to the system's cache
 * flush and invalidate routine. This will allow proper operation
 * of the driver if data cache are turned on.
 */

IMPORT STATUS   cacheArchInvalidate (CACHE_TYPE, void *, size_t);
IMPORT STATUS   cacheArchFlush (CACHE_TYPE, void *, size_t);

IMPORT STATUS   sysFccEnetAddrGet (int unit,UCHAR *address);
IMPORT STATUS   sysFccEnetEnable (UINT32 immrVal, UINT8 fccNum);
IMPORT STATUS   sysFccEnetDisable (UINT32 immrVal, UINT8 fccNum);
IMPORT STATUS   sysFccMiiBitWr (UINT32 immrVal, UINT8 fccNum,
                INT32 bit);
IMPORT STATUS   sysFccMiiBitRd (UINT32 immrVal, UINT8 fccNum,
                INT8 * bit);
IMPORT STATUS   miiPhyInit (PHY_INFO * phyInfo);


#ifdef __cplusplus
}
#endif

#endif /* __INCmotFcc2Endh */

