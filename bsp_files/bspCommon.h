/*bspCommon.h -----
---BSP通用接口头文件声明*/

#ifndef	__BSP_COMMON_H__
#define	__BSP_COMMON_H__


#include "vxWorks.h"

#ifdef __cplusplus
    extern "C" {
#endif


#ifndef	_ASMLANGUAGE

#include "stdlib.h"
#include "stdio.h"
#include "taskLib.h"
#include "inetLib.h"


#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)
#define ABS_ADDR_TYPE	UINT32
#else
#define ABS_ADDR_TYPE	void*
#endif

enum{ BIT0 = 0x1, BIT1 = 0x02,BIT2 = 0x04, BIT3 = 0x08, BIT4 = 0x10, BIT5 = 0x20, BIT6 = 0x40, BIT7 = 0x80};

extern char g_CardImgName[32] ; 

/***********************************************************************
BSP 自升级，文件上传下载
1.FTP文件传送接口 ，相关宏INCLUDE_BSP_FTP_TXRX
************************************************************************/
#define INCLUDE_BSP_FILE_OP

#define	MAX_BIN_SIZE 	 	 0x100000
#define   MAX_APP_IMG_SIZE	 0x400000

int	bsp_ftp_get_file(char * ip , char* user, char *password , char * remote_full_name , char * local_full_name);
int	bsp_ftp_put_file(char * ip , char* user, char *password , char * remote_full_name , char * local_full_name);
int	bsp_ftp_download_from(char * ip , char* user, char *password , char *path , char * name, char * buffer, int len, int * reallen);
int 	bsp_fs_download(char * name, char * buffer, int len, int * reallen);

typedef struct _bsp_if_attr
{
	volatile unsigned long		ipaddr;
	volatile unsigned long		ipmask;
	volatile unsigned char		mac[6];
	volatile unsigned char		crc;
	volatile unsigned char		cardType;
}BSP_IF_ATTR;

#define   DEFAULT_IP_ADRS_STRING 	"192.168.0.254"
#define   DEFAULT_IP_MASK 	0xFFFFFF00

/*if 接口 IP mac地址接口*/
int bsp_ifadrs_get_attr(int num, unsigned long* ip, unsigned long* mask, unsigned char* mac) ;
int bsp_ifadrs_get_mac(int num , unsigned char * mac) ;


/***********************************************************************
* FPGA download
************************************************************************/

#define INCLUDE_IO_DOWNLOAD_FPGA
 #define	CHASS_MAX_FPGA	       8                 
#define BSP_CHK_FPGA_NUM_OK(num)		(((num) > 0) && ((num) <= CHASS_MAX_FPGA))


typedef struct _b_bsp_fpga_dl_ctrl
{
	volatile unsigned char rev1[26];
	volatile unsigned char clke;		/* 0x1a */
	volatile unsigned char prog_h;		/* 0x1b */
	volatile unsigned char prog_l;		/* 0x1c */
	volatile unsigned char data;		/* 0x1d */
	volatile unsigned char rev2;		/* 0x1e */
	volatile unsigned char done;		/* 0x1f */
}* B_BSP_FPGA_CTRL_ID;

typedef struct _w_bsp_fpga_dl_ctrl
{
	volatile unsigned short rev1[26];
	volatile unsigned short clke;		/* 0x1a */
	volatile unsigned short prog_h;		/* 0x1b */
	volatile unsigned short prog_l;		/* 0x1c */
	volatile unsigned short data;		/* 0x1d */
	volatile unsigned short rev2;		/* 0x1e */
	volatile unsigned short done;		/* 0x1f */
}* W_BSP_FPGA_CTRL_ID;

typedef struct _l_bsp_fpga_dl_ctrl
{
	volatile unsigned int rev1[26];
	volatile unsigned int clke;		/* 0x1a */
	volatile unsigned int prog_h;		/* 0x1b */
	volatile unsigned int prog_l;		/* 0x1c */
	volatile unsigned int data;		/* 0x1d */
	volatile unsigned int rev2;		/* 0x1e */
	volatile unsigned int done;		/* 0x1f */
}* L_BSP_FPGA_CTRL_ID;


#define	FPGA_GET_DOWN_TIMES	100

#define BSP_FPGA_PIN(num)			(1 << ((num) - 1))

#define BSP_SET_FPGA_PROG_DN(pctrl, num)		\
{\
	pctrl->prog_h &= ~(BSP_FPGA_PIN(num));\
	pctrl->prog_l |= (BSP_FPGA_PIN(num));\
}

#define BSP_SET_FPGA_PROG_UP(pctrl, num)		\
{\
	pctrl->prog_h |= (BSP_FPGA_PIN(num));\
	pctrl->prog_l &= ~(BSP_FPGA_PIN(num));\
}

#define BSP_SET_FPGA_CLKE(pctrl, num)	pctrl->clke |= BSP_FPGA_PIN(num)
#define BSP_CLR_FPGA_CLKE(pctrl, num)	pctrl->clke &= ~(BSP_FPGA_PIN(num))
#define BSP_GET_FPGA_DONE(pctrl, num)   ((pctrl->done & (BSP_FPGA_PIN(num))) == (BSP_FPGA_PIN(num)))

#define CPLD_FPGA_CLK				0x2
#define CPLD_FPGA_DOUT				0x1

int bsp_chass_get_base_adrs(char chass,	unsigned long *	adrs);
int bsp_io_acs_pin_init(char chass);
int bsp_io_send_data(char chass, char *adrs, unsigned int len) ;
int bsp_io_send_data_cyclone(char chass, char *adrs, unsigned int len);

/************************************************************
* CPU 系统硬件狗
************************************************************/

int bsp_init_watchdog() ;
void bsp_clear_dog() ;

/************************************************************
* sysMmuProtectText - protect text segment in RAM. 
************************************************************/
STATUS sysMmuProtectText(int enable) ;	

/************************************************************
* i2c 接口
*************************************************************/

void bsp_i2c_init(void) ;
STATUS bsp_i2c_output(char * pBuf, UINT32 numBytes) ;

/*兼容原有的老的接口*/
INT32 bsp_i2c_write
    (
    UINT32 	deviceAddress,	/* Devices I2C bus address */
    UINT32 	numBytes,	/* number of bytes to write */
    char *	pBuf ,		/* pointer to buffer of send data */
    UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
     UINT32   stop            /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    ) ;
INT32 bsp_i2c_read
    (
    UINT32 	deviceAddress,	/* Device I2C  bus address */
    UINT32 	numBytes,	/* number of Bytes to read */
    char *	pBuf ,		/* pointer to buffer to receive data */
    UINT32   repeatStart ,   /* repeatStart == 0 :i2c normal start , else repeat start*/
    UINT32   stop          /* stop == 0 action , i2c do not stop , else stop after tx/rx*/        
    ) ;

UINT32 bsp_i2c_scl_freq (UINT32   num_of_1k ) ;/*	设置I2C 时钟频率, 最大值195k, 最小6k ,参数以1k 为单位*/

/************************************************************
* SPI 接口
*************************************************************/

void bsp_spi_init(void) ;
STATUS bsp_spi_output(char *buffer, int length , unsigned short option , unsigned char read_action) ;

/************************************************************
* FPAG load 接口
*************************************************************/
int	bsp_fpga_io_dl(char* ip_or_fullName, char* p_name, int chass_id, int num) ;
int	bsp_fpga_io_dl_cyclone(char* ip_or_fullName, char* p_name, int chass_id, int num) ;


/***********************************************************************
* CPLD 面板点灯控制
************************************************************************/
void bsp_control_led(int ledctlfun, unsigned int led) ;

/***********************************************************************
* other interface
************************************************************************/
int bsp_ifadrs_check(int num) ;
STATUS sys_rename_ipaddress (char *pNetDevName, int DevNum,char *pNewDevAddr,unsigned int  SubNetMask) ;
void myWelcome(void) ;

/***********************************************************************
* flash 写,读，擦除接口
************************************************************************/
int flTakeFlashMutex(int serialNo , int mode) ;
void flFreeFlashMutex(int serialNo) ;
int bsp_flash_erase(int chipno, int first, int num);
int bsp_flash_write(int chipno, int address, char * buffer, int length);
int bsp_flash_read(int chipno, int address, char * buffer, int length);

/***********************************************************************
* flash 写使能/禁止控制 
************************************************************************/
void  flash_write_enable(int chipno) ;
void  flash_write_disable(int chipno) ;

/***********************************************************************
* Upgrade软件升级模式
************************************************************************/
#define  LFileSystem_sys1   1 
#define  LFileSystem_sys2   2 
#define  LFileSystem_remotesys1     3 
#define  LFileSystem_remotesys2     4 

char SetFirstActiveSys(char sysX , char *img_name) ;/*设置优先启动的文件系统源*/
char WhichSysXActiveNow(); /*当前启动的文件系统源*/
char WhichSysXActiveNext();  /*下一次启动的文件系统源*/

/***********************************************************************
* 其它接口
************************************************************************/
char * sysMemTop (void) ;
unsigned int vxImmrGet() ;
int sysClkRateGet (void) ;
int bsp_arp_broadcast(char*  ifname)  ;
int	bsp_ramdisk_init(unsigned int  ramdiskAddr , unsigned int ramdiskSize) ;
extern  UINT32 g_sysPhysMemSize ;

/***********************************************************************
* GZIP 压缩解压缩接口
************************************************************************/
#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define PROG_GZ_READ_SIZE (1024*512)
void *gzopen(char *file, char  *mode);
int  gzread(void *fp ,  char*dst_addr , int size);
int  gzclose(void *fp);
int  gzerror(void *fp,int *err) ;


#endif	/*_ASMLANGUAGE*/

#ifdef __cplusplus
    }
#endif

#endif /*__BSP_COMMON_H__*/

