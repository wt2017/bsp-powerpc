/* lld.h - Source Code for Spansion's Low Level Driver */
/* v7.2.0 */
/**************************************************************************
* Copyright (C)2007 Spansion LLC. All Rights Reserved. 
*
* This software is owned and published by: 
* Spansion LLC, 915 DeGuigne Dr. Sunnyvale, CA  94088-3453 ("Spansion").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND 
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software constitutes driver source code for use in programming Spansion's 
* Flash memory components. This software is licensed by Spansion to be adapted only 
* for use in systems utilizing Spansion's Flash memories. Spansion is not be 
* responsible for misuse or illegal use of this software for devices not 
* supported herein.  Spansion is providing this source code "AS IS" and will 
* not be responsible for issues arising from incorrect user implementation 
* of the source code herein.  
*
* SPANSION MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE, 
* REGARDING THE SOFTWARE, ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED 
* USE, INCLUDING, WITHOUT LIMITATION, NO IMPLIED WARRANTY OF MERCHANTABILITY, 
* FITNESS FOR A  PARTICULAR PURPOSE OR USE, OR NONINFRINGEMENT.  SPANSION WILL 
* HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT, NEGLIGENCE OR 
* OTHERWISE) FOR ANY DAMAGES ARISING FROM USE OR INABILITY TO USE THE SOFTWARE, 
* INCLUDING, WITHOUT LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, 
* SPECIAL, OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA, SAVINGS OR PROFITS, 
* EVEN IF SPANSION HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.  
*
* This software may be replicated in part or whole for the licensed use, 
* with the restriction that this Copyright notice must be included with 
* this software, whether used in part or whole, at all times.  
*/


#ifndef __INC_lldh
#define __INC_lldh

/* NOTICE
MirrorBit flash devices requires 4us from the time    
a programming command is issued before the data polling 
bits can be read.  Without the delay, it is likely
that you will read invalid status from the flash.
The invalid status may lead the software to believe
that programming finished early without problems or 
that programming failed.  If your system has more 
than 4us of delay inherently, you don't need any 
additional delay.  Otherwise, change the #undef to 
a #define
WAIT_4us_FOR_DATA_POLLING_BITS_TO_BECOME_ACTIVE
in lld_target_specific.h.  Make sure your optimization does not          
remove the delay loop.  You must replace DELAY_4us    
with a value which makes sense for your system.       

It is possible to suspend the erase operation 
with such frequency that it is unable to complete 
the erase operation and eventually times-out.                                            
Change the #undef to #define PAUSE_BETWEEN_ERASE_SUSPENDS 
in lld_target_specific.h if you are using erase suspend and the 
following is true.
Time between suspends is less that 10 milliseconds    
AND total number of suspends per erase can exceed 5000.         
Make sure that your optimization does not remove the  
delay loop.  You must replace DELAY_10ms with a value 
which make sense in your system.

For more information, visit our web site at www.spansion.com,
email us at spansion.solutions@spansion.com.
*/

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

#define SA_OFFSET_MASK	0xFFFFF000	// mask off the offset

/* LLD Command Definition */
#define NOR_AUTOSELECT_CMD               ((0x90)*LLD_DEV_MULTIPLIER)
#define NOR_CFI_QUERY_CMD                ((0x98)*LLD_DEV_MULTIPLIER)
#define NOR_CHIP_ERASE_CMD               ((0x10)*LLD_DEV_MULTIPLIER)
#define NOR_ERASE_SETUP_CMD              ((0x80)*LLD_DEV_MULTIPLIER)
#define NOR_PROGRAM_CMD                  ((0xA0)*LLD_DEV_MULTIPLIER)
#define NOR_RESET_CMD                    ((0xF0)*LLD_DEV_MULTIPLIER)
#define NOR_SECSI_SECTOR_ENTRY_CMD       ((0x88)*LLD_DEV_MULTIPLIER)
#define NOR_SECSI_SECTOR_EXIT_SETUP_CMD  ((0x90)*LLD_DEV_MULTIPLIER)
#define NOR_SECSI_SECTOR_EXIT_CMD        ((0x00)*LLD_DEV_MULTIPLIER)
#define NOR_SECTOR_ERASE_CMD             ((0x30)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_ENTRY_CMD      ((0x20)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_PROGRAM_CMD    ((0xA0)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_RESET_CMD1     ((0x90)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_BYPASS_RESET_CMD2     ((0x00)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_DATA1                 ((0xAA)*LLD_DEV_MULTIPLIER)
#define NOR_UNLOCK_DATA2                 ((0x55)*LLD_DEV_MULTIPLIER)
#define NOR_WRITE_BUFFER_ABORT_RESET_CMD ((0xF0)*LLD_DEV_MULTIPLIER)
#define NOR_WRITE_BUFFER_LOAD_CMD        ((0x25)*LLD_DEV_MULTIPLIER)
#define NOR_WRITE_BUFFER_PGM_CONFIRM_CMD ((0x29)*LLD_DEV_MULTIPLIER) 
#define NOR_SUSPEND_CMD                  ((0xB0)*LLD_DEV_MULTIPLIER)
#define NOR_RESUME_CMD                   ((0x30)*LLD_DEV_MULTIPLIER)
#define NOR_SET_CONFIG_CMD			     ((0xD0)*LLD_DEV_MULTIPLIER)
#define NOR_READ_CONFIG_CMD			     ((0xC6)*LLD_DEV_MULTIPLIER)


/* LLD System Specific Typedefs */
typedef unsigned char  BYTE;         /* 8 bits wide */
typedef unsigned short LLD_UINT16;  /* 16 bits wide */
typedef unsigned long  LLD_UINT32;  /* 32 bits wide */
typedef LLD_UINT32     ADDRESS;     /* Used for system level addressing */
typedef unsigned int   WORDCOUNT;   /* used for multi-byte operations */

/*
#define UINT16 LLD_UINT16
#define UINT32 LLD_UINT32
*/

/* boolean macros */
#ifndef TRUE
 #define TRUE  (1)
#endif
#ifndef FALSE
 #define FALSE (0)
#endif

/* Data Bus Configurations */
#define A_MINUS_1        (0x0100)
#define MULTI_DEVICE_CFG (0x0200)

#define DEVICE_TYPE_MASK (0x00FF)
#define X8_DEVICE        (0x0001)
#define X16_DEVICE       (0x0002)
#define X8X16_DEVICE     (0x0004)
#define X16X32_DEVICE    (0x0008)
#define X32_DEVICE       (0x0010)

#define DATA_WIDTH_MASK  (0xFF00)
#define X8_DATA          (0x0100)
#define X16_DATA         (0x0200)
#define X32_DATA         (0x0400)
#define X64_DATA         (0x0800) /* RFU */

/* device/bus width configurations */
#define X8_AS_X8      (X8_DEVICE     | X8_DATA)
#define X8X16_AS_X8   (X8X16_DEVICE  | X8_DATA |    A_MINUS_1)

#define X8_AS_X16     (X8_DEVICE     | X16_DATA | MULTI_DEVICE_CFG)
#define X16_AS_X16    (X16_DEVICE    | X16_DATA)
#define X8X16_AS_X16  (X8X16_DEVICE  | X16_DATA)
#define X16X32_AS_X16 (X16X32_DEVICE | X16_DATA | A_MINUS_1)

#define X8_AS_X32     (X8_DEVICE     | X32_DATA | MULTI_DEVICE_CFG)
#define X16_AS_X32    (X16_DEVICE    | X32_DATA | MULTI_DEVICE_CFG)
#define X8X16_AS_X32  (X8X16_DEVICE  | X32_DATA | MULTI_DEVICE_CFG)
#define X16X32_AS_X32 (X16X32_DEVICE | X32_DATA) 
#define X32_AS_X32    (X32_DEVICE    | X32_DATA)

/* polling routine options */
typedef enum
{
LLD_P_POLL_NONE = 0,			/* pull program status */
LLD_P_POLL_PGM,				    /* pull program status */
LLD_P_POLL_WRT_BUF_PGM,			/* Poll write buffer   */
LLD_P_POLL_SEC_ERS,			    /* Poll sector erase   */
LLD_P_POLL_CHIP_ERS,			/* Poll chip erase     */
LLD_P_POLL_RESUME,
LLD_P_POLL_BLANK			    /* Poll device sector blank check */
}POLLING_TYPE;

typedef enum {
 DEV_STATUS_UNKNOWN = 0,
 DEV_NOT_BUSY,
 DEV_BUSY,
 DEV_EXCEEDED_TIME_LIMITS,
 DEV_SUSPEND,
 DEV_WRITE_BUFFER_ABORT,
 DEV_STATUS_GET_PROBLEM,
 DEV_VERIFY_ERROR,
 DEV_BYTES_PER_OP_WRONG,
 DEV_ERASE_ERROR,				
 DEV_PROGRAM_ERROR,				
 DEV_SECTOR_LOCK

} DEVSTATUS;

#include "lld_target_specific.h"

#ifdef STATUS_REG
// Status bit definition for status register 
#define DEV_RDY_MASK			(0x80*LLD_DEV_MULTIPLIER)	// Device Ready Bit
#define DEV_ERASE_SUSP_MASK		(0x40*LLD_DEV_MULTIPLIER)	// Erase Suspend Bit
#define DEV_ERASE_MASK			(0x20*LLD_DEV_MULTIPLIER)	// Erase Status Bit
#define DEV_PROGRAM_MASK		(0x10*LLD_DEV_MULTIPLIER)	// Program Status Bit
#define DEV_RFU_MASK			(0x08*LLD_DEV_MULTIPLIER)	// Reserved
#define DEV_PROGRAM_SUSP_MASK	(0x04*LLD_DEV_MULTIPLIER)	// Program Suspend Bit
#define DEV_SEC_LOCK_MASK		(0x02*LLD_DEV_MULTIPLIER)	// Sector lock Bit
#define DEV_BANK_MASK			(0x01*LLD_DEV_MULTIPLIER)	// Operation in current bank
#define NOR_STATUS_REG_READ_CMD			 ((0x70)*LLD_DEV_MULTIPLIER)
#define NOR_STATUS_REG_CLEAR_CMD		 ((0x71)*LLD_DEV_MULTIPLIER)
#endif

#if  LLD_CONFIGURATION == X16_AS_X16
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000002
typedef UINT16 FLASHDATA;

#elif  LLD_CONFIGURATION == X8X16_AS_X8
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x000000FF
#define LLD_DEV_READ_MASK  0x000000FF
#define LLD_UNLOCK_ADDR1   0x00000AAA
#define LLD_UNLOCK_ADDR2   0x00000555
#define LLD_BYTES_PER_OP   0x00000001
typedef BYTE FLASHDATA;

#elif  LLD_CONFIGURATION == X8X16_AS_X16
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000002
typedef UINT16 FLASHDATA;

#elif  LLD_CONFIGURATION == X16_AS_X32
#define LLD_DEV_MULTIPLIER 0x00010001
#define LLD_DB_READ_MASK   0xFFFFFFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000004
typedef UINT32 FLASHDATA;

#elif  LLD_CONFIGURATION == X8X16_AS_X32
#define LLD_DEV_MULTIPLIER 0x00010001
#define LLD_DB_READ_MASK   0xFFFFFFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000004
typedef UINT32 FLASHDATA;

#elif  LLD_CONFIGURATION == X8_AS_X8
#define LLD_DEV_MULTIPLIER 0x00000001
#define LLD_DB_READ_MASK   0x000000FF
#define LLD_DEV_READ_MASK  0x000000FF
#define LLD_UNLOCK_ADDR1   0x00000555
#define LLD_UNLOCK_ADDR2   0x000002AA
#define LLD_BYTES_PER_OP   0x00000001
typedef BYTE FLASHDATA;

#elif  LLD_CONFIGURATION == X8_AS_X16
#define LLD_DEV_MULTIPLIER 0x00000101
#define LLD_DB_READ_MASK   0x0000FFFF
#define LLD_DEV_READ_MASK  0x0000FFFF
#define LLD_UNLOCK_ADDR1   0x00000AAA
#define LLD_UNLOCK_ADDR2   0x00000554
#define LLD_BYTES_PER_OP   0x00000002
typedef UINT16 FLASHDATA;

#elif  LLD_CONFIGURATION == X8_AS_X32
#define LLD_DEV_MULTIPLIER 0x01010101
#define LLD_DB_READ_MASK   0xFFFFFFFF
#define LLD_DEV_READ_MASK  0xFFFFFFFF
#define LLD_UNLOCK_ADDR1   0x00000AAA
#define LLD_UNLOCK_ADDR2   0x00000554
#define LLD_BYTES_PER_OP   0x00000004
typedef UINT32 FLASHDATA;
#endif                   

/* public function prototypes */

/* Operation Functions */
#ifndef REMOVE_LLD_READ_OP
extern FLASHDATA lld_ReadOp
(
FLASHDATA * base_addr,   /* device base address is system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_WRITE_BUFFER_PROGRAMMING
extern DEVSTATUS lld_WriteBufferProgramOp
(
FLASHDATA * base_addr,   /* device base address is system */
ADDRESS offset,      /* address offset from base address */
WORDCOUNT word_cnt,  /* number of words to program */
FLASHDATA *data_buf  /* buffer containing data to program */
);
#endif

#ifndef REMOVE_LLD_PROGRAM_OP
extern DEVSTATUS lld_ProgramOp
(
FLASHDATA * base_addr,   /* device base address is system */
ADDRESS offset,      /* address offset from base address */
FLASHDATA write_data /* variable containing data to program */
);
#endif

#ifndef REMOVE_LLD_SECTOR_ERASE_OP
extern DEVSTATUS lld_SectorEraseOp
(
FLASHDATA * base_addr,   /* device base address is system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_CHIP_ERASE_OP
extern DEVSTATUS lld_ChipEraseOp
(
FLASHDATA * base_addr    /* device base address is system */
);
#endif

#ifdef WS_P_CR1
extern void lld_SetConfigRegCmd
(
  FLASHDATA *   base_addr,	/* device base address in system */
  FLASHDATA value,			/* Configuration Register 0 value*/
  FLASHDATA value1			/* Configuration Register 1 value*/
);

extern FLASHDATA lld_ReadConfigRegCmd
(
  FLASHDATA *   base_addr,	/* device base address in system */
  FLASHDATA offset			/* configuration reg. offset 0/1 */
);
#else
extern void lld_SetConfigRegCmd
(
  FLASHDATA *   base_addr,	/* device base address in system */
  FLASHDATA value			/* Configuration Register 0 value*/
);

extern FLASHDATA lld_ReadConfigRegCmd
(
  FLASHDATA *   base_addr	/* device base address in system */
);
#endif

/* Command Functions */
extern void lld_ResetCmd
(
FLASHDATA * base_addr   /* device base address in system */
);

#ifndef REMOVE_LLD_SECTOR_ERASE_CMD
extern void lld_SectorEraseCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_CHIP_ERASE_CMD
extern void lld_ChipEraseCmd
(
FLASHDATA * base_addr    /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_PROGRAM_CMD
extern void lld_ProgramCmd
(
FLASHDATA * base_addr,               /* device base address in system */
ADDRESS offset,                  /* address offset from base address */
FLASHDATA *pgm_data_ptr          /* variable containing data to program */
);
#endif

#ifndef REMOVE_LLD_UNLOCK_BYPASS_ENTRY_CMD
extern void lld_UnlockBypassEntryCmd
(
FLASHDATA * base_addr    /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_UNLOCK_BYPASS_PROGRAM_CMD
extern void lld_UnlockBypassProgramCmd
(
FLASHDATA * base_addr,               /* device base address in system */
ADDRESS offset,                  /* address offset from base address */
FLASHDATA *pgm_data_ptr          /* variable containing data to program */
);
#endif

#ifndef REMOVE_LLD_UNLOCK_BYPASS_RESET_CMD
extern void lld_UnlockBypassResetCmd
(
FLASHDATA * base_addr   /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_AUTOSELECT_ENTRY_CMD
extern void lld_AutoselectEntryCmd
(
FLASHDATA * base_addr   /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_AUTOSELECT_EXIT_CMD
extern void lld_AutoselectExitCmd
(
FLASHDATA * base_addr   /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_SECSI_SECTOR_ENTRY_CMD
extern void lld_SecSiSectorEntryCmd
(
FLASHDATA * base_addr    /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_SECTI_SECTOR_EXIT_CMD
extern void lld_SecSiSectorExitCmd
(
FLASHDATA * base_addr   /* device base address in system */
);
#endif

#ifndef REMOVE_WRITE_BUFFER_PROGRAMMING
extern void lld_WriteToBufferCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_WRITE_BUFFER_PROGRAMMING
extern void lld_ProgramBufferToFlashCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_WRITE_BUFFER_PROGRAMMING
extern void lld_WriteBufferAbortResetCmd
(
FLASHDATA * base_addr    /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_PROGRAM_SUSPEND_CMD
extern void lld_ProgramSuspendCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_ERASE_SUSPEND_CMD
extern void lld_EraseSuspendCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_ERASE_RESUME_CMD
extern void lld_EraseResumeCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_PROGRAM_RESUME_CMD
extern void lld_ProgramResumeCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_CFI_ENTRY_CMD
extern void lld_CfiEntryCmd
(
FLASHDATA * base_addr    /* device base address in system */
);
#endif

#ifndef REMOVE_LLD_CFI_EXIT_CMD
extern void lld_CfiExitCmd
(
FLASHDATA * base_addr    /* device base address in system */
);
#endif

/* Utility Functions */
#ifndef REMOVE_LLD_STATUS_GET
extern DEVSTATUS lld_StatusGet
(
FLASHDATA * base_addr,      /* device base address in system */
ADDRESS 	 offset    ,	  /* address offset from base address */
int    is_writeBufferProgramming		/*is is operation in writeBufferProgram status*/
);
#endif

#ifndef REMOVE_LLD_POLL
extern DEVSTATUS lld_Poll
(
FLASHDATA * base_addr,          /* device base address in system */
ADDRESS offset,             /* address offset from base address */
FLASHDATA *exp_data_ptr,    /* expect data */
FLASHDATA *act_data_ptr,    /* actual data */
POLLING_TYPE polling_type   /* type of polling to perform */
);
#endif

#ifndef REMOVE_LLD_GET_DEVICE_ID
extern unsigned int lld_GetDeviceId
(
FLASHDATA * base_addr    /* device base address is system */
);
#endif

#ifndef REMOVE_WRITE_BUFFER_PROGRAMMING
DEVSTATUS lld_memcpy
(
FLASHDATA * base_addr,   /* device base address is system */
ADDRESS offset,      /* address offset from base address */
WORDCOUNT words_cnt, /* number of words to program */
FLASHDATA *data_buf  /* buffer containing data to program */
);
#endif

#ifndef REMOVE_LLD_READ_CFI_WORD
FLASHDATA lld_ReadCfiWord
(
FLASHDATA * base_addr,   /* device base address is system */
ADDRESS offset       /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_STATUS_REG_READ_CMD
extern void wlld_StatusRegReadCmd
(
FLASHDATA * base_addr,    /* device base address in system */
ADDRESS offset           /* address offset from base address */
);
#endif

#ifndef REMOVE_LLD_STATUS_REG_CLEAR_CMD
extern void wlld_StatusRegClearCmd
(
FLASHDATA * base_addr,   /* device base address in system */
ADDRESS offset           /* sector address offset from base address */
);
#endif

/* WARNING - Make sure the macro DELAY_1Us (lld_target_specific.h) */
/* is defined appropriately for your system !!                     */
/* If you decide to use your own delay functions, change the       */
/* macros DELAY_MS and DELAY_US in lld_target_specific.h.          */
#ifndef REMOVE_DELAY_MILLISECONDS
extern void DelayMilliseconds
 (
 int milliseconds
 );
#endif

#ifndef REMOVE_DELAY_MICROSECONDS
extern void DelayMicroseconds
 (
 int microseconds
 );
#endif

/*****************************************************************************/
#ifdef TRACE
extern void FlashWrite(FLASHDATA * addr, ADDRESS offset, FLASHDATA data);
extern FLASHDATA FlashRead(FLASHDATA * addr, ADDRESS offset);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_lldh */


