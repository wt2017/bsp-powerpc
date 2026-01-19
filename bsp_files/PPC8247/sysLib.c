/* sysLib.c - Motorola ads 827x board system-dependent library */

/* Copyright 1984-2003 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,04oct04,dtr  Add support for latest security engine drivers
01g,28jan04,dtr  Use m82xxSio.c from target/src/drv/sio.
01f,08jan04,dtr  Add MMU mapping for security processor.
01e,08dec03,dtr  Modified for ads827x BSP.
01d,08oct03,dee  nvram routines check limits of nvram offset
01c,01oct03,dee  use sysHwInit to adjust mmu tables depending on size of SDRAM
01b,08jan03,dtr  Added support for PCI DMA and Error Handling.
                 Implemented workaround for PCI Bridge read errata.
01w,13jul02,dtr  Add support for PCI.
01v,08mar02,jnz  add support for ads8266
01u,12dec01,jrs  Add BAT table entry to correct mem map.
01t,10dec01,jrs  change copyright date
01s,30nov01,gls  fixed sysBatDesc definition (SPR #20321)
01r,17oct01,jrs  Upgrade to veloce
		 set M8260_SCCR to BRGCLK_DIV_FACTOR,
		 added global variable baudRateGenClk-set by sysBaudClkFreq(),
		 added sysCpmFreqGet() and sysBaudClkFreq() functions,
		 added 8260 Reset Configuration Table - SPR66989
		 changed INCLUDE_MOT_FCC to INCLUDE_MOTFCCEND - SPR #33914
01q,14mar00,ms_  add support for PILOT revision of board
01p,04mar00,mtl  minor changes in macros to be consistent
01o,18oct99,ms_  vxImmrGet must return only bits 14:0 (SPR 28533)
01n,18sep99,ms_  fix comment for ram on 60x bus
01m,16sep99,ms_  sysMotFccEnd.c is local to bsp after all...
01l,16sep99,ms_  fix include files path
01k,16sep99,ms_  some included files come from src/drv instead of locally
01j,16sep99,ms_  get miiLib.h from h/drv instead of locally
01i,13sep99,cn   added sysMiiOptRegsHandle () (SPR# 28305).
01g,08jun99,ms_  remove definition of M8260_SIUMCR that doesn't belong in here
01f,17apr99,ms_  unnesessary setting of MAMR
01e,17apr99,cn   added a temporary fix to initialize the boot line.
01d,17apr99,ms_  final cleanup for EAR
01c,14apr99,cn   added support for motFccEnd
01b,06apr99,ms_  reset the CPM in sysHwInit()
01a,28jan99,ms_  adapted from ads860/sysLib.c version 01j
*/

/*
DESCRIPTION
This library provides board-specific routines.  The chip drivers included are:

SEE ALSO:
.pG "Configuration"
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
#include "tyLib.h"
#include "arch/ppc/mmu603Lib.h"
#include "arch/ppc/vxPpcLib.h"
#include "private/vmLibP.h"
#include "drv/mem/m8260Memc.h"
#include "drv/sio/m8260Cp.h"
#include "drv/parallel/m8260IOPort.h"
#include "drv/timer/m8260Clock.h"
#include "drv/sio/m8260Cp.h"
#include "drv/sio/m8260CpmMux.h"
#include "miiLib.h"
#include "drv/mem/m8260Siu.h" 
#include "PPC8247/m8260IntrCtl.h" 
#include "bspCommon.h"
#include "proj_config.h"

#ifdef INCLUDE_PCI
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h"
#endif /* INCLUDE_PCI */

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
 * All BATs point to PROM/FLASH memory so that end customer may configure
 * them as required.
 *
 * [Ref: chapter 7, PowerPC Microprocessor Family: The Programming Environments]
 */

UINT32 sysBatDesc [2 * (_MMU_NUM_IBAT + _MMU_NUM_DBAT)] =
    {

	/* I BAT 0 */
	0, 0,

	/* I BAT 1 */
	0, 0,

	/* I BAT 2 */
	0, 0,

	/* I BAT 3 */
	0, 0,


    /* D BAT 0 */
	((CS_A_BASE_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_256M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((CS_A_BASE_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT),  

    /* D BAT 1 */
	((CS_B_BASE_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_256M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((CS_B_BASE_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT),  

	/* D BAT 2 */
	((CS_C_BASE_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_256M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((CS_C_BASE_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT),  

	/* D BAT 3 */
	((CS_D_BASE_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_256M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((CS_D_BASE_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT),  
     };


/*
 * sysPhysMemDesc[] is used to initialize the Page Table Entry (PTE) array
 * used by the MMU to translate addresses with single page (4k) granularity.
 * PTE memory space should not, in general, overlap BAT memory space but
 * may be allowed if only Data or Instruction access is mapped via BAT.
 *
 * Address translations for local RAM, the Board Control and Status registers,
 * the MPC8260 Internal Memory Map,  and local FLASH RAM are set here.
 *
 * PTEs are held, strangely enough, in a Page Table.  Page Table sizes are
 * integer powers of two based on amount of memory to be mapped and a
 * minimum size of 64 kbytes.  The MINIMUM recommended Page Table sizes
 * for 32-bit PowerPCs are:
 *
 * Total mapped memory		Page Table size
 * -------------------		---------------
 *        8 Meg			     64 K
 *       16 Meg			    128 K
 *       32 Meg			    256 K
 *       64 Meg			    512 K
 *      128 Meg			      1 Meg
 * 	.				.
 * 	.				.
 * 	.				.
 *
 * [Ref: chapter 7, PowerPC Microprocessor Family: The Programming Environments]
 *
 */
IMPORT char     etext [];               /* defined by the loader */

PHYS_MEM_DESC sysPhysMemDesc [] =
{
	/*some memory use the BAT(block address translation)(xiexy), see sysBatDesc defined previously*/
	{	
	(ABS_ADDR_TYPE)LOCAL_MEM_LOCAL_ADRS ,
	(ABS_ADDR_TYPE)LOCAL_MEM_LOCAL_ADRS,
	(BCTL_SDRAM_128M_SIZE - USER_RESERVED_MEM),	/*1024*1024*96*/
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_MEM_COHERENCY,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE | VM_STATE_MEM_COHERENCY
	},

	{
	/*USER reserved Memory*/	
	(ABS_ADDR_TYPE)(LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE - USER_RESERVED_MEM),
	(ABS_ADDR_TYPE)(LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE - USER_RESERVED_MEM),
	USER_RESERVED_MEM ,/*1024*1024*32*/
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED ,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | VM_STATE_GUARDED
	},

	{
	/*USER reserved Memory*/	
	(ABS_ADDR_TYPE) (FLASH_BASE_ADRS),
	(ABS_ADDR_TYPE) (FLASH_BASE_ADRS),
	FLASH_SIZE ,/*1024*1024*16*/
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | VM_STATE_GUARDED
	},

	{
	/* MPC8260 Internal Memory Map */

	(ABS_ADDR_TYPE) INTERNAL_MEM_MAP_ADDR,
	(ABS_ADDR_TYPE) INTERNAL_MEM_MAP_ADDR,
	INTERNAL_MEM_MAP_SIZE,/*0x20000*/
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT  | VM_STATE_GUARDED
	},

#ifdef INCLUDE_PCI

	{
	(ABS_ADDR_TYPE) PCI_MEM_0_ADRS, 
	(ABS_ADDR_TYPE) PCI_MEM_0_ADRS,
	PCI_MEM_0_SIZE,
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_GUARDED,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT  | VM_STATE_GUARDED
	}, 

#endif  
};


int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

int   sysBus      = BUS;                /* system bus type (VME_BUS, etc) */
int   sysCpu      = CPU;                /* system CPU type (PPC8260) */
char *sysBootLine = BOOT_LINE_ADRS;	/* address of boot line */
char *sysExcMsg   = EXC_MSG_ADRS;	/* catastrophic message area */
int   sysProcNum;			/* processor number of this CPU */
int   sysFlags;				/* boot flags */
char  sysBootHost [BOOT_FIELD_LEN];	/* name of host from which we booted */
char  sysBootFile [BOOT_FIELD_LEN];	/* name of file from which we booted */
BOOL  sysVmeEnable = FALSE;		/* by default no VME */
UINT32   baudRateGenClk;
extern int sysStartType ;



/* forward declarations */

uint32_t sysDecGet(void);
uint32_t sysBaudClkFreq(void);
STATUS sysToMonitor (int startType) ;

#ifdef INCLUDE_PCI
STATUS sysPciSpecialCycle (int busNo, UINT32 message);
STATUS sysPciConfigRead   (int busNo, int deviceNo, int funcNo,
			         int offset, int width, void * pData);
STATUS sysPciConfigWrite  (int busNo, int deviceNo, int funcNo,
			         int offset, int width, ULONG data);

#ifdef PCI_BRIDGE_READ_ERRATA_WORKAROUND
/* Partial Fix for Errata on read access to PCI bridge registers */
/* Uses IDMA2 to access instead of direct read.  */
	UINT8   pciBridgeRegisterReadByte(int);
	UINT16  pciBridgeRegisterReadWord(int);
	UINT32  pciBridgeRegisterReadLong(int);

#	undef  PCI_IN_BYTE
#	undef  PCI_IN_WORD
#	undef  PCI_IN_LONG
#	define PCI_IN_BYTE(x) pciBridgeRegisterReadByte(x)
#	define PCI_IN_WORD(x) pciBridgeRegisterReadWord(x)
#	define PCI_IN_LONG(x) pciBridgeRegisterReadLong(x)
#endif  /* PCI_BRIDGE_READ_ERRATA_WORKAROUND */

ULONG sysPciConfAddr = (PCI_CFG_ADR_REG | INTERNAL_MEM_MAP_ADDR);          /* PCI Configuration Address */
ULONG sysPciConfData = (PCI_CFG_DATA_REG | INTERNAL_MEM_MAP_ADDR);          /* PCI Configuration Data */

/* use pci auto config */
#include "PPC8247/mot8247Pci.c"

#endif /* INCLUDE_PCI */

UINT32 vxImmrGet();

#include "PPC8247/m8260smc.c"

#include "PPC8247/m8260IntrCtl.c"
#include "PPC8247/m8260Timer.c"
#include "mem/nullNvRam.c"

#ifdef  INCLUDE_MOTFCCEND
/* set the following array to a unique Ethernet hardware address */

/* last 5 nibbles are board specific, initialized in sysHwInit */

unsigned char sysFccEnetAddr [2][6] = {{0x08, 0x00, 0x3e, 0x33, 0x02, 0x01},
				       {0x08, 0x00, 0x3e, 0x33, 0x02, 0x02}};
STATUS sysFccEnetAddrGet (int unit, UCHAR * address);

#endif  /* INCLUDE_MOTFCCEND */

/* locals */
STATUS sysFccEnetDisable (UINT32 immrVal, UINT8 fccNum);


#include "PPC8247/m8260i2c.c" 

#include "PPC8247/m8260spi.c" 

#include "bspDriver.c" 

#include "bspChassis.c" 

/*img软件加载及上层软件升级模块*/
#include "biosmain.c" 

#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)

#include "ftpd6lib.c"
#else

#include "ftpdlib.c"
#endif

/*CPU 软件加载the CPLD 模块*/
#include "hardware.c"
#include "ivm_core.c"
#include "ispvm_ui.c"


/* defines */

#define ZERO    0
#define SYS_MODEL       "UTstarcom MPC8247"
#define SYS_MODEL_HIP4  "UTstarcom MPC8247  - HIP4"
#define SYS_MODEL_HIP7  "UTstarcom MPC8247  - HIP7"

/* needed to enable timer base */
#ifdef INCLUDE_PCI
/*depend on hardware design ,bits (4,5) should be (0,0) ,bit0 should be(1) */
#define      M8260_DPPC_MASK	0x0C000000 /* bits 4 and 5 */
#define      M8260_DPPC_VALUE 0x80000000    /* bit 0*/
#else
#define      M8260_DPPC_MASK	0x0C000000 /* bits 4 and 5 */
#define      M8260_DPPC_VALUE	0x08000000 /* bits (4,5) should be (1,0) */
#endif /*INCLUDE_PCI */

#ifdef  INCLUDE_MOTFCCEND
#include "sysMotFccEnd.c"
#endif /* INCLUDE_MOTFCCEND */


#ifdef PCI_BRIDGE_READ_ERRATA_WORKAROUND
UCHAR	sysInByte(ULONG port)
{
	return(pciBridgeRegisterReadByte(port));
}
USHORT	sysInWord(ULONG port)
{
	return(pciBridgeRegisterReadWord(port));
}
ULONG	sysInLong(ULONG port)
{
	return(pciBridgeRegisterReadLong(port));
}
#endif /*PCI_BRIDGE_READ_ERRATA_WORKAROUND */

/******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string.
*/

char * sysModel (void)
    {
    UINT device;

    if (((device = vxPvrGet()) & HIP4_MASK) == HIP4_ID)
        return(SYS_MODEL_HIP4);
    if (((device = vxPvrGet()) & HIP4_MASK) == HIP7_ID)
        return(SYS_MODEL_HIP7);

    return (SYS_MODEL);
    }

/******************************************************************************
*
* sysBspRev - return the bsp version with the revision eg 1.0/<x>
*
* This function returns a pointer to a bsp version with the revision.
* for eg. 1.0/<x>. BSP_REV defined in config.h is concatanated to
* BSP_VERSION and returned.
*
* RETURNS: A pointer to the BSP version/revision string.
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION);
    }

UINT32 g_sysPhysMemSize = BCTL_SDRAM_128M_SIZE ;
static int bsp_physMemDesc_config(void)
{
	g_sysPhysMemSize = *((UINT32 *)(BCTL_SDRAM_SIZE_SAVED_ADDR)) ;/*Get memory size from dpram 0 Address*/
	if( g_sysPhysMemSize == BCTL_SDRAM_256M_SIZE)
		{		
		/*hardware memory 256M to the system os*/
		sysPhysMemDesc[0].len = BCTL_SDRAM_256M_SIZE -USER_RESERVED_MEM ;	
		
		sysPhysMemDesc[1].virtualAddr =(ABS_ADDR_TYPE) (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_256M_SIZE - USER_RESERVED_MEM) ;
		sysPhysMemDesc[1].physicalAddr =(ABS_ADDR_TYPE) (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_256M_SIZE - USER_RESERVED_MEM) ;
		return 0 ;
		}
	else
		{
		g_sysPhysMemSize = BCTL_SDRAM_128M_SIZE; /*reset to default value*/
		/*unsupport memery hardware version or SDRAM memery hardware error*/
		return -1 ;
		}
}

LOCAL void m8247SerialHwInit()
{
	int immrVal = vxImmrGet() ;

	/* SMC1 用于扣板串口，可连入底板插针PC5 SMC1: SMTXD ; PC4 SMC1: SMRXD */
	* M8260_IOP_PCPAR(immrVal) |= (PC4 | PC5); 
	* M8260_IOP_PCDIR(immrVal) &= ~PC4;  
	* M8260_IOP_PCDIR(immrVal) |= PC5;		
	* M8260_IOP_PCSO(immrVal) &= ~(PC4 | PC5);	

#ifdef M8247_SMC2
	/* SMC 2  提供的串口接口，预留（没有电压转换）PA9 SMC2: SMTXD ;PA8 SMC2: SMRXD*/
	* M8260_IOP_PAPAR(immrVal) |= (PA8 | PA9); 
	* M8260_IOP_PADIR(immrVal) &= ~PA8;  
	* M8260_IOP_PADIR(immrVal) |= PA9;		
	* M8260_IOP_PASO(immrVal) &= ~(PA8 | PA9);	
#endif /*M8247_SMC2*/
	
	/* CPM muxs  :  SMC1:BRG1, SMC2:BRG2  */    
	* M8260_CMXSMR(immrVal) = (VUINT8) 0x00 ;

	/* initialize baud rate generators to 19200 buad */    
	/*((CPM FRQ*2)/16)=BAUD_RATE * (BRGCx[DIV16])*(BRGCx[CD] + 1)*(GSMRx_L[xDCR])*/
	/*((  200 *2  )/16)=19200*            (1)     *(   X +1   )*(      16     ) */        
	/*BRGC[CD] value X = (CPM_FRQ*2)/16/16/19200-1) */
	
	*((UINT32 *)(immrVal + M8260_BRGC1_ADRS)) = (M8260_BRGC_EN | ((CPM_FRQ*2)/16/16/19200-1)<< 1) ;	/*SMC1:BRG1*/

	sysSerialHwInit();

}

LOCAL void bsp_CS_init()
{

	unsigned int	immr = vxImmrGet();

	*M8260_OR4(immr) =(((~(CS_A_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_ACS_DIV2 | M8260_OR_SCY_8_CLK | M8260_OR_TRLX) ;		
	*M8260_BR4(immr) = ((CS_A_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_32 | M8260_BR_V);

	*M8260_OR5(immr) =(((~(CS_B_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_ACS_DIV2 | M8260_OR_SCY_15_CLK | M8260_OR_TRLX) ;		
	*M8260_BR5(immr) = ((CS_B_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_32 | M8260_BR_V);

	*M8260_OR2(immr) =(((~(CS_C_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_ACS_DIV2 | M8260_OR_SCY_5_CLK | M8260_OR_TRLX) ;		
	*M8260_BR2(immr) = ((CS_C_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_16 | M8260_BR_V);

	*M8260_OR3(immr) =(((~(CS_D_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_ACS_DIV2 | M8260_OR_SCY_4_CLK| M8260_OR_TRLX) ;		
	*M8260_BR3(immr) = ((CS_D_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_8 | M8260_BR_V);

}

void bsp_clear_dog()
{
	unsigned int immr = vxImmrGet();
	*M8260_IOP_PCDAT(immr) |= PC14 ;
	*M8260_IOP_PCDAT(immr) &= ~PC14 ;	
}

/******************************************************************************
*
* sysHwInit - initialize the system hardware
*
* This routine initializes various feature of the MPC8260 ADS board. It sets up
* the control registers, initializes various devices if they are present.
*
* NOTE: This routine should not be called directly by the user.
*
* RETURNS: NA
*/

void sysHwInit (void)
{
	int immrVal = vxImmrGet() ;

	/* reset the parallel ports and set the according value */
	* M8260_IOP_PADIR(immrVal) = 0;
	* M8260_IOP_PAPAR(immrVal) = 0;
	* M8260_IOP_PASO(immrVal)  = 0;
	* M8260_IOP_PAODR(immrVal) = 0;

	* M8260_IOP_PBDIR(immrVal) = 0;
	* M8260_IOP_PBPAR(immrVal) = 0;
	* M8260_IOP_PBSO(immrVal)  = 0;
	* M8260_IOP_PBODR(immrVal) = 0;

	 /*set hardware dog control I/O: output PC14*/
	  /*set 81BCTL flash WP : output PC11*/
	* M8260_IOP_PCPAR(immrVal) = 0;
	* M8260_IOP_PCDIR(immrVal) = PC14 | PC11;
	* M8260_IOP_PCSO(immrVal)  = 0;
	* M8260_IOP_PCODR(immrVal) = 0;


	 /*set led control I/O : output PD20,PD21*/
	* M8260_IOP_PDPAR(immrVal) = 0;
	* M8260_IOP_PDDIR(immrVal) = (PD20|PD21);
	* M8260_IOP_PDSO(immrVal)  = 0;
	* M8260_IOP_PDODR(immrVal) = 0;

			
	/* set the BRGCLK division factor */
	* M8260_SCCR(immrVal) = M8260_SCCR_DFBRG_16 | M8260_SCCR_PCI_DIV ;

	/*External master pins configuration. set DPPC in SIUMCR[EXTMC] to 00 so that pins used as IRQ3,4,5, and so on */	
	* M8260_SIUMCR(immrVal) &= ~M8260_DPPC_MASK;  

	/*SIUMCR[BBD] to 1b so that pins used as IRQ2,3*/	
	* M8260_SIUMCR(immrVal) |= M8260_DPPC_VALUE  ;  

	/* reset the Communications Processor */

	*M8260_CPCR(immrVal) = M8260_CPCR_RESET | M8260_CPCR_FLG;


	CACHE_PIPE_FLUSH();

	/* Re-Config the sysphysMemeDesc ,for dynamic memory re-config */

	bsp_physMemDesc_config() ;

	/* Get the Baud Rate Generator Clock  frequency */

	baudRateGenClk = sysBaudClkFreq();

	/* Initialize interrupts */

	bsp_CS_init();
	
	m8260IntrInit();

	m8247SerialHwInit();

#ifdef INCLUDE_PCI

	/* config pci */
	if (pciConfigLibInit (PCI_MECHANISM_1,sysPciConfAddr, sysPciConfData, 0) != OK)
	{
		sysToMonitor(BOOT_NO_AUTOBOOT);  /* BAIL */
	}

	/*  Initialize PCI interrupt library. */
	if ((pciIntLibInit ()) != OK)
	{
		sysToMonitor(BOOT_NO_AUTOBOOT);
	}
	mot82xxBridgeInit(); 
	/*	if (PCI_CFG_TYPE == PCI_CFG_AUTO)
		sysPciAutoConfig();
	*/
#endif /* INCLUDE_PCI */

	/*
	 * The power management mode is initialized here. Reduced power mode
	 * is activated only when the kernel is iddle (cf vxPowerDown).
	 * Power management mode is selected via vxPowerModeSet().
	 * DEFAULT_POWER_MGT_MODE is defined in config.h.
	 */
	vxPowerModeSet (DEFAULT_POWER_MGT_MODE);	

	bsp_clear_dog();	
	}

 


/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* Determine installed memory by reading memory control registers
* and calculating if one or 2 chip selects are used for SDRAM.
* Use the address mask and valid bit to determine each bank size.
*
* RETURNS: The address of the top of physical memory.
*
* SEE ALSO: sysMemTop()
*/

char * sysPhysMemTop (void)
    {
    LOCAL char * physTop = NULL;

    physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + g_sysPhysMemSize);

    return (physTop) ;
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
    LOCAL char * memTop = NULL;

    if (memTop == NULL)
	{
	memTop = sysPhysMemTop () - USER_RESERVED_MEM;
	}

    return memTop;
    }


/******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  Normally, it is called
* only by reboot()--which services ^X--and bus errors at interrupt level.
* However, in some circumstances, the user may wish to introduce a
* <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
     int startType	/* parameter passed to ROM to tell it how to boot */
    )
    {
	FUNCPTR pRom = (FUNCPTR) (ROM_TEXT_ADRS + 4);	/* Warm reboot */

    	externalPciMasterDisable(0);	/* disable the ALL the PCI master(include CPU pci)*/
    	
	flTakeFlashMutex(0 , 1) ; /*保证boot flash不再被其它任务读写*/

	logMsg("sysToMonitor : long-jump to 0x%x\n" , (UINT32)pRom,0,0,0,0,0);
	taskDelay(5);

	intLock();

	cacheDisable(INSTRUCTION_CACHE);
	cacheDisable(DATA_CACHE);

	sysAuxClkDisable(); 

	sysSerialReset();		/* reset the serial device */
	
	vxMsrSet (0);

	(*pRom) (startType);	/* jump to bootrom entry point */

	return (OK);	/* in case we ever continue from ROM monitor */
    }

/******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary.
*
* RETURNS: NA
*/

void sysHwInit2 (void)
    {
    LOCAL BOOL configured = FALSE;
    int immrVal = vxImmrGet();

    CACHE_PIPE_FLUSH();

    if (!configured)
	{

	/* initialize serial interrupts */

	sysSerialHwInit2();

	* M8260_SCCR(immrVal) &= ~M8260_SCCR_TBS;
	CACHE_PIPE_FLUSH();

	configured = TRUE;

#ifdef INCLUDE_PCI_DMA
	pciDmaInit();
#endif /* INCLUDE_PCI_DMA */

#ifdef INCLUDE_PCI_ERROR_HANDLING
        pciErrorHandlingInit();
#endif /* INCLUDE_PCI_ERROR_HANDLING */

	}
    }

/******************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
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
* Not applicable for the busless 8260Ads.
*
* RETURNS: NA
*
* SEE ALSO: sysProcNumGet()
*
*/

void sysProcNumSet
    (
    int 	procNum			/* processor number */
    )
    {
    sysProcNum = procNum;
    }

/******************************************************************************
*
* sysLocalToBusAdrs - convert a local address to a bus address
*
* This routine gets the VMEbus address that accesses a specified local
* memory address.
*
* Not applicable for the 8260Ads
*
* RETURNS: ERROR, always.
*
* SEE ALSO: sysBusToLocalAdrs()
*/

STATUS sysLocalToBusAdrs
    (
    int 	adrsSpace,	/* bus address space where busAdrs resides */
    char *	localAdrs,	/* local address to convert */
    char **	pBusAdrs	/* where to return bus address */
    )
    {
    return (ERROR);
    }

/******************************************************************************
*
* sysBusToLocalAdrs - convert a bus address to a local address
*
* This routine gets the local address that accesses a specified VMEbus
* physical memory address.
*
* Not applicable for the 8260Ads
*
* RETURNS: ERROR, always.
*
* SEE ALSO: sysLocalToBusAdrs()
*/

STATUS sysBusToLocalAdrs
    (
     int  	adrsSpace, 	/* bus address space where busAdrs resides */
     char *	busAdrs,   	/* bus address to convert */
     char **	pLocalAdrs 	/* where to return local address */
    )
    {
    return (ERROR);
    }


/*******************************************************************************
*
* vxImmrGet - return the current IMMR value
*
* This routine returns the current IMMR value
*
* RETURNS: current IMMR value
*
*/

UINT32  vxImmrGet (void)
    {

    return (INTERNAL_MEM_MAP_ADDR & IMMR_ISB_MASK);

    }

/*区分不同的cpu外部中断向量接口函数*/
int BspInumToIvec(int intNum)
{
    return intNum + 18 ;
}

uint32_t sysBaudClkFreq(void)
{
    UINT32 cpmFreq =CPM_FRQ;	
    return cpmFreq*2/16;
}

UINT32 sysClkFreqGet(void)
{
	return	DEC_CLOCK_FREQ ;
}

/*Time Base/Decrementer freq is BUS clock/8*/
UINT32 sysTBFreqGet(void) 
{
	return (sysClkFreqGet() /4) ;
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
* drive number, 0.  The second argument, 3, is the function number (also 
* known as TFFS_PHYSICAL_ERASE).  The third argument, 0, specifies the unit 
* number of the first erase unit you want to erase.  The fourth argument, 8,
* specifies how many erase units you want to erase.  
*
* RETURNS: OK, or ERROR if it fails.
*/
#include "tffs/tffsDrv.h"

STATUS sysTffsFormat (int tffsNo)
{
	STATUS status;
	tffsDevFormatParams params = 
	{

		{0x100000, 99, 1, 0x20000, NULL, {0,0,0,0}, NULL, 2, 0, NULL},
		FTL_FORMAT_IF_NEEDED
	};
	/*when tffsNo = 0 , reserve 1M for bios, else all flash area is dosfs/tffs*/
	if(tffsNo > 0)
		params.formatParams.bootImageLen = 0x000000 ;
	
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
	int immr = vxImmrGet();
	if(chipno == 0)
		*M8260_IOP_PCDAT(immr) |= PC11;/*PC11 pin*/
	else if(chipno == 1)
		*CPLD_REG(CPLD_FLASH_WP) |= BIT0;/*bit 0 pin*/
	else if(chipno == 2)
		*CPLD_REG(CPLD_FLASH_WP) |= BIT1;/*bit 1 pin*/
	
}
/*flash WP pin protect diable(De-Assert)*/
void flash_write_disable(int chipno)
{
	int immr = vxImmrGet();	
	if(chipno == 0)
		*M8260_IOP_PCDAT(immr) &= ~PC11;/*PC11 pin*/
	else if(chipno == 1)
		*CPLD_REG(CPLD_FLASH_WP) &= ~BIT0;/*bit 0 pin*/
	else if(chipno == 2)
		*CPLD_REG(CPLD_FLASH_WP) &= ~BIT1;/*bit 1 pin*/
		
}	

#endif  /* INCLUDE_TFFS*/


#ifdef INCLUDE_MOTFCCEND

unsigned char bsp_default_cpm_enet_addr[6]  = {0x08, 0xC0, 0xA8, 0x00, 0xFE, 0x01};

/*******************************************************************************
*
* sysFccEnetEnable - enable the MII interface to the FCC controller
*
* This routine is expected to perform any target specific functions required
* to enable the Ethernet device and to connect the MII interface to the FCC.
*
* RETURNS: OK, or ERROR if the FCC controller cannot be enabled.
*/
 
STATUS sysFccEnetEnable
    (
    UINT32      immrVal,      	/* base address of the on-chip RAM */
    UINT8	fccNum		/* FCC being used */
    )
{

   if(fccNum == 1)
	    {
	    /*
	     PxPAR:   0(general)         1(peripheral) 
	     PxDIR:   0(input)           1(output)
	     PxSO:    0(peripheral 1)    1(peripheral 2) 
	     PC11:MDC  PC10:MDIO
	     PC22(CLK10):RXD CLOCK,PC23(CLK9):TXD CLOCK
	     */

	 
	    /* set Port A and C to use MII signals */
	    /* Notes:the sequence is versed.the MSB is bit0,not bit31*/

	    /*MDIO,MDC configuration as general function*/
	    *M8260_IOP_PCPAR(immrVal) &= ~(0x03000000);	/* ~(6,7),*/    
	    
	    /*peripheral*/
	    *M8260_IOP_PAPAR(immrVal) |= (0x0003FC3F);	/* (14-21,26-31,) */

		/*output*/
	    *M8260_IOP_PADIR(immrVal) |= (0x00003C0C);	/* (18-21,28-29) */
	    /*input*/
	    *M8260_IOP_PADIR(immrVal) &= ~(0x0003C033); /* ~(14-17,26-27,30-31) */

	    /*option 0*/						                             
	    *M8260_IOP_PASO(immrVal)  &= ~(0x0003FC00); /* ~(14-21) */
	    /*option 1*/				                              	 
	    *M8260_IOP_PASO(immrVal)  |= (0x0000003F);	/* (26-31) */

	    /*CLK9,10 for FCC1*/
	    *M8260_IOP_PCPAR(immrVal) |=  (0x00000300);	/* (22-23) peripheral */
	    *M8260_IOP_PCDIR(immrVal) &= ~(0x00000300);	/* ~(22-23),input */
	    *M8260_IOP_PCSO(immrVal)  &= ~(0x00000300);	/* ~(22-23),option 0 */
	    
	    /* connect FCC1 clocks */
	    *M8260_CMXFCR (immrVal)  |= (M8260_CMXFCR_R1CS_CLK10 | M8260_CMXFCR_T1CS_CLK9);

	    /* NMSI mode */
	    *M8260_CMXFCR (immrVal)  &= ~(M8260_CMXFCR_FC1_MUX);         

	    return (OK);
	    }
   else if(fccNum == 2)
		{
	    /*
	     PxPAR:   0(general)         1(peripheral) 
	     PxDIR:   0(input)           1(output)
	     PxSO:    0(peripheral 1)    1(peripheral 2) 
	     PC28:Switch_MDC  PC27:Switch_MDIO
	     PC19(CLK13):RXD CLOCK,PC18(CLK14):TXD CLOCK
	     */
	    	
	    /* enable the Ethernet tranceiver for the FCC */
	    /* set Port B and C to use MII signals */
	    /* Notes:the sequence is versed.the MSB is bit0,not bit31*/

	    /*MDIO,MDC configuration as general function*/
//        *M8260_IOP_PCPAR(immrVal) &= ~(0x00300000);	/* ~(10,11),*/    
	    
	    /*peripheral*/
	    *M8260_IOP_PBPAR(immrVal) |= (0x00003FFF);	/* (18-31,) */
	    /*output*/
	    *M8260_IOP_PBDIR(immrVal) |= (0x000003C5);	/* (22-25,29,31) */
	    /*input*/
	    *M8260_IOP_PBDIR(immrVal) &= ~(0x00003C3A); /* ~(18-21,26-28,30) */

	    /*option 0*/						                             
	    *M8260_IOP_PBSO(immrVal)  &= ~(0x00003FFB); /* ~(18-28,30~31) */
	    /*option 1*/				                              	 
	    *M8260_IOP_PBSO(immrVal)  |= (0x00000004);	/* (29) */

	    /*PC18(CLK14),PC19(CLK13) for FCC2*/
	    *M8260_IOP_PCPAR(immrVal) |=  (0x00003000);	/*  (18,19) peripheral */	    
	    *M8260_IOP_PCDIR(immrVal) &= ~(0x00003000);	/* ~(18,19),input */
	    *M8260_IOP_PCSO(immrVal)  &= ~(0x00003000);	/* ~(18,19),option 0 */
	    
	    /* connect FCC2 clocks */
	    *M8260_CMXFCR (immrVal)  |= (M8260_CMXFCR_R2CS_CLK14 | M8260_CMXFCR_T2CS_CLK13);

	    /* NMSI mode */
	    *M8260_CMXFCR (immrVal)  &= ~(M8260_CMXFCR_FC2_MUX);         

	    return (OK);
	    }
   return ERROR;
   }
        
/*******************************************************************************
*
* sysFccEnetDisable1 - disable MII interface to the FCC controller
*
* This routine is expected to perform any target specific functions required
* to disable the Ethernet device and the MII interface to the FCC
* controller.  This involves restoring the default values for all the Port
* B and C signals.
*
* RETURNS: OK, always.
*/
 
STATUS sysFccEnetDisable
    (
    UINT32      immrVal,      	/* base address of the on-chip RAM */
    UINT8	fccNum		/* FCC being used */
    )
    {
    int intLevel = intLock();

    if(fccNum == 1)
	 	{
	    /*
	     PxPAR:   0(general)         1(peripheral) 
	     PxDIR:   0(input)           1(output)
	     PxSO:    0(peripheral 1)    1(peripheral 2) 
	     PC10:MDIO   PC11:MDC  
	     PC22(CLK10):RXD CLOCK, PC23(CLK9):TXD CLOCK
	     */

	    /*input*/
	    *M8260_IOP_PCDIR(immrVal) &= ~(0x00100000);	/* MDC:PC11 */
	    /*input*/
	    *M8260_IOP_PADIR(immrVal) &= ~(0x00003C0C);	/* (18-21,28-29) */
	   /*general*/
	    *M8260_IOP_PAPAR(immrVal) &= ~(0x0003FC3F);	/* (14-21,26-31) */

	    /*CLK9,10,general*/                        						
	    *M8260_IOP_PCPAR(immrVal) &= ~(0x00000300);	/* (22-23) */
	    
	     }
	else if(fccNum == 2)
		{
	  	/*
	     PxPAR:   0(general)         1(peripheral) 
	     PxDIR:   0(input)           1(output)
	     PxSO:    0(peripheral 1)    1(peripheral 2) 
	     PC28:Switch_MDC  PC27:Switch_MDIO
	     PC19(CLK13):RXD CLOCK, PC18(CLK14):TXD CLOCK
	     */

	    /*input*/
	    *M8260_IOP_PCDIR(immrVal) &= ~(0x00000008);	/* MDC:PC28 */
	    /*input*/
	    *M8260_IOP_PBDIR(immrVal) &= ~(0x000003C5);	/* (22-25,29,31) */
	   /*general*/
	    *M8260_IOP_PBPAR(immrVal) &= ~(0x0003FFF);	/* (18-31) */

	    /*PC18(CLK14),PC19(CLK13),general*/                        						
	    *M8260_IOP_PCPAR(immrVal) &= ~(0x00003000);	/* (18,19) */
	 	}

    /* disable the Ethernet tranceiver for the FCC */
	
    intUnlock (intLevel);
 
    return (OK);	
}

     
/*******************************************************************************
*
* sysFccEnetAddrGet - get the hardware Ethernet address
*
* This routine provides the six byte Ethernet hardware address that will be
* used by each individual FCC device unit.  This routine must copy
* the six byte address to the space provided by <addr>.
*
* RETURNS: OK, or ERROR if the Ethernet address cannot be returned.
*/
 
STATUS sysFccEnetAddrGet
    (
    int	unit,		/* FCC being used */
    UCHAR *     addr            /* where to copy the Ethernet address */
    )
    {
	extern	unsigned char bsp_get_chss_pos(void) ;
	int chssPose;

	if(unit == 1)
	{
		if(bsp_ifadrs_check(0) == 0) 
		{
			bsp_ifadrs_get_mac(0 , addr);
		}
		else                          /*使用默认的MAC地址*/
		{
			memcpy(addr , bsp_default_cpm_enet_addr , 6);
		}
	}
	else if(unit == 0)
	{
		chssPose = bsp_get_chss_pos();
		if(chssPose  !=  0xfe )
		{
			addr[0] = 0x80;
			addr[1] = 0xC0;
			addr[2] = 0xA8;
			addr[3] = 0xD2;
			addr[4] = chssPose;
			addr[5] = 0x01;
		}
		else if(bsp_ifadrs_check(1) == 0) 
		{
			bsp_ifadrs_get_mac(1 , addr);
		}
		else						  /*使用默认的MAC地址*/
		{
			memcpy(addr , bsp_default_cpm_enet_addr , 6);
		}

	}
	printf("motfcc %d mac address = %02x:%02x:%02x:%02x:%02x:%02x\n" , unit , addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]) ;
	return 0;
    }


/*******************************************************************************
*
* sysFccEnetCommand - issue a command to the Ethernet interface controller
*
* RETURNS: OK, or ERROR if the Ethernet controller could not be restarted.
*/

STATUS sysFccEnetCommand
    (
    UINT32      immrVal,      	/* base address of the on-chip RAM */
    UINT8	fccNum,		/* FCC being used */
    UINT16	command
    )
    {
    return (OK);
    }


#ifndef NSDELAY

#define MOT_FCC_LOOP_NS   1 
#define NSDELAY(nsec)                                                   \
		{																	\
		volatile int nx = 0;												\
		volatile int loop = (int)(nsec*MOT_FCC_LOOP_NS);					\
																			\
		for (nx = 0; nx < loop; nx++);										\
		}
#endif /* NSDELAY */
/**************************************************************************
*
* sysFccMiiBitWr - write one bit to the MII interface
*
* This routine writes the value in <bitVal> to the MDIO line of a MII
* interface. The MDC line is asserted for a while, and then negated.
* If <bitVal> is NONE, then the MDIO pin should be left in high-impedance
* state.
*
* SEE ALSO: sysFccMiiBitRd()
*
* RETURNS: OK, or ERROR.
*/


STATUS sysFccMiiBitWr
    (
    UINT32      immrVal,      	/* base address of the on-chip RAM */
    UINT8	fccNum,		/* FCC being used */
    INT32        bitVal          /* the bit being written */
    )
    {
    /* 
     * we create the timing reference for transfer of info on the MDIO line 
     * MDIO is mapped on PC10, MDC on PC11. We need to keep the same data
     * on MDIO for at least 400 nsec.
     */
     
    /*PC6:MDC PC7:MDIO*/
    *M8260_IOP_PCPAR(immrVal) &= ~(0x03000000);	/* ~(6-7) */
    *M8260_IOP_PCDIR(immrVal) |= (0x03000000);	/* (6-7) */
    *M8260_IOP_PCDAT(immrVal) &= ~(0x02000000);	/* (6) */

    switch (bitVal)
	{
	case 0:
	    *M8260_IOP_PCDAT(immrVal) &= ~(0x01000000);	/* ~(10) */
	    break;

	case 1:
	    *M8260_IOP_PCDAT(immrVal) |= (0x01000000);	/* (10) */
	    break;

	case ((INT32) NONE):
	    /* put it in high-impedance state */

	    *M8260_IOP_PCDIR(immrVal) &= ~(0x01000000);	/* ~(10) */

	    break;

	default:
	    return (ERROR);
	}

    /* delay about 200 nsec. */

    NSDELAY (200);

    /* now we toggle the clock and delay again */

    *M8260_IOP_PCDAT(immrVal) |= (0x02000000);	/* ~(11) */

    NSDELAY (200);

    return (OK);
    }
 
/**************************************************************************
*
* sysFccMiiBitRd - read one bit from the MII interface
*
* This routine reads one bit from the MDIO line of a MII
* interface. The MDC line is asserted for a while, and then negated.
*
* SEE ALSO: sysFccMiiBitWr()
*
* RETURNS: OK, or ERROR.
*/

STATUS sysFccMiiBitRd
    (
    UINT32      immrVal,      	/* base address of the on-chip RAM */
    UINT8	fccNum,		/* FCC being used */
    INT8 *      bitVal          /* the bit being read */
    )
    {
    /* 
     * we create the timing reference for transfer of info on the MDIO line 
     * MDIO is mapped on PC10, MDC on PC11. We can read data on MDIO after
     * at least 400 nsec.
     */
    /*PC6:MDC PC7:MDIO   */

    *M8260_IOP_PCPAR(immrVal) &= ~(0x03000000);	/* ~(6-7) */
    *M8260_IOP_PCDIR(immrVal) &= ~(0x01000000);	/* (7) */

    *M8260_IOP_PCDIR(immrVal) |=  (0x02000000);	/* (6),keep output state for ever */
    *M8260_IOP_PCDAT(immrVal) |= (0x02000000);	/* (6) */

    /* delay about 200 nsec. */

    NSDELAY (200);

    /* now we toggle the clock and delay again */

    *M8260_IOP_PCDAT(immrVal) &= ~(0x02000000);	/* (6) */

    NSDELAY (200);

    /* we can now read the MDIO data on PC7 */

    *bitVal = (*M8260_IOP_PCDAT(immrVal) & (0x01000000)) >> 24; /* (7) */
 
    return (OK);
    }
 
/**************************************************************************
*
* sysMiiOptRegsHandle - handle some MII optional registers
*
* This routine handles some MII optional registers in the PHY 
* described by <pPhyInfo>.
*
* In the case of the ads8260, the PHY that implements the physical layer 
* for the FCC is an LXT970. The default values for some of its chip-specific
* registers seem to be uncompatible with 100Base-T operations. This routine 
* is expected to perform any PHY-specific functions required to bring the
* PHY itself to a state where 100Base-T operations may be possible.
*
* SEE ALSO: miiLib, motFccEnd.
*
* RETURNS: OK, or ERROR.
*/
STATUS sysMiiOptRegsHandle
    (
    PHY_INFO	* pPhyInfo		/* PHY control structure pointer */
    )
    {
    int		retVal;			/* a convenience */

    /* 
     * the LXT971A on the ads8260 comes up with the scrambler
     * function disabled, so that it will not work in 100Base-T mode.
     * write 0 to the configuration register (address 0x10) 
     * to enable the scrambler function 
     */

   /*LED 1.no use 
     LED 2.link
     LED 3.receive.
   */
   MII_WRITE (pPhyInfo->phyAddr, 0x14, 0x0422, retVal);/*LED*/

    MII_WRITE (pPhyInfo->phyAddr, 0x10, 0x0, retVal);
    if (retVal != OK)
        return (ERROR);

    return (OK);
    }

#endif	/* INCLUDE_MOTFCCEND */

#ifdef INCLUDE_PCI 	/* board level PCI routines */
/*******************************************************************************
*
* sysPciSpecialCycle - generate a special cycle with a message
*
* This routine generates a special cycle with a message.
*
* NOMANUAL
*
* RETURNS: OK
*/

STATUS sysPciSpecialCycle
    (
    int		busNo,
    UINT32	message
    )
    {
    int deviceNo	= 0x0000001f;
    int funcNo		= 0x00000007;

    PCI_OUT_LONG (sysPciConfAddr,
                  pciConfigBdfPack (busNo, deviceNo, funcNo) |
                  0x80000000);

    PCI_OUT_LONG (sysPciConfData, message);

    return (OK);
    }

/*******************************************************************************
*
* sysPciConfigRead - read from the PCI configuration space
*
* This routine reads either a byte, word or a long word specified by
* the argument <width>, from the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS sysPciConfigRead
    (
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,	  /* function number */
    int	offset,	  /* offset into the configuration space */
    int width,	  /* width to be read */
    void * pData /* data read from the offset */
    )
    {
    UINT8  retValByte = 0;
    UINT16 retValWord = 0;
    UINT32 retValLong = 0;
    STATUS retStat = ERROR;

    if ((busNo == 0) && (deviceNo == 12))
        return (ERROR);

    switch (width)
        {
        case 1:	/* byte */
            PCI_OUT_LONG (sysPciConfAddr,
                          pciConfigBdfPack (busNo, deviceNo, funcNo) |
                          (offset & 0xfc) | 0x80000000);

            retValByte = PCI_IN_BYTE (sysPciConfData + (offset & 0x3));
            *((UINT8 *)pData) = retValByte;
	    retStat = OK;
            break;

        case 2: /* word */
            PCI_OUT_LONG (sysPciConfAddr,
                          pciConfigBdfPack (busNo, deviceNo, funcNo) |
                          (offset & 0xfc) | 0x80000000);

	    retValWord = PCI_IN_WORD (sysPciConfData + (offset & 0x2));
            *((UINT16 *)pData) = retValWord;
	    retStat = OK;
	    break;

        case 4: /* long */
	    PCI_OUT_LONG (sysPciConfAddr,
		          pciConfigBdfPack (busNo, deviceNo, funcNo) |
		          (offset & 0xfc) | 0x80000000);
	    retValLong = PCI_IN_LONG (sysPciConfData);
            *((UINT32 *)pData) = retValLong;
	    retStat = OK;
            break;

        default:
            retStat = ERROR;
            break;
        }

    return (retStat);
    }

/*******************************************************************************
*
* sysPciConfigWrite - write to the PCI configuration space
*
* This routine writes either a byte, word or a long word specified by
* the argument <width>, to the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*/

STATUS sysPciConfigWrite
    (
    int	busNo,    /* bus number */
    int	deviceNo, /* device number */
    int	funcNo,	  /* function number */
    int	offset,	  /* offset into the configuration space */
    int width,	  /* width to write */
    ULONG data	  /* data to write */
    )
    {
    if ((busNo == 0) && (deviceNo == 12))
        return (ERROR);

    switch (width)
        {
        case 1:	/* byte */
            PCI_OUT_LONG (sysPciConfAddr,
                          pciConfigBdfPack (busNo, deviceNo, funcNo) |
                          (offset & 0xfc) | 0x80000000);
	    PCI_OUT_BYTE ((sysPciConfData + (offset & 0x3)), data);
            break;

        case 2: /* word */
            PCI_OUT_LONG (sysPciConfAddr,
                          pciConfigBdfPack (busNo, deviceNo, funcNo) |
                          (offset & 0xfc) | 0x80000000);
	    PCI_OUT_WORD ((sysPciConfData + (offset & 0x2)), data);
	    break;

        case 4: /* long */
	    PCI_OUT_LONG (sysPciConfAddr,
		          pciConfigBdfPack (busNo, deviceNo, funcNo) |
		          (offset & 0xfc) | 0x80000000);
	    PCI_OUT_LONG (sysPciConfData, data);
            break;

        default:
            return (ERROR);

        }
    return (OK);
    }
#endif /* INCLUDE_PCI */

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
	sysUsDelay ( (UINT32) delay * 1000 );
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
        if ( MPC8260_DELTA(oldval,newval) < 1000 )
            decElapsed += MPC8260_DELTA(oldval,newval);  /* no rollover */
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

#if  0
/* defined by link script */
extern char wrs_kernel_text_start [];          
extern char wrs_kernel_text_end [];
extern char wrs_kernel_data_start [];
extern char wrs_kernel_data_end [];
extern char wrs_kernel_bss_start [];
extern char wrs_kernel_bss_end [];

/******************************************************************************
*
* sysMmuProtectText - protect text segment in RAM. 
*
* This routine set text segment to read-only. Structure "dataSegPad" is declared 
* to fill the last TEXT page of 4KB, and insures that the data segment does not
* lie in a page that has been write protected.
*
* RETURNS: ERROR
* 
*/

#include "vmLib.h"
STATUS sysMmuProtectText(int enable)

{

	STATUS  rc;
	unsigned int     lenText , vm_state;

	lenText = (int)(wrs_kernel_text_end - wrs_kernel_text_start);
	lenText = (lenText / VM_PAGE_SIZE) * VM_PAGE_SIZE;
	if(enable)
	{
		vm_state = VM_STATE_VALID | VM_STATE_WRITABLE_NOT | VM_STATE_CACHEABLE ;
		printf("Enable text segment protect : --ensure no breakpoint be set \n");
	}
	else
	{
		printf("Disable text segment protect \n");
		vm_state = VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE ;		
	}

	
	rc = vmBaseStateSet(
					NULL, 
					(ABS_ADDR_TYPE)wrs_kernel_text_start,
					lenText,
					VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
					vm_state 
					);
	return rc;

}
#endif 


#if  defined(INCLUDE_PCI)
  
#include "pci/pciIntLib.c"           /* PCI int support */
#include "pci/pciConfigLib.c"        /* pci configuration library */
  
  /* use show routine */
#ifdef INCLUDE_SHOW_ROUTINES
#include "pci/pciConfigShow.c"       /* pci configuration show routines */
#endif

#endif /* defined(INCLUDE_PCI)*/

/* TMR2[PS](diver) is 20, CLK source is system clock is 33,333,333HZ
 * divided by 16. so timer source is system clock/320 = 104,116
 * SO the Counter of the 500 us is  0.104,116 *500 = 52
 * so...
 */

#if  defined(INCLUDE_MPC8260) ||defined(INCLUDE_MPC8247) 

#define COUNTER_100us  21
#include "drv/mem/m8260Siu.h"
#include "drv/sio/m8260Sio.h"
#include "PPC8247/m8260IntrCtl.h"
#include "drv/timer/m8260Timer.h"

/*中断处理函数中必须清中断*/
static VOIDFUNCPTR  timerHandle = NULL ;
void timer_isr_handle(int handle_param)
{
	int	immrVal = vxImmrGet();
	*M8260_TER1(immrVal) = 0x0003;/*clear event register*/
	if(timerHandle)
		timerHandle(handle_param) ;
	
}
 
/* 
void bsp_timer1_set(int action  , int timer_value , VOIDFUNCPTR *timer_handle , int handle_param)
设置周期性的中断处理接口
action = 0 ,关闭中断(其它参数可以不填)，else 使能中断
timer_value : 执行的中断的时间间隔= COUNTER_100us*timer_value [1,1000],100微妙到100毫秒之间
timer_handle :中断处理函数
handle_param :中断处理函数的参数
*/
STATUS bsp_timer1_set(int action  , int timer_value , VOIDFUNCPTR timer_handle , int handle_param)
{
    int	immrVal = vxImmrGet();

	if(action && ((timer_value <= 0) || (timer_value > 1000)) && timer_handle)
		{
		printf("bsp_timer1_set param error , timer_value  = %d( 0 < timer_value <1000) ,timer_handle = 0x%x\n" 
			, timer_value,(int)timer_handle );
		return ERROR ;
		}
	if (action)
	{
		timerHandle = timer_handle;
		*M8260_TGCR1(immrVal) = 0x19;

		/* PS : Prescaler 0xfa = 251 ? 
		 * CE :	Capture on any TINx edge and enable interrupt on the capture event
		 * OM : 
		 * Restart, internal general system clock divided by 16
		 */
		*M8260_TMR1(immrVal) = 0x13dc; 
		*M8260_TRR1(immrVal) = timer_value*COUNTER_100us; /*count value*/
		*M8260_TER1(immrVal) = 0x0003;        /*clear event register*/
		(void) intConnect (INUM_TO_IVEC(INUM_TIMER1), 
			(VOIDFUNCPTR) timer_isr_handle, (int) handle_param);
		intEnable(INUM_TIMER1);/*Timer2 int enable*/
	}
	else
	{
		timerHandle = NULL;
		*M8260_TGCR1(immrVal) = 0x19;
		*M8260_TMR1(immrVal) = 0x0 ;
		intDisable(INUM_TIMER1);
		*M8260_TER1(immrVal) = 0x0003;/*clear event register*/
	}
	return OK ;
}
#endif /*defined(INCLUDE_MPC8260) ||defined(INCLUDE_MPC8247) */
