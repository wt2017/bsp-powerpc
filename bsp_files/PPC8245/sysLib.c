/* sysLib.c - generic PPC  system-dependent library 
 * Copyright 1998-2000 Wind River Systems, Inc.
 * Copyright 1984-1997 Wind River Systems, Inc. 
 * Copyright 1996,1999 Motorola, Inc. 


modification history
--------------------

01a,02Feb99,My  Copied from Yellowknife platform & modified for Sandpoint
*/

/*
DESCRIPTION
This library provides board-specific routines.

INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

/* includes */

#include "stdio.h"
#include "ctype.h"
#include "vxWorks.h"
#include "vme.h"
#include "memLib.h"
#include "cacheLib.h"
#include "sysLib.h"
#include "config.h"
#include "string.h"
#include "time.h"
#include "intLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "vxLib.h"
#include "tyLib.h"
#include "symLib.h"
#include "rebootLib.h"
#include "arch/ppc/mmu603Lib.h"
#include "arch/ppc/vxPpcLib.h"
#include "private/vmLibP.h"

/*
 * Always include Ethernet driver for onboard ethernet (21143)
 */
#include "PPC8245/fei82557End.c"

#include "ons8245.h"
#include "bspcommon.h"
#include "proj_config.h"

#ifdef INCLUDE_PCI
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h"
#endif /* INCLUDE_PCI */


/* last 5 nibbles are board specific, initialized in sysHwInit */
unsigned char sysFeiEnetAddr[6] = {0x08, 0x00, 0x3e, 0x03, 0x02, 0x02}; 

/* prototype   */
ULONG sys107RegRead( ULONG regNum );
extern void  sys107RegWrite( ULONG regNum, ULONG regVal );
extern ULONG sysEUMBBARRead(ULONG regNum);
void  sysEUMBBARWrite(ULONG regNum, ULONG regVal);
static void m8245PciInit();

/* defines */

#define ZERO	0

/* PCI Slot information */

/* Macro to swap a 16 bit value from big to little endian 
 * and vice versa  
 */

#define BYTE_SWAP_16_BIT(x)    ( (((x) & 0x00ff) << 8) | ( (x) >> 8) )

/* globals */

/*
 * sysBatDesc[] is used to initialize the block address translation (BAT)
 * registers within the PowerPC 603/604 MMU.  BAT hits take precedence
 * over Page Table Entry (PTE) hits and are faster.  Overlap of memory
 * coverage by BATs and PTEs is permitted in cases where either the IBATs
 * or the DBATs do not provide the necessary mapping (PTEs apply to both
 * instruction AND data space, without distinction).
 *
 * The primary means of memory control for VxWorks is the MMU PTE support
 * provided by vmLib and cacheLib.  Use of BAT registers will conflict
 * with vmLib support.  User's may use BAT registers for i/o mapping and
 * other purposes but are cautioned that conflicts with cacheing and mapping
 * through vmLib may arise.  Be aware that memory spaces mapped through a BAT
 * are not mapped by a PTE and any vmLib() or cacheLib() operations on such
 * areas will not be effective, nor will they report any error conditions.
 *
 * Note: BAT registers CANNOT be disabled - they are always active.
 * For example, setting them all to zero will yield four identical data
 * and instruction memory spaces starting at local address zero, each 128KB
 * in size, and each set as write-back and cache-enabled.  Hence, the BAT regs
 * MUST be configured carefully.
 *
 * With this in mind, it is recommended that the BAT registers be used
 * to map LARGE memory areas external to the processor if possible.
 * If not possible, map sections of high RAM and/or PROM space where
 * fine grained control of memory access is not needed.  This has the
 * beneficial effects of reducing PTE table size (8 bytes per 4k page)
 * and increasing the speed of access to the largest possible memory space.
 * Use the PTE table only for memory which needs fine grained (4KB pages)
 * control or which is too small to be mapped by the BAT regs.
 *
 * The BAT configuration for 4xx/6xx-based PPC boards is as follows:
 * All BATs point to PROM/FLASH memory so that end customer may configure
 * them as required.
 *
 * [Ref: chapter 7, PowerPC Microprocessor Family: The Programming Environments]
 */


UINT32 sysBatDesc [2 * (_MMU_NUM_IBAT + _MMU_NUM_DBAT)] =   
{  
	/* I BAT 0,-- disabled */   
	((0 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_128K),    
	((0 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_CACHE_INHIBIT ),    

	/* I BAT 1 -- disabled */   
	((0 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_128K),    
	((0 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_CACHE_INHIBIT),   

	/* I BAT 2 -- disabled */    
	((0 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_128K),    
	((0 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_CACHE_INHIBIT),  

	/* I BAT 3 -- disabled */   
	((0 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_128K),   
	((0 & _MMU_LBAT_BRPN_MASK) |  _MMU_LBAT_CACHE_INHIBIT),   

	/* D BAT 0 */    
	((EUMB_BASE_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_1M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((EUMB_BASE_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT | _MMU_LBAT_GUARDED),  

	/* D BAT 1 - PCI prefetchable memory + non-prefetchable memory  = 256MB */  
	((CPU_PCI_MEM_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_256M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((CPU_PCI_MEM_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT | _MMU_LBAT_GUARDED),  

	/* D BAT 2 - PCI IO space 0xfe000000 to 0xff000000 -- covers the config address and data regs too */    
	((CPU_PCI_IO_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_16M | _MMU_UBAT_VS | _MMU_UBAT_VP),    
	((CPU_PCI_IO_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW  | _MMU_LBAT_CACHE_INHIBIT | _MMU_LBAT_GUARDED),  

	/* D BAT 3 -- disabled */    
	((0 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_128K),   
	((0 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_CACHE_INHIBIT),   
};


/*
 * sysPhysMemDesc[] is used to initialize the Page Table Entry (PTE) array
 * used by the MMU to translate addresses with single page (4k) granularity.
 * PTE memory space should not, in general, overlap BAT memory space but
 * may be allowed if only Data or Instruction access is mapped via BAT.
 *
 * All entries in this table both addresses and lengths must be page aligned.
 *
 * PTEs are held in a Page Table.  Page table sizes are integer powers
 * of two based on amount of memory to be mapped and a minimum size of
 * 64 kbytes.  The MINIMUM recommended page table sizes for 32-bit
 * PowerPCs are:
 *
 * Total mapped memory		Page Table size
 * -------------------		---------------
 *        8 Meg			     64 K
 *       16 Meg			    128 K
 *       32 Meg			    256 K
 *       64 Meg			    512 K
 *      128 Meg			      1 Meg
 * 	...				...
 *
 * [Ref: chapter 7, PowerPC Microprocessor Family: The Programming Env.]
 */

#define PCI_STATE_MASK		(VM_STATE_MASK_VALID | \
				 VM_STATE_MASK_WRITABLE | \
				 VM_STATE_MASK_CACHEABLE | \
				 VM_STATE_MASK_MEM_COHERENCY | \
				 VM_STATE_MASK_GUARDED)

#define PCI_STATE_VAL		(VM_STATE_VALID | \
				 VM_STATE_WRITABLE | \
				 VM_STATE_CACHEABLE_NOT | \
				 VM_STATE_MEM_COHERENCY | \
				 VM_STATE_GUARDED)

/*
 * Note: 0xfe000000 through the end of memory (includes all of port X space)
 *       are covered by two IBATs.  See sysBatDesc[] above.
 */

PHYS_MEM_DESC sysPhysMemDesc [] =
    {
    /* Vector Table and Interrupt Stack */
    {
    (ABS_ADDR_TYPE) LOCAL_MEM_LOCAL_ADRS,
    (ABS_ADDR_TYPE) LOCAL_MEM_LOCAL_ADRS,
    LOCAL_MEM_SIZE, /*1024*1024*128*/
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE
    },  
    {
    /* 8-bit ROM/FLASH RCS 0,TFFS and BIOS area */
    (ABS_ADDR_TYPE) FLASH_BASE_ADRS,
    (ABS_ADDR_TYPE) FLASH_BASE_ADRS,
    FLASH_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT  | VM_STATE_GUARDED
    },
    
    {
    /* 32-bit extern ROM/FLASH RCS 2*/
    (ABS_ADDR_TYPE) 0x74000000,
    (ABS_ADDR_TYPE) 0x74000000,
    0x2000000,/*32M*/
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT  | VM_STATE_GUARDED
    },

    {
    /* 32-bit extern ROM/FLASH RCS 3*/
    (ABS_ADDR_TYPE) 0x7C000000,
    (ABS_ADDR_TYPE) 0x7C000000,
    0x2000000,/*32M*/
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT  | VM_STATE_GUARDED
    },
};

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

int   sysCpu      = CPU;                /* system CPU type (MC680x0) */
char *sysBootLine = BOOT_LINE_ADRS;	/* address of boot line */

char *sysExcMsg   = EXC_MSG_ADRS;	/* catastrophic message area */
int   sysProcNum = 0;			/* processor number of this CPU */
int   sysFlags;				/* boot flags */
char  sysBootHost [BOOT_FIELD_LEN];	/* name of host from which we booted */
char  sysBootFile [BOOT_FIELD_LEN];	/* name of file from which we booted */
int   sysVectorIRQ0 = INT_VEC_IRQ0;

unsigned long  sysPciConfAddr;
unsigned long  sysPciConfData;


/* locals */

LOCAL char sysModelStr[64];

#include "i8250Sio.c"

#include "epic.c"

#include "mpc8245I2c.c"

#include "bspdriver.c"

#include "bspChassis.c"

/*img软件加载及上层软件升级模块*/
#include "biosmain.c" 

#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)

#include "ftpd6lib.c"
#else

#include "ftpdlib.c"
#endif


#if  defined(INCLUDE_PCI)
#include "pci/pciConfigLib.c"        /* pci configuration library */ 
  /* use show routine */
#ifdef INCLUDE_SHOW_ROUTINES
#include "pci/pciConfigShow.c"       /* pci configuration show routines */
#endif
#endif /* defined(INCLUDE_PCI)*/

/* BSP DRIVERS */
#include "mem/nullNvRam.c"
#define INCLUDE_TIMESTAMP /* Include timeStamp driver for timing */
/* See ppcDecTimer.c for more info on decrement timer */
#include "../../src/drv/timer/ppcDecTimer.c"

/*
 * Aux clock routines utilizing EPIC in MPC8240
 */
LOCAL 		int sysAuxClkConnected	= FALSE;
FUNCPTR 	sysAuxClkRoutine;
int 		sysAuxClkArg;
int 		sysAuxClkTicksPerSecond;
int 		sysAuxClkFrequency;

void sysAuxClkInt(void)
{
    if (sysAuxClkRoutine)
		(*(FUNCPTR) sysAuxClkRoutine) (sysClkArg);
}

STATUS sysAuxClkConnect(FUNCPTR routine, int arg)
{

   if (!sysAuxClkConnected &&
        (intConnect(INUM_TO_IVEC(EPIC_VECTOR_TM0), sysAuxClkInt, 0) == ERROR))
        {
        return (ERROR);
        }

    sysAuxClkConnected = TRUE;
    sysAuxClkRoutine   = routine;
    sysAuxClkArg       = arg;

    return OK;
}

STATUS sysAuxClkRateSet(int ticksPerSecond)
{
    UINT32		reg;

    if (ticksPerSecond < AUX_CLK_RATE_MIN || ticksPerSecond > AUX_CLK_RATE_MAX)
        return ERROR;

    sysAuxClkFrequency = 100000000/8;
    sysAuxClkTicksPerSecond = ticksPerSecond;

    /*
     * Set clock rate to default just in case, which is 1/8 of the timebase
     * divider input frequency.  The PPC timebase counter itself operates
     * at 1/4 the divider input frequency (twice the aux clock).
     */

    reg = sysEUMBBARRead(EPIC_INT_CONF_REG);
    reg &= ~0x70000000;
    reg |=  0x40000000;
    sysEUMBBARWrite(EPIC_INT_CONF_REG, reg);

    sysEUMBBARWrite(EPIC_TM0_BASE_COUNT_REG,
		    sysAuxClkFrequency / sysAuxClkTicksPerSecond);

    return OK;
}

int sysAuxClkRateGet(void)
{
    return sysAuxClkTicksPerSecond;
}

void sysAuxClkEnable(void)
{
    intEnable(EPIC_VECTOR_TM0);
}

void sysAuxClkDisable(void)
{
    intDisable(EPIC_VECTOR_TM0);
}

int isMPC8245(void)
{
	int retVal;
	retVal = sys107RegRead(BMC_BASE+8);	/*Motorola's RevisionID*/
	/*  judge the CPU type
	
	Offset     Register Name Description
        
        0x00       Vendor ID(0x1057 = Motorola)
        0x02       Device ID(0x0006 = MPC8245).
        0x04       PCI command Provides
        0x06       PCI status Records status information
        0x08       Revision ID revision code (assigned by Motorola)
                   0x12 = 8241,0x14 = 8245
        0x09       Standard programming interface Identifies of the MPC8245(0x00)        
        0x0A       Subclass code Identifies of the MPC8245(0x00)
        0x0B       Base class code of MPC8245 performs
                   (Host mode = 0x06 bridge device, Agent mode = 0x0E intelligent I/O controller)
	MPC8241 = 0x06000012
	MPC8245 = 0x06000014
	*/
	return ((retVal==0x6000012) ? 0 : 1);
}

/******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.  The returned string
* depends on the board model and CPU version being used, for example,
* Motorola YellowKnife
*
* RETURNS: A pointer to the string.
*/
char * sysModel (void)
    {
    sprintf (sysModelStr, "MPC 8245 --UTstarcome BSP. 8245 Board");
    return (sysModelStr);
    }


/*****************************************************************************
*
* sysBspRev - return the BSP version and revision number
*
* This routine returns a pointer to a BSP version and revision number, for
* example, 1.1/0. BSP_REV is concatenated to BSP_VERSION and returned.
*
* RETURNS: A pointer to the BSP version/revision string.
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION);
    }


/*****************************************************************************
 * Network support routines
 */

STATUS sysLanIntEnable(int intLevel)
{
    intEnable(intLevel);
    return OK;
}

STATUS sysLanIntDisable(int intLevel)
{
    intDisable(intLevel);
    return OK;
}

STATUS sysEnetAddrGet(int unit, unsigned char *mac)
{
    bcopy ((char *) sysFeiEnetAddr, (char *) mac, sizeof (sysFeiEnetAddr));
 
    return (OK);
}    

#define SCB_STATUS  0x00            /* SCB status byte */
#define BIT_9       0x0200
#define BIT_12_15   0xF000


int func_Intel82559IntAck()
{
	short *temp,val;
	temp =(short *)(PCI_ENET_MEMADDR + SCB_STATUS) ;
	
	/* ACK interrupt */
	val =sysInWord((int)temp);
	sysOutWord((int)temp,val & (BIT_9 | BIT_12_15));  /* ACK mask bit 9,12~15,in LAN(Ethernet) Control/Status Registers */ 
        return 0;       
}

STATUS sys557Init (int unit, FEI_BOARD_INFO *pBoard)
{
    int BusNo;
    int DevNo;
    int FuncNo;
    int devConfigCommand = (PCI_CMD_IO_ENABLE  |
			    PCI_CMD_MEM_ENABLE |
			    PCI_CMD_MASTER_ENABLE);
    int intLine;

    /* Only one unit is supported for now  */

    #define PCI_82551_VENDOR_ID1	        0x8086
    #define PCI_82551ER_DEVICE_ID	0x1209	/* 82551ER:0x1209  82559ER:0x1229 */

    if(pciFindDevice(PCI_82551_VENDOR_ID1, PCI_82551ER_DEVICE_ID, 0,
			     &BusNo, &DevNo, &FuncNo) != ERROR)
	{
	/* Found Intel 82559ER/82551 Ethernet Device */
	pciDevConfig(BusNo, DevNo, FuncNo,
		     PCI_ENET_IOADDR,
		     PCI_ENET_MEMADDR,
		     devConfigCommand);
	} 
   else 
        {
	logMsg("ERROR: onboard Intel 82551ER not detected.\n\n",0,0,0,0,0,0);
	return ERROR;
        }

    switch (DevNo) {
	    case MOUSSE_IDSEL_ENET:	/* On-board 82559ER */
		intLine = MOUSSE_IRQ_ENET;
		break;
	    default:			/* Some other chassis slot */
		intLine = MOUSSE_IRQ_LPCI;
		break;
    }

    pciConfigOutByte(BusNo,DevNo,FuncNo,PCI_CFG_DEV_INT_LINE,intLine);
   
   	pBoard->vector = intLine;
   	pBoard->baseAddr = PCI_ENET_MEMADDR;

   	pBoard->enetAddr[0]= sysFeiEnetAddr[0];
   	pBoard->enetAddr[1]= sysFeiEnetAddr[1];
   	pBoard->enetAddr[2]= sysFeiEnetAddr[2];
   	pBoard->enetAddr[3]= sysFeiEnetAddr[3];
   	pBoard->enetAddr[4]= sysFeiEnetAddr[4];
   	pBoard->enetAddr[5]= sysFeiEnetAddr[5]; 
   	
   	pBoard->intEnable= sysLanIntEnable;
   	pBoard->intDisable= sysLanIntDisable;
   	pBoard->intAck	  = func_Intel82559IntAck;
 
#if 0   	
   	logMsg("Intel 82559 Ethernet adaptor found!\n\n",0,0,0,0,0,0);
#endif
 
    return OK;
}


/*****************************************************************************
*
* i just use this function to change the MAC address of the "fei0" interface.
*
*/
unsigned char bsp_default_fei_enet_addr[6]  = {0x08, 0xC0, 0xA8, 0x00, 0xFE, 0x01};
unsigned long bsp_default_fei_if_addr = 0xC0A8d2FE;	/* 192.168.210.254 */
unsigned long bsp_default_fei_if_mask = 0xFFFFFF00;


int bspSetMacAdrs(void)
{
    extern	 unsigned char bsp_get_chss_pos(void) ;
    unsigned char chssPos;

    chssPos = bsp_get_chss_pos();
    sysFeiEnetAddr[0] = 0x80;
    sysFeiEnetAddr[1] = 0xC0;
    sysFeiEnetAddr[2] = 0xA8;
    sysFeiEnetAddr[3] = 0xD2;
    sysFeiEnetAddr[4] = chssPos;
    sysFeiEnetAddr[5] = 0x01;

    return 0;
}

UINT32 g_sysPhysMemSize = BCTL_SDRAM_128M_SIZE ;

LOCAL void i8250SerialHwInit()
{
#define M8245_UART_DCR_REG     (EUMB_BASE_ADRS+ 0x4511)	
	*((char*)M8245_UART_DCR_REG) = 0x1 ;  /*2 line mode for UART1 , UART2  UNUSE*/	
	sysSerialHwInit();
}


/*****************************************************************************
*
* sysHwInit - initialize the system hardware
*
* This routine initializes various features of the board.
* It is the first board-specific C code executed, and runs with
* interrupts masked in the processor.
* This routine resets all devices to a quiescent state, typically
* by calling driver initialization routines.
*
* NOTE: Because this routine will be called from sysToMonitor, it must
* shutdown all potential DMA master devices.  If a device is doing DMA
* at reboot time, the DMA could interfere with the reboot. For devices
* on non-local busses, this is easy if the bus reset or sysFail line can
* be asserted.
*
* NOTE: This routine should not be called directly by the user application.
*
* RETURNS: N/A
*/

void sysHwInit (void)
	{
	int mechanism = 1;			/* PCI mechanism - My 6/98 */
	ULONG retVal;

	sysPciConfAddr = CHRP_REG_ADDR;
	sysPciConfData = CHRP_REG_DATA;
	
	retVal = sys107RegRead(BMC_BASE);   /* Read Device & Vendor Ids */
	       
	/* Compare with expected value */
	if  (retVal != VEN_DEV_ID) 
		{
			for(;;)	;
		}

	/*
	 * Initialize the EUMBBAR (Embedded Util Mem Block Base Addr Reg).
	 * This is necessary before the EPIC, DMA ctlr, I2C ctlr, etc. can
	 * be accessed.
	 */
	sys107RegWrite(EUMBBAR_REG, EUMB_BASE_ADRS);

	/* set ethernet ip */
	bspSetMacAdrs();

	/*BOOT LINE initialize */
	strcpy (BOOT_LINE_ADRS, DEFAULT_BOOT_LINE);

	/*
	 * Initialize EPIC 
	 */
	epic_init();

	i8250SerialHwInit ();		/* serial devices */

#ifdef INCLUDE_NETWORK
	/* sysNetHwInit (); */	/* network interface */
#endif
	        
	        
#ifdef INCLUDE_PCI
	
	pciConfigLibInit (mechanism, sysPciConfAddr, sysPciConfData, 0x0);
	m8245PciInit();
#endif

	g_sysPhysMemSize = BCTL_SDRAM_128M_SIZE ;

	}

/*******************************************************************************
*
* sysHwInit2 - initialize additional system hardware
*
* This routine connects system interrupt vectors and configures any
* required features not configured by sysHwInit. It is called from usrRoot()
* in usrConfig.c after the multitasking kernel has started. NOT!
*
* RETURNS: N/A
*/

void sysHwInit2 (void)
{
    static int	initialized;		/* must protect against double call! */

    if (initialized)
	return;

    initialized = TRUE;

    /*	sysPciIntRoute();   set up PCI INTA,B,C,d -> IRQ9,10,11,12 */

    sysSerialHwInit2();		/* connect serial interrupts */

//   usrClockInit(100); /* 1 interrupt every 100 seconds */

#ifdef INCLUDE_NETWORK
    /* sysNetHwInit2 ();	 network interface */
#endif

#ifdef INCLUDE_PCI
    /* TODO - any secondary PCI setup */
#endif

}

static void m8245PciInit()
	{
	/*PCI Arbiter Control Register*/
	pciConfigOutWord(0, 0,0,0x46,0xc080);
	}


/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* Normally, the user specifies the amount of physical memory with the
* macro LOCAL_MEM_SIZE in config.h.  BSPs that support run-time
* memory sizing do so only if the macro LOCAL_MEM_AUTOSIZE is defined.
* If not defined, then LOCAL_MEM_SIZE is assumed to be, and must be, the
* true size of physical memory.
*
* NOTE: Do no adjust LOCAL_MEM_SIZE to reserve memory for application
* use.  See sysMemTop() for more information on reserving memory.
*
* RETURNS: The address of the top of physical memory.
*
* SEE ALSO: sysMemTop()
*/

char * sysPhysMemTop (void)
    {
    static char * physTop = NULL;

    if (physTop == NULL)
	{
#ifdef LOCAL_MEM_AUTOSIZE

#	error	"Dynamic memory sizing not supported"

#else
	/* ONS 8245 has 128MB SDRAM */
	physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);

#endif /* LOCAL_MEM_AUTOSIZE */
	}

    return physTop;
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of VxWorks memory
*
* This routine returns a pointer to the first byte of memory not
* controlled or used by VxWorks.
*
* The user can reserve memory space by defining the macro USER_RESERVED_MEM
* in config.h.  This routine returns the address of the reserved memory
* area.  The value of USER_RESERVED_MEM is in bytes.
*
* RETURNS: The address of the top of VxWorks memory.
*/

char * sysMemTop (void)
    {
    static char * memTop = NULL;

    if (memTop == NULL)
	{
	memTop = sysPhysMemTop () - USER_RESERVED_MEM;
	}

    return memTop;
    }

#define SYSBT_MAX_FUNC_WORDS	4096
/******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  Normally, it is called
* only by reboot()--which services ^X--and by bus errors at interrupt level.
* However, in some circumstances, the user may wish to introduce a
* <startType> to enable special boot ROM facilities.
*
* The entry point for a warm boot is defined by the macro ROM_WARM_ADRS
* in config.h.  We do an absolute jump to this address to enter the
* ROM code.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
    int startType    /* parameter passed to ROM to tell it how to boot */
    )
    {
    FUNCPTR pRom = (FUNCPTR) (ROM_WARM_ADRS);
    logMsg("sysToMonitor , jump to 0x%08x\n" , ROM_WARM_ADRS ,2,3,4,5,6);
    taskDelay(5);
	
    intLock ();			/* disable interrupts */

    cacheDisable (INSTRUCTION_CACHE);  	/* Disable the Instruction Cache */
    cacheDisable (DATA_CACHE);   	/* Disable the Data Cache */

    sysHwInit ();		/* disable all sub systems to a quiet state */

    vxMsrSet (0);		/* Clear the MSR */
    (*pRom) (startType);	/* jump to romInit.s */
   
    return (OK);		/* in case we continue from ROM monitor */
    }

/******************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for this CPU board.
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)
    {
    return (sysProcNum); 
    }

/******************************************************************************
*
* sysProcNumSet - set the processor number
*
* This routine sets the processor number for the CPU board.  Processor numbers
* should be unique on a single backplane.
*
* For bus systems, it is assumes that processor 0 is the bus master and
* exports its memory to the bus.
*
* RETURNS: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum			/* processor number */
    )
    {
    sysProcNum = procNum;

    if (procNum == 0)
        {

#ifdef INCLUDE_PCI
	/* TODO - Enable/Initialize the interface as bus slave */
#endif
	}
    }


/************************************************************************ 
 *  This procedure reads a 32-bit address MPC107 register, and returns   
 *  a 32 bit value.  It swaps the address to little endian before        
 *  writing it to config address, and swaps the value to big endian      
 *  before returning to the caller.     
 */

ULONG sys107RegRead(regNum)
ULONG regNum;
{
    ULONG temp;

    /* swap the addr. to little endian */
    *(ULONG *)sysPciConfAddr = PCISWAP(regNum);
    temp = *(ULONG *)sysPciConfData;
    return ( PCISWAP(temp));		      /* swap the data upon return */
}
/************************************************************************
 *	This procedure writes a 32-bit address MPC107 register, and returns   
 *	a 32 bit value.  It swaps the address to little endian before		 
 *	writing it to config address, and swaps the value to big endian 	 
 *	before returning to the caller. 	
 */

void sys107RegWrite(regNum, regVal)
ULONG regNum;
ULONG regVal;
{
	/* swap the addr. to little endian */
	*(ULONG *)sysPciConfAddr = PCISWAP(regNum);
	*(ULONG *)sysPciConfData = PCISWAP(regVal);
	return;
}


/*******************************************************************
 *  Read a register in the Embedded Utilities Memory Block address
 *  space.  
 *  Input: regNum - register number + utility base address.  Example, 
 *         the base address of EPIC is 0x40000, the register number
 *	   being passed is 0x40000+the address of the target register.
 *	   (See epic.h for register addresses).
 *  Output:  The 32 bit little endian value of the register.
 */

ULONG sysEUMBBARRead(ULONG regNum)
{
  ULONG temp;

  temp = *(ULONG *) (EUMB_BASE_ADRS + regNum) ;
  temp = PCISWAP(temp);

  return (temp);
}

	
/*******************************************************************
 *  Write a value to a register in the Embedded Utilities Memory 
 *  Block address space.
 *  Input: regNum - register number + utility base address.  Example, 
 *                  the base address of EPIC is 0x40000, the register 
 *	            number is 0x40000+the address of the target register.
 *	            (See epic.h for register addresses).
 *         regVal - value to be written to the register.
 */

void sysEUMBBARWrite(ULONG regNum, ULONG regVal)
{  
  *(ULONG *) (EUMB_BASE_ADRS + regNum) = PCISWAP(regVal);
  return ;
}

#ifdef INCLUDE_TFFS
	
/*******************************************************************************
*
* sysTffsFormat - format the flash memory above an offset
*
* This routine formats the flash memory.  Because this function defines 
* the symbolic constant, HALF_FORMAT, the lower half of the specified flash 
* memory is left unformatted.  If the lower half of the flash memory was
* previously formated by TrueFFS, and you are trying to format the upper half,
* you need to erase the lower half of the flash memory before you format the
* upper half.  To do this, you could use:
* .CS
* tffsRawio(0, 3, 0, 8)  
* .CE
* The first argument in the tffsRawio() command shown above is the TrueFFS 
* drive number, 0.	The second argument, 3, is the function number (also 
* known as TFFS_PHYSICAL_ERASE).  The third argument, 0, specifies the unit 
* number of the first erase unit you want to erase.  The fourth argument, 8,
* specifies how many erase units you want to erase.  
*
* RETURNS: OK, or ERROR if it fails.
*/
#include "tffs/tffsDrv.h"

STATUS sysTffsFormat (int tffsNo)
{
	STATUS status = OK ;
	tffsDevFormatParams params = 
	{

		{0x00000, 99, 1, 0x20000, NULL, {0,0,0,0}, NULL, 2, 0, NULL},
		FTL_FORMAT_IF_NEEDED
	};	

	FLSocket *vol = flSocketOf (tffsNo);		
	/*flash 的最高1M地址保留出来存放bios ，所以减少1M size*/	
	flSetWindowSize(vol , (FLASH_SIZE-0x100000)>>12);	
	status = tffsDevFormat (tffsNo, (int)&params);
	
	if (status != 0)
	{
		printf("flash dev(tffs %d) format error! , status = 0x%x\n" , tffsNo , status);
		return(status);
	}
	printf("flash dev(tffs %d) format success!\n" , tffsNo);
	return (status);
}

/*flash  WP pin protect enalbe(Assert)*/
void flash_write_enable(int chipno)
{
	/*on this 8245 board,flash WP always be enable by hardware designed (Assert), nothing need to do...*/
}
/*flash WP pin protect diable(De-Assert)*/
void flash_write_disable(int chipno)
{
	/*on this 8245 board,flash WP always be enable by hardware designed (Assert), nothing need to do...*/	
}		
#endif  /* INCLUDE_TFFS*/

/***************************************************************************
*
* bsp_cycle_delay() - delay for the specified amount of time (MS)
*
* This routine will delay for the specified amount of time by counting
* decrementer ticks.
*
* This routine is not dependent on a particular rollover value for
* the decrementer, it should work no matter what the rollover
* value is.
*
* A small amount of count may be lost at the rollover point resulting in
* the bsp_cycle_delay() causing a slightly longer delay than requested.
*
* This routine will produce incorrect results if the delay time requested
* requires a count larger than 0xffffffff to hold the decrementer
* elapsed tick count.  For a System Bus Speed of 67 MHZ this amounts to
* about 258 seconds.
*
* RETURNS: N/A
*/
#define MPC82xx_DELTA(a,b)              (abs((int)a - (int)b))
void bsp_cycle_delay
    (
    UINT        delay            /* length of time in MS to delay */
    )
    {
    register UINT32 	oldval;      /* decrementer value */
    register UINT32 	newval;      /* decrementer value */
    register UINT32 	totalDelta;  /* Dec. delta for entire delay period */
    register UINT32 	decElapsed;  /* cumulative decrementer ticks */

    /* Calculate delta of decrementer ticks for desired elapsed time. */

    totalDelta = ((DEC_CLOCK_FREQ / 4) / 1000) * delay;

    /*
     * Now keep grabbing decrementer value and incrementing "decElapsed" until
     * we hit the desired delay value.  Compensate for the fact that we may
     * read the decrementer at 0xffffffff before the interrupt service
     * routine has a chance to set in the rollover value.
     */

    decElapsed = 0;
    oldval = vxDecGet ();
    while (decElapsed < totalDelta)
        {
        newval = vxDecGet();
        if ( MPC82xx_DELTA(oldval,newval) < 1000 )
            decElapsed += MPC82xx_DELTA(oldval,newval);  /* no rollover */
        else
            if (newval > oldval)
                decElapsed += abs((INT32)oldval);  /* rollover */
        oldval = newval;
        }
  }

/*******************************************************************************
*
* sysUsDelay - delay for x microseconds.
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void sysUsDelay
    (
   	unsigned int delay
    )
    {
    register UINT32 oldval;      /* decrementer value */
    register UINT32 newval;      /* decrementer value */
    register UINT32 totalDelta;  /* Dec. delta for entire delay period */

    register UINT32 decElapsed;  /* cumulative decrementer ticks */

    /* Calculate delta of decrementer ticks for desired elapsed time. */
    totalDelta = ((DEC_CLOCK_FREQ / 4) / 1000000) * (UINT32)delay;

    /*
     * Now keep grabbing decrementer value and incrementing "decElapsed"
     * we hit the desired delay value.  Compensate for the fact that we
     * read the decrementer at 0xffffffff before the interrupt service
     * routine has a chance to set in the rollover value.
     */

    decElapsed = 0;
    oldval = vxDecGet();
    while ( decElapsed < totalDelta )
        {
        newval = vxDecGet();
        if ( MPC8245_DELTA(oldval,newval) < 1000 )
            decElapsed += MPC8245_DELTA(oldval,newval);  /* no rollover */
        else if ( newval > oldval )
            decElapsed += abs((int)oldval);  /* rollover */
        oldval = newval;
        }
    }

void sysMsDelay
    (
    UINT      delay                   /* length of time in MS to delay */
    )
    {
    sysUsDelay ( (UINT32) delay * 1000 );
    }
