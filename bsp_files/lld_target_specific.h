#ifndef LLD_TARGET_SPECIFIC_H
#define LLD_TARGET_SPECIFIC_H

/* lld_target_specific.h - Source Code for Spansion's Low Level Driver */
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
********************************************************************************/

/**************************************************** 
*		Define how to access your hardware.         *
* Describe to the LLD the flash chip configuration  *
* un-comment the one that matches your system       *
****************************************************/
#define LLD_CONFIGURATION X16_AS_X16		/* no-interleaving, a single x16 device in x16 mode  */
// #define LLD_CONFIGURATION X8X16_AS_X16 */  /* no-interleaving, a single x8/x16 device in x16 mode */
// #define LLD_CONFIGURATION X8X16_AS_X8    /* no-interleaving, a single x8/x16 device in x8 mode */
// #define LLD_CONFIGURATION X16_AS_X32     /* two x16 devices interleaved to form x32 */                  
// #define LLD_CONFIGURATION X8X16_AS_X32   /* two x8/x16 devices interleaved to form x32 */ 
// #define LLD_CONFIGURATION X8_AS_X8       /* no-interleaving, a single x8 device in x8 mode  */
// #define LLD_CONFIGURATION X8_AS_X32      /* special case when four X8X16 devices in X8 mode interleaving to form X32 */
// #define LLD_CONFIGURATION X8_AS_X16      /* special case when two X8X16 devices in X8 mode interleaving to form X16 */    

/******************************************************
* Store and display every I/O operation to the flash  *
******************************************************/
/*#define TRACE    */

/*************************************************************
* Define WS-P_CR1 to access the CR1(Configuration Register 1) *
* for WS-P type device. Confiugration Register 1 bit 0 is     *
* used with configuration register 0 bit 11 - 13 to set the   *
* wait state to 8 and above for WS-P device. See WS-P         *
* datasheet for detail.                                       *
**************************************************************/
// #define WS_P_CR1  

/******************************************************
* Define STATUS_REG to enable the status read command   *
* when in embedded operation mode. This feature is only *
* available for device GL-R.                            *
* This also enable the 'EnableStatusCmd' which is used  *
* to switch to use read status command when polling the *
* operation status.	                                    *
********************************************************/
#define STATUS_REG

/*****************************************************
* Define Flash read/write macro to be used by LLD    *
*****************************************************/
#define FLASH_OFFSET(b,o)       (*(( (volatile FLASHDATA*)(b) ) + (o)))

#ifndef TRACE
  #ifndef  F_SIMULATOR
    #define FLASH_WR(b,o,d)     FLASH_OFFSET((b),(o)) = (d)
    #define FLASH_RD(b,o)       FLASH_OFFSET((b),(o))
  #else	
    #define FLASH_WR(b,o,d)		pFS_Pcb->FS_PCB_FLASH_WR((UINT32) ((volatile FLASHDATA *)(b) + o),d,"FLASH_WR")
    #define FLASH_RD(b,o)		pFS_Pcb->FS_PCB_FLASH_RD((UINT32) ((volatile FLASHDATA *)(b) + o) )
  #endif	
#else
#define FLASH_WR(b,o,d)         FlashWrite( b,o,d )
#define FLASH_RD(b,o)           FlashRead(b,o)
#endif 

/*******************************************************
* Define flash simulator R/W debug macro (no trace)    *
*******************************************************/
#ifdef F_SIMULATOR
#define DEBUG_FLASH_WR(b,d)		pFS_Pcb->FS_PCB_FLASH_WR((UINT32)(b),d,"FLASH_WR")
#define DEBUG_FLASH_RD(b)		pFS_Pcb->FS_PCB_FLASH_RD((UINT32)(b))
#endif

/************************************************************************** 
* Remove the following 65nm related functions                             *
**************************************************************************/
#ifndef STATUS_REG
#define REMOVE_LLD_STATUS_REG_READ_CMD	/* remove status register read command */
#define REMOVE_LLD_STATUS_REG_CLEAR_CMD /* remove status register clear command */
#define REMOVE_LLD_STATUS_READ_CMD      /* remove status read cmd when polling */
#endif
#define REMOVE_LLD_BLANK_CHECK_CMD		/* remove blank check cmd */
#define REMOVE_LLD_BLANK_CHECK_OP		/* remove blank check operation */
#define REMOVE_ASP_SECTOR_UNLOCK		/* sector alllock and unlock command */
#define REMOVE_ASP_SECTOR_LOCK_RANGE    /* sector loack range command */
 
/* If you are worried about code size, you can      */
/* remove an LLD function by un-commenting the      */
/* appropriate macro below.                         */
/*                                                  */
/*  #define REMOVE_CHECK_WRITE_BUFFER_STATUS        */
/*  #define REMOVE_CHECK_TIMEOUT_STATUS             */
/*  #define REMOVE_CHECK_TOGGLE_BIT_STATUS          */
/*  #define REMOVE_ACTIVE_MASK                      */
/*  #define REMOVE_LLD_SECTOR_ERASE_CMD             */
/*  #define REMOVE_LLD_CHIP_ERASE_CMD               */
/*  #define REMOVE_LLD_PROGRAM_CMD                  */
/*  #define REMOVE_LLD_UNLOCK_BYPASS_ENTRY_CMD      */
/*  #define REMOVE_LLD_UNLOCK_BYPASS_PROGRAM_CMD    */
/*  #define REMOVE_LLD_UNLOCK_BYPASS_RESET_CMD      */
/*  #define REMOVE_LLD_AUTOSELECT_ENTRY_CMD         */
/*  #define REMOVE_LLD_AUTOSELECT_EXIT_CMD          */
/*  #define REMOVE_LLD_SECSI_SECTOR_ENTRY_CMD       */
/*  #define REMOVE_LLD_SECTI_SECTOR_EXIT_CMD        */
/*  #define REMOVE_WRITE_BUFFER_PROGRAMMING         */
/*  #define REMOVE_LLD_PROGRAM_SUSPEND_CMD          */
/*  #define REMOVE_LLD_ERASE_SUSPEND_CMD            */
/*  #define REMOVE_LLD_ERASE_RESUME_CMD             */
/*  #define REMOVE_LLD_PROGRAM_RESUME_CMD           */
/*  #define REMOVE_LLD_CFI_ENTRY_CMD                */
/*  #define REMOVE_LLD_CFI_EXIT_CMD                 */
/*  #define REMOVE_LLD_POLL                         */
/*  #define REMOVE_LLD_STATUS_GET                   */
/*  #define REMOVE_LLD_PROGRAM_OP                   */
/*  #define REMOVE_LLD_SECTOR_ERASE_OP              */
/*  #define REMOVE_LLD_CHIP_ERASE_OP                */
/*  #define REMOVE_DELAY_MILLISECONDS               */
/*  #define REMOVE_DELAY_MICROSECONDS               */
/*  #define REMOVE_LLD_READ_OP                      */
/*  #define REMOVE_LLD_GET_DEVICE_ID                */
/*  #define REMOVE_LLD_READ_CFI_WORD                */
/*  #define REMOVE_LLD_PROGRAM_CMD					*/
/*  #define REMOVE_LLD_UNLOCK_BYPASS_ENTRY_CMD		*/
/*  #define REMOVE_LLD_UNLOCK_BYPASS_PROGRAM_CMD    */
/*  #define REMOVE_LLD_UNLOCK_BYPASS_RESET_CMD      */
/*  #define REMOVE_LLD_PROGRAM_OP					*/
/*  #define PROGRAM_SUSPEND_RESUME					*/
/*  #define REMOVE_ASP_PASSWORD						*/
/*  #define REMOVE_ASP_PPB							*/
/*  #define REMOVE_ASP_PPB_LOCK_BIT					*/
/*  #define REMOVE_ASP_DYB							*/
/*  #define REMOVE_LLD_AUTOSELECT_ENTRY_CMD			*/
/*  #define REMOVE_LLD_AUTOSELECT_EXIT_CMD			*/

/************************************************************
* If your part has advanced sector protection, uncomment     *
* the following macro.                                       *
*************************************************************/
#define INCLUDE_ADVANCED_SECTOR_PROTECTION                 

/****************************************************
* Define device specific buffer size in words here  *
****************************************************/
#define LLD_BUFFER_SIZE    16

/*******************************************************
* Some of the functions require minimal delays.        *
* You need to provide info relative to your system     *
*******************************************************/
#ifndef REMOVE_DELAY_MILLISECONDS
#define DELAY_MS(milliseconds) DelayMilliseconds(milliseconds)
#endif
#ifndef REMOVE_DELAY_MICROSECONDS
#define DELAY_US(microseconds) DelayMicroseconds(microseconds)
#endif
/* Tell the LLD how many for loops of i=i it takes to burn    */
/* up 1 microsecond of time.                                  */
#define DELAY_1us 150

/************************************************************************
* determines whether or not your system supports timestamp in the trace *
* can also be used to turn off printing of the timestamp in the trace   *
************************************************************************/
/* #define PRINT_TIMESTAMP	*/

/*************************************************************************
* Displays file name and line number information when an LLD_ASSERT call *
* fails.  This information is displayed in LLDCmdlineAssert().           *
*************************************************************************/
/* #define ASSERT_DIAGNOSTICS */

/*************************************************************************
* enables code to execute commands from a file instead of directly from *
* the command line. 
**************************************************************************/
/* #define ENABLE_SCRIPTING_MACRO */

// **************************************************
// Enable NST (NOR Supper Tests) 
//****************************************************
//#define NST_TESTS

/******************** NOTICE*******************************
First generation MirrorBit flash devices (M and A series)
requires 4us from the time    
a programming command is issued before the data polling 
bits can be read.  Without the delay, it is likely
that you will read invalid status from the flash.
The invalid status may lead the software to believe
that programming finished early without problems or 
that programming failed.  If your system has more 
than 4us of delay inherently, you don't need any 
additional delay.  Otherwise, change the #undef to 
a #define
WAIT_4us_FOR_DATA_POLLING_BITS_TO_BECOME_ACTIVE.
Make sure your optimization does not          
remove the delay loop.  You must replace DELAY_4us    
with a value which makes sense for your system.       

It is possible to suspend the erase operation 
with such frequency that it is unable to complete 
the erase operation and eventually times-out.                                            
Change the #undef to #define PAUSE_BETWEEN_ERASE_SUSPENDS.
if you are using erase suspend and the 
following is true.
Time between suspends is less that 10 milliseconds    
AND total number of suspends per erase can exceed 5000.         
Make sure that your optimization does not remove the  
delay loop.  You must replace DELAY_10ms with a value 
which make sense in your system.

If you are using First generation MirrorBit devices (M and A series), 
change the macro #undef FIRST_MIRRORBIT_DEVICE 
to #define FIRST_MIRRORBIT_DEVICE.

For more information, visit our web site at www.spansion.com,
email us at spansion.solutions@spansion.com.
************************************************************/
#define FIRST_MIRRORBIT_DEVICE

#ifdef FIRST_MIRRORBIT_DEVICE
#define WAIT_4us_FOR_DATA_POLLING_BITS_TO_BECOME_ACTIVE
#undef  PAUSE_BETWEEN_ERASE_SUSPENDS
#endif


/***********************************************************
* Define external IO routine for Spansion Flash Simulator. *
***********************************************************/
#ifdef  F_SIMULATOR	
class FS_Pcb;
extern FS_Pcb * pFS_Pcb;

class Pcb;
extern Pcb* _pcb;
#endif

#endif
