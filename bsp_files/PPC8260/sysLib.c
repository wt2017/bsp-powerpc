/* sysLib.c - Motorola ads 8260 board system-dependent library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
2005-06-16 bjm   revise mmu setting (BBTN710)
                 added FCC2 setting,not finished.  
01u,12dec01,jrs  Add BAT table entry to correct mem map.
01t,10dec01,jrs  change copyright date
01s,30nov01,gls  fixed sysBatDesc definition (SPR #20321)
01r,17oct01,jrs  Upgrade to veloce
		 set M8260_SCCR to BRGCLK_DIV_FACTOR,
		 added global variable baudRateGenClk - set by sysBaudClkFreq(),
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
#include "drv/timer/m8260Timer.h"
#include "drv/sio/m8260CpmMux.h"
#include "miiLib.h"
#include "ads8260.h"
#include "drv/mem/m8260Siu.h" 
#include "drv/intrCtl/m8260IntrCtl.h" 

#include "proj_config.h"



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
	((0x04000000 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_64M |_MMU_UBAT_VS | _MMU_UBAT_VP),
	((0x02000000 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_WRITE_THROUGH | _MMU_LBAT_MEM_COHERENT ),

	/* I BAT 1 */
	((0x08000000 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_64M |_MMU_UBAT_VS | _MMU_UBAT_VP),
	((0x08000000 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_WRITE_THROUGH | _MMU_LBAT_MEM_COHERENT ),
	/* I BAT 2 */
	0, 0,

	/* I BAT 3 */
	0, 0,

	/* D BAT 0 */
	((0x04000000 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_64M |_MMU_UBAT_VS | _MMU_UBAT_VP),
	((0x04000000 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW |_MMU_LBAT_WRITE_THROUGH | _MMU_LBAT_MEM_COHERENT),

	/* D BAT 1 */
	((FLASH_BASE_ADRS & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_64M | _MMU_UBAT_VS | _MMU_UBAT_VP),   
	((FLASH_BASE_ADRS & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW | _MMU_LBAT_CACHE_INHIBIT),	   

	/* D BAT 2 */
	((0x08000000 & _MMU_UBAT_BEPI_MASK) | _MMU_UBAT_BL_64M |_MMU_UBAT_VS | _MMU_UBAT_VP),
	((0x08000000 & _MMU_LBAT_BRPN_MASK) | _MMU_LBAT_PP_RW |_MMU_LBAT_WRITE_THROUGH | _MMU_LBAT_MEM_COHERENT),

	/* D BAT 3 */
	0, 0,  
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

PHYS_MEM_DESC sysPhysMemDesc [] =
{
	{
	(VIRT_ADDR) LOCAL_MEM_LOCAL_ADRS ,
	(PHYS_ADDR) LOCAL_MEM_LOCAL_ADRS ,
	0x04000000 ,	
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE |VM_STATE_MASK_MEM_COHERENCY,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE |VM_STATE_MEM_COHERENCY
	},

	/*some memory use the BAT(block address translation),see sysBatDesc defined previously*/

	{
	/*user reserved uncacheable Memory*/	
	(VIRT_ADDR) (LOCAL_MEM_LOCAL_ADRS + 0x0c000000),
	(PHYS_ADDR) (LOCAL_MEM_LOCAL_ADRS + 0x0c000000),
	0x04000000 , 
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
	},

	{
	/* MPC8260 Internal Memory Map */

	(VIRT_ADDR) DEFAULT_IMM_ADRS,/*0xfff00000*/
	(PHYS_ADDR) DEFAULT_IMM_ADRS,
	IMM_SIZE,/*0x20000*/
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE |
	VM_STATE_MASK_GUARDED,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT  |
	VM_STATE_GUARDED
	},

	{
	/* CS2 main CPLD */

	(VIRT_ADDR) LOCAL_CPLD_BASE_ADRS,
	(PHYS_ADDR) LOCAL_CPLD_BASE_ADRS,
	LOCAL_CPLD_ADRS_SIZE ,
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
	},

	{
	/* CS4  NVRAM*/
	(VIRT_ADDR) NVRAM_BASE_ADRS,
	(PHYS_ADDR) NVRAM_BASE_ADRS,
	NVRAM_SIZE ,
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
	},

	{
	/* CS9  little CPLD*/

	(VIRT_ADDR) IO_BASE_ADRS,
	(PHYS_ADDR) IO_BASE_ADRS,
	IO_SIZE,
	VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
	VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
	},        
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
  

/* 8260 Reset Configuration Table (From page 9-2 in Rev0 of 8260 book) */
#define END_OF_TABLE 0

struct config_parms {
    UINT32 inputFreq;     /*          MODCK_H                        */
    UINT8  modck_h;       /*             |                           */
    UINT8  modck13;       /*             |MODCK[1-3]                 */
    UINT32 cpmFreq;       /*   Input     |  |     CPM          Core  */
    UINT32 coreFreq;      /*     |       |  |      |            |    */
    } modckH_modck13[] = {/*     V       V  V      V            V    */
                            {FREQ_33_MHZ, 1, 0, FREQ_66_MHZ,  FREQ_133_MHZ},
                            {FREQ_33_MHZ, 1, 1, FREQ_66_MHZ,  FREQ_166_MHZ},
                            {FREQ_33_MHZ, 1, 2, FREQ_66_MHZ,  FREQ_200_MHZ},
                            {FREQ_33_MHZ, 1, 3, FREQ_66_MHZ,  FREQ_233_MHZ},
                            {FREQ_33_MHZ, 1, 4, FREQ_66_MHZ,  FREQ_266_MHZ},
                            {FREQ_33_MHZ, 1, 5, FREQ_100_MHZ, FREQ_133_MHZ},
                            {FREQ_33_MHZ, 1, 6, FREQ_100_MHZ, FREQ_166_MHZ},
                            {FREQ_33_MHZ, 1, 7, FREQ_100_MHZ, FREQ_200_MHZ},
                            {FREQ_33_MHZ, 2, 0, FREQ_100_MHZ, FREQ_233_MHZ},
                            {FREQ_33_MHZ, 2, 1, FREQ_100_MHZ, FREQ_266_MHZ},
                            {FREQ_33_MHZ, 2, 2, FREQ_133_MHZ, FREQ_133_MHZ},
                            {FREQ_33_MHZ, 2, 3, FREQ_133_MHZ, FREQ_166_MHZ},
                            {FREQ_33_MHZ, 2, 4, FREQ_133_MHZ, FREQ_200_MHZ},
                            {FREQ_33_MHZ, 2, 5, FREQ_133_MHZ, FREQ_233_MHZ},
                            {FREQ_33_MHZ, 2, 6, FREQ_133_MHZ, FREQ_266_MHZ},
                            {FREQ_33_MHZ, 2, 7, FREQ_166_MHZ, FREQ_133_MHZ},
                            {FREQ_33_MHZ, 3, 0, FREQ_166_MHZ, FREQ_166_MHZ},
                            {FREQ_33_MHZ, 3, 1, FREQ_166_MHZ, FREQ_200_MHZ},
                            {FREQ_33_MHZ, 3, 2, FREQ_166_MHZ, FREQ_233_MHZ},
                            {FREQ_33_MHZ, 3, 3, FREQ_166_MHZ, FREQ_266_MHZ},
                            {FREQ_33_MHZ, 3, 4, FREQ_200_MHZ, FREQ_133_MHZ},
                            {FREQ_33_MHZ, 3, 5, FREQ_200_MHZ, FREQ_166_MHZ},
                            {FREQ_33_MHZ, 3, 6, FREQ_200_MHZ, FREQ_200_MHZ},
                            {FREQ_33_MHZ, 3, 7, FREQ_200_MHZ, FREQ_233_MHZ},
                            {FREQ_33_MHZ, 4, 0, FREQ_200_MHZ, FREQ_266_MHZ},
                            {FREQ_40_MHZ, 5, 7, FREQ_80_MHZ,  FREQ_120_MHZ},
                            {FREQ_66_MHZ, 5, 5, FREQ_133_MHZ, FREQ_133_MHZ},
                            {FREQ_66_MHZ, 5, 6, FREQ_133_MHZ, FREQ_166_MHZ},
                            {FREQ_66_MHZ, 5, 7, FREQ_133_MHZ, FREQ_200_MHZ},
                            {FREQ_66_MHZ, 6, 0, FREQ_133_MHZ, FREQ_233_MHZ},
                            {FREQ_66_MHZ, 6, 1, FREQ_133_MHZ, FREQ_266_MHZ},
                            {FREQ_66_MHZ, 6, 2, FREQ_133_MHZ, FREQ_300_MHZ},
                            {FREQ_66_MHZ, 6, 3, FREQ_166_MHZ, FREQ_133_MHZ},
                            {FREQ_66_MHZ, 6, 4, FREQ_166_MHZ, FREQ_166_MHZ},
                            {FREQ_66_MHZ, 6, 5, FREQ_166_MHZ, FREQ_200_MHZ},
                            {FREQ_66_MHZ, 6, 6, FREQ_166_MHZ, FREQ_233_MHZ},
                            {FREQ_66_MHZ, 6, 7, FREQ_166_MHZ, FREQ_266_MHZ},
                            {FREQ_66_MHZ, 7, 0, FREQ_166_MHZ, FREQ_300_MHZ},
                            {FREQ_66_MHZ, 7, 1, FREQ_200_MHZ, FREQ_133_MHZ},
                            {FREQ_66_MHZ, 7, 2, FREQ_200_MHZ, FREQ_166_MHZ},
                            {FREQ_66_MHZ, 7, 3, FREQ_200_MHZ, FREQ_200_MHZ},
                            {FREQ_66_MHZ, 7, 4, FREQ_200_MHZ, FREQ_233_MHZ},
                            {FREQ_66_MHZ, 7, 5, FREQ_200_MHZ, FREQ_266_MHZ},
                            {FREQ_66_MHZ, 7, 6, FREQ_200_MHZ, FREQ_300_MHZ},
                            {FREQ_66_MHZ, 7, 7, FREQ_233_MHZ, FREQ_133_MHZ},
                            {FREQ_66_MHZ, 8, 0, FREQ_233_MHZ, FREQ_166_MHZ},
                            {FREQ_66_MHZ, 8, 1, FREQ_233_MHZ, FREQ_200_MHZ},
                            {FREQ_66_MHZ, 8, 2, FREQ_233_MHZ, FREQ_233_MHZ},
                            {FREQ_66_MHZ, 8, 3, FREQ_233_MHZ, FREQ_266_MHZ},
                            {FREQ_66_MHZ, 8, 4, FREQ_233_MHZ, FREQ_300_MHZ},
                            {END_OF_TABLE,0,0,0,0}
                         };

/* locals */
LOCAL UINT32 *immrAddress = (UINT32 *) IMMR_ADDRESS_RESET_VALUE;


#include "m8260smc.c" 
#include "intrCtl/m8260IntrCtl.c"
#include "timer/m8260Timer.c"
#include "mem/nullNvRam.c"


#include "PPC8247/m8260i2c.c" 

#include "PPC8247/m8260spi.c" 

#ifdef  INCLUDE_MOTFCCEND
/* set the following array to a unique Ethernet hardware address */
 
/* last 5 nibbles are board specific, initialized in sysHwInit */
unsigned char sysFccEnetAddr1 [6] = {0x08, 0x00, 0x3e, 0x03, 0x02, 0x01};
unsigned char sysFccEnetAddr2 [6] = {0x08, 0x00, 0x3e, 0x03, 0x02, 0x02};

STATUS sysFccEnetAddrGet (int fccNum, UCHAR * address);
STATUS sysFccMiiBitWr (UINT32 immrVal, UINT8 fccNum, INT32 bitVal);
STATUS sysFccMiiBitRd (UINT32 immrVal, UINT8 fccNum, INT8 * bitVal);
STATUS sysMiiOptRegsHandle (PHY_INFO * pPhyInfo);
 
STATUS sysFccEnetEnable (UINT32 immrVal, UINT8 fccNum);

/* locals */
STATUS sysFccEnetDisable (UINT32 immrVal, UINT8 fccNum);
#endif  /* INCLUDE_MOTFCCEND */
 


/* defines */
#define SYS_MODEL       "Motorola MPC8260 TN71X OAM"

/* needed to enable timer base */
#define      M8260_DPPC_MASK	0x0C000000 /* bits 4 and 5 */
#define      M8260_DPPC_VALUE	0x00000000 /* bits (4,5) should be (0,0),changed by bjm */

#include "sysMotFccEnd.c"

#include "bspdriver.c"

#include "bspChassis.c"

/*img软件加载及上层软件升级模块*/
#include "biosmain.c" 

#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)

#include "ftpd6lib.c"
#else

#include "ftpdlib.c"
#endif

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
/*****************************************************************************
*
* i just use this function to change the MAC address of the "cpm0" interface.
*
*/
unsigned char bsp_default_cpm_enet_addr[6]  = {0x08, 0xC0, 0xA8, 0x00, 0xFE, 0x01};
 /******************************************************************************
  *
  * sysCpmFreqGet - Determines the CPM Operating Frequency
  *
  * From page 9-2 Rev. 0  MPC8260  PowerQUICC II User's Manual
  *
  * RETURNS: CPM clock frequency for the current MOD_CK and MOD_CK_H settings  
  */

UINT32 sysCpmFreqGet
    (
     void
    ) 
    {
    UINT  n;
    UINT32 modck_H = MOD_CK_H;
    UINT32 modck13 = MOD_CK;
    UINT32 oscFreq = OSCILLATOR_FREQ;

    for (n=0; modckH_modck13[n].coreFreq != END_OF_TABLE ;n++)
        {
        if ((modckH_modck13[n].modck_h == modck_H) &&
            (modckH_modck13[n].modck13 == modck13))
            {
            if (modckH_modck13[n].inputFreq == oscFreq)
                return  modckH_modck13[n].cpmFreq;
            }
        }

    return ERROR;
    }

/******************************************************************************
*
* sysBaudClkFreq - Obtains frequency of the BRG_CLK in HZ
*
* From page 9-5 in Rev. 0 MPC8260 PowerQUICC II User's Manual
*
*     baud clock = 2*cpm_freq/2^2*(DFBRG+1) where DFBRG = 01
*                = 2*cpm_freq/16
*
* RETURNS: frequency of the BRG_CLK in HZ
*/

int sysBaudClkFreq
    (
     void
    )
    {
    UINT32 cpmFreq = sysCpmFreqGet();

    if (cpmFreq == ERROR)
        return ERROR;
    else
        return cpmFreq*2/16;
    }

/******************************************************************************
 *
 * sysHwMemInit - initialize and configure system memory.
 *
 * This routine is called before sysHwInit(). It performs memory auto-sizing
 * and updates the system's physical regions table, `sysPhysRgnTbl'. It may
 * include the code to do runtime configuration of extra memory controllers.
 *
 * NOTE: This routine should not be called directly by the user application.  It
 * cannot be used to initialize interrupt vectors.
 *
 * RETURNS: N/A
 */

void sysHwMemInit (void)
    {
    /* Call sysPhysMemTop() to do memory autosizing if available */

    sysPhysMemTop ();

    }
UINT32 g_sysPhysMemSize = BCTL_SDRAM_256M_SIZE ;


LOCAL void m8260SerialHwInit()
{
	int immrVal = vxImmrGet() ;

	/* SMC1 用于扣板串口，可连入底板插针PD9 SMC1: SMTXD ; PD8 SMC1: SMRXD */
	* M8260_IOP_PDPAR(immrVal) |= (PD8 | PD9); 
	* M8260_IOP_PDDIR(immrVal) &= ~PD8;  
	* M8260_IOP_PDDIR(immrVal) |= PD9;		
	* M8260_IOP_PDSO(immrVal) &= ~(PD8 | PD9);	

	
	/* CPM muxs  :  SMC1:BRG1, SMC2:BRG2  */    
	* M8260_CMXSMR(immrVal) = (VUINT8) 0x00 ;

	/* initialize baud rate generators to 19200 buad */    
	/*((CPM FRQ*2)/16)=BAUD_RATE * (BRGCx[DIV16])*(BRGCx[CD] + 1)*(GSMRx_L[xDCR])*/
	/*((  166*2  )/16)=19200*            (1)     *(   X +1   )*(      16     ) */        
	/*BRGC[CD] value X = (CPM_FRQ*2)/16/16/19200-1) */

	*((UINT32 *)(immrVal + M8260_BRGC1_ADRS)) = (M8260_BRGC_EN | ((CPM_FRQ*2)/16/16/19200-1)<< 1) ;	/*SMC1:BRG1*/

	sysSerialHwInit();
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
	int	immrVal = vxImmrGet();

	/* reset the parallel ports and set the according value */
	* M8260_IOP_PADIR(immrVal) = 0xbc7c3fcc;
	* M8260_IOP_PAPAR(immrVal) = 0x03c3fc3f;
	* M8260_IOP_PASO(immrVal)  = 0x03c0003f;
	* M8260_IOP_PADAT(immrVal) = 0x802403c0; 

	* M8260_IOP_PBDIR(immrVal) = 0xf1ffc3c5;
	* M8260_IOP_PBPAR(immrVal) = 0x0f003fff;
	* M8260_IOP_PBSO(immrVal) = 0x0f000000;
	* M8260_IOP_PBDAT(immrVal) = 0xf0ffc000;
	    
	* M8260_IOP_PCDIR(immrVal) = 0xa010acee;
	* M8260_IOP_PCPAR(immrVal) = 0x00000011;
	* M8260_IOP_PCSO(immrVal)  = 0x00000000;
	* M8260_IOP_PCDAT(immrVal) = 0xa000acee;

	* M8260_IOP_PDDIR(immrVal) = 0xff7e1dee;
	* M8260_IOP_PDPAR(immrVal) = 0x00c3e003;
	* M8260_IOP_PDSO(immrVal)  = 0x0003f002;
	* M8260_IOP_PDDAT(immrVal) = 0xff3c19e4; 

	/* set the BRGCLK division factor */

	* M8260_SCCR(immrVal) = DIV_FACT_16; /*1*/

	/* set DPPC in SIUMCR to 10 so that timer is enabled (TBEN) */

	* M8260_SIUMCR(immrVal) &= ~M8260_DPPC_MASK;  /* clear the dppc */

	* M8260_SIUMCR(immrVal) |= M8260_DPPC_VALUE;  /* or in the desired value */

	/* reset the Communications Processor */

	*M8260_CPCR(immrVal) = 0x80010000;

	CACHE_PIPE_FLUSH();

	/* Get the Baud Rate Generator Clock  frequency */
	baudRateGenClk = sysBaudClkFreq();

	/* Reset serial channels */
	m8260SerialHwInit();

	/* Initialize interrupts */
	m8260IntrInit();

	/* handle the LXT971 properly */
#ifdef INCLUDE_MOTFCCEND
	miiPhyOptFuncSet ((FUNCPTR) sysMiiOptRegsHandle);
#endif  /* INCLUDE_MOTFCCEND */

	/*
	* The power management mode is initialized here. Reduced power mode
	* is activated only when the kernel is idle (cf vxPowerDown).
	* Power management mode is selected via vxPowerModeSet().
	* DEFAULT_POWER_MGT_MODE is defined in config.h.
	*/

	vxPowerModeSet (DEFAULT_POWER_MGT_MODE);
	g_sysPhysMemSize = BCTL_SDRAM_256M_SIZE    ;

	*M8260_SWSR(immrVal) = 0x556c;
	*M8260_SWSR(immrVal) = 0xaa39;
	
}

/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* RETURNS: The address of the top of physical memory.
*
* SEE ALSO: sysMemTop()
*/

char * sysPhysMemTop (void)
    {
    LOCAL char * physTop = NULL;

    if (physTop == NULL)
	{
	physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
	}

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

    logMsg("sysToMonitor , jump to 0x%08x\n" , ROM_WARM_ADRS ,2,3,4,5,6);
    taskDelay(5);
	
    intLock();

    cacheDisable(INSTRUCTION_CACHE);
    cacheDisable(DATA_CACHE);

    sysAuxClkDisable();

    /* disable both RS232 ports on the board */


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

		* M8260_SCCR(immrVal) &= ~M8260_SCCR_TBS; /*NO such a field ,doubted by bjm*/
		CACHE_PIPE_FLUSH();

		configured = TRUE;
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

/******************************************************************************
*
* sysBusTas - test and set a location across the bus
*
* This routine does an atomic test-and-set operation across the backplane.
*
* Not applicable for the 8260Ads.
*
* RETURNS: FALSE, always.
*
* SEE ALSO: vxTas()
*/

BOOL sysBusTas
    (
     char *	adrs		/* address to be tested-and-set */
    )
    {
    return (FALSE);
    }

/******************************************************************************
*
* sysBusClearTas - test and clear 
*
* This routine is a null function.
*
* RETURNS: NA
*/

void sysBusClearTas
    (
     volatile char * address	/* address to be tested-and-cleared */
    )
    {
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
    return ((*immrAddress) & IMMR_ISB_MASK);
    }

#ifdef INCLUDE_MOTFCCEND 
 
/*******************************************************************************
*
* sysFccEnetEnable1 - enable the MII interface to the FCC controller
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
    	
    /* enable the Ethernet tranceiver for the FCC */
 
    /* introduce a little delay */
    taskDelay (sysClkRateGet ());
 
    /* set Port A and C to use MII signals */
    /* Notes:the sequence is versed.the MSB is bit0,not bit31*/

    /*MDIO,MDC configuration as general function*/
    *M8260_IOP_PCPAR(immrVal) &= ~(0x00300000);	/* ~(10,11),*/    
    
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
    *M8260_IOP_PCDIR(immrVal) &= ~(0x00000300);	/* ~(22-23),input */
    *M8260_IOP_PCPAR(immrVal) |=  (0x00000300);	/* (22-23) peripheral */
    *M8260_IOP_PCSO(immrVal)  &= ~(0x00000300);	/* ~(22-23),option 0 */
    
    /* connect FCC1 clocks */
    *M8260_CMXFCR (immrVal)  |= (M8260_CMXFCR_R1CS_CLK10 | M8260_CMXFCR_T1CS_CLK9);

    /* NMSI mode */

    *M8260_CMXFCR (immrVal)  &= ~(M8260_CMXFCR_FC1_MUX);         

    return (OK);
    }
   else
	{
	    /*
	     PxPAR:   0(general)         1(peripheral) 
	     PxDIR:   0(input)           1(output)
	     PxSO:    0(peripheral 1)    1(peripheral 2) 
	     PC28:Switch_MDC  PC27:Switch_MDIO
	     PC19(CLK13):RXD CLOCK,PC17(CLK15):TXD CLOCK
	     */
	    	
	    /* enable the Ethernet tranceiver for the FCC */
	 
	    /* introduce a little delay */
	    taskDelay (sysClkRateGet ());
	 
	    /* set Port B and C to use MII signals */
	    /* Notes:the sequence is versed.the MSB is bit0,not bit31*/

	    /*MDIO,MDC configuration as general function*/
	    *M8260_IOP_PCPAR(immrVal) &= ~(0x00300000);	/* ~(10,11),*/    
	    
	    /*peripheral*/
	    *M8260_IOP_PBPAR(immrVal) |= (0x00003FFF);	/* (18-31,) */
	    /*output*/
	    *M8260_IOP_PBDIR(immrVal) |= (0x000003C05);	/* (22-25,29,31) */
	    /*input*/
	    *M8260_IOP_PBDIR(immrVal) &= ~(0x00003C3A); /* ~(18-21,26-28,30) */
	    /*option 0*/						                             
	    *M8260_IOP_PBSO(immrVal)  &= ~(0x00003FFB); /* ~(18-28,30~31) */
	    /*option 1*/				                              	 
	    *M8260_IOP_PBSO(immrVal)  |= (0x00000004);	/* (29) */

	    /*PC17(CLK15),PC19(CLK13) for FCC2*/
	    *M8260_IOP_PCDIR(immrVal) &= ~(0x00005000);	/* ~(17,19),input */
	    *M8260_IOP_PCPAR(immrVal) |=  (0x00005000);	/*  (17,19) peripheral */
	    
	    *M8260_IOP_PCSO(immrVal)  &= ~(0x00005000);	/* ~(17,19),option 0 */
	    
	    /* connect FCC2 clocks */
	    *M8260_CMXFCR (immrVal)  |= (M8260_CMXFCR_R2CS_CLK13 | M8260_CMXFCR_T2CS_CLK15);

	    /* NMSI mode */

	    *M8260_CMXFCR (immrVal)  &= ~(M8260_CMXFCR_FC2_MUX);         

	    return (OK);
	    }
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
     PC19(CLK13):RXD CLOCK, PC17(CLK15):TXD CLOCK
     */

    /*input*/
    *M8260_IOP_PCDIR(immrVal) &= ~(0x00000008);	/* MDC:PC28 */
    /*input*/
    *M8260_IOP_PBDIR(immrVal) &= ~(0x00003C05);	/* (22-25,29,31) */
   /*general*/
    *M8260_IOP_PBPAR(immrVal) &= ~(0x0003FFF);	/* (18-31) */

    /*PC17(CLK15),PC19(CLK13),general*/                        						
    *M8260_IOP_PCPAR(immrVal) &= ~(0x00005000);	/* (17,19) */
    
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
    int	fccNum,		/* FCC being used */
    UCHAR *     addr            /* where to copy the Ethernet address */
    )
    {
	extern	unsigned char bsp_get_chss_pos(void) ;
	int chssPose;
	if(fccNum == 0)
	{
		if(bsp_ifadrs_check(0) == 0) /*check 0x38400700 + 0*0x10处的MAC地址*/
		{
			bsp_ifadrs_get_mac(0 , addr);
		}
		else                          /*使用默认的MAC地址*/
		{
			memcpy(addr , bsp_default_cpm_enet_addr , 6);
		}
	}
	else if(fccNum == 1)	/*根据槽位号固定分配一个内网MAC地址*/
	{
	    chssPose = bsp_get_chss_pos();
	    addr[0] = 0x80;
	    addr[1] = 0xC0;
	    addr[2] = 0xA8;
	    addr[3] = 0xD2;
	    addr[4] = chssPose;
	    addr[5] = 0x01;
	}
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
    {                                                                   \
    volatile int nx = 0;                                                \
    volatile int loop = (int)(nsec*MOT_FCC_LOOP_NS);                    \
                                                                        \
    for (nx = 0; nx < loop; nx++);                                      \
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
     
    /* PC10:MDIO  PC11:MDC */
    *M8260_IOP_PCPAR(immrVal) &= ~(0x00300000);	/* ~(10-11) */
    *M8260_IOP_PCDIR(immrVal) |= (0x00300000);	/* (10-11) */
    *M8260_IOP_PCDAT(immrVal) |= (0x00100000);	/* (11) */

    switch (bitVal)
	{
	case 0:
	    *M8260_IOP_PCDAT(immrVal) &= ~(0x00200000);	/* ~(10) */
	    break;

	case 1:
	    *M8260_IOP_PCDAT(immrVal) |= (0x00200000);	/* (10) */
	    break;

	case ((INT32) NONE):
	    /* put it in high-impedance state */

	    *M8260_IOP_PCDIR(immrVal) &= ~(0x00200000);	/* ~(10) */

	    break;

	default:
	    return (ERROR);
	}

    /* delay about 200 nsec. */

    NSDELAY (200);

    /* now we toggle the clock and delay again */

    *M8260_IOP_PCDAT(immrVal) &= ~(0x00100000);	/* ~(11) */

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
    /* PC10:MDIO  PC11:MDC */

    *M8260_IOP_PCPAR(immrVal) &= ~(0x00300000);	/* ~(10-11) */
    *M8260_IOP_PCDIR(immrVal) &= ~(0x00200000);	/* (10) */

    *M8260_IOP_PCDIR(immrVal) |=  (0x00100000);	/* (11),keep output state for ever */
    *M8260_IOP_PCDAT(immrVal) |= (0x00100000);	/* (11) */

    /* delay about 200 nsec. */

    NSDELAY (200);

    /* now we toggle the clock and delay again */

    *M8260_IOP_PCDAT(immrVal) &= ~(0x00100000);	/* (11) */

    NSDELAY (200);

    /* we can now read the MDIO data on PC9 */

    *bitVal = (*M8260_IOP_PCDAT(immrVal) & (0x00200000)) >> 21; /* (10) */
 
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
void flash_write_enable(int chipNo)
{
	int immrVal = vxImmrGet();
	if(chipNo == 0)
		* M8260_IOP_PADAT(immrVal) |= 0x20000000;	 /*PA2 high lead to write enable*/			 
	else if(chipNo == 1)
		* M8260_IOP_PADAT(immrVal) |= 0x00080000;	 /*PA12 high lead to write enable*/  
}
/*flash WP pin protect diable(De-Assert)*/
void flash_write_disable(int chipNo)
{
	int immrVal = vxImmrGet();
	if(chipNo == 0)
		* M8260_IOP_PADAT(immrVal) &= ~0x20000000; /*PA2 low lead to write protected*/			 
	else if (chipNo == 1)
		* M8260_IOP_PADAT(immrVal)&= ~0x00080000;	 /*PA12 low lead to write protected*/ 
}

#define USER_RESERVED_UNCAHEABLE_SIZE   0x01000000

void bsp_get_os_mem(unsigned int *base_adres , unsigned int *len)
{
	if(base_adres) 	*base_adres = LOCAL_MEM_LOCAL_ADRS ;
	if(len)			*len = LOCAL_MEM_SIZE ;	
}

void bsp_get_reserve_cacheable_mem(unsigned int *base_adres , unsigned int *len)
{
	if(base_adres) 	*base_adres = LOCAL_MEM_LOCAL_ADRS+LOCAL_MEM_SIZE-USER_RESERVED_MEM;
	if(len)			*len = USER_RESERVED_MEM-USER_RESERVED_UNCAHEABLE_SIZE;	
}

void bsp_get_reserve_uncacheable_mem(unsigned int *base_adres , unsigned int *len)
{
	if(base_adres) 	*base_adres = LOCAL_MEM_LOCAL_ADRS+LOCAL_MEM_SIZE-USER_RESERVED_UNCAHEABLE_SIZE;
	if(len)			*len = USER_RESERVED_UNCAHEABLE_SIZE;	
}

/*****************************************************************************
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


/* TMR2[PS](diver) is 20, CLK source is system clock is 33,333,333HZ
 * divided by 16. so timer source is system clock/320 = 104,116
 * SO the Counter of the 500 us is  0.104,116 *500 = 52
 * so...
 */

#define COUNTER_100us  21

#include "drv/mem/m8260Siu.h"
#include "drv/sio/m8260Sio.h"
#include "drv/intrctl/m8260IntrCtl.h"
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