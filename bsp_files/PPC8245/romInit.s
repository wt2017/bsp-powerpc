/* romInit.s - general PPC 603/604 ROM initialization module
 * Copyright 1984-1998 Wind River Systems, Inc.
 * Copyright 1996-1998 Motorola, Inc.


modification history
--------------------
01a,02Feb99, My  Copied from Yellowknife platform and deleted unused code.


DESCRIPTION
This module contains the entry code for VxWorks images that start
running from ROM, such as 'bootrom' and 'vxWorks_rom'.
The entry point, romInit(), is the first code executed on power-up.
It performs the minimal setup needed to call
the generic C routine romStart() with parameter BOOT_COLD.

RomInit() typically masks interrupts in the processor, sets the initial
stack pointer (to STACK_ADRS which is defined in configAll.h), and
readies system memory by configuring the DRAM controller if necessary.
Other hardware and device initialization is performed later in the
BSP's sysHwInit() routine.

A second entry point in romInit.s is called romInitWarm(). It is called
by sysToMonitor() in sysLib.c to perform a warm boot.
The warm-start entry point must be written to allow a parameter on
the stack to be passed to romStart().

*/

#define	_ASMLANGUAGE
#include "vxWorks.h"
#include "sysLib.h"
#include "asm.h"
#include "config.h"
#include "regs.h"
#include "ons8245.h"
#include "bspcommon.h"
#include "proj_config.h"

/* defines */

/*
 * Some releases of h/arch/ppc/toolPpc.h had a bad definition of
 * LOADPTR. So we will define it correctly. [REMOVE WHEN NO LONGER NEEDED].
 *
 * LOADPTR initializes a register with a 32 bit constant, presumably the
 * address of something.
 */
#undef LOADPTR
#define	LOADPTR(reg,const32) \
	  addis reg,r0,HIADJ(const32); addi reg,reg,LO(const32)


#define ORCR32(reg,val) \
	 lis	   r3,0x8000;\
	 ori	   r3,r3,reg;\
	 stwbrx    r3,0,r4;\
	 sync             ;\
	 lwbrx     r6,0,r5;\
	 lis	   r3,HI(val);\
	 ori	   r3,r3,LO(val);\
	 or	   r3,r3,r6;\
	 stwbrx    r3,0,r5;\
	 sync

#define  OUTCR32(reg,val)\
         lis     r3,0x8000;\
         ori     r3,r3,LO(reg);\
         stwbrx  r3,0,r4;\
	 sync             ;\
         lis     r3,HI(val);\
         ori     r3,r3,LO(val);\
         stwbrx  r3,0,r5;\
	 sync

#define  OUTCR16(reg,val)\
         lis     r3,0x8000;\
         ori     r3,r3,LO(reg);\
         rlwinm  r6,r3,0,30,31;\
         rlwinm  r3,r3,0,0,29;\
         stwbrx  r3,r0,r4;\
	 sync            ;\
         li      r3,LO(val);\
         sthbrx  r3,r6,r5;\
	 sync

#define  OUTCR8(reg,val)\
         lis     r3,0x8000;\
         ori     r3,r3,LO(reg);\
         rlwinm  r6,r3,0,30,31;\
         rlwinm  r3,r3,0,0,29;\
         stwbrx  r3,r0,r4;\
	 sync            ;\
         li      r3,LO(val);\
         stbx    r3,r6,r5;\
	 sync	  

 
	/* Exported internal functions */

	.data
	.globl	_romInit	/* start of system code */
	.globl	romInit		/* start of system code */
	.globl	_romInitWarm	/* start of system code */
	.globl	romInitWarm	/* start of system code */

	.globl	SEROUT		/* useful serial output routine */

	/* externals */

	.extern romStart	/* system initialization routine */

	.text

#if 1   //only use it In tornado 2.2
	.ascii	 "8245 BIOS\0"              /* Added by baijm for identifying BIOS */
    .fill   (0x100-10),1,0              /* Fill with (256-10) zeros */

	.align 4
#endif

/******************************************************************************
*
* romInit - entry point for VxWorks in ROM
*

* romInit
*     (
*     int startType	/@ only used by 2nd entry point @/
*     )

*/

_romInit:
romInit:

	/* This is the cold boot entry (ROM_TEXT_ADRS) */

	bl	cold

	/*
	 * This warm boot entry point is defined as ROM_WARM_ADRS in config.h.
	 * It is defined as an offset from ROM_TEXT_ADRS.  Please make sure
	 * that the offset from _romInit to romInitWarm matches that specified
	 * in config.h
	 */

_romInitWarm:
romInitWarm:
	bl	warm

	/* copyright notice appears at beginning of ROM (in TEXT segment) */

	.ascii	BSP_COPYRIGHT
	.align 4

cold:
	li	r11, BOOT_COLD
	bl	start		/* skip over next instruction */

warm:
	or	r11, r3, r3	/* startType to r11 */

start:
	/* Zero-out registers */

	addis	r0,r0,0
	mtspr	SPRG0,r0
	mtspr	SPRG1,r0
	mtspr	SPRG2,r0
	mtspr	SPRG3,r0


	/* initialize the stack pointer */

	LOADPTR (sp, STACK_ADRS)

	/* Set MPU/MSR to a known state. Turn on FP */

	LOADPTR (r3, _PPC_MSR_FP)
	sync
	mtmsr	r3
	isync
       
	
	/* Init the floating point control/status register */

	mtfsfi	7,0x0
	mtfsfi	6,0x0
	mtfsfi	5,0x0
	mtfsfi	4,0x0
	mtfsfi	3,0x0
	mtfsfi	2,0x0
	mtfsfi	1,0x0
	mtfsfi	0,0x0
	isync

	/* Turn off data and instruction cache control bits */
	mfspr	r3, HID0
	isync
	rlwinm	r4, r3, 0, 18, 15	/* r4 has ICE and DCE bits cleared */
	sync
	isync
	mtspr	HID0, r4		/* HID0 = r4 */
	isync

	/* Get cpu type */
	mfspr	r28, PVR
	rlwinm	r28, r28, 16, 16, 31


	/* invalidate the MPU's data/instruction caches */
	lis	r3, 0x0
	cmpli	0, 0, r28, CPU_TYPE_603
	beq	cpuIs603
	cmpli	0, 0, r28, CPU_TYPE_603E
	beq	cpuIs603

cpuIs603:
	ori	r3, r3, (_PPC_HID0_ICE | _PPC_HID0_DCE)
	or	r4, r4, r3		/* set bits */
	sync
	isync
	mtspr	HID0, r4		/* HID0 = r4 */
	andc	r4, r4, r3		/* clear bits */
	isync
	mtspr	HID0, r4
	isync

#ifdef USER_I_CACHE_ENABLE
	b	instCacheOn603
#else
	b	cacheEnableDone
#endif

	/* turn the Instruction cache ON for faster FLASH ROM boots */

#ifdef USER_I_CACHE_ENABLE

	ori	r4, r4, (_PPC_HID0_ICE | _PPC_HID0_ICFI)
	isync				/* Synchronize for ICE enable */
	b	writeReg4
instCacheOn603:
	ori	r4, r4, (_PPC_HID0_ICE | _PPC_HID0_ICFI)
	rlwinm	r3, r4, 0, 21, 19	/* clear the ICFI bit */

	/*
         * The setting of the instruction cache enable (ICE) bit must be
         * preceded by an isync instruction to prevent the cache from being
         * enabled or disabled while an instruction access is in progress.
         */
	isync
writeReg4:
	mtspr	HID0, r4		/* Enable Instr Cache & Inval cache */
	mtspr	HID0, r3		/* using 2 consec instructions */
					/* PPC603 recommendation */				
#endif
cacheEnableDone:

        /* 
         * OUTCrx macros used below require 
         * r4 & r5 are already loaded with the values below 
         *
         * r4 = 0xfec00000 
         * r5 = 0xfee00000
         * r3,r6:temp register
         */
        lis  r4,0xfec0
        lis  r5,0xfee0

REG_INIT_START:

        OUTCR16(0x04,0x0006)     /* PCI command */
        OUTCR16(0x06,0xffff)     /* PCI status,clear all bit by write 1*/
        OUTCR16(0x0C,0xf800)     /* Latency timer = 0xf8*/

        OUTCR32(0xa8,0x00141b58)   /* PCIR1 **/ 
        OUTCR32(0xac,0x00000000)   /* PCIR2 **/ 
	
        #define MC_BSTOPRE	                0x0		

	/*------- MCCR1 ------- */

        #define MC1_ROMNAL			15		/* 31-28 **/
        #define MC1_ROMFAL			11		/* 27-23 **/
        #define MC1_DBUS_SIZE01		        3		/* 22-21,SDRAM 64bit,FLASH 8bit **/
        #define MC1_BURST			0		/* 20 **/
        #define MC1_MEMGO			0		/* 19 **/
        #define MC1_SREN			0		/* 18 **/
        #define MC1_SDRAM			0		/* 17 **/
        #define MC1_PCKEN			0		/* 16 **/
        #define MC1_BANKBITS			0x0002		/* 15-0 **/ /* 13*9*4 */

	OUTCR32(0xf0, \
		MC1_ROMNAL << 28 | MC1_ROMFAL << 23 | \
		MC1_DBUS_SIZE01 << 21 | MC1_BURST << 20 | \
		MC1_MEMGO << 19 | MC1_SREN << 18 | \
		MC1_SDRAM << 17 | MC1_PCKEN << 16 | \
		MC1_BANKBITS)

	/*------- MCCR2 ------- */

        #define MC2_TS_WAIT_TIMER		0		/* 31-29 **/
        #define MC2_ASRISE			0		/* 28-25 **/
        #define MC2_ASFALL			0		/* 24-21 **/
        #define MC2_INLINE_PAR_NOT_ECC		0		/* 20 **/
        #define MC2_INLINE_WR_EN		0		/* 19 **/
        #define MC2_INLINE_RD_EN		0		/* 18 **/
                                                                /* 17-16 **/  
        #define MC2_REFINT			0x1D0		/* 15-2 **/
        #define MC2_RSV_PG			0		/* 1 **/
        #define MC2_RMW_PAR			0		/* 0 **/

	OUTCR32(0xf4, MC2_TS_WAIT_TIMER << 29 | MC2_ASRISE << 25 | \
		MC2_ASFALL << 21 | MC2_INLINE_PAR_NOT_ECC << 20 | \
		MC2_INLINE_WR_EN << 19 | MC2_INLINE_RD_EN << 18 | \
		MC2_REFINT << 2 | MC2_RSV_PG << 1 | MC2_RMW_PAR)

	/*------- MCCR3 ------- */
       
        #define MC3_BSTOPRE2_5			(MC_BSTOPRE >> 4 & 0xf) /* 31-28 **/
        #define MC3_REFREC			 4		        /* 27-24 */ 
                                                        	        /* 23-0,RESERVED **/
	OUTCR32(0xf8, MC3_BSTOPRE2_5 << 28 | MC3_REFREC << 24 )

	/*------- MCCR4 ------- */

        #define MC4_PRETOACT		3			/* 31-28 **/
        #define MC4_ACTOPRE		5			/* 27-24 */
        #define MC4_WMODE		0			/* 23 **/
        #define MC4_BUF_TYPE0		0		        /* 22 **/
        #define MC4_EXTROM		1			/* 21 **/
        #define MC4_BUF_TYPE1		1	                /* 20 **/
        #define MC4_BSTOPRE0_1		(MC_BSTOPRE >> 8 & 3)   /* 19-18 **/
        #define MC4_DBUS_SIZE2		1                       /* 17 **/
                                                                /* 16,RESERVED **/
        #define MC4_REGDIMM		0			/* 15 **/
        #define MC4_SDMODE_CAS		3			/* 14-12 **/
        #define MC4_SDMODE_WRAP		0			/* 11 **/
        #define MC4_SDMODE_BURST	2			/* 10-8 **/
        #define MC4_ACTORW		3			/* 7-4 **/
        #define MC4_BSTOPRE6_9		(MC_BSTOPRE & 0xf)

        OUTCR32(0xfc, MC4_PRETOACT << 28 | MC4_ACTOPRE << 24 | \
		MC4_WMODE << 23 | MC4_BUF_TYPE0 << 22 | \
		MC4_EXTROM <<21 | MC4_BUF_TYPE1 << 20 | MC4_BSTOPRE0_1 << 18 | \
		MC4_DBUS_SIZE2 << 17 | MC4_REGDIMM << 15 | MC4_SDMODE_CAS << 12 | \
		MC4_SDMODE_WRAP << 11 | MC4_SDMODE_BURST << 8 | \
		MC4_ACTORW << 4 | MC4_BSTOPRE6_9)

        OUTCR8(0xe0,0xC0)  /* AMBOR bit5 =0,bit5=1;bit5=0 **/ 
        OUTCR8(0xe0,0xE0)  /* 0xFD00_0000(local bus)-->0x0000_0000(PCI bus)*/
        OUTCR8(0xe0,0xC0)  /* 8245 as PCI target,0x000A_0000(PCI bus)-->system memory*/

        OUTCR8(0xe1,0x0)  /* for debug,max performance */
        OUTCR8(0xe2,0x0)
        OUTCR8(0xe3,0x80)
             
        OUTCR32(0x80,0x80808000)  /* START MEMADD1 **/
        OUTCR32(0x84,0x80808080)  /* START MEMADD2 **/
        OUTCR32(0x88,0x00000000)  /* EXT START MEM1 **/
        OUTCR32(0x8c,0x00000000)  /* EXT START MEM2 **/

        OUTCR32(0x90,0xffffff7f)  /* END MEMADD1,128M BYTE SDRAM **/
        OUTCR32(0x94,0x80808080)  /* END MEMADD2 **/
        OUTCR32(0x98,0x00000000)  /* EXT END MEM1 **/
        OUTCR32(0x9c,0x00000000)  /* EXT END MEM2 **/

	/*------- Extended ROM Configuration Register 1 ------- */
	
        #define RCS2_EN		        1		       /* 31 **/
        #define RCS2_BURST	        0		       /* 30 **/
        #define RCS2_DBW		2   /*32bit*/	       /* 29-28 **/
        #define RCS2_CTL		0   /*independent*/    /* 27-26 **/
                                                               /* 25 RESERVED**/ 
        #define RCS2_ROMFAL	        18    /*300ns*/	       /* 24-20 **/
        #define RCS2_ROMNAL		18		       /* 19-15 **/
        #define RCS2_ASFALL	        0		       /* 14-10 **/
        #define RCS2_ASRISE	        0		       /*  9-5 **/
        #define RCS2_TS_WAIT_TIMER	0		       /*  4-0 **/

        OUTCR32(0xd0, RCS2_EN << 31 | RCS2_BURST << 30 | RCS2_DBW << 28 | \
                      RCS2_CTL << 26 | RCS2_ROMFAL <<20 | RCS2_ROMNAL << 15 | \
                      RCS2_ASFALL << 10 | RCS2_ASRISE << 5 | RCS2_TS_WAIT_TIMER )

	/*------- Extended ROM Configuration Register 2 ------- */
	
        #define RCS3_EN		        1  		       /* 31 **/
        #define RCS3_BURST	        0		       /* 30 **/
        #define RCS3_DBW		2   /*32bit*/	       /* 29-28 **/

        #define RCS3_CTL		2   /*handshaking*/    /* 27-26 **/
        #define RCS3_ASFALL	        3/*0*/		       /* 14-10 **/
        #define RCS3_ASRISE	        7/*0*/		       /*  9-5 **/

        #define RCS3_ROMFAL	        24    /*380ns*/	       /* 24-20 **/
        #define RCS3_ROMNAL		24		       /* 19-15 **/
        #define RCS3_TS_WAIT_TIMER	0		       /*  4-0 **/

        OUTCR32(0xd4, RCS3_EN << 31 | RCS3_BURST << 30 | RCS3_DBW << 28 | \
                      RCS3_CTL << 26 | RCS3_ROMFAL <<20 | RCS3_ROMNAL << 15 | \
                      RCS3_ASFALL << 10 | RCS3_ASRISE << 5 | RCS3_TS_WAIT_TIMER )

        OUTCR32(0xd8,0x0400000d)  /* Extended ROM Configuration Register 3 */
                                  /* m_cs2----start:0x74000000,size:32M */
        OUTCR32(0xdc,0x0c00000d)  /* Extended ROM Configuration Register 4 */
                                  /* m_cs3----start:0x7c000000,size:32M */

        OUTCR16(0x70,0x0000)      /* PMCR1,test clock disable*/
        OUTCR16(0x72,0xd524)      /* PMCR2:0x24;ODCR:0xd5 **/ 
        OUTCR16(0x74,0xfc00)      /* PCI_SYNC_OUT(no),PCI_CLK0(no),SDRAM_CLK0*(yes) **/ 
        OUTCR8(0x77,0x10)         /* SDRAM data in sample clock delay **/ 

        OUTCR8(0xa0,0x0001)       /* MEMBNKEN (BANK 0,only 1 128Meg Bank) **/
        OUTCR8(0xa3,0x32)         /* MEMPMODE,active to precharge **/

        OUTCR32(0x78,EUMB_BASE_ADRS)       /* EUMBBAR **/
       
	/* Wait before initializing other registers */
	lis	r3, 0x0001
	mtctr	r3
wait1:
	bdnz	wait1

	/*------- MCCR1 ------- */
        ORCR32(0xf0,0x00080000)   /* MEMGO enable */

	/* Wait again */
        LOADPTR (r3, 0x0002ffff)
	mtctr	r3
wait2:
	bdnz	wait2

	sync
	eieio

#if 1 /*serial port input start*/
	/* Display a message indicating PROM has started */
        #define PUTC(ch)	li r3, ch; bl SEROUT

	PUTC(13)
	PUTC(10)
	PUTC(10)
	PUTC('B')
	PUTC('o')
	PUTC('o')
	PUTC('t')
	PUTC('i')
	PUTC('n')
	PUTC('g')
	PUTC('.')
	PUTC('.')
	PUTC('.')
	PUTC(13)
	PUTC(10)
	PUTC(10)
#endif /*serial port input end*/




    /* first STEP:	disable the external dog by config epld address 
      assert epld address line A22 is to config the epld dog , epld-A22--connect with CPU A24,so address =0x74000000+4*0x400000
      BIT27 write 0 ,disable the dog, BIT27 write 1 enable the dog
      0x74000000 is the epld clear dog register, BIT27 write 1,then write to 0 ----the way to clear the dog;
    */

    /*enable the epld dog*/
   	lis 	r21, HIADJ (BSP_DOG_EN_REG)	
	addi	r21, r21, LO (BSP_DOG_EN_REG)

#if 1
    /*enable the epld dog*/
	lis 	r22, HIADJ (BSP_DOG_ENABLE)	
	addi	r22, r22, LO (BSP_DOG_ENABLE)

	lwz	    r20, 0(r21)
	or	    r20, r20, r22
	stw	    r20, 0(r21)
#else
    /*disable the epld dog*/
	lis 	r22, HIADJ (~BSP_DOG_ENABLE)	
	addi	r22, r22, LO (~BSP_DOG_ENABLE)

	lwz	    r20, 0(r21)
	and	    r20, r20, r22
	stw	    r20, 0(r21)
#endif

    /* 0x74000000 is the epld clear dog reg , BIT27 write 1 then write to 0 ----is the way to clear the dog;*/
   	lis 	r21, HIADJ (BSP_DOG_CLR_REG)	
	addi	r21, r21, LO (BSP_DOG_CLR_REG)

    /*clear dog in epld*/
	lis	    r22, HIADJ(BSP_DOG_CLR)	/*write 1 to BIT27*/
	addi	r22, r22, LO(BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	or 	    r20, r20, r22
	stw	    r20, 0(r21)    

	lis	    r22, HIADJ(~BSP_DOG_CLR)	/*write 0 to BIT27*/
	addi	r22, r22, LO(~BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	and 	r20, r20, r22
	stw	    r20, 0(r21)    




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
	 * r23,	light regester address in epld
	 * r21,	clear dog regester address in epld
	 */
check_sdram:
	/* first, prepare to check the sdram. */
	lis		r15, HIADJ (0x5a5a5a5a)
	addi	r15, r15, LO (0x5a5a5a5a)
	lis		r16, HIADJ (0xa5a5a5a5)
	addi	r16, r16, LO (0xa5a5a5a5)
	lis		r17, HIADJ (0x5aa55aa5)
	addi	r17, r17, LO (0x5aa55aa5)
	lis		r18, HIADJ (0xa55aa55a)
	addi	r18, r18, LO (0xa55aa55a)

    /* r23,	light regester address in epld*/
   	lis 	r23, HIADJ (BSP_LIGHT_REG)	
	addi	r23, r23, LO (BSP_LIGHT_REG)
	
	lis		r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET)	/* set the start address. */
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)

	lis		r7,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET + MEM_CHECK_SIZE)	/* set the end address.	*/
	addi	r7,	r7,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET + MEM_CHECK_SIZE)      

	lis		r13, HIADJ (0x40000)		/* set the time	counter.*/
	addi	r13, r13, LO (0x40000)
	xor		r12, r12, r12

	lis	    r22, HIADJ (BSP_LED_RED | BSP_LED_GREEN| BSP_DUAL_LED_GREEN)		/* light the low dual light yellow*/
	addi	r22, r22, LO (BSP_LED_RED | BSP_LED_GREEN| BSP_DUAL_LED_GREEN)
	stw     r22,0(r23)

	/* second, Write the memory	*/
		
memWrite1:

	addi	r12, r12, 1			/* counting	the	time. */
	cmpw	r12, r13
	bne 	clrWatchDog_11

    xor		r12, r12, r12		/* clear the counter */

    /* flash red light. other light unchange*/
	lis 	r22, HIADJ (BSP_LED_RED)		
	addi	r22, r22, LO (BSP_LED_RED)

	lwz 	r20, 0(r23)
	xor 	r20, r20, r22
	stw	    r20, 0(r23)

    /*clear dog in epld*/
	lis	    r22, HIADJ(BSP_DOG_CLR)	/*write 1 to BIT27*/
	addi	r22, r22, LO(BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	or 	    r20, r20, r22
	stw	    r20, 0(r21)    

	lis	    r22, HIADJ(~BSP_DOG_CLR)	/*write 0 to BIT27*/
	addi	r22, r22, LO(~BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	and 	r20, r20, r22
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
	lis	r6,	HIADJ (LOCAL_MEM_LOCAL_ADRS	+ MEM_CHECK_OFFSET)	/* set the start address again.	*/
	addi	r6,	r6,	LO (LOCAL_MEM_LOCAL_ADRS + MEM_CHECK_OFFSET)

	lis	    r22, HIADJ ( BSP_LED_GREEN)		/* light the red light*/
	addi	r22, r22, LO ( BSP_LED_GREEN)
	stw     r22,0(r23)


	lis		r13, HIADJ (0x40000)		/* set the time	counter.*/
	addi	r13, r13, LO (0x40000)
	
	xor	r12, r12, r12			/* time	counter	clear. */

	/* forth, check	the	memory's context is	all	right or not. */
	
memCheck1:

	addi	r12, r12, 1
	cmpw	r12, r13
	bne	    clrWatchDog_12

	xor	    r12, r12, r12			/* clear the counter */

    /* flash green light. other light unchange*/
	lis	    r22, HIADJ (BSP_DUAL_LED_GREEN)        
	addi	r22, r22, LO (BSP_DUAL_LED_GREEN)

	lwz 	r20, 0(r23)
	xor	    r20, r20, r22
	stw	    r20, 0(r23)


    /*clear dog in epld*/
	lis	    r22, HIADJ(BSP_DOG_CLR)	/*write 1 to BIT27*/
	addi	r22, r22, LO(BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	or 	    r20, r20, r22
	stw	    r20, 0(r21)    

	lis	    r22, HIADJ(~BSP_DOG_CLR)	/*write 0 to BIT27*/
	addi	r22, r22, LO(~BSP_DOG_CLR)

	lwz	    r20, 0(r21)
	and 	r20, r20, r22
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

	lis	    r22, HIADJ (BSP_DUAL_LED_GREEN)		/* dark	all... */
	addi	r22, r22, LO (BSP_DUAL_LED_GREEN) 
	stw     r22,0(r23)

death_loop:
	xor 	r20, r20, r20			/* relax 1 second... */
	lis 	r21, HIADJ (0x20000)
	addi	r21, r21, LO(0x20000)
	
ddelay_deatchloop:
    addi	r20, r20, LO(1)
    cmpw	r20, r21
    bne 	ddelay_deatchloop

    lis 	r22, HIADJ (BSP_LED_ALL)		/* flash all...	*/
    addi	r22, r22, LO (BSP_LED_ALL)

	lwz 	r20, 0(r23)
	xor	    r22, r20, r22
	stw	    r22, 0(r23)

    b	    death_loop


	/* 7th STEP: the SDRAM check is	passed.	*/
	
memCheckPassed:
	lis	    r22, HIADJ (BSP_LED_RED|BSP_LED_GREEN)		/* dark	all... */
	addi	r22, r22, LO (BSP_LED_RED|BSP_LED_GREEN) 
	stw     r22,0(r23)


	/************************************************/
	/*	end   check	the	SDRAM		*/
	/************************************************/    

#ifndef ALPHA_VERSION
	/* go to C entry point */
	or	    r3, r11, r11		/* put startType in r3 (p0) */
	addi	sp, sp, -FRAMEBASESZ	/* save one frame stack */

	LOADPTR (r6, romStart)
	LOADPTR (r7, romInit)
	LOADPTR (r8, ROM_TEXT_ADRS)

	sub	r6, r6, r7
	add	r6, r6, r8

	mtlr	r6		/* romStart - romInit + ROM_TEXT_ADRS */
	blr
#else
	b check_sdram
#endif
/**********************************************************************
 *  void SEROUT(int char);
 *
 *  Configure COM1 to 19200 baud and write a character.
 *  Should be usable basically at any time.
 *
 *  r4,r5 = reserved for PCI configuration space access
 *  r3 = character to output
 *  r7,r8 = temp vars
 */

SEROUT:
	/* r7 = intLock() */
	mfmsr	r7
	rlwinm	r8,r7,0,17,15
	mtmsr	r8
	isync

	/* r8 = serial port address */
	LOADPTR(r8, EUMB_BASE_ADRS | 0x4500) /*UART1/UART2 offset:0x4500/0x4600*/

	/* ULCR,Init serial port,set ULCR[DLAB]=1 */
	li	r6, 0x83
	stb	r6, 3(r8)	
	/* UDLB&UDMB,Set baud rate to 19200(SDRAM_CLK=66.666666M) for MPC8245 */
	li	r6, 0xd9
	stb	r6, 0(r8)
	li	r6, 0x00
	stb	r6, 1(r8)
	
	/* ULCR,set ULCR[DLAB]=0 */	
	li	r6, 0x03
	stb	r6, 3(r8)
	/* UMCR,set */
	li	r6, 0x02
	stb	r6, 4(r8)

	/* Wait for transmit buffer available */
sowait:
	lbz	r6, 5(r8) /*ULSR*/
	andi.	r0, r6, 0x40
	bc	12, 2, sowait

	/* Transmit byte */
	stb	r3, 0(r8)

	/* intUnlock(r7) */
	rlwinm	r7,r7,0,16,16
	mfmsr	r8
	or	r7,r7,r8
	mtmsr	r7
	isync

	blr
	
		
