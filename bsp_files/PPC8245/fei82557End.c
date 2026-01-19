/* fei82557End.c - END style Intel 82557 Ethernet network interface driver */

/* Copyright 1989-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,09dec00,kw  Added code to setup the ifSpecific MIB II object.
01o,18sep00,ss  fix for PR#1244 out of range enum for ifConnectorPresent
01n,18sep00,ss  fix for PR#1241 out of range enum for ifLinkUpDownTrapEnable
01m,07sep00,ba  fix for PR#1242 out of range enumeration for ifPromiscuousMode.
01l,12jun00,kw  If link is not found continue initing the device.
01k,11mar99,cn  added support for shared PCI interrupts (SPR# 24379).
		Also corrected a potential memory leak in xxxRecv.
01j,09mar99,cn  moved callout to adapter init routine before xxxMemInit(),
		and corrected usage of xxxSCBCommand() (SPR# 25242).
01i,22sep98,dat  removed lint warnings.
01h,21jul98,cn  added support for multi-device configuration. Also made
		chip-access macros redefineable and moved them to the header
		file.
01g,16apr98,cn  added globals feiEndIntConnect and feiEndIntDisconnect and 
		modified SYS_INT_CONNECT and SYS_INT_DISCONNECT accordingly.
		Changed state to scbStatus in fei82557Action().
01f,23mar98,cn  fixed compiling problem when building for big-endian machines
01e,11mar98,cn  checked-in 
01d,05mar98,cn  more code clean-up after code review, added support for
		MULTIALL
01c,23feb98,cn  code clean-up
01b,18feb98,cn  changed all the once called LINK macros to normal
01a,07nov97,cn  created from ../netif/if_fei.c, version 01k
*/

/*
DESCRIPTION
This module implements an Intel 82557 Ethernet network interface driver.
This is a fast Ethernet PCI bus controller, IEEE 802.3 10Base-T and 
100Base-T compatible. It also features a glueless 32-bit PCI bus master 
interface, fully compliant with PCI Spec version 2.1. An interface to 
MII compliant physical layer devices is built-in in the card. The 82557
Ethernet PCI bus controller also includes Flash support up to 1 MByte
and EEPROM support, altough these features are not dealt with in the 
driver.

The 82557 establishes a shared memory communication system with the CPU,
which is divided into three parts: the Control/Status Registers (CSR),
the Command Block List (CBL) and the Receive Frame Area (RFA). The CSR
is on chip and is either accessible with I/O or memory cycles, whereas the
other structures reside on the host.

The CSR is the main meance of communication between the device and the
host, meaning that the latter issues commands through these registers
while the chip posts status changes in it, occurred as a result of those
commands. Pointers to both the CBL and RFA are also stored in the CSR.

The CBL consists of a linked list of frame descriptors through which 
individual action commands can be performed. These may be transmit 
commands as well as non-transmit commands, e.g. Configure or Multicast
setup commands. While the CBL list may function in two different modes,
only the simplified memory mode is implemented in the driver.

The RFA is a linked list of receive frame descriptors. Only support for
the simplified memory mode is granted. In this model, the data buffer 
immediately follows the related frame descriptor.

The driver is designed to be moderately generic, operating unmodified
across the range of architectures and targets supported by VxWorks.  
To achieve this, this driver must be given several target-specific 
parameters, and some external support routines must be provided.  These 
parameters, and the mechanisms used to communicate them to the driver, 
are detailed below.

BOARD LAYOUT
This device is on-board.  No jumpering diagram is necessary.

EXTERNAL INTERFACE

The driver provides the standard external interface, fei82557EndLoad(), which
takes a string of colon separated parameters. The parameters should be
specified in hexadecimal, optionally preceeded by "0x" or a minus sign "-".

The parameter string is parsed using strtok_r() and each parameter is
converted from a string representation to binary by a call to
strtoul(parameter, NULL, 16).

The format of the parameter string is:

 "<memBase>:<memSize>:<nTfds>:<nRfds>:<flags>"

In addition, the two global variables 'feiEndIntConnect' and 
'feiEndIntDisconnect' specify respectively the interrupt connect routine 
and the interrupt disconnect routine to be used depending on the BSP. 
The former defaults to intConnect() and the user can override this to use 
any other interrupt connect routine (say pciIntConnect()) in sysHwInit() 
or any device specific initialization routine called in sysHwInit(). 
Likewise, the latter is set by default to NULL, but it may be overridden 
in the BSP in the same way.
 
TARGET-SPECIFIC PARAMETERS

.IP <memBase>
This parameter is passed to the driver via fei82557EndLoad().

The Intel 82557 device is a DMA-type device and typically shares
access to some region of memory with the CPU.  This driver is designed
for systems that directly share memory between the CPU and the 82557.

This parameter can be used to specify an explicit memory region for use
by the 82557.  This should be done on targets that restrict the 82557
to a particular memory region.  The constant `NONE' can be used to 
indicate that there are no memory limitations, in which case the driver 
will allocate cache safe memory for its use using cacheDmaAlloc().

.IP <memSize>
The memory size parameter specifies the size of the pre-allocated memory
region. If memory base is specified as NONE (-1), the driver ignores this
parameter. Otherwise, the driver checks the size of the provoded memory 
region is adequate with respect to the given number of Command Frame
Descriptor and Receive Frame Descriptor.

.IP <nTfds>
This parameter specifies the number of transmit descriptor/buffers to be
allocated. If this parameter is less than two, a default of 32 is used.

.IP <nRfds>
This parameter specifies the number of receive descriptor/buffers to be
allocated. If this parameter is less than two, a default of 32 is used.

.IP <flags>
User flags may control the run-time characteristics of the Ethernet
chip. Not implemented.
.LP
 
EXTERNAL SUPPORT REQUIREMENTS

This driver requires one external support function:
.CS
STATUS sys557Init (int unit, FEI_BOARD_INFO *pBoard)
.CE
This routine performs any target-specific initialization
required before the 82557 device is initialized by the driver.
The driver calls this routine every time it wants to [re]initialize
the device.  This routine returns OK, or ERROR if it fails.
.LP

SYSTEM RESOURCE USAGE
The driver uses cacheDmaMalloc() to allocate memory to share with the 82557.
The size of this area is affected by the configuration parameters specified
in the fei82557EndLoad() call. 

Either the shared memory region must be non-cacheable, or else
the hardware must implement bus snooping.  The driver cannot maintain
cache coherency for the device because fields within the command
structures are asynchronously modified by both the driver and the device,
and these fields may share the same cache line.

TUNING HINTS

The only adjustable parameters are the number of TFDs and RFDs that will be
created at run-time.  These parameters are given to the driver when 
fei82557EndLoad() is called.  There is one TFD and one RFD associated with 
each transmitted frame and each received frame respectively.  For 
memory-limited applications, decreasing the number of TFDs and RFDs may be 
desirable.  Increasing the number of TFDs will provide no performance 
benefit after a certain point. Increasing the number of RFDs will provide 
more buffering before packets are dropped.  This can be useful if there are 
tasks running at a higher priority than the net task.

SEE ALSO: ifLib,
.I "Intel 82557 User's Manual,"
.I "Intel 32-bit Local Area Network (LAN) Component User's Manual"
*/

#include "vxWorks.h"
#include "wdLib.h"
#include "iv.h"
#include "vme.h"
#include "net/mbuf.h"
#include "net/unixLib.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "errno.h"
#include "memLib.h"
#include "intLib.h"
#include "net/route.h"
#include "iosLib.h"
#include "errnoLib.h"
#include "vxLib.h"    /* from if_fei.c */
#include "private/funcBindP.h"

#include "cacheLib.h"
#include "logLib.h"
#include "netLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "tickLib.h"

#include "etherLib.h"
#include "net/systm.h"
#include "sys/times.h"
#include "net/if_subr.h"

#include "fei82557End.h"	
#include "drv/pci/pciIntLib.h"
#undef ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"
#include "end.h"
#include "semLib.h"

#define    END_MACROS
#include "endLib.h"
#include "lstLib.h"

/* defines */

/* Driver debug control */
#define DRV_DEBUG557

/* Driver debug control */
#ifdef DRV_DEBUG557
#define DRV_DEBUG_OFF		0x0000
#define DRV_DEBUG_RX		0x0001
#define DRV_DEBUG_TX		0x0002
#define DRV_DEBUG_POLL		(DRV_DEBUG_POLL_RX | DRV_DEBUG_POLL_TX)
#define DRV_DEBUG_POLL_RX    	0x0004
#define DRV_DEBUG_POLL_TX    	0x0008
#define DRV_DEBUG_LOAD		0x0010
#define DRV_DEBUG_IOCTL		0x0020
#define DRV_DEBUG_INT		0x0040
#define DRV_DEBUG_START    	0x0080
#define DRV_DEBUG_DUMP    	0x0100
#define DRV_DEBUG_ALL    	0xffff

int	fei82557Debug = DRV_DEBUG_OFF ;

#define DRV_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)			\
    {									\
    if (fei82557Debug & FLG)						\
	if (_func_logMsg != NULL)					\
	    _func_logMsg (X0, X1, X2, X3, X4, X5, X6);			\
}

#define DRV_PRINT(FLG, X)						\
    {									\
    if (fei82557Debug & FLG) printf X;					\
    }

#else /* DRV_DEBUG557 */

#define DRV_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)
#define DRV_PRINT(FLG, X)

#endif /* DRV_DEBUG557 */

/* general macros for reading/writing from/to specified locations */

#if (_BYTE_ORDER == _BIG_ENDIAN)
 
#define FEI_SWAP_LONG(x)        LONGSWAP(x)
#define FEI_SWAP_WORD(x)        (MSB(x) | LSB(x) << 8)
 
#else
 
#define FEI_SWAP_LONG(x)        (x)
#define FEI_SWAP_WORD(x)        (x)
 
#endif /* _BYTE_ORDER == _BIG_ENDIAN */

/* Cache and PCI-bus related macros */

#define FEI_VIRT_TO_SYS(virtAddr)					    \
	(FEI_LOCAL_TO_SYS (((UINT32) FEI_VIRT_TO_PHYS (virtAddr))))

#define FEI_SYS_TO_VIRT(sysAddr)					    \
	((FEI_PHYS_TO_VIRT ((UINT32) FEI_SYS_TO_LOCAL (sysAddr))))

#define FEI_PHYS_TO_VIRT(physAddr)					    \
	CACHE_DRV_PHYS_TO_VIRT (&pDrvCtrl->cacheFuncs, (char *)(physAddr))

#define FEI_VIRT_TO_PHYS(virtAddr)					    \
	CACHE_DRV_VIRT_TO_PHYS (&pDrvCtrl->cacheFuncs, (char *)(virtAddr))

#define FEI_LOCAL_TO_SYS(physAddr)					    \
	LOCAL_TO_SYS_ADDR (pDrvCtrl->unit, (physAddr))

#define FEI_SYS_TO_LOCAL(sysAddr)					    \
	SYS_TO_LOCAL_ADDR (pDrvCtrl->unit, (sysAddr))

#define FEI_CACHE_INVALIDATE(address, len)				    \
        CACHE_DRV_INVALIDATE (&pDrvCtrl->cacheFuncs, (address), (len))

/* driver flags */
#define FEI_OWN_MEM	0x01		/* internally provided memory */
#define FEI_INV_NCFD    0x02		/* invalid nCFDs provided */
#define FEI_INV_NRFD    0x04		/* invalid nRFDs provided */
#define FEI_POLLING	0x08		/* polling mode */
#define FEI_PROMISC	0x20    	/* promiscuous mode */
#define FEI_MCAST	0x40    	/* multicast addressing mode */
#define FEI_MCASTALL	0x80    	/* all multicast addressing mode */
#define FEI_MEMOWN	0x10    	/* device mem allocated by driver */
 
#define FEI_FLAG_CLEAR(clearBits)					\
    (pDrvCtrl->flags &= ~(clearBits))

#define FEI_FLAG_SET(setBits)						\
    (pDrvCtrl->flags |= (setBits))

#define FEI_FLAG_GET()							\
    (pDrvCtrl->flags)

#define FEI_FLAG_ISSET(setBits)						\
    (pDrvCtrl->flags & (setBits))

/* shortcuts */
#define END_FLAGS_ISSET(setBits)					\
    ((&pDrvCtrl->endObj)->flags & (setBits))

#define FEI_VECTOR(pDrvCtrl)						\
    ((pDrvCtrl)->board.vector)

#define FEI_INT_ENABLE(pDrvCtrl)					\
    ((int)(pDrvCtrl)->board.intEnable)

#define FEI_INT_DISABLE(pDrvCtrl)					\
    ((int)(pDrvCtrl)->board.intDisable)

#define FEI_INT_ACK(pDrvCtrl)						\
    ((int)(pDrvCtrl)->board.intAck)

#define CL_OVERHEAD	4		/* prepended cluster header */
#define CL_RFD_SIZE	(RFD_SIZE + CL_OVERHEAD)		

#define FEI_SAFE_MEM(pDrvCtrl)						\
    ((pDrvCtrl)->memSize)

#define FEI_RFD_MEM(pDrvCtrl)						\
    (CL_RFD_SIZE * (pDrvCtrl)->nRFDs)

#define FEI_CFD_MEM(pDrvCtrl)						\
    (CFD_SIZE * (pDrvCtrl)->nCFDs)

#define FEI_HADDR(pEnd)                                                 \
        ((pEnd)->mib2Tbl.ifPhysAddress.phyAddress)

#define FEI_HADDR_LEN(pEnd)                                             \
        ((pEnd)->mib2Tbl.ifPhysAddress.addrLength)

/* Control Status Register definitions, some of them came from if_fei.h */

#define CSR_STAT_OFFSET		SCB_STATUS	/* CSR status byte */
#define CSR_ACK_OFFSET		0x01		/* CSR acknowledge byte */
#define CSR_COMM_OFFSET		SCB_CMD		/* CSR command byte */
#define CSR_INT_OFFSET		0x03		/* CSR Interrupt byte */
#define CSR_GP_OFFSET		SCB_POINTER	/* CSR General Pointer */
#define CSR_PORT_OFFSET		SCB_PORT	/* CSR PORT Register */
#define CSR_FLASH_OFFSET	SCB_FLASH	/* CSR FLASH Register */
#define CSR_EEPROM_OFFSET	SCB_EEPROM	/* CSR EEPROM Register */
#define CSR_MDI_OFFSET		SCB_MDI		/* CSR MDI Register */
#define CSR_RXBC_OFFSET		SCB_EARLYRX	/* CSR early RCV Byte Count */

/* Control Status Registers read/write macros */

/* Control Status Registers write macros */

#define CSR_BYTE_WR(offset, value)					\
    FEI_BYTE_WR (((UINT32) (pDrvCtrl->pCSR) + (offset)), (value))

#define CSR_WORD_WR(offset, value)					\
    FEI_WORD_WR (((UINT32) (pDrvCtrl->pCSR) + (offset)), (value))

#define CSR_LONG_WR(offset, value)					\
    FEI_LONG_WR (((UINT32) (pDrvCtrl->pCSR) + (offset)), (value))

/* this is a special case, as the device will read it as an address */

#define CSR_GP_WR(value)						\
    {									\
    volatile UINT32 temp = FEI_VIRT_TO_SYS (value);			\
    									\
    CSR_LONG_WR (CSR_GP_OFFSET, (temp));				\
    }

/* Control Status Registers read macros */

#define CSR_BYTE_RD(offset, value)					\
    FEI_BYTE_RD ((UINT8 *) ((UINT32) (pDrvCtrl->pCSR) + (offset)),	\
	     (value))

#define CSR_WORD_RD(offset, value)					\
    FEI_WORD_RD ((UINT16 *) ((UINT32) (pDrvCtrl->pCSR) + (offset)),	\
	     (value))

#define CSR_LONG_RD(offset, value)					\
    FEI_LONG_RD ((UINT32 *) ((UINT32) (pDrvCtrl->pCSR) + (offset)),	\
	     (value))

/* this is a special case, as the CPU will read it as an address */

#define CSR_GP_RD(value)						\
    {									\
    volatile UINT32      tempVal;                                       \
									\
    CSR_LONG_RD (CSR_GP_OFFSET), (tempVal))				\
									\
    value = FEI_SYS_TO_VIRT (((UINT32) (tempVal)));			\
    }

/* FD rings available */
 
#define CFD_FREE                0x01            /* CFD free ring */
#define CFD_USED                0x02            /* CFD used ring */
#define RFD_FREE                0x04            /* RFD free ring */
 
#define CFD_COMM_WORD           0x01            /* CFD command word */
#define CFD_STAT_WORD           0x02            /* CFD status word */
#define RFD_COMM_WORD           0x04            /* RFD command word */
#define RFD_STAT_WORD           0x08            /* RFD status word */
 
#define CFD_ACTION              0x01            /* generic action command */
#define CFD_TX                  0x02            /* transmit command */

/* frame descriptors macros: these are generic among RFDs and CFDs */

#define FD_FLAG_ISSET(fdStat, bits)                                     \
    (((UINT16) (fdStat)) & ((UINT16) (bits)))

/* get the pointer to the appropriate frame descriptor ring */

#define FREE_CFD_GET(pCurrFD)						\
    ((pCurrFD) = pDrvCtrl->pFreeCFD)

#define USED_CFD_GET(pCurrFD)						\
    ((pCurrFD) = pDrvCtrl->pUsedCFD)

#define RFD_GET(pCurrFD)						\
    ((pCurrFD) = pDrvCtrl->pRFD)

/* command frame descriptors write macros */

#define CFD_BYTE_WR(base, offset, value)				\
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define CFD_WORD_WR(base, offset, value)				\
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define CFD_LONG_WR(base, offset, value)				\
    FEI_LONG_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

/* this is a special case, as the device will read as an address */

#define CFD_NEXT_WR(base, value)					\
    {									\
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS (value);		\
    									\
    CFD_LONG_WR ((UINT32) (base), CFD_NEXT_OFFSET, (temp));		\
    }

/* this is a special case, as the device will read as an address */

#define CFD_TBD_WR(base, value)						\
    {									\
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS (value);		\
    									\
    CFD_LONG_WR ((UINT32) (base), CFD_TBD_OFFSET, (temp));		\
    }

/* receive frame descriptors write macros */

#define RFD_BYTE_WR(base, offset, value)				\
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define RFD_WORD_WR(base, offset, value)				\
    FEI_WORD_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

#define RFD_LONG_WR(base, offset, value)				\
    FEI_LONG_WR ((UINT32 *) ((UINT32) (base) + (offset)), 		\
	     (value))

/* this is a special case, as the device will read as an address */

#define RFD_NEXT_WR(base, value)					\
    {									\
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS ((UINT32) (value));	\
    									\
    RFD_LONG_WR ((UINT32) (base), RFD_NEXT_OFFSET, (temp));		\
    }

/* this is a special case, as the device will read as an address */

#define RFD_RBD_WR(base, value)						\
    {									\
    volatile UINT32 temp = (UINT32) FEI_VIRT_TO_SYS ((UINT32) (value));	\
    									\
    RFD_LONG_WR ((UINT32) (base), RFD_RBD_OFFSET, (temp));		\
    }

/* command frame descriptors read macros */



#define CFD_BYTE_RD(base, offset, value)				\
    FEI_BYTE_RD ((UINT8 *) ((UINT32) (base) + (offset)), (value))

#define CFD_WORD_RD(base, offset, value)				\
    FEI_WORD_RD ((UINT16 *) ((UINT32) (base) + (offset)), (value))


#define CFD_LONG_RD(base, offset, value)				\
    FEI_LONG_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

/* this is a special case, as the device will read as an address */

#define CFD_NEXT_RD(base, value)					\
    {									\
    volatile UINT32      tempVal;                                       \
									\
    CFD_LONG_RD ((UINT32) (base), CFD_NEXT_OFFSET, (tempVal));		\
									\
    value = (UINT32) FEI_SYS_TO_VIRT (((UINT32) (tempVal)));		\
    }

/* this is a special case, as the device will read as an address */

#define CFD_TBD_RD(base, value)						\
    {									\
    volatile UINT32      tempVal;                                       \
									\
    CFD_LONG_RD ((UINT32) (base), CFD_TBD_OFFSET, (tempVal));		\
									\
    value = (UINT32) FEI_SYS_TO_VIRT (((UINT32) (tempVal)));		\
    }

/* receive frame descriptors read macros */

#define RFD_BYTE_RD(base, offset, value)				\
    FEI_BYTE_RD ((UINT8 *) ((UINT32) (base) + (offset)), (value))

#define RFD_WORD_RD(base, offset, value)				\
    FEI_WORD_RD ((UINT16 *) ((UINT32) (base) + (offset)), (value))

#define RFD_LONG_RD(base, offset, value)				\
    FEI_LONG_RD ((UINT32 *) ((UINT32) (base) + (offset)), (value))

/* this is a special case, as the device will read as an address */

#define RFD_NEXT_RD(base, value)					\
    {									\
    volatile UINT32      tempVal;                                       \
									\
    RFD_LONG_RD ((UINT32) (base), RFD_NEXT_OFFSET, (tempVal));		\
									\
    value = (UINT32) FEI_SYS_TO_VIRT (((UINT32) (tempVal)));		\
    }

/* various command frame descriptors macros */

#define CFD_PKT_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) cfdBase + CFD_PKT_OFFSET))

#define RFD_PKT_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) cfdBase + RFD_PKT_OFFSET))

#define CFD_IA_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) (cfdBase) + CFD_IA_OFFSET))

#define CFD_MC_ADDR(cfdBase)						\
    ((UINT32 *) ((UINT32) (cfdBase) + CFD_MC_OFFSET))

#define CFD_CONFIG_WR(address, value)					\
    FEI_BYTE_WR ((UINT32 *) ((UINT32) (address)),          		\
	     (value))

#define I82557_INT_ENABLE						\
    CSR_BYTE_WR (CSR_INT_OFFSET, (UINT8) 0)

#define I82557_INT_DISABLE						\
    CSR_BYTE_WR (CSR_INT_OFFSET, (UINT8) SCB_C_M)

/* globals */

FUNCPTR feiEndIntConnect = (FUNCPTR) intConnect;
FUNCPTR feiEndIntDisconnect = (FUNCPTR) NULL;

/* locals */

/* The definition of the driver control structure */

typedef struct drv_ctrl
    {
    END_OBJ        	endObj;		/* base class */
    int		   	unit;		/* unit number */
    char *		pMemBase;	/* 82557 memory pool base */
    ULONG		memSize;	/* 82557 memory pool size */
    char *		pMemArea; 	/* cluster block pointer */
    int	    		nCFDs;		/* how many CFDs to create */
    int	    		nRFDs;		/* how many RFDs to create */
    volatile CSR_ID	pCSR;		/* pointer to CSR base */
    volatile CFD_ID	pFreeCFD;	/* current free CFD */
    volatile CFD_ID	pUsedCFD;	/* first used CFD */
    volatile RFD_ID	pRFD;		/* current Receive Frame Descriptor */
    INT8		flags;		/* driver state */
    BOOL		attached;	/* interface has been attached */
    volatile BOOL	rxHandle;	/* rx handler scheduled */
    BOOL		txHandle;	/* tx handler scheduled */
    BOOL		txStall;	/* tx handler stalled - no CFDs */
    CACHE_FUNCS		cacheFuncs;	/* cache descriptor */
    FEI_BOARD_INFO  	board;		/* board specific info */
    CL_POOL_ID  	pClPoolId;	/* cluster pool identifier */
    } DRV_CTRL;

/* Function declarations not in any header files */

IMPORT STATUS    sys557Init (int unit, FEI_BOARD_INFO *pBoard);

/* forward function declarations */

LOCAL int	fei82557ClkRate = 0;

LOCAL STATUS    fei82557InitParse (DRV_CTRL *pDrvCtrl, char *initString);
LOCAL STATUS    fei82557InitMem (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Send (DRV_CTRL *pDrvCtrl, M_BLK *pMblk);

LOCAL UINT16	fei82557Action (DRV_CTRL *pDrvCtrl, UINT16 action);
LOCAL STATUS    fei82557PhyInit (DRV_CTRL *pDrvCtrl);
LOCAL STATUS	fei82557Stop (DRV_CTRL *pDrvCtrl);
LOCAL STATUS 	fei82557Reset (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557SCBCommand (DRV_CTRL *pDrvCtrl, UINT8 cmd, 
			       BOOL addrValid, UINT32 *addr);
LOCAL STATUS    fei82557Diag (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557IASetup (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Config (DRV_CTRL *pDrvCtrl);
LOCAL void	fei82557MCastListForm (DRV_CTRL *pDrvCtrl, CFD_ID pCFD);
LOCAL void	fei82557ConfigForm (DRV_CTRL *pDrvCtrl, CFD_ID pCFD);
LOCAL int	fei82557MDIPhyLinkSet (DRV_CTRL *pDrvCtrl, int phyAddr);
LOCAL STATUS    fei82557NOP (DRV_CTRL *pDrvCtrl);
LOCAL void	fei82557CFDFree (DRV_CTRL *pDrvCtrl);
LOCAL void	fei82557FDUpdate (DRV_CTRL *pDrvCtrl, UINT8 fdList);
LOCAL void	fei82557RFDReturn (DRV_CTRL *pDrvCtrl, volatile RFD_ID pRFDNew);
LOCAL STATUS	fei82557MDIPhyConfig (DRV_CTRL *pDrvCtrl, int phyAddr);
LOCAL void	fei82557Int (DRV_CTRL *pDrvCtrl);
LOCAL void      fei82557HandleRecvInt (DRV_CTRL *pDrvCtrl);
LOCAL void	fei82557Receive (DRV_CTRL *pDrvCtrl, RFD_ID pRfd);
LOCAL int	fei82557MDIRead (DRV_CTRL *pDrvCtrl, int regAddr,
			    int phyAddr, UINT16 *retVal);
LOCAL int	fei82557MDIWrite (DRV_CTRL *pDrvCtrl, int regAddr,
			    int phyAddr, UINT16 writeData);

LOCAL STATUS    fei82557ErrCounterDump (DRV_CTRL *pDrvCtrl, UINT32 *memAddr);
LOCAL STATUS    fei82557DumpPrint (DRV_CTRL *pDrvCtrl);


/* END Specific interfaces. */

END_OBJ *	fei82557EndLoad (char *initString);    
LOCAL STATUS    fei82557Start (DRV_CTRL *pDrvCtrl);
LOCAL STATUS	fei82557Unload (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557Stop (DRV_CTRL *pDrvCtrl);
LOCAL int       fei82557Ioctl (DRV_CTRL *pDrvCtrl, int cmd, caddr_t data);
LOCAL STATUS    fei82557Send (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    fei82557MCastAddrAdd (DRV_CTRL *pDrvCtrl, char* pAddress);
LOCAL STATUS    fei82557MCastAddrDel (DRV_CTRL *pDrvCtrl, char* pAddress);
LOCAL STATUS    fei82557MCastAddrGet (DRV_CTRL *pDrvCtrl,
                                        MULTI_TABLE *pTable);
LOCAL STATUS    fei82557PollSend (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    fei82557PollReceive (DRV_CTRL *pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    fei82557PollStart (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    fei82557PollStop (DRV_CTRL *pDrvCtrl);


/* 
 * Define the device function table.  This is static across all driver
 * instances.
 */

LOCAL NET_FUNCS netFuncs = 
    {
    (FUNCPTR)fei82557Start,		/* start func. */		 
    (FUNCPTR)fei82557Stop,		/* stop func. */
    (FUNCPTR)fei82557Unload,		/* unload func. */		
    (FUNCPTR)fei82557Ioctl,		/* ioctl func. */		 
    (FUNCPTR)fei82557Send,		/* send func. */		  
    (FUNCPTR)fei82557MCastAddrAdd,    	/* multicast add func. */	 
    (FUNCPTR)fei82557MCastAddrDel,    	/* multicast delete func. */      
    (FUNCPTR)fei82557MCastAddrGet,    	/* multicast get fun. */	  
    (FUNCPTR)fei82557PollSend,    	/* polling send func. */	  
    (FUNCPTR)fei82557PollReceive,    	/* polling receive func. */
    endEtherAddressForm,   	/* put address info into a NET_BUFFER. */
    endEtherPacketDataGet, 	/* get pointer to data in NET_BUFFER. */
    endEtherPacketAddrGet  	/* Get packet addresses. */
    };		

/*******************************************************************************
*
* fei82557EndLoad - initialize the driver and device
*
* This routine initializes both, driver and device to an operational state
* using device specific parameters specified by <initString>.
*
* The parameter string, <initString>, is an ordered list of parameters each
* separated by a colon. The format of <initString> is,
* "<unit>:<memBase>:<memSize>:<nCFDs>:<nRFDs>:<flags>"
*
* The 82557 shares a region of memory with the driver.  The caller of this
* routine can specify the address of this memory region, or can specify that
* the driver must obtain this memory region from the system resources.
*
* A default number of transmit/receive frames of 32 can be selected by
* passing zero in the parameters <nTfds> and <nRfds>. In other cases, the
* number of frames selected should be greater than two.
*
* The <memBase> parameter is used to inform the driver about the shared
* memory region.  If this parameter is set to the constant "NONE," then this
* routine will attempt to allocate the shared memory from the system.  Any
* other value for this parameter is interpreted by this routine as the address
* of the shared memory region to be used. The <memSize> parameter is used
* to check that this region is large enough with respect to the provided
* values of both transmit/receive frames.
*
* If the caller provides the shared memory region, then the driver assumes
* that this region does not require cache coherency operations, nor does it
* require conversions between virtual and physical addresses.
*
* If the caller indicates that this routine must allocate the shared memory
* region, then this routine will use cacheDmaMalloc() to obtain
* some  non-cacheable memory.  The attributes of this memory will be checked,
* and if the memory is not write coherent, this routine will abort and
* return ERROR.
*
* RETURNS: an END object pointer, or NULL on error.
*
* SEE ALSO: ifLib,
* .I "Intel 82557 User's Manual"
*/

END_OBJ* fei82557EndLoad
    (
    char *initString      /* parameter string */
    )
    {
    DRV_CTRL *	pDrvCtrl;       /* pointer to DRV_CTRL structure */
    UCHAR   	enetAddr[6];	/* ethernet address */
    UINT32	speed;
    UINT32	scbStatus;
    char        bucket[2];

    DRV_LOG (DRV_DEBUG_LOAD, ("Loading end...\n"), 1, 2, 3, 4, 5, 6);

    if (initString == NULL)
	return (NULL);

    if (initString[0] == 0)
	{
		bcopy ((char *)DEV_NAME, (void *)initString, DEV_NAME_LEN);
		return (0);
	}

    /* allocate the device structure */

    pDrvCtrl = (DRV_CTRL *) calloc (1, sizeof (DRV_CTRL));
    if (pDrvCtrl == NULL)
		{	
		printf("Error:fei82557EndLoad calloc memory fail\n");
		return (NULL);
		}

    lstInit( &pDrvCtrl->endObj.multiList );

    /* Parse InitString */

    if (fei82557InitParse (pDrvCtrl, initString) == ERROR)
		{	
		printf("Error:fei82557InitParse (%s) fail\n" , initString);
		goto errorExit;	
    	}

    /* sanity check the unit number */

    if (pDrvCtrl->unit < 0 )
		{	
		printf("Error: pDrvCtrl->unit (=%d) error\n" , pDrvCtrl->unit);
		goto errorExit;    
    	}

    /* 
     * initialize the default parameter for the Physical medium 
     * layer control user has his chance to override in the BSP, 
     * just be CAREFUL 
     */

    pDrvCtrl->board.phyAddr  = 1;
    pDrvCtrl->board.phySpeed = PHY_AUTO_SPEED;
    pDrvCtrl->board.phyDpx   = PHY_AUTO_DPX;
    pDrvCtrl->board.others   = 0;
    pDrvCtrl->board.tcbTxThresh = FEI_TCB_TX_THRESH;

    /* callout to perform adapter init */

    if (sys557Init (pDrvCtrl->unit, &pDrvCtrl->board) == ERROR)
		{
		printf("Error: sys557Init init 82559 fail\n");
		goto errorExit;
    	}

    /* get CSR address from the FEI_BOARD_INFO structure */

    if ((int)(pDrvCtrl->pCSR = (CSR_ID) pDrvCtrl->board.baseAddr) == (int)NULL)
		{		
		printf("Error: pDrvCtrl->board.baseAddr == NULL\n");
		goto errorExit;
    	}

    /* probe for memory-mapped CSR */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if (vxMemProbe ((char *) &scbStatus, VX_READ, 2,
		    &bucket[0]) != OK)
	{
	DRV_LOG (DRV_DEBUG_LOAD,
		   (": need MMU mapping for address 0x%x\n"),
		   (UINT32) pDrvCtrl->pCSR, 2, 3, 4, 5, 6);
	goto errorExit;
	}

    /* memory initialization */

    if (fei82557InitMem (pDrvCtrl) == ERROR)
	{
		printf("Error:fei82557InitMem fail\n");
		goto errorExit;
    }

    I82557_INT_DISABLE;

    /* initialize the Physical medium layer */

    if (fei82557PhyInit (pDrvCtrl) != OK)
	{
		printf("Error:fei82557PhyInit fail\n");
	}

    speed = ((((int) pDrvCtrl->board.phySpeed) == PHY_100MBS) ?
		FEI_100MBS : FEI_10MBS);

    if (fei82557ClkRate == 0)
	fei82557ClkRate = sysClkRateGet ();

    /* 
     * reset the chip: this should be replaced by a true 
     * adapter reset routine, once the init code is here.
     */

    if (fei82557Reset (pDrvCtrl) != OK)
	{	
		printf("Error:fei82557Reset fail\n");
		goto errorExit;
    }

    /* CU and RU should be idle following fei82557Reset() */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CULDBASE, TRUE, 0x0) == ERROR)
	{
		printf("Error:fei82557SCBCommand:SCB_C_CULDBASE fail\n");
		goto errorExit;
    }

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_RULDBASE, TRUE, 0x0) == ERROR)
	{
		printf("Error:fei82557SCBCommand:SCB_C_RULDBASE fail\n");
		goto errorExit;
    }

    pDrvCtrl->attached = TRUE;

    /* get our ethernet hardware address */

    bcopy ((char *)&pDrvCtrl->board.enetAddr,
	   (char *)&enetAddr[0],
	   FEI_ADDR_LEN);

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557Load...\n"
			 "ADRR: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n "), 
			enetAddr[0],
			enetAddr[1],
			enetAddr[2],
			enetAddr[3],
			enetAddr[4],
			enetAddr[5]);

    /* endObj initializations */

    if (END_OBJ_INIT (&pDrvCtrl->endObj, (DEV_OBJ*) pDrvCtrl,
		      DEV_NAME, pDrvCtrl->unit, &netFuncs,
		      "Intel 82557 Ethernet Enhanced Network Driver") == ERROR)
	goto errorExit;

    if (END_MIB_INIT (&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd, 
		      (u_char *) &enetAddr[0], FEI_ADDR_LEN,
		      ETHERMTU, speed) == ERROR)
	goto errorExit;

    /* Mark the device ready */

    END_OBJ_READY (&pDrvCtrl->endObj,
                   IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST);


    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557Load... Done \n"), 1, 2, 3, 4, 5, 6);

    return (&pDrvCtrl->endObj);

errorExit:
    fei82557Unload (pDrvCtrl);
    free ((char *) pDrvCtrl);
    return NULL;

    }

/*******************************************************************************
*
* fei82557Unload - unload a driver from the system
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557Unload
    (
    DRV_CTRL *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    
    DRV_LOG (DRV_DEBUG_LOAD, ("Unloading end..."), 1, 2, 3, 4, 5, 6);

    pDrvCtrl->attached = FALSE;

    /* free lists */

    END_OBJECT_UNLOAD (&pDrvCtrl->endObj);

    /* free allocated memory if necessary */

    if ((FEI_FLAG_ISSET (FEI_OWN_MEM)) && 
	(pDrvCtrl->pMemBase != NULL))
	cacheDmaFree (pDrvCtrl->pMemBase);

    /* free allocated memory if necessary */

    cfree (pDrvCtrl->pMemArea);

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557Unload... Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);
    }

/*******************************************************************************
*
* fei82557InitParse - parse parameter values from initString
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS fei82557InitParse
    (
    DRV_CTRL *	pDrvCtrl,      		/* pointer to DRV_CTRL structure */
    char *	initString		/* parameter string */
    )

    {
    char *  tok;		/* an initString token */
    char *  holder = NULL;	/* points to initString fragment beyond tok */

    tok = strtok_r (initString, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->unit = atoi (tok);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->pMemBase = (char *) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->memSize = strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->nCFDs = strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->nRFDs = strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
	return ERROR;
    pDrvCtrl->flags = atoi (tok);

    if (!pDrvCtrl->nCFDs || pDrvCtrl->nCFDs <= 2)
	{
	FEI_FLAG_SET (FEI_INV_NCFD);
	pDrvCtrl->nCFDs = DEF_NUM_CFDS;
	}

    if (!pDrvCtrl->nRFDs || pDrvCtrl->nRFDs <= 2)
	{
	FEI_FLAG_SET (FEI_INV_NRFD);
	pDrvCtrl->nRFDs = DEF_NUM_RFDS;
	}

    DRV_LOG (DRV_DEBUG_LOAD,
	    ("fei82557EndLoad: unit=%d pMemBase=0x%x memSize=0x%x"   
	     "nCFDs=%d nRFDs=%d flags=%d\n"),
	    pDrvCtrl->unit, 
	    (int) pDrvCtrl->pMemBase,
	    (int) pDrvCtrl->memSize,
	    pDrvCtrl->nCFDs,
	    pDrvCtrl->nRFDs,
	    pDrvCtrl->flags);

    return (OK);
    }


/*******************************************************************************
* fei82557InitMem - initialize memory
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS fei82557InitMem
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    volatile CFD_ID	pCFD;	   	/* pointer to CFDs */
    volatile RFD_ID	pRFD;	   	/* pointer to RFDs */
    UINT32		size;	   	/* temporary size holder */
    int			ix;		/* a counter */
    int			nRFDLoan;	/* number of loaning RFDs */
    BOOL		firstCFD = TRUE;/* is this the first CFD? */
    BOOL		firstRFD = TRUE;/* is this the first RFD? */
    M_CL_CONFIG	 fei82557MclBlkConfig =
    {
	/*
	 *  mBlkNum  	clBlkNum     	memArea     memSize
	 *  -----------  	----		-------     -------
	 */
	    0,		0, 		NULL, 	    0
    };

    CL_DESC	 fei82557ClDescTbl [] =
    {
	/*
	 *  clusterSize  		num    	memArea     memSize
	 *  -----------  		----	-------     -------
	 */

	{ETHERMTU + EH_SIZE + 2,	0,  	NULL,       0}    
    };

    int fei82557ClDescTblNumEnt = (NELEMENTS (fei82557ClDescTbl));

    /* initialize the netPool */

    if ((pDrvCtrl->endObj.pNetPool = malloc (sizeof (NET_POOL))) == NULL)
	return (ERROR);

    /* it looks sensible */

    nRFDLoan = pDrvCtrl->nRFDs;

    /* 
     * the final 4 is to allow for alignment 
     */

    size = (((pDrvCtrl->nRFDs) * CL_RFD_SIZE) +
	    (nRFDLoan * CL_RFD_SIZE) +
	    ((pDrvCtrl->nCFDs) * CFD_SIZE) + 
	    4);

    /* 
     * Establish the memory area that we will share with the device.  If
     * the caller has provided an area, then we assume it is non-cacheable
     * and will not require the use of the special cache routines.
     * If the caller did not provide an area, then we must obtain it from
     * the system, using the cache savvy allocation routine.
     */

    switch ((int) pDrvCtrl->pMemBase)
	{
	case NONE :			     /* we must obtain it */

	    /* this driver can't handle write incoherent caches */

	    if (!CACHE_DMA_IS_WRITE_COHERENT ()) 
		{
		DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: shared" 
					    "memory not cache coherent\n"),
					    1, 2, 3, 4, 5, 6);
		return (ERROR);
		}

	    pDrvCtrl->pMemBase = cacheDmaMalloc (size);

	    if (pDrvCtrl->pMemBase == NULL)      /* no memory available */
		{
		DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: could not" 
					    "obtain memory\n"),
					    1, 2, 3, 4, 5, 6);
		return (ERROR);
		}


	    pDrvCtrl->memSize = size;
	    FEI_FLAG_SET (FEI_OWN_MEM);
	    pDrvCtrl->cacheFuncs = cacheDmaFuncs;

	    break;

	default :			       /* the user provided an area */

	    if (pDrvCtrl->memSize == 0) 
		{
		DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: not enough" 
					   "memory\n"),
					   1, 2, 3, 4, 5, 6);
		return (ERROR);
		}

	    /* 
	     * check the user provided enough memory with reference
	     * to the given number of receive/transmit frames, if any.
	     */

	    if (FEI_FLAG_ISSET (FEI_INV_NCFD) && 
		FEI_FLAG_ISSET (FEI_INV_NRFD))
		{
		pDrvCtrl->nCFDs = pDrvCtrl->nRFDs = 
				(FEI_SAFE_MEM (pDrvCtrl) / 
				 ((2 * CL_RFD_SIZE) + CFD_SIZE));
		}
	    else if (FEI_FLAG_ISSET (FEI_INV_NCFD))
		{
		pDrvCtrl->nCFDs = 
				((FEI_SAFE_MEM (pDrvCtrl) - 
				 (2 * FEI_RFD_MEM (pDrvCtrl))) /
				 (CFD_SIZE));
		}
	    else if (FEI_FLAG_ISSET (FEI_INV_NRFD))
		{
		pDrvCtrl->nRFDs = 
				((FEI_SAFE_MEM (pDrvCtrl) - 
				 FEI_CFD_MEM (pDrvCtrl)) /
				 (2 * CL_RFD_SIZE));
		}
	    else
		{
		if (pDrvCtrl->memSize < size) 
		    {
		    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: not enough " 
					      "memory\n"),
					      1, 2, 3, 4, 5, 6);
		    return (ERROR);
		    }
		}

	    if ((pDrvCtrl->nCFDs <= 2) || (pDrvCtrl->nRFDs <= 2))
		{
		DRV_LOG (DRV_DEBUG_LOAD, ("fei82557EndLoad: not enough "
					  "memory\n"),
					  1, 2, 3, 4, 5, 6);
		return (ERROR);
		}

	    FEI_FLAG_CLEAR (FEI_OWN_MEM);
	    pDrvCtrl->cacheFuncs = cacheNullFuncs;

	    break;
	}

    /* zero the shared memory */

    memset (pDrvCtrl->pMemBase, 0, (int) pDrvCtrl->memSize);

    /* pool of mblks */

    if (fei82557MclBlkConfig.mBlkNum == 0)
	fei82557MclBlkConfig.mBlkNum = (pDrvCtrl->nRFDs) * 4;

    /* pool of clusters, including loaning buffers */

    if (fei82557ClDescTbl[0].clNum == 0)
	{
	fei82557ClDescTbl[0].clNum = ((pDrvCtrl->nRFDs) * 2);
	fei82557ClDescTbl[0].clSize = RFD_SIZE;
	}

    fei82557ClDescTbl[0].memSize = (fei82557ClDescTbl[0].clNum * 
				  (fei82557ClDescTbl[0].clSize + CL_OVERHEAD));
    fei82557ClDescTbl[0].memArea = pDrvCtrl->pMemBase + 
				  (pDrvCtrl->nCFDs * CFD_SIZE);

    /* align the shared memory */

    fei82557ClDescTbl[0].memArea = (char *) 
				   (((u_long) fei82557ClDescTbl[0].memArea + 0x3)
				   & ~0x3);

    /* pool of cluster blocks */

    if (fei82557MclBlkConfig.clBlkNum == 0)
	fei82557MclBlkConfig.clBlkNum = fei82557ClDescTbl[0].clNum;

    /* get memory for mblks */

    if (fei82557MclBlkConfig.memArea == NULL)
	{
	/* memory size adjusted to hold the netPool pointer at the head */

	fei82557MclBlkConfig.memSize = ((fei82557MclBlkConfig.mBlkNum *
				       (M_BLK_SZ + 4))  +
				       (fei82557MclBlkConfig.clBlkNum * 
					CL_BLK_SZ + 4));

	if ((fei82557MclBlkConfig.memArea = (char *) memalign
					    (4, fei82557MclBlkConfig.memSize))
	    == NULL)
	    return (ERROR);

	/* store the pointer to the cluster area */

	pDrvCtrl->pMemArea = fei82557MclBlkConfig.memArea;
	}

    /* init the mem pool */

    if (netPoolInit (pDrvCtrl->endObj.pNetPool, &fei82557MclBlkConfig, 
		     &fei82557ClDescTbl[0], fei82557ClDescTblNumEnt, NULL) 
		     == ERROR)
	return (ERROR);

    if ((pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool,
					       RFD_SIZE, FALSE)) == NULL)
	return (ERROR);

    /* carve up the shared-memory region */

    /*
     * N.B.
     * We are setting up the CFD ring as a ring of TxCBs, tied off with a
     * CFD_C_SUSP as frames are copied into the data buffers.  The
     * susp/resume model is used because the links to the next CFDs do
     * not change -- it is a fixed ring.  Also note that if a CFD is needed
     * for anything else (e.g., DIAG, NOP, DUMP, CONFIG, or IASETUP comands),
     * then the commands will use the current CFD in the ring.  After the
     * command is complete, it will be set back to a TxCB by fei82557Action().
     */

    /* First ready CFD pointer */

    pCFD = pDrvCtrl->pFreeCFD = (CFD_ID) pDrvCtrl->pMemBase;

    /* initialize the CFD ring */

    for (ix = 0; ix < pDrvCtrl->nCFDs; ix++)
	{
	CFD_WORD_WR (pCFD, CFD_STAT_OFFSET,
		     (CFD_S_COMPLETE | CFD_S_OK));
    
	/* tie the current CFD to the next one */

    DRV_LOG (DRV_DEBUG_LOAD, ("CFD %08lx Virt %08lx\n"),
            ((UINT32)pCFD + CFD_SIZE),
            FEI_VIRT_TO_SYS( ((UINT32)pCFD + CFD_SIZE) ), 3, 4, 5, 6);
	CFD_NEXT_WR (pCFD, ((UINT32) pCFD + CFD_SIZE));
    
	if (!firstCFD)
	    {
	    /* Previous CFD pointer */

	    CFD_LONG_WR (pCFD, CFD_PREV_OFFSET,
			 ((UINT32) pCFD - CFD_SIZE));
	    }
	else
	    {
	    /* remember this CFD */

	    pDrvCtrl->pFreeCFD = pCFD;

	    /* tie the first CFD to the last one */

	    CFD_LONG_WR (pCFD, CFD_PREV_OFFSET,
			 ((UINT32) pCFD + (CFD_SIZE * (pDrvCtrl->nCFDs - 1))));

	    firstCFD = FALSE;
	    }

	/* no TBDs used */

	CFD_TBD_WR (pCFD, (UINT32) TBD_NOT_USED);
	CFD_BYTE_WR (pCFD, CFD_NUM_OFFSET, 0);

	/* set the thresh value */

	CFD_BYTE_WR (pCFD, CFD_THRESH_OFFSET, 
		     pDrvCtrl->board.tcbTxThresh);

	/* bump to the next CFD */

	pCFD = (CFD_ID) ((UINT32) pCFD + CFD_SIZE);
	}

    pCFD = (CFD_ID) ((UINT32) pCFD - CFD_SIZE);

    /* tie the last CFD to the first one */

    CFD_NEXT_WR (pCFD, ((UINT32) pDrvCtrl->pFreeCFD));

    /* set the used CFDs ring to the free one */

    pDrvCtrl->pUsedCFD = pDrvCtrl->pFreeCFD;

    /*
     * N.B.
     * Use RFD_C_EL to tie the end of the RBD ring, and not RFD_C_SUSP
     * This is because we want the option of inserting a new RFD into
     * the ring on the fly (i.e., via an SCB_RUSTART command).  Why would
     * we do this?  Buffer loaning....  A suspend/resume reception model
     * will not allow us to do this, so we must use an idle/start model.
     */

    /* initialize the RFD ring, some of the fields will be done 
     * in fei82557Start 
     */

    for (ix = 0; ix < pDrvCtrl->nRFDs; ix++)
	{
	volatile RFD_ID    pRFDOld;

	if ((pRFD = (RFD_ID) netClusterGet (pDrvCtrl->endObj.pNetPool,
					   pDrvCtrl->pClPoolId)) == NULL)
	    return (ERROR);

	if (firstRFD)
	    {
	    /* first ready RFD pointer */

	    pDrvCtrl->pRFD = pRFD;
	    firstRFD = FALSE;
	    }
	else
	    {
	    /* tie the previous RFD to the current one */

	    RFD_NEXT_WR (pRFDOld, (UINT32) pRFD);
	    RFD_LONG_WR (pRFD, RFD_PREV_OFFSET, (UINT32) pRFDOld);
	    }

	/* no RBDs used */

        RFD_RBD_WR (pRFD, (UINT32) RBD_NOT_USED);

	/* set the actual count field to zero */

        RFD_WORD_WR (pRFD, RFD_COUNT_OFFSET, 0);

	/* set the size field for Ethernet packets */

        RFD_WORD_WR (pRFD, RFD_SIZE_OFFSET,
		     (UINT32) (ETHERMTU + EH_SIZE));

	/* remember this RFD */

	pRFDOld = pRFD;
	}

    /* tie the last RFD to the first one */

    RFD_NEXT_WR ((UINT32) pRFD, (UINT32) pDrvCtrl->pRFD);

    /* link the first RFD to the last one */

    RFD_LONG_WR (pDrvCtrl->pRFD, RFD_PREV_OFFSET, (UINT32) pRFD);

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    DRV_LOG (DRV_DEBUG_LOAD, ("fei82557InitMem... Done\n"),
				0, 0, 0, 0, 0, 0);

    return OK;
    }

/**************************************************************************
*
* fei82557Start - start the device
*
* This routine starts the 82557 device and brings it up to an operational
* state.  The driver must have already been attached with the fei82557Load()
* routine.
*
* RETURNS: OK, or ERROR if the device could not be initialized.
*/

LOCAL STATUS fei82557Start
    (
    DRV_CTRL *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    RFD_ID       pRFD;
    UINT32       tempVar;
    int		retVal;

    DRV_LOG (DRV_DEBUG_START, ("Starting end...\n"), 1, 2, 3, 4, 5, 6);

    /* must have been attached */

    if (!pDrvCtrl->attached)
    	{
    	printf("!pDrvCtrl->attached\n");
		return (ERROR);
    	}

    /* reset the chip */

    if (fei82557Reset (pDrvCtrl) == ERROR)
    	{
    	printf("fei82557Reset error\n");
		return (ERROR);
    	}

    /* connect the int handler */

    SYS_INT_CONNECT (pDrvCtrl, fei82557Int, (int) pDrvCtrl, &retVal);
    if (retVal == ERROR)
    	{
    	printf("SYS_INT_CONNECT error\n");
		return (ERROR);
    	}

    /* acknowledge interrupts */

    SYS_INT_ACK (pDrvCtrl);

    /* enable chip interrupts after fei82557Reset disabled them */

    I82557_INT_ENABLE;

    /* enable system interrupts after fei82557Reset disabled them */

    SYS_INT_ENABLE (pDrvCtrl);

    /* run diagnostics */
  	
    if (fei82557Diag (pDrvCtrl) == ERROR)    
    	{
    	printf("fei82557Diag error\n");
		return (ERROR);
    	}

    /* setup address */

    if (fei82557IASetup (pDrvCtrl) == ERROR)
    	{
    	printf("fei82557IASetup error\n");
		return (ERROR);
    	}

    if (END_FLAGS_ISSET (IFF_MULTICAST))
        FEI_FLAG_SET (FEI_MCAST);
 
    /* configure chip */

    if (fei82557Config (pDrvCtrl) == ERROR)
    	{
    	printf("fei82557Config error\n");
		return (ERROR);
    	}
    /* set some flags to default values */

    pDrvCtrl->rxHandle = FALSE;
    pDrvCtrl->txHandle = FALSE;
    pDrvCtrl->txStall = FALSE;

    /* get the current free RFD */

    RFD_GET (pRFD);

    /* initiailize RFD ring */

    do 
	{

	/* set state and command for this RFD */

	RFD_WORD_WR (pRFD, RFD_STAT_OFFSET, (UINT32) 0);
	RFD_WORD_WR (pRFD, RFD_COMM_OFFSET, (UINT32) 0);

	/* bump to the next RFD */

	RFD_NEXT_RD (pRFD, tempVar);
	pRFD = (RFD_ID) tempVar;

	} while (pRFD != pDrvCtrl->pRFD);

    /* mark the last RFD for the chip to became idle */

    RFD_LONG_RD (pRFD, RFD_PREV_OFFSET, tempVar);
    RFD_WORD_WR (tempVar, RFD_COMM_OFFSET, (UINT32) RFD_C_EL);

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* put CU into suspended state */

    if (fei82557NOP (pDrvCtrl) == ERROR)
    	{
    	printf("fei82557NOP error\n");
		return (ERROR);
    	}

    /* mark the interface as up */

    END_FLAGS_SET (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

    /* startup the receiver */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_RUSTART, TRUE, pRFD)== ERROR)
    	{
    	printf("fei82557SCBCommand error\n");
		return (ERROR);
    	}
    DRV_LOG (DRV_DEBUG_START, ("Starting end... Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);

    }


/*******************************************************************************
* fei82557Send - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* muxSend() calls this routine each time it wants to send a packet.
*
* RETURNS: N/A
*/

LOCAL STATUS fei82557Send
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    M_BLK * 	pMblk           /* pointer to the mBlk/cluster pair */
    )

    {
    volatile CFD_ID 	pCFD;
    volatile CFD_ID 	pPrevCFD;
    int			len = 0;
    volatile UINT16	command;
    volatile UINT16	status;
    char *		pEnetHdr;
    volatile UINT16	scbStatus;

    DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...\n"), 1, 2, 3, 4, 5, 6);

    /* check device mode */

    if (FEI_FLAG_ISSET (FEI_POLLING))
        {
        return (ERROR);
        }

    /* check for CFDs availability */

    /* get the current free CFD */

    FREE_CFD_GET (pCFD);
 
    /* Make cache consistent with memory */

    FEI_CACHE_INVALIDATE (pCFD, 4);
 
    /* read the CFD status word */

    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);
 
    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
	{

	DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...NO CFDS \n"), 1, 2, 3, 4, 5, 6);

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1);

	/* set to stall condition */

	END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);
	pDrvCtrl->txStall = TRUE;
	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	return (END_ERR_BLOCK);

	}

    /* interlock with fei82557CFDFree */

    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

    pEnetHdr = (char *) (CFD_PKT_ADDR (pCFD));
    len = netMblkToBufCopy (pMblk, (char *) pEnetHdr, NULL);
    netMblkClChainFree (pMblk);

    len = max (ETHERSMALL, len); 

    /* set up the current CFD */

    CFD_WORD_WR (pCFD, CFD_COUNT_OFFSET, ((len & 0x3fff) | TCB_CNT_EOF));
    CFD_WORD_WR (pCFD, CFD_COMM_OFFSET, (CFD_C_XMIT | CFD_C_SUSP));
    CFD_WORD_WR (pCFD, CFD_STAT_OFFSET, 0);

    /* set the thresh value */

    CFD_BYTE_WR (pCFD, CFD_THRESH_OFFSET, 
		 pDrvCtrl->board.tcbTxThresh);

    /* no TBDs used */

    CFD_BYTE_WR (pCFD, CFD_NUM_OFFSET, 0);

    /* this is a transmit command */

    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_TX);

    /* tie to the previous CFD */

    CFD_LONG_RD (pCFD, CFD_PREV_OFFSET, pPrevCFD);
    CFD_WORD_RD (pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR (pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

    /* kick the CU if needed */

    /* read CSR status word */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if ((scbStatus & SCB_S_CUMASK) == SCB_S_CUSUSP)
	{

	if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE,(UINT32) 0) 
	    == ERROR)
	    {
	    DRV_LOG (DRV_DEBUG_TX, ("Could not send packet\n"), 
				      1, 2, 3, 4, 5, 6);
	    return (ERROR);
	    }
	else
	    {
	    /* in the old driver ther was a second attempt 
	     * at transimitting the packet, but is it sensible?
	     */
	    }
	}

    /* interlock with fei82557CFDFree */

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    DRV_LOG (DRV_DEBUG_TX, ("fei82557Send...Done\n"), 1, 2, 3, 4, 5, 6);

    return (OK);

    }

/*******************************************************************************
* fei82557Int - entry point for handling interrupts from the 82557
*
* The interrupting events are acknowledged to the device, so that the device
* will de-assert its interrupt signal.  The amount of work done here is kept
* to a minimum; the bulk of the work is deferred to the netTask.
*
* RETURNS: N/A
*/

LOCAL void fei82557Int
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    UINT16  event;
    UINT16  scbStatus;
    UINT16  scbCommand;

    /* read and save CSR status word */

    CSR_WORD_RD (CSR_STAT_OFFSET, event);

    if ((event & SCB_S_STATMASK) == 0)
	{
	/* This device was not the cause of the shared int */

	return;  
	}

    /* clear chip level interrupt pending, use byte access */

    CSR_BYTE_WR (CSR_ACK_OFFSET, ((event & SCB_S_STATMASK) >> 8));

    /* board level interrupt acknowledge */

    SYS_INT_ACK (pDrvCtrl);

    /* read CSR status word again */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    DRV_LOG (DRV_DEBUG_INT, ("fei82557Int: event 0x%x, scbStatus 0x%x\n"), 
			     event, scbStatus, 0, 0, 0, 0);

    /* read CSR command word */

    CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);

    /* handle transmit interrupts */

    if ((event & SCB_S_CNA)  && (!((UINT8) (scbCommand >> 8) & SCB_C_M)) &&
	(!(pDrvCtrl->txHandle))) 
	{
	I82557_INT_DISABLE;

	pDrvCtrl->txHandle = TRUE;

	(void) netJobAdd ((FUNCPTR) fei82557CFDFree, (int) pDrvCtrl,
			  0, 0, 0, 0);
	}

    /* handle receive interrupts only if fei82557Receive is not running */

    if ((event & SCB_S_FR) && !((UINT8) (scbCommand >> 8) & SCB_C_M)
	&& !(pDrvCtrl->rxHandle))
	{

	I82557_INT_DISABLE;

	pDrvCtrl->rxHandle = TRUE;

	(void) netJobAdd ((FUNCPTR) fei82557HandleRecvInt, (int) pDrvCtrl,
			    0, 0, 0, 0);

	}

    }

/******************************************************************************
*
* fei82557HandleRecvInt - service task-level interrupts for receive frames
*
* This routine is run in netTask's context.  The ISR scheduled
* this routine so that it could handle receive packets at task level.
*
* RETURNS: N/A
*/

LOCAL void fei82557HandleRecvInt
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    volatile RFD_ID     pRFD;
    volatile UINT16     status = 0;
    volatile UINT16     scbStatus;
    volatile UINT16     scbCommand;
    int	ix;

    DRV_LOG (DRV_DEBUG_RX, ("fei82557HandleRecvInt: \n"),
			    0, 0, 0, 0, 0, 0);

    /* while RFDs to be processed */
 
    FOREVER
    {

    	/* check for RFDs availability */
    
    	/* get the first used RFD */
    
    	RFD_GET (pRFD);
    
    	/* Make cache consistent with memory */
    
    	FEI_CACHE_INVALIDATE (pRFD, RFD_SIZE);
    					
    	/* read the RFD status word */		
    
    	RFD_WORD_RD (pRFD, RFD_STAT_OFFSET, status);
    
    	if (!(FD_FLAG_ISSET (status, RFD_S_COMPLETE)))
    	{
    	    /* interlock with fei82557Int */
    
    	    pDrvCtrl->rxHandle = FALSE;
    
    	    break;
    	}
    
    	/* read and save CSR status word */
    
    	CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);
    
    	/* clear chip level interrupt pending, use byte access */
    
    	CSR_BYTE_WR (CSR_ACK_OFFSET, 
    		    ((scbStatus & (SCB_S_FR | SCB_S_RNR)) >> 8));
    
    	fei82557Receive (pDrvCtrl, pRFD);
    
	}

    /* interlock with fei82557Int */

    pDrvCtrl->rxHandle = FALSE;

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* kick receiver (if needed) */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if ((scbStatus & SCB_S_RUMASK) != SCB_S_RURDY)
    {
        DRV_LOG (DRV_DEBUG_RX, ("HandleRecvInt: ********* RU NOT READY\n"),
                                    0, 0, 0, 0, 0, 0);

    	/* get the first used RFD */
    
    	RFD_GET (pRFD);

        if (fei82557SCBCommand (pDrvCtrl, SCB_C_RUSTART, TRUE, pRFD)
            == ERROR)
        {
            DRV_LOG (DRV_DEBUG_RX, ("fei82557HandleRecvInt: command %d "
				    "failed\n"),
                                    SCB_C_RUSTART, 0, 0, 0, 0, 0);
	    }
        else
	    {
    	    /* read CSR command word */
    
    	    CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);
    
    	    DRV_LOG (DRV_DEBUG_RX, ("fei82557HandleRecvInt: CSR command word 0x%x\n"),
    	    (int) scbCommand, 0, 0, 0, 0, 0);
    
                /* wait for RUSTART accept */
    
    	    for (ix = (FEI_INIT_TMO * fei82557ClkRate); --ix;)
    		{
    		    /* read CSR command word */
        
        		CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);
        
        		if (!(scbCommand & SCB_C_RUMASK))
        		{
		    DRV_LOG (DRV_DEBUG_RX, ("fei82557HandleRecvInt: "
					    "RUSTART accepted\n"),
        					    0, 0, 0, 0, 0, 0);
        
        		    break;
        		}
        
        		/*taskDelay (1);*/
    		}
    
    	    if (!ix)
    		{
		DRV_LOG (DRV_DEBUG_RX, ("fei82557HandleRecvInt: "
					"RUSTART FAILED#######\n"),
    					0, 0, 0, 0, 0, 0);
    		}
	    }
   }

    /* re-enable receiver interrupts */

    I82557_INT_ENABLE;

    DRV_LOG (DRV_DEBUG_RX, ("fei82557HandleRecvInt... Done, \n"),
                             0, 0, 0, 0, 0, 0);

    }

/******************************************************************************
*
* fei82557Receive - pass a received frame to the next layer up
*
* RETURNS: N/A
*/

LOCAL void fei82557Receive
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    RFD_ID      pRFD        	/* pointer to a RFD */
    )
    {
    volatile RFD_ID     pNewRFD; /*= NULL; added by ju 2004-9-8 for safety of fei82557RecvError*/
    M_BLK_ID            pMblk           = NULL;
    CL_BLK_ID           pClBlk          = NULL;
    volatile UINT16     status;
    volatile UINT16     count;

    /* pass the packet up only if reception was Ok */

    RFD_WORD_RD (pRFD, RFD_STAT_OFFSET, status);

    if (!(FD_FLAG_ISSET (status, RFD_S_OK)))
	{
        goto fei82557RecvError;
        }

    if ((pMblk = netMblkGet (pDrvCtrl->endObj.pNetPool,
                            M_DONTWAIT, MT_DATA)) == NULL)
        goto fei82557RecvError;

    if ((pClBlk = netClBlkGet (pDrvCtrl->endObj.pNetPool,
                               M_DONTWAIT)) == NULL)
        goto fei82557RecvError;

    if ((pNewRFD = (RFD_ID) netClusterGet (pDrvCtrl->endObj.pNetPool,
                                          pDrvCtrl->pClPoolId)) == NULL)
        goto fei82557RecvError;

    /* is the RFD to be given back?? */

    if (netClBlkJoin (pClBlk, (char *) pRFD, RFD_SIZE,
                      NULL, 0, 0, 0) == NULL)
        goto fei82557RecvError;

    if (netMblkClJoin (pMblk, pClBlk) == NULL)
        goto fei82557RecvError;

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);

    /* get the actual received bytes number */

    RFD_WORD_RD (pRFD, RFD_COUNT_OFFSET, count);

    pMblk->mBlkHdr.mFlags   |= M_PKTHDR;
    pMblk->mBlkHdr.mData    = (char *) (RFD_PKT_ADDR (pRFD));
    pMblk->mBlkHdr.mLen = count & ~0xc000;    
    pMblk->mBlkPktHdr.len   = pMblk->mBlkHdr.mLen;
    
    /* put the new RFD on the RFD queue */

    fei82557RFDReturn(pDrvCtrl, pNewRFD);

    END_RCV_RTN_CALL (&pDrvCtrl->endObj, pMblk);

    DRV_LOG (DRV_DEBUG_RX, ("fei82557Receive... Done, \n"),
                             0, 0, 0, 0, 0, 0);

    return;

fei82557RecvError:

    if (pMblk != NULL)
        netMblkFree (pDrvCtrl->endObj.pNetPool, pMblk);
    if (pClBlk != NULL)
        netClBlkFree (pDrvCtrl->endObj.pNetPool, pClBlk);
    if (pNewRFD != NULL)
	netClFree (pDrvCtrl->endObj.pNetPool, (UCHAR *) pNewRFD);
 
    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);
      
    /* put the errored RFD on the RFD queue */

    fei82557RFDReturn(pDrvCtrl, pRFD);

    }

/**************************************************************************
*
* fei82557RFDReturn - put a Receive Frame on the Receive Queue
*
* This routine puts a Receive Frame on the Receive Queue.
*
* RETURNS: N/A
*
*/

LOCAL void fei82557RFDReturn
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    RFD_ID	pNewRFD		/* pointer to a RFD */
    )

    {
    volatile UINT32   tempVar;
    volatile RFD_ID   pRFD;  
    volatile UINT16   command;

    /* get the last used RFD */

    RFD_GET (pRFD);

    /* link it to the RFD chain */

    /* link the new RFD to the next one */

    RFD_NEXT_RD (pRFD, (UINT32) tempVar);
    RFD_LONG_WR (tempVar, RFD_PREV_OFFSET, (UINT32) pNewRFD);
    RFD_NEXT_WR (pNewRFD, tempVar);

    /* link the new RFD to the previous one */

    RFD_LONG_RD (pRFD, RFD_PREV_OFFSET, tempVar);
    RFD_NEXT_WR (tempVar, pNewRFD);
    RFD_LONG_WR (pNewRFD, RFD_PREV_OFFSET, tempVar);

    /* init the new RFD, mirrors fei82557Start */

    /* set state and command for this RFD */

    RFD_WORD_WR (pNewRFD, RFD_STAT_OFFSET, (UINT32) 0);
    RFD_WORD_WR (pNewRFD, RFD_COMM_OFFSET, (UINT32) RFD_C_EL);

    /* no RBDs used */

    RFD_RBD_WR (pNewRFD, (UINT32) RBD_NOT_USED);

    /* set the actual count field to zero */

    RFD_WORD_WR (pNewRFD, RFD_COUNT_OFFSET, 0);

    /* set the size field for Ethernet packets */

    RFD_WORD_WR (pNewRFD, RFD_SIZE_OFFSET, 
		 (UINT32) (ETHERMTU + EH_SIZE));

    /* chain the previous RFD */

    RFD_LONG_RD (pNewRFD, RFD_PREV_OFFSET, tempVar);
    RFD_WORD_RD (tempVar, RFD_COMM_OFFSET, command);
    RFD_WORD_WR (tempVar, RFD_COMM_OFFSET, (command & (~RFD_C_EL)));

    /* bump to the next free RFD */

    fei82557FDUpdate (pDrvCtrl, RFD_FREE);

    }

/**************************************************************************
*
* fei82557CFDFree - free all used command frames
*
* This routine frees all used command frames and notifies upper protocols
* that new resources are available.
*
* RETURNS: N/A
*
*/

LOCAL void     fei82557CFDFree
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    volatile CFD_ID    	pUsedCFD = NULL;
    volatile UINT16	status;
    volatile UINT16	scbStatus;
    UINT8		action;

    if (!(FEI_FLAG_ISSET (FEI_POLLING)))
	{
	I82557_INT_ENABLE;

	/* interlock with fei82557Send */

	END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

	/* read CSR status word */

	CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

	/* if CU is active, do not go on */

	if ((scbStatus & SCB_S_CUMASK) != SCB_S_CUSUSP)
	    {
	    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	    pDrvCtrl->txHandle = FALSE;

	    return;
	    }
	}

    /* process all the used CFDs until we find either:
     * a. a non-complete frame;
     * b. a frame with the SUSPEND bit set
     */

    FOREVER
        {
        static BOOL first = TRUE;

        USED_CFD_GET (pUsedCFD);
 
        /* Make cache consistent with memory */

        FEI_CACHE_INVALIDATE (pUsedCFD, 4);
 
        /* read the CFD status word */

        CFD_WORD_RD (pUsedCFD, CFD_STAT_OFFSET, status);
 
        /* if it's not ready, don't touch it! */

        if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
            {
            break;
            }

	/* put CFD back to a TxCB - mirrors fei82557InitMem() */

	/* no TBDs used */

	CFD_TBD_WR (pUsedCFD, (UINT32) TBD_NOT_USED);
	CFD_BYTE_WR (pUsedCFD, CFD_NUM_OFFSET, 0);

	/* set the thresh value */

	CFD_BYTE_WR (pUsedCFD, CFD_THRESH_OFFSET, 
		     pDrvCtrl->board.tcbTxThresh);

        /* correct statistic only in case of non-action command */

	CFD_BYTE_RD (pUsedCFD, CFD_ACTION_OFFSET, action);

        if ((action == CFD_TX) &&
            !(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
            {
	    if (!(FEI_FLAG_ISSET (FEI_POLLING)))
		{
		DRV_LOG (DRV_DEBUG_INT, ("fei82557CFDFree: Errored Frame \n"), 
					 0, 0, 0, 0, 0, 0);
		}

	    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1);
        END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, -1);

            /* make the errored CFD available */

	    CFD_WORD_WR (pUsedCFD, CFD_STAT_OFFSET, 
			 (CFD_S_OK | CFD_S_COMPLETE));
            }

	if (!(FEI_FLAG_ISSET (FEI_POLLING)))
	    {
	    if (first)
		{
		/* soon notify upper protocols CFDs are available */

		if (pDrvCtrl->txStall)
		    {
		    DRV_LOG (DRV_DEBUG_INT, ("fei82557CFDFree: Restart mux \n"), 
					     0, 0, 0, 0, 0, 0);

		    (void) netJobAdd ((FUNCPTR) muxTxRestart,
				      (int) &pDrvCtrl->endObj,
				      0, 0, 0, 0);

		    pDrvCtrl->txStall = FALSE;
		    }

		first = FALSE;
		}
	    }

        /* we have finished our work if this is a suspend CFD */

        CFD_WORD_RD (pUsedCFD, CFD_COMM_OFFSET, status);
 
        if (FD_FLAG_ISSET (status, CFD_C_SUSP))
            {
            /* advance pointer to the used CFDs even if we are exiting */

	    fei82557FDUpdate (pDrvCtrl, CFD_USED);
 
            break;
            }

        /* advance pointer to the used CFDs */

	fei82557FDUpdate (pDrvCtrl, CFD_USED);

        }

    if (!(FEI_FLAG_ISSET (FEI_POLLING)))
	{
	pDrvCtrl->txHandle = FALSE;

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	DRV_LOG (DRV_DEBUG_INT, ("fei82557CFDFree: Done \n"),
				 0, 0, 0, 0, 0, 0);
	}

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    }


/**************************************************************************
*
* fei82557FDUpdate - update the current frame descriptor
*
* This routine updates the pointer to the appropriate frame descriptor 
* ring with the value stored in the link field of the current frame 
* descriptor. Frame descriptor here can be either an RFD or a CFD in the
* free CFDs ring as well as in the used CFDs ring.
*
* RETURNS: N/A
*
*/

LOCAL void     fei82557FDUpdate
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT8	fdList		/* FD list selector */
    )

    {									
    volatile UINT32	fdAddr;						
    volatile FD_ID	pFD;						
									
    switch (fdList)							
        {								
									
        case CFD_FREE:							
									
            /* CFDs ready to be processed */				
									
	    FREE_CFD_GET (pFD);					
									
            /* get the next free CFD address */				
									
	    CFD_NEXT_RD (pFD, fdAddr);					
									
            /* bump to the next free CFD */			
									
            pDrvCtrl->pFreeCFD = (FD_ID) fdAddr;			
									
            break;							
									
        case CFD_USED:							
									
            /* CFDs already processed */				
									
	    USED_CFD_GET (pFD);					
									
            /* get the next free CFD address */				
									
	    CFD_NEXT_RD (pFD, fdAddr);					
									
            /* bump to the next used CFD */				
									
            pDrvCtrl->pUsedCFD = (FD_ID) fdAddr;			
									
            break;							
									
        case RFD_FREE:							
									
            /* RFDs ready to be processed */				
									
	    RFD_GET (pFD);					
									
            /* get the next free RFD address */				
									
	    RFD_NEXT_RD (pFD, fdAddr);					
									
            /* bump to the next free RFD */				
									
            pDrvCtrl->pRFD = (FD_ID) fdAddr;				
									
            break;							
									
        default:							
            break;							
        }								
									
    }

/**************************************************************************
*
* fei82557Stop - stop the fei82557 interface
*
* This routine marks the interface as inactive, disables interrupts and 
* resets the chip. It brings down the interface to a non-operational state. 
* To bring the interface back up, fei82557Start() must be called.
*
* RETURNS: OK, always.
*/

LOCAL STATUS fei82557Stop
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    int	retVal;

    /* disable system interrupt */

    SYS_INT_DISABLE (pDrvCtrl);     

    /* disable chip interrupt   */

    I82557_INT_DISABLE;	  

    /* mark the interface as down */

    END_FLAGS_CLR (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

    /* reset the chip */

    fei82557Reset (pDrvCtrl);

    /* disconnect the interrupt handler */

    SYS_INT_DISCONNECT (pDrvCtrl, fei82557Int, (int)pDrvCtrl, &retVal);
    if (retVal == ERROR)
	return (ERROR);

    /* wait for the reset... */

    taskDelay (2);	  

    return OK;
    } 

/**************************************************************************
*
* fei82557Reset - reset the `fei82557' interface
*
* This routine resets the chip by issuing a Port Selective Reset Command
* to the chip. The Receiver And the Transmit Unit are in idle state after
* this command is performed.
*
* RETURNS: OK or ERROR, if the command was not successful.
*/

LOCAL STATUS fei82557Reset
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    int	ix;

    /* issue a selective reset to the 82557 chip */

    CSR_LONG_WR (CSR_PORT_OFFSET, FEI_PORT_SELRESET);

    /* wait for the receive handler to catch up to the [reset] 557 */

    for (ix = (FEI_INIT_TMO * fei82557ClkRate); --ix;)
	{
	if (!pDrvCtrl->rxHandle)
	    break;

	taskDelay (1);
	}

    if (!ix)
	return (ERROR);

    return (OK);
    }

/**************************************************************************
*
* fei82557SCBCommand - deliver a command to the 82557 via the SCB
*
* This function causes the device to execute a command. An error status is
* returned if the command field does not return to zero, from a previous
* command, in a reasonable amount of time.
*
* RETURNS: OK, or ERROR if the command field appears to be frozen.
*/

LOCAL STATUS fei82557SCBCommand
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT8   	cmd,		/* command identifier */
    BOOL    	addrValid,	/* optionally fill the GP */
    UINT32 *	pAddr		/* Frame Descriptor address */
    )

    {
    volatile UINT16     scbCommand;
    int     ix;

    for (ix = 0x8000; --ix;)
	{
	/* read CSR command word */

	CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);

	if ((scbCommand & SCB_CR_MASK) == 0)
	    {
	    break;
	    }
	}

    if (!ix)
	{
	DRV_LOG (DRV_DEBUG_START,
		 ("fei82557SCBCommand: command 0x%x failed, scb 0x%x\n"),
		 cmd, scbCommand, 0, 0, 0, 0);

	return (ERROR);
	}

    /* 
     * writing the GP and SCB Command should be protected from preemption 
     * we attain this by disabling task context switch before we write 
     * to the general pointer and re-enabling it after we write the
     * command word. We also flush the write pipe in-between. 
     */

    /* optionally fill the GP */

    if (addrValid)
	{
	if (taskLock() == ERROR)
	    DRV_LOG (DRV_DEBUG_ALL, ("fei82557SCBCommand: task lock failed\n"),
		    0, 0, 0, 0, 0, 0);

	CSR_GP_WR ((UINT32) pAddr);
	}

    /* read CSR command byte */

    CSR_WORD_RD (CSR_COMM_OFFSET, scbCommand);

    /* write CSR command byte */

    CSR_BYTE_WR (CSR_COMM_OFFSET, (UINT8) (scbCommand | cmd));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    if (addrValid)
	{
	if (taskUnlock() == ERROR)
	    DRV_LOG (DRV_DEBUG_ALL, ("fei82557SCBCommand: task unlock failed\n"),
		    0, 0, 0, 0, 0, 0);
	}

    return (OK);

    } 

/*******************************************************************************
* fei82557Action - execute the specified action command
*
* This routine executes the specified action
*
* We do the command contained in the CFD synchronously, so that we know
* it's complete upon return.
*
* RETURNS: The status return of the action command, or 0 on failure.
*/

LOCAL UINT16 fei82557Action
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT16	action		/* action command identifier */
    )
    {
    volatile CFD_ID 	pCFD;
    volatile CFD_ID 	pPrevCFD;
    int			ix;
    volatile UINT16	command = 0;
    volatile UINT16	status;
    volatile UINT16     scbStatus;

    /* interlock with fei82557Send */

    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

    /* check for CFDs availability */
 
    FREE_CFD_GET (pCFD);
 
    /* Make cache consistent with memory */

    FEI_CACHE_INVALIDATE (pCFD, 4);
 
    /* read the CFD status word */

    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);
 
    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
        {
	/* interlock with fei82557CFDFree */

	END_TX_SEM_GIVE (&pDrvCtrl->endObj);

        return (0);
        }

    /* set up the current CFD */

    CFD_WORD_WR (pCFD, CFD_COMM_OFFSET, (action | CFD_C_SUSP));
    CFD_WORD_WR (pCFD, CFD_STAT_OFFSET, 0);
    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_ACTION);

    /* for some actions we need to do additional work */

    switch (action)
	{

	case CFD_C_MASETUP:

	    /* form the multicast address list */

	    fei82557MCastListForm (pDrvCtrl, pCFD);

	    break;

	case CFD_C_CONFIG:

	    /* fill the CFD with config bytes */

	    fei82557ConfigForm (pDrvCtrl, pCFD);
	    DRV_LOG (DRV_DEBUG_START, ("fei82557Action: flags 0x%x \n"),
				    (&pDrvCtrl->endObj)->flags, 0, 0, 0, 0, 0);


	    break;

	case CFD_C_IASETUP:

	    /* fill the CFD with our Ethernet address */

	    bcopy ((char *) FEI_HADDR (&pDrvCtrl->endObj),
		   (char *) CFD_IA_ADDR (pCFD),
		   FEI_HADDR_LEN (&pDrvCtrl->endObj));

	    break;

	default:
	    break;

	}

    /* tie to the previous CFD */

    CFD_LONG_RD (pCFD, CFD_PREV_OFFSET, pPrevCFD);
    CFD_WORD_RD (pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR (pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

    /* check CU operation -- kick if needed */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if ((scbStatus & SCB_S_CUMASK) != SCB_S_CUACTIVE)
        {
        if (scbStatus == SCB_S_CUIDLE)
            {
            if (fei82557SCBCommand (pDrvCtrl, SCB_C_CUSTART, TRUE, pCFD)
		== ERROR)
                {
		DRV_LOG (DRV_DEBUG_START,
			 ("fei82557SCBCommand: command failed\n"),
			 0, 0, 0, 0, 0, 0);
                }
            }
        else
            {
            if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE, 0x0) 
		== ERROR)
                {
		DRV_LOG (DRV_DEBUG_START,
			 ("fei82557SCBCommand: command failed\n"),
			 0, 0, 0, 0, 0, 0);
                }
            }
        }

    /* wait for command to complete */

    for (ix = (FEI_ACTION_TMO * fei82557ClkRate); --ix;)
	{
		CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);

		if (status & CFD_S_COMPLETE)
		    break;

		taskDelay (1);
	        }

	    if (!ix)
		{
			DRV_LOG (DRV_DEBUG_START, ("fei82557Action(action = 0x%x): Command 0x%x Status 0x%x , pCFD = 0x%x\n"),
					   action, command, status, (int)pCFD, 0, 0);

		}

    /* interlock with fei82557CFDFree */

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    return (status);
    }

/**************************************************************************
*
* fei82557MCastListForm - set up the multicast address list
*
* This routine populates the given CFD with the address list found in
* the apposite structure provided by the upper layers.
*
* RETURNS: N/A
*/

LOCAL void fei82557MCastListForm
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    CFD_ID	pCFD		/* pointer to a CFD */
    )

    {
    UINT32		mCastAddr;
    ETHER_MULTI	*	mCastNode;
    UINT16		count = 0;

    /* get the starting address for the multicast list */

    mCastAddr = (UINT32) CFD_MC_ADDR (pCFD);

    /* copy the multicast address list */

    for (mCastNode = (ETHER_MULTI *) lstFirst (&pDrvCtrl->endObj.multiList);
	 mCastNode != NULL; 
	 mCastNode = (ETHER_MULTI *) lstNext (&mCastNode->node))
	{
	bcopy ((char *) mCastNode->addr, (char *) mCastAddr, FEI_ADDR_LEN);
	mCastAddr += FEI_ADDR_LEN;
	count += FEI_ADDR_LEN;
	}

    /* set the byte counter for the list */

    CFD_WORD_WR (pCFD, CFD_MCOUNT_OFFSET, (count & ~0xc000));

    }

/**************************************************************************
*
* fei82557ConfigForm - fill the config CFD with proper values
*
* RETURNS: N/A
*/

LOCAL void fei82557ConfigForm
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    CFD_ID	pCFD		/* pointer to a CFD */
    )

    {
    UINT32 addr; 

    /* get to the starting address for config data */

    addr = (UINT32) pCFD + CFD_SIZE_HDR;

    /* set to config values recommeded by the i82557 User's Manual */

    CFD_CONFIG_WR (addr + 0, 0x16);
    CFD_CONFIG_WR (addr + 1, 0x88);
    CFD_CONFIG_WR (addr + 2, 0x00);
    CFD_CONFIG_WR (addr + 3, 0x00);
    CFD_CONFIG_WR (addr + 4, 0x00);
    CFD_CONFIG_WR (addr + 5, 0x00);

    /* it was 0x38, now 0x30 to generate CNA interrupts */

    CFD_CONFIG_WR (addr + 6, 0x30); /* save bad frame, no TNO 0x3a; */

    CFD_CONFIG_WR (addr + 7, 0x03);
    CFD_CONFIG_WR (addr + 8, 0x01);	/* MII operation */
    CFD_CONFIG_WR (addr + 9, 0x00);
    CFD_CONFIG_WR (addr + 10, 0x2e);
    CFD_CONFIG_WR (addr + 11, 0x00);
    CFD_CONFIG_WR (addr + 12, 0x60);
    CFD_CONFIG_WR (addr + 13, 0x00);
    CFD_CONFIG_WR (addr + 14, 0xf2);

    if (FEI_FLAG_ISSET (FEI_PROMISC))
	{
	CFD_CONFIG_WR (addr + 15, 0x49);
	DRV_LOG (DRV_DEBUG_START, ("fei82557ConfigForm: PROMISC!!!!!!! \n"),
				    0, 0, 0, 0, 0, 0);
	}

    else
	CFD_CONFIG_WR (addr + 15, 0x48);      /* 0x49 in the old driver */

    CFD_CONFIG_WR (addr + 16, 0x00);
    CFD_CONFIG_WR (addr + 17, 0x40);
    CFD_CONFIG_WR (addr + 18, 0xf2);

    if (pDrvCtrl->board.phyDpx != PHY_HALF_DPX)
	CFD_CONFIG_WR (addr + 19, 0x80);
    else
	CFD_CONFIG_WR (addr + 19, 0x00); /* c0 force full duplex 0x80 */
 
    CFD_CONFIG_WR (addr + 20, 0x3f);

    if (FEI_FLAG_ISSET (FEI_MCASTALL))
	{
	CFD_CONFIG_WR (addr + 21, 0x0d);
	DRV_LOG (DRV_DEBUG_START, ("fei82557ConfigForm: MULTIALL \n"),
				    0, 0, 0, 0, 0, 0);
	}

    else
	CFD_CONFIG_WR (addr + 21, 0x05);      /* 0x05 in the old driver */

    }

/**************************************************************************
*
* fei82557Diag - format and issue a diagnostic command
*
* RETURNS: OK, or ERROR if the diagnostic command failed.
*/

LOCAL STATUS fei82557Diag
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    UINT16  stat;

    if (((stat = fei82557Action (pDrvCtrl, CFD_C_DIAG)) &
	(CFD_S_OK | CFD_S_DIAG_F)) != CFD_S_OK)
	{
	    DRV_LOG (DRV_DEBUG_START,
		     (" 82557 diagnostics failed, cfdStatus 0x%x\n"),
		      stat, 0, 0, 0, 0, 0);
	return (ERROR);
	}

     return (OK);
    } 

/**************************************************************************
*
* fei82557IASetup - issue an individual address command
*
* RETURNS: OK, or ERROR if the individual address command failed.
*/

LOCAL STATUS fei82557IASetup
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {

    if (!(fei82557Action (pDrvCtrl, CFD_C_IASETUP) & CFD_S_OK))
	return (ERROR);

    return (OK);
    } 

/**************************************************************************
*
* fei82557Config - issue a config command
*
* RETURNS: OK, or ERROR if the config command failed.
*/

LOCAL STATUS fei82557Config
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    if (!(fei82557Action (pDrvCtrl, CFD_C_CONFIG) & CFD_S_OK))
	return (ERROR);

    return (OK);
    } 

/*******************************************************************************
* fei82557Ioctl - interface ioctl procedure
*
* Process an interface ioctl request.
*
* RETURNS: OK, or ERROR if the config command failed.
*/

LOCAL int fei82557Ioctl
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int         cmd,		/* command to process */
    caddr_t     data            /* pointer to data */
    )
    {
    int         error = OK;
    INT8        savedFlags;
    long        value;
    END_OBJ *   pEndObj=&pDrvCtrl->endObj;

    DRV_LOG (DRV_DEBUG_IOCTL,
             "Ioctl unit=0x%x cmd=%d data=0x%x\n",
             pDrvCtrl->unit, cmd, (int)data, 0, 0, 0);

    switch (cmd)
        {
        case EIOCSADDR:
            if (data == NULL)
                error = EINVAL;
            else
                {
                /* Copy and install the new address */

		bcopy ((char *) data,
		       (char *) FEI_HADDR (&pDrvCtrl->endObj),
		       FEI_HADDR_LEN (&pDrvCtrl->endObj));

		error = fei82557IASetup (pDrvCtrl);
                }

            break;

        case EIOCGADDR:                      
            if (data == NULL)
                error = EINVAL;
            else
		bcopy ((char *) FEI_HADDR (&pDrvCtrl->endObj),
		       (char *) data,
		       FEI_HADDR_LEN (&pDrvCtrl->endObj));

            break;

        case EIOCSFLAGS:
            value = (long) data;
            if (value < 0)
                {
                value = -value;
                value--;
                END_FLAGS_CLR (pEndObj, value);
                }
            else
                END_FLAGS_SET (pEndObj, value);

            DRV_LOG (DRV_DEBUG_IOCTL, ("endFlags=0x%x \n"),
					END_FLAGS_GET(pEndObj), 
					0, 0, 0, 0, 0);

            /* handle IFF_PROMISC */

            savedFlags = FEI_FLAG_GET();
            if (END_FLAGS_ISSET (IFF_PROMISC))
                FEI_FLAG_SET (FEI_PROMISC);
            else
                FEI_FLAG_CLEAR (FEI_PROMISC);
 
            /* handle IFF_MULTICAST */

            if (END_FLAGS_GET(pEndObj) & (IFF_MULTICAST))
                FEI_FLAG_SET (FEI_MCAST);
            else
                FEI_FLAG_CLEAR (FEI_MCAST);

            /* handle IFF_ALLMULTI */

            if (END_FLAGS_GET(pEndObj) & (IFF_ALLMULTI))
                FEI_FLAG_SET (FEI_MCASTALL);
            else
                FEI_FLAG_CLEAR (FEI_MCASTALL);

            DRV_LOG (DRV_DEBUG_IOCTL, "EIOCSFLAGS: 0x%x: 0x%x\n",
                    pEndObj->flags, savedFlags, 0, 0, 0, 0);

            if ((FEI_FLAG_GET () != savedFlags) &&
                (END_FLAGS_GET (pEndObj) & IFF_UP))
                {
		/* config down */

                END_FLAGS_CLR (pEndObj, IFF_UP | IFF_RUNNING); 

                error = fei82557Config (pDrvCtrl);

                END_FLAGS_SET (pEndObj, IFF_UP | IFF_RUNNING); /* config up */
                }

            break;

        case EIOCGFLAGS:
            DRV_LOG (DRV_DEBUG_IOCTL, "EIOCGFLAGS: 0x%x: 0x%x\n",
                    pEndObj->flags, *(long *)data, 0, 0, 0, 0);

            if (data == NULL)
                error = EINVAL;
            else
                *(long *)data = END_FLAGS_GET(pEndObj);

            break;

        case EIOCMULTIADD:
            error = fei82557MCastAddrAdd (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIDEL:
            error = fei82557MCastAddrDel (pDrvCtrl, (char *) data);
            break;

        case EIOCMULTIGET:
            error = fei82557MCastAddrGet (pDrvCtrl, (MULTI_TABLE *) data);
            break;

        case EIOCPOLLSTART:
            fei82557PollStart (pDrvCtrl);
            break;

        case EIOCPOLLSTOP:
	    DRV_LOG (DRV_DEBUG_POLL, ("IOCTL about to call fei82557PollStop\n"), 
					0, 0, 0, 0, 0, 0);

            fei82557PollStop (pDrvCtrl);
            break;

        case EIOCGMIB2:  
            if (data == NULL)
                error=EINVAL;
            else
		bcopy ((char *) &pEndObj->mib2Tbl, (char *) data,
			sizeof (pEndObj->mib2Tbl));

            break;

        default:
            DRV_LOG (DRV_DEBUG_IOCTL, ("INVALID IO COMMAND!! \n"),
					0, 0, 0, 0, 0, 0);
            error = EINVAL;
        }

    return (error);

    }

/*******************************************************************************
* fei82557MCastAddrAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557MCastAddrAdd
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    char *      pAddr		/* address to be added */
    )
    {

    int		retVal;

    DRV_LOG (DRV_DEBUG_IOCTL, ("MCastAddrAdd\n"), 0, 0, 0, 0, 0, 0);

    retVal = etherMultiAdd (&pDrvCtrl->endObj.multiList, pAddr);

    if (retVal == ENETRESET)
	{
	
        pDrvCtrl->endObj.nMulti++;
 
        if (pDrvCtrl->endObj.nMulti > N_MCAST)
            {
            etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);
            pDrvCtrl->endObj.nMulti--;
            }
        else
	    if (!(fei82557Action (pDrvCtrl, CFD_C_MASETUP) & CFD_S_OK))
		return (ERROR);
	    else
		return (OK);
	}

    return ((retVal == OK) ? OK : ERROR);

    }

/*******************************************************************************
*
* fei82557MCastAddrDel - delete a multicast address for the device
*
* This routine deletes a multicast address from the current list of
* multicast addresses.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557MCastAddrDel
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    char *      pAddr		/* address to be deleted */
    )
    {
    int		retVal;

    DRV_LOG (DRV_DEBUG_IOCTL, ("fei82557MCastAddrDel\n"), 0, 0, 0, 0, 0, 0);

    retVal = etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);

    if (retVal == ENETRESET)
	{
	if (!(fei82557Action (pDrvCtrl, CFD_C_MASETUP) & CFD_S_OK))
	    return (ERROR);

	pDrvCtrl->endObj.nMulti--;

	}

    return ((retVal == OK) ? OK : ERROR);
    }

/*******************************************************************************
* fei82557MCastAddrGet - get the current multicast address list
*
* This routine returns the current multicast address list in <pTable>
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557MCastAddrGet
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    MULTI_TABLE *pTable		/* table into which to copy addresses */
    )
    {
    DRV_LOG (DRV_DEBUG_IOCTL, ("fei82557MCastAddrGet\n"), 0, 0, 0, 0, 0, 0);

    return (etherMultiGet (&pDrvCtrl->endObj.multiList, pTable));
    }

/*******************************************************************************
* fei82557PollSend - transmit a packet in polled mode
*
* This routine is called by a user to try and send a packet on the
* device. It sends a packet directly on the network, without having to
* go through the normal process of queuing a packet on an output queue
* and the waiting for the device to decide to transmit it.
*
* These routine should not call any kernel functions.
*
* RETURNS: OK or EAGAIN.
*/

LOCAL STATUS fei82557PollSend
    (
    DRV_CTRL    *pDrvCtrl,	/* pointer to DRV_CTRL structure */
    M_BLK_ID    pMblk		/* pointer to the mBlk/cluster pair */
    )
    {
    volatile CFD_ID 	pCFD;
    volatile CFD_ID 	pPrevCFD;
    int			len = 0;
    UINT16		command;
    volatile UINT16	status;
    char *		pEnetHdr;
    volatile UINT16	scbStatus;
    int			ix;

    /* check if the command unit is active */

    CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

    if ((scbStatus & SCB_S_CUMASK) != SCB_S_CUSUSP)
	{
	DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend: CU Active\n"), 
				     0, 0, 0, 0, 0, 0);
	return (EAGAIN);
	}

    /* check for CFDs availability */

    FREE_CFD_GET (pCFD);
 
    /* Make cache consistent with memory */

    FEI_CACHE_INVALIDATE (pCFD, 4);
 
    /* read the CFD status word */

    CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);
 
    if (!(FD_FLAG_ISSET (status, (CFD_S_COMPLETE | CFD_S_OK))))
	{

	DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend: No CFDs\n"), 
				     0, 0, 0, 0, 0, 0);

	return (EAGAIN);

	}

    pEnetHdr = (char *) (CFD_PKT_ADDR (pCFD));
    len = netMblkToBufCopy (pMblk, (char *) pEnetHdr, NULL);
    netMblkClChainFree (pMblk);

    len = max (ETHERSMALL, len);

    /* set up the current CFD */

    CFD_WORD_WR (pCFD, CFD_COUNT_OFFSET, ((len & 0x3fff) | TCB_CNT_EOF));
    CFD_WORD_WR (pCFD, CFD_COMM_OFFSET, (CFD_C_XMIT | CFD_C_SUSP));
    CFD_WORD_WR (pCFD, CFD_STAT_OFFSET, 0);

    /* set the thresh value */

    CFD_BYTE_WR (pCFD, CFD_THRESH_OFFSET, 
		 pDrvCtrl->board.tcbTxThresh);

    /* no TBDs used */

    CFD_BYTE_WR (pCFD, CFD_NUM_OFFSET, 0);

    /* this is a transmit command */

    CFD_BYTE_WR (pCFD, CFD_ACTION_OFFSET, CFD_TX);

    /* tie to the previous CFD */

    CFD_WORD_RD (pPrevCFD, CFD_COMM_OFFSET, command);
    CFD_WORD_WR (pPrevCFD, CFD_COMM_OFFSET, (command & (~CFD_C_SUSP)));

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* advance the current free CFD pointer */

    fei82557FDUpdate (pDrvCtrl, CFD_FREE);

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1);

    /* kick the CU */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CURESUME, FALSE,(UINT32) 0) 
	    == ERROR)
	    {
	    DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend: Send Error\n"), 
					 0, 0, 0, 0, 0, 0);

	    return (EAGAIN);
	    }
	else
	    {
	    /* in the old driver ther was a second attempt 
	     * at transimitting the packet, but is it sensible?
	     */
	    }

    /* wait for command to complete */

    for (ix = (FEI_ACTION_TMO * fei82557ClkRate); --ix;)
	{
	CFD_WORD_RD (pCFD, CFD_STAT_OFFSET, status);

	if (status & CFD_S_COMPLETE)
	    break;

	taskDelay (1);
        }

    if (!ix)
	{
	DRV_LOG (DRV_DEBUG_POLL_TX, ("F\n"), 0, 0, 0, 0, 0, 0);
	}

    /* wait for command to complete */

    for (ix = (FEI_ACTION_TMO * fei82557ClkRate); --ix;)
	{
	CSR_WORD_RD (CSR_STAT_OFFSET, scbStatus);

        if ((scbStatus & SCB_S_CUMASK) == SCB_S_CUSUSP)
	    break;

	taskDelay (1);
        }

    if (!ix)
	{
	DRV_LOG (DRV_DEBUG_POLL_TX, ("F\n"), 0, 0, 0, 0, 0, 0);
	}

    /* free this CFD */

    fei82557CFDFree (pDrvCtrl);

    DRV_LOG (DRV_DEBUG_POLL_TX, ("fei82557PollSend\n"), 0, 0, 0, 0, 0, 0);

    return (OK);
    }

/*******************************************************************************
* fei82557PollReceive - receive a packet in polled mode
*
* This routine is called by a user to try and get a packet from the
* device. It returns EAGAIN if no packet is available. The caller must
* supply a M_BLK_ID with enough space to contain the received packet. If
* enough buffer is not available then EAGAIN is returned.
*
* These routine should not call any kernel functions.
*
* RETURNS: OK or EAGAIN.
*/

LOCAL STATUS fei82557PollReceive
    (
    DRV_CTRL    *pDrvCtrl,       /* pointer to DRV_CTRL structure */
    M_BLK_ID    pMblk            /* pointer to the mBlk/cluster pair */
    )

    {
    volatile UINT16	status;
    UINT16		retVal = OK;
    volatile RFD_ID   	pRFD;  
    volatile UINT16     count;

    DRV_LOG (DRV_DEBUG_POLL_RX, ("fei82557PollReceive\n"), 0, 0, 0, 0, 0, 0);

    if ((pMblk->mBlkHdr.mFlags & M_EXT) != M_EXT)
        return (EAGAIN);

    /* get the first free RFD */

    RFD_GET (pRFD);

    /* Make cache consistent with memory */

    FEI_CACHE_INVALIDATE (pRFD, RFD_SIZE);
					
    /* read the RFD status word */		

    RFD_WORD_RD (pRFD, RFD_STAT_OFFSET, status);

    /* check for RFDs completion */

    if (!(FD_FLAG_ISSET (status, RFD_S_COMPLETE)))
	{
	return (EAGAIN);
	}

    /* check for RFDs correctness */

    if (!(FD_FLAG_ISSET (status, RFD_S_OK)))
	{

	END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_ERRS, +1);

	retVal = EAGAIN;

	goto fei82557PollReceiveEnd;    
	}

    END_ERR_ADD (&pDrvCtrl->endObj, MIB2_IN_UCAST, +1);

    /* get the actual received bytes number */

    RFD_WORD_RD (pRFD, RFD_COUNT_OFFSET, count);

    /*
     * Upper layer provides the buffer. If buffer is not large enough, 
     * we do not copy the received buffer.
     */

    if (pMblk->mBlkHdr.mLen < (count & ~0xc000))
	{
	retVal = EAGAIN;

	goto fei82557PollReceiveEnd;    
	}

    pMblk->mBlkHdr.mFlags   |= M_PKTHDR;
    pMblk->mBlkHdr.mLen = count & ~0xc000;    
    pMblk->mBlkPktHdr.len   = pMblk->mBlkHdr.mLen;

    bcopy ((char *) (RFD_PKT_ADDR (pRFD)),
	   (char *) pMblk->mBlkHdr.mData, 
	   (count & ~0xc000));

fei82557PollReceiveEnd:

    /* put the used RFD on the RFD queue */

    fei82557RFDReturn(pDrvCtrl, pRFD);

    DRV_LOG (DRV_DEBUG_POLL_RX, ("fei82557PollReceive... return \n"), 
				 0, 0, 0, 0, 0, 0);

    return (retVal);

    }

/*******************************************************************************
* fei82557PollStart - start polling mode
*
* This routine starts polling mode by disabling ethernet interrupts and
* setting the polling flag in the END_CTRL stucture.
*
* RETURNS: OK, always.
*/

LOCAL STATUS fei82557PollStart
    (
    DRV_CTRL    *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int         intLevel;

    DRV_LOG (DRV_DEBUG_POLL, ("fei82557PollStart\n"), 0, 0, 0, 0, 0, 0);

    intLevel = intLock();

    /* disable system interrupt */

    SYS_INT_DISABLE (pDrvCtrl);     

    /* disable chip interrupt   */

    I82557_INT_DISABLE;	  

    FEI_FLAG_SET (FEI_POLLING);

    intUnlock (intLevel);

    return (OK);

    }

/*******************************************************************************
* fei82557PollStop - stop polling mode
*
* This routine stops polling mode by enabling ethernet interrupts and
* resetting the polling flag in the END_CTRL structure.
*
* RETURNS: OK, always.
*/

LOCAL STATUS fei82557PollStop
    (
    DRV_CTRL    *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int         intLevel;

    intLevel = intLock();

    /* enable system interrupt */

    SYS_INT_ENABLE (pDrvCtrl);     

    /* enable chip interrupt   */

    I82557_INT_ENABLE;	  

    /* set flags */

    FEI_FLAG_CLEAR (FEI_POLLING);

    intUnlock (intLevel);

    DRV_LOG (DRV_DEBUG_POLL, ("fei82557PollStop... end\n"), 0, 0, 0, 0, 0, 0);

    return (OK);
    }

/**************************************************************************
*
* fei82557PhyInit - initialize and configure the PHY device if there is one
*
* This routine initialize and configure the PHY device if there is one.
*
* RETURNS: OK or ERROR.
*
*/

LOCAL STATUS fei82557PhyInit
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    int status;
    int phyDevice;

    /* not a MII interface, no need to initialize PHY */

    if (pDrvCtrl->board.phyAddr > 31)
	return (OK);  

    /* configure the Physical layer medium if it's MII interface */

    /* starts with logical PHY 1 */


    phyDevice = (pDrvCtrl->board.phyAddr) ? pDrvCtrl->board.phyAddr : 1;

    status = fei82557MDIPhyLinkSet (pDrvCtrl, phyDevice);

/* removed to allow the port to be inited without link */
#if 0
    if (status != OK)
	{
	phyDevice = 0;
	status = fei82557MDIPhyLinkSet (pDrvCtrl, phyDevice);
	}

    if (status != OK)
	{
	DRV_LOG (DRV_DEBUG_LOAD,
		 ("LINK_FAIL error, check the line connection !!!\n"),
		 0, 0, 0, 0, 0, 0);
	return (status);
	}
#endif

    /* we are here if a valid link is established */

    status = fei82557MDIPhyConfig (pDrvCtrl, phyDevice); 

    if (status == PHY_AUTO_FAIL)
	{
	/* force default speed and duplex */

	pDrvCtrl->board.phySpeed = PHY_10MBS;
	pDrvCtrl->board.phyDpx = PHY_HALF_DPX; 
	pDrvCtrl->board.others = 0;

	/* and configure it again */

	status = fei82557MDIPhyConfig (pDrvCtrl, phyDevice);  
	}

    return (status);
    } 
/**************************************************************************
*
* fei82557MDIPhyLinkSet - detect and set the link for the PHY device
*
* This routine first checks if the link has been established.  If not, it
* isolates the other one, and tries to establish the link.  PHY device 0 is
* always at PHY address 0.  PHY 1 can be at 1-31 PHY addresses.
*
* RETURNS: OK or ERROR.
*
*/

LOCAL STATUS fei82557MDIPhyLinkSet
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int phyAddr			/* physical address */
    )

    {
    UINT16 ctlReg;
    UINT16 statusReg;
    int isoDev, i;

    /* read control register */

    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, phyAddr, &ctlReg);

    /* read again */

    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, phyAddr, &ctlReg);

    /* check if the PHY is there */

    if (ctlReg == (UINT16)0xffff)
	return (ERROR);   /* no PHY present */

    /* The PHY is there, read status register  */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* in case the status bit is the latched bit */

    if ( !(statusReg & MDI_SR_LINK_STATUS))
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    if (statusReg & MDI_SR_LINK_STATUS)
	return (OK);  /* Device found and link OK */

    /* no link is established, let's configure it */

    /* isolates the other PHY */

    isoDev = (phyAddr) ? 0 : 1;
    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, isoDev, MDI_CR_ISOLATE);

    /* wait for a while */

    taskDelay (2);

    /* enable the PHY device we try to configure */

    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, phyAddr, MDI_CR_SELECT);

    /* wait for a while for command take effect */

    taskDelay (2); 

    /* restart the auto negotiation process, execute anyways even if
     * it has no such capability.
     */

    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, phyAddr, 
		     MDI_CR_RESTART | MDI_CR_SELECT);

    /* wait for auto-negotiation to complete */

    for (i = 0; i < 80; i++)
	{
	/* read the status register */

	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

	if (statusReg & (MDI_SR_AUTO_NEG | MDI_SR_REMOTE_FAULT) )
	    break;

	taskDelay (2);

	if (!(statusReg & MDI_SR_AUTO_SELECT))
	    break;  /* no such capability */
	}

    /* Read the status register */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* some of the status bits require to clear a latch */

    if (!(statusReg & MDI_SR_LINK_STATUS))
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    if (statusReg & MDI_SR_LINK_STATUS)
	return (OK);  /* Link configure done and successful */

    /* device is there, cann't establish link */

    return (PHY_LINK_ERROR);   

     return (OK);
    } 

/**************************************************************************
*
* fei82557MDIPhyConfig - configure the PHY device
*
* This routine configures the PHY device according to the parameters
* specified by users or the default value.
*
* RETURNS: OK or ERROR.
*
*/

LOCAL STATUS fei82557MDIPhyConfig
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int phyAddr
    )

    {
    UINT16 ctlReg = 0;
    int fullDpx=FALSE;
    int autoSelect =FALSE;
    UINT16 statusReg;
    int status, i;

    /* find out what capabilities the device has */

    /*  read status register  */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* some of the status bits require to read twice */

    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

    /* The device at least has to have the half duplex and 10mb speed */

    if (statusReg & (MDI_SR_10T_FULL_DPX | MDI_SR_TX_FULL_DPX))
	fullDpx = TRUE;

    if (statusReg & MDI_SR_AUTO_SELECT)
	autoSelect = TRUE;

    DRV_LOG (DRV_DEBUG_LOAD, ("status REG = %x !!!! \n"),
				statusReg, 0, 0, 0, 0, 0);

    if (pDrvCtrl->board.phyDpx == PHY_FULL_DPX && fullDpx == TRUE)
	ctlReg |= MDI_CR_FDX;

    if (pDrvCtrl->board.phySpeed == PHY_100MBS)
	ctlReg |= MDI_CR_100;

    if (pDrvCtrl->board.phySpeed == PHY_AUTO_SPEED || 
	pDrvCtrl->board.phyDpx == PHY_AUTO_DPX )
	{
	if (autoSelect != TRUE)
	    {
	    /* set back to default */

	    pDrvCtrl->board.phySpeed = PHY_10MBS;
	    pDrvCtrl->board.phyDpx = PHY_HALF_DPX;
	    ctlReg |= (PHY_10MBS | PHY_HALF_DPX);
	    }
	else
	    {
	    ctlReg |= (MDI_CR_SELECT | MDI_CR_RESTART);
	    }
	 }

    /* or other possible board level selection */

    ctlReg |= pDrvCtrl->board.others;

    /* configure the PHY */

    fei82557MDIWrite (pDrvCtrl, MDI_CTRL_REG, phyAddr, ctlReg);
   
    /* wait for a while */

    taskDelay (2);   

    if (!(ctlReg & MDI_CR_RESTART))
	return (OK);

    /* we are here if the restart auto negotiation is selected */

    DRV_LOG (DRV_DEBUG_LOAD, ("auto NEGOTIATION STARTS !!!! \n"),
			      0, 0, 0, 0, 0, 0);

    /* wait for it done */

    for (status = PHY_AUTO_FAIL, i = 0; i < 80; i++)
	{
	/* read status register, first read clears */

	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);
	fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, phyAddr, &statusReg);

	if (statusReg & MDI_SR_AUTO_NEG)
	    {
	    status = OK;  /* auto negotiation completed */
	    break;
	    }

	if (statusReg & MDI_SR_REMOTE_FAULT)
	    {
	    status = PHY_AUTO_FAIL;  /* auto negotiation fails */
	    break;
	    }
	}
	      
	return (status);
} 

/**************************************************************************
*
* fei82557MDIRead - read the MDI register
*
* This routine reads the specific register in the PHY device
* Valid PHY address is 0-31
*
* RETURNS: OK or ERROR.
*
*/

static int fei82557MDIRead
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int regAddr,
    int phyAddr,
    UINT16 *retVal
    )

    {

    int			i;
    volatile UINT32	mdrValue;

    mdrValue = ((regAddr << 16) | (phyAddr << 21) | (MDI_READ << 26));

    /* write to MDI it was done differently!! */

    CSR_LONG_WR (CSR_MDI_OFFSET, mdrValue);

    /* wait for a while */

    taskDelay (2);    

    /* check if it's done */

    for (i = 0; i < 40; i++)
	{
	CSR_LONG_RD (CSR_MDI_OFFSET, mdrValue);

	/* check for correct completion */

	if (mdrValue & (1 << 28))
	    {
	    break;
	    }

	taskDelay (1);
	}

     if (i==40)
	 return (ERROR);

     *retVal = (UINT16) (mdrValue & 0xffff);

     return (OK);

     }

/**************************************************************************
*
* fei82557MDIWrite - write to the MDI register
*
* This routine writes the specific register in the PHY device
* Valid PHY address is 0-31
*
* RETURNS: OK or ERROR.
*/

static int fei82557MDIWrite
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int regAddr,
    int phyAddr,
    UINT16 writeData
    )

    {

    int			i;
    volatile UINT32	mdrValue;

    mdrValue = ((regAddr << 16) | (phyAddr << 21) | 
		(MDI_WRITE << 26) | writeData);

    /* write to MDI it was done differently!! */

    CSR_LONG_WR (CSR_MDI_OFFSET, mdrValue);

    /* wait for a while */

    taskDelay (2);    

    /* check if it's done */

    for (i = 0; i < 40; i++)
	{
	CSR_LONG_RD (CSR_MDI_OFFSET, mdrValue);

	/* check for correct completion */

	if (mdrValue & (1 << 28))
	    {
	    break;
	    }

	taskDelay (1);
	}

     if (i==40)
	 return (ERROR);

     return (OK);

     }

/**************************************************************************
*
* fei82557NOP - format and issue a NOP command
*
* RETURNS: OK, or ERROR if the NOP command failed.
*
*/

LOCAL STATUS fei82557NOP
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )

    {
    if (!(fei82557Action (pDrvCtrl, CFD_C_NOP) & CFD_S_OK))
	return (ERROR);

    return (OK);
    } 

/******************************************************************************
*
* fei82557DumpPrint - Display statistical counters
*
* This routine displays i82557 statistical counters
*
* RETURNS: OK, or ERROR if the DUMP command failed.
*/

LOCAL STATUS fei82557DumpPrint
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    UINT32      dumpArray [18];
    UINT32 *    pDump;
    STATUS status = OK;

    /* dump area must be long-word allign */


    pDump = (UINT32 *) (((UINT32) dumpArray + 4) & 0xfffffffc);

    status = fei82557ErrCounterDump (pDrvCtrl, pDump);

    DRV_LOG (DRV_DEBUG_DUMP, ("\n"), 0, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx good frames:	     %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx MAXCOL errors:	   %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx LATECOL errors:	  %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx underrun errors:	 %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx lost CRS errors:	 %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx deferred:		%d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx single collisions:       %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx multiple collisions:     %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Tx total collisions:	%d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx good frames:	     %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx CRC errors:	      %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx allignment errors:       %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx resource errors:	 %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx overrun errors:	  %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx collision detect errors: %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    DRV_LOG (DRV_DEBUG_DUMP, ("Rx short frame errors:      %d\n"),
			      *pDump++, 0, 0, 0, 0, 0);

    return (status);
    }

/*****************************************************************************
*
* fei82557ErrCounterDump - dump statistical counters
*
* This routine dumps statistical counters for the purpose of debugging and
* tuning the 82557.
*
* The <memAddr> parameter is the pointer to an array of 68 bytes in the
* local memory.  This memory region must be allocated before this routine is
* called.  The memory space must also be DWORD (4 bytes) aligned.  When the
* last DWORD (4 bytes) is written to a value, 0xa007, it indicates the dump
* command has completed.  To determine the meaning of each statistical
* counter, see the Intel 82557 manual.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS fei82557ErrCounterDump
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UINT32 *	memAddr
    )

    {
    STATUS status = OK;
    int i;

    memAddr[16]=0;    /* make sure the last DWORD is 0 */

    /* tells the 82557 where to write dump data */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CULDDUMP, TRUE, memAddr)
	== ERROR)
	status = ERROR;

    /* issue the dump and reset counter command */

    if (fei82557SCBCommand (pDrvCtrl, SCB_C_CUDUMPRST, 
			   FALSE, (UINT32) 0) 
	== ERROR)
	status = ERROR;

    /* waits for it done */

    for (i = 0; i < 60; i++)
    {
    if (memAddr[16] == (UINT32) 0xa007)
	break;

	taskDelay (2);
    }

    if (i==60)
	status = ERROR;

    return (status);
    }


/***********************************************************************
* Added by Yu zhijun. 2001/11/21
*************************************************************************/
/* Get the local lan (management) port link status */
STATUS feiLinkStatusGet(int *linkstatus)
{
    UINT16 ctlReg;
    UINT16 statusReg;
    DRV_CTRL *pDrvCtrl;
	END_OBJ *pObj = NULL;
	
	pObj = endFindByName(DEV_NAME, 0);
	if (pObj == NULL)
	{
		*linkstatus = 0;
		return OK;
	}

	pDrvCtrl = (DRV_CTRL *)pObj->devObject.pDevice;
	
    /* read control register */
    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, pDrvCtrl->board.phyAddr, &ctlReg);

    /* read again */
    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, pDrvCtrl->board.phyAddr, &ctlReg);

    /* check if the PHY is there */
    if (ctlReg == (UINT16)0xffff)   /* no PHY present */
    {
		*linkstatus = 0;
    	return OK;
    }
    /* The PHY is there, read status register  */
    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, pDrvCtrl->board.phyAddr, &statusReg);

    *linkstatus = (statusReg & MDI_SR_LINK_STATUS) ? 1 : 0;
    return (OK);
}

/* Get the local lan port (management) Dupex mode and speed */
STATUS feiDuplexSpeedGet(int *duplex, int *speed)
{
    UINT16 ctlReg;
    UINT16 statusReg;
	DRV_CTRL *pDrvCtrl;
	END_OBJ *pObj = NULL;
	
	pObj = endFindByName(DEV_NAME, 0);
	if (pObj == NULL)
	{
		*speed = 0;
		*duplex = -1;
		return OK;
	}

	pDrvCtrl = (DRV_CTRL *)pObj->devObject.pDevice;

    /* read control register */
    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, pDrvCtrl->board.phyAddr, &ctlReg);

    /* read again */
    fei82557MDIRead (pDrvCtrl, MDI_CTRL_REG, pDrvCtrl->board.phyAddr, &ctlReg);

    /* check if the PHY is there */
    if (ctlReg == (UINT16)0xffff) /* no PHY present */
    {
		*speed = 0;
		*duplex = -1;
		return OK;
    }
    
    /* The PHY is there, read status register  */
    fei82557MDIRead (pDrvCtrl, MDI_STATUS_REG, pDrvCtrl->board.phyAddr, &statusReg);

    *speed = 0;
    *duplex = -1;
    if (statusReg & MDI_SR_10T_HALF_DPX)
    {
	*speed = 10;
	*duplex = 0;
    }
    else if (statusReg & MDI_SR_10T_FULL_DPX)
    {
	*speed = 10;
	*duplex = 1;
    }
    else if (statusReg & MDI_SR_TX_HALF_DPX)
    {
	*speed = 100;
	*duplex = 0;
    }
    else if (statusReg & MDI_SR_TX_FULL_DPX)
    {
	*speed = 100;
	*duplex = 1;
    }

    return (OK);
}

