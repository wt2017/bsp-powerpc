/* includes */
#include <vxWorks.h>		/* vxWorks generics */
#include "semLib.h"
#include "bspspi.h"
#include "stdio.h"
#include "string.h"

extern void sysUsDelay(UINT32 delay);

LOCAL SEM_ID SpiMutexSem = NULL ;

void SpiClkCtl(int LowOrHigh)
{
    unsigned long volatile *pulReg = (unsigned long volatile*)BSP_SPI_OUT_REG;
    if(LowOrHigh)
    {
        *pulReg |= BSP_SPI_CLK_BIT;
    }
    else
    {
        *pulReg &= ~BSP_SPI_CLK_BIT;        
    }
}

void SpiCsCtl(int LowOrHigh)
{
    unsigned long volatile *pulReg = (unsigned long volatile*)BSP_SPI_OUT_REG;
    if(LowOrHigh)
    {
        *pulReg |= BSP_SPI_CS_BIT;
    }
    else
    {
        *pulReg &= ~BSP_SPI_CS_BIT;        
    }
}

void SpiMosiCtl(int LowOrHigh)
{
    unsigned long volatile *pulReg = (unsigned long volatile*)BSP_SPI_OUT_REG;
    if(LowOrHigh)
    {
        *pulReg |= BSP_SPI_MOSI_BIT;
    }
    else
    {
        *pulReg &= ~BSP_SPI_MOSI_BIT;        
    }
}

int SpiMisoCtl(void)
{
    unsigned long volatile *pulReg = (unsigned long volatile*)BSP_SPI_IN_REG;
    return (((*pulReg) & BSP_SPI_MISO_BIT) ? 1:0);
}

void BspSpiInit()
{
    unsigned long volatile *pulDEVReg = (unsigned long volatile*)M85XX_DEVDISR;        
	if(SpiMutexSem == NULL)
   		SpiMutexSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);

    *pulDEVReg |= BSP_PCI2_DISABLE;
    SpiCsCtl(1);    
    SpiClkCtl(1);    
}

void BspCycleSpiWrite(unsigned char ucVal)
{
//    semTake (SpiMutexSem, WAIT_FOREVER);
    unsigned long ulLoop = 0x80;
    int tmp = 0;

    while(ulLoop)
    {
        tmp = (ucVal & ulLoop) ? 1:0;
        SpiClkCtl(0);
        sysUsDelay(1);
        SpiMosiCtl(tmp);
        SpiClkCtl(1);
        sysUsDelay(1);        
        ulLoop >>= 1;
    }   
//	semGive (SpiMutexSem);
}

void BspCycleSpiRead(unsigned char *pucVal)
{
//	semTake (SpiMutexSem, WAIT_FOREVER);    
    unsigned long ulLoop = 0x80;
    int tmp = 0;

    while(ulLoop)
    {
        SpiClkCtl(0);
        sysUsDelay(1);
        tmp = SpiMisoCtl();
        SpiClkCtl(1);
        sysUsDelay(1);        
        *pucVal |= tmp ? ulLoop :0;
        ulLoop >>= 1;
    }   
//    semGive (SpiMutexSem);
}

int BspSpiWrite(char *buffer, int length)
{
    unsigned long ulLoop = 0;
    if((NULL == buffer) && (length < 3))
    {
        printf("\r\nbuffer == NULL!!!");
        return -1;
    }

    SpiCsCtl(0);
    BspCycleSpiWrite(buffer[0]); /*Ð´ Ð´ÃüÁî*/
    BspCycleSpiWrite(buffer[1]); /*Ð´ µØÖ·*/
    
    for(ulLoop = 0; ulLoop < length -2; ulLoop++)
    {
        BspCycleSpiWrite(buffer[ulLoop + 2]);
    }
    SpiCsCtl(1);

    return 0;
}

int BspSpiRead(char *buffer, int length)
{
    unsigned long ulLoop = 0;
    if((NULL == buffer) && (length < 3))
    {
        printf("\r\nbuffer == NULL!!!");
        return -1;
    }

    SpiCsCtl(0);
    BspCycleSpiWrite(buffer[0]); /*Ð´ ¶ÁÃüÁî*/
    BspCycleSpiWrite(buffer[1]); /*Ð´ µØÖ·*/
    
    for(ulLoop = 0; ulLoop < length -2; ulLoop++)
    {
        BspCycleSpiRead(&buffer[ulLoop +2]);
    }
    SpiCsCtl(1);
    return 0;
}

int spi_action_for_bcm532x(char *buffer, int length)
{
    if(SPI_532x_READ == buffer[0]) /*¶Á²Ù×÷*/
    {
        BspSpiRead(buffer, length);
    }
    else if(SPI_532x_WRITE== buffer[0]) /*Ð´²Ù×÷*/
    {
        BspSpiWrite(buffer, length);
    }
    else
    {
        printf("\r\nerror op!!!");
        return -1;
    }
    return 0;
}

STATUS bsp_spi_output(char *buffer, int length , unsigned short option , unsigned char read_action)
{
    if((NULL == buffer) || (0 == length))
    {
        return 0;
    }

	if(read_action) //read command , must read date from
	{
        return BspSpiRead(buffer, length);
    }
    else
    {
        return BspSpiWrite(buffer, length);
    }
}



STATUS Read532x(char page, char reg, char* data, int len)
{
	int i, j, byteCount = 0 , timeout_counter = 0;
	char buffer[100] ;

read_start:
	i = 0;
	while(1) 
	{ 
		i++;
    	buffer[0] = SPI_532x_READ ; buffer[1] =0xFE ; buffer[2] = 0 ;  byteCount = 3 ;
        spi_action_for_bcm532x(buffer , byteCount) ;

		if( (*(char *)(buffer + 2) & BCM532x_STS_SPIF) == 0 )
			break;

		if(i == 0x100 ) //if time out rewirte
		{
			i = 0; 
			printf("READ SPIF != 0 (%d)\n" , timeout_counter++);  	
			if (timeout_counter >= 0x20) 
				{
				printf("ERROR: timeout return\n") ;
				return -1;
				}
			buffer[0] = SPI_532x_WRITE ; buffer[1] = 0xFF ; buffer[2] = page ;  byteCount = 3 ;
			spi_action_for_bcm532x(buffer , byteCount) ;
		}  
	}

	buffer[0] = SPI_532x_WRITE ; buffer[1] = 0xFF ; buffer[2] = page ;  byteCount = 3 ;
	spi_action_for_bcm532x(buffer , byteCount) ;

	buffer[0] = SPI_532x_READ ; buffer[1] = reg ; buffer[2] = 0 ;  byteCount = 3 ;
	spi_action_for_bcm532x(buffer , byteCount) ;

	if(data == NULL ) /*OUTPUT to stdio*/
	{
		printf("Read532x page = 0x%x , reg = 0x%x  ,len = %d :\n ", page , reg , len);
	}

	for(j = 0 ;  j < len ; j++)
	{
		i = 0;   
		while(1)  
		{
			i++;
			buffer[0] = SPI_532x_READ ; buffer[1] =0xFE ; buffer[2] = 0 ;  byteCount = 3 ;
			spi_action_for_bcm532x(buffer , byteCount) ;	

			if( (*(char *)(buffer + 2) & BCM532x_STS_RACK) == BCM532x_STS_RACK )
				break;

			if( i==0x100 )
			{
				i = 0; 
				printf("RACK != 1 (%d)\n" , timeout_counter++);
				if (timeout_counter >= 0x10) 
					{
					printf("ERROR: timeout return\n") ;
					return ERROR;
					}
				buffer[0] = SPI_532x_WRITE ; buffer[1] =0xFF ; buffer[2] = 0 ;  byteCount = 3 ;
				spi_action_for_bcm532x(buffer , byteCount) ;	
				
				goto read_start;
			}  
		}

		buffer[0] = SPI_532x_READ ; buffer[1] =0xF0 ; buffer[2] = 0 ;  byteCount = 3 ;
		spi_action_for_bcm532x(buffer , byteCount) ;			
		if(data)
			data[j] =  buffer[2] ;
		else
			printf("  0x%02x" , buffer[2]) ;
		
		printf("\n");
	}
	
	return(OK);  
} /* end of Read532x */



/******************************************************************************
*
* Write532x - write the reg value of 532x Spi device
*
* This routine write the reg value of 532x Spi device
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/
STATUS Write532x(char page, char reg, char* data, int len)
{
	int i = 0 , byteCount = 0 , timeout_counter = 0;
	char buffer[BCM532x_MAX_BYTE_CNT] ;

	if(data == NULL || len > BCM532x_MAX_BYTE_CNT)
	{
		printf("\nError : param Error  data = 0x%x [not NULL] , len = %d[1:8]\n" ,(UINT32)buffer , len) ;
		return -1;
	}	
	
	while(1) 
	{
		i++;
		buffer[0] = SPI_532x_READ ; buffer[1] =0xFE ; buffer[2] = 0 ;  byteCount = 3 ;
		spi_action_for_bcm532x(buffer , byteCount) ;

		if( (*(char *)(buffer + 2) & BCM532x_STS_SPIF) == 0 )
			break;

		if( i==0x100 )
		{
			i = 0;
			printf("WRITE SPIF != 0 (%d)\n" , timeout_counter++);  	
			if (timeout_counter >= 0x10) 
			{
				printf("ERROR: timeout return\n") ;
				return -1;
			}
			buffer[0] = SPI_532x_WRITE ; buffer[1] =0xFF ; buffer[2] = page ;  byteCount = 3 ;
			spi_action_for_bcm532x(buffer , byteCount) ;
		}  
	}

	buffer[0] = SPI_532x_WRITE ; buffer[1] = 0xFF ; buffer[2] = page ;  byteCount = 3 ;
	spi_action_for_bcm532x(buffer , byteCount) ;

	buffer[0] = SPI_532x_WRITE ; buffer[1] = reg ; 
	memcpy(&buffer[2] , data , len);
	byteCount = 2 + len ;
	spi_action_for_bcm532x(buffer , byteCount) ;
	
	return(0);  
}/* end of Write532x */

int BspSpiTest()
{
    unsigned char ucVal = 0;
    Read532x( 0x2,0x88,&ucVal,1);
    if(ucVal != 0x24)
    {
        return -1;
    } 
    return 0;
}


