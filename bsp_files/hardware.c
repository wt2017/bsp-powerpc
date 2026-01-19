/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2007
* 
* ispVME Embedded allows programming of Lattice's suite of FPGA
* devices on embedded systems through the JTAG port.  The software
* is distributed in source code form and is open to re - distribution
* and modification where applicable.
*
*
***************************************************************/
/**************************************************************
* 
* Revision History:
* 
*
*
***************************************************************/
#include "proj_config.h"


/***************************************************************
*
* Holds the current byte when writing to the device. Each
* bit represents TDI, TDO, TMS, TCK, etc, respectively.
*
***************************************************************/

unsigned int g_siIspPins = 0x0000;

/***************************************************************
*
* Stores the current input port address.
*
***************************************************************/

unsigned int g_usInPort = CPLD_JTAG_IN_ADDR;

/***************************************************************
*
* Stores the current output port address.
*
***************************************************************/

unsigned int g_usOutPort = CPLD_JTAG_OUT_ADDR;

/***************************************************************
*
* This is the definition of the bit locations of each respective
* signal in the global variable g_siIspPins.
*
* NOTE: users must add their own implementation here to define
*       the bit location of the signal to target their hardware.
*       The example below is for the Lattice download cable on
*       on the parallel port.
*
***************************************************************/

unsigned int    g_ucPinTDI		= Cpld_TDI;    	/* Bit address of TDI signal */
unsigned int 	g_ucPinTCK		= Cpld_TCK;   	 /* Bit address of TCK signal */
unsigned int	g_ucPinTMS		= Cpld_TMS;  	  /* Bit address of TMS signal */
unsigned int	g_ucPinENABLE	= Cpld_PIN_ENABLE;    /* Bit address of chip enable */
unsigned int 	g_ucPinTRST		= Cpld_RST;  	  /* Bit address of TRST signal */
unsigned int 	g_ucPinCE		= Cpld_PINCE;    /* Bit address of buffer chip enable */
unsigned int 	g_ucPinTDO		= Cpld_TDO;  	  /* Bit address of TDO signal */
//#define ISP_PORT_GROUP	(g_ucPinTDI | g_ucPinTCK | g_ucPinTMS | g_ucPinTRST | g_ucPinTDO)
#define ISP_PORT_GROUP	(g_ucPinTDI | g_ucPinTCK | g_ucPinTMS | g_ucPinTRST)

/***************************************************************
*
* External variables and functions.
*
***************************************************************/

extern unsigned short g_usDelayPercent;
void sysMsDelay  ( UINT   delay    /* length of time in MS to delay */) ;
STATUS taskDelay (int ticks /* number of ticks to delay task */  ) ;

/***************************************************************
*
* writePort
*
* To apply the specified value to the pins indicated. This routine will
* likely be modified for specific systems. As an example, this code
* is for the PC, as described below.
*
* This routine uses the IBM-PC standard Parallel port, along with the
* schematic shown in Lattice documentation, to apply the signals to the
* programming loop. 
*
* PC Parallel port pin    Signal name   Port bit address
*  --------------------    -----------   ------------------
*        2                   g_ucPinTDI        1
*        3                   g_ucPinTCK        2
*        4                   g_ucPinTMS        4
*        5                   g_ucPinENABLE     8
*        6                   g_ucPinTRST       16
*        7                   g_ucPinCE	       32
*        10                  g_ucPinTDO        64
*                     
*  Parameters:
*   - a_ucPins, which is actually a set of bit flags (defined above)
*     that correspond to the bits of the data port. Each of the I/O port
*     bits that drives an isp programming pin is assigned a flag 
*     (through a #define) corresponding to the signal it drives. To 
*     change the value of more than one pin at once, the flags are added 
*     together, much like file access flags are.
*
*     The bit flags are only set if the pin is to be changed. Bits that 
*     do not have their flags set do not have their levels changed. The 
*     state of the port is always manintained in the static global 
*     variable g_siIspPins, so that each pin can be addressed individually 
*     without disturbing the others.
*
*   - a_ucValue, which is either HIGH (0x01 ) or LOW (0x00 ). Only these two
*     values are valid. Any non-zero number sets the pin(s) high.
*
***************************************************************/

void writePort( unsigned int a_ucPins, unsigned int a_ucValue )
{
	unsigned int tmp ;
	if ( a_ucValue ) {
		g_siIspPins = a_ucPins | g_siIspPins;
	}
	else {
		g_siIspPins = ~a_ucPins & g_siIspPins;
	}

	/***************************************************************
	*
	* NOTE: users must add their own implementation here to write
	*       to the device.
	*
	***************************************************************/
	tmp =(*((unsigned int *)g_usOutPort)) ; 
	(*((unsigned int *)g_usOutPort)) = (tmp & (~ISP_PORT_GROUP)) | (g_siIspPins & ISP_PORT_GROUP) ;
}

/***************************************************************
*
* readPort
*
* Returns the value of the TDO from the device.
*
***************************************************************/

unsigned char readPort()
{
	unsigned char ucRet = 0;

	/***************************************************************
	*
	* NOTE: users must add their own implementation here to read the
	*       TDO signal from the device.
	*
	***************************************************************/
/* This is a smaple code for Windows/DOS*/
	if ( (*((unsigned int *)g_usInPort)) & g_ucPinTDO ) {
		ucRet = 0x01;
	}
	else {
		ucRet = 0x00;
	}

	return ( ucRet );
} 

/***************************************************************
*
* ispVMDelay
*
*
* The user must implement a delay to observe a_usMicroSecondDelay, where
* a_usMicroSecondDelay is the number of micro - seconds that must pass before
* data is read from in_port.  Since platforms and processor speeds
* vary greatly, this task is left to the user.
* This subroutine is called upon to provide a delay from 1 millisecond
* to a few hundreds milliseconds each time. That is the
* reason behind using unsigned long integer in this subroutine.
* It is OK to provide longer delay than required. It is not
* acceptable if the delay is shorter than required. 
*
* Note: user must re - implement to target specific hardware.
*
* Example: Use the for loop to create the micro-second delay.
*  
*          Let the CPU clock (system clock) be F Mhz. 
*                                   
*          Let the for loop represented by the 2 lines of machine code:
*                    LOOP:  DEC RA;
*                           JNZ LOOP;
*          Let the for loop number for one micro-second be L.
*          Lets assume 4 system clocks for each line of machine code.
*          Then 1 us = 1/F (micro-seconds per clock) 
*                       x (2 lines) x (4 clocks per line) x L
*                     = 8L/F          
*          Or L = F/8;
*        
*          Lets assume the CPU clock is set to 48MHZ. The C code then is:
*
*          unsigned long F = 48;  
*          unsigned long L = F/8;
*          unsigned long index;
*          if (L < 1) L = 1;
*          for (index=0; index < a_usMicroSecondDelay * L; index++);
*          return 0;
*           
*	
* The timing function used here is for PC only by hocking the clock chip. 
*
***************************************************************/

void ispVMDelay( unsigned short a_usMicroSecondDelay )
{
	unsigned short usiDelayTime = 0; /* Stores the delay time in milliseconds */
/* This is a sample code for Windows/DOS 
	DWORD dwCurrentTime = 0;
	DWORD dwStartTime = 0;
*/
	if ( a_usMicroSecondDelay & 0x8000 ) {

		/***************************************************************
		*
		* Since MSB is set, the delay time must be decoded to 
		* millisecond. The SVF2VME encodes the MSB to represent 
		* millisecond.
		*
		***************************************************************/

		a_usMicroSecondDelay &= ~0x8000;
		usiDelayTime = a_usMicroSecondDelay;
	}
	else {

		/***************************************************************
		*
		* Since MSB is not set, the delay time is given as microseconds.
		* Convert the microsecond delay to millisecond.
		*
		***************************************************************/

		usiDelayTime = a_usMicroSecondDelay / 1000;
		if ( usiDelayTime <= 0 ) {

			/***************************************************************
			*
			* Any delay less than or equal to 0 will automatically receive
			* one millisecond delay.
			*
			***************************************************************/

			usiDelayTime = 1;
		}
	}

	if ( g_usDelayPercent ) {

		/***************************************************************
		*
		* Increase the delay time by the specified percentage given at
		* the command line.
		*
		***************************************************************/

		usiDelayTime = usiDelayTime * ( unsigned short ) ( 1 + ( g_usDelayPercent * 0.01 ) );
	}

	/***************************************************************
	*
	* NOTE: users must add their own implementation here to make
	*       that usiDelayTime milliseconds are observed correctly.
	*
	***************************************************************/
	//taskDelay(usiDelayTime) ;
	sysMsDelay(usiDelayTime) ;
		
}
int ispVMEReset()
	{
	*(unsigned int*)g_usOutPort &= ~g_ucPinTRST ;
	taskDelay(1) ;
	*(unsigned int*)g_usOutPort |= g_ucPinTRST ;
	return 0 ;
	}

