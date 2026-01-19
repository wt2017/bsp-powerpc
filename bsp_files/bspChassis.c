/* bspChassis.c - BSP chassic application interface */

/*
modification history
2007-01-1 ,xie  writen
*/
 
/******************************************************************************
*
* This module include all the API the bsp could	provide.
*
******************************************************************************/

#include <stdio.h>

#include "vxWorks.h"
#include "stat.h"
#include "iflib.h"
#include "drv/mem/m8260Siu.h"
#include "drv/sio/m8260CpmMux.h"
#include "drv/sio/m8260sio.h"
#include "taskLib.h"
#include "inetLib.h"

#include "tffs/tffsdrv.h"
#include "tffs/flflash.h"
#include "tffs/backgrnd.h"
#include "tffs/flbase.h"

#include "proj_config.h"
#include "bspCommon.h"
#include "drv/parallel/m8260IOPort.h"
#include "config.h"
#include "version.h"
#include "lld.h"
#include "arpLib.h"
#include "routeLib.h"
#include "hostLib.h"
#include "dosFsLib.h"
/******************************************************************************
*
* The driver interface description.
*
*/
char g_CardImgName[32] = "default.img" ;
extern int bsp_io_send_data(char chass, char *adrs, unsigned int len);
extern int bsp_io_send_data_cyclone(char chass, char *adrs, unsigned int len);

static unsigned long Table_CRC16_BIOS[256] =
{ 
  0x0   ,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,  
  0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,  
  0x1231,  0x210 ,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,  
  0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,  
  0x2462,  0x3443,  0x420 ,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,  
  0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,  
  0x3653,  0x2672,  0x1611,  0x630 ,  0x76d7,  0x66f6,  0x5695,  0x46b4,  
  0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,  
  0x48c4,  0x58e5,  0x6886,  0x78a7,  0x840 ,  0x1861,  0x2802,  0x3823,  
  0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,  
  0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0xa50 ,  0x3a33,  0x2a12,  
  0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,  
  0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0xc60 ,  0x1c41,  
  0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,  
  0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0xe70 ,  
  0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,  
  0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,  
  0x1080,  0xa1  ,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,  
  0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,  
  0x2b1 ,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,  
  0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,  
  0x34e2,  0x24c3,  0x14a0,  0x481 ,  0x7466,  0x6447,  0x5424,  0x4405,  
  0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,  
  0x26d3,  0x36f2,  0x691 ,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,  
  0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,  
  0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x8e1 ,  0x3882,  0x28a3,  
  0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,  
  0x4a75,  0x5a54,  0x6a37,  0x7a16,  0xaf1 ,  0x1ad0,  0x2ab3,  0x3a92,  
  0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,  
  0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0xcc1 ,  
  0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,  
  0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0xed1 ,  0x1ef0,  
} ;

// CRC-16 CRC-CCITT 
unsigned short CRC_16_BIOS( unsigned char * aData, unsigned long aSize ) 
{ 
    unsigned long i; 
    unsigned short nAccum = 0; 
    
    for ( i = 0; i < aSize; i++ ) 
        nAccum = ( unsigned short )(( nAccum << 8 ) ^ ( unsigned short )Table_CRC16_BIOS[( nAccum >> 8 ) ^ *aData++]); 
    return nAccum; 
} 

void myWelcome(void)
{
	printf ("%28s%s", "","VxWorks System Boot\n");
	printf ("\nCPU: %s\n", sysModel ());
	printf ("FLASH: %dM ,  SDRAM: %dM \n\n\n" , FLASH_SIZE/0x100000 , g_sysPhysMemSize/0x100000);	
	printf ("vxWorks Version: %s\n", vxWorksVersion);
	printf ("BSP Module version: " BSP_VERSION "\n");
	printf ("Creation date: %s\n\n", creationDate);
}

unsigned int bios_get_version_addr(void)
{
	return (unsigned int)(BIOS_VERSION_BASE_ADRS+SRAM_BIOS_VER_OFF) ;
}

static void WatchDogEntry()
{
	while (1)
	{
		bsp_clear_dog() ;	
		taskDelay(sysClkRateGet()/2);
	}
}
int bsp_init_watchdog()
{
	int id = taskSpawn("tHwDog",0,0,10*1024,(FUNCPTR)WatchDogEntry,0,0,0,0,0,0,0,0,0,0);
	if (id == ERROR)
		{
		printf("NOTIFY:hardware dog create error ...\n");
		return -1;
		}
	return 0 ;
}


void bsp_control_led(int ledctlfun, unsigned int led)
{
	unsigned int  *LedReg =(unsigned int *)BSP_LIGHT_REG ;	
	switch (ledctlfun)
	{
	case BSP_LED_FUN_LIGHT:
		*LedReg = (*LedReg) & (~(led & BSP_SINGLE_LIGHT_MASK)); 

		// control dual-light
		if((led & BSP_DUAL_LIGHT_MASK) == BSP_DUAL_LED_YELLOW)
			*LedReg = ((*LedReg)&(~BSP_DUAL_LIGHT_MASK))| BSP_DUAL_LED_YELLOW ;
		else if((led & BSP_DUAL_LIGHT_MASK) == BSP_DUAL_LED_GREEN)
			*LedReg = ((*LedReg)&(~BSP_DUAL_LIGHT_MASK))| BSP_DUAL_LED_GREEN;
		break;
	case BSP_LED_FUN_DARK:	
		*LedReg = (*LedReg) | (led & BSP_SINGLE_LIGHT_MASK) ;

		 // control dual-light
	   if((led & BSP_DUAL_LIGHT_MASK) == BSP_DUAL_LED_YELLOW)
			*LedReg = (*LedReg)&(~BSP_DUAL_LIGHT_MASK);
		else if((led & BSP_DUAL_LIGHT_MASK) == BSP_DUAL_LED_GREEN)
			*LedReg = (*LedReg)&(~BSP_DUAL_LIGHT_MASK);
		break;
	case BSP_LED_FUN_FLIP:	
		*LedReg = (*LedReg) ^ (led & BSP_SINGLE_LIGHT_MASK) ;

		 // control dual-light
		if((led & BSP_DUAL_LIGHT_MASK) == BSP_DUAL_LED_YELLOW)
			*LedReg = (*LedReg)^ BSP_DUAL_LED_YELLOW;
		else if((led & BSP_DUAL_LIGHT_MASK) == BSP_DUAL_LED_GREEN)
			*LedReg = (*LedReg)^ BSP_DUAL_LED_GREEN;
		break;
	default:
		break;
	}
	
}

/******************************************************************************
*
* The API interface	to calculate the physical address of every chassis.
*
*/

#if  defined(INCLUDE_IO_DOWNLOAD_FPGA)
/*****************************************************************************/

int	bsp_chass_set_fpga_prog(char chass,	char num)
{
	unsigned long adrs = 0;
	BSP_FPGA_CTRL_ID plctrl;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;

	if (bsp_chass_get_base_adrs(chass, &adrs) != 0)
		return -2;

	plctrl = (BSP_FPGA_CTRL_ID)adrs;
	BSP_SET_FPGA_PROG_DN(plctrl, num);
	taskDelay(2);
	BSP_SET_FPGA_PROG_UP(plctrl, num);



	return 0;
}
static void	bsp_error_ftp_download(int rs);

int	bsp_chass_set_fpga_clear(char chass,	char num)
{
	unsigned long adrs = 0;
	BSP_FPGA_CTRL_ID pbctrl;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;

	if (bsp_chass_get_base_adrs(chass, &adrs) != 0)
		return -2;

	pbctrl = (BSP_FPGA_CTRL_ID)adrs;
	BSP_SET_FPGA_PROG_DN(pbctrl, num);

	return 0;
}

int	bsp_chass_set_fpga_clke(char chass,	char num)
{
	unsigned long adrs = 0;
	BSP_FPGA_CTRL_ID pbctrl;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;

	if (bsp_chass_get_base_adrs(chass, &adrs) != 0)
		return -2;

	bsp_io_acs_pin_init(chass);
	taskDelay(1);

	pbctrl = (BSP_FPGA_CTRL_ID)adrs;
	BSP_SET_FPGA_CLKE(pbctrl, num);

	return 0;
}

int	bsp_chass_clr_fpga_clke(char chass,	char num)
{
	unsigned long adrs = 0;
	BSP_FPGA_CTRL_ID pbctrl;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;

	if (bsp_chass_get_base_adrs(chass, &adrs) != 0)
		return -2;

	pbctrl = (BSP_FPGA_CTRL_ID)adrs;
	BSP_CLR_FPGA_CLKE(pbctrl, num);

	return 0;
}

int	bsp_chass_get_fpga_down(char chass,	char num, char*	flag)
{
	unsigned long adrs = 0;
	BSP_FPGA_CTRL_ID pbctrl;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;

	if (bsp_chass_get_base_adrs(chass, &adrs) != 0)
		return -2;

	*flag = 0;
	pbctrl = (BSP_FPGA_CTRL_ID)adrs;
	if (BSP_GET_FPGA_DONE(pbctrl, num))
		*flag = 1;
	return 0;
}


/******************************************************************************
*
* FPGA download	through	io pin.
*
*/
#define BSP_FPGA_DOWNLOAD_SIZE	0x600000
/*****************************************************************************/
int	bsp_io_download(int	chass_id, int num, char* p_data, int len)
{
	int get_down_times;
	char flag = 0;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;
		
	if ((p_data == 0) || (len == 0))
		return -2;

	bsp_chass_set_fpga_prog(chass_id, num);
	bsp_chass_get_fpga_down(chass_id, num, &flag);
	taskDelay(1);
	if (flag == 1)
		return -4;

	bsp_chass_set_fpga_clke(chass_id, num);
	bsp_io_send_data(chass_id,p_data, len);
	bsp_chass_clr_fpga_clke(chass_id, num);
	
	for (get_down_times = 0; get_down_times	< FPGA_GET_DOWN_TIMES; get_down_times++)
	{
		taskDelay(10);
		bsp_chass_get_fpga_down(chass_id, num, &flag);
		if (flag !=	0)
			break;
	}
	if (get_down_times == FPGA_GET_DOWN_TIMES)
		return -10;

	return 0;
}
/*****************************************************************************/
int	bsp_io_download_cyclone(int chass_id, int num, char* p_data, int len)
{
	int get_down_times;
	char flag = 0;

	if (!BSP_CHK_FPGA_NUM_OK(num))
		return -1;
		
	if ((p_data == 0) || (len == 0))
		return -2;

	bsp_chass_set_fpga_prog(chass_id, num);
	bsp_chass_get_fpga_down(chass_id, num, &flag);
	taskDelay(1);
	if (flag == 1)
		return -4;

	bsp_chass_set_fpga_clke(chass_id, num);
	bsp_io_send_data_cyclone(chass_id,p_data, len);
	bsp_chass_clr_fpga_clke(chass_id, num);
	
	for (get_down_times = 0; get_down_times < FPGA_GET_DOWN_TIMES; get_down_times++)
	{
		taskDelay(10);
		bsp_chass_get_fpga_down(chass_id, num, &flag);
		if (flag !=	0)
			break;
	}
	if (get_down_times == FPGA_GET_DOWN_TIMES)
		return -10;

	return 0;
}


void bsp_fpga_io_dl_err(int rs)
{
	switch (rs)
	{
	case -1:
		printf("the fpga number you input is wrong, check it again. it couldn't be large than 8.\n");
		break;
	case -2:
		printf("software couldn't allocate the memory which is needed by fpga downloading routine, or the file length is equal to zero.\n");
		break;
	case -3:
		printf("the chassis number you input is wrong, it just may be 1 or 2.\n");
		break;
	case -4:
		printf("the fpga you want download is not exist!, pls check again.\n");
		break;
	case -10:
		printf("sorry, the 'done' signal is still negated.\n");
		break;
	default:
		printf("congratulations! you meet the problem that is unexpected to the designer! God bless you!\n");
		break;
	}

	printf("\n");
}


int	bsp_fpga_io_dl(char* ip_or_fullName, char* p_name, int chass_id, int num)
{
	char * p_data = NULL;
	int	rs, status = OK , len = 0;

	if((p_data = (char *)malloc(BSP_FPGA_DOWNLOAD_SIZE * (sizeof(char)))) == NULL)
	{
		printf("Error : malloc memory(size = 0x%x) fail \n" , (BSP_FPGA_DOWNLOAD_SIZE * (sizeof(char)))) ;
		return 	-1;
	}

	if(ip_or_fullName[0] == '/')  /*file's full name */
	{	
		rs = bsp_fs_download(ip_or_fullName, p_data, BSP_FPGA_DOWNLOAD_SIZE, &len);
		if (rs != OK)
		{
			printf("\nfile %s read error, maybe file do not existed\n", ip_or_fullName);
			status = -2;
		}			
	}
	else						/*get file from ftp server*/
	{
		rs = bsp_ftp_download_from(ip_or_fullName,"weihu", "cjhyy300","." , p_name, p_data, BSP_FPGA_DOWNLOAD_SIZE, &len);
		if (rs != OK)
		{
			bsp_error_ftp_download(rs);
			status = -3;
		}
	}

	if(rs == OK )
	{
		printf("\nThe file length of FPGA is %d bytes.\n", len);

		rs = bsp_io_download(chass_id, num, p_data, len);
		if (rs != 0)
		{
			printf("\n1.Sorry, You are failed! Try again.\n");
			bsp_fpga_io_dl_err(rs);
			status = -2;
		}
		else
		{
			printf("\nMission Complished!\n");
			status = 0;
		}
	}

	free(p_data);

	return status;
}

int	bsp_fpga_io_dl_cyclone(char* ip_or_fullName, char* p_name, int chass_id, int num)
{
	char * p_data = NULL;
	int	rs, status = OK , len = 0;

	if((p_data = (char *)malloc(BSP_FPGA_DOWNLOAD_SIZE * (sizeof(char)))) == NULL)
	{
		printf("Error : malloc memory(size = 0x%x) fail \n" , (BSP_FPGA_DOWNLOAD_SIZE * (sizeof(char))));
		return 	-1;
	}

	if(ip_or_fullName[0] == '/')  /*file's full name */
	{	
		rs = bsp_fs_download(ip_or_fullName, p_data, BSP_FPGA_DOWNLOAD_SIZE, &len);
		if (rs != OK)
		{
			printf("\nfile %s read error, maybe file do not existed\n", ip_or_fullName);
			status = -2;
		}			
	}
	else						/*get file from ftp server*/
	{
		rs = bsp_ftp_download_from(ip_or_fullName,"weihu", "cjhyy300","." , p_name, p_data, BSP_FPGA_DOWNLOAD_SIZE, &len);
		if (rs != OK)
		{
			bsp_error_ftp_download(rs);
			status = -3;
		}
	}
	
	if(rs == OK)
	{
		printf("\nThe file length of FPGA is %d bytes.\n", len);

		rs = bsp_io_download_cyclone(chass_id, num, p_data, len);
		if (rs != 0)
		{
			printf("\nSorry, You are failed! Try again.\n");
			bsp_fpga_io_dl_err(rs);
			status = -2;
		}
		else
		{
			printf("\nMission Complished!\n");
			status = 0;
		}
	}

	free(p_data);

	return status;
}
#endif  /*defined(INCLUDE_IO_DOWNLOAD_FPGA)*/

/****************************************************************************/
#define _INCLUDE_BIOS_UPGRADE_
#if defined(_INCLUDE_BIOS_UPGRADE_)
int	bsp_flash_erase_addr(int chipno,int addr, int len)
{
	int	first, num;

	first = addr / FLASH_BLOCK_SIZE ;
	num	=  (len / FLASH_BLOCK_SIZE) + ((len % FLASH_BLOCK_SIZE)? 1 : 0 );
    printf("\r\nfirst = 0x%x, num = 0x%x", first, num);
	if (bsp_flash_erase(chipno,first, num)	!= 0)
		return -2;
	return 0;
}

static int bsp_write_img_to_boot_adrs(char* p_data, int len, unsigned int adrs)
{
	int	rs, status = OK ;
//  unsigned int  adrs = FLASH_BIOS_BOOT_OFF ;
	char *pBuf = NULL ;

	if(len > 0x100000)
	{
		printf("\nERROR:the file you want download to the flash is 0x%x bytes(must less than 0x10000),----too big.\n", len);
		return -1 ;
	}
		
	printf("\nthe length of the file you want download to the flash is %d bytes.\n", len);
	rs = bsp_flash_erase_addr(0 , adrs , len);
	if (rs != 0)
	{
		printf("\nFLASH is done. You have made a real big trouble! Call the hardware designer.0x%x\n", rs);
		status = -3;
	}
	else
	{
		printf("\nthe relative region of FLASH is clear.\n");
		rs = bsp_flash_write(0 , adrs  , p_data,  len) ;				
		if (rs !=	OK)
		{
			printf("\nYour writing is failed! Why? i don't known!! help!!!0x%x\n", rs);
			status = -4;
		}
		else
		{
			printf("\nNow, writing is over.\n");	
			pBuf = (char *)malloc(len) ;
			bsp_flash_read(0 , adrs , pBuf,  len) ;
			if (memcmp(pBuf , p_data, len) != 0)
			{
				status = -5;
				printf("\nError: write to flash not equal to read from it.\n");
			}
			else
			{
				status = 0;
				printf("\nbios has been write to the flash <0x%08x> successfully.\n", adrs);
			}
			free(pBuf) ;
		}
	}
	return status ;
}

int	bsp_image_download(char* ip_or_fullName, char * name, unsigned int adrs)
{
	char * p_data =	0;
	int	rs, status = OK , len = 0;

	if((p_data = (char *)malloc(MAX_BIN_SIZE * (sizeof(char)))) == NULL)
	{
		printf("Error : malloc memory(size = 0x%x) fail \n" , (MAX_BIN_SIZE * (sizeof(char)))) ;
		return 	-1;
	}

	if(ip_or_fullName[0] == '/')  /*file's full name */
	{	
		rs = bsp_fs_download(ip_or_fullName, p_data, MAX_BIN_SIZE, &len);
		if (rs != OK)
		{
			printf("\nfile %s read error, maybe file do not existed\n", ip_or_fullName);
			status = -2;
		}			
	}
	else						/*get file from ftp server*/
	{
		rs = bsp_ftp_download_from(ip_or_fullName,"weihu", "cjhyy300","." , name, p_data, MAX_BIN_SIZE, &len);
		if (rs != OK)
		{
			bsp_error_ftp_download(rs);
			status = -3;
		}
	}
	if(rs == OK)
		status = bsp_write_img_to_boot_adrs(p_data, len, adrs) ;
	
	free(p_data);
	return status;
}

int	bsp_bios_download(char * ip_or_fullName, char * name)
{
	return  bsp_image_download(ip_or_fullName, name , (unsigned int)FLASH_BIOS_BOOT_OFF);
}

int	bsp_flash_bios_download(char * name)
{
	return bsp_image_download(name , NULL , (unsigned int)FLASH_BIOS_BOOT_OFF );
}

int g_Bsprebootdebug = FALSE;
int	bsp_bootrom_download(char * ip_or_fullName, char * name, unsigned int addr)
{
    if(TRUE == g_Bsprebootdebug)
    {
    	return  bsp_image_download(ip_or_fullName, name , addr);
    }
    return -1;
}

int	bsp_flash_bootrom_download(char * name, unsigned int addr)
{
    if(TRUE == g_Bsprebootdebug)
    {
        return bsp_image_download(name , NULL , addr );
    }        
    return -1;
}

#endif /*_INCLUDE_BIOS_UPGRADE_*/

#if defined(_INCLUDE_IMG_UPGRADE_BY_NVRAM_)

#define  BSP_CRC_OFF               	BSP_BOOT_FLAG_OFF
#define  BSP_ACTIVE_STARTUP_OFF 	(BSP_CRC_OFF + 1)
#define  BSP_CUR_STARTUP_OFF 		(BSP_CRC_OFF + 2)
#define  BSP_CNT_OFF    			(BSP_CRC_OFF + 3)

static char CRC_8(char offset, char aSize)
{ 
	char nAccum = 0xa5;  
	int i ;
	for (i = 0; i < aSize; i++) 
		nAccum = (nAccum ^ ((char)*NVRAM_ADDR(offset+i))); 
	return nAccum; 
}
static void updateCrc()
{
	*NVRAM_ADDR(BSP_CRC_OFF) = CRC_8(BSP_ACTIVE_STARTUP_OFF,2);
}

static BOOL IsCRCOK()
{
	if(((char)*NVRAM_ADDR(BSP_CRC_OFF)) == CRC_8(BSP_ACTIVE_STARTUP_OFF,2))
		return TRUE;
	else
	{
//		SetFirstActiveSys(LFileSystem_sys1);
		return FALSE;
	}	
}

char  GetSysXCountNum(char sysX)
{
	return (char) *NVRAM_ADDR(sysX + BSP_CNT_OFF -1);
	return 0;
}

char  SetSysXCountNum(char sysX , char num)
{
	*NVRAM_ADDR(sysX + BSP_CNT_OFF -1) = num ;	
	return OK;
}

char SetFirstActiveSys(char sysX , char *img_name)
{
	*NVRAM_ADDR(BSP_ACTIVE_STARTUP_OFF) = sysX;
	*NVRAM_ADDR(BSP_CNT_OFF + sysX - 1) = 0;	
	updateCrc();
	return OK;
}

char  SetCurActiveSys(char sysX)
{
	*NVRAM_ADDR(BSP_CUR_STARTUP_OFF) = sysX;
	updateCrc();
	return OK;
}

char WhichSysXActiveNext()
{
	if (IsCRCOK())
	{
		return (char) *NVRAM_ADDR(BSP_ACTIVE_STARTUP_OFF) ;			
	}
	return LFileSystem_sys1;
}


char WhichSysXActiveNow()
{
	if (IsCRCOK())
	{
		return (char) *NVRAM_ADDR(BSP_CUR_STARTUP_OFF);		
	}
	return LFileSystem_sys1;
}
#endif /*defined(_INCLUDE_IMG_UPGRADE_BY_NVRAM_)*/

#if defined(_INCLUDE_IMG_UPGRADE_BY_FLASH_)
#define STRING_BUFFER_SIZE 		64 
static int bsp_startup_parse 
    (
    char *  startupString      /* parameter string */
    )
    
{
	char *  tok , *holder = NULL;        /* an initString token */
	int starup_type = LFileSystem_sys1 ;
	
	if(strstr(startupString,"local=") != NULL)
		starup_type = LFileSystem_sys1 ; /*locla*/
	else if(strstr(startupString,"local_update=") != NULL)
		starup_type = LFileSystem_sys2 ; /*local ,for update*/
	else
		return starup_type ;

	tok = strtok_r (startupString, "[", &holder);
	if (tok == NULL) 
		return starup_type ;		
	tok = strtok_r (NULL, "]", &holder);
	if (tok == NULL) 
		return starup_type ;
	
	memset(g_CardImgName, 0 ,16) ;
	strncpy(g_CardImgName , tok ,(UINT32)(holder-tok-1));
	return starup_type ;
}

char WhichSysXActiveNext()
{
	FILE * fp = NULL ;
	char buffer[STRING_BUFFER_SIZE] ;
	int startupType = LFileSystem_sys1 ;
	
	fp = fopen("/tffs/startup.txt" , "r");
	if(fp)
	{
		if(fgets(buffer , STRING_BUFFER_SIZE - 1 , fp) != NULL) /*not the end-of-file*/
			startupType = bsp_startup_parse(buffer);	 	
		fclose(fp);
	}
	return startupType;
}

char WhichSysXActiveNow()
{
	return *((char*)(BIOS_VERSION_BASE_ADRS + BSP_BOOT_FLAG_OFF));	
}

char  SetCurActiveSys(char sysX)
{
	*((char*)(BIOS_VERSION_BASE_ADRS + BSP_BOOT_FLAG_OFF)) = sysX ;
	return OK ;
}

char SetFirstActiveSys(char sysX , char *appName)
{

	FILE * fp = NULL ;
	char buffer[STRING_BUFFER_SIZE] ;
	if((sysX != LFileSystem_sys1) && (sysX  != LFileSystem_sys2))
		{
		printf("Error: param1 must be correct startType(%d) %d or %d\n" , sysX ,LFileSystem_sys1 , LFileSystem_sys2);
		return ERROR ;
		}
	if(appName == NULL || strlen(appName) > 16)
		{		
		printf("Error: param2 must be correct file_name(%s)\n" , appName);
		return ERROR ;
		}	
	
	fp = fopen("/tffs/startup.txt" , "w");
	if(fp)
	{
		if(sysX == LFileSystem_sys1)
			{
			sprintf(buffer , "local=[%s]" ,appName);
			}
		else if(sysX == LFileSystem_sys2)
			{
			sprintf(buffer , "local_update=[%s]" ,appName);
			}
		fwrite(buffer , 1 , strlen(buffer), fp);
		fclose(fp);
		return OK ;
	}
	else
		return ERROR ;
}

#endif /*defined(_INCLUDE_IMG_UPGRADE_BY_FLASH_)*/

#if defined(_INCLUDE_IIC_)
/*IIC 器件调试接口----bios 下调试各IIC硬件*/ 

#define 	I2C_MAX_BUF_LEN  	128  
/*why?-- 用户自定义之，因为各I2C 器件的寄存器地址空间
温度芯片 0~4
Inventory 0~255
光模块 0~127
其它....都小于127
so .... 定义为128,
注意:不可随意变大此值，它与各CPU的I2C驱动定义的最大缓冲相关；
*/

/***********************************************************************/
int  i2cRead(unsigned char device_addr , unsigned char offset ,  unsigned char length)
{
	unsigned char buffer[I2C_MAX_BUF_LEN] ;	
	unsigned char device_select = 0x90 , i ; 
	int status =  0 ; 

	memset(buffer , 0xff , I2C_MAX_BUF_LEN) ;

	if(device_addr)
		device_select = device_addr ;
	if(length <= 1 || length >= I2C_MAX_BUF_LEN)
		length = 1 ;

	buffer[0] = offset ;
	status = bsp_i2c_write(device_select , 1 , buffer , FALSE , TRUE) ;
	if(status)
	{
		printf("Error : bsp_i2c_write offset(%d) to i2c fail ,status = %d\n" , offset , status) ;
		return status;
	}
	taskDelay(2) ;
	buffer[0] = 0xff ;
	
	status = bsp_i2c_read(device_select , length  , buffer , FALSE , TRUE) ;	
	if(status)
	{
		printf("Error : bsp_i2c_read from i2c fail ,status = %d\n" , status);
		return status;
	}

	printf("device_addr(0x%x),offset(0x%x) value=: \n" ,device_addr , offset);
	for(i = 0 ; i < length ; i ++)
	{	
		printf("0x%02x " ,buffer[i]);
		if(((i+1)%8) == 0)  printf("\n");
	}
	printf("\n");

	return status;
}
	
/*************************************************************************/
int i2cWrite(unsigned char device_addr ,unsigned char offset, char *val ,unsigned char length)
{ 
	unsigned char buffer[I2C_MAX_BUF_LEN] ;
	unsigned char device_select = 0x90 ; 
	int status =  0 ; 
	
	memset(buffer , 0xff , I2C_MAX_BUF_LEN) ;

	if(device_addr)
		device_select = device_addr ;
	if(length <= 1 || length >= (I2C_MAX_BUF_LEN-1) )
		length = 1 ;
	
	buffer[0] = offset ;
	
	memcpy((char *)(&(buffer[1])) , (char *)val , length) ; 
	status = bsp_i2c_write(device_select , (length + 1) , buffer , FALSE , TRUE ) ;
	if(status)
	{
		printf("Error : bsp_i2c_write offset(%d) to i2c fail ,status = %d\n" , offset , status) ;
	}	
	taskDelay(2) ;
	return status;
}
void i2cDevShow()
{
	unsigned char i , buffer[4] ;
	int status ;
	
	printf("I2C-Dev -Address ,	First-Bye-value\n");
	for(i = 0 ; i < 0x7f ; i++)
	{
		buffer[0] = (i << 1) |0x1 ; 
		buffer[1] = 0 ;
		status = bsp_i2c_output(buffer, 2) ;

		if(status  ==  0)
			printf("\t0x%02x	   \t0x%02x \n" , (i << 1) , buffer[1]) ;				
	}
		
}
#endif /* defined(_INCLUDE_IIC_)*/

/*****************************************************************************/
#define	__BSP_CHASS_API_FUNNY__

static void	bsp_error_ftp_download(int rs)
{
	int i = 0;
	
	printf("\n\ni could not find the file you want.\n");
	printf("pls check the item following:\n");
	printf("1) is the connection of the net OK?\n");
	printf("2) are you really launch the ftp server?\n");
	printf("3) is the user name and the password of the server correct?\n");
	printf("4) is the directory of the ftp server correct?\n");
	printf("5) do you really confirm that the file you want is exist?\n");
	printf("6) your 'bit' file length is exceed 6000000 bytes.\n");
	
	for	(i = 0;	i <	32;	i++)
	{
		printf(".");
		taskDelay(10);
	}
	printf("\n0x%x\n\n", rs);
}

void bsp_help(void)
{
	printf("\n***************************************************************\n");
	printf("ip控制接口:	\n");

	printf("\nint bsp_ifadrs_set_ip(char* ip); \n");	
	printf("修改外网口的ip地址,子网掩码和MAC地址\n"); 
	printf("ip:ip的地址,格式是XXX.XXX:XXX:XXX ,子网掩码是0xFFFFFF00\n"); 
	printf("Mac 地址跟随IP 变化, 是08: IP 地址四个字节:01\n"); 

	printf("\nint bsp_ifadrs_set_ip_all(char* ip, char* mask, char* mac); \n");	
	printf("修改外网口的ip地址,子网掩码和MAC地址\n"); 
	printf("ip:ip的地址,格式是XXX.XXX.XXX.XXX , 不能为NULL\n"); 
	printf("mask:ip的子网掩码, 格式是XXX.XXX.XXX.XXX , 不能为NULL\n"); 
	printf("mac:mac的地址,格式是XX:XX:XX:XX:XX:XX , 不能为NULL\n"); 
	printf("更改IP 或者MAC之后必须重起单板才有效.\n");
	
	printf("\n***************************************************************\n");
	printf("bios和fpga下载接口 : \n");
	
	printf("\nint bsp_bios_download(char * ip_or_fullname,char * name);\n");
	printf("使用这个接口烧写BIOS 文件到BIOS区.\n");
	printf("参数1表示FTP SERVER 的ip 地址, 或是本地BIOS  文件full_name (此时第2个参数可以为任意值)\n");
	printf("参数2表示BIOS的名字.注意：FTP SERVER与主控板的网段及掩码必须一致.\n");
	printf("\nint bsp_flash_bios_download(char * full_name);\n");
	printf("这个接口把本地BIOS 文件烧写到BIOS区.\n");
	printf("参数1: full_name  BIOS 文件的路径+ 文件名\n");
	
	printf("\nint bsp_fpga_io_dl(char * ip,char * p_name,int chass_id,int num);\n");
	printf("int bsp_fpga_io_dl_cyclone(char* ip, char* p_name, int chass_id, int num);\n");
	printf("这两个接口使用io管脚进行FPGA加载,速度为460K bps.\n");
	printf("参数1表示远程FTP IP 地址, 或是本地FPGA 文件full_name (此时第2个参数可以为任意值)\n");
	printf("本地加载可gzip压缩格式或非压缩格式(注意:远程加载不支持压缩格式)\n");
	printf("参数2表示FPGA名字,参数3表示板位号,参数4表示第几个FPGA.\n");

	printf("\n***************************************************************\n");
	printf("flash格式化和删除接口 : \n");
	
	printf("\n步骤是:1.删除文件系统的引导扇区所在的块; 2.flash 格式化;\n");	
	printf("3.重启单板文件系统创建成功; 本板操作如下:\n\n");	
	printf("1.用bsp_flash_erase(0, 8, 1) 删除TFFS 引导扇区的块;2.用sysTffsFormat(0)  格式化第1 片FLASH\n");	
	printf("然后复位本板,查看启动时的文件系统创建信息.\n");

	printf("\nint bsp_flash_erase(int chipno,int first, int num);\n");
	printf("这个接口用来删除FLASH块上的数据.\n");
	printf("参数1表示格式化哪部分FLASH, 0 表示第一片flash \n");
	printf("参数2表示要删除的起始块,0 表示第一块,当前flash每块是128K.\n");
	printf("参数3表示要删除块的数目.\n\n");
	
	printf("\n***************************************************************\n");
	printf("在线加载CPLD 接口: \n");
	printf("\nint bsp_jtag_download(char *fileName);\n");
	printf("这个接口把本地CLPD 逻辑加载到CPLD 中\n");
	printf("full_name : CPLD vme 格式文件的路径+ 文件名\n");
	printf("\n***************************************************************\n");

}

/*刷新motfcc1 接口的MAC地址，并广播出去*/
int bsp_arp_broadcast(char*  ifname) 
{
	char ipadrr[INET_ADDR_LEN];
	int loop;
	struct in_addr ip ;
	struct arpcom* ac = NULL ;
			
	if (OK != ifAddrGet(ifname, ipadrr))
	{
		return  ERROR ;
	}
	
	inet_aton(ipadrr , &ip);
	arpFlush();

	/* broadcast my ip 2 times */
	ac = (struct arpcom*)ifunit(ifname);
	
	for(loop = 0; loop < 10; ++loop)
	{
#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)
		arprequest(ac, &ip , &ip, ac->ac_enaddr);
#else
		arpwhohas(ac,&ip);
#endif
		taskDelay(50);
	}
	return OK;
}


int  sys_rename_ipaddress (char *pNetDevName, int DevNum,char *pNewDevAddr, unsigned int  SubNetMask)
	{
	char OldNetDevAddrs [INET_ADDR_LEN ] ;
//	char NetWorkAddrs [INET_ADDR_LEN ];
	char ifname [20],targetName[20];
	STATUS hostOK = ERROR , gatewayFlag = ERROR ;
	unsigned int netMask ;

	/* build interface name */
	sprintf (ifname, "%s%d", pNetDevName, DevNum);

	/* get the old ipAddress & check if the interface attach */
	if (ifAddrGet(ifname,OldNetDevAddrs)!=OK)
	{
		return ERROR ;
	}
	/*get the old mask of ipAddress*/
	ifMaskGet(ifname , &netMask);
	
	/*set if down status*/
	ifFlagChange(ifname , IFF_UP  , FALSE);
	
	/* flush arp table */
	arpFlush();
	
	/* delete any relevant arp to NetDev */
	arpDelete(OldNetDevAddrs);
	
	/* delete the Host from the Host table */
	if ((hostOK=hostGetByAddr(inet_addr(OldNetDevAddrs),targetName))==OK)
		{
		hostDelete(targetName,OldNetDevAddrs) ;
		}
	
	/* detach the interface */
	//ipDetach(DevNum,pNetDevName);

	/* delete any route to the interface */
//	ifRouteDelete(pNetDevName,DevNum);
	
	/* delete any route to the interface */
	mRouteDelete(OldNetDevAddrs , netMask , 0 , 0);	
	/*routeDelete(NetWorkAddrs,OldNetDevAddrs);*/
		
	/* check if the ip address is gateway, then delete gateway route,else do nothing  */
	//gatewayFlag = mRouteDelete("0.0.0.0" , 0x0 , 0 , 0) ; /*mask = 0x0*/
	gatewayFlag = routeDelete("0.0.0.0",OldNetDevAddrs) ;          

	/* attach the interface */
	//ipAttach(DevNum,pNetDevName) ;
	
	/* set the new IP address mask */
	ifMaskSet(ifname, SubNetMask);

	/* set the new IP address */
	ifAddrSet(ifname, pNewDevAddr);

	/* add host name to host table */
	if (hostOK==OK)
		{
		hostAdd (targetName, pNewDevAddr) ;
		}

    /* check if have delete the gateway, then add the the interface as gateway ,else do nothing  */
	if(gatewayFlag == OK)
		{
		routeNetAdd("0.0.0.0", pNewDevAddr);
		}

	ifFlagChange(ifname , IFF_UP  , TRUE);
	return(OK);
}
int	bsp_ramdisk_init(unsigned int ramdiskAddr , unsigned int ramdiskSize)
{
	BLK_DEV *dev ;
/*	FILE *fp = NULL;*/

	if(ramdiskAddr == 0 || ramdiskSize == 0) 
	{
		ramdiskSize = 0x500000 ;
		ramdiskAddr =(unsigned int)(malloc(ramdiskSize)) ;
	}

	memset((void *)ramdiskAddr , 0 , ramdiskSize) ;	
	/*创建ramdisk系统*/
#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)
	dev = ramDevCreate((char *)ramdiskAddr, 512, 32, ramdiskSize/512, 0) ;
	xbdBlkDevCreate(dev , "/ramdisk");
	taskDelay(1);
#else	
	dev = (BLK_DEV *)ramDevCreate((char *)ramdiskAddr,512,ramdiskSize/512,ramdiskSize/512,0) ;
	dosFsDevCreate("/ramdisk:0",(CBIO_DEV_ID)dev , 100,0);
#endif	
//	dosFsVolFormat("/ramdisk:0",DOS_OPT_DEFAULT ,NULL);

	return   OK;
}

