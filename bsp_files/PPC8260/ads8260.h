/* ads8260.h - Motorola MPC8260 ADS board header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
2005-06-16 ,bjm  revise osc from 33M to 66M(BBTN710)
01j,12jun02,kab  SPR 74987: cplusplus protection
01i,22oct01,jrs  Upgrade to veloce - changed arch from PPCEC603 to PPC603.
		 set DEC_CLOCK_FREQ to OSCILLATOR_FREQ
		 removed MOD_CK, MOD_CK_H, and FREQ_40_MHZ definitions,
		 added Clock Speed and source definitions - SPR#66989
01h,20oct01,yvp  Corrected CPU type to PPC603 from PPCC603
01f,14mar00,ms_  add support for PILOT revision of board
01e,16sep99,ms_  get some .h files from h/drv instead of locally
01d,15jul99,ms_  make compliant with our coding standards
01c,17apr99,ms_  remove unnecessary macros
01b,17apr99,ms_  final EAR cleanup
01a,08jan99,ms_  adapted from ads860.h
*/

/*
This file contains I/O addresses and related constants for the
Motorola MPC8260 ADS board. 
*/

#ifndef	INCads8260h
#define	INCads8260h

#ifdef __cplusplus
    extern "C" {
#endif

#include "drv/mem/memDev.h"
#include "drv/intrCtl/m8260IntrCtl.h"

#define BUS	0				/* bus-less board */
#undef  CPU
#define CPU	PPC603				/* CPU type */

#define N_SIO_CHANNELS	 	1		/* No. serial I/O channels */

/* define the decrementer input clock frequency */

#define DEC_CLOCK_FREQ	OSCILLATOR_FREQ

/*define CPM frequency  == 166M */
#define   CPM_FRQ   	(166*1000000)

/* define system clock rate */


/* CPU type in the PVR */

#define CPU_TYPE_860		0x0050		/* value for PPC860 */
#define CPU_TYPE_8260		0xAAAA		/* value for PPC8260 */
#define	CPU_REV_A1_MASK_NUM	0x0010		/* revision mask num */

/* Port A, B, C and D Defines */
#define PA31    (0x00000001)
#define PA30    (0x00000002)
#define PA29    (0x00000004)
#define PA28    (0x00000008)
#define PA27    (0x00000010)
#define PA26    (0x00000020)
#define PA25    (0x00000040)
#define PA24    (0x00000080)
#define PA23    (0x00000100)
#define PA22    (0x00000200)
#define PA21    (0x00000400)
#define PA20    (0x00000800)
#define PA19    (0x00001000)
#define PA18    (0x00002000)
#define PA17    (0x00004000)
#define PA16    (0x00008000)
#define PA15    (0x00010000)
#define PA14    (0x00020000)
#define PA13    (0x00040000)
#define PA12    (0x00080000)
#define PA11    (0x00100000)
#define PA10    (0x00200000)
#define PA9     (0x00400000)
#define PA8     (0x00800000)
#define PA7     (0x01000000)
#define PA6     (0x02000000)
#define PA5     (0x04000000)
#define PA4     (0x08000000)
#define PA3     (0x10000000)
#define PA2     (0x20000000)
#define PA1     (0x40000000)
#define PA0     (0x80000000)

#define PB31    (0x00000001)
#define PB30    (0x00000002)
#define PB29    (0x00000004)
#define PB28    (0x00000008)
#define PB27    (0x00000010)
#define PB26    (0x00000020)
#define PB25    (0x00000040)
#define PB24    (0x00000080)
#define PB23    (0x00000100)
#define PB22    (0x00000200)
#define PB21    (0x00000400)
#define PB20    (0x00000800)
#define PB19    (0x00001000)
#define PB18    (0x00002000)
#define PB17    (0x00004000)
#define PB16    (0x00008000)
#define PB15    (0x00010000)
#define PB14    (0x00020000)
#define PB13    (0x00040000)
#define PB12    (0x00080000)
#define PB11    (0x00100000)
#define PB10    (0x00200000)
#define PB9     (0x00400000)
#define PB8     (0x00800000)
#define PB7     (0x01000000)
#define PB6     (0x02000000)
#define PB5     (0x04000000)
#define PB4     (0x08000000)

#define PC31    (0x00000001)
#define PC30    (0x00000002)
#define PC29    (0x00000004)
#define PC28    (0x00000008)
#define PC27    (0x00000010)
#define PC26    (0x00000020)
#define PC25    (0x00000040)
#define PC24    (0x00000080)
#define PC23    (0x00000100)
#define PC22    (0x00000200)
#define PC21    (0x00000400)
#define PC20    (0x00000800)
#define PC19    (0x00001000)
#define PC18    (0x00002000)
#define PC17    (0x00004000)
#define PC16    (0x00008000)
#define PC15    (0x00010000)
#define PC14    (0x00020000)
#define PC13    (0x00040000)
#define PC12    (0x00080000)
#define PC11    (0x00100000)
#define PC10    (0x00200000)
#define PC9     (0x00400000)
#define PC8     (0x00800000)
#define PC7     (0x01000000)
#define PC6     (0x02000000)
#define PC5     (0x04000000)
#define PC4     (0x08000000)
#define PC3     (0x10000000)
#define PC2     (0x20000000)
#define PC1     (0x40000000)
#define PC0     (0x80000000)

#define PD31    (0x00000001)
#define PD30    (0x00000002)
#define PD29    (0x00000004)
#define PD28    (0x00000008)
#define PD27    (0x00000010)
#define PD26    (0x00000020)
#define PD25    (0x00000040)
#define PD24    (0x00000080)
#define PD23    (0x00000100)
#define PD22    (0x00000200)
#define PD21    (0x00000400)
#define PD20    (0x00000800)
#define PD19    (0x00001000)
#define PD18    (0x00002000)
#define PD17    (0x00004000)
#define PD16    (0x00008000)
#define PD15    (0x00010000)
#define PD14    (0x00020000)
#define PD13    (0x00040000)
#define PD12    (0x00080000)
#define PD11    (0x00100000)
#define PD10    (0x00200000)
#define PD9     (0x00400000)
#define PD8     (0x00800000)
#define PD7     (0x01000000)
#define PD6     (0x02000000)
#define PD5     (0x04000000)
#define PD4     (0x08000000)

/* 
 * Maximum number of SCC channels to configure as SIOs. Note that this 
 * assumes sequential usage of SCCs.
*/
#define MAX_SCC_SIO_CHANS 2

#ifdef __cplusplus
    }
#endif

#endif /* INCads8260h */
