#include <vxworks.h>
#include "bspCommon.h"
#include "proj_config.h"
#include "string.h"
#include "cacheLib.h"
#include "intLib.h"

#ifdef   BIOS_APP
#define PROG_GZ_READ_SIZE (1024*512)

//总共使用128个字节保存BIOS版本信息
#define SRAM_BIOS_VER_CODE_OFF SRAM_BIOS_VER_OFF
#define SRAM_BIOS_VER_CODE_SIZE 4
#define SRAM_BIOS_VER_NAME_OFF (SRAM_BIOS_VER_OFF+SRAM_BIOS_VER_CODE_SIZE)
#define SRAM_BIOS_VER_NAME_SIZE 32
#define SRAM_BIOS_VER_MEMO_OFF (SRAM_BIOS_VER_NAME_OFF+SRAM_BIOS_VER_NAME_SIZE)
#define SRAM_BIOS_VER_MEMO_SIZE 92

#define  VER_NR_BIOS (((VER_NR_BIOS_MAJOR)<<24) | ((VER_NR_BIOS_MINOR)<<16) | ((VER_NR_BIOS_MICRO)<<8) | (VER_NR_BIOS_DEBUG))
#define  VER_NR_BIOS_ID    BSP_DEV_NAME
#define  BSP_DEV_DATA	__DATE__","__TIME__
#define  VER_NR_BIOS_MEMO  BSP_DEV_NAME BSP_VERSION "BIOS software build at " BSP_DEV_DATA

static void bios_save_verInfo()
{
	unsigned long code = (VER_NR_BIOS);	
	strncpy((char *)(BIOS_VERSION_BASE_ADRS+SRAM_BIOS_VER_CODE_OFF), (char *)&code , 4);		
	strcpy((char *)(BIOS_VERSION_BASE_ADRS+SRAM_BIOS_VER_NAME_OFF), VER_NR_BIOS_ID) ;		
	strcpy((char *)(BIOS_VERSION_BASE_ADRS+SRAM_BIOS_VER_MEMO_OFF), VER_NR_BIOS_MEMO) ;		
} 

void bios_go_up(unsigned int ptr)
{
	int i  = 0 ;
	void (*pfn)() = (void (*)())ptr;
	
	cacheTextUpdate ((void *) (0),(size_t) (sysMemTop()));
	
	i = intLock();
	
	pfn();
	
	intUnlock(i);
}

int load_netring_img(char* file)
{
	/// first check netring_img.gz
	void * f = NULL;
	
	printf("\nCheck main software(%s) ...",file);
	f = gzopen(file,"rb");
	if (f)
	{
		char* a = (char*)VXWORKS_IMAGE_START_ADDR;
		int size , err ;
		printf("\nfind %s, now loading         ", file);
		while ((size = gzread(f,a,PROG_GZ_READ_SIZE)) > 0)
		{
			if (a >= (char*)0x01B00000)
			{
				printf("\nfind error, %s is error(too big > 27M)\r\n", file);
				break;
			}
			a += size;
			printf("\b\b\b\b\b\b\b\b%8d",(int)(a-VXWORKS_IMAGE_START_ADDR));
			taskDelay(1);
		}
		
		gzerror(f,&err);
		gzclose(f);
		if ((err == Z_STREAM_END||err == Z_OK) && ((int)a-VXWORKS_IMAGE_START_ADDR) >= 1024 && a < (char*)0x01B00000)
		{
			printf("\nnow jump ....\n");
			
			taskDelay(50);

			bios_go_up(VXWORKS_IMAGE_START_ADDR);
		}
		if (size < 0 || err != Z_STREAM_END)
		{
			printf("\n%s read error\n", file); 
		}
	}
	printf("...open file fail\n");
	return -1;
}

 char  SetCurActiveSys(char sysX);
 char WhichSysXActiveNext();
 int	bsp_ftp_get_file(char * ip , char* user, char *password , char * remote_full_name , char * local_full_name) ;
 int get_main_oam_ip(char * ip) ;


static int  bootTffsXProcess(char tffsX)
{
	char full_name[64] , remote_full_name[64] ,  ip[32];

	bsp_control_led(BSP_LED_FUN_DARK, BSP_LED_ALL);	
	if(tffsX == LFileSystem_sys1) //the first tffs system(always the first tffs in flash)
	{	
		
		/*点灯(点亮最低的绿灯表示当前加载状态)*/
		bsp_control_led(BSP_LED_FUN_LIGHT, BSP_DUAL_LED_GREEN);
		
		sprintf(full_name , "%s%s", BOOT_FILE_DIR1, g_CardImgName) ;		
	}
	else if(tffsX == LFileSystem_sys2) //the 2th tffs system
	{			
		/*点灯(点亮最低的绿灯表示当前加载状态)*/
		bsp_control_led(BSP_LED_FUN_LIGHT, BSP_DUAL_LED_YELLOW);

		sprintf(full_name , "%s%s", BOOT_FILE_UPGRADE_DIR, g_CardImgName) ;		
	}
	else if(tffsX == LFileSystem_remotesys2) //the 3th tffs system(tffs in oam , get by ftpClient)
	{	
		/*点灯(点亮最低的黄灯表示当前加载状态)*/		
		bsp_control_led(BSP_LED_FUN_LIGHT , BSP_DUAL_LED_YELLOW);
		memset(ip , 0 , 32) ;

		sprintf(remote_full_name , "%s%s", BOOT_FILE_DIR2, g_CardImgName) ; 
		sprintf(full_name , "%s%s", BOOT_FILE_DIR3, g_CardImgName) ;			
			
		if((get_main_oam_ip(ip) != OK) 
			|| (bsp_ftp_get_file(ip, "weihu" , "cjhyy300", full_name, remote_full_name) !=OK))
			return ERROR ;
	}
	else	
	{
		printf("ERROR:unsupport startup way %d in this linecard\n" , tffsX);
	 	return ERROR ;
	}

	SetCurActiveSys(tffsX) ;
	taskDelay(30) ;	
	return load_netring_img(full_name) ;			
}


void tBiosMain() 
{
	char startup_type ;	
	
	bios_save_verInfo(); /*save bios version info to reserved memory*/	
	startup_type = WhichSysXActiveNext();	
	
	if((startup_type != LFileSystem_sys1) && (bootTffsXProcess(startup_type) == ERROR))
		{
		taskDelay(sysClkRateGet()*1); /*delay 1 second*/
		bootTffsXProcess(LFileSystem_sys1) ;
		}
	else if((startup_type == LFileSystem_sys1) && (bootTffsXProcess(startup_type) == ERROR))
		{
		taskDelay(sysClkRateGet()*1); /*delay 1 second*/
		bootTffsXProcess(BACKUP_FILE_SYS) ;
		}		
				
    while(1)
		{
		taskDelay(sysClkRateGet()/2);
		bsp_control_led(BSP_LED_FUN_DARK , BSP_LED_ALL);
		bsp_control_led(BSP_LED_FUN_LIGHT , BSP_LED_RED);
		
		taskDelay(sysClkRateGet()/2);
		bsp_control_led(BSP_LED_FUN_DARK , BSP_LED_ALL);
		bsp_control_led(BSP_LED_FUN_LIGHT , BSP_DUAL_LED_GREEN);
		}	
}


extern int NSPsntpsClockHook(int request, void *pBuffer)
{
	return 0 ;
}
#endif /*BIOS_APP*/



