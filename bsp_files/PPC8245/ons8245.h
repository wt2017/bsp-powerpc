/*
 * Mousse Board constants and defines
 *
 * Copyright 1998-2000 Wind River Systems, Inc.
 * Copyright 1998 Broadcom Corp.
 */

#ifndef MOUSSE_H
#define MOUSSE_H

#include "epic.h"
/* Timer constants */
#ifdef __cplusplus
extern "C" {
#endif

#undef  BUS_SPEED_100MHZ

#ifdef  BUS_SPEED_100MHZ
  #define DEC_CLOCK_FREQ        100000000        /* 100MHz SDRAM */	
#else  /* 66 MHZ */
  #define DEC_CLOCK_FREQ         66666000        /* 66MHz SDRAM */	
#endif
/* Only one can be selected, FULL overrides BASIC */

#ifdef INCLUDE_MMU_FULL
#   undef INCLUDE_MMU_BASIC
#endif

/*
 * If INCLUDE_ECC is defined, ECC is enabled in the boot ROM.  Since
 * ECC increases RDLAT by 1, there is a performance penalty for ECC
 * that varies by application.
 */

/*
 * Miscellaneous definitions go here.
 * For example, macro definitions for various devices.
 */

#define CPU_TYPE		((vxPvrGet() >> 16) & 0xffff)
#define CPU_TYPE_601		0x01		/* PPC 601 CPU */
#define CPU_TYPE_602		0x02		/* PPC 602 CPU */
#define CPU_TYPE_603		0x03		/* PPC 603 CPU */
#define CPU_TYPE_603E		0x06		/* PPC 603e CPU */
#define CPU_TYPE_603P		0x07		/* PPC 603p CPU */
#define CPU_TYPE_604		0x04		/* PPC 604 CPU */
#define CPU_TYPE_604E		0x09		/* PPC 604e CPU */
#define CPU_TYPE_604R		0x0a		/* PPC 604r CPU */
#define CPU_TYPE_750		0x08		/* PPC 750  CPU */
#define CPU_TYPE_8240		0x81		/* PPC 8240 CPU */
#define CPU_TYPE_8245   0x8081    /* PPC 8245 CPU */
#define CPU_TYPE_7400   0x0C    /* PPC 7400 CPU */
#define CPU_TYPE_7410   0x800C  /* PPC 7410 CPU */
#define CPU_TYPE_7450   0x8000  /* PPC 7450 CPU */
#define CPU_TYPE_7455   0x8001  /* PPC 7455 CPU */
#define CPU_TYPE_745    0x3202  /* PPC 745 CPU */
#define CPU_TYPE_755    0x3200  /* PPC 755 CPU */
#define CPU_VER_745   0x3100  /* Processor verion for MPC745/755 */
#define CPU_VER_750     0x0202  /* Processor version for MPC750 */
#define CPU_VER_740     0x0200  /* Processor version for MPC740 */

/* System addresses */
#define PCI_SPECIAL_BASE	0xFE000000
#define PCI_SPECIAL_SIZE	0x01000000

#define EUMB_BASE_ADRS  	0xFC000000	/* Location of EUMB region */
#define EUMB_REG_SIZE		0x00100000	/* Size of EUMB region */

#define VEN_DEV_ID      0x00061057    	/* Vendor and Dev. ID for ppc8241 */
#define BMC_BASE	0x80000000      /* Configuration Register Base */

#define PCI_COMMAND	0x80000004	/* PCI command register */
#define PCI_STATUS	0x80000006	/* PCI status register */
#define LOC_MEM_BAR	0x80000010	/* Local mem base address reg (BAR) */
#define PCSRBAR		0x80000014	/* Peripheral Control & Status BAR */
#define PMCR1_ADR	0x80000070	/* Power Management Configuration Register1 */
#define PMCR2_ADR	0x80000072	/* Power Management Configuration Register2 */
#define MIOCR1_ADR	0x80000076	/* Miscellaneous I/O Control Register 1 */
#define MIOCR2_ADR	0x80000077	/* Miscellaneous I/O Control Register 2 */
#define EUMBBAR_REG	0x80000078	/* Embedded Util. Memory Block BAR */
#define MEM_START1_ADR	0x80000080    	/* Memory starting addr */
#define MEM_START2_ADR  0x80000084    	/* Memory starting addr-lower */
#define XMEM_START1_ADR 0x80000088   	/* Extended mem. starting addr-upper */
#define XMEM_START2_ADR 0x8000008c	/* Extended mem. starting addr-lower */
#define MEM_END1_ADR	0x80000090	/* Memory ending address */
#define MEM_END2_ADR	0x80000094	/* Memory ending address-lower */
#define XMEM_END1_ADR   0x80000098	/* Extended mem. ending address-upper*/
#define XMEM_END2_ADR   0x8000009c	/* Extended mem. ending address-lower*/
#define MEM_EN_ADR	0x800000a0	/* Memory bank enable */
#define PAGE_MODE	0x800000a3	/* Page Mode Counter/Timer */
#define PROC_INT1_ADR   0x800000a8	/* Processor interface config. 1 */
#define PROC_INT2_ADR   0x800000ac	/* Processor interface config. 2 */
#define SBE_CTR_ADR   	0x800000b8	/* Single-bit error counter */
#define SBE_TRIG_ADR  	0x800000b9	/* Single-bit error trigger thresh. */
#define MEM_ERREN1_ADR  0x800000c0	/* Memory error enable 1 */
#define MEM_ERRDET1_ADR 0x800000c1	/* Memory error detection 1 */
#define MEM_ERREN2_ADR  0x800000c4	/* Memory error enable 2 */
#define MEM_ERRDET2_ADR	0x800000c5	/* Memory error detection 2 */
#define MEM_PCIERRS_ADR	0x800000c7	/* PCI error status */
#define MEM_PCIERRA_ADR	0x800000c8	/* CPU/PCI error address */
#define MEM_CS2_CFG_ADR	0x800000D0	/* PCI error status */
#define MEM_CS3_CFG_ADR	0x800000D4	/* CPU/PCI error address */
#define AMBOR_ADRS	0x800000e0	/* Address Map B Options Register */
#define PCMBCR_ADRS	0x800000e1	/* PCI/Memory Buffer Configuration Register */
#define PCR_ADRS	0x800000e2	/* PLL Configuration Register */
#define SPE_BIT_ADRS	0x800000e3	/* Special bit setting Register */

#define MCCR1           0x800000f0	/* Memory control config. 1 */
#define MCCR2           0x800000f4	/* Memory control config. 2 */
#define MCCR3           0x800000f8	/* Memory control config. 3 */
#define MCCR4           0x800000fc	/* Memory control config. 4 */


#ifndef SYNC
# define SYNC  __asm__(" sync")
#endif  /* SYNC */ 

#define MPC8245_BUS_CLOCK	DEC_CLOCK_FREQ

/* PPC Decrementer - used as vxWorks system clock */
#define MPC8245_DELTA(a,b)              (abs((int)a - (int)b))

/* PortX Device Addresses for Mousse */
#define	PORTX_DEV_BASE		0xff000000
#define PORTX_DEV_SIZE		0x01000000

#define PLD_REG_BASE		(PORTX_DEV_BASE | 0xe09000)
#define PLD_REG(off)		(*(volatile UINT8 *) (PLD_REG_BASE + (off)))

#define PLD_REVID_B1		0x7f
#define PLD_REVID_B2		0x01

#define	SERIAL_BASE(_x)		(EUMB_BASE_ADRS | ((_x) ? 0x4600 : 0x4500))

#define N_SIO_CHANNELS		1	/* Note: sysSerial assumes 2 */

/* PCI ADDRESS SPACE */
#define CPU_PCI_MEM_ADRS	0x80000000  /* 32 bit prefetchable memory base addres of PCI mem */
#define CPU_PCI_MEM_SIZE	0x08000000  /* 32 bit prefetchable memory size  */
#define CPU_PCI_IO_ADRS	        0xFE000000  /* base addres of PCI IO */
#define CPU_PCI_IO_SIZE	        0x01000000  

/*
 * On-board PCI Ethernet
 */
#define	PCI_ENET_MEMADDR	0x80000000 
#define	PCI_ENET_IOADDR		0xFE000000

#if (_BYTE_ORDER == _BIG_ENDIAN)

#define PCISWAP(x)		LONGSWAP(x)
#define PCISWAP_SHORT(x)	(MSB(x) | LSB(x) << 8)
#define SROM_SHORT(pX)		(*(UINT8*)(pX) << 8 | *((UINT8*)(pX)+1))

#else

#define PCISWAP(x)		(x)			
#define	PCISWAP_SHORT(x)	(x)
#define SROM_SHORT(pX)		(*(UINT8*)(pX) | *((UINT8*)(pX)+1) << 8)

#endif /* _BYTE_ORDER == _BIG_ENDIAN */


/*
 * CHRP (MAP B) definitions.
 */
#define CHRP_REG_ADDR		0xfec00000	/* MPC106 Config, Map B */
#define CHRP_REG_DATA		0xfee00000	/* MPC106 Config, Map B */

/*
 * Mousse PCI IDSEL Assignments (Device Number)
 */
 
#define MOUSSE_IDSEL_LPCI	14		/* On-board PCI slot */
#define MOUSSE_IDSEL_APP550	17		/* APP550 NE*/
#define MOUSSE_IDSEL_ENET	23		/* On-board 82559 Ethernet,for ONS 8245 board */
#define MOUSSE_IDSEL_BCM5650	31		/* BCM5650 */

/*
 * Mousse Interrupt Mapping:
 *
 *	IRQ1	Enet (intA|intB|intC|intD)
 *	IRQ2	CPCI intA (See below)
 *	IRQ3	Local PCI slot intA|intB|intC|intD
 *	IRQ4	COM1 Serial port (Actually higher addressed port on duart)
 *
 * PCI Interrupt Mapping in Broadcom Test Chassis:
 *
 *                 |           CPCI Slot
 * 		   | 1 (CPU)	2	3	4       5       6
 *      -----------+--------+-------+-------+-------+-------+-------+
 *	  intA 	   |    X               X		X
 *	  intB     |            X               X		X
 *	  intC     |    X               X		X
 *	  intD	   |            X               X		X
 */

#define INT_VEC_IRQ0		0
#define INT_NUM_IRQ0		INT_VEC_IRQ0

#define MOUSSE_IRQ_ENET		EPIC_VECTOR_EXT1	/* Hardwired */
#define MOUSSE_IRQ_BCM5650	EPIC_VECTOR_EXT2	/* Hardwired */
#define MOUSSE_IRQ_LPCI		EPIC_VECTOR_EXT3	/* Hardwired */
#define MOUSSE_IRQ_APP550	EPIC_VECTOR_EXT4	/* Hardwired */

#ifndef _ASMLANGUAGE

/*
 * Mousse-related routines
 */

extern void	SEROUT(int ch);				/* sysALib.s */
extern UINT32	vxHid1Get(void);			/* sysALib.s */

extern void	sysSerialPutc(int c);			/* sysSerial.c */
extern int	sysSerialGetc(void);			/* sysSerial.c */
extern void	sysSerialPrintString(char *s);		/* sysSerial.c */
extern void	sysSerialPrintHex(UINT32 val, int cr);	/* sysSerial.c */

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
#endif /* MOUSSE_H */
