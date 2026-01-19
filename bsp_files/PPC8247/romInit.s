/* bctl8247_romInit.s - Motorola ads8247 ROM initialization module */

/* Copyright 1984-2003 Wind River Systems, Inc. */
	.data
	
#ifndef ALPHA_VERSION
	.globl  copyright_wind_river
	.long   copyright_wind_river
#endif
/*
modification history
--------------------
2006-07-18   xiexy change for tn750
01a,23dec03,dtr  create from ads826x/romInit.s
*/

/*
DESCRIPTION
This module contains the entry code for the VxWorks bootrom.
The entry point romInit, is the first code executed on power-up.
It sets the BOOT_COLD parameter to be passed to the generic
romStart() routine.

The routine sysToMonitor() jumps to the location 8 bytes
past the beginning of romInit, to perform a "warm boot".
This entry point allows a parameter to be passed to romStart().

*/

#define	_ASMLANGUAGE

#include "vxWorks.h"
#include "asm.h"
#include "cacheLib.h"
#include "config.h"
#include "regs.h"	
#include "sysLib.h"
#include "drv/timer/m8260Clock.h"
#include "drv/mem/m8260Siu.h"
#include "drv/mem/m8260Memc.h"
#include "proj_config.h"


/* internals */

FUNC_EXPORT(_romInit)		/* start of system code */
FUNC_EXPORT(romInit)		/* start of system code */

/* externals */

FUNC_IMPORT(romStart)	/* system initialization routine */
/*
* A flash based Hard Reset Config Word can be used to boot the board by
* changing jumper JP3.  However the reset word must be located at the
* base of flash memory.  The directives below configure a hard reset
* config word at ROM_BASE_ADRS, which is usually 0xFFF00000.
*
* The following defines what the hard reset config word should look like,
* but the table is located at the wrong address.
* The following must be programmed into the first 32 bytes of flash
* to use it as a hard reset configuration word.
*/
	.text                       /* byte 0 (MSByte) of the configuration master's */
	.fill   1,1,HRCW_BYTE_0     /* Hard Reset Configuration Word */
	.fill   7,1,0
	.fill   1,1,HRCW_BYTE_1
	.fill   7,1,0
	.fill   1,1,HRCW_BYTE_2
	.fill   7,1,0
	.fill   1,1,HRCW_BYTE_3		/* This is the LSByte */
	.fill   231,1,0     		/* The rest of the space are filled with zeros */

	.align 2

/******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*
*
* romInit
*		(
*		int startType	/@ only used by 2nd entry point @/
*		)

*/

FUNC_BEGIN(_romInit)
FUNC_BEGIN(romInit)

	bl	  	cold		/* jump to the cold boot initialization */
	mr		r31, r3		/* save	passed parameter in r31 */
	bl	  	start		/* jump to the warm boot initialization */
						/* r3 should have BOOT_WARM value */

	/* copyright notice appears at beginning of ROM (in TEXT segment) */
    /* notice:the ascii text must be mutiplier of 4,or it will cause compiler error */
	.ascii	BSP_COPYRIGHT  
	.align 4

cold:
	li		r31, BOOT_COLD	/* set cold boot as start type */
							/* saved in r31 to later pass as */
							/* argument to romStart() */

	/*
	 * initialize the IMMR register before any non-core register
	 * modifications. The default IMMR base address was 0x0F000000,
	 * as originally programmed in the Hard Reset Configuration Word.
	 * ONLY IF A COLD BOOT
	 */

	/* put desired internal memory map base address in r4 */
	lis		r4, HIADJ (INTERNAL_MEM_MAP_ADDR)
	addi		r4, r4, LO (INTERNAL_MEM_MAP_ADDR)

	/* get the reset value of IMM base address */
	lis		r8, HIADJ (RST_INTERNAL_MEM_MAP_ADDR + PQII_REG_BASE_OFF)
	addi	r8, r8, LO (RST_INTERNAL_MEM_MAP_ADDR + PQII_REG_BASE_OFF)
	stw		r4, IMMR_OFFSET(r8)		/* IMMR now at bsp specified value */
	isync

	/* load the base register 0 */
	lis		r5, HIADJ ((0x00 & M8260_BR_BA_MSK) | M8260_BR_PS_16 | M8260_BR_V)
	addi	r5,	r5, LO ((0x00 & M8260_BR_BA_MSK) | M8260_BR_PS_16 | M8260_BR_V)
	lis 	r6, HIADJ (M8260_BR0 (INTERNAL_MEM_MAP_ADDR))
	addi	r6,	r6, LO (M8260_BR0 (INTERNAL_MEM_MAP_ADDR)) 
	stw 	r5, 0(r6)

	/* load the option register 0 */
	stw 	r5, 0(r6)
	lis	r5, HIADJ(	((~(CS0_DEFAULT_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_SCY_10_CLK)
	addi	r5,	r5, LO (((~(CS0_DEFAULT_SIZE - 1)) & M8260_OR_AM_MSK) |M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_SCY_10_CLK)
	lis 	r6, HIADJ (M8260_OR0 (INTERNAL_MEM_MAP_ADDR))
	addi	r6,	r6, LO (M8260_OR0 (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0 (r6)
	isync

	/*
	 * When the PowerPC 8260 is powered on, the processor fetches the
	 * instructions located at the address 0x100. We need to jump
	 * from the address 0x100 to the Flash space.
	 */	 

	lis     	r7, HIADJ(romInit)
	addi    	r7, r7, LO(romInit)

	lis 		r6, HIADJ(start)	
	addi		r6, r6, LO(start)	/* load R6 with C entry point */

	lis     	r8, HIADJ(ROM_TEXT_ADRS)
	addi    	r8, r8, LO(ROM_TEXT_ADRS)

	sub 		r6, r6, r7		/* routine - entry point */
	add	    	r6, r6, r8 		/* + ROM base */

	mtspr	LR, r6		/* save destination address  into LR register */
	blr				 /* jump to flash mem address */

start:    

	/*
	 * Initialize all necessary core registers before continuing
	 *
	 * 1. Clear MSR to disable all exceptions, no mmu, no fpu etc
	 * 2. Clear out the SPRGs
	 * 3. Clear out the Segment Registers
	 * 4. Clear Instruction BAT registers
	 * 5. Clear Data BAT registers
	 * 6. Initialize FPU registers
	 *
	 */
	/* clear the MSR */
	xor	r3, r3, r3  
	sync
	mtmsr   r3
	isync

	/* clear the SPRGs */

	xor     r0,r0,r0
	mtspr   272,r0
	mtspr   273,r0
	mtspr   274,r0
	mtspr   275,r0

	/* clear the Segment registers */
	andi.   r3, r3, 0
	isync
	mtsr    0,r3
	isync
	mtsr    1,r3
	isync
	mtsr    2,r3
	isync
	mtsr    3,r3
	isync
	mtsr    4,r3
	isync
	mtsr    5,r3
	isync
	mtsr    6,r3
	isync
	mtsr    7,r3
	isync
	mtsr    8,r3
	isync
	mtsr    9,r3
	isync
	mtsr    10,r3
	isync
	mtsr    11,r3
	isync
	mtsr    12,r3
	isync
	mtsr    13,r3
	isync
	mtsr    14,r3
	isync
	mtsr    15,r3
	isync

	li	p3,0	 		/* clear r3 */

	isync
	mtspr	IBAT0U,p3		/* SPR 528 (IBAT0U) */
	isync
	mtspr	IBAT0L,p3		/* SPR 529 (IBAT0L) */
	isync
	mtspr	IBAT1U,p3		/* SPR 530 (IBAT1U) */
	isync
	mtspr	IBAT1L,p3		/* SPR 531 (IBAT1L) */
	isync
	mtspr	IBAT2U,p3		/* SPR 532 (IBAT2U) */
	isync
	mtspr	IBAT2L,p3		/* SPR 533 (IBAT2L) */
	isync
	mtspr	IBAT3U,p3		/* SPR 534 (IBAT3U) */
	isync
	mtspr	IBAT3L,p3		/* SPR 535 (IBAT3L) */
	isync
	/*MPC 8247, 8272 owned only*/
	mtspr	560,p3		/* SPR 560 (IBAT4U) */
	isync
	mtspr	561,p3		/* SPR 561 (IBAT4L) */
	isync
	mtspr	562,p3		/* SPR 562 (IBAT5U) */
	isync
	mtspr	563,p3		/* SPR 563 (IBAT5L) */
	isync
	mtspr	564,p3		/* SPR 564 (IBAT6U) */
	isync
	mtspr	565,p3		/* SPR 565 (IBAT6L) */
	isync
	mtspr	566,p3		/* SPR 566 (IBAT7U) */
	isync
	mtspr	567,p3		/* SPR 567 (IBAT7L) */
	isync

	mtspr	DBAT0U,p3		/* SPR 536 (DBAT0U) */
	isync
	mtspr	DBAT0L,p3		/* SPR 537 (DBAT0L) */
	isync
	mtspr	DBAT1U,p3		/* SPR 538 (DBAT1U) */
	isync
	mtspr	DBAT1L,p3		/* SPR 539 (DBAT1L) */
	isync
	mtspr	DBAT2U,p3		/* SPR 540 (DBAT2U) */
	isync
	mtspr	DBAT2L,p3		/* SPR 541 (DBAT2L) */
	isync
	mtspr	DBAT3U,p3		/* SPR 542 (DBAT3U) */
	isync
	mtspr	DBAT3L,p3		/* SPR 543 (DBAT3L) */
	isync
    /*MPC 8247, 8272 owned only*/
	mtspr	568,p3		/* SPR 568 (DBAT4U) */
	isync
	mtspr	569,p3		/* SPR 568 (DBAT4L) */
	isync	
	mtspr	570,p3		/* SPR 568 (DBAT5U) */
	isync	
	mtspr	571,p3		/* SPR 568 (DBAT5L) */
	isync		
	mtspr	572,p3		/* SPR 568 (DBAT6U) */
	isync	
	mtspr	573,p3		/* SPR 568 (DBAT6L) */
	isync	
	mtspr	574,p3		/* SPR 568 (DBAT7U) */
	isync		
	mtspr	575,p3		/* SPR 568 (DBAT7L) */
	isync	


    /* invalidate entries within both TLBs */
    	li	r5,32
	mtctr	r5			/* CTR = 32  */
	xor	r3,r3,r3		/* r3 = 0    */
	isync				/* context sync req'd before tlbie */
	
InitLoop:
	tlbie	r3
	addi	r3,r3,0x1000		/* increment bits 15-19 */
	bdnz	InitLoop		/* decrement CTR, branch if CTR != 0 */
	sync				/* sync instr req'd after tlbie      */

	/* clear HID0 */
	li      r3, 0
	sync
	isync
	mtspr   HID0, r3

	/* 
	 * Invalidate both caches by setting and then clearing the cache 
	 * invalidate bits
	 */

	 mfspr   r3, HID0
	ori     r3, r3, (_PPC_HID0_ICFI | _PPC_HID0_DCFI) /* set bits */
	sync
	isync
	mtspr   HID0, r3

	/* clear HID0, which clears cache invalidate bits */
	li      r3, 0
	sync
	isync
	mtspr   HID0, r3
	

	/* SYPCR - turn off the system protection stuff */
	lis		r5, HIADJ (M8260_SYPCR_SWTC | M8260_SYPCR_BMT | M8260_SYPCR_PBME | M8260_SYPCR_LBME | M8260_SYPCR_SWP)
	addi	r5, r5, LO (M8260_SYPCR_SWTC | M8260_SYPCR_BMT | M8260_SYPCR_PBME | M8260_SYPCR_LBME | M8260_SYPCR_SWP)
	lis 	r6, HIADJ (M8260_SYPCR (INTERNAL_MEM_MAP_ADDR))
	addi	r6, r6, LO (M8260_SYPCR (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0(r6) 

	lis	    r22,HIADJ (0x00000000)  
	addi	r22, r22, LO (0x00000000)
	lis	    r23, HIADJ(INTERNAL_MEM_MAP_ADDR + 0x010D04) 
	addi	r23, r23, LO (INTERNAL_MEM_MAP_ADDR + 0x010D04)
	stw	    r22, 0(r23)    
	stw	    r22, 0x20(r23)  
	stw	    r22, 0x40(r23)  
	stw	    r22, 0x60(r23)  

	/* Inialize I/0 which control the led --M8260_IOP_PDPAR IO_Red PD20 ; IO_Grn	PD21*/	
	lis	    r22,HIADJ (PD20|PD21)  
	addi	r22, r22, LO (PD20|PD21)
	lis	    r23, HIADJ(INTERNAL_MEM_MAP_ADDR + 0x10D60) 
	addi	r23, r23, LO (INTERNAL_MEM_MAP_ADDR + 0x10D60)
	stw	    r22, 0(r23)    

	/* Inialize I/0 which control the hardward dog --M8260_IOP_PC IO_WDI PC14*/	
	lis	    r22,HIADJ (PC14)  
	addi	r22, r22, LO (PC14)
	lis	    r23, HIADJ(INTERNAL_MEM_MAP_ADDR + 0x10D40) 
	addi	r23, r23, LO (INTERNAL_MEM_MAP_ADDR + 0x10D40)
	stw	    r22, 0(r23)    

   	lis 	r21, HIADJ (BSP_DOG_CLR_REG)	
	addi	r21, r21, LO (BSP_DOG_CLR_REG)

    /*clear dog by i/o*/
	lis	    r22, HIADJ(BSP_DOG_CLR)	/*write 1 to BIT27*/
	addi	r22, r22, LO(BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	or 	    r20, r20, r22
	stw	    r20, 0(r21)    

	lis	    r22, HIADJ(~BSP_DOG_CLR)	/*write 0 to BIT27*/
	addi	r22, r22, LO(~BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	and	    r20, r20, r22
	stw	    r20, 0(r21)    

	/* program the BCR */
	lis 	r5, HIADJ (BCR_VAL)
	addi	r5, r5, LO (BCR_VAL)
	lis	    r6, HIADJ (M8260_BCR (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BCR (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* program the PPC_ACR ,8 bit*/
	addi    r5, 0, PPC_ACR_VAL
	lis 	r6, HIADJ (M8260_PPC_ACR (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_PPC_ACR (INTERNAL_MEM_MAP_ADDR))
	stb	r5, 0 (r6)

	/* program the PPC_ALRH */
	lis	    r5, HIADJ (PPC_ALRH)
	addi    r5, r5, LO (PPC_ALRH)
	lis	    r6, HIADJ (M8260_PPC_ALRH (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_PPC_ALRH (INTERNAL_MEM_MAP_ADDR))
	stw	    r5, 0 (r6)

	/* program the SIUMCR  */
	lis     r5, HIADJ (SIUMCR_VAL) 
    addi    r5, r5, LO (SIUMCR_VAL)
	lis	r6, HIADJ (M8260_SIUMCR (INTERNAL_MEM_MAP_ADDR))	 
	addi	r6, r6, LO (M8260_SIUMCR (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6) 

	/* program the TESCR1 */
    lis     r5, HIADJ (TESCR1_VAL)
    addi    r5, r5, LO (TESCR1_VAL)
	lis	r6, HIADJ (M8260_TESCR1 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_TESCR1 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)
 
 	
	/*
	 * Map the bank 0 to the flash area - On the ADS board at reset time
	 * the bank 0 is already used to map the flash but has the base at
	 * 0xFE000000.  This is determined by the hard reset config word.
	 */
 
	/* load the base register 0 */
	lis		r5, HIADJ ((FLASH_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_16 | M8260_BR_V)
	addi	r5,	r5, LO ((FLASH_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_16 | M8260_BR_V)
	lis 	r6, HIADJ (M8260_BR0 (INTERNAL_MEM_MAP_ADDR))
	addi	r6,	r6, LO (M8260_BR0 (INTERNAL_MEM_MAP_ADDR)) 
	stw 	r5, 0(r6)
	
	/* load the option register 0 */
	lis	r5, HIADJ(	((~(FLASH_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_SCY_10_CLK)
	addi	r5,	r5, LO (((~(FLASH_SIZE - 1)) & M8260_OR_AM_MSK) |M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_SCY_10_CLK)
	lis 	r6, HIADJ (M8260_OR0 (INTERNAL_MEM_MAP_ADDR))
	addi	r6,	r6, LO (M8260_OR0 (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0 (r6)


	/*
	 * Map the CS 4 to the cpld area ;
	 */
 
	/* load the option register 0 */
	lis	    r5, HIADJ(((~(CS_A_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_ACS_DIV2 | M8260_OR_SCY_8_CLK | M8260_OR_TRLX)
	addi	r5,	r5, LO(((~(CS_A_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_EHTR_1 | M8260_OR_CSNT_EARLY | M8260_OR_ACS_DIV2 | M8260_OR_SCY_8_CLK | M8260_OR_TRLX)
	lis 	r6, HIADJ (M8260_OR4 (INTERNAL_MEM_MAP_ADDR))
	addi	r6,	r6, LO (M8260_OR4 (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0 (r6)

	/* load the base register 0 */
	lis		r5, HIADJ ((CS_A_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_32 | M8260_BR_V)
	addi	r5,	r5, LO ((CS_A_BASE_ADRS & M8260_BR_BA_MSK) | M8260_BR_PS_32 | M8260_BR_V)
	lis 	r6, HIADJ (M8260_BR4 (INTERNAL_MEM_MAP_ADDR))
	addi	r6,	r6, LO (M8260_BR4 (INTERNAL_MEM_MAP_ADDR)) 
	stw 	r5, 0(r6)


	/* program the SCCR: normal operation */
	lis 	r5, HIADJ (M8260_SCCR_DFBRG_16)
	addi	r5, r5, LO (M8260_SCCR_DFBRG_16)

	lis 	r6, HIADJ (M8260_SCCR (INTERNAL_MEM_MAP_ADDR))
	addi	r6, r6, LO (M8260_SCCR (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0(r6)

	/* program the BCR */

	lis     r5, 0x100c
	lis 	r6, HIADJ (M8260_BCR (INTERNAL_MEM_MAP_ADDR))
	addi	r6, r6, LO (M8260_BCR (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0 (r6)

	/* program the PPC_ACR */

	addi	r5, 0, 0x02
	lis 	r6, HIADJ (M8260_PPC_ACR (INTERNAL_MEM_MAP_ADDR))
	addi	r6, r6, LO (M8260_PPC_ACR (INTERNAL_MEM_MAP_ADDR))
	stb 	r5, 0(r6)


	/* program the PPC_ALRH */
#ifdef INCLUDE_PCI
	addis   r5, 0, 0x3012
	ori		r5, r5, 0x6745
#else
	addis   r5, 0, 0x0126
	ori		r5, r5, 0x7893
#endif /*INCLUDE_PCI*/
	lis 	r6, HIADJ (M8260_PPC_ALRH (INTERNAL_MEM_MAP_ADDR))
	addi	r6, r6, LO (M8260_PPC_ALRH (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0(r6)

	/* program the TESCR1 */

	addis   r5, 0, 0x0000
	ori	r5, r5, 0x4000
	lis 	r6, HIADJ (M8260_TESCR1 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_TESCR1 (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0(r6)

	/* program the LTESCR1 */

	addis   r5, 0, 0x0000
	ori	r5, r5, 0x0000
	lis 	r6, HIADJ (M8260_LTESCR1 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_LTESCR1 (INTERNAL_MEM_MAP_ADDR))
	stw 	r5, 0(r6)

	
SdramInit:

	/* program the MPTPR */

	addi    r5,0,0x2000	     /* MPTPR[PTP] */
	lis     r6, HIADJ (M8260_MPTPR (INTERNAL_MEM_MAP_ADDR))
	addi    r6, r6, LO (M8260_MPTPR (INTERNAL_MEM_MAP_ADDR))
	sth     r5, 0x0 (r6)      /* store upper half-word */

	/* program the PSRT */

	addi    r5,0,0x0C        /*SDRAM FRE:66MHZ*/
	lis     r6, HIADJ (M8260_PSRT (INTERNAL_MEM_MAP_ADDR))
	addi    r6, r6, LO (M8260_PSRT (INTERNAL_MEM_MAP_ADDR))
	stb     r5, 0x0 (r6)      /* store byte - bits[24-31] */


	/* load OR1 :  
	ROWST :A5 , ROW_line num :13 , and PSDMR [PBI] = 1 , 
	OR[ROWST] = 4,  101 (CONIFG AS 256M)
	OR[NUMR] = 13,  100
	*/    
	
	lis     r5, HIADJ (((~(BCTL_SDRAM_256M_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_SDRAM_BPD_4 | M8260_OR_SDRAM_ROWST_A4 | M8260_OR_SDRAM_NUMR_13 )
	addi    r5, r5, LO (((~(BCTL_SDRAM_256M_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_SDRAM_BPD_4 | M8260_OR_SDRAM_ROWST_A4 | M8260_OR_SDRAM_NUMR_13 )  
	lis     r6, HIADJ (M8260_OR1 (INTERNAL_MEM_MAP_ADDR))
	addi    r6, r6, LO (M8260_OR1 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load BR1 */

	lis 	r5, HIADJ ((LOCAL_MEM_LOCAL_ADRS & M8260_BR_BA_MSK) | M8260_BR_MS_SDRAM_60X | M8260_BR_V) 
	addi	r5, r5, LO ((LOCAL_MEM_LOCAL_ADRS & M8260_BR_BA_MSK) | M8260_BR_MS_SDRAM_60X | M8260_BR_V)
	lis     r6, HIADJ (M8260_BR1 (INTERNAL_MEM_MAP_ADDR))
	addi    r6, r6, LO (M8260_BR1 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)


	/* The following describes the bit settings of the PSDMR register */
	/* 
	* [PBI] = 1 On board SDRAM supporting page base interleaving
	* [RFEN] =0 Refresh services are off for now
	* [OP] = 101  precharge all bank
	* [SDAM] =011 Address multiplex size.
	* [BSMA] =001 Bank select multiplexed address line.,selects A13-A15 as bank select lines (4-bank)
	* [SDA10] =100  A6 is selected as control pin for SDA10 on SDRAM (PBI)
	* [RFRC] = From device data sheet  100 6 clocks Refresh recovery time.
	* [PRETOACT] =From device data sheet precharge-to-activate interval is 010 2 clock-cycle wait states	 
	* [ACTTOROW] = From device data sheet activate-to-read/write interval is 010 2 clock cycle
	* [BL] =0  Burst lenght is 4
	* [LDOTOPRE] = From device data sheet  last data out to precharge is 01 1 clock cycle
	* [WRC] = From device data sheet write recovery time is 10 2 clock cycles
	* [EAMUX] =0  no external address multiplexing
	* [BUFCMD] =0  normal timing for the control lines
	* [CL] = From device data sheet  CAS latency is 10 2 clock cycles   
	 */

	/*first,Prechargeg all bank , OP selects the Precharge all banks command */ 
	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_PRE_ALL | PSDMR_PARAM_256M_VALUE )  
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_PRE_ALL | PSDMR_PARAM_256M_VALUE )     
	lis     r6, HIADJ (M8260_PSDMR (INTERNAL_MEM_MAP_ADDR))
	addi    r6, r6, LO (M8260_PSDMR (INTERNAL_MEM_MAP_ADDR))
	stw	    r5, 0 (r6)

	lis     r0,HIADJ(LOCAL_MEM_LOCAL_ADRS)  
	addi    r0,r0,LO(LOCAL_MEM_LOCAL_ADRS)     

		/* do a single write to an arbitrary location */

	addi    r5,0,0xFF      /* Load 0xFF into r5 */
	stb     r5,0(r0)       /* Write 0xFF to address 0 - bits [24-31] */

	  
	/* issue a "CBR Refresh" command to SDRAM */	
	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_CBR | PSDMR_PARAM_256M_VALUE)
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_CBR | PSDMR_PARAM_256M_VALUE)
	stw	    r5, 0 (r6)

	/* Loop 8 times, writing 0xFF to address 0 */

	addi	r7,0,0x0008      /* loop 8 times */ 
	mtspr   CTR,r7           /* Load spr CTR with 8,counter register */
	addi 	r8,0,0x00FF      /* Load 0xFF into r8 */

SdramWrLoop:

	stb  	r8,0(r0)        	/* Write 0xFF to address 0 */
	bc   	16,0,SdramWrLoop 	/* Decrement CTR, and possibly branch */

	/* issue a "Mode Register Write" command to SDRAM */

	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_MODE | PSDMR_PARAM_256M_VALUE)
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_MODE | PSDMR_PARAM_256M_VALUE)
	stw	r5, 0 (r6)

	/* do a single write to an arbitrary location */

	addi    r8,0,0x00FF      /* Load 0x000000FF into r8 */
	stb     r8,0(r0)         /* Write 0xFF to address 0 - bits [24-31] */

	/* enable refresh services and put SDRAM into normal operation */

	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_NORM | M8260_PSDMR_RFEN | PSDMR_PARAM_256M_VALUE)
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_NORM | M8260_PSDMR_RFEN | PSDMR_PARAM_256M_VALUE)
	stw	r5, 0 (r6)

	/************************************************/
	/* start detect  the SDRAM	size	        */
	/************************************************/

      /* addresss BCTL_SDRAM_128M_SIZE and address (BCTL_SDRAM_128M_SIZE + 0x1000)  is the same address (because of the address multiplexed) */

	/*so ,FISRT step  is write to adresss  BCTL_SDRAM_128M_SIZE  =  a special value 0xaaaa5555*/
	lis		r5,	HIADJ (BCTL_SDRAM_128M_SIZE)	
	addi		r5,	r5,	LO (BCTL_SDRAM_128M_SIZE)

	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE)	
	addi		r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE)
	stw		r5, 0(r6)							/* write the memory. */    


	/*SECOND step is write to adresss  BCTL_SDRAM_128M_SIZE/2  =  another special value 0x5555aaaa*/
	lis		r5,	HIADJ (BCTL_SDRAM_128M_SIZE + 0x1000)	
	addi		r5,	r5,	LO (BCTL_SDRAM_128M_SIZE + 0x1000)

	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE + 0x1000)	
	addi		r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE + 0x1000)
	stw		r5, 0(r6)							/* write the memory. */    

	/*THIRD step is read from adresss  BCTL_SDRAM_128M_SIZE,  and compare with value that the FIRST step written*/
	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE)	
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + BCTL_SDRAM_128M_SIZE)	
	lwz		r5,	0(r6)						/* read	the	data from the memory. */
	
	cmpw	r5,	r6
	bne      	set_sdram_128M
	
	/* save the memery size var to internal memory of cpu*/
	lis		r5,	HIADJ (BCTL_SDRAM_256M_SIZE)	
	addi	r6,	r5,	LO (BCTL_SDRAM_256M_SIZE)	
	
	lis    	r6, HIADJ (BCTL_SDRAM_SIZE_SAVED_ADDR)
	addi    	r6, r6, LO (BCTL_SDRAM_SIZE_SAVED_ADDR)
	stw		r5, 0 (r6)
	
	b		bsp_check_sdram_start
	

	/************************************************/
	/* end check the SDRAM, confirm the SDRAM size , then re-initialize the register*/
	/************************************************/


set_sdram_128M: 

       lis     r5, HIADJ (((~(BCTL_SDRAM_128M_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_SDRAM_BPD_4 | M8260_OR_SDRAM_ROWST_A5 | M8260_OR_SDRAM_NUMR_13 )
	addi  r5, r5, LO (((~(BCTL_SDRAM_128M_SIZE - 1)) & M8260_OR_AM_MSK) | M8260_OR_SDRAM_BPD_4 | M8260_OR_SDRAM_ROWST_A5 | M8260_OR_SDRAM_NUMR_13 )  

	lis     r6, HIADJ (M8260_OR1 (INTERNAL_MEM_MAP_ADDR))
	addi    r6, r6, LO (M8260_OR1 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/*first,Prechargeg all bank , OP selects the Precharge all banks command */ 
	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_PRE_ALL | PSDMR_PARAM_128M_VALUE )  
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_PRE_ALL | PSDMR_PARAM_128M_VALUE )     
	lis    	r6, HIADJ (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	addi    	r6, r6, LO (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	stw		r5, 0 (r6)

	lis     r0,HIADJ(LOCAL_MEM_LOCAL_ADRS)  
	addi    r0,r0,LO(LOCAL_MEM_LOCAL_ADRS)     

	/* do a single write to an arbitrary location */

	addi    r5,0,0xFF      /* Load 0xFF into r5 */
	stb     r5,0(r0)       /* Write 0xFF to address 0 - bits [24-31] */
	  
	/* issue a "CBR Refresh" command to SDRAM */	
	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_CBR | PSDMR_PARAM_128M_VALUE)
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_CBR | PSDMR_PARAM_128M_VALUE)

	lis    	r6, HIADJ (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	addi    	r6, r6, LO (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	stw		r5, 0 (r6)

	/* Loop 8 times, writing 0xFF to address 0 */

	addi	r7,0,0x0008      /* loop 8 times */ 
	mtspr   CTR,r7           /* Load spr CTR with 8,counter register */
	addi 	r8,0,0x00FF      /* Load 0xFF into r8 */

SdramWrLoop1:

	stb  	r8,0(r0)        	/* Write 0xFF to address 0 */
	bc   	16,0, SdramWrLoop1 	/* Decrement CTR, and possibly branch */

	/* issue a "Mode Register Write" command to SDRAM */

	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_MODE | PSDMR_PARAM_128M_VALUE)
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_MODE | PSDMR_PARAM_128M_VALUE)

	lis    	r6, HIADJ (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	addi    	r6, r6, LO (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	stw		r5, 0 (r6)

	/* do a single write to an arbitrary location */

	addi    r8,0,0x00FF      /* Load 0x000000FF into r8 */
	stb     r8,0(r0)         /* Write 0xFF to address 0 - bits [24-31] */

	/* enable refresh services and put SDRAM into normal operation */

	lis     r5,HIADJ(M8260_PSDMR_PBI | M8260_PSDMR_OP_NORM | M8260_PSDMR_RFEN | PSDMR_PARAM_128M_VALUE)
	addi    r5,r5,LO(M8260_PSDMR_PBI | M8260_PSDMR_OP_NORM | M8260_PSDMR_RFEN | PSDMR_PARAM_128M_VALUE)
	lis    	r6, HIADJ (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	addi    	r6, r6, LO (M8260_PSDMR(INTERNAL_MEM_MAP_ADDR))
	stw		r5, 0 (r6)
	isync

	/* save the memery size var to internal memory of cpu*/
	lis		r5,	HIADJ (BCTL_SDRAM_128M_SIZE)	
	addi	r6,	r5,	LO (BCTL_SDRAM_128M_SIZE)	

	lis    	r6, HIADJ (BCTL_SDRAM_SIZE_SAVED_ADDR)
	addi    	r6, r6, LO (BCTL_SDRAM_SIZE_SAVED_ADDR)
	stw		r5, 0 (r6)

	/************************************************/
	/* start check the SDRAM		        */
	/************************************************/

bsp_check_sdram_start:

	/* FIRST STEP: Turn on instruction cache , to fasten the speed of sdram	checking. */
#if 1
	addis    r6,0,0xffbf    
	ori      r6,r6,0x8088	 

/*  addi    r5,0,0x8088      */	
	xor      r5,r5,r5    
	ori       r5,0,0x8088    
	
	mfspr   r7, HID0
	isync
	sync			/* synchronize */

	and	    r7,r7,r6
	or      r7,r7,r5
	mtspr	HID0,r7
	isync
	sync			/* synchronize */
#else
	/* Turn off data and instruction cache control bits */
	mfspr   r7, HID0
	isync
	sync				/* synchronize */
	andi.	r7,r7,0x3FFF   		/* Clear DCE and ICE bits */
	mtspr	HID0,r7
	isync
	sync				/* synchronize */
#endif	

	/* SECOND STEP:	check sdram	using fixed	data. */

	/*
	 * local register description.
	 * r4 = watchdong
	 * r5 =	variable;
	 * r6 =	memory address pointer;
	 * r7 =	memory end address;
	 * r8 =	transmit variable
	 * r11,	r12	= time counter and reference;
	 * r15,	r16, r17, r18 =	the	test data.
 	 * r23(BCTL LED) and r24 (CPLD LED)	light control register address(PA DATA) 
	 */

	/* first, prepare to check the sdram. */
	lis		r15, HIADJ (0x5a5a5a5a)
	addi	r15, r15, LO (0x5a5a5a5a)
	lis		r16, HIADJ (0xa5a5a5a5)
	addi	r16, r16, LO (0xa5a5a5a5)
	lis		r17, HIADJ (0x5aa55aa5)
	addi	r17, r17, LO (0x5aa55aa5)
	lis		r18, HIADJ (0xa55aa55a)
	addi	r18, r18, LO (0xa55aa55a)

    /* r23,	light control register address(PA DATA) */
   	lis 	r23, HIADJ (BSP_LIGHT_REG_1)	
	addi	r23, r23, LO (BSP_LIGHT_REG_1)

    /* r24,	light control register address(CPLD) */
   	lis 	r24, HIADJ (BSP_LIGHT_REG)	
	addi	r24, r24, LO (BSP_LIGHT_REG)
	
   	lis 	r21, HIADJ (BSP_DOG_CLR_REG)	
	addi	r21, r21, LO (BSP_DOG_CLR_REG)	
	
	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)	/* set the start address. */
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)

	lis		r7,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET + MEM_CHECK_SIZE)	/* set the end address.	*/
	addi	r7,	r7,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET + MEM_CHECK_SIZE)      

	lis		r11, HIADJ (0x40000)		/* set the time	counter.*/
	addi	r11, r11, LO (0x40000)
	xor		r12, r12, r12

	lis	    r22, HIADJ (BSP_LED_RED_1)		 
	addi	r22, r22, LO (BSP_LED_RED_1)
	stw     r22,0(r23)
    
	lis	    r22, HIADJ (BSP_LED_RED | BSP_LED_GREEN | BSP_DUAL_LED_GREEN)		 
	addi	r22, r22, LO (BSP_LED_RED | BSP_LED_GREEN | BSP_DUAL_LED_GREEN)
	stw     r22,0(r24)
	
	/* second, Write the memory	*/
		
memWrite1:

	addi	r12, r12, 1			/* counting	the	time. */
	cmpw	r12, r11
	bne	    clrWatchDog_11

	/* periodly	clear software watch dog and show the light. */	
	xor		r12, r12, r12		/* clear the counter */

    /* flash red light. other light unchange*/
	lis 	r22, HIADJ (BSP_LED_RED_1)		
	addi	r22, r22, LO (BSP_LED_RED_1)

	lwz 	r20, 0(r23)
	xor 	r20, r20, r22
	stw	    r20, 0(r23)

    /* flash red light. other light unchange*/
	lis 	r22, HIADJ (BSP_LED_RED)		
	addi	r22, r22, LO (BSP_LED_RED)

	lwz 	r20, 0(r24)
	xor 	r20, r20, r22
	stw	    r20, 0(r24)
	
    /*clear dog by i/o*/
	lis	    r22, HIADJ(BSP_DOG_CLR)	/*write 1 to BIT27*/
	addi	r22, r22, LO(BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	or 	    r20, r20, r22
	stw	    r20, 0(r21)    

	lis	    r22, HIADJ(~BSP_DOG_CLR)	/*write 0 to BIT27*/
	addi	r22, r22, LO(~BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	and	    r20, r20, r22
	stw	    r20, 0(r21)    

	
clrWatchDog_11:

	stw		r15, 0(r6)				/* write the memory. */
	stw		r16, 4(r6)
	stw		r17, 8(r6)
	stw		r18, 12(r6)

	addi	r6,	r6,	0x10				/* move	the	address	pointer. */
	cmpw	r6,	r7					/* compare the curent pointer to */
								/* the memory end address. */

	blt		memWrite1				/* loop	to set the all memory. */

    
	/* third, prepare to check the memory. */
	lis 	r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)	/* set the start address again.	*/
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)

	lis	    r22, HIADJ (BSP_LED_GREEN_1)		/* light the red light*/
	addi	r22, r22, LO (BSP_LED_GREEN_1)
	stw     r22,0(r23)	

	lis	    r22, HIADJ (BSP_LED_GREEN)		/* light the red light*/
	addi	r22, r22, LO (BSP_LED_GREEN)
	stw     r22,0(r24)	
	
	xor	    r12, r12, r12			/* time	counter	clear. */

	/* forth, check	the	memory's context is	all	right or not. */	
memCheck1:

	addi	r12, r12, 1
	cmpw	r12, r11
	bne	    clrWatchDog_12

	xor	r12, r12, r12			/* clear the counter */

    /* flash green light. other light unchange*/
	lis	    r22, HIADJ (BSP_LED_GREEN_1)        
	addi	r22, r22, LO (BSP_LED_GREEN_1)

	lwz 	r20, 0(r23)
	xor	    r20, r20, r22
	stw	    r20, 0(r23)

    /* flash dual green light. other light unchange*/
	lis	    r22, HIADJ (BSP_DUAL_LED_GREEN)        
	addi	r22, r22, LO (BSP_DUAL_LED_GREEN)

	lwz 	r20, 0(r24)
	xor	    r20, r20, r22
	stw	    r20, 0(r24)
	
    /*clear dog by i/o*/
	lis	    r22, HIADJ(BSP_DOG_CLR)	/*write 1 to BIT27*/
	addi	r22, r22, LO(BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	or 	    r20, r20, r22
	stw	    r20, 0(r21)    

	lis	    r22, HIADJ(~BSP_DOG_CLR)	/*write 0 to BIT27*/
	addi	r22, r22, LO(~BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	and	    r20, r20, r22
	stw	    r20, 0(r21)    


clrWatchDog_12:
    
	lwz		r5,	0(r6)		/* read	the	data from the memory. */
	cmpw	r5,	r15
	bne		death_pointer

	lwz		r5,	4(r6)
	cmpw	r5,	r16
	bne		death_pointer

	lwz		r5,	8(r6)
	cmpw	r5,	r17
	bne		death_pointer

	lwz		r5,	12(r6)
	cmpw	r5,	r18
	bne		death_pointer
	
	addi	r6,	r6,	0x10		/* move	the	address	pointer. */
	cmpw	r6,	r7			/* compare the curent pointer to */
						/* the memory end address. */
														
	blt		memCheck1		/* loop	to check the all memory. */

	b   	memCheckPassed


	/* 4th STEP: check sdram using address as data.	*/
	/* 5th STEP: check sdram using full	mathod.	*/
	

	/* 6th STEP: the SDRAM checking	is failed. Waiting for death. */

death_pointer:
	lis	    r22, HIADJ (BSP_LED_ALL_1)		/* dark all... */
	addi	r22, r22, LO (BSP_LED_ALL_1)
	stw     r22,0(r23)

	lis	    r22, HIADJ (BSP_LED_RED|BSP_LED_GREEN)		/* dark all... */
	addi	r22, r22, LO (BSP_LED_RED|BSP_LED_GREEN) 
	stw     r22,0(r24)
	
death_loop:
	xor	r20, r20, r20			/* relax 1 second... */
	lis	r21, HIADJ (0x80000)
	addi	r21, r21, LO(0x80000)
ddelay_deatchloop:
	addi	r20, r20, LO(1)
	cmpw	r20, r21
	bne	ddelay_deatchloop

    lis 	r22, HIADJ (BSP_LED_ALL_1)		/* flash all...	*/
    addi	r22, r22, LO (BSP_LED_ALL_1)

	lwz 	r20, 0(r23)
	xor	    r22, r20, r22
	stw	    r22, 0(r23)

    lis 	r22, HIADJ (BSP_LED_ALL)		/* flash all...	*/
    addi	r22, r22, LO (BSP_LED_ALL)

	lwz 	r20, 0(r24)
	xor	    r22, r20, r22
	stw	    r22, 0(r24)

	
	b	death_loop


	/* 7th STEP: the SDRAM check is	passed.	*/
	
memCheckPassed:
	lis	    r22, HIADJ (BSP_LED_ALL_1)		/* dark	all... */
	addi	r22, r22, LO (BSP_LED_ALL_1)
	stw     r22,0(r23)

	lis	    r22, HIADJ (BSP_LED_RED|BSP_LED_GREEN)		/* dark	all... */
	addi	r22, r22, LO (BSP_LED_RED|BSP_LED_GREEN) 
	stw     r22,0(r24)	
    
	/* 8th STEP: Turn off data and instruction caches */

	/************************************************/
	/*	end   check	the	SDRAM		*/
	/************************************************/
    /* initialize the stack pointer */

	lis	    sp, HIADJ(STACK_ADRS)
	addi	sp, sp, LO(STACK_ADRS)

	/* go to C entry point */

	addi	sp, sp, -FRAMEBASESZ		/* get frame stack */

	/* 
	 * calculate C entry point: routine - entry point + ROM base 
	 * routine	= romStart
	 * entry point	= romInit	= R7
	 * ROM base	= ROM_TEXT_ADRS = R8
	 * C entry point: romStart - R7 + R8 
	 */
#ifndef ALPHA_VERSION
    lis     r7, HIADJ(romInit)
    addi    r7, r7, LO(romInit)

    lis 	r6, HIADJ(romStart)	
    addi	r6, r6, LO(romStart)	/* load R6 with C entry point */

    lis     r8, HIADJ(ROM_TEXT_ADRS)
    addi    r8, r8, LO(ROM_TEXT_ADRS)
    
	sub 	r6, r6, r7		/* routine - entry point */
	add	    r6, r6, r8 		/* + ROM base */

    mr		r3, r31		/* save	start type parameter in r3 */
	mtspr	LR, r6		/* save destination address*/
    					/* into LR register */
	blr				/* jump to the C entry point */
#else
    b bsp_check_sdram_start

/*TEST CODE*/
    test_led:
    /* r23,	light control register address(PA DATA) */
   	lis 	r23, HIADJ (BSP_LIGHT_REG)	
	addi	r23, r23, LO (BSP_LIGHT_REG)

	lis	    r22, HIADJ (BSP_LED_RED)		
	addi	r22, r22, LO (BSP_LED_RED)
	stw     r22,0(r23)

	lis		r11, HIADJ (0x40000)		/* set the time	counter.*/
	addi	r11, r11, LO (0x40000)
	xor		r12, r12, r12

Loop_Delay:
	addi	r12, r12, 1			/* counting	the	time. */
	cmpw	r12, r11
	bne 	Loop_Delay	

	lis	    r22, HIADJ (BSP_LED_RED | BSP_LED_GREEN)		
	addi	r22, r22, LO (BSP_LED_RED | BSP_LED_GREEN)
	stw     r22,0(r23)

    xor		r12, r12, r12
Loop_Delay1:
	addi	r12, r12, 1			/* counting	the	time. */
	cmpw	r12, r11
	bne 	Loop_Delay1	

    b       test_led 	      

#endif /*ALPHA_VERSION*/
