/* sysLib.c - Freescale cds8548 board system-dependent library */

/* Copyright (c) 2005-2006 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,30aug06,dtr  Support for latest rev2 silicon and board revision.s.
01e,03feb06,wap  Don't use sysMotTsecEnd.c in VXBUS case
01d,26jan06,dtr  Tidyup post code review.
01c,27oct05,mdo  SPR#114197 - protect against multiple defines of
                 INCLUDE_PCICFG_SHOW
01b,27jul05,dtr  Provide sysGetPeripheralBase for security engine support.
01a,04jun05,dtr   Created from cds85xx/sysLib.c/01v
*/

/*
DESCRIPTION
This library provides board-specific routines for cDS8548.

INCLUDE FILES:

SEE ALSO:
.pG "Configuration"
*/

/* includes */
#include <vxWorks.h>
#include <vme.h>
#include <memLib.h>
#include <cacheLib.h>
#include <sysLib.h>
#include "config.h"
#include <string.h>
#include <intLib.h>
#include <logLib.h>
#include <stdio.h>
#include <taskLib.h>
#include <vxLib.h>
#include <tyLib.h>
#include <arch/ppc/mmuE500Lib.h>
#include <arch/ppc/vxPpcLib.h>
#include <private/vmLibP.h>
#include <miiLib.h>
#include "proj_config.h"
#include "../bspcommon.h"

#ifdef INCLUDE_PCI
    #include <drv/pci/pciConfigLib.h>
    #include <drv/pci/pciIntLib.h>
    #include "mot85xxPci.h"
#endif /* INCLUDE_PCI */


/* globals */

TLB_ENTRY_DESC sysStaticTlbDesc [] =
{

    /* effAddr,  Unused,  realAddr, ts | size | attributes | permissions */

    /* TLB #0.  Flash */

    /* needed be first entry here */
    { FLASH_BASE_ADRS, 0x0, FLASH_BASE_ADRS, _MMU_TLB_TS_0 | _MMU_TLB_SZ_64M |
        _MMU_TLB_IPROT | _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_I | 
        _MMU_TLB_ATTR_G
    },
    /* LOCAL MEMORY needed be second entry here  - 
     * one TLB would be 256MB so use to get required 512MB 
     */
    { 0x00000000, 0x0, 0x00000000, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M | 
        _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_M | 
        CAM_DRAM_CACHE_MODE | _MMU_TLB_IPROT
    },
#if (LOCAL_MEM_SIZE > 0x10000000)
    { 0x10000000, 0x0, 0x10000000, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M | 
        _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_M | 
        CAM_DRAM_CACHE_MODE | _MMU_TLB_IPROT
    }, 
#endif
    { CCSBAR, 0x0, CCSBAR, _MMU_TLB_TS_0 | _MMU_TLB_SZ_1M | 
        _MMU_TLB_ATTR_I | _MMU_TLB_ATTR_G | _MMU_TLB_PERM_W | _MMU_TLB_IPROT
    }
    
#ifdef INCLUDE_LBC_SDRAM
    ,
    { LOCAL_MEM_LOCAL_ADRS2, 0x0, LOCAL_MEM_LOCAL_ADRS2, _MMU_TLB_TS_0 | 
        _MMU_TLB_SZ_64M | _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | 
        CAM_DRAM_CACHE_MODE | _MMU_TLB_ATTR_M | _MMU_TLB_IPROT
    }
#endif /* LBC_SDRAM */

#ifdef INCLUDE_L2_SRAM
    ,
    { L2SRAM_ADDR, 0x0, L2SRAM_ADDR, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256K | 
        _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_I | 
        _MMU_TLB_ATTR_G
    }

#endif /* INCLUDE_L2_SRAM */
#ifdef INCLUDE_LBC_CS3
    /* 16 MB of LBC CS 3 area */
    , {
        LBC_CS3_LOCAL_ADRS, 0x0, LBC_CS3_LOCAL_ADRS,
        _MMU_TLB_TS_0   | _MMU_TLB_SZ_16M | _MMU_TLB_IPROT |
        _MMU_TLB_PERM_W | _MMU_TLB_ATTR_I | _MMU_TLB_ATTR_G | _MMU_TLB_ATTR_M
    }
#endif
#ifdef INCLUDE_RAPIDIO_BUS
    ,
    {
    RAPIDIO_ADRS, 0x0, RAPIDIO_ADRS, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M |
    _MMU_TLB_ATTR_I | _MMU_TLB_PERM_W | _MMU_TLB_ATTR_G 
    }
#endif 

    /* Assume PCI space contiguous and within 256MB */
#ifdef INCLUDE_PCI
    ,
    { PCI_MEM_ADRS, 0x0, PCI_MEM_ADRS, _MMU_TLB_TS_0 | PCI_MMU_TLB_SZ | 
        _MMU_TLB_ATTR_I | _MMU_TLB_ATTR_G | _MMU_TLB_PERM_W
    }
#ifdef INCLUDE_CDS85XX_PCIEX
    ,
    { PCI_MEM_ADRS3, 0x0, PCI_MEM_ADRS3, _MMU_TLB_TS_0 |  PCI_MMU_TLB_SZ | 
        _MMU_TLB_ATTR_I | _MMU_TLB_ATTR_G | _MMU_TLB_PERM_W
    }
#endif /* INCLUDE_CDS85XX_PCIEX */

#endif  /* INCLUDE_PCI */
    ,
    { CS_A_BASE_ADRS, 0x0, CS_A_BASE_ADRS, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M |
        _MMU_TLB_IPROT | _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_I | 
        _MMU_TLB_ATTR_G
    }
    ,
    { 0xB0000000, 0x0, 0xB0000000, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M |
        _MMU_TLB_IPROT | _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_I | 
        _MMU_TLB_ATTR_G
    }
    ,
    { CS_C_BASE_ADRS, 0x0, CS_C_BASE_ADRS, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M |
        _MMU_TLB_IPROT | _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_I | 
        _MMU_TLB_ATTR_G
    }
    ,
    { 0xD0000000, 0x0, 0xD0000000, _MMU_TLB_TS_0 | _MMU_TLB_SZ_256M |
        _MMU_TLB_IPROT | _MMU_TLB_PERM_W | _MMU_TLB_PERM_X | _MMU_TLB_ATTR_I | 
        _MMU_TLB_ATTR_G
    }

};

int sysStaticTlbDescNumEnt = NELEMENTS (sysStaticTlbDesc);

#ifdef MMU_ASID_MAX     /* Base 6 MMU library in effect */
  /* macro to compose 64-bit PHYS_ADDRs */
# define PHYS_64BIT_ADDR(h, l)  (((PHYS_ADDR)(h) << 32) + (l))
#endif

/*
* sysPhysMemDesc[] is used to initialize the Page Table Entry (PTE) array
* used by the MMU to translate addresses with single page (4k) granularity.
* PTE memory space should not, in general, overlap BAT memory space but
* may be allowed if only Data or Instruction access is mapped via BAT.
*
* Address translations for local RAM, memory mapped PCI bus, the Board Control and
* Status registers, the MPC8260 Internal Memory Map, and local FLASH RAM are set here.
*
* PTEs are held in a Page Table.  Page Table sizes are
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
        /* Vector Table and Interrupt Stack */
        /* Must be sysPhysMemDesc [0] to allow adjustment in sysHwInit() */

        (VIRT_ADDR) LOCAL_MEM_LOCAL_ADRS,
        (PHYS_ADDR) LOCAL_MEM_LOCAL_ADRS,
        LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | TLB_CACHE_MODE | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) CS_A_BASE_ADRS,
        (PHYS_ADDR) CS_A_BASE_ADRS,
        0x10000000,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) 0xB0000000,
        (PHYS_ADDR) 0xB0000000,
        0x10000000,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) CS_C_BASE_ADRS,
        (PHYS_ADDR) CS_C_BASE_ADRS,
        CS_C_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    
    ,
    {
        (VIRT_ADDR) 0xD0000000,
        (PHYS_ADDR) 0xD0000000,
        0x10000000,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        /*
         * CCSBAR
        */
        (VIRT_ADDR) CCSBAR,
        (PHYS_ADDR) CCSBAR,
        0x00100000,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE |
        VM_STATE_MASK_MEM_COHERENCY | VM_STATE_MASK_GUARDED,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT |
        VM_STATE_MEM_COHERENCY | VM_STATE_GUARDED
    }

#ifdef INCLUDE_LBC_SDRAM
    ,
    {
        /* Must be sysPhysMemDesc [2] to allow adjustment in sysHwInit() */
        (VIRT_ADDR) LOCAL_MEM_LOCAL_ADRS2,
	(PHYS_ADDR) LOCAL_MEM_LOCAL_ADRS2,
        LOCAL_MEM_SIZE2,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | VM_STATE_MASK_MEM_COHERENCY ,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | TLB_CACHE_MODE | VM_STATE_MEM_COHERENCY
    }
#endif /* INCLUDE_LBC_SDRAM */

#ifdef INCLUDE_L2_SRAM
    ,
    {
        (VIRT_ADDR) L2SRAM_ADDR,
        (PHYS_ADDR) L2SRAM_ADDR,
        L2SRAM_WINDOW_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
#endif

#ifdef INCLUDE_LBC_CS3
    ,{
        (VIRT_ADDR) LBC_CS3_LOCAL_ADRS,
        (PHYS_ADDR) LBC_CS3_LOCAL_ADRS,
        LBC_CS3_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
#endif /* INCLUDE_LBC_CS3 */

#ifdef INCLUDE_PCI
    ,
    {
        (VIRT_ADDR) PCI_MEM_ADRS,
        (PHYS_ADDR) PCI_MEM_ADRS,
        PCI_MEM_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) PCI_MEMIO_ADRS,
        (PHYS_ADDR) PCI_MEMIO_ADRS,
        PCI_MEMIO_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) PCI_IO_ADRS,
        (PHYS_ADDR) PCI_IO_ADRS,
        PCI_IO_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }

#ifdef INCLUDE_CDS85XX_SECONDARY_PCI
    ,
    {
        (VIRT_ADDR) PCI_MEM_ADRS2,
        (PHYS_ADDR) PCI_MEM_ADRS2,
        PCI_MEM_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) PCI_MEMIO_ADRS2,
        (PHYS_ADDR) PCI_MEMIO_ADRS2,
        PCI_MEMIO_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) PCI_IO_ADRS2,
        (PHYS_ADDR) PCI_IO_ADRS2,
        PCI_IO_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
#endif /* INCLUDE_CDS85XX_SECONDARY_PCI */

#ifdef INCLUDE_CDS85XX_PCIEX
    ,
    {
        (VIRT_ADDR) PCI_MEM_ADRS3,
        (PHYS_ADDR) PCI_MEM_ADRS3,
        PCI_MEM_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) PCI_MEMIO_ADRS3,
        (PHYS_ADDR) PCI_MEMIO_ADRS3,
        PCI_MEMIO_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
    ,
    {
        (VIRT_ADDR) PCI_IO_ADRS3,
        (PHYS_ADDR) PCI_IO_ADRS3,
        PCI_IO_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }
#endif /* INCLUDE_CDS85XX_PCIEX */
#endif /* INCLUDE_PCI */

#ifdef INCLUDE_RAPIDIO_BUS
    ,
    {
    (VIRT_ADDR) RAPIDIO_ADRS,
    (PHYS_ADDR) RAPIDIO_ADRS,
    RAPIDIO_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE ,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT 
    }
#endif /* INCLUDE_RAPIDIO_BUS */
    ,
    {
        (VIRT_ADDR) FLASH_BASE_ADRS,
        (PHYS_ADDR) FLASH_BASE_ADRS,
        TOTAL_FLASH_SIZE,
        VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE | \
        VM_STATE_MASK_GUARDED | VM_STATE_MASK_MEM_COHERENCY,
        VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT | \
        VM_STATE_GUARDED      | VM_STATE_MEM_COHERENCY
    }

};

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

/* Clock Ratio Tables */
#define MAX_VALUE_PLAT_RATIO 32
UINT32 platRatioTable[MAX_VALUE_PLAT_RATIO][2] = 
{
    { 0, 0},
    { 0, 0},
    { 2, 0},
    { 3, 0},
    { 4, 0},
    { 5, 0},
    { 6, 0},
    { 7, 0}, 
    { 8, 0},
    { 9, 0},
    { 10, 0},
    { 0, 0},
    { 12, 0},
    { 0, 0},
    { 0, 0},
    { 0, 0},
    { 16, 0},
    { 0, 0},
    { 0, 0},
    { 0, 0},
    { 20, 0},
    { 0, 0} 
};

#define MAX_VALUE_E500_RATIO 10
UINT32 e500RatioTable[MAX_VALUE_PLAT_RATIO][2] = 
{
    { 0, 0},
    { 0, 0},
    { 1, 0},
    { 3, 1},
    { 2, 0},
    { 5, 1},
    { 3, 0},
    { 7, 1},
    { 4, 0},
    { 9, 1}
};

int   sysBus      = BUS_TYPE_NONE;                /* system bus type (VME_BUS, etc) */
int   sysCpu      = CPU;                /* system CPU type (PPC8260) */
char *sysBootLine = BOOT_LINE_ADRS; /* address of boot line */
char *sysExcMsg   = EXC_MSG_ADRS;   /* catastrophic message area */
int   sysProcNum;           /* processor number of this CPU */
int   sysFlags;             /* boot flags */
char  sysBootHost [BOOT_FIELD_LEN]; /* name of host from which we booted */
char  sysBootFile [BOOT_FIELD_LEN]; /* name of file from which we booted */
BOOL  sysVmeEnable = FALSE;     /* by default no VME */
UINT32  coreFreq;

IMPORT void     mmuE500TlbDynamicInvalidate();
IMPORT void     mmuE500TlbStaticInvalidate();
IMPORT void mmuE500TlbStaticInit (int numDescs, 
                                  TLB_ENTRY_DESC *pTlbDesc, 
                                  BOOL cacheAllow);
IMPORT BOOL     mmuPpcIEnabled;
IMPORT BOOL     mmuPpcDEnabled;
IMPORT void     sysIvprSet(UINT32);

/* forward declarations */

void   sysUsDelay (UINT32);

#ifdef INCLUDE_L1_IPARITY_HDLR_INBSP
    #define _EXC_OFF_L1_PARITY 0x1500
IMPORT void jumpIParity();
IMPORT void sysIvor1Set(UINT32);
UINT32 instrParityCount = 0;
#endif  /* INCLUDE_L1_IPARITY_HDLR_INBSP */


#ifdef INCLUDE_PCI

STATUS sysPciSpecialCycle (int busNo, UINT32 message);
STATUS sysPciConfigRead   (int busNo, int deviceNo, int funcNo,
                           int offset, int width, void * pData);
STATUS sysPciConfigWrite  (int busNo, int deviceNo, int funcNo,
                           int offset, int width, ULONG data);

void   sysPciConfigEnable (int);

#endif /* INCLUDE_PCI */

/* 8260 Reset Configuration Table (From page 9-2 in Rev0 of 8260 book) */
#define END_OF_TABLE 0

UINT32 sysClkFreqGet(void);
UINT32 ppcE500ICACHE_LINE_NUM = (128 * 12);
UINT32 ppcE500DCACHE_LINE_NUM = (128 * 12);

UINT32 ppcE500CACHE_ALIGN_SIZE = 32;

#ifdef INCLUDE_PCI
LOCAL ULONG sysPciConfAddr = PCI_CFG_ADR_REG;   /* PCI Configuration Address */
LOCAL ULONG sysPciConfData = PCI_CFG_DATA_REG;  /* PCI Configuration Data */

#ifdef INCLUDE_GEI8254X_END
LOCAL int   sysPci1SysNum  = CDS85XX_PCI_1_BUS;
#ifdef  INCLUDE_CDS85XX_SECONDARY_PCI
LOCAL int   sysPci2SysNum  = CDS85XX_PCI_2_BUS;
#endif /* INCLUDE_CDS85XX_SECONDARY_PCI */

#ifdef  INCLUDE_CDS85XX_PCIEX
LOCAL int   sysPci3SysNum  = CDS85XX_PCIEX_BUS;
#endif /* INCLUDE_CDS85XX_PCIEX */
#endif /* INCLUDE_GEI8254X_END */

#include <pci/pciIntLib.c>           /* PCI int support */
#include <pci/pciConfigLib.c>        /* pci configuration library */
#if (defined(INCLUDE_PCI_CFGSHOW) && !defined(PRJ_BUILD))
#include <pci/pciConfigShow.c>
#endif /* (defined(INCLUDE_PCI_CFGSHOW) && !defined(PRJ_BUILD)) */
/* use pci auto config */
#include <pci/pciAutoConfigLib.c>    /* automatic PCI configuration */
#include "sysBusPci.c"               /* pciAutoConfig BSP support file */
#include "mot85xxPci.c"


#ifdef INCLUDE_GEI8254X_END
#include "sysGei8254xEnd.c"
#endif  /* INCLUDE_GEI_END */

#endif /* INCLUDE_PCI */

#include "m85xxTimer.c"
#include "sysMotI2c.c"
#include "sysMpc85xxI2c.c"

#ifdef INCLUDE_NV_RAM
    #include <mem/byteNvRam.c>      /* Generic NVRAM routines */
#else
    #include <mem/nullNvRam.c>
#endif /* INCLUDE_NV_RAM */


#ifdef INCLUDE_L1_IPARITY_HDLR
    #include "sysL1ICacheParity.c"
#endif

UINT32 inFullVxWorksImage=FALSE;

#define EXT_VEC_IRQ0            56
#define EXT_NUM_IRQ0            EXT_VEC_IRQ0
#define EXT_MAX_IRQS            200

STATUS  sysIntEnablePIC     (int intNum);   /* Enable i8259 or EPIC */
STATUS  sysCascadeIntEnable      (int intNum);
STATUS  sysCascadeIntDisable     (int intNum);
void    flashTest(VUINT16 *address,VUINT16 *buffer,VINT32 length);

UINT32   baudRateGenClk;

#include "sysEpic.c"


#ifdef INCLUDE_DUART
    #include "sysDuart.c"
#endif

#include "sysL2Cache.c"


#ifdef INCLUDE_TFFS
//    #include "am29lv64xdMtd.c"
#endif /* INCLUDE_TFFS */



//#include "cmdLine.c"

#ifdef INCLUDE_VXBUS
IMPORT void hardWareInterFaceInit();
#endif /* INCLUDE_VXBUS */

#define WB_MAX_IRQS 256


/* defines */

#define ZERO    0

#define SYS_MODEL_8548E   "Freescale CDS MPC8548E - Security Engine"
#define SYS_MODEL_8548    "Freescale CDS MPC8548"
#define SYS_MODEL_8547E   "Freescale CDS MPC8547E - Security Engine"
#define SYS_MODEL_8547    "Freescale CDS MPC8547"
#define SYS_MODEL_8545E   "Freescale CDS MPC8545E - Security Engine"
#define SYS_MODEL_8545    "Freescale CDS MPC8545"
#define SYS_MODEL_8543E   "Freescale CDS MPC8543E - Security Engine"
#define SYS_MODEL_8543    "Freescale CDS MPC8543"


#define SYS_MODEL_E500    "Freescale E500 : Unknown system version" 
#define SYS_MODEL_UNKNOWN "Freescale Unknown processor"

/* needed to enable timer base */
#ifdef INCLUDE_PCI
    #define      M8260_DPPC_MASK	0x0C000000 /* bits 4 and 5 */
    #define      M8260_DPPC_VALUE	0x0C000000 /* bits (4,5) should be (1,0) */
#else
    #define      M8260_DPPC_MASK	0x0C000000 /* bits 4 and 5 */
    #define      M8260_DPPC_VALUE	0x08000000 /* bits (4,5) should be (1,0) */
#endif /*INCLUDE_PCI */

#define DELTA(a,b)                 (abs((int)a - (int)b))
#define HID0_MCP 0x80000000
#define HID1_ABE 0x00001000
#define HID1_ASTME 0x00002000
#define HID1_RXFE  0x00020000


#ifdef INCLUDE_VXBUS
#include <hwif/vxbus/vxBus.h>
#include <../src/hwif/h/busCtlr/m85xxRio.h>
#endif

#ifdef INCLUDE_MOT_TSEC_END
#include "sysNet.c"
#ifndef INCLUDE_VXBUS
#include "sysMotTsecEnd.c"
#endif
#endif

#ifdef INCLUDE_VXBUS
#include "hwconf.c"
#endif

#ifdef INCLUDE_BRANCH_PREDICTION
IMPORT void disableBranchPrediction();
#endif

#ifdef INCLUDE_L2_SRAM
LOCAL void sysL2SramEnable(BOOL both);
#endif /* INCLUDE_L2_SRAM */

#ifdef INCLUDE_SPE
    #include <speLib.h>
IMPORT int       (* _func_speProbeRtn) () ;
#endif /* INCLUDE_SPE */

#ifdef INCLUDE_CACHE_SUPPORT
LOCAL void sysL1CacheQuery();
#endif

UINT32 sysTimerClkFreq = OSCILLATOR_FREQ;

IMPORT  void    sysL1Csr1Set(UINT32);
IMPORT  UINT    sysTimeBaseLGet(void);

LOCAL char * physTop = NULL;
LOCAL char * memTop = NULL;
void bsp_cs_init()
{
    *M85XX_BR1(CCSBAR) = 0xc0001001;   
    *M85XX_OR1(CCSBAR) = 0xf0006f27;    
    WRS_ASM("isync");

    *M85XX_BR2(CCSBAR) = 0xD0000801; /*8bit*/    
    *M85XX_OR2(CCSBAR) = 0xf0006f37;    
    WRS_ASM("isync");

    *M85XX_BR3(CCSBAR) = 0xa0001801;   
    *M85XX_OR3(CCSBAR) = 0xf0006f47;
    WRS_ASM("isync");

    *M85XX_BR4(CCSBAR) = 0xb0001801;   
    *M85XX_OR4(CCSBAR) = 0xf0006f87;    
    WRS_ASM("isync");

    *M85XX_LAWBAR2(CCSBAR) = 0xa0000000 >> LAWBAR_ADRS_SHIFT;
    *M85XX_LAWAR2(CCSBAR)  = LAWAR_ENABLE | LAWAR_TGTIF_LBC | LAWAR_SIZE_256MB;
    WRS_ASM("isync");

    *M85XX_LAWBAR3(CCSBAR) = 0xb0000000 >> LAWBAR_ADRS_SHIFT;
    *M85XX_LAWAR3(CCSBAR)  = LAWAR_ENABLE | LAWAR_TGTIF_LBC | LAWAR_SIZE_256MB;
    WRS_ASM("isync");

    *M85XX_LAWBAR8(CCSBAR) = 0xc0000000 >> LAWBAR_ADRS_SHIFT;
    *M85XX_LAWAR8(CCSBAR)  = LAWAR_ENABLE | LAWAR_TGTIF_LBC | LAWAR_SIZE_256MB;
    WRS_ASM("isync");

    *M85XX_LAWBAR9(CCSBAR) = 0xd0000000 >> LAWBAR_ADRS_SHIFT;
    *M85XX_LAWAR9(CCSBAR)  = LAWAR_ENABLE | LAWAR_TGTIF_LBC | LAWAR_SIZE_256MB;
    WRS_ASM("isync");

}
void ctlLed_bctl(int ledctlfun, unsigned int led)
{
	unsigned int  *LedReg =(unsigned int *)BSP_LIGHT_REG_1 ;	

	switch (ledctlfun)
	{
	case BSP_LED_FUN_LIGHT:
		*LedReg = (*LedReg) & (~(led & BSP_SINGLE_LIGHT_MASK_1)); 
		break;
	case BSP_LED_FUN_DARK:  
		*LedReg = (*LedReg) | (led & BSP_SINGLE_LIGHT_MASK_1) ;
		break;
	case BSP_LED_FUN_FLIP:	
		*LedReg = (*LedReg) ^ (led & BSP_SINGLE_LIGHT_MASK_1) ;
		break;
	default:
		break;
	}	
}

void bsp_led_task()
{
    unsigned long *pulLedReg = (unsigned long volatile*)BSP_LIGHT_REG_1;
    while(1)
    {
        taskDelay(50);
        *pulLedReg &= ~BSP_LED_RED_1;
        *pulLedReg |= BSP_LED_GREEN_1; 
        taskDelay(50);
        *pulLedReg &= ~BSP_LED_GREEN_1;
        *pulLedReg |= BSP_LED_RED_1; 
    }
}
unsigned char bsp_get_pcb_ver()
{
    unsigned long *pulLedReg = (unsigned long volatile*)M85XX_GPINDR;
    unsigned char temp = (unsigned char)((*pulLedReg & 0x00700000) >> 20);
    return (0xA0 | temp);
}
void bsp_gpio_enable()
{
    unsigned long *pulLedReg = (unsigned long volatile*)M85XX_GPIOCR;
    *pulLedReg |= (BSP_GPIO_PCI_OUT_ENABLE | BSP_GPIO_PCI_IN_ENABLE |BSP_GPOUT_ENABLE);
}
void bsp_phy_enable()
{
    unsigned long *pulLedReg = (unsigned long volatile*)M85XX_GPOUTDR;
    *pulLedReg  |= BSP_PHY_ENABLE;
}
void bsp_clear_dog()
{
    unsigned long volatile *pulReg = (unsigned long volatile*)M85XX_GPOUTDR;
    *pulReg &= ~BSP_WTD_CLR; 
    WRS_ASM("nop");
    WRS_ASM("nop");
    WRS_ASM("nop");        
    *pulReg |= BSP_WTD_CLR;
}
void bsp_cpld_release()
{
    unsigned long volatile *pulReg = (unsigned long volatile*)M85XX_GPOUTDR;
    *pulReg |= Cpld_RST;
}
void flash_write_enable(int chipno)
{
    unsigned long *pulLedReg = (unsigned long volatile*)M85XX_GPOUTDR;
	if(chipno == 0)
    *pulLedReg  |= BSP_FLASH_WR;
	else if(chipno == 1)
		*CPLD_REG(CPLD_FLASH_WP) |= BIT0;/*bit 0 pin*/
	else if(chipno == 2)
		*CPLD_REG(CPLD_FLASH_WP) |= BIT1;/*bit 1 pin*/
}

void flash_write_disable(int chipno)
{
    unsigned long *pulLedReg = (unsigned long volatile*)M85XX_GPOUTDR;
	if(chipno == 0)
    *pulLedReg  &= ~BSP_FLASH_WR;   
	else if(chipno == 1)
		*CPLD_REG(CPLD_FLASH_WP) &= ~ BIT0;/*bit 0 pin*/
	else if(chipno == 2)
		*CPLD_REG(CPLD_FLASH_WP) &= ~ BIT1;/*bit 1 pin*/
    
}


#if     defined (INCLUDE_SPE)

/*******************************************************************************
*
* sysSpeProbe - Check if the CPU has SPE unit.
*
* This routine returns OK it the CPU has an SPE unit in it.
* Presently it assumes available.
* 
* RETURNS: OK 
*
* ERRNO: N/A
*/

int  sysSpeProbe (void)
    {
    ULONG regVal;
    int speUnitPresent = OK;

    /* The CPU type is indicated in the Processor Version Register (PVR) */

    regVal = 0;

    switch (regVal)
        {
        case 0:
        default:
            speUnitPresent = OK;
            break;
        }      /* switch  */

    return(speUnitPresent);
    }
#endif  /* INCLUDE_SPE */


/****************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string.
*
* ERRNO: N/A
*/

char * sysModel (void)
    {
    UINT device;
    char* retChar = NULL;
    device = *M85XX_SVR(CCSBAR);

    switch(device & 0xffffff00)
	{
	case 0x80390000:
	    retChar = SYS_MODEL_8548E;
	    break;
	case 0x80310000:
	    retChar = SYS_MODEL_8548;
	    break;
	case 0x80390100:
	    retChar = SYS_MODEL_8547E;
	    break;
	case 0x80390200:
	    retChar = SYS_MODEL_8545E;
	    break;
	case 0x80310200:
	    retChar = SYS_MODEL_8545;
	    break;
	case 0x803A0000:
	    retChar = SYS_MODEL_8543E;
	    break;
	case 0x80320000:
	    retChar = SYS_MODEL_8543;
	    break;
	default:
	    retChar = SYS_MODEL_E500;
	    break;
	}
 

    device = *M85XX_PVR(CCSBAR);
    
    if ((device & 0xfff00000) != 0x80200000)
        retChar =SYS_MODEL_UNKNOWN;

    return(retChar);

    }

/******************************************************************************
*
* sysBspRev - return the BSP version with the revision eg 1.0/<x>
*
* This function returns a pointer to a BSP version with the revision.
* for eg. 1.0/<x>. BSP_REV defined in config.h is concatenated to
* BSP_VERSION and returned.
*
* RETURNS: A pointer to the BSP version/revision string.
*
* ERRNO: N/A
*/

char * sysBspRev (void)
    {
    return(BSP_VERSION BSP_REV);
    }
UINT32 g_sysPhysMemSize = BCTL_SDRAM_512M_SIZE ;

UINT32 sysClkFreqGet
(
void
)
    {
    UINT32  sysClkFreq;
    UINT32 e500Ratio,platRatio;

    platRatio = M85XX_PORPLLSR_PLAT_RATIO(CCSBAR); 

#ifdef FORCE_DEFAULT_FREQ
    return(DEFAULT_SYSCLKFREQ);
#endif

    if ((platRatio>MAX_VALUE_PLAT_RATIO)||(platRatioTable[platRatio][0]==0))
        return(DEFAULT_SYSCLKFREQ); /* A default value better than zero or -1 */

    sysClkFreq = ((UINT32)(OSCILLATOR_FREQ * platRatioTable[platRatio][0]))>>((UINT32)platRatioTable[platRatio][1]);

    e500Ratio = M85XX_PORPLLSR_E500_RATIO(CCSBAR);
    coreFreq = ((UINT32)(sysClkFreq * e500RatioTable[e500Ratio][0]))>>((UINT32)e500RatioTable[e500Ratio][1]);


    return(sysClkFreq);
    }

/*Time Base/Decrementer freq is BUS clock/8*/
UINT32 sysTBFreqGet(void) 
{
	return (sysClkFreqGet() /8) ;
}

/******************************************************************************
*
* sysCpmFreqGet - Determines the CPM Operating Frequency
*
* This routine determines the CPM Operating Frequency.
*
* From page 9-2 Rev. 0  MPC8260  PowerQUICC II User's Manual
*
* RETURNS: CPM clock frequency for the current MOD_CK and MOD_CK_H settings  
*
* ERRNO: N/A
*/

UINT32 sysCpmFreqGet (void)
    {
    UINT32 sysClkFreq = sysClkFreqGet();
    return(sysClkFreq);

    }

/******************************************************************************
*
* sysBaudClkFreq - Obtains frequency of the BRG_CLK in Hz
*
* From page 9-5 in Rev. 0 MPC8260 PowerQUICC II User's Manual
*
*     baud clock = 2*cpm_freq/2^2*(DFBRG+1) where DFBRG = 01
*                = 2*cpm_freq/16
*
* RETURNS: frequency of the BRG_CLK in Hz
*
* ERRNO: N/A
*/

UINT32 sysBaudClkFreq (void)
    {
    UINT32 cpmFreq = sysCpmFreqGet();

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
*
* ERRNO: N/A
*/

void sysHwMemInit (void)
    {
    /* Call sysPhysMemTop() to do memory autosizing if available */

    sysPhysMemTop ();

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
*
* ERRNO: N/A
*/
void sysHwInit (void)
    {


#ifdef INCLUDE_RAPIDIO_BUS
    /* Errata not yet described - required for rapidIO TAS */ 
    *(UINT32*)(CCSBAR + 0x1010) = 0x01040004;
#endif
    bsp_gpio_enable();
    bsp_phy_enable();
    bsp_cpld_release();

    /*输出系统时钟，频率为SYSCLK*/
    *M85XX_CLKOCR(CCSBAR) = 0x80000002;   
    
    sysIvprSet(0x0);

    /* Disable L1 Icache */
    sysL1Csr1Set(vxL1CSR1Get() & ~0x1);


    /* Check for architecture support for 36 bit physical addressing */
#if defined(PPC_e500v2)
    vxHid0Set(_PPC_HID0_MAS7EN|vxHid0Get());
#endif
    /* Enable machine check pin */
    vxHid0Set(HID0_MCP|vxHid0Get());

#ifdef E500_L1_PARITY_RECOVERY
    /* Enable Parity in L1 caches */
    vxL1CSR0Set(vxL1CSR0Get() | _PPC_L1CSR_CPE);
    vxL1CSR1Set(vxL1CSR1Get() | _PPC_L1CSR_CPE);
#endif  /* E500_L1_PARITY_RECOVERY */

    /* enable time base for delay use before DEC interrupt is setup */
    vxHid0Set(vxHid0Get() | _PPC_HID0_TBEN);

    sysTimerClkFreq = sysClkFreqGet()>>3 /* Clock div is 8 */;

#ifdef INCLUDE_AUX_CLK
    sysAuxClkRateSet(127);
#endif

#ifdef INCLUDE_CACHE_SUPPORT
    sysL1CacheQuery(); 
#endif /* INCLUDE_CACHE_SUPPORT */

    /* Initialise L2CTL register */
    vxL2CTLSet(0x28000000,M85XX_L2CTL(CCSBAR));

    /* Need to setup static TLB entries for bootrom or any non-MMU 
     * enabled images */

    mmuE500TlbDynamicInvalidate();
    mmuE500TlbStaticInvalidate();
    mmuE500TlbStaticInit(sysStaticTlbDescNumEnt, &sysStaticTlbDesc[0], TRUE);

#if (!defined(INCLUDE_MMU_BASIC) && !defined(INCLUDE_MMU_FULL))
    mmuPpcIEnabled=TRUE;
    mmuPpcDEnabled=TRUE;
#else /* !defined(INCLUDE_MMU_BASIC) && !defined(INCLUDE_MMU_FULL) */
    if (inFullVxWorksImage==FALSE)
        {
        mmuPpcIEnabled=TRUE;
        mmuPpcDEnabled=TRUE;
        }
    /* Enable I Cache if instruction mmu disabled */
#if (defined(USER_I_CACHE_ENABLE) && !defined(USER_I_MMU_ENABLE))
    mmuPpcIEnabled=TRUE;
#endif /* (defined(USER_I_CACHE_ENABLE) && !defined(USER_I_MMU_ENABLE)) */

#endif /* !defined(INCLUDE_MMU_BASIC) && !defined(INCLUDE_MMU_FULL) */


#if (defined(INCLUDE_L2_CACHE) && defined(INCLUDE_CACHE_SUPPORT))
    vxHid1Set(HID1_ABE); /* Address Broadcast enable */ 
    sysL2CacheInit();
#endif /* INCLUDE_L2_CACHE  && INCLUDE_CACHE_SUPPORT*/

    /* Machine check via RXFE for RIO */
    vxHid1Set(vxHid1Get()| HID1_ASTME | HID1_RXFE); /* Address Stream Enable */

    /* enable the flash window */

//    *M85XX_LAWBAR3(CCSBAR) = FLASH1_BASE_ADRS >> LAWBAR_ADRS_SHIFT;
//    *M85XX_LAWAR3(CCSBAR)  = LAWAR_ENABLE | LAWAR_TGTIF_LBC | LAWAR_SIZE_8MB;

    bsp_cs_init();

    WRS_ASM("isync");

    /* Initialize the Embedded Programmable Interrupt Controller */

    sysEpicInit();

#ifdef INCLUDE_VXBUS
    hardWareInterFaceInit();
#endif /* INCLUDE_VXBUS */


#ifdef INCLUDE_DUART
    sysDuartHwInit ();
#endif


#ifdef INCLUDE_PCI

    /* config pci */

    if (pciConfigLibInit (PCI_MECHANISM_0,(ULONG) sysPciConfigRead,
                          (ULONG) sysPciConfigWrite,(ULONG) sysPciSpecialCycle) != OK)
        {
        sysToMonitor (BOOT_NO_AUTOBOOT);  /* BAIL */
        }

    /*  Initialize PCI interrupt library. */

    if ((pciIntLibInit ()) != OK)
        {
        sysToMonitor (BOOT_NO_AUTOBOOT);
        }

//    if ( *((char*)PCI_AUTO_CONFIG_ADRS) == FALSE )
        {
        mot85xxBridgeInit();

#ifdef INCLUDE_PCI_AUTOCONF
//        sysPciAutoConfig();
#endif
//        *((char*)PCI_AUTO_CONFIG_ADRS) = TRUE;
        }


#endif /* INCLUDE_PCI */


#ifdef E500_L1_PARITY_RECOVERY
    vxIvor1Set(_EXC_OFF_L1_PARITY);
#endif  /* E500_L1_PARITY_RECOVERY */
#ifdef INCLUDE_L1_IPARITY_HDLR
    installL1ICacheParityErrorRecovery();
#endif /* INCLUDE_L1_IPARITY_HDLR */

    /*
     * The power management mode is initialized here. Reduced power mode
     * is activated only when the kernel is idle (cf vxPowerDown).
     * Power management mode is selected via vxPowerModeSet().
     * DEFAULT_POWER_MGT_MODE is defined in config.h.
     */

#if defined(INCLUDE_L2_SRAM) 
#if (defined(INCLUDE_L2_CACHE) && defined(INCLUDE_CACHE_SUPPORT))
    sysL2SramEnable(TRUE);
#elif (defined(INCLUDE_L2_SRAM))
    sysL2SramEnable(FALSE);
#endif
#endif

    CACHE_PIPE_FLUSH();

    vxPowerModeSet (DEFAULT_POWER_MGT_MODE);
    }

#ifdef INCLUDE_L2_SRAM
/*************************************************************************
*
* sysL2SramEnable - enables L2SRAM if L2SRAM only
*
* This routine enables L2SRAM if L2SRAM only or initializes blk 
* size etc and leaves the rest to L2 cache code.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
LOCAL void sysL2SramEnable
(
BOOL both
)
    {
    volatile int l2CtlVal;

    /* if INCLUDE_L2_CACHE and CACHE_SUPPORT */
    /* if ((L2_SRAM_SIZE + L2_CACHE_SIZE) > l2Siz) */
    /* Setup Windows for L2SRAM */    
#ifndef REV2_SILICON
    *(M85XX_L2SRBAR0(CCSBAR)) = (UINT32)(L2SRAM_ADDR >> 4) & M85XX_L2SRBAR_ADDR_MSK);
#else
     *(M85XX_L2SRBAR0(CCSBAR)) = (UINT32)(L2SRAM_ADDR & M85XX_L2SRBAR_ADDR_MSK);
#endif
    /* Get present value */
    l2CtlVal = vxL2CTLGet(M85XX_L2CTL(CCSBAR));

    /* Disable L2CTL initially to allow changing of block size */
    l2CtlVal &=(~M85XX_L2CTL_L2E_MSK);
    vxL2CTLSet(l2CtlVal,M85XX_L2CTL(CCSBAR));
    l2CtlVal &= ~M85XX_L2CTL_L2BLKSIZ_MSK;
    l2CtlVal &= ~M85XX_L2CTL_L2SRAM_MSK;

    if (both == TRUE)
        {
        /* Setup size of SRAM */
        l2CtlVal |= (L2SIZ_256KB << M85XX_L2CTL_L2BLKSIZ_BIT) | 
                    (0x2 << M85XX_L2CTL_L2SRAM_BIT);
        }
    else
        {
        l2CtlVal |= (L2SIZ_512KB << M85XX_L2CTL_L2BLKSIZ_BIT) | 
                    (0x1 << M85XX_L2CTL_L2SRAM_BIT);
        }

    /* Setup L2CTL for SRAM */
    vxL2CTLSet(l2CtlVal,M85XX_L2CTL(CCSBAR));

    if (both == FALSE)
        {
        /* This is done here so L2SRAM is set before enable */
        l2CtlVal |= M85XX_L2CTL_L2E_MSK; /* No cache so go ahead and enable */
        /* Enable L2CTL for SRAM */
        vxL2CTLSet(l2CtlVal,M85XX_L2CTL(CCSBAR));
        }

    }
#endif /* INCLUDE_L2_SRAM */

/**************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of memory.
*
* RETURNS: The address of the top of physical memory.
*
* ERRNO: N/A
*
* SEE ALSO: sysMemTop()
*/

char * sysPhysMemTop (void)
    {

    if (physTop == NULL)
        {
        physTop = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
        }

    return(physTop) ;
    }

/***************************************************************************
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
*
* ERRNO: N/A
*/

char * sysMemTop (void)
    {

    if (memTop == NULL)
        {
        memTop = sysPhysMemTop () - USER_RESERVED_MEM;

#ifdef INCLUDE_EDR_PM

        /* account for ED&R persistent memory */

        memTop = memTop - PM_RESERVED_MEM;
#endif

        }

    return memTop;
    }

/**************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  Normally, it is called
* only by reboot()--which services ^X--and bus errors at interrupt level.
* However, in some circumstances, the user may wish to introduce a
* <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*
* ERRNO: N/A
*/
extern UINT32 g_Bsprebootdebug;

STATUS sysToMonitor
    (
    int startType   /* parameter passed to ROM to tell it how to boot */
    )
    {

    FUNCPTR pRom = NULL;

    if(TRUE == g_Bsprebootdebug )
    {
        pRom = (FUNCPTR) (0xFFE00100 + 4);   /* Warm reboot */
    }
    else
    {
        pRom = (FUNCPTR) (ROM_TEXT_ADRS + 4);   /* Warm reboot */
    }

    intLock();	
	
#ifdef INCLUDE_BRANCH_PREDICTION
    disableBranchPrediction();
#endif /* INCLUDE_BRANCH_PREDICTION */

#ifdef INCLUDE_CACHE_SUPPORT
    cacheDisable(INSTRUCTION_CACHE);
    cacheDisable(DATA_CACHE);
#endif
    sysClkDisable();


#ifdef INCLUDE_AUX_CLK
    sysAuxClkDisable();
#endif

    vxMsrSet(0);
    /* Clear unnecessary TLBs */
    mmuE500TlbDynamicInvalidate();
    mmuE500TlbStaticInvalidate();

    (*pRom) (startType);    /* jump to bootrom entry point */

    return(OK);    /* in case we ever continue from ROM monitor */
    }

/******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysHwInit2 (void)
    {

    vxMsrSet(vxMsrGet() & 0xFFFFFFCF);/*非常重要，必须设置0*/
#ifdef  INCLUDE_VXBUS
    vxbDevInit();
#endif /* INCLUDE_VXBUS */


    excIntConnect ((VOIDFUNCPTR *) _EXC_OFF_DECR,
                   (VOIDFUNCPTR) sysClkInt);

    sysClkEnable();

#ifdef INCLUDE_AUX_CLK
    excIntConnect ((VOIDFUNCPTR *) _EXC_OFF_FIT, (VOIDFUNCPTR) sysAuxClkInt);
#endif

    /* This was previously reqd for errata workaround #29, the workaround 
     * has been replaced with patch for spr99776, so it now serves as an 
     * example of implementing an l1 instruction parity handler 
     */  
#ifdef INCLUDE_L1_IPARITY_HDLR_INBSP
    memcpy((void*)_EXC_OFF_L1_PARITY, (void *)jumpIParity, sizeof(INSTR));
    cacheTextUpdate((void *)_EXC_OFF_L1_PARITY, sizeof(INSTR));
    sysIvor1Set(_EXC_OFF_L1_PARITY);
    cacheDisable(INSTRUCTION_CACHE);
    vxL1CSR1Set(vxL1CSR1Get() | _PPC_L1CSR_CPE);
    cacheEnable(INSTRUCTION_CACHE);
#endif  /* INCLUDE_L1_IPARITY_HDLR_INBSP */

    /* initialize the EPIC interrupts */
    sysEpicIntrInit ();


    /* initialize serial interrupts */

#if defined(INCLUDE_DUART)
    sysSerialHwInit2 ();
#endif /* INCLUDE_DUART */

#if     defined (INCLUDE_SPE)
    _func_speProbeRtn = sysSpeProbe;
#endif  /* INCLUDE_SPE */


#ifdef INCLUDE_PCI

#ifdef INCLUDE_GEI8254X_END

    sysPciConfigEnable (CDS85XX_PCI_1_BUS);
    pciConfigForeachFunc (0, FALSE, (PCI_FOREACH_FUNC) sys8254xDeviceCheck, (void *)&sysPci1SysNum);

#ifdef  INCLUDE_CDS85XX_SECONDARY_PCI
#ifdef INCLUDE_GEI8254X_END
    sysPciConfigEnable (CDS85XX_PCI_2_BUS);

    pciConfigForeachFunc (0, FALSE, (PCI_FOREACH_FUNC) sys8254xDeviceCheck, (void *)&sysPci2SysNum); 
#endif
#endif  /*  INCLUDE_CDS85XX_SECONDARY_PCI */

#ifdef  INCLUDE_CDS85XX_PCIEX
#ifdef INCLUDE_GEI8254X_END
    sysPciConfigEnable (CDS85XX_PCIEX_BUS);

    pciConfigForeachFunc (1, FALSE, (PCI_FOREACH_FUNC) sys8254xDeviceCheck, (void *)&sysPci3SysNum); 
#endif /* INCLUDE_GEI8254X_END */
#endif  /*  INCLUDE_CDS85XX_SECONDARY_PCI */

#endif /* INCLUDE_GEI8254X_END */
#endif /* INCLUDE_PCI */

#ifdef  INCLUDE_VXBUS
    taskSpawn("devConnect",0,0,10000,vxbDevConnect,0,0,0,0,0,0,0,0,0,0);
#endif /* INCLUDE_VXBUS */

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
* ERRNO: N/A
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)
    {
    return(sysProcNum);
    }

/******************************************************************************
*
* sysProcNumSet - set the processor number
*
* This routine sets the processor number for the CPU board.  Processor numbers
* should be unique on a single backplane.
*
* Not applicable for the bus-less 8260Ads.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int     procNum         /* processor number */
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
* ERRNO: N/A
*
* SEE ALSO: sysBusToLocalAdrs()
*/

STATUS sysLocalToBusAdrs
    (
    int     adrsSpace,  /* bus address space where busAdrs resides */
    char *  localAdrs,  /* local address to convert */ 
    char ** pBusAdrs    /* where to return bus address */ 
    )
    {

    *pBusAdrs = localAdrs;

    return(OK);
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
* ERRNO: N/A
*
* SEE ALSO: sysLocalToBusAdrs()
*/

STATUS sysBusToLocalAdrs
    (
    int     adrsSpace,  /* bus address space where busAdrs resides */
    char *  busAdrs,    /* bus address to convert */
    char ** pLocalAdrs  /* where to return local address */
    )
    {

    *pLocalAdrs = busAdrs;

    return(OK);
    }



#ifdef INCLUDE_PCI 	/* board level PCI routines */

/*******************************************************************************
* sysPciConfigEnable -  enable PCI 1 or PCI 2 bus configuration
*
* This function enables PCI 1 or PCI 2 bus configuration
*  
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysPciConfigEnable 
    (
    int pciHost
    )
    {
    int level;

    level = intLock ();

    if (pciHost == CDS85XX_PCI_2_BUS)
        {
        sysPciConfAddr = PCI2_CFG_ADR_REG;   /* PCI Configuration Address */
        sysPciConfData = PCI2_CFG_DATA_REG;  /* PCI Configuration Data */
        }
    else if (pciHost == CDS85XX_PCIEX_BUS)
        {
        sysPciConfAddr = PCIEX_CFG_ADR_REG;   /* PCI Configuration Address */
        sysPciConfData = PCIEX_CFG_DATA_REG;  /* PCI Configuration Data */
        }
    else /* default is for PCI 1 host */
        {
        sysPciConfAddr = PCI_CFG_ADR_REG;   /* PCI Configuration Address */
        sysPciConfData = PCI_CFG_DATA_REG;  /* PCI Configuration Data */
        }

    WRS_ASM("sync;eieio");

    intUnlock (level);
    }

/*******************************************************************************
*
* sysPciSpecialCycle - generate a special cycle with a message
*
* This routine generates a special cycle with a message.
*
* \NOMANUAL
*
* RETURNS: OK
*
* ERRNO: N/A
*/

STATUS sysPciSpecialCycle
    (
    int     busNo,
    UINT32  message
    )
    {
    int deviceNo    = 0x0000001f;
    int funcNo      = 0x00000007;

    pciRegWrite ((UINT32 *)sysPciConfAddr,
                 (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                 0x80000000);

    PCI_OUT_LONG (sysPciConfData, message);

    return(OK);
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
* \NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*
* ERRNO: N/A
*/

STATUS sysPciConfigRead
    (
    int busNo,    /* bus number */
    int deviceNo, /* device number */
    int funcNo,   /* function number */
    int offset,   /* offset into the configuration space */
    int width,    /* width to be read */
    void * pData /* data read from the offset */
    )
    {
    UINT8  retValByte = 0;
    UINT16 retValWord = 0;
    UINT32 retValLong = 0;
    STATUS retStat = ERROR;

    if ((busNo == 0) && (deviceNo == 0x1f) /* simulator doesn't like this device being used */)
        return(ERROR);


    /* This is for PCI Express */
    if(sysPciConfAddr == PCIEX_CFG_ADR_REG)
	{
	if((busNo == 1) && (deviceNo > 0))
	    return(ERROR);
	}

    switch (width)
        {
        case 1: /* byte */
            pciRegWrite ((UINT32 *)sysPciConfAddr,
                         (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                         (offset & 0xffc) | 0x80000000 | ((offset & 0xf00) << 16)) ;

            retValByte = PCI_IN_BYTE (sysPciConfData + (offset & 0x3));
            *((UINT8 *)pData) = retValByte;
            retStat = OK;
            break;

        case 2: /* word */
            pciRegWrite ((UINT32 *)sysPciConfAddr,
                         (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                         (offset & 0xfc) | 0x80000000 | ((offset & 0xf00) << 16));

            retValWord = PCI_IN_WORD (sysPciConfData + (offset & 0x2));
            *((UINT16 *)pData) = retValWord;
            retStat = OK;
            break;

        case 4: /* long */
            pciRegWrite ((UINT32 *)sysPciConfAddr,
                         (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                         (offset & 0xfc) | 0x80000000 | ((offset & 0xf00) << 16));
            retValLong = PCI_IN_LONG (sysPciConfData);
            *((UINT32 *)pData) = retValLong;
            retStat = OK;
            break;

        default:
            retStat = ERROR;
            break;
        }

    return(retStat);
    }

/*******************************************************************************
*
* sysPciConfigWrite - write to the PCI configuration space
*
* This routine writes either a byte, word or a long word specified by
* the argument <width>, to the PCI configuration space
* This routine works around a problem in the hardware which hangs
* PCI bus if device no 12 is accessed from the PCI configuration space.
* Errata PCI-Express requires RMW to ensure always write 4byte aligned.
*
* \NOMANUAL
*
* RETURNS: OK, or ERROR if this library is not initialized
*
* ERRNO: N/A
*/

STATUS sysPciConfigWrite
    (
    int busNo,    /* bus number */
    int deviceNo, /* device number */
    int funcNo,   /* function number */
    int offset,   /* offset into the configuration space */
    int width,    /* width to write */
    ULONG data    /* data to write */
    )
    {
 /* PCI-Express errata RMW workaround should work for all PCI 
  * Requires always to do 4 byte read/write  
  */ 
   ULONG data2,mask;

   /* Establish the required mask based on width and offset */
    switch(width)
	{
	case 1:
	    mask=0xff;
	    mask=~(mask << ((offset&0x3)*8));
	    break;
	case 2:
	    mask=0xffff;
	    mask=~(mask << ((offset&0x3)*8));
	    break;
	case 4:
	    mask=0;
	    break;
	default:
	    return(ERROR);
	}
    
    data2 = pciConfigRead(busNo,deviceNo,funcNo,4,(offset & ~0x3));
    
    data2 &= mask;

    data2 |= data << ((offset&0x3)*8); /* Overwite only part of word reqd */

    data = data2;             /* final 32 bit value to write */
    width = 4;                /* always do 32 bit write */
    offset = (offset & ~0x3); /* align by 32 bits */


    if ((busNo == 0) && (deviceNo == 0x1f))
        return(ERROR);

    switch (width)
        {
        case 1: /* byte */
            pciRegWrite ((UINT32 *)sysPciConfAddr,
                         (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                         (offset & 0xfc) | 0x80000000 | ((offset & 0xf00) << 16));
            PCI_OUT_BYTE ((sysPciConfData + (offset & 0x3)), data);
            break;

        case 2: /* word */
            pciRegWrite ((UINT32 *)sysPciConfAddr,
                         (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                         (offset & 0xfc) | 0x80000000 | ((offset & 0xf00) << 16));
            PCI_OUT_WORD ((sysPciConfData + (offset & 0x2)), data);
            break;

        case 4: /* long */
            pciRegWrite ((UINT32 *)sysPciConfAddr,
                         (UINT32)pciConfigBdfPack (busNo, deviceNo, funcNo) |
                         (offset & 0xfc) | 0x80000000 | ((offset & 0xf00) << 16));
            PCI_OUT_LONG (sysPciConfData, data);
            break;

        default:
            return(ERROR);

        }
    return(OK);
    }
#endif /* INCLUDE_PCI */

/******************************************************************************
*
* sysUsDelay - delay at least the specified amount of time (in microseconds)
*
* This routine will delay for at least the specified amount of time using the
* lower 32 bit "word" of the Time Base register as the timer.  
*
* NOTE:  This routine will not relinquish the CPU; it is meant to perform a
* busy loop delay.  The minimum delay that this routine will provide is
* approximately 10 microseconds.  The maximum delay is approximately the
* size of UINT32; however, there is no roll-over compensation for the total
* delay time, so it is necessary to back off two times the system tick rate
* from the maximum.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysUsDelay
    (
    UINT32    delay        /* length of time in microsec to delay */
    )
    {
    register UINT baselineTickCount;
    register UINT curTickCount;
    register UINT terminalTickCount;
    register int actualRollover = 0;
    register int calcRollover = 0;
    UINT ticksToWait;
    UINT requestedDelay;
    UINT oneUsDelay;

    /* Exit if no delay count */

    if ((requestedDelay = delay) == 0)
        return;

    /*
     * Get the Time Base Lower register tick count, this will be used
     * as the baseline.
     */

    baselineTickCount = sysTimeBaseLGet();

    /*
     * Calculate number of ticks equal to 1 microsecond
     *
     * The Time Base register and the Decrementer count at the same rate:
     * once per 8 System Bus cycles.
     *
     * e.g., 199999999 cycles     1 tick      1 second            25 ticks
     *       ----------------  *  ------   *  --------         ~  --------
     *       second               8 cycles    1000000 microsec    microsec
     */

    /* add to round up before div to provide "at least" */
    oneUsDelay = ((sysTimerClkFreq + 1000000) / 1000000);

    /* Convert delay time into ticks */

    ticksToWait = requestedDelay * oneUsDelay;

    /* Compute when to stop */

    terminalTickCount = baselineTickCount + ticksToWait;

    /* Check for expected rollover */

    if (terminalTickCount < baselineTickCount)
        {
        calcRollover = 1;
        }

    do
        {

        /*
         * Get current Time Base Lower register count.
         * The Time Base counts UP from 0 to
         * all F's.
         */

        curTickCount = sysTimeBaseLGet();

        /* Check for actual rollover */

        if (curTickCount < baselineTickCount)
            {
            actualRollover = 1;
            }

        if (((curTickCount >= terminalTickCount)
             && (actualRollover == calcRollover)) ||
            ((curTickCount < terminalTickCount)
             && (actualRollover > calcRollover)))
            {

            /* Delay time met */

            break;
            }
        }
    while (TRUE); /* breaks above when delay time is met */
    }


void sysMsDelay
    (
    UINT      delay                   /* length of time in MS to delay */
    )
    {
    sysUsDelay ( (UINT32) delay * 1000 );
    }

void bsp_cycle_delay
    (
    UINT        delay            /* length of time in MS to delay */
    )
{
    sysMsDelay(delay);
}
/*********************************************************************
*
* sysDelay - Fixed 1ms delay. 
*
* This routine consumes 1ms of time. It just calls sysMsDelay.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysDelay (void)
    {
    sysMsDelay (1);
    }

/***************************************************************************
*
* sysIntConnect - connect the BSP interrupt to the proper epic/i8259 handler.
*
* This routine checks the INT level and connects the proper routine.
* pciIntConnect() or intConnect().
*
* RETURNS:
* OK, or ERROR if the interrupt handler cannot be built.
*
* ERRNO: N/A
*/

STATUS sysIntConnect
    (
    VOIDFUNCPTR *vector,        /* interrupt vector to attach to     */
    VOIDFUNCPTR routine,        /* routine to be called              */
    int parameter               /* parameter to be passed to routine */
    )
    {
    int tmpStat = ERROR;
    UINT32 read;

    if (((int) vector < 0) || ((int) vector >= EXT_VEC_IRQ0 + EXT_MAX_IRQS))
        {
        logMsg ("Error in sysIntConnect: out of range vector = %d.\n",
                (int)vector,2,3,4,5,6);

        return(ERROR);
        }

    if (vxMemProbe ((char *) routine, VX_READ, 4, (char *) &read) != OK)
        {
        logMsg ("Error in sysIntConnect: Cannot access routine.\n",
                1,2,3,4,5,6);
        return(ERROR);
        }

    if ((int) vector < EXT_VEC_IRQ0)
        {
        tmpStat = intConnect (vector, routine, parameter);
        }
    else
        {
        /* call external int controller connect */
        /* tmpStat = cascadeIntConnect (vector, routine, parameter); */
        }

    if (tmpStat == ERROR)
        {
        logMsg ("Error in sysIntConnect: Connecting vector = %d.\n",
                (int)vector,2,3,4,5,6);
        }

    return(tmpStat);
    }

#if 0
/***************************************************************************
*
* sysCascadeIntConnect - connect an external controller interrupt
*
* This function call is used to connect an interrupt outside of the MPC8540 
* PIC.
*
* RETURNS: OK or ERROR if unable to connect interrupt.
*
* ERRNO: N/A
*/

STATUS sysCascadeIntConnect
    (
    VOIDFUNCPTR *vector,        /* interrupt vector to attach to     */
    VOIDFUNCPTR routine,        /* routine to be called              */
    int parameter               /* parameter to be passed to routine */
    )
    {
    return(ERROR);
    }
#endif /* 0 */


/*******************************************************************************
*
* sysIntEnable - enable an interrupt
*
* This function call is used to enable an interrupt.
*
* RETURNS: OK or ERROR if unable to enable interrupt.
*
* ERRNO: N/A
*/

STATUS sysIntEnable
    (
    int intNum
    )
    {
    int tmpStat = ERROR;
    if (((int) intNum < 0) || ((int) intNum >= EXT_NUM_IRQ0 + EXT_MAX_IRQS))
        {
        logMsg ("Error in sysIntEnable: Out of range intNum = %d.\n",
                (int)intNum,2,3,4,5,6);

        return(ERROR);
        }

    if ((int) intNum < EXT_NUM_IRQ0)
        {
        tmpStat = intEnable (intNum);
        }
    else
        {
        /* call external int controller connect */
        tmpStat = sysCascadeIntEnable (intNum - EXT_NUM_IRQ0);
        }

	
    if (tmpStat == ERROR)
        {
        logMsg ("Error in sysIntEnable: intNum = %d.\n",
                (int)intNum,2,3,4,5,6);
        }

    return(tmpStat);
    }

/****************************************************************************
*
* sysCascadeIntEnable - enable an external controller interrupt
*
* This function call is used to enable an interrupt outside of the MPC8540 PIC.
*
* RETURNS: OK or ERROR if unable to enable interrupt.
*
* ERRNO: N/A
*/

STATUS sysCascadeIntEnable
    (
    int intNum
    )
    {
    return(ERROR);
    }

/****************************************************************************
*
* sysIntDisable - disable an interrupt
*
* This function call is used to disable an interrupt.
*
* RETURNS: OK or ERROR if unable to disable interrupt.
*
* ERRNO: N/A
*/

STATUS sysIntDisable
    (
    int intNum
    )
    {
    int tmpStat = ERROR;
    if (((int) intNum < 0) || ((int) intNum >= EXT_NUM_IRQ0 + EXT_MAX_IRQS))
        {
        logMsg ("Error in sysIntDisable: Out of range intNum = %d.\n",
                (int)intNum,2,3,4,5,6);

        return(ERROR);
        }

    if ((int) intNum < EXT_NUM_IRQ0)
        {
        tmpStat = intDisable (intNum);
        }
    else
        {
        /* call external int controller connect */
        tmpStat = sysCascadeIntDisable (intNum - EXT_NUM_IRQ0);
        }


    if (tmpStat == ERROR)
        {
        logMsg ("Error in sysIntDisable: intNum = %d.\n",
                (int)intNum,2,3,4,5,6);
        }

    return(tmpStat);
    }

/**************************************************************************
*
* sysCascadeIntDisable - disable an external controller interrupt
*
* This function call is used to disable an interrupt outside of the MPC8540 PIC.
*
* RETURNS: OK or ERROR if unable to disable interrupt.
*
* ERRNO: N/A
*/

STATUS sysCascadeIntDisable
    (
    int intNum
    )
    {
    return(ERROR);
    }

#ifdef INCLUDE_CACHE_SUPPORT
/***********************************************************************
* 
* sysL1CacheQuery - configure L1 cache size and alignment
*
* Populates L1 cache size and alignment from configuration registers.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void sysL1CacheQuery(void)
    {
    UINT32 temp;
    UINT32 align;
    UINT32 cachesize;

    temp = vxL1CFG0Get();

    cachesize = (temp & 0xFF) << 10;

    align = (temp >> 23) & 0x3;


    switch (align)
        {
        case 0:
            ppcE500CACHE_ALIGN_SIZE=32;
            break;
        case 1:
            ppcE500CACHE_ALIGN_SIZE=64;
            break;
        default:
            ppcE500CACHE_ALIGN_SIZE=32;
            break;
        }       

    ppcE500DCACHE_LINE_NUM = (cachesize / ppcE500CACHE_ALIGN_SIZE);
    ppcE500ICACHE_LINE_NUM = (cachesize / ppcE500CACHE_ALIGN_SIZE);

    /* The core manual suggests for a 32 byte cache line and 8 lines per set 
       we actually need 12 unique address loads to flush the set. 
       The number of lines to flush should be ( 3/2 * cache lines ) */
    ppcE500DCACHE_LINE_NUM = (3*ppcE500DCACHE_LINE_NUM)>>1;
    ppcE500ICACHE_LINE_NUM = (3*ppcE500ICACHE_LINE_NUM)>>1;

    }

#endif /* INCLUDE_CACHE_SUPPORT */

/***************************************************************************
*
* saveExcMsg - write exception message to save area for catastrophic error
*
* The message will be displayed upon rebooting with a bootrom.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void saveExcMsg 
    (
    char *errorMsg
    )
    {
    strcpy ((char *)EXC_MSG_OFFSET, errorMsg);
    }


void chipErrataCpu29Print(void)
    {
    saveExcMsg("Silicon fault detected, possible machine state corruption.\nSystem rebooted to limit damage.");
    }



/***************************************************************************
*
* vxImmrGet - get the CPM DPRAM base address
*
* This routine returns the CPM DP Ram base address for CPM device drivers.
*
* RETURNS:
*
* ERRNO: N/A
*/

UINT32 vxImmrGet(void)
    {
    return(CCSBAR + 0x80000);
    }

/***************************************************************************
*
* sysGetPeripheralBase   - Provides CCSRBAR address fro security engine 
*                          drivers.
*
* RETURNS: CCSRBAR value
*
* ERRNO: N/A
*/
UINT32 sysGetPeripheralBase()
    {
    return(CCSBAR);
    }

#if defined (_GNU_TOOL)
void sysIntHandler (void)
    {
    }

void vxDecInt (void)
    {
    }

int excRtnTbl = 0;
#endif  /* _GNU_TOOL */


#if 1

/***************************************************************************
*
* coreLbcShow - Show routine for local bus controller
* 
* This routine shows the local bus controller registers.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void coreLbcShow(void)
    {
    VINT32 tmp, tmp2;

    tmp = * (VINT32 *) M85XX_BR0(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR0(CCSBAR);
    printf("Local bus BR0 = 0x%x\tOR0 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR1(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR1(CCSBAR);
    printf("Local bus BR1 = 0x%x\tOR1 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR2(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR2(CCSBAR);
    printf("Local bus BR2 = 0x%x\tOR2 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR3(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR3(CCSBAR);
    printf("Local bus BR3 = 0x%x\tOR3 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR4(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR4(CCSBAR);
    printf("Local bus BR4 = 0x%x\tOR4 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR5(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR5(CCSBAR);
    printf("Local bus BR5 = 0x%x\tOR5 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR6(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR6(CCSBAR);
    printf("Local bus BR6 = 0x%x\tOR6 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_BR7(CCSBAR);
    tmp2 = * (VINT32 *) M85XX_OR7(CCSBAR);
    printf("Local bus BR7 = 0x%x\tOR7 = 0x%x\n", tmp, tmp2);

    tmp = * (VINT32 *) M85XX_LBCR(CCSBAR);
    printf("Local bus LBCR = 0x%x\n", tmp);

    tmp = * (VINT32 *) M85XX_LCRR(CCSBAR);
    printf("Local bus LCRR = 0x%x\n", tmp);
    }


    #define xbit0(x, n)    ((x & (1 << (31 - n))) >> (31 - n))  /* 0..31 */
    #define xbit32(x, n)   ((x & (1 << (63 - n))) >> (63 - n))  /* 32..63 */
    #define onoff0(x, n)   xbit0(x, n) ? "ON", "OFF"
    #define onoff32(x, n)  xbit32(x, n) ? "ON", "OFF"

/***************************************************************************
*
* coreShow - Show routine for core registers
*
* This routine shows the core registers.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*/

void coreShow(void)
    {
    VUINT32 tmp, tmp2;

    tmp = vxMsrGet();
    printf("MSR - 0x%x\n", tmp);
    printf("      UCLE-%x SPE-%x WE-%x CE-%x EE-%x PR-%x ME-%x\n",
           xbit32(tmp,37), xbit32(tmp,38), xbit32(tmp,45), xbit32(tmp,46),
           xbit32(tmp,48), xbit32(tmp,49), xbit32(tmp,51));
    printf("      UBLE-%x DE-%x IS-%x DS-%x PMM-%x\n",
           xbit32(tmp,53), xbit32(tmp,54), xbit32(tmp,58), xbit32(tmp,59),
           xbit32(tmp,61));
    tmp = vxHid0Get();
    tmp2 = vxHid1Get();
    printf("HID0 = 0x%x, HID1 = 0x%x\n", tmp, tmp2);
    tmp = vxL1CSR0Get();
    printf("L1CSR0: cache is %s - 0x%x\n", tmp&1?"ON":"OFF", tmp);
    tmp = vxL1CSR1Get();
    printf("L1CSR1: Icache is %s - 0x%x\n", tmp&1?"ON":"OFF", tmp);
    tmp = vxL1CFG0Get();
    tmp2 = vxL1CFG1Get();
    printf("L1CFG0 = 0x%x, L1CFG1 = 0x%x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x20000);
    printf("L2CTL - 0x%x\n", tmp);
    printf("        l2 is %s\n", tmp&0x80000000?"ON":"OFF");
    printf("        l2siz-%x l2blksz-%x l2do-%x l2io-%x\n",
           (xbit0(tmp,2)<<1)|xbit0(tmp,3), (xbit0(tmp,4)<<1)|xbit0(tmp,5),
           xbit0(tmp,9), xbit0(tmp,10));
    printf("        l2pmextdis-%x l2intdis-%x l2sram-%x\n",
           xbit0(tmp,11), xbit0(tmp,12),
           (xbit0(tmp,13)<<2)|(xbit0(tmp,14)<<1)|xbit0(tmp,15));
    tmp = *(VUINT32 *) (CCSBAR + 0x20100);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x20108);
    printf("L2SRBAR0 - 0x%x\n", tmp);
    printf("L2SRBAR1 - 0x%x\n", tmp2);

    printf("Core Freq = %3d Hz\n",coreFreq);
    printf("CCB Freq = %3d Hz\n",sysClkFreqGet());
    printf("PCI Freq = %3d Hz\n",OSCILLATOR_FREQ);
    printf("DDR Freq = %3d Hz\n",sysClkFreqGet()>>1);
    tmp = *(VUINT32 *) (CCSBAR + 0x0c08);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0c10);
    printf("LAWBAR0 = 0x%8x\t LAWAR0 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0c28);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0c30);
    printf("LAWBAR1 = 0x%8x\t LAWAR1 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0c48);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0c50);
    printf("LAWBAR2 = 0x%8x\t LAWAR2 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0c68);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0c70);
    printf("LAWBAR3 = 0x%8x\t LAWAR3 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0c88);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0c90);
    printf("LAWBAR4 = 0x%8x\t LAWAR4 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0ca8);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0cb0);
    printf("LAWBAR5 = 0x%8x\t LAWAR5 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0cc8);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0cd0);
    printf("LAWBAR6 = 0x%8x\t LAWAR6 = 0x%8x\n", tmp, tmp2);
    tmp = *(VUINT32 *) (CCSBAR + 0x0ce8);
    tmp2 = *(VUINT32 *) (CCSBAR + 0x0cf0);
    printf("LAWBAR7 = 0x%8x\t LAWAR7 = 0x%8x\n", tmp, tmp2);

    }
#endif
#define	__BSP_DRIVER_CPM_TIMER___


#define COUNTER_100us 6666
#define EPIC_TM_BASE_COUNT_REG(timer_no)  	(EPIC_TM0_BASE_COUNT_REG + 0x40 * (timer_no))
#define EPIC_GTM_INT_VEC(timer_no)   (EPIC_GTM0_INT_VEC + timer_no)
#define EPIC_TIMER_NUM   4 
static int  timer_connected[EPIC_TIMER_NUM] = {FALSE , FALSE ,FALSE ,FALSE} ;
STATUS bsp_timer_set(int timer_no ,  int action  , int timer_value , VOIDFUNCPTR timer_handle , int handle_param)
{
	
	if(timer_no <= 0 ||  timer_no > EPIC_TIMER_NUM)
	{
		printf("timer_no  = %d( 1<= timer_value <= 6) , unsupport timer number\n" , timer_no);
		return ERROR ;
	}

	if(action && ((timer_value <= 0) || (timer_value > 100000) ||  (timer_handle == NULL)))
	{
    	printf("bsp_timer1_set param error , timer_value  = %d( 0 < timer_value <1000) ,timer_handle = 0x%x\n" 
    		, timer_value,(int)timer_handle );
    	return ERROR ;
	}
		
	if (action)
	{
		sysEpicRegWrite(EPIC_TM_BASE_COUNT_REG(timer_no), COUNTER_100us*timer_value); 
		if(timer_connected[timer_no] == FALSE)
		{	
			intConnect((VOIDFUNCPTR *)EPIC_GTM_INT_VEC(timer_no) , (VOIDFUNCPTR) timer_handle , (int) handle_param);  		
			timer_connected[timer_no] = TRUE ;
		}
		intEnable(EPIC_GTM_INT_VEC(timer_no));
	}
	else
	{
		intDisable(EPIC_GTM_INT_VEC(timer_no));
	}
	return OK ;
}

/* 
bsp_timer1_set(int action  , int timer_value , VOIDFUNCPTR *timer_handle , int handle_param)
设置周期性的中断处理接口
action = 0 ,关闭中断(其它参数可以不填)，else 使能中断
timer_value : 执行的中断的时间间隔= COUNTER_100us*timer_value [1,1000],100微妙到100豪秒之间
timer_handle :中断处理函数
handle_param :中断处理函数的参数
*/

STATUS bsp_timer1_set(int action  , int timer_value , VOIDFUNCPTR timer_handle , int handle_param)
{
#if 0
	if(action && ((timer_value <= 0) || (timer_value > 100000) ||  (timer_handle == NULL)))
	{
    	printf("bsp_timer1_set param error , timer_value  = %d( 0 < timer_value <1000) ,timer_handle = 0x%x\n" 
    		, timer_value,(int)timer_handle );
    	return ERROR ;
	}
		
	if (action)
	{
		sysEpicRegWrite(EPIC_TM1_BASE_COUNT_REG, COUNTER_100us*timer_value); 		
		intConnect((VOIDFUNCPTR *)EPIC_GTM1_INT_VEC , (VOIDFUNCPTR) timer_handle , (int) handle_param);  		
		intEnable(EPIC_GTM1_INT_VEC);/*Timer1 int enable*/
	}
	else
	{
		intDisable(EPIC_GTM1_INT_VEC);
	}
	return OK ;
#else
	return  bsp_timer_set(1 ,  action  ,  timer_value , timer_handle , handle_param);
#endif
}


/*区分不同的cpu*/
int BspInumToIvec(int intNum)
{
    return intNum;
}

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

		{0x000000, 97, 1, 0x20000, NULL, {0,0,0,0}, NULL, 2, 0, NULL},
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

#define ____BSP_DEVICE_TEST____
#include "../lld.h"
extern int BspPciBusTest();
extern int BspI2cTest();
extern int BspSpiTest();

int bsp_flash_upload(char * name, char * buffer, int len)
{
	int status = 0;
	int fd;
	
	fd = open (name, O_RDWR , 0);	
	write(fd, buffer, len);
	close (fd);

	return status;
}

int BspRamTest()
{
    unsigned long ulRamCheckAddrStart = (int)sysPhysMemTop () - USER_RESERVED_MEM;
    unsigned long ulRamCheckAddrEnd =   (int)sysPhysMemTop ();
    unsigned long ulRamCheckLoop = 0;    
    unsigned long ulRamCheckWriteVal = 0x5a5a5a5a;
    unsigned long ulRamCheckWriteValNext = 0xa5a5a5a5;
    
    unsigned long ulRamCheckReadVal = 0;    
    unsigned long ulRamCheckReadValNext = 0;    

    unsigned long *pulWritePrt = (unsigned long *)ulRamCheckAddrStart;
    unsigned long *pulReadPrt = (unsigned long *)ulRamCheckAddrStart;
    unsigned long ulRamCheckErr = 0;
    
    /*first write ram*/
    ulRamCheckLoop = ulRamCheckAddrStart;
    while(ulRamCheckLoop < ulRamCheckAddrEnd)
    {        
        *pulWritePrt = ulRamCheckWriteVal; 
        pulWritePrt += 1;
        ulRamCheckLoop += 4;
        *pulWritePrt = ulRamCheckWriteValNext; 
        pulWritePrt += 1;
        ulRamCheckLoop += 4;
    }    
    
    /*read ram*/
    ulRamCheckLoop = ulRamCheckAddrStart;
    while(ulRamCheckLoop < ulRamCheckAddrEnd)
    {        
        ulRamCheckReadVal = *pulReadPrt;
        /*判断是否相等*/
        if(ulRamCheckReadVal != ulRamCheckWriteVal)
        {
            ulRamCheckErr = 1;
            break;
        }
        pulReadPrt += 1;
        ulRamCheckLoop += 4;

        ulRamCheckReadValNext = *pulReadPrt;
        /*判断是否相等*/
        if(ulRamCheckReadValNext != ulRamCheckWriteValNext)
        {
            ulRamCheckErr = 1;
            break;
        }
        pulReadPrt += 1;
        ulRamCheckLoop += 4;
    }
    
    
    if(ulRamCheckErr)
    {
        return -1;
    }

    return 0;
}
int BspNVRamTest()
{
    unsigned long ulNVRamCheckAddrStart = 0xD0000000;
    unsigned long ulNVRamCheckAddrEnd =   0xD0100000;
    unsigned long ulNVRamCheckLoop = 0;    
    unsigned char ucNVRamCheckWriteVal = 0x5a;
    unsigned char ucNVRamCheckWriteValNext = 0xa5;
    
    unsigned char ucNVRamCheckReadVal = 0;    
    unsigned char ucNVRamCheckReadValNext = 0;    

    unsigned char *pucWritePrt = (unsigned char *)ulNVRamCheckAddrStart;
    unsigned char *pucReadPrt = (unsigned char *)ulNVRamCheckAddrStart;
    unsigned long ulRamCheckErr = 0;
    
    /*first write ram*/
    ulNVRamCheckLoop = ulNVRamCheckAddrStart;
    while(ulNVRamCheckLoop < ulNVRamCheckAddrEnd)
    {        
        *pucWritePrt = ucNVRamCheckWriteVal; 
        pucWritePrt += 1;
        ulNVRamCheckLoop += 1;
        *pucWritePrt = ucNVRamCheckWriteValNext; 
        pucWritePrt += 1;
        ulNVRamCheckLoop += 1;
    }    
    
    /*read ram*/
    ulNVRamCheckLoop = ulNVRamCheckAddrStart;
    while(ulNVRamCheckLoop < ulNVRamCheckAddrEnd)
    {        
        ucNVRamCheckReadVal = *pucReadPrt;
        /*判断是否相等*/
        if(ucNVRamCheckReadVal != ucNVRamCheckWriteVal)
        {
            ulRamCheckErr = 1;
            break;
        }
        pucReadPrt += 1;
        ulNVRamCheckLoop += 1;

        ucNVRamCheckReadValNext = *pucReadPrt;
        /*判断是否相等*/
        if(ucNVRamCheckReadValNext != ucNVRamCheckWriteValNext)
        {
            ulRamCheckErr = 1;
            break;
        }
        pucReadPrt += 1;
        ulNVRamCheckLoop += 1;
    }
    
    
    if(ulRamCheckErr)
    {
        return -1;
    }

    return 0;
}

int BspFlashTest(int chipno)
{
    unsigned long ulFlashCheckWriteVal = 0x5a5a;
    unsigned long ulFlashCheckWriteValNext = 0xa5a5;
    
    unsigned long ulFlashCheckReadVal = 0;    
    unsigned long ulFlashCheckReadValNext = 0;    

    unsigned long ulFlashCheckAddrStart = 0;
    unsigned long ulFlashCheckAddrStartTmp = 0;
    unsigned long ulFlashCheckAddrEnd = 0;
    unsigned long ulFlashCheckAddrOffser = 0;    
    unsigned long ulFlashTestNum = 8;  /*只测试1M*/
    unsigned long *pulReadPrt = (unsigned long *)ulFlashCheckAddrStart;

    switch(chipno)
    {
    	case 0:
    	    ulFlashCheckAddrStart = 0xfe000000;
        	break;
    	case 1:  
    	    ulFlashCheckAddrStart = 0xc2000000;	
    		break;
    	case 2:	
    	    ulFlashCheckAddrStart = 0xc6000000;	
    		break;
    	default:
    		break;
	}
	
    bsp_flash_erase(chipno, 256, ulFlashTestNum);    
    ulFlashCheckAddrEnd = ulFlashCheckAddrStart + FLASH_BLOCK_SIZE * ulFlashTestNum;
    ulFlashCheckAddrStartTmp = ulFlashCheckAddrStart;

    flash_write_enable(chipno);
    pulReadPrt = (unsigned long *)ulFlashCheckAddrStart;
    while((ulFlashCheckAddrStart + ulFlashCheckAddrOffser)< ulFlashCheckAddrEnd)
    {
    
        lld_memcpy((FLASHDATA *)ulFlashCheckAddrStart, ulFlashCheckAddrOffser/2, 2,(FLASHDATA *)&ulFlashCheckWriteVal);

        ulFlashCheckReadVal = *pulReadPrt;

        if(ulFlashCheckReadVal != ulFlashCheckWriteVal)
        {
            printf("\r\nulFlashCheckReadVal = 0x%x", ulFlashCheckReadVal);
            return -1;                        
        }
        ulFlashCheckAddrOffser += 4;
        pulReadPrt++;

        lld_memcpy((FLASHDATA *)ulFlashCheckAddrStart, ulFlashCheckAddrOffser/2, 2,(FLASHDATA *)&ulFlashCheckWriteValNext);

        ulFlashCheckReadValNext = *pulReadPrt;
        if(ulFlashCheckReadValNext != ulFlashCheckWriteValNext)
        {
            printf("\r\nulFlashCheckReadValNext = 0x%x", ulFlashCheckReadValNext);
            return -1;                        
        }
        ulFlashCheckAddrOffser += 4;
        pulReadPrt++;
    }
    flash_write_disable(chipno);

    return 0;
}

void BspCpldTest()
{
    unsigned char ucCpldTestVal = 0xaa;
    unsigned char tmp = 0;
    
    ucCpldTestVal = ~ucCpldTestVal;
    *CPLD_REG(6) = ucCpldTestVal;

    tmp = *CPLD_REG(6);
    if(((*CPLD_REG(6)) & 0xff) != ucCpldTestVal)
    {
        printf("\r\n error ...");
    }
    printf("\r\nok");
}

void DeviceTest(void)
{
    BSP_DEVICE_TEST_STR testResult;
    memset(&testResult, 0, sizeof(testResult));

    unsigned char ucCpldTestVal = 0xaa;
    unsigned long ulNum = 0;

    *CPLD_REG(0x11) = 0xff;
    *CPLD_REG(0x22) = 0xff;    
    
    while(1)
    {
        taskDelay(10);
        /*DDR TEST*/
        if(BspRamTest() != 0)
        {
            testResult.DDRTestErrorNum++;
            printf("\r\nDDRTestErrorNum = 0x%x", testResult.DDRTestErrorNum); 
        }

        if(BspFlashTest(0) != 0)
        {
            testResult.Flash0TestErrorNum++;
            printf("\r\nFlash0TestErrorNum = 0x%x", testResult.Flash0TestErrorNum); 
        }

        if(BspFlashTest(1) != 0)
        {
            testResult.Flash1TestErrorNum++;
            printf("\r\nFlash1TestErrorNum = 0x%x", testResult.Flash1TestErrorNum); 
        }
        if(BspFlashTest(2) != 0)
        {
            testResult.Flash2TestErrorNum++;
            printf("\r\nFlash2TestErrorNum = 0x%x", testResult.Flash2TestErrorNum); 
        }

        if(BspNVRamTest() != 0)
        {
            testResult.NVRAMTestErrorNum++;
            printf("\r\nNVRAMTestErrorNum = 0x%x", testResult.NVRAMTestErrorNum); 
        }

        if(BspPciBusTest() != 0)
        {
            testResult.PCITestErrorNum++;
            printf("\r\nPCITestErrorNum = 0x%x", testResult.PCITestErrorNum); 
        }

        ucCpldTestVal = ~ucCpldTestVal;
        *CPLD_REG(6) = ucCpldTestVal;
        if(((*CPLD_REG(6)) & 0xff) != ucCpldTestVal)
        {
            testResult.CPLDTestErrorNum++;
            printf("\r\nCPLDTestErrorNum = 0x%x", testResult.CPLDTestErrorNum); 
        }

        if(BspI2cTest() != 0)
        {
            testResult.IICTestErrorNum++;
            printf("\r\nIICTestErrorNum = 0x%x", testResult.IICTestErrorNum); 
        }
        if(BspSpiTest() != 0)
        {
            testResult.SPITestErrorNum++;
            printf("\r\nSPITestErrorNum = 0x%x", testResult.SPITestErrorNum); 
        }
        testResult.DeviceTestTotalNum++;
        bsp_flash_upload("/tffs/bios/testresult1", (char *)&testResult, sizeof(testResult));
        bsp_flash_upload("/tffs/bios/testresult2", (char *)&testResult, sizeof(testResult));
        printf("TotalTestNum = 0x%x\r\n", ++ulNum);
    }
}

void BspDeviceTestShow(char * name)
{
    BSP_DEVICE_TEST_STR testResult;
    memset(&testResult, 0, sizeof(testResult));

	char * p_data =	(char *)malloc(0x100000 * sizeof(char));
	int	rs, len = 0;

    rs = bsp_fs_download(name, p_data, 0x100000, &len);


    if (rs != 0)
    {
        printf("\nfile read error, maybe file do not existed\n");
    }

    if(sizeof(testResult) == len)
    {
        memcpy(&testResult, p_data, len);
        printf("\r\ntestResult.DeviceTestTotalNum = 0x%x", testResult.DeviceTestTotalNum);
        printf("\r\ntestResult.DDRTestErrorNum = 0x%x", testResult.DDRTestErrorNum);
        printf("\r\ntestResult.Flash0TestErrorNum = 0x%x", testResult.Flash0TestErrorNum);
        printf("\r\ntestResult.Flash1TestErrorNum = 0x%x", testResult.Flash1TestErrorNum);
        printf("\r\ntestResult.Flash2TestErrorNum = 0x%x", testResult.Flash2TestErrorNum);
        printf("\r\ntestResult.PCITestErrorNum = 0x%x", testResult.PCITestErrorNum);
        printf("\r\ntestResult.IICTestErrorNum = 0x%x", testResult.IICTestErrorNum);
        printf("\r\ntestResult.SPITestErrorNum = 0x%x", testResult.SPITestErrorNum);
        printf("\r\ntestResult.NVRAMTestErrorNum = 0x%x", testResult.NVRAMTestErrorNum);
        printf("\r\ntestResult.CPLDTestErrorNum = 0x%x", testResult.CPLDTestErrorNum);
        printf("\r\n");
    }
}
int BspDeviceTestTaskId = 0;
void BspDeviceTestStart()
{
    printf("\r\nStep1: Load FPGA...");
    bsp_fpga_io_dl("/tffs/bios/qtr6083.bit",NULL,0,1);
    printf("\r\nStep2: Begin Device Test...");
    taskDelay(10);
    BspDeviceTestTaskId = taskSpawn("tDevieceTest", 20, 0, 0x1000, (FUNCPTR)DeviceTest, 0,0,0,0,0,0,0,0,0,0);   	
}

void BspDeviceTestStop()
{
    if(0 == taskSuspend(BspDeviceTestTaskId))
    {
        printf("\r\nDevice Test Stop!\r\n");
    }
}


