/*
* $Log:   P:/user/amir/lite/vcs/i28f640.c_v  $
* 
*    Rev 1.10   06 Oct 1997  9:45:48   danig
* VPP functions under #ifdef
* 
*    Rev 1.9   10 Sep 1997 16:48:24   danig
* Debug messages & got rid of generic names
* 
*    Rev 1.8   31 Aug 1997 15:09:20   danig
* Registration routine return status
* 
*    Rev 1.7   24 Jul 1997 17:52:58   amirban
* FAR to FAR0
* 
*    Rev 1.6   20 Jul 1997 17:17:06   amirban
* No watchDogTimer
* 
*    Rev 1.5   07 Jul 1997 15:22:08   amirban
* Ver 2.0
* 
*    Rev 1.4   04 Mar 1997 16:44:22   amirban
* Page buffer bug fix
* 
*    Rev 1.3   18 Aug 1996 13:48:24   amirban
* Comments
* 
*    Rev 1.2   12 Aug 1996 15:49:04   amirban
* Added suspend/resume
* 
*    Rev 1.1   31 Jul 1996 14:30:50   amirban
* Background stuff
* 
*    Rev 1.0   18 Jun 1996 16:34:30   amirban
* Initial revision.
*/

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1996			*/
/*									*/
/************************************************************************/

/*----------------------------------------------------------------------*/
/*                                                                      */
/* This MTD supports the following Flash technologies:                  */
/*                                                                      */
/* - Intel 28F128SA/28128SV/Cobra 16-mbit devices			*/
/*									*/
/* And (among else), the following Flash media and cards:		*/
/*                                                                      */
/* - Intel Series-2+ PCMCIA cards                                       */
/*									*/
/*----------------------------------------------------------------------*/


/* JEDEC ids for this MTD */
#define I28F640_FLASH	         0x8917

#define SETUP_ERASE	         0x20
#define CONFIRM_ERASE	         0xd0
#define SETUP_WRITE_BUFFER       0xe8
#define CONFIRM_WRITE_BUFFER     0xd0
#define CLEAR_STATUS	         0x50
#define READ_STATUS	         0x70
#define READ_EXTENDED_REGS       0x71
#define READ_ID 	         0x90
#define READ_ARRAY	         0xff
#define BIT_7                    0x80

/* FLASH基本参数 */
#define FLASH_CHIP_NUM           1  
#define FLASH_INTERLEAVING       1
/* report size must equal windows size */
#define ERASE_UNIT_SIZE          0x20000

#define FLASH_I28F640_SIZE 	   0x00800000

/* 等待时间 */
#undef FLASH_WAIT_TIME          
#define FLASH_WAIT_TIME          0xffffff

#define REG8(Addr)     (*(volatile unsigned char*)(Addr)) 

/* 数据类型定义 */
typedef unsigned short WORD;
typedef unsigned long  DWORD;


/* 外部函数函数预定义 */
extern FLBoolean flTakeFlashMutex(int serialNo , int mode) ;
extern void flFreeFlashMutex(int serialNo);

/******************************************************************/
/*        函数名: DrvReset                                        */
/*        功  能: 复位芯片interl i28fxxx  系列FLASH               */
/******************************************************************/
static void DrvReset(void)
{
    REG8(FLASH_BASE_ADRS) = CLEAR_STATUS;
    REG8(FLASH_BASE_ADRS) = READ_ARRAY;

    REG8(FLASH_BASE_ADRS) = CLEAR_STATUS;
    REG8(FLASH_BASE_ADRS) = READ_ARRAY;

    REG8(FLASH_BASE_ADRS) = CLEAR_STATUS;
    REG8(FLASH_BASE_ADRS) = READ_ARRAY;

    return;
}

static FLStatus WriteBufferByte(DWORD FlashAddr, DWORD SrcAddr , int DataLen)
{
    int i , aa;
 
    FLStatus  Status= flGeneralFailure;
	if (DataLen <= 0)
		return flOK;
    
    for(i=0; i<FLASH_WAIT_TIME; i++)
    {
    	REG8(FlashAddr) = SETUP_WRITE_BUFFER;  /*写命令*/
        aa = REG8(FlashAddr);
        if((REG8(FlashAddr) & BIT_7) == BIT_7)
        {
            Status = flOK;
            break;
        }
    }
    if(i == FLASH_WAIT_TIME)
         return flGeneralFailure;

    REG8(FlashAddr) = DataLen-1;    /*<=32个字节*/	
    /* tffscpyWords((unsigned long FAR0 *) FlashAddr, (const char FAR1 *) SrcAddr , DataLen); */
    for(i = 0; i < DataLen; i = i + 1)
    {
        REG8(FlashAddr + i) = REG8(SrcAddr + i);
    }
    
    REG8(FlashAddr) = CONFIRM_WRITE_BUFFER;
    
    
    for(i=0; i<FLASH_WAIT_TIME; i++)
    {
        REG8(FlashAddr) = READ_STATUS;
        if((REG8(FlashAddr) & BIT_7) == BIT_7)
        {
            Status = flOK;
            break;
        }
    }

    DrvReset();
    /* verify the data */
    if (Status == flOK && (bcmp((char *) FlashAddr, (char *)SrcAddr,DataLen)!=0))/*DataLen = length in byte*/
    {        
        logMsg("ERROR: write failed for falsh %p, len: %x.\n",FlashAddr,DataLen ,3,4,5,6);
        Status = flGeneralFailure;
    }  
    return Status;
}

static FLStatus i28f640Read(FLFlash vol,
			 CardAddress address, /* target flash address */
			 void FAR1 *buffer,    /* source RAM buffer  */
			 int length ,      /* bytes to write      */
			 int modes)           /* Overwrite, EDC flags etc. */
{
	DWORD DstAddrOrg;
	if (length < 0)	/* Only write words on word-boundary */
		{
		return flBadParameter;
		}
	
	flTakeFlashMutex(vol.socket->serialNo, 1);

	DstAddrOrg = (DWORD) vol.map(&vol,address,length);

	memcpy(buffer,(void *)DstAddrOrg,length);
	flFreeFlashMutex(vol.socket->serialNo);
	return flOK;    
}
/*----------------------------------------------------------------------*/
/*                      i 2 8 f 1 2 8 W r i t e				*/
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

static FLStatus i28f640Write(FLFlash vol,
                             CardAddress address,/* target flash offset address */
                             const void FAR1 *buffer, /* source RAM buffer    */
                             int length,          /* bytes to write       */
                             FLBoolean overwrite)  /* Overwrite, EDC flags etc. */
{
    DWORD SrcAddr, DstAddr, DstAddrOrg;
    DWORD ByteLen ;
    DWORD BufNo, BufOffset, ByteOffset;
    FLStatus Status = flOK;
   
    flTakeFlashMutex(vol.socket->serialNo, 1);
    if(vol.socket->writeProtected(vol.socket))
    {
	flFreeFlashMutex(vol.socket->serialNo);
    	return flWriteProtect;
    }
	
    if(address >= FLASH_SIZE || length > FLASH_SIZE || (address + length) > FLASH_SIZE)	
    {
    	logMsg("Error : i28f640Write overflow address = 0x%x , length=%d \n"  ,address , length ,3,4,5,6);
       	taskSuspend(0);
	flFreeFlashMutex(vol.socket->serialNo);
   	return flBadParameter;
    }
    
#ifdef SOCKET_12_VOLTS
    checkStatus(flNeedVpp(vol.socket));
#endif
    DstAddrOrg = (DWORD) vol.map(&vol,address,length);

    DstAddr = DstAddrOrg;
    SrcAddr = (DWORD)buffer;
    ByteLen = length ;
    
    ByteOffset = 32-DstAddr%32; 
    DrvReset();
    
    /* *((unsigned int*)(0xfff00ac4)) |= 0x00020000;*/    
    vol.socket->VccOn(vol.socket);
	
#if 1    
    if(ByteLen<=ByteOffset)
    {
        /*can not exceed the erase unit, because the erase unit =128k*/
        Status = WriteBufferByte(DstAddr, SrcAddr,  ByteLen);  
    }
    else
    {
        Status = WriteBufferByte(DstAddr, SrcAddr,  ByteOffset);
        if(Status == flOK)
        {        
            ByteLen =ByteLen -ByteOffset;
            SrcAddr = SrcAddr + ByteOffset;
            DstAddr = DstAddr + ByteOffset;  /*now DstAddr have the A4~A0 = 0;*/
            
            BufNo = ByteLen / 32;
            BufOffset = ByteLen % 32;
            
            for(; BufNo >0; BufNo--)
            {
                
                Status = WriteBufferByte(DstAddr, SrcAddr, 32);
                if(Status != flOK)
                    break;
                
                SrcAddr = SrcAddr + 32;
                DstAddr = DstAddr + 32;
                
            }
            
            if(Status == flOK)
            {
                Status = WriteBufferByte(DstAddr, SrcAddr,  BufOffset);
            }
            
        }
    }
    
#endif

    DrvReset();
    vol.socket->VccOff(vol.socket);
	/* *((unsigned int*)(0xfff00ac4)) &= ~0x00020000; */   /*PB14 low not lead to write*/

#ifdef SOCKET_12_VOLTS
    flDontNeedVpp(vol.socket);
#endif

    flFreeFlashMutex(vol.socket->serialNo);
    return Status;
}


/*----------------------------------------------------------------------*/
/*                      i 2 8 f 6 4 0 E r a s e				*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks			*/
/*									*/
/* This routine will be registered as the MTD vol.erase routine	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

static FLStatus i28f640Erase(FLFlash vol,
                             int firstErasableBlock,
                             int numOfErasableBlocks)
{
    FLStatus status = flOK;	/* unless proven otherwise */
    int iBlock;
    
   flTakeFlashMutex(vol.socket->serialNo, 1);
    if((firstErasableBlock+numOfErasableBlocks) > 56)	
    {
    	logMsg("Error : i28f640Erase overflow firstErasableBlock = d ,numOfErasableBlocks = %d \n"  ,firstErasableBlock , numOfErasableBlocks,3,4,5,6);
       	taskSuspend(0);
    }	
	
    if(vol.socket->writeProtected(vol.socket))
    {
	flFreeFlashMutex(vol.socket->serialNo);
    	return flWriteProtect;
    }
    
#ifdef SOCKET_12_VOLTS
    checkStatus(flNeedVpp(vol.socket));
#endif
    
    DrvReset();
    vol.socket->VccOn(vol.socket);
    /* *((unsigned int*)(0xfff00ac4)) |= 0x00020000; */   /*PB14 high lead to write*/
    
    for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) 
    {
        DWORD i, BlockAddr;
        WORD ReadBack;
        
        status = flGeneralFailure;
        /*是否应该用flMap???*/
        /* BlockAddr = FLASH_FFS_BASE_ADDR + SegNo * ERASE_UNIT_SIZE;   */
        BlockAddr = (DWORD)vol.map(&vol,(firstErasableBlock + iBlock) * vol.erasableBlockSize,0);
        
        REG8(BlockAddr) = SETUP_ERASE;
        REG8(BlockAddr) = CONFIRM_ERASE;
        
        for (i = 0; i < FLASH_WAIT_TIME; i++)
        {
            REG8(BlockAddr) = READ_STATUS;
            ReadBack = REG8(BlockAddr);
            if ((ReadBack & BIT_7) == BIT_7)
            {
                status = flOK;
                break;
            }
        }
        
        if(status != flOK)
            break;
    }
    
#ifdef SOCKET_12_VOLTS
    flDontNeedVpp(vol.socket);
#endif
    DrvReset();
    vol.socket->VccOff(vol.socket);
    flFreeFlashMutex(vol.socket->serialNo);
    return status;
}

void FAR0 * i28f640Map(FLFlash vol,
			    CardAddress address,
			    int length )
{
	if (address > vol.socket->window.size) 
		{
		printf("i28fxxx address error = (address =  %x)\n" , address) ;
		taskSuspend(0);
		}	
	return (void*)((vol.socket->window.baseAddress<<12)+address);
}

/*----------------------------------------------------------------------*/
/*                     i 2 8 f 640 I d e n t i f y			*/
/*									*/
/* Identifies media based on Intel 28F128 and registers as an MTD for	*/
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

FLStatus i28fIdentify(FLFlash vol)
{
    FlashPTR flashPtr; /* byte pointer */
    
#ifdef DEBUG_PRINT
    DEBUG_PRINT("Debug: entering 8-bit Intel media identification routine.\n");
#endif
    taskDelay(1);
    flTakeFlashMutex(vol.socket->serialNo, 1);
	
    flashPtr = (FlashPTR)vol.map(&vol,0,0);
    
    flashPtr[0] = READ_ID;
    if (flashPtr[0] == 0x89 && flashPtr[2] == 0x17) 
    {
        /* Word mode */
        vol.noOfChips = FLASH_CHIP_NUM;
        vol.type = I28F640_FLASH;
        vol.interleaving = FLASH_INTERLEAVING;
        flashPtr[0] = READ_ARRAY;
    }
    
    if (vol.type == I28F640_FLASH) 
    {
        vol.chipSize = FLASH_I28F640_SIZE;
        vol.erasableBlockSize = ERASE_UNIT_SIZE * vol.interleaving;
        
        /* Register our flash handlers */
        vol.write = i28f640Write;
        vol.erase = i28f640Erase;
        vol.read  = i28f640Read;
	 vol.map   = i28f640Map; 
		
	flSetWindowBusWidth(vol.socket,8);/* use 8-bits */
	flSetWindowSpeed(vol.socket,120);  /* 120 nsec. */
	flSetWindowSize(vol.socket ,vol.chipSize >>12);			
        
	flFreeFlashMutex(vol.socket->serialNo);
       return flOK;
    }
    flFreeFlashMutex(vol.socket->serialNo);
    return flUnknownMedia; 	/* not ours */
 }

int bsp_flash_erase(int chipno, int firstErasableBlock, int numOfErasableBlocks)
{
	int	iBlock, status = flGeneralFailure ;
	DWORD i, BlockAddr;
	unsigned int base_addr = 0 ;
	WORD ReadBack;

	
	GET_FLASH_BASE_ADDR(chipno, base_addr) ;	
	
	flTakeFlashMutex(chipno , 1);		
	flash_write_enable(chipno) ;
	
	DrvReset();

	for (iBlock = 0; iBlock < numOfErasableBlocks && status == flOK; iBlock++) 
	{		
		BlockAddr =base_addr + (firstErasableBlock + iBlock) * ERASE_UNIT_SIZE ;
		
		REG8(BlockAddr) = SETUP_ERASE;
		REG8(BlockAddr) = CONFIRM_ERASE;
		
		for (i = 0; i < FLASH_WAIT_TIME; i++)
		{
			REG8(BlockAddr) = READ_STATUS;
			ReadBack = REG8(BlockAddr);
			if ((ReadBack & BIT_7) == BIT_7)
			{
				status = flOK;
				break;
			}
		}
		
		if(status != flOK)
			break;
	}

	DrvReset();

	flash_write_disable(chipno) ;
	flFreeFlashMutex(chipno);

	return status;
}
int	bsp_flash_write(int chipno, int address, char *buffer , int length)
{
	unsigned int base_addr  = 0 ;	
	DWORD SrcAddr, DstAddr, DstAddrOrg;
	DWORD ByteLen ,BufNo, BufOffset, ByteOffset;
	int Status = flOK ;

	if(address >= FLASH_SIZE || length > FLASH_SIZE || (address + length) > FLASH_SIZE) 
	{
		logMsg("Error : i28f640Write overflow address = 0x%x , length=%d \n"  ,address , length ,3,4,5,6);
		taskSuspend(0);
		return flBadParameter;
	}

	GET_FLASH_BASE_ADDR(chipno, base_addr) ;	
	
	flTakeFlashMutex(chipno , 1);		
	flash_write_enable(chipno) ;
		
		
	DstAddrOrg =base_addr+address ;

	DstAddr = DstAddrOrg;
	SrcAddr = (DWORD)buffer;
	ByteLen = length ;
	
	ByteOffset = 32-DstAddr%32; 
	DrvReset();	

	if(ByteLen<=ByteOffset)
	{
		/*can not exceed the erase unit, because the erase unit =128k*/
		Status = WriteBufferByte(DstAddr, SrcAddr,	ByteLen);  
	}
	else
	{
		Status = WriteBufferByte(DstAddr, SrcAddr,	ByteOffset);
		if(Status == flOK)
		{		 
			ByteLen =ByteLen -ByteOffset;
			SrcAddr = SrcAddr + ByteOffset;
			DstAddr = DstAddr + ByteOffset;  /*now DstAddr have the A4~A0 = 0;*/
			
			BufNo = ByteLen / 32;
			BufOffset = ByteLen % 32;
			
			for(; BufNo >0; BufNo--)
			{
				
				Status = WriteBufferByte(DstAddr, SrcAddr, 32);
				if(Status != flOK)
					break;
				
				SrcAddr = SrcAddr + 32;
				DstAddr = DstAddr + 32;
				
			}
			
			if(Status == flOK)
			{
				Status = WriteBufferByte(DstAddr, SrcAddr,	BufOffset);
			}
			
		}
	}

	DrvReset();
	
	flash_write_disable(chipno) ;	
	flFreeFlashMutex(chipno);
	return Status ; 
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

