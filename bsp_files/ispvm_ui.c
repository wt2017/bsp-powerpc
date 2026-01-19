/**************************************************************
*
* Lattice Semiconductor Corp. Copyright 2007
* 
* ispVME Embedded allows programming of Lattice's suite of FPGA
* devices on embedded systems through the JTAG port.  The software
* is distributed in source code form and is open to re - distribution
* and modification where applicable.
*
* ispVME Embedded C Source comprised with 3 modules:
* ispvm_ui.c is the module provides input and output support.
* ivm_core.c is the module interpret the VME file(s).
* hardware.c is the module access the JTAG port of the device(s).                 
*
* The optional module cable.c is for supporting Lattice's parallel 
* port ispDOWNLOAD cable on DOS and Windows 95/98 O/S. It can be 
* requested from Lattice's ispVMSupport.
*
***************************************************************/


/**************************************************************
* 
* Revision History of ispvm_ui.c
* 
* 3/6/07 ht Added functions vme_out_char(),vme_out_hex(), 
*           vme_out_string() to provide output resources.
*           Consolidate all printf() calls into the added output 
*           functions.	
*
***************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmopcode.h"

/***************************************************************
*
* File pointer to the VME file.
*
***************************************************************/

FILE * g_pVMEFile = NULL;

/***************************************************************
*
* Functions declared in this ispvm_ui.c module
*
***************************************************************/
unsigned char GetByte(void);
void vme_out_char(unsigned char charOut);
void vme_out_hex(unsigned char hexOut);
void vme_out_string(unsigned char *stringOut);
void ispVMMemManager( signed char cTarget, unsigned short usSize );
void ispVMFreeMem(void);
void error_handler( short a_siRetCode, char * pszMessage );
signed char ispVM( const char * a_pszFilename );

/***************************************************************
*
* External variables and functions declared in ivm_core.c module.
*
***************************************************************/
extern signed char ispVMCode();
extern void ispVMCalculateCRC32( unsigned char a_ucData );
extern void ispVMStart();
extern void ispVMEnd();
extern unsigned short g_usCalculatedCRC;
extern unsigned short g_usDataType;
extern unsigned short g_usDelayPercent;
extern unsigned char * g_pucOutMaskData,
                     * g_pucInData,
					 * g_pucOutData,
					 * g_pucHIRData,
					 * g_pucTIRData,
					 * g_pucHDRData,
					 * g_pucTDRData,
					 * g_pucOutDMaskData,
                     * g_pucIntelBuffer;
extern unsigned char * g_pucHeapMemory;
extern unsigned short g_iHeapCounter;
extern unsigned short g_iHEAPSize;
extern unsigned short g_usIntelDataIndex;
extern unsigned short g_usIntelBufferSize;
extern const char * const g_szSupportedVersions[];
extern LVDSPair * g_pLVDSList;

/***************************************************************
*
* GetByte
*
* Returns a byte to the caller. The returned byte depends on the
* g_usDataType register. If the HEAP_IN bit is set, then the byte
* is returned from the HEAP. If the LHEAP_IN bit is set, then
* the byte is returned from the intelligent buffer. Otherwise,
* the byte is returned directly from the VME file.
*
***************************************************************/

unsigned char GetByte()
{
	unsigned char ucData;
	
	if ( g_usDataType & HEAP_IN ) {

		/***************************************************************
		*
		* Get data from repeat buffer.
		*
		***************************************************************/

		if ( g_iHeapCounter > g_iHEAPSize ) {

			/***************************************************************
			*
			* Data over-run.
			*
			***************************************************************/

			return 0xFF;
		}

		ucData = g_pucHeapMemory[ g_iHeapCounter++ ];
	}
	else if ( g_usDataType & LHEAP_IN ) {

		/***************************************************************
		*
		* Get data from intel buffer.
		*
		***************************************************************/

		if ( g_usIntelDataIndex >= g_usIntelBufferSize ) {

			/***************************************************************
			*
			* Data over-run.
			*
			***************************************************************/

			return 0xFF;
		}

		ucData = g_pucIntelBuffer[ g_usIntelDataIndex++ ];
	}
	else {

		/***************************************************************
		*
		* Get data from file.
		*
		***************************************************************/

		ucData = fgetc( g_pVMEFile );

		if ( feof( g_pVMEFile ) ) {

			/***************************************************************
			*
			* Reached EOF.
			*
			***************************************************************/

			return 0xFF;
		}
	}
	
	return ( ucData );
}

/***************************************************************
*
* vme_out_char
*
* Send a character out to the output resource if available. 
* The monitor is the default output resource. 
*
*
***************************************************************/
void vme_out_char(unsigned char charOut)
{
	printf("%c",charOut);
}
/***************************************************************
*
* vme_out_hex
*
* Send a character out as in hex format to the output resource 
* if available. The monitor is the default output resource. 
*
*
***************************************************************/
void vme_out_hex(unsigned char hexOut)
{
	printf("%.2X",hexOut);
}
/***************************************************************
*
* vme_out_string
*
* Send a text string out to the output resource if available. 
* The monitor is the default output resource. 
*
*
***************************************************************/
void vme_out_string(unsigned char *stringOut)
{
	if(stringOut)
	{
		printf("%s",stringOut);
	}

}
/***************************************************************
*
* ispVMMemManager
*
* Allocate memory based on cTarget. The memory size is specified
* by usSize.
*
***************************************************************/

void ispVMMemManager( signed char cTarget, unsigned short usSize )
{
	static unsigned short usPreviousSize = 0;
	switch ( cTarget ) {
	case XTDI:
    case TDI:  
		if ( g_pucInData != NULL ) {
			if ( usPreviousSize == usSize ) {/*memory exist*/
				break;
			}
			else {
				free( g_pucInData );
				g_pucInData = NULL;
			}
		}
		g_pucInData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		usPreviousSize = usSize;
    case XTDO:
    case TDO:
		if ( g_pucOutData!= NULL ) { 
			if ( usPreviousSize == usSize ) { /*already exist*/
				break;
			}
			else {
				free( g_pucOutData );
				g_pucOutData = NULL;
			}
		}
		g_pucOutData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		usPreviousSize = usSize;
		break;
    case MASK:
		if ( g_pucOutMaskData != NULL ) {
			if ( usPreviousSize == usSize ) {/*already allocated*/
				break;
			}
			else {
				free( g_pucOutMaskData ); 
				g_pucOutMaskData = NULL;
			}
		}
		g_pucOutMaskData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		usPreviousSize = usSize;
		break;
    case HIR:
		if ( g_pucHIRData != NULL ) {
			free( g_pucHIRData );
			g_pucHIRData = NULL;
		}
		g_pucHIRData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		break;
    case TIR:
		if ( g_pucTIRData != NULL ) {
			free( g_pucTIRData );
			g_pucTIRData = NULL;
		}
		g_pucTIRData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		break;
    case HDR:
		if ( g_pucHDRData != NULL ) {
			free( g_pucHDRData );
			g_pucHDRData = NULL;
		}
		g_pucHDRData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		break;
    case TDR:
		if ( g_pucTDRData != NULL ) {
			free( g_pucTDRData );
			g_pucTDRData = NULL;
		}
		g_pucTDRData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		break;
    case HEAP:
		if ( g_pucHeapMemory != NULL ) {
			free( g_pucHeapMemory );
			g_pucHeapMemory = NULL;
		}
		g_pucHeapMemory = ( unsigned char * ) malloc( usSize + 2 );
		break;
	case DMASK: 
		if ( g_pucOutDMaskData != NULL ) {
			if ( usPreviousSize == usSize ) { /*already allocated*/
				break;
			}
			else {
				free( g_pucOutDMaskData ); 
				g_pucOutDMaskData = NULL;
			}
		}
		g_pucOutDMaskData = ( unsigned char * ) malloc( usSize / 8 + 2 );
		usPreviousSize = usSize;
		break;
	case LHEAP:
		if ( g_pucIntelBuffer != NULL ) {
			free( g_pucIntelBuffer );
			g_pucIntelBuffer = NULL;
		}
		g_pucIntelBuffer = ( unsigned char * ) malloc( usSize + 2 );
		break;
	case LVDS:
		if ( g_pLVDSList != NULL ) {
			free( g_pLVDSList );
			g_pLVDSList = NULL;
		}
		g_pLVDSList = ( LVDSPair * ) calloc( usSize, sizeof( LVDSPair ) );
		break;
	default:
		return;
    }
}

/***************************************************************
*
* ispVMFreeMem
*
* Free memory that were dynamically allocated.
*
***************************************************************/

void ispVMFreeMem()
{
	if ( g_pucHeapMemory != NULL ) {
		free( g_pucHeapMemory ); 
		g_pucHeapMemory = NULL;
	}

	if ( g_pucOutMaskData != NULL ) {
		free( g_pucOutMaskData );
		g_pucOutMaskData = NULL;
	}
	
	if ( g_pucInData != NULL ) {
		free( g_pucInData );
		g_pucInData = NULL;
	}
	
	if ( g_pucOutData != NULL ) {
		free( g_pucOutData );
		g_pucOutData = NULL;
	}
	
	if ( g_pucHIRData != NULL ) {
		free( g_pucHIRData );
		g_pucHIRData = NULL;
	}
	
	if ( g_pucTIRData != NULL ) {
		free( g_pucTIRData );
		g_pucTIRData = NULL;
	}
	
	if ( g_pucHDRData != NULL ) {
		free( g_pucHDRData );
		g_pucHDRData = NULL;
	}
	
	if ( g_pucTDRData != NULL ) {
		free( g_pucTDRData );
		g_pucTDRData = NULL;
	}
	
	if ( g_pucOutDMaskData != NULL ) {
		free( g_pucOutDMaskData );
		g_pucOutDMaskData = NULL;
	}
	
	if ( g_pucIntelBuffer != NULL ) {
		free( g_pucIntelBuffer );
		g_pucIntelBuffer = NULL;
	}

	if ( g_pLVDSList != NULL ) {
		free( g_pLVDSList );
		g_pLVDSList = NULL;
	}
} 

/***************************************************************
*
* error_handler
*
* Reports the error message.
*
***************************************************************/

void error_handler( short a_siRetCode, char * pszMessage )
{
	const char * pszErrorMessage[] = { "pass",
									   "verification fail",
									   "can't find the file",
									   "wrong file type",
									   "file error",
									   "option error",
									   "crc verification error" };

	strcpy( pszMessage, pszErrorMessage[ -a_siRetCode ] );
}
/***************************************************************
*
* ispVM
*
* The entry point of the ispVM embedded. If the version and CRC
* are verified, then the VME will be processed.
*
***************************************************************/

signed char ispVM( const char * a_pszFilename )
{
	char szFileVersion[ 9 ] = { 0 };
	signed char cRetCode = 0;
	signed char cIndex;
	signed char cVersionIndex;
	unsigned char ucReadByte;
	unsigned short usExpectedCRC = 0;
	int iCRCData;
	
	/***************************************************************
	*
	* Open a file pointer to the VME file.
	*
	***************************************************************/

	if ( ( g_pVMEFile = fopen( a_pszFilename, "rb" ) ) == NULL ) {
		return VME_FILE_READ_FAILURE;
	}
	g_usCalculatedCRC = 0;
	ucReadByte = GetByte();
	switch( ucReadByte ) {
	case FILE_CRC:

		/***************************************************************
		*
		* Get the expected CRC.  Only versions 3.0 and higher support
		* CRC protection.
		*
		***************************************************************/

		usExpectedCRC = fgetc( g_pVMEFile );
		usExpectedCRC <<= 8;
		usExpectedCRC |= fgetc( g_pVMEFile );

		while ( ( iCRCData = getc( g_pVMEFile ) ) != EOF ) {
			ispVMCalculateCRC32( ( unsigned char ) iCRCData );
		}

		/***************************************************************
		*
		* Compare the expected CRC versus the calculated CRC.
		*
		***************************************************************/

		if ( usExpectedCRC && ( usExpectedCRC != g_usCalculatedCRC ) ) {
			printf( "Expected CRC:   0x%.4X\n", usExpectedCRC );
			printf( "Calculated CRC: 0x%.4X\n", g_usCalculatedCRC );
			fclose( g_pVMEFile );
			g_pVMEFile = NULL;
			return VME_CRC_FAILURE;
		}

		/***************************************************************
		*
		* Rewind the VME file pointer back to the beginning and discard
		* the three CRC related bytes.
		*
		***************************************************************/

		rewind( g_pVMEFile );
		GetByte();
		GetByte();
		GetByte();

		/***************************************************************
		*
		* Read and store the version of the VME file.
		*
		***************************************************************/

		for ( cIndex = 0; cIndex < 8; cIndex++ ) {
			szFileVersion[ cIndex ] = GetByte();
		}

		break;
	default:

		/***************************************************************
		*
		* Read and store the version of the VME file.  Must be version 2.0.
		*
		***************************************************************/

		szFileVersion[ 0 ] = ( signed char ) ucReadByte;
		for ( cIndex = 1; cIndex < 8; cIndex++ ) {
			szFileVersion[ cIndex ] = GetByte();
		}

		break;
	}

	/***************************************************************
	*
	* Compare the VME file version against the supported version.
	*
	***************************************************************/

	for ( cVersionIndex = 0; g_szSupportedVersions[ cVersionIndex ] != 0; cVersionIndex++ ) {
		for ( cIndex = 0; cIndex < 8; cIndex++ ) {
			if ( szFileVersion[ cIndex ] != g_szSupportedVersions[ cVersionIndex ][ cIndex ] ) {
				cRetCode = VME_VERSION_FAILURE;
				break;
			}	
			cRetCode = 0;
		}

		if ( cRetCode == 0 ) {

			/***************************************************************
			*
			* Found matching version, break.
			*
			***************************************************************/

			break;
		}
	}

	if ( cRetCode < 0 ) {

		/***************************************************************
		*
		* VME file version failed to match the supported versions.
		*
		***************************************************************/

		fclose( g_pVMEFile );
		g_pVMEFile = NULL;
		return VME_VERSION_FAILURE;
	}

	/***************************************************************
	*
	* Enable the JTAG port to communicate with the device.
    * Set the JTAG state machine to the Test-Logic/Reset State.
	*
	***************************************************************/

    ispVMStart();

	/***************************************************************
	*
	* Process the VME file.
	*
	***************************************************************/

    cRetCode = ispVMCode();

	/***************************************************************
	*
	* Set the JTAG State Machine to Test-Logic/Reset state then disable
    * the communication with the JTAG port.
	*
	***************************************************************/

    ispVMEnd();
                   
    fclose( g_pVMEFile );
	g_pVMEFile = NULL;


	ispVMFreeMem();
    return ( cRetCode );
}

/***************************************************************
*
* main
*
***************************************************************/


#include "bspCommon.h"

extern int ispVMEReset() ;

int _jtag_download(char *fileName)
	{
	short siRetCode = 0;
	vme_out_string( "                 Lattice Semiconductor Corp.\n" );
	vme_out_string( "\n             ispVME(tm) V");
	vme_out_string( VME_VERSION_NUMBER );
	vme_out_string(" Copyright 1998-2007.\n");
	vme_out_string( "\nFor daisy chain programming of all in-system programmable devices\n\n" );

	g_usDelayPercent = 0 /* delayPercent*/ ;
	siRetCode = 0;
	vme_out_string("Processing virtual machine file (");
	vme_out_string(fileName);
	vme_out_string (")......\n\n");
	siRetCode = ispVM(fileName);

	if ( siRetCode < 0 ) {
	error_handler( siRetCode, fileName );
	vme_out_string( "Failed due to ");
	vme_out_string ( fileName );
	vme_out_string ("\n\n");
	vme_out_string( "+=======+\n" );
	vme_out_string( "| FAIL! |\n" );
	vme_out_string( "+=======+\n\n" );
	ispVMEReset() ;
	} 
	else {
	ispVMEReset() ;
	vme_out_string( "+=======+\n" );
	vme_out_string( "| PASS! |\n" );
	vme_out_string( "+=======+\n\n" );
	}

	return( siRetCode );
	} 

