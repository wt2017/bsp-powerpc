/* Spansion's flash drvier for tffs*/
/*
	modification history
	
*/

#if  defined(INCLUDE_MTD_AMDXXX)

#include "dosFsLib.h"      /*for dosfs functions*/
#include "tffs/tffsdrv.h"
#include "tffs/flflash.h"
#include "tffs/backgrnd.h"
#include "tffs/flbase.h"
#include "sys/stat.h"

#define FLASH_BUF_SIZE		16
#define FLASH_DATA_WIDTH	(sizeof(unsigned short int))
#define FLASH_BUF_LEN		(FLASH_BUF_SIZE * FLASH_DATA_WIDTH)
#define	FLASH_ERASE_UNIT_SIZE		FLASH_BLOCK_SIZE
#define	FLASH_WAIT_TIME				0xffffff


#include "lld.c"

       
/* JEDEC ids for this MTD */
#define S29GL512N_FLASH	   0x2301
#define S29GL256N_FLASH	   0x2201
#define S29GL128N_FLASH	   0x2101
#define S29GL128N_SIZE	   0x1000000

FLBoolean flTakeFlashMutex(int serialNo , int mode) ;
void flFreeFlashMutex(int serialNo);

static FLStatus amdRead(  FLFlash vol,
			 CardAddress address, /* target flash address */
			 void FAR1 *buffer,   /* source RAM buffer    */
			 int length,          /* bytes to write       */
			 int modes)           /* Overwrite, EDC flags etc. */
{
	unsigned long SourceAddr;

	if (length < 0)	/* Only write words on word-boundary */
		{
		return flBadParameter;
		}

	flTakeFlashMutex(vol.socket->serialNo, 1);

	SourceAddr = (unsigned long) vol.map(&vol,address,length);
	memcpy(buffer,(void *)SourceAddr,length);

	flFreeFlashMutex(vol.socket->serialNo) ;	   
	return flOK;    
}

/*----------------------------------------------------------------------*/
/*                      a m d 2 5 6 W r i t e				*/
/*									*/
/* Write a block of bytes to Flash					*/
/*									*/
/* This routine will be registered as the MTD flash.write routine	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to write to			*/
/*      buffer		: Address of data to write			*/
/*	length		: Number of bytes to write			*/
/*	overwrite	: TRUE if overwriting old Flash contents	*/
/*			  FALSE if old contents are known to be erased	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus amdWrite(FLFlash vol,
                             CardAddress address,
                             const void FAR1 *buffer,
                             int length,
                             FLBoolean overwrite)
{
	unsigned long FlashPtr;
	FLStatus status = flOK;

	if (length <= 0 || (address & 1))	/* Only write words on word-boundary */
		{
		return flBadParameter;
		}

	flTakeFlashMutex(vol.socket->serialNo, 1);   	 
	
	if(vol.socket->writeProtected(vol.socket))
		{
		flFreeFlashMutex(vol.socket->serialNo);
		return flWriteProtect;
		}

#ifdef SOCKET_12_VOLTS
	checkStatus(flNeedVpp(vol.socket));
#endif

	vol.socket->VccOn(vol.socket);

	FlashPtr = (unsigned long) vol.map(&vol,0,0);

	if(lld_memcpy((FLASHDATA *)FlashPtr, address/2, length/2, (FLASHDATA *)buffer) == DEV_NOT_BUSY)
		status = flOK;
	else
		status = flGeneralFailure;
	
	if (memcmp((void *)(FlashPtr+ address), (void *)buffer , length) != 0)
	{
		printf("\namdWrite Error: the content you in the flash is not exactly you want write to it(at 0x%x)\n" , (int)(FlashPtr+ address));
		status = flGeneralFailure ; 
		taskSuspend(0);
	}
	
	//lld_ResetCmd((FLASHDATA *)FlashPtr);
	vol.socket->VccOff(vol.socket);

#ifdef SOCKET_12_VOLTS
	flDontNeedVpp(vol.socket);
#endif
	flFreeFlashMutex(vol.socket->serialNo);
	return status;
}


/*----------------------------------------------------------------------*/
/*                      a m d 2 5 6 E r a s e				*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks			*/
/*									*/
/* This routine will be registered as the MTD vol.erase routine 	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus amdErase(FLFlash vol,
                             int firstErasableBlock,
                             int numOfErasableBlocks)
{
	FLStatus status = flOK;	/* unless proven otherwise */
	unsigned long FlashPtr;
	int iBlock;
	  
	flTakeFlashMutex(vol.socket->serialNo, 1);     	 

	if(vol.socket->writeProtected(vol.socket))
		{
		flFreeFlashMutex(vol.socket->serialNo);
		return flWriteProtect;
		}

#ifdef SOCKET_12_VOLTS
	checkStatus(flNeedVpp(vol.socket));
#endif
	vol.socket->VccOn(vol.socket);

	FlashPtr = (unsigned long) vol.map(&vol,0,0);    
	//lld_ResetCmd((FLASHDATA *)FlashPtr);

	for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) 
	{
		unsigned long BlockAddr = (firstErasableBlock + iBlock) * vol.erasableBlockSize;
		if((status = lld_SectorEraseOp((FLASHDATA *)FlashPtr,BlockAddr/2)) == DEV_NOT_BUSY) 
			status = flOK;         
		else
		{
			printf("\namdErase(%d , %d) error ,return status = 0x%x\n" , firstErasableBlock , numOfErasableBlocks , status);
			status = flGeneralFailure ; 
			taskSuspend(0);
		}
	}

#ifdef SOCKET_12_VOLTS
	flDontNeedVpp(vol.socket);
#endif
	//lld_ResetCmd((FLASHDATA *)FlashPtr);
	vol.socket->VccOff(vol.socket);

	flFreeFlashMutex(vol.socket->serialNo);
	return status;
}

/*----------------------------------------------------------------------*/
/*                      a m d 2 5 6 M a p				*/
/*									*/
/* Map one Flash memory space             			        */
/*									*/
/*----------------------------------------------------------------------*/
void FAR0 * amdMap(FLFlash vol,
  		      CardAddress address,
		      int length )
{
	int res;

	if (address > vol.socket->window.size) 
		{
		printf("amdMap address error = (address =  0x%x)\n" , (int)address) ;
		taskSuspend(0);
		}	
	res = (vol.socket->window.baseAddress<<12) + address;
	return (void*)(res);
}

/*----------------------------------------------------------------------*/
/*                     a m d 5 1 2 I d e n t i f y            		*/
/*									*/
/* Identifies media based on AMD256 and registers as an MTD for	*/
/* such.								*/
/*									*/
/* This routine will be placed on the MTD list in custom.h. It must be	*/
/* an extern routine.							*/
/*									*/
/* On successful identification, the Flash structure is filled out and	*/
/* the write and erase routines registered.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on positive identificaion, failed otherwise	*/
/*----------------------------------------------------------------------*/

FLStatus amdIdentify(FLFlash vol)
{
	FlashWPTR flashPtr;
	int deviceId;
	int manufactureId;
            
	flTakeFlashMutex(vol.socket->serialNo , 1);
	    
	flashPtr = (FlashWPTR)vol.map(&vol,0,0);
	manufactureId = lld_GetManufactureId((FLASHDATA *)flashPtr);
	deviceId = lld_GetDeviceId((FLASHDATA *)flashPtr);

	if (manufactureId == 0x01 && deviceId == 0x7e2101) /*S29GL128N*/
		{
		/* Word mode */
		vol.noOfChips = 1;
		vol.type = S29GL128N_FLASH;
		vol.interleaving = 1;
		vol.chipSize = S29GL128N_SIZE;
		vol.erasableBlockSize = FLASH_BLOCK_SIZE * vol.interleaving;
		}
	else if (manufactureId == 0x01 && deviceId == 0x7E2201) /*S29GL256N*/
		{
		/* Word mode */
		vol.noOfChips = 1;
		vol.type = S29GL256N_FLASH;
		vol.interleaving = 1;
		vol.chipSize = S29GL128N_SIZE * 2;
		vol.erasableBlockSize = FLASH_BLOCK_SIZE * vol.interleaving;
		}		
	else if (manufactureId == 0x01 && deviceId == 0x7E2301) /*S29GL512N*/
		{
		/* Word mode */
		vol.noOfChips = 1;
		vol.type = S29GL512N_FLASH;
		vol.interleaving = 1;

		if(0 == vol.socket->serialNo)
		{
            vol.chipSize = S29GL128N_SIZE * 4 - FLASH_TFFS_SIZE_RESERVE;
		}
		else
		{
    		vol.chipSize = S29GL128N_SIZE * 4;
    	}	
		vol.erasableBlockSize = FLASH_BLOCK_SIZE * vol.interleaving;
		}	
	else 
		{
		flFreeFlashMutex(vol.socket->serialNo);
		return flUnknownMedia;	/* not ours */
		}

	flSetWindowSize(vol.socket , vol.chipSize >>12 );			
	flSetWindowBusWidth(vol.socket,16);/* use 16-bits */
	flSetWindowSpeed(vol.socket,120);  /* 120 nsec. */
	
	/* Register our flash handlers */
	vol.write = amdWrite;
	vol.erase = amdErase;
	vol.read  = amdRead;
	vol.map   = amdMap; 		        
	flFreeFlashMutex(vol.socket->serialNo);
	return flOK;
}

static int bsp_amd_erase_block(int chipno,unsigned long seg_no) 
{
	unsigned int base_addr  = 0 ; 
	GET_FLASH_BASE_ADDR(chipno, base_addr) ;
	
	if(lld_SectorEraseOp((void *)base_addr , seg_no * (FLASH_ERASE_UNIT_SIZE/2)) == DEV_NOT_BUSY)
		return OK ; 
	else 
		return ERROR ;
	
}
int bsp_flash_erase(int chipno, int first, int num)
{
	int	i_block, status	= 0;
	
	flTakeFlashMutex(chipno , 1);	

	flash_write_enable(chipno) ;
	for	(i_block = 0; i_block <	num ;	i_block++) 
	{
		status = bsp_amd_erase_block(chipno,first + i_block);
		if(status != OK)
		{
			printf("bsp_amd_erase() error when block_num = %d \n" , (first + i_block));
			break ;
		}
	}	
	flash_write_disable(chipno) ;

	flFreeFlashMutex(chipno);
	return status;
}
int	bsp_flash_write(int chipno, int address, char *buffer , int length)
{
	unsigned int base_addr  = 0 , status = 0;	
	GET_FLASH_BASE_ADDR(chipno, base_addr) ;	
	
	flTakeFlashMutex(chipno , 1);		

	flash_write_enable(chipno) ;

	if(lld_memcpy((FLASHDATA *)base_addr, address/2, length/2, (FLASHDATA *)buffer) == DEV_NOT_BUSY)
		status = OK ;
	else
		status = ERROR ;
	
	flash_write_disable(chipno) ;

	flFreeFlashMutex(chipno);
	return status ; 
}

int  bsp_flash_read(int chipno, int address, char *buffer , int length)
{
	unsigned int base_addr  = 0 ;
	GET_FLASH_BASE_ADDR(chipno, base_addr) ;	
	
	flTakeFlashMutex(chipno , 1);		
	memcpy(buffer ,(char *)(base_addr+address),  length) ;
	flFreeFlashMutex(chipno);
	
	return OK ; 
}

#endif	/*INCLUDE_FLASH_AMDXXX*/

