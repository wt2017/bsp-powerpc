/* config.h - Motorola cpu board configuration header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
2005-06-16 bjm  revise bspname to BBTN710(BBTN710) 
                revise CRYSTAL_FREQ to 66MHZ
01s,12jun02,kab  SPR 74987: cplusplus protection
01r,14dec01,jrs  Remove NFS requirement as default
01q,17oct01,jrs  Upgrade to veloce
		 changed INCLUDE_MOT_FCC to INCLUDE_MOTFCCEND - SPR #33914
01p,14mar00,ms_  add support for PILOT revision of board
01o,04mar00,mtl  made PILOT as default revision
01n,03mar00,mtl  changed rom address & size to reflect system exc vector
01m,29sep99,ms_  NV_RAM_SIZE size is NONE; removed NV_RAM_SIZE_WRITEABLE
01l,19sep99,ms_  undef MMU and CACHE to assure FLASH visibility
01k,17sep99,ms_  undef MOT_FCC_DBG; use generic bootline
01j,26jul99,ms_  remove wrs references from default boot line
01i,16jun99,ms_  bumped version for beta
01h,16jun99,cn   corrected macro definitions about FLASH device.
01g,28may99,ms_  bump bsp version level for second EAR release
01f,21may99,ms_  added BCSR and IMM macros
01e,20may99,cn   added support for bootrom and FLASH device.
01d,17apr99,ms_  remove SPLL
01c,14apr99,cn   added support for motFccEnd
01b,08apr99,ms_  upgrade for multiple serial channels
01a,07jan99,ms_  adapted from ads860 config.h
*/

/*
This file contains the configuration parameters for the
Motorola MPC8260 ADS board.
*/

#ifndef	INCconfigh
#define	INCconfigh

#ifdef __cplusplus
    extern "C" {
#endif


/* default board revision is PILOT */

#define BOARD_REV_PILOT                 /* undefine for revision ENG */

/* Define Clock Speed and source  */
 
#define	FREQ_33_MHZ	 	33333000
#define	FREQ_40_MHZ	 	40000000
#define	FREQ_66_MHZ	 	66666000
#define	FREQ_80_MHZ	 	80000000
#define	FREQ_100_MHZ	100000000
#define	FREQ_120_MHZ	120000000
#define	FREQ_133_MHZ	133000000
#define	FREQ_150_MHZ	150000000
#define	FREQ_166_MHZ	166000000
#define	FREQ_200_MHZ	200000000
#define	FREQ_233_MHZ	233000000
#define	FREQ_266_MHZ	266000000
#define	FREQ_300_MHZ	300000000

/*
 * This define must be set to the value of the resonant oscillator
 * inserted in position U16 of the FADS8260 board.
 */
#define	OSCILLATOR_FREQ	FREQ_66_MHZ


/*
 * This value is the setting for the MPTPR[PTP] Refresh timer prescaler.
 * The value is dependent on the OSCILLATOR_FREQ value.  For other values
 * a conditionally compiled term must be created here for that OSCILLATOR_FREQ
 * value.
 * The MOD_CK and MOD_CK_H values are predicated on the
 * system clock frequency which is determined by the OSCILLATOR_FREQ
 * and the CPM clock and Core clock requirements of the MCP6260 utilized.
 *
 * These values MUST be set to the EXACT values set on the DS1 switches!
 *
 * BRGCLK_DIV_FACTOR
 * Baud Rate Generator division factor - 0 for division by 1
 *					 1 for division by 4
 *					 2 for division by 16
 *					 3 for division by 64
 */

#define	DIV_FACT_1	0
#define	DIV_FACT_16	1

#if (OSCILLATOR_FREQ == FREQ_33_MHZ)
#define	PTP			0x2000
#define	MOD_CK_H		1
#define	MOD_CK			7
#define BRGCLK_DIV_FACTOR	DIV_FACT_16
#endif

#if (OSCILLATOR_FREQ == FREQ_40_MHZ)
#define	PTP			0x2600
#define	MOD_CK_H		5
#define	MOD_CK			7
#define BRGCLK_DIV_FACTOR	DIV_FACT_1
#endif

#if (OSCILLATOR_FREQ == FREQ_66_MHZ)
#define	PTP			0x4000
#define	MOD_CK_H		5
#define	MOD_CK			7
#define BRGCLK_DIV_FACTOR	DIV_FACT_16
#endif

#define M8260_BRGC_DIVISOR	BRGCLK_DIV_FACTOR

#include "configAll.h"

#define INCLUDE_WDB_COMM_END
#define  DEFAULT_BOOT_LINE \
"motfcc(0,0)host:vxWorks e=192.168.0.254 u=user pw=password tn=bbtn710"

#undef INCLUDE_WINDVIEW

#define INCLUDE_MMU_BASIC

#define USER_I_MMU_ENABLE
#define  USER_D_MMU_ENABLE
#define INCLUDE_CACHE_SUPPORT

#define USER_D_CACHE_ENABLE
#undef  USER_D_CACHE_MODE
#define USER_D_CACHE_MODE  CACHE_COPYBACK /* select COPYBACK or WRITETHROUGH */

#define USER_I_CACHE_ENABLE
#undef  USER_I_CACHE_MODE
#define USER_I_CACHE_MODE  CACHE_COPYBACK /* select COPYBACK or WRITETHROUGH */

#define INCLUDE_SYM_TBL
#define INCLUDE_NET_SYM_TBL
#define INCLUDE_STAT_SYM_TBL

#define INCLUDE_SHOW_ROUTINES 
#define INCLUDE_ZBUF_SOCK 

#undef INCLUDE_NFS

#ifdef INCLUDE_NFS
/* Default NFS parameters - constants may be changed here, variables
 * may be changed in usrConfig.c at the point where NFS is included.
 */

#define NFS_USER_ID             2001            /* dummy nfs user id */
#define NFS_GROUP_ID            100             /* dummy nfs user group id */
#define NFS_MAXPATH             255             /* max. file path length */

#endif /* INCLUDE_NFS */

/* Number of TTY definition */

#undef	NUM_TTY
#define	NUM_TTY		N_SIO_CHANNELS		/* defined in ads8260.h */

/* Optional timestamp support */

#undef	INCLUDE_TIMESTAMP
#define	INCLUDE_AUX_CLK

/* optional TrueFFS support */

#define  INCLUDE_TFFS

#ifdef INCLUDE_TFFS
#define INCLUDE_DOSFS		/* dosFs file system */
#define INCLUDE_SHOW_ROUTINES	/* show routines for system facilities*/
#endif /* INCLUDE_DOSFS */

/* clock rates */

#define	SYS_CLK_RATE_MIN	1	/* minimum system clock rate */
#define	SYS_CLK_RATE_MAX	8000	/* maximum system clock rate */
#define	AUX_CLK_RATE_MIN	1	/* minimum auxiliary clock rate */
#define	AUX_CLK_RATE_MAX	8000	/* maximum auxiliary clock rate */

/*
 * DRAM refresh frequency - This macro defines the DRAM refresh frequency.
 * e.i: A DRAM with 8192 rows to refresh in 64ms: 
 * DRAM_REFRESH_FREQ = 8192/ 64E-3 = 128 khz
 */

#define DRAM_REFRESH_FREQ	128000			/* 128 kHz */

/*
 * Baud Rate Generator division factor - 0 for division by 1
 *					 1 for division by 4
 *					 2 for division by 16
 *					 3 for division by 64
 */


/* add necessary drivers */

#define INCLUDE_END 
#define INCLUDE_BSD 
  
/* remove unnecessary drivers */

#undef INCLUDE_BP
#undef INCLUDE_EX
#undef INCLUDE_ENP
#undef INCLUDE_SM_NET
#undef INCLUDE_SM_SEQ_ADD


#define FLASH_AREA_TOTAL_SIZE   0x2000000        /* 32 Mbytes total flash size */

#define NV_RAM_SIZE             NONE            /* no NVRAM */
 
/* Memory addresses */
 
#define LOCAL_MEM_LOCAL_ADRS    0x00000000      /* Base of RAM */
#define LOCAL_MEM_SIZE          0x10000000      /* 256 Mbyte memory available */

#define MEM_CHECK_SIZE			0x04000000		/* 64 Mbyte memory available */
#define MEM_CHECK_OFFSET	    0x02000000	    /* check offset of RAM  */

#define M8501_MODULE_MEM_SIZE   (1024*1024*128)
#define M8501_RESERVE_MEM_SIZE  (1024*1024*128) 
 
/*
 * The constants ROM_TEXT_ADRS, ROM_SIZE, and RAM_HIGH_ADRS are defined
 * in config.h, MakeSkel, Makefile, and Makefile.*
 * All definitions for these constants must be identical.
 */
 
#define ROM_BASE_ADRS           0x10000000      /* base address of ROM */
#define ROM_TEXT_ADRS           ROM_BASE_ADRS + 0x100
#define ROM_SIZE                0x100000         /* 1M ROM space */
#define ROM_WARM_ADRS           (ROM_TEXT_ADRS+8) /* warm reboot entry */

/* RAM address for ROM boot */
#define RAM_HIGH_ADRS           (LOCAL_MEM_LOCAL_ADRS + 0x400000)
 
/* RAM address for sys image */
#define RAM_LOW_ADRS            (LOCAL_MEM_LOCAL_ADRS + 0x10000)
 

/* Address and size of MPC8260 Internal Memory Map */

#define DEFAULT_IMM_ADRS        0xfff00000      /*revised by bjm for compatiable with zhangsc*/
#define IMM_SIZE                0x00020000      /* 128K of address space */

/*
 * Default power management mode - selected via vxPowerModeSet() in
 * sysHwInit().
 */

#define DEFAULT_POWER_MGT_MODE  VX_POWER_MODE_DISABLE

#ifndef	_ASMLANGUAGE
extern STATUS NSPsntpsClockHook (int request, void *pBuffer);
#endif /*_ASMLANGUAGE*/

#ifdef __cplusplus
    }
#endif

#endif	/* INCconfigh */
#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif
