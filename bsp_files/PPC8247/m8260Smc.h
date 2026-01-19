/* m8260Smc.h - Motorola MPC8260 Serial Management Controller header file */

/*
modification history
--------------------
2005-06-21 ,bjm  set M8260_SMC_BASE as 0x3000+0x400
01a,12sep99,ms_	 created from m8260Cpm.h, 01d.
01a,15sep2001,modified from m8260Scc.h
*/

/*
 * This file contains constants for the Serial Management Controllers
 * (SMC2) in the Motorola MPC8260 PowerQUICC II integrated Communications 
 * Processor
 */

#ifndef __INCm8260Smch
#define __INCm8260Smch

#include "drv/sio/m8260Brg.h"
#include "drv/sio/m8260Sio.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef M8260ABBREVIATIONS
#define M8260ABBREVIATIONS
   
#ifdef  _ASMLANGUAGE
#define CAST(x)
#else /* _ASMLANGUAGE */
typedef volatile UCHAR  VCHAR;    /* shorthand for volatile UCHAR */
typedef volatile INT32  VINT32;   /* volatile unsigned word */
typedef volatile INT16  VINT16;   /* volatile unsigned halfword */
typedef volatile INT8   VINT8;     /* volatile unsigned byte */
typedef volatile UINT32 VUINT32; /* volatile unsigned word */
typedef volatile UINT16 VUINT16; /* volatile unsigned halfword */
typedef volatile UINT8  VUINT8;   /* volatile unsigned byte */
#define CAST(x) (x)
#endif  /* _ASMLANGUAGE */

#endif /* M8260ABBREVIATIONS */


/* CPM mux SMC clock route register,added by bjm */
#define M8260_CMXSMR(base)   (CAST(VUINT8 *)((base) + 0x11B0C))

/*CPM 波特率发生器寄存器 8 个*/
#define M8260_BRGC1_ADRS 	0x000119F0

#define M8260_BRGC5_ADRS 	0x000115F0


/*
 * MPC8260 internal register/memory map (section 17 of prelim. spec)
 * note that these are offsets from the value stored in the IMMR
 * register. Also note that in the MPC8260, the IMMR is not a special
 * purpose register, but it is memory mapped.
 */
 
/* SMC1 Parameter Ram */
#define M8260_SMC_BASE_REG		(vxImmrGet()+ 0x87FC)  /* SMC's DPRAM pointer */
#define M8260_BRGCx_OFFSET(num)	((num< 5)?(0x119F0+(num-1)*4):(0x115F0+(num-5)*4))

#define SMC1_BD_OFF 			(SMC_PARAM_OFF+0x40)

#define SMC1_RX_BUF_OFF 		(SMC_PARAM_OFF+0x50)  /* SMC1 RX BD descriptor offset addr in dpram */
#define SMC1_TX_BUF_OFF 		(SMC_PARAM_OFF+0x60)  /* SMC1 tX BD descriptor offset addr in dpram */	
	
#define SMC2_PARAM_OFF			(SMC_PARAM_OFF+ 0x80)  /* SMC2 parameter RAM offset addr in dpram */
#define SMC2_BD_OFF 			(SMC_PARAM_OFF+ 0xc0)  /* SMC2 RX BD descriptor offset addr in dpram */
#define SMC2_RX_BUF_OFF 		(SMC_PARAM_OFF+ 0xd0)  /* SMC2 RX BD descriptor offset addr in dpram */
#define SMC2_TX_BUF_OFF 		(SMC_PARAM_OFF+ 0xe0)  /* SMC2 tX BD descriptor offset addr in dpram */


#define M8260_SMC_PRAM_RBASE	  0x00000000  /* Rx Buff Descr Base */
#define M8260_SMC_PRAM_TBASE	  0x00000002  /* Tx Buff Descr Base */
#define M8260_SMC_PRAM_RFCR	  0x00000004  /* Rx Function Code */
#define M8260_SMC_PRAM_TFCR	  0x00000005  /* Tx Function Code */
#define M8260_SMC_PRAM_MRBLR	  0x00000006  /* Max Rcv Buff Length */
#define M8260_SMC_PRAM_MAXIDL	  0x00000028
#define M8260_SMC_PRAM_BRKLN      0x0000002c
#define M8260_SMC_PRAM_BRKCR      0x00000030
#define M8260_SMC_UART_PRAM_BRKEC 0x0000002e  /*error count*/


/*smc define start*/
#define M8260_SMC_BD_SIZE 	    8	/* size, in bytes, of a single BD */

#define M8260_SMC_RCV_BD_OFF	0	/* offset from BD base to receive BD */
#define M8260_SMC_TX_BD_OFF	    M8260_SMC_BD_SIZE	
/* offset from BD base to transmit BD, since there is just one BD each */
#define M8260_SMC_BD_STAT_OFF 	0	/* offset to status field */
#define M8260_SMC_BD_LEN_OFF	      2	/* offset to data length field */
#define M8260_SMC_BD_ADDR_OFF	4	/* offset to address pointer field */

/* SMC - Serial Management Controller */

#define M8260_SMCMR		0x00011A82
#define M8260_SMCE			0x00011A86
#define M8260_SMCM			0x00011A8A
#define M8260_SMC_OFFSET_NEXT_SMC  0x10


/* SMC Mode Register - Configure as UART Mode */

#define M8260_SMCMR_UART	0x0020	/* UART mode */

/* enable bits for transmit and receive */

#define M8260_SMCMR_ENT	0x0002	/* enable transmitter */
#define M8260_SMCMR_ENR	0x0001	/* enable receiver */

/* local loopback mode */

#define M8260_SMCMR_LOOPBACK	0x0004	/* local loopback mode */

/* automatic echo mode */

#define M8260_SMCMR_ECHO	0x0008	/* echo mode */

/* PARITY mode */

#define M8260_SMCMR_NO_PARITY	0x0000	/* parity disabled */
#define M8260_SMCMR_PARITY	0x0200	/* parity enabled */
#define M8260_SMCMR_PARITY_ODD	0x0000	/* odd parity if enabled */
#define M8260_SMCMR_PARITY_EVEN 0x0100	/* even parity if enabled */
#define M8260_SMCMR_STOPLEN_1	0x0000	/* stop length = 1 */
#define M8260_SMCMR_STOPLEN_2	0x0400	/* stop length = 2 */
#define M8260_SMCMR_NORM  	0x0000	/* normal operation mode */
#define M8260_SMCMR_CLEN_MASK	0x7800	/* # bits in char minus one */
					/* set to # data bits + # parity bits + # stop bits */
#define M8260_SMCMR_CLEN_STD	0x4800	/* for 1+8+0+1 std serial I/O */

/* SMC UART Event and Mask Register definitions */

#define M8260_SMCE_UART_BRK	0x10	/* break char received */
#define M8260_SMCE_UART_BSY	0x04	/* char discarded no bufs */
#define M8260_SMCE_UART_TX	0x02	/* char transmitted */
#define M8260_SMCE_UART_RX	0x01	/* char received  */

#define M8260_SMCM_UART_BRK	0x10	/* break char received */
#define M8260_SMCM_UART_BSY	0x04	/* char discarded no bufs */
#define M8260_SMCM_UART_TX	0x02	/* char transmitted */
#define M8260_SMCM_UART_RX	0x01	/* char received  */

/* SMC UART Receive Buffer Descriptor definitions */

#define M8260_SMC_UART_RX_BD_EMPTY	0x8000	/* buffer is empty */
#define M8260_SMC_UART_RX_BD_WRAP	0x2000	/* last BD in chain */
#define M8260_SMC_UART_RX_BD_INT	0x1000	/* set interrupt when filled */
#define M8260_SMC_UART_RX_BD_CM	    0x0200	/* Continuous Mode bit */
#define M8260_SMC_UART_RX_BD_ID	    0x0100	/* Close on IDLE recv bit */
#define M8260_SMC_UART_RX_BD_BR	    0x0020	/* Close on break recv bit */
#define M8260_SMC_UART_RX_BD_FR	    0x0010	/* Close on frame error bit */
#define M8260_SMC_UART_RX_BD_PR	    0x0008	/* Parity error in last byte */
#define M8260_SMC_UART_RX_BD_OV	    0x0002	/* receiver Overrun occurred */

/* SMC UART Transmit Buffer Descriptor definitions */

#define M8260_SMC_UART_TX_BD_READY	  0x8000	/* Transmit ready/busy bit */
#define M8260_SMC_UART_TX_BD_WRAP	  0x2000	/* last BD in chain */
#define M8260_SMC_UART_TX_BD_INT	  0x1000	/* set interrupt when emptied */
#define M8260_SMC_UART_TX_BD_CM	      0x0200	/* Continuous Mode bit */
#define M8260_SMC_UART_TX_BD_PREAMBLE 0x0100	/* send preamble sequence */

/* Miscellaneous SMC UART definitions */

#define M8260_SMC_POLL_OUT_DELAY	100	/* polled mode delay */
#define M8260_CPM_CLOCK			166000000
#define M8260_SCCR_DFBRG		0

typedef M8260_SCC_CHAN  M8260_SMC_CHAN ;

#ifdef __cplusplus
}
#endif

#endif /* __INCm8260Smch */
