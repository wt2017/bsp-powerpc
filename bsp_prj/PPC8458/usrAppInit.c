/* usrAppInit.c - stub application initialization routine */

/* Copyright (c) 1998,2006 Wind River Systems, Inc. 
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,16mar06,jmt  Add header file to find USER_APPL_INIT define
01a,02jun98,ms   written
*/

/*
DESCRIPTION
Initialize user application code.
*/ 

#include <vxWorks.h>
#include <stdio.h>
#include "tasklibcommon.h"
#include "iflib.h"
#include "intlib.h"
#include "syslib.h"
#include "muxlib.h"
#include "cachelib.h"
#include "ipproto.h"
#include "proj_config.h"
#include "timers.h"
#include "inetlib.h"
#include "usrFsLib.h"
#if defined(PRJ_BUILD)
#include "prjParams.h"
#endif /* defined PRJ_BUILD */


extern void bsp_led_task();
extern void BspDeviceTest();
extern void BspSpiInit();
extern void BspI2cInit();
extern int bsp_init_watchdog();
extern STATUS ftpdInit(FUNCPTR pLoginFunc, int stackSize);
extern void flRegisterComponents (void);
extern void tffsShow (int driveNo);
extern STATUS usrTffsConfig(int drive,int removable, char *fileName);
extern void tBiosMain() ;
extern void testmain();

#if 1
void bspgo
    (
    FUNCPTR entry
    )
    {
    printf ("Starting at 0x%x...\n\n", (int) entry);

    taskDelay (sysClkRateGet ());   /* give the network a moment to close */

#ifdef INCLUDE_END
    /* Stop all ENDs to restore to known state for interrupts and DMA */

    (void) muxDevStopAll (0);
#endif  /* INCLUDE_END */


#if (CPU_FAMILY == PPC)
    cacheTextUpdate ((void *) (LOCAL_MEM_LOCAL_ADRS),   /* cache coherency */
             (size_t) (sysMemTop() - LOCAL_MEM_LOCAL_ADRS));
#else
#ifdef  INCLUDE_CACHE_SUPPORT
    cacheClear (DATA_CACHE, NULL, ENTIRE_CACHE);    /* push cache to mem */
#endif  /* INCLUDE_CACHE_SUPPORT */
#endif  /* (CPU_FAMILY == PPC) */

#if (CPU_FAMILY == I80X86)
    sysClkDisable ();           /* disable the system clock interrupt */
    sysIntLock ();          /* lock the used/owned interrupts */
#   if defined (SYMMETRIC_IO_MODE) || defined (VIRTUAL_WIRE_MODE)
    {
    extern void loApicEnable ();
    loApicEnable (FALSE);       /* disable the LOAPIC */
    }
#       if defined (SYMMETRIC_IO_MODE)
    {
    extern BOOL sysBp;          /* TRUE for BP, FALSE for AP */
    extern void ioApicEnable ();
    if (sysBp)
        ioApicEnable (FALSE);       /* disable the IO APIC */
    }
#       endif /* (SYMMETRIC_IO_MODE) */
#   endif /* (SYMMETRIC_IO_MODE) || (VIRTUAL_WIRE_MODE) */
#endif  /* (CPU_FAMILY == I80X86) */

    /* Lock interrupts before jumping out of boot image. The interrupts
     * enabled during system initialization in sysHwInit()
     */

    intLock();

    (entry) (2);     /* go to entry point - never to return */
    }
#endif


#define TN725_OAM_APP
#ifdef TN725_OAM_APP

#define SWITCH_BCM532X_DRVIER
#ifdef  SWITCH_BCM532X_DRVIER


STATUS Read532x(char page, char reg, char* data, int len) ;
STATUS Write532x(char page, char reg, char* data, int len);
int bsp_bcm532x_read(char pageAddr, char regAddr, char* data ,int len) ;

#define 	  BCM5324_PORT_NUM   26
#define 	PORT_BASED_VLAN_PAGE  	0x31
#define 	PORT_REG_OFFSET(num)	  (num*4) 

/*初始化725 OAM单板的VLAN*/
void _init_tn725_oam_vlan( ) 
{	
	/*see more BCM532x information  in <<532xM-DS00-RDS.pdf>>*/
	unsigned int   i  ,temp, temp1;
	char buffer[16] ;

#define IN_VLAN_725   0x00FFFFFE
#define OUT_VLAN_725   0x01000001

	for(i = 0 ; i < BCM5324_PORT_NUM ; i ++ ) 
	{
	
		temp = 0x1 << i ;
		if(temp & IN_VLAN_725)
			temp1 = IN_VLAN_725 ;
		else if(temp & OUT_VLAN_725)
			temp1 = OUT_VLAN_725;				
		else
			continue ;
		
		buffer[0] = (char)(temp1 & 0x000000FF);
		buffer[1] = (char)((temp1 & 0x0000FF00) >> 8);
		buffer[2] = (char)((temp1 & 0x00FF0000) >> 16);
		buffer[3] = (char)((temp1 & 0xFF000000) >> 24);
		Write532x(PORT_BASED_VLAN_PAGE, PORT_REG_OFFSET(i), buffer, 4) ;  
	}
}


/*初始化725 OAM单板的VLAN*/
void _init_tn705_oam_vlan( ) 
{	
	/*see more BCM532x information  in <<532xM-DS00-RDS.pdf>>*/
	unsigned int  i  ,temp , temp1 ;
	char buffer[16] ;

#define IN_VLAN_705     0x0000FFEF
#define OUT_VLAN_705   0x01000010

	for(i = 0 ; i < BCM5324_PORT_NUM ; i ++ ) 
	{
		temp = 0x1 << i ;
		if(temp & IN_VLAN_705)
			temp1 = IN_VLAN_705 ;
		else if(temp & OUT_VLAN_705)
			temp1 = OUT_VLAN_705;				
		else
			continue ;		

		buffer[0] = (char)(temp1 & 0x000000FF);
		buffer[1] = (char)((temp1 & 0x0000FF00) >> 8);
		buffer[2] = (char)((temp1 & 0x00FF0000) >> 16);
		buffer[3] = (char)((temp1 & 0xFF000000) >> 24);		
		Write532x(PORT_BASED_VLAN_PAGE, PORT_REG_OFFSET(i), buffer, 4) ;  
	}
}


#endif  	/*SWITCH_BCM532X_DRVIER*/


#define NRRAM_DS3065_DRIVER
#ifdef NRRAM_DS3065_DRIVER

/*DS3065 chip read /write interface*/
inline void DS3065_REG_WRITE(unsigned short offset,unsigned char v)
{
	*((volatile unsigned char*)(OAM_RTC_BASE_ADRS + offset)) = v;
}
inline unsigned char DS3065_REG_READ(unsigned short offset)
{
	return *((volatile unsigned char*)(OAM_RTC_BASE_ADRS + offset));
}
#define BSP_NVRAM_WRITE_BYTE(offset , val)	 (*(unsigned char *)(offset + NVRAM_BASE_ADRS)) = (val)
#define BSP_NVRAM_READ_BYTE(offset) 	(*(unsigned char *)(offset + NVRAM_BASE_ADRS))

#define DS3065_YEAR 	(0x000f)
#define DS3065_MONTH 	(0x000e)
#define DS3065_DATE 	(0x000d)
#define DS3065_DAY 		(0x000c)
#define DS3065_HOUR 	(0x000b)
#define DS3065_MINUTE 	(0x000a)
#define DS3065_SECOND 	(0x0009)
#define DS3065_CENTURY 	(0x0008)


#define DS3065_READ_BEGIN DS3065_REG_WRITE(DS3065_CENTURY,0x00000040);
#define DS3065_READ_END DS3065_REG_WRITE(DS3065_CENTURY,0);

#define DS3065_WRITE_BEGIN DS3065_REG_WRITE(DS3065_CENTURY,0x00000080);
#define DS3065_WRITE_END DS3065_REG_WRITE(DS3065_CENTURY,0);

#define DS3065_RTC_RUN \
{\
	DS3065_WRITE_BEGIN \
	DS3065_REG_WRITE(0x7f9,0);\
	DS3065_WRITE_END \
}

#define DS3065_RTC_STOP \
{\
	DS3065_WRITE_BEGIN \
	DS3065_REG_WRITE(0x7f9,0x80); \
	DS3065_WRITE_END \
}


static unsigned char toBCD(unsigned char intValue)
{
    unsigned char bcdValue;
	
    bcdValue = intValue%10;
    intValue /= 10;
    intValue *= 0x10;
    bcdValue += intValue;
	
    return (bcdValue);
}

static unsigned char fromBCD(unsigned char bcdValue)
{
    unsigned char intValue;
	
    intValue = bcdValue&0xf;
    intValue += ((bcdValue&0xf0)>>4)*10;
	
    return (intValue);
}


void oam_rtc_set(short year , char month , char date, char day, char hour , char min ,char sec)
{
	unsigned char osc ;
	DS3065_WRITE_BEGIN;	

	DS3065_REG_WRITE(DS3065_YEAR , toBCD(year%100)) ;  
	DS3065_REG_WRITE(DS3065_MONTH ,toBCD(month)) ;
	DS3065_REG_WRITE(DS3065_DATE ,toBCD(date)) ;
	DS3065_REG_WRITE(DS3065_DAY ,toBCD(day)) ;
	DS3065_REG_WRITE(DS3065_HOUR ,toBCD(hour)) ;
	DS3065_REG_WRITE(DS3065_MINUTE ,toBCD(min)) ;
	osc = DS3065_REG_READ(DS3065_SECOND);
	osc &= 0x80;
	DS3065_REG_WRITE(DS3065_SECOND ,toBCD(sec)| osc) ;
	DS3065_REG_WRITE(DS3065_CENTURY ,toBCD(year/100)) ;

	DS3065_WRITE_END;
}

void oam_rtc_show()
{	
	short year ;
	char month ,date,day ,  hour ,  min , sec ;
	DS3065_READ_BEGIN;	
	year =fromBCD(0xff&(DS3065_REG_READ(DS3065_YEAR)))
	                + (fromBCD(0x3f&(DS3065_REG_READ(DS3065_CENTURY))))*100 ; /* years since 1900	*/ 
	month =fromBCD(0x1f&(DS3065_REG_READ(DS3065_MONTH))); /* months since January		- [0, 11] */
	day = (fromBCD(0x07&(DS3065_REG_READ(DS3065_DAY)))) ; /* days since Sunday	- [0, 6] */
	date = fromBCD(0x3f&(DS3065_REG_READ(DS3065_DATE)));  /* day of the month		- [1, 31] */
	hour = fromBCD(0x3f&(DS3065_REG_READ(DS3065_HOUR)));  /* hours after midnight		- [0, 23] */ 
	min = fromBCD(0x7f&(DS3065_REG_READ(DS3065_MINUTE)));  /* minutes after the hour	- [0, 59] */
	sec = fromBCD(0x7f&(DS3065_REG_READ(DS3065_SECOND)));   /* seconds after the minute	- [0, 59] */
	DS3065_READ_END;

	printf("\nRTC Date:%04d-%02d-%02d", year,month,date);
	printf("\tDay:%02d",day);
	printf("\tTime:%02d:%02d:%02d\n",hour,min,sec);

}

/*rtc 和 NVRAM是一个芯片，使用相同的片选*/
void oam_DS3065_init()
{
	struct tm tm ;	/* ANSI time format */
	struct timespec tv ; /* POSIX time */	
	
	DS3065_READ_BEGIN;	
	
	if ((DS3065_REG_READ(DS3065_SECOND))&0x80)
	{
		DS3065_RTC_RUN
		printf("NOTIFY:DS3065 start running...\n");
	}	

	/* ANSI time */
	tm.tm_year =fromBCD(0xff&(DS3065_REG_READ(DS3065_YEAR)))
	                + (fromBCD(0x3f&(DS3065_REG_READ(DS3065_CENTURY))))*100 -1900; /* years since 1900	*/ 
	tm.tm_mon =fromBCD(0x1f&(DS3065_REG_READ(DS3065_MONTH))) -1; /* months since January		- [0, 11] */
	tm.tm_wday = (fromBCD(0x07&(DS3065_REG_READ(DS3065_DAY)))) ; /* days since Sunday	- [0, 6] */
	tm.tm_mday = fromBCD(0x3f&(DS3065_REG_READ(DS3065_DATE)));  /* day of the month		- [1, 31] */
	tm.tm_hour = fromBCD(0x3f&(DS3065_REG_READ(DS3065_HOUR)));  /* hours after midnight		- [0, 23] */ 
	tm.tm_min = fromBCD(0x7f&(DS3065_REG_READ(DS3065_MINUTE)));  /* minutes after the hour	- [0, 59] */
	tm.tm_sec = fromBCD(0x7f&(DS3065_REG_READ(DS3065_SECOND)));   /* seconds after the minute	- [0, 59] */
	tm.tm_yday = 0 ;
	tm.tm_isdst = 0 ;

	DS3065_READ_END;
	
	/* convert ANSI to POSIX */
	tv.tv_sec = mktime( &tm );
	tv.tv_nsec = 0;

	/* set system time */
	clock_settime(CLOCK_REALTIME, &tv ); 

	oam_rtc_show () ;
	
	return ;
}

#endif /*NRRAM_DS3065_DRIVER*/

void oam_flash_init()
{
#define FLASH_FS_NAME1  "/fs1"
#define FLASH_FS_NAME2  "/fs2"

	if (usrTffsConfig(1,0,FLASH_FS_NAME1)) 
		printf("\nError: "FLASH_FS_NAME1" create fail, use bsp_help() to know how to format the flash\n"); 
	else
	{
		tffsShow(1) ;
		chkdsk(FLASH_FS_NAME1 ,1/*DOS_CHK_ONLY*/,0xff00/*DOS_CHK_VERB_SILENT*/) ;/*check the disk*/
	}

	if (usrTffsConfig(2,0,FLASH_FS_NAME2)) 
		printf("\nError: "FLASH_FS_NAME2" create fail, use bsp_help() to know how to format the flash\n" ); 
	else
	{
		tffsShow(2) ;		
		chkdsk(FLASH_FS_NAME2 ,1/*DOS_CHK_ONLY*/,0xff00/*DOS_CHK_VERB_SILENT*/) ;/*check the disk*/
	}
}
#endif /*TN725_OAM_APP*/


#define CPLD_RESET_REG1_OFF    0x11
	/*板类型信息寄存器*/
#define    BOARD_TYPE_1_REG  	 	0x2
#define    BOARD_TYPE_2_REG  		0x3
#define    TN725_PPC_TYPE			0x3306	
#define	 TN725_OAM_TYPE	 		0x3307
#define	 TN705_OAM_TYPE	 		0x3305
#define 	 BSP_BK_VER_OFFSET		0x0B 	
#define    BSP_OAM_STATUS_OFF 	 	0x27
#define 	 BSP_SLOT_ID_OFFSET		0x12 

static int s_fpga_load_state  = 0 ; /*intial state = load end*/
static  int  tn705_load_qtr6083()
{
	int re ;
	s_fpga_load_state = 0xffff ;  /*load begin*/
	printf("LOAD  FPGA [/tffs/sys/qtr6083.bit]  BEGIN..........\n");
	re = bsp_fpga_io_dl("/tffs/sys/qtr6083.bit",NULL , 0 , 1);
	printf("LOAD  FPGA [/tffs/sys/qtr6083.bit]  END..........\n") ;
	s_fpga_load_state = 0 ;/*load end*/
	return  re ;
}

int init_linecard_info()
{
	unsigned short card_type = 0 , i  ;
	char  temp[4] ;
	card_type = (unsigned short)(*CPLD_REG(BOARD_TYPE_1_REG));
	card_type = (card_type << 8) | ((unsigned char)(*CPLD_REG(BOARD_TYPE_2_REG)));
	
	if((card_type == TN725_OAM_TYPE)  || (card_type == TN705_OAM_TYPE))
	{
		printf("--------card type (0x%x): OAM --------\n" , card_type) ;

		if(card_type == TN705_OAM_TYPE)  /*兼容TN705 OPCA PCBV0*/
		{
			temp[0] = 0x45;  /*TN705 inverntory IO address*/
			temp[1] = 0;		
			*CPLD_REG(0x31) = 0x10 ; /*enable the I2C channel of local board*/
			if((bsp_i2c_output(temp, 2)  == OK)  && ((temp[1]&0x0f) == 0x00))  /*PCBV0 in inventory IO*/
			{		
				printf("TN705 OPCA PCBV0 --------\n" ) ;
				*CPLD_REG(0x200003) = 0xA5A5 ;	 /*FPGA 白板寄存器写入特殊值*/

				/*打开总复位开关 BIT7*/
				*CPLD_REG(CPLD_RESET_REG1_OFF) = (*CPLD_REG(CPLD_RESET_REG1_OFF)) | 0x80 ;	
				
				if(((*CPLD_REG(0x200003)) & 0xffff) != 0xA5A5)
				{
					/*复位switch芯片*/			
					*CPLD_REG(CPLD_RESET_REG1_OFF) = (*CPLD_REG(CPLD_RESET_REG1_OFF)) & (~0x40) ;				

					s_fpga_load_state = 0xffff ;  /*load begin*/
					taskSpawn("tLoadQtr6083", 100, 0, 0x7000, (FUNCPTR)tn705_load_qtr6083, 0,0,0,0,0,0,0,0,0,0);
					while(s_fpga_load_state)
					{
						taskDelay(50);	
					}
				}
			}
		}
		
		/*打开总复位开关 BIT7*/
		*CPLD_REG(CPLD_RESET_REG1_OFF) = (*CPLD_REG(CPLD_RESET_REG1_OFF)) | 0x80 ;	

		/*逐个打开各个芯片复位，且加入延时,是防止一次解开导致电流过大*/		
		for(i = 0 ; i < 8 ; i++)
		{
			*CPLD_REG(CPLD_RESET_REG1_OFF) = (*CPLD_REG(CPLD_RESET_REG1_OFF)) | (0x1 << i) ;	
			taskDelay(5) ;
		}
		
#ifdef	TN725_OAM_APP		
		oam_DS3065_init() ;
		oam_flash_init() ;
		if(card_type == TN725_OAM_TYPE)
			_init_tn725_oam_vlan() ;
		if(card_type == TN705_OAM_TYPE)
			_init_tn705_oam_vlan() ;

#endif	/*TN725_OAM_APP  || TN705_OAM_APP */
	}
	else
	{
		/*should to add..*/
		return 0 ;
	}
	return 0 ;
}		

/* 获取本板所在槽位 */
unsigned char bsp_get_chss_pos(void)
{

	const static unsigned char s_aPhySlot_725[18] =
	{
		1, 2, 3, 4, 5, 6, 7, 8 , /*1号到8*/
		13,14,15,16,17,18 ,  /*13号到18*/
		0xfe,0xfe,0xfe,0xfe  /*unused*/
	};

	const static unsigned char s_aPhySlot_705[8] =
	{
		1, 2, 5, 6, 7, 8, 3,4 
	};

	unsigned short card_type = 0 ;
	unsigned char  slot_id = (*CPLD_REG(BSP_SLOT_ID_OFFSET))&0x0F ;
	unsigned int 	 bk_ver = *CPLD_REG(BSP_SLOT_ID_OFFSET)  & 0x30;
	card_type = (unsigned short)(*CPLD_REG(BOARD_TYPE_1_REG));
	card_type = (card_type << 8) | ((unsigned char)(*CPLD_REG(BOARD_TYPE_2_REG)));


	if(card_type  == TN725_OAM_TYPE  || card_type  == TN725_PPC_TYPE)  /*主控，PPC板*/
	{
		if(slot_id == 0x00)  /*OAM1 10*/
			return 	10 ;
		else if(slot_id == 0x05)  /*OAM2 12*/
			return 	12 ;
		else if(slot_id == 0x06)  /*PPC1 9*/
			return 	9 ;
		else if(slot_id == 0x09)  /*PPC2 11*/
			return 	11 ;
		else
			return 0xfe;
	}
	else  if(bk_ver  == 0x00 ) /*TN725单板*/
	{
		return (s_aPhySlot_725[slot_id] );
	}
	else if(bk_ver == 0x20)
	{
		if(slot_id <= 7)
			return (s_aPhySlot_705[slot_id] );
	}
	
	return 0xfe;
	}

int get_main_oam_ip(char * ip) 
{    
	unsigned int cards_status1 = *CPLD_REG(BSP_OAM_STATUS_OFF);

	if((cards_status1&0x80) == 0x00)  /*oam1 在位*/
	{
		strcpy(ip , "192.168.210.10") ;	
		printf("main OAM IP is: %s\n" , ip) ;
		return OK ;
	}
	else if((cards_status1&0x40) == 0x00)  /*oam2 在位*/
	{		
		strcpy(ip , "192.168.210.12") ;	
		printf("main OAM IP is: %s\n" , ip) ;
		return OK ;
	}
	else
	{
		printf("Main DO NOT find : cards_status1= 0x%x \n" , cards_status1) ;
		return ERROR ; // get main oam fail ;
	}
}

static void bsp_motfcc_init()
	{
	struct in_addr iaddr;
	char ipadrr[INET_ADDR_LEN];
	unsigned long ifmask ;
	unsigned char ifmac[INET_ADDR_LEN];
	unsigned char chssPos = 0;	
	unsigned short card_type = 0 ;
	card_type = (unsigned short)(*CPLD_REG(BOARD_TYPE_1_REG));
	card_type = (card_type << 8) | ((unsigned char)(*CPLD_REG(BOARD_TYPE_2_REG)));

	/* 读取单板板位 */
	chssPos = bsp_get_chss_pos();

	if(chssPos != 0xfe)
	{
		/* 构造IP地址和子网掩码 */
		sprintf(ipadrr, "192.168.210.%d", chssPos);
		ifmask = DEFAULT_IP_MASK;
	}
	else  if(0 != bsp_ifadrs_check(2))
	{
		sprintf(ipadrr, "192.168.210.254");
		ifmask = DEFAULT_IP_MASK;
	}
	else				
	{
		bsp_ifadrs_get_attr(2 , &(iaddr.s_addr), &ifmask, ifmac);
		inet_ntoa_b(iaddr, ipadrr);
	}
	
	/*change ip address for motfcc 0*/
	sys_rename_ipaddress("mottsec", 0, ipadrr , ifmask) ;
	

	
	/*如果是OAM板，则需要初始化物理端口 fcc1 */
	if(card_type == TN705_OAM_TYPE || card_type == TN725_OAM_TYPE)
	{
		/*没有用户配置IP 的情况下，FCC1 是默认IP 地址*/
		if(0 != bsp_ifadrs_check(0)) 
		{
			sprintf(ipadrr, DEFAULT_IP_ADRS_STRING);
			ifmask = DEFAULT_IP_MASK;
		}
		else  				
		{
			/*如果用户配置了IP 地址，则使用用户配置的FCC1 IP 地址*/
			bsp_ifadrs_get_attr(0 , &(iaddr.s_addr), &ifmask, ifmac);
			inet_ntoa_b(iaddr, ipadrr);
		}	
		
		if(ipAttach(2 , "mottsec") == OK)
		{
			ifMaskSet("mottsec2", ifmask);
			ifAddrSet("mottsec2", ipadrr);
		}
	}	

	if (OK == ifAddrGet("mottsec0", ipadrr))
	{
		printf("\nmottsec0 ip address is %s.\r\n", ipadrr);
	}

	if (OK == ifAddrGet("mottsec2", ipadrr))
	{
		printf("\nmottsec2 ip address is %s.\r\n", ipadrr);
	}
}

/**************************获取其它槽位的CPLD基地址*****************/
int	bsp_chass_get_base_adrs(char chass , unsigned long *adrs)
{ 
	unsigned char chssPos = bsp_get_chss_pos();	
	
	if((chass == chssPos) || (chass == 0))  /* 本板CPLD基地址*/
		*adrs = LOCAL_CPLD_BASE_ADRS ;
	else if (chass <= 16 )/* 其它槽位的CPLD基地址*/
		*adrs = LOCAL_CPLD_BASE_ADRS + 0x01000000*chass ;
	else
		return -1 ;
	return 0;
}	
void ctlLed(int ledctlfun, unsigned int led_select)
{	
	bsp_control_led(ledctlfun , led_select) ;	
}

/******************************************************************************
*
* usrAppInit - initialize the users application
*/ 

void usrAppInit (void)
    {
#ifdef	USER_APPL_INIT
	USER_APPL_INIT;		/* for backwards compatibility */
#endif
    /* add application specific code here */
    
	STATUS sc;	
	ftpdInit(NULL, 0);

    bsp_init_watchdog();
    /* 绑定flash文件系统 */
    flRegisterComponents();
    sc = usrTffsConfig(0,0,"/tffs");
    if (sc != OK)
    {
        printf("\nError: /tffs create fail, use bsp_help() to know how to format the flash\n" );
    }
    else
    {
        tffsShow(0);
        //dosFsConfigShow("/tffs");
        //chkdsk ("/tffs",2/*DOS_CHK_REPAIR*/,0xff00/*DOS_CHK_VERB_1*/) ; /*check the disk*/
    }
    BspI2cInit();
    BspSpiInit();

    bsp_motfcc_init();

    init_linecard_info();


//    i2ctest();

	//taskSpawn("tled", 20, 0, 0x1000, (FUNCPTR)bsp_led_task, 0,0,0,0,0,0,0,0,0,0);
    taskSpawn("tBM", 20, 0, 0x1000, (FUNCPTR)tBiosMain, 0,0,0,0,0,0,0,0,0,0);
}

int bsp_jtag_download(char *fileName)
{
    return _jtag_download(fileName);
}


