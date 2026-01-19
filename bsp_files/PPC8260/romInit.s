/* romInit.s - Motorola ads8260 ROM initialization module */

/* Copyright 1984-2002 Wind River Systems, Inc. */
	.data
	.globl  copyright_wind_river
	.long   copyright_wind_river
/*
modification history
--------------------
01i,10dec01,jrs  change copyright date
01h,17oct01,jrs  Upgrade to veloce
		 set MPTPR[PTP] to PTP and PSRT to PSRT_VALUE
		 setting refresh correctly for the oscillator frequ - SPR #66989
01g,08may01,pch  Add assembler abstractions (FUNC_EXPORT, FUNC_BEGIN, etc.)
01f,14mar00,ms_  add support for PILOT revision of board
01e,04mar00,ml   changes for board rev "PILOT" (SIUMCR.DPPC/L2CPC & PSDMR.BSMA)
01d,16sep99,ms_  get some .h files from h/drv instead of locally
01c,15jul99,ms_  make compliant with our coding standards
01b,20may99,cn   fixed include m8260Cpm.h
01a,19may99,cn   written from ads860/romInit.s, 01m.
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
#include "cacheLib.h "
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

	.extern romStart	 /* system initialization routine */

	.text

 .fill   1,1,HRCW_BYTE_0    /* byte 0 (MSByte) of the configuration master's */
                            /* Hard Reset Configuration Word */
 
 .fill   7,1,0              /* Fill with 7 zeros */
 .fill   1,1,HRCW_BYTE_1
 .fill   7,1,0
 .fill   1,1,HRCW_BYTE_2
 .fill   7,1,0
 .fill   1,1,HRCW_BYTE_3    /* This is the LSByte */
 .fill   231,1,0            /* The rest of the space are filled with zeros */
	.align 2

/******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*
*
* romInit
*     (
*     int startType	/@ only used by 2nd entry point @/
*     )

*/


FUNC_BEGIN(_romInit)
FUNC_BEGIN(romInit)

        bl	cold		/* jump to the cold boot initialization */
	nop
	bl	start		/* jump to the warm boot initialization */

	/* copyright notice appears at beginning of ROM (in TEXT segment) */
    /* notice:the ascii text must be mutiplier of 4,or it will cause compiler error */
	.ascii	BSP_COPYRIGHT
	.align 4


cold:
        li      r3, BOOT_COLD   /* set cold boot as start type */

	lis	r5, HIADJ (OR0_VAL_L) /* R5 holds the OR0 content */
	addi	r5, r5, LO (OR0_VAL_L)
	lis	r6, HIADJ (0x0f010104)	/*option register for bank 0, Clear out all mask bits */
	addi	r6, r6, LO (0x0f010104) 
	stw	r5, 0 (r6) 
        isync    /* synchronize */
        
	/*
	 * initialize the IMMR register before any non-core registers
	 * modification. The default IMMR base address was 0x0F000000,
	 * as originally programmed in the Hard Reset Configuration Word.
	 */

	lis	r5, HIADJ (INTERNAL_MEM_MAP_ADDR)	
	addi	r5, r5, LO (INTERNAL_MEM_MAP_ADDR)
	lis	r6, HIADJ (0x0f0101a8)	/*IMMR was at 0x0f000000 */
	addi	r6, r6, LO (0x0f0101a8) 
	stw     r5, 0 (r6)	        /* IMMR now at 0xfff00000 */
        isync

	/*
	 * When the PowerPC 8260 is powered on, the processor fetches the
	 * instructions located at the address 0x100. We need to jump
	 * from the address 0x100 to the Flash space.
	 */
	 
        lis     r4, HIADJ(start)                /* load r4 with the address */
        addi    r4, r4, LO(start)               /* of start */
 
        lis     r5, HIADJ(romInit)              /* load r5 with the address */
        addi    r5, r5, LO(romInit)             /* of romInit() */
 
        lis     r6, HIADJ(ROM_TEXT_ADRS)        /* load r6 with the address */
        addi    r6, r6, LO(ROM_TEXT_ADRS)       /* of ROM_TEXT_ADRS */
 
        sub     r4, r4, r5                      /*  */
        add     r4, r4, r6
 
        mtspr   LR, r4                          /* save destination address*/
                                                /* into LR register */
        blr                                     /* jump to flash mem address */

start:
       
	/* set the MSR register to a known state */
        /* note: address translate function is open in core by os,we couldn't see the code,added by bjm*/ 
	xor	r0, r0, r0		/* clear register R0 */
	isync				/* synchronize */
	mtmsr 	r0			/* clear the MSR register */
	isync				/* synchronize */

	/* program the SCCR: normal operation */
	
	lis	r5, HIADJ (M8260_SCCR_DFBRG_16)
	addi	r5, r5, LO (M8260_SCCR_DFBRG_16)
	lis	r6, HIADJ (M8260_SCCR (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_SCCR (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)
  
	/* SYPCR - turn off the system protection stuff */
#if 0  
	lis	r5, HIADJ (SYPCR_VAL)
	addi	r5, r5, LO (SYPCR_VAL)
	lis	r6, HIADJ (M8260_SYPCR (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_SYPCR (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)
#endif
	/* program the BCR */
	lis	r5, HIADJ (BCR_VAL)
	addi	r5, r5, LO (BCR_VAL)
	lis	r6, HIADJ (M8260_BCR (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BCR (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* program the PPC_ACR ,8 bit*/
	addi    r5, 0, PPC_ACR_VAL
	lis	r6, HIADJ (M8260_PPC_ACR (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_PPC_ACR (INTERNAL_MEM_MAP_ADDR))
	stb	r5, 0 (r6)

	/* program the PPC_ALRH */
	lis	r5, HIADJ (PPC_ALRH)
	addi    r5, r5, LO (PPC_ALRH)
	lis	r6, HIADJ (M8260_PPC_ALRH (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_PPC_ALRH (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

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

   /************************************************/
	/* phase1:Inialize I/0 which control the led   */
	/************************************************/	
	lis	    r22,HIADJ (0xBC7C3FCC)  /*set some pin as output*/  
	addi	r22, r22, LO (0xBC7C3FCC)
	lis	    r23, HIADJ (0xfff10d00)	
	addi	r23, r23, LO (0xfff10d00)
	stw	    r22, 0(r23) 	

	lis	    r22, HIADJ (BSP_LED_ALL)	   /*dark all lamp*/
	addi	r22, r22, LO (BSP_LED_ALL)
	lis	    r23, HIADJ (BSP_LIGHT_REG)	
	addi	r23, r23, LO (BSP_LIGHT_REG)
	stw     r22,0(r23)        

   /************************************************/
	/* phase1:Inialize the CS0	                */
	/************************************************/
	
	/*
	 * Map the bank 0 to the bootrom and flash area - On the ADS board at reset time
	 * the bank 0 is already used to map the flash.
	 */

	/* load the base register 0 */ 
        lis     r5, HIADJ (BR0_VAL)
        addi    r5, r5, LO (BR0_VAL)
	lis	r6, HIADJ (M8260_BR0 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BR0 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)
	
	/* load the option register 0*/
        lis     r5, HIADJ (OR0_VAL)
        addi    r5, r5, LO (OR0_VAL)
	lis	r6, HIADJ (M8260_OR0 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_OR0 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/************************************************/
	/* phase2:Inialize the SDRAM	                */
	/************************************************/
SdramInit:

	/* program the MPTPR */

	addi    r5,0,MPTPR_VAL 
        lis     r6, HIADJ (M8260_MPTPR (INTERNAL_MEM_MAP_ADDR))
        addi    r6, r6, LO (M8260_MPTPR (INTERNAL_MEM_MAP_ADDR))
	sth     r5, 0x0 (r6)      /* store upper half-word */

	/* program the PSRT */
  
	addi    r5,0,PSRT_VAL      
        lis     r6, HIADJ (M8260_PSRT (INTERNAL_MEM_MAP_ADDR))
        addi    r6, r6, LO (M8260_PSRT (INTERNAL_MEM_MAP_ADDR))
	stb     r5, 0x0 (r6)      /* store byte - bits[24-31] */
 
	/* load OR3,SDRAM */

        lis     r5, HIADJ (OR3_VAL)
        addi    r5, r5, LO (OR3_VAL)
        lis     r6, HIADJ (M8260_OR3 (INTERNAL_MEM_MAP_ADDR))
        addi    r6, r6, LO (M8260_OR3 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load BR3,SDRAM */

	lis	r5, HIADJ (BR3_VAL)	
	addi	r5, r5, LO (BR3_VAL)
        lis     r6, HIADJ (M8260_BR3 (INTERNAL_MEM_MAP_ADDR))
        addi    r6, r6, LO (M8260_BR3 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* 
	 * program the PSDMR as explained below: 
	 * PBI is set to one, support page-based interleaving 
	 * Refresh services are off for now
	 * OP selects the "Precharge all banks" command
	 * SDAM = b001
	 * BSMA selects A15-A17 as bank select lines 
	 * A9 is selected as control pin for SDA10 on SDRAM
	 * 7-clock refresh recovery time
	 * precharge-to-activate interval is 3-clock time
	 * activate-to-read/write interval is 2-clock time
	 * Burst lenght is 4
	 * last data out to precharge is 1 clock
	 * write recovery time is 1 clock
	 * no external address multiplexing
	 * normal timing for the control lines
	 * CAS latency is 2
	 */
	 
	/*first,Prechargeg all bank*/ 

	addis   r5,0,PSDMR_101_H /*101*/    
 	ori     r5,r5,PSDMR_VAL_L
        lis     r6, HIADJ (M8260_PSDMR (INTERNAL_MEM_MAP_ADDR))
        addi    r6, r6, LO (M8260_PSDMR (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	addis   r0,0,0  /*r0 at 0*/
 
 	/* do a single write to an arbitrary location */

	addi    r5,0,0xFF      /* Load 0xFF into r5 */
	stb     r5,0(r0)       /* Write 0xFF to address 0 - bits [24-31] */

      
	/* issue a "CBR Refresh" command to SDRAM */

	addis   r5,0,PSDMR_001_H   /*001*/
	ori     r5,r5,PSDMR_VAL_L
	stw	r5, 0 (r6)         /*now r6 represent PSDMR ADDR*/		
	
	/* Loop 8 times, writing 0xFF to address 0 */

	addi	r7,0,0x0008      /* loop 8 times */ 
	mtspr   CTR,r7           /* Load spr CTR with 8,counter register */
	addi 	r8,0,0x00FF      /* Load 0xFF into r8 */

SdramWrLoop:

	stb  	r8,0(r0)        	/* Write 0xFF to address 0 */
	bc   	16,0,SdramWrLoop 	/* Decrement CTR, and possibly branch */


	/* issue a "Mode Register Write" command to SDRAM */

	addis   r5,0,PSDMR_011_H  /*011*/
	ori     r5,r5,PSDMR_VAL_L
	stw	r5, 0 (r6)   /*now r6 represent PSDMR ADDR*/
 
	/* do a single write to an arbitrary location */

	addi    r8,0,0xFF    /* Load 0xFF into r8 */
	stb     r8,0x0(r0)       /* Write 0xFF to address 0x0 - bits [24-31] */

	/* enable refresh services and put SDRAM into normal operation */
	
	addis   r5,0,PSDMR_000_H /*000*/
	ori     r5,r5,PSDMR_VAL_L
	stw	r5, 0 (r6)

	/************************************************/
	/* phase3:Inialize CS1,CS2,CS4	                */
	/************************************************/

	/* load the option register 1*/
        lis     r5, HIADJ (OR1_VAL)
        addi    r5, r5, LO (OR1_VAL)
	lis	r6, HIADJ (M8260_OR1 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_OR1 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the base register 1*/
        lis     r5, HIADJ (BR1_VAL)
        addi    r5, r5, LO (BR1_VAL)
	lis	r6, HIADJ (M8260_BR1 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BR1 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the option register 2*/
        lis     r5, HIADJ (OR2_VAL)
        addi    r5, r5, LO (OR2_VAL)
	lis	r6, HIADJ (M8260_OR2 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_OR2 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the base register 2*/
        lis     r5, HIADJ (BR2_VAL)
        addi    r5, r5, LO (BR2_VAL)
	lis	r6, HIADJ (M8260_BR2 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BR2 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the option register 4*/
        lis     r5, HIADJ (OR4_VAL)
        addi    r5, r5, LO (OR4_VAL)
	lis	r6, HIADJ (M8260_OR4 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_OR4 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the base register 4*/
        lis     r5, HIADJ (BR4_VAL)
        addi    r5, r5, LO (BR4_VAL)
	lis	r6, HIADJ (M8260_BR4 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BR4 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the option register 9*/
        lis     r5, HIADJ (OR9_VAL)
        addi    r5, r5, LO (OR9_VAL)
	lis	r6, HIADJ (M8260_OR9 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_OR9 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/* load the base register 9*/
        lis     r5, HIADJ (BR9_VAL)
        addi    r5, r5, LO (BR9_VAL)
	lis	r6, HIADJ (M8260_BR9 (INTERNAL_MEM_MAP_ADDR))	
	addi	r6, r6, LO (M8260_BR9 (INTERNAL_MEM_MAP_ADDR))
	stw	r5, 0 (r6)

	/****************************************/
	/* phase4:initilize watchdog	        */
	/****************************************/
	lis	r6, HIADJ(0x3800002C)
	addi	r6, r6, LO(0x3800002C)
	
	lbz		r5, 0(r6)
	xor		r7,	r7,	r7
	rlwinm	r7,	r5,	0x0, 28, 31
    	
	lis		r5,	HIADJ( M8260_SYPCR_SWTC |M8260_SYPCR_BMT |M8260_SYPCR_PBME |M8260_SYPCR_LBME)
	addi	r5,	r5,        LO( M8260_SYPCR_SWTC |M8260_SYPCR_BMT |M8260_SYPCR_PBME |M8260_SYPCR_LBME)

	lis		r6,	HIADJ(M8260_SYPCR_SWE | M8260_SYPCR_SWRI |M8260_SYPCR_SWP)
	addi	r6,	r6,        LO(M8260_SYPCR_SWE | M8260_SYPCR_SWRI |M8260_SYPCR_SWP)

	cmpwi	r7,	0x0c                /*default is 0x0f*/
	beq		bsp_watch_dog_set

	or		r5,	r5,	r6

bsp_watch_dog_set:

	lis	r10, HIADJ (M8260_SYPCR (INTERNAL_MEM_MAP_ADDR))	
	addi	r10, r10, LO (M8260_SYPCR (INTERNAL_MEM_MAP_ADDR))
	stw	r5,	0(r10)
	lis	r10, HIADJ (M8260_SWSR (INTERNAL_MEM_MAP_ADDR))	/* clear the software watchdog. */
	addi	r10, r10, LO (M8260_SWSR (INTERNAL_MEM_MAP_ADDR))
	li		r25, 0x556C
	sth		r25, 0(r10)
//	li		r25, 0xAA39
	xor     r25,r25,r25    
	ori		r25,0,0xAA39 
	sth		r25, 0(r10)


        /* Zero-out registers: SPRGs */
 
	addis    r0,0,0
	isync				/* synchronize */

    mtspr   272,r0
    mtspr   273,r0
    mtspr   274,r0
    mtspr   275,r0
	isync				/* synchronize */
 
    /* zero-out the Segment registers */

    mtsr    0,r0
    isync
    mtsr    1,r0
    isync
    mtsr    2,r0
    isync
    mtsr    3,r0
    isync
    mtsr    4,r0
    isync
    mtsr    5,r0
    isync
    mtsr    6,r0
    isync
    mtsr    7,r0
    isync
    mtsr    8,r0
    isync
    mtsr    9,r0
    isync
    mtsr    10,r0
    isync
    mtsr    11,r0
    isync
    mtsr    12,r0
    isync
    mtsr    13,r0
    isync
    mtsr    14,r0
    isync
    mtsr    15,r0
    isync

	/* invalidate DBATs: clear VP and VS bits */

	mtspr   536,r0   /* Data bat register 0 upper */
	isync
	mtspr   538,r0   /* Data bat register 1 upper */
	isync
	mtspr   540,r0   /* Data bat register 2 upper */
	isync
	mtspr   542,r0   /* Data bat register 3 upper */
	isync
	
	/* invalidate IBATs: clear VP and VS bits */

	mtspr   528,r0   /* Instruction bat register 0 upper */
	isync
	mtspr   530,r0   /* Instruction bat register 1 upper */
	isync
	mtspr   532,r0   /* Instruction bat register 2 upper */
	isync
	mtspr   534,r0   /* Instruction bat register 3 upper */
	isync

	/* invalidate TLBs: loop on all TLB entries using r7 as an index */

	addi     r0,0,0x0020
	mtspr    9,r0			/* Load CTR with 32 */
	addi     r7,0,0            	/* Use r7 as the tlb index */
 
tlb_write_loop:
 
	tlbie    r7                	/* invalidate the tlb entry */
	sync
	addi     r7,r7,0x1000          	/* increment the index */
	bc       16,0,tlb_write_loop   	/* Decrement CTR, then branch if the */
				       	/* decremented CTR is not equal to 0 */
    	  
         /* Turn off data and instruction cache control bits */
#if 0 
    mfspr   r7, HID0
    isync
	sync				/* synchronize */
	andi.	r7,r7,0x3FFF   		/* Clear DCE and ICE bits */
	mtspr	HID0,r7
        isync
	sync				/* synchronize */
#endif	

	/************************************************/
	/* phase5:start check the SDRAM		        */
	/************************************************/

bsp_check_sdram_start:

	/* FIRST STEP: Turn on instruction cache , to fasten the speed of sdram	checking. */

    addis    r4,0,0xffbf    
    ori      r4,r4,0x8088	 

/*  addi    r5,0,0x8088      */	
	xor      r5,r5,r5    
	ori       r5,0,0x8088     
            
    mfspr   r7, HID0
    isync
    sync			/* synchronize */

    and	    r7,r7,r4
    or      r7,r7,r5
    mtspr	HID0,r7
    isync
    sync			/* synchronize */

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
 	 * r23,	light control regester address(PA DATA) 
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

    /* r23,	light control regester address(PA DATA) */
   	lis 	r23, HIADJ (BSP_LIGHT_REG)	
	addi	r23, r23, LO (BSP_LIGHT_REG)

	
	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET)	/* set the start address. */
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)

	lis		r7,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET + MEM_CHECK_SIZE)	/* set the end address.	*/
	addi	r7,	r7,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET + MEM_CHECK_SIZE)    

	lis		r11, HIADJ (0x40000)		/* set the time	counter.*/
	addi	r11, r11, LO (0x40000)
	xor		r12, r12, r12
   
	lis	    r22, HIADJ (BSP_LED_RED | BSP_LED_GREEN | BSP_DUAL_LED_GREEN)		 
	addi	r22, r22, LO (BSP_LED_RED | BSP_LED_GREEN | BSP_DUAL_LED_GREEN)
	stw     r22,0(r23)

	/* second, Write the memory	*/
		
memWrite1:

	addi	r12, r12, 1			/* counting	the	time. */
	cmpw	r12, r11
	bne	clrWatchDog_11

	/* periodly	clear software watch dog and show the light. */
	
	xor		r12, r12, r12		/* clear the counter */

    /* flash red light. other light unchange*/
	lis 	r22, HIADJ (BSP_LED_RED)		
	addi	r22, r22, LO (BSP_LED_RED)

	lwz 	r20, 0(r23)
	xor 	r20, r20, r22
	stw	    r20, 0(r23)

	
	lis	r4, HIADJ (M8260_SWSR (INTERNAL_MEM_MAP_ADDR))	/* clear the software watchdog. */
	addi	r4, r4, LO (M8260_SWSR (INTERNAL_MEM_MAP_ADDR))
	li		r25, 0x556C		
	sth		r25, 0(r4)
//	li		r25, 0xAA39
	xor     r25,r25,r25    
	ori		r25,0,0xAA39 
	sth		r25, 0(r4)

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
	
	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET)	/* set the start address. */
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)

	lis	    r22, HIADJ  (/*BSP_LED_RED|*/  BSP_LED_GREEN | BSP_DUAL_LED_GREEN)		/* light the red light*/
	addi	r22, r22, LO  (/*BSP_LED_RED | */ BSP_LED_GREEN | BSP_DUAL_LED_GREEN)	
	stw     r22,0(r23)	
	
	xor	r12, r12, r12			/* time	counter	clear. */

	/* forth, check	the	memory's context is	all	right or not. */
	
memCheck1:

	addi	r12, r12, 1
	cmpw	r12, r11
	bne	clrWatchDog_12

	xor	r12, r12, r12			/* clear the counter */

    /* flash green light. other light unchange*/
	lis	    r22, HIADJ (BSP_DUAL_LED_GREEN)        
	addi	r22, r22, LO (BSP_DUAL_LED_GREEN)

	lwz 	r20, 0(r23)
	xor	    r20, r20, r22
	stw	    r20, 0(r23)

	li		r25, 0x556C		
	sth		r25, 0(r4)
	
//	li		r25, 0xAA39
	xor     r25,r25,r25    
	ori		r25,0,0xAA39    	
	
	sth		r25, 0(r4)


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

    xor    r22 ,r22, r22
	stw		r22, 0(r6)				/* write the memory to zero */
	stw		r22, 4(r6)              /* write the memory to zero */
	stw		r22, 8(r6)              /* write the memory to zero */
	stw		r22, 12(r6)             /* write the memory to zero */
	
	addi	r6,	r6,	0x10		/* move	the	address	pointer. */
	cmpw	r6,	r7			/* compare the curent pointer to */
						/* the memory end address. */
														
	blt		memCheck1		/* loop	to check the all memory. */

	b	memCheckPassed


	/* 4th STEP: check sdram using address as data.	*/
	/* 5th STEP: check sdram using full	mathod.	*/
	

	/* 6th STEP: the SDRAM checking	is failed. Waiting for death. */

death_pointer:

	lis	    r22, HIADJ (BSP_LED_ALL)		/* dark	all... */
	addi	r22, r22, LO (BSP_LED_ALL) 
	stw     r22,0(r23)

death_loop:
	xor	r20, r20, r20			/* relax 1 second... */
	lis	r21, HIADJ (0x80000)
	addi	r21, r21, LO(0x80000)
ddelay_deatchloop:
	addi	r20, r20, LO(1)
	cmpw	r20, r21
	bne	ddelay_deatchloop

    lis 	r22, HIADJ (/*BSP_LED_RED | BSP_LED_GREEN | */BSP_DUAL_LED_GREEN)		/* flash all...	*/
    addi	r22, r22, LO  (/*BSP_LED_RED| BSP_LED_GREEN | */BSP_DUAL_LED_GREEN)

	lwz 	r20, 0(r23)
	xor	    r22, r20, r22
	stw	    r22, 0(r23)

	
	b	death_loop


	/* 7th STEP: the SDRAM check is	passed.	*/
	
memCheckPassed:
	lis	    r22, HIADJ (BSP_LED_ALL)		/* dark	all... */
	addi	r22, r22, LO (BSP_LED_ALL) 
	stw     r22,0(r23)
	
	/* 8th STEP: Turn off data and instruction caches */

	/************************************************/
	/*	end   check	the	SDRAM		*/
	/************************************************/

        /* initialize the stack pointer */

	lis	sp, HIADJ(STACK_ADRS)
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

        lis     r7, HIADJ(romInit)
        addi    r7, r7, LO(romInit)
 
        lis     r8, HIADJ(ROM_TEXT_ADRS)
        addi    r8, r8, LO(ROM_TEXT_ADRS)
 
        lis	r6, HIADJ(romStart)	
        addi	r6, r6, LO(romStart)	/* load R6 with C entry point */

	sub	r6, r6, r7		/* routine - entry point */
	add	r6, r6, r8 		/* + ROM base */

	mtspr	LR, r6			/* save destination address*/
					/* into LR register */
	blr				/* jump to the C entry point */


