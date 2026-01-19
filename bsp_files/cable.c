#include <windows.h>
#include <conio.h>
#include "vmopcode.h"

/***************************************************************
*
* Global constants.
*
***************************************************************/

const unsigned char	g_ucOUT_SENSE_CABLE_OUT		= 64;		/* dO6 = pin 8  */
const unsigned char	g_ucIN_VCC_OK				= 8;		/* dI3 = pin 15 */
const unsigned char	g_ucIN_CABLE_V2_SENSE_IN	= 16;		/* dI4 = pin 13 */
const unsigned char	g_ucIN_CABLE_SENSE_IN		= 32;		/* dI5 = pin 12 */
const signed char g_cCABLE_NOT_DETECT			= -5;
const signed char g_cPOWER_OFF					= -6;

/***************************************************************
*
* Function prototypes.
*
***************************************************************/

int power_ok();
int port_ok();
int cable_port_check();
int get_possible_ports( int i );
int get_port_addresses(int lptnum, short int *in_address, short int *OUT_address);
int GetPortAddr(short int *inport, short int *outport);
int AnyCablesConnected();
int Check_Cable_Power();

/***************************************************************
*
* External variables and functions.
*
***************************************************************/

extern unsigned short g_usInPort;
extern unsigned short g_usOutPort;
extern void writePort(unsigned char pins, unsigned char value);
extern void ispVMDelay( unsigned short int a_usMicroSecondDelay );
extern unsigned int g_ucPinTDI;
extern unsigned int  g_ucPinTCK;
extern unsigned int  g_ucPinTMS;
extern unsigned int  g_ucPinENABLE;
extern unsigned int  g_ucPinTRST;
extern unsigned int  g_ucPinCE;

/***************************************************************
*
* Check_Cable_Power
*
* Check the cable power and connection.
*
***************************************************************/

int Check_Cable_Power()
{
	int iRetCode = 0;
	
	if ( g_usOutPort == 0x03BC ) {
		_outp( 0x03BE, 0x00 );
	}
	else if ( g_usOutPort == 0x0378 ) {
		_outp( 0x037A, 0x00 );
	}
	else if ( g_usOutPort == 0x0278 ) {
		_outp( 0x027A, 0x00 );
	}
	else {
		_outp( 0x037A, 0x00 );
	}

	if ( !AnyCablesConnected() ) {
		return( g_cCABLE_NOT_DETECT );
	}

	iRetCode = GetPortAddr( &g_usInPort, &g_usOutPort ); 
	if ( iRetCode == g_cPOWER_OFF ) {
		if ( g_usOutPort ==0 ){
			return( g_cCABLE_NOT_DETECT );
		}
		if ( !power_ok() ) {
			return( g_cPOWER_OFF );
		}
	}

	return(0);
}

/***************************************************************
*
* AnyCablesConnected
*
***************************************************************/

int AnyCablesConnected()
{ 
	int lptnum;
    short int inport, outport;
    int flag = FALSE;  
    inport = g_usInPort;
    outport = g_usOutPort;
    for (lptnum = 1; lptnum <= 3; lptnum++)
	{
		get_port_addresses(lptnum, &g_usInPort, &g_usOutPort);
		if (port_ok()){
			flag = TRUE;
		}
    }       
    g_usInPort = inport;
    g_usOutPort = outport;  
    return (flag);
}

/***************************************************************
*
* GetPortAddr
*
***************************************************************/

int GetPortAddr(short int *inport, short int *outport)
{ 
	int rcode, rcode2 = 0;
	int lptnum;    
    for (lptnum = 1; lptnum <= 3; lptnum++)
	{
		rcode = get_possible_ports(lptnum);
		if (rcode == 1){
			rcode2 = 0;
			break;
		}
		else if (rcode == 2)
			rcode2 = g_cPOWER_OFF;
    }  
	get_port_addresses(lptnum, inport, outport);
	return (rcode2);
}

/***************************************************************
*
* get_port_addresses
*
***************************************************************/

int get_port_addresses(int lptnum, short int *in_address, short int *OUT_address)
{
    switch (lptnum)
	{ 
		case 1:   /* LPT1 */
			*OUT_address = 0x0378;
			*in_address = 0x0379;
			break;
			
		case 2:   /* LPT2 */
			*OUT_address = 0x0278;
			*in_address = 0x0279;
			break;
			
		case 3:   /* LPT3 */
			*OUT_address = 0x03BC;
			*in_address = 0x03BD;
			break;
		default:   /* LPT1 */
			*OUT_address = 0;/* 0x0378; */
			*in_address = 1;/* 0x0379; */
			break;
    } 
	return (TRUE);  
}

/***************************************************************
*
* get_possible_ports
*
***************************************************************/

int get_possible_ports( int i )
{
    int valid_port = FALSE;
    BOOL next_tests = TRUE;

	get_port_addresses( i, &g_usInPort, &g_usOutPort );
	if ( g_usOutPort == 0x00 ) {
		next_tests = FALSE;  
		return ( 0 );
	}
	if ( next_tests ){
		get_port_addresses( i, &g_usInPort, &g_usOutPort );
	}
    return ( 1 );
}

/***************************************************************
*
* cable_port_check
*
* Checks to see if there is a Lattice download cable V2.0 connect
* to the parallel port.
*
***************************************************************/

int cable_port_check()
{
    unsigned int d1, d2;
    short int  portData = 0;
    int success = FALSE;     
	int old_cable = 0;
	if (g_usOutPort ==0x03BC)
		_outp(0x03BE, 0x00);
	else if (g_usOutPort == 0x0378) 
		_outp(0x037A, 0x00);
	else if (g_usOutPort == 0x0278) 
		_outp(0x027A, 0x00);
	else
		_outp(0x037A, 0x00);
	writePort(g_ucOUT_SENSE_CABLE_OUT, 0x00);
    ispVMDelay( 10000 );
    d1 =((g_ucIN_CABLE_V2_SENSE_IN + g_ucIN_CABLE_SENSE_IN) & _inp(g_usInPort));
    writePort(g_ucOUT_SENSE_CABLE_OUT, 0x01);
    ispVMDelay( 10000 );
    d2 =((g_ucIN_CABLE_V2_SENSE_IN + g_ucIN_CABLE_SENSE_IN) & _inp(g_usInPort));
    if (d2 == g_ucIN_CABLE_V2_SENSE_IN)
		old_cable = 0;
    else
		old_cable++;
    /* d2 and d1 will be equal if the cable is not installed */
    if (d2 != d1){
		/* V2.0 cable exist?*/
		writePort(g_ucOUT_SENSE_CABLE_OUT, 0x00);
		ispVMDelay( 1000 );
		d1 =(g_ucIN_CABLE_V2_SENSE_IN & _inp(g_usInPort));
		writePort(g_ucOUT_SENSE_CABLE_OUT, 0x01);
		ispVMDelay( 1000 );
		d2 =(g_ucIN_CABLE_V2_SENSE_IN & _inp(g_usInPort));
		if (d2 != d1)
			old_cable = 0;
		else
			old_cable++;
		success = TRUE;
    }         
    return (success);
}

/***************************************************************
*
* port_ok
*
* Determines if the cable is connected to the specified parallel
* port.
*
***************************************************************/

int port_ok()
{
	int d1, d2;     
	short int portData = 0;

	writePort( g_ucPinENABLE, 0x00 ); 
	writePort( g_ucPinCE, 0x01 );
	writePort( g_ucOUT_SENSE_CABLE_OUT, 0x01 );

	ispVMDelay( 10000 );
	d1 = ( g_ucIN_CABLE_SENSE_IN & _inp( g_usInPort ) );

	writePort( g_ucOUT_SENSE_CABLE_OUT, 0x00 );
	
	ispVMDelay( 10000 );
	d2 = ( g_ucIN_CABLE_SENSE_IN & _inp( g_usInPort ) );
	
	if ( d2 != d1 ) {
		return 1;
	}
	else {
		return 0;
	}
}

/***************************************************************
*
* power_ok
*
* Determines if the parallel port has power.
*
***************************************************************/

int power_ok()
{
	int iRetCode, iPrevRetCode;
	
	writePort( g_ucPinENABLE, 0x00 );
	writePort( g_ucPinCE, 0x01 );
	writePort( g_ucPinTCK + g_ucPinTMS + g_ucPinTDI + g_ucPinTRST, 0x00 );

	ispVMDelay( 5000 );
	iPrevRetCode = ( g_ucIN_VCC_OK & _inp( g_usInPort ) );

	ispVMDelay( 5000 );
	iRetCode = ( g_ucIN_VCC_OK & _inp( g_usInPort ) );

	if ( iPrevRetCode != iRetCode ) {
		iRetCode = 0;
	}
	return iRetCode;
}
