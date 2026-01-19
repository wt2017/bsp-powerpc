/*proj_config.h ---
   工程或者单板特定参数的配置;
*/
#ifndef	__BSP_PROJ_CONFIG__
#define	__BSP_PROJ_CONFIG__

#ifdef __cplusplus
extern "C" {
#endif

#define VER_NR_BIOS_MAJOR 3  /*1 : vxWorks 5.4 , 2: vxWorks 5.5 , 3: vxWorks 6.3*/
#define VER_NR_BIOS_MINOR 6  /*2 :mpc860 , 3 : mpc8260 , 4:mpc8245 ,5:MPC8247, 6:MPC8548*/
#define VER_NR_BIOS_MICRO 1   /*main version   */
#define VER_NR_BIOS_DEBUG 2   /*version */
#define	BSP_DEV_NAME	"MPC8548 BSP"
#define BSP_VERSION     "3.6.1.2"

#include "config.h"
#define _INCLUDE_IMG_UPGRADE_BY_FLASH_
#define INCLUDE_IO_DOWNLOAD_FPGA
#define	INCLUDE_MTD_AMDXXX		/* AMD: Am29LV256M */
#define INCLUDE_BSP_FILE_OP
#undef  _INCLUDE_IP_SAVEIN_FLASH_
#define _INCLUDE_IP_SAVEIN_NVRAM_
#define 	_INCLUDE_IIC_
#define 	_INCLUDE_SPI_

#define VXWORKS_IMAGE_START_ADDR 0X30000

#define M85XX_DEVDISR           (CCSBAR + 0xE0070)   
#define M85XX_GPIOCR            (CCSBAR + 0xE0030)   
#define M85XX_GPOUTDR           (CCSBAR + 0xE0040)   
#define M85XX_GPINDR            (CCSBAR + 0xE0050)   

#define BSP_GPOUT_ENABLE        0X200
#define BSP_FLASH_WR            0X2
#define BSP_PHY_ENABLE          0X4
#define BSP_WTD_CLR             0X8  
#define BSP_GPIO_PCI_OUT_ENABLE 0X00020000
#define BSP_GPIO_PCI_IN_ENABLE  0X00010000

/*CPU相关的点灯寄存器*/
#define BSP_LIGHT_REG_1 	    M85XX_GPOUTDR
#define BSP_LED_RED_1	   		0x00000020 
#define BSP_LED_GREEN_1			0x00000010
#define BSP_LED_ALL_1 			(BSP_LED_RED_1 | BSP_LED_GREEN_1)
#define BSP_SINGLE_LIGHT_MASK_1 (BSP_LED_RED_1 | BSP_LED_GREEN_1)

/*
	CPLD reg 0x17 : address = LOCAL_CPLD_BASE_ADRS + 0x17
    BIT31 -- RED light ,0:light ,1:dark
    BIT30  --GREEN light ,0:light ,1:dark
    BIT28,BIT29--DUAL light ,01:light_green ,10:light ,else dark
*/
#define BSP_LIGHT_REG 			(LOCAL_CPLD_BASE_ADRS + 4*0x18)
#define BSP_LED_GREEN			0x00000008
#define BSP_LED_RED	   			0x00000004 
#define BSP_DUAL_LED_GREEN	    0x00000001 
#define BSP_DUAL_LED_YELLOW	    0x00000002 
#define BSP_LED_ALL 			(BSP_LED_RED|BSP_LED_GREEN|BSP_DUAL_LED_YELLOW)

#define BSP_LIGHT_MASK    		BSP_LED_ALL    
#define BSP_SINGLE_LIGHT_MASK   (BSP_LED_RED|BSP_LED_GREEN)
#define BSP_DUAL_LIGHT_MASK     (BSP_DUAL_LED_GREEN | BSP_DUAL_LED_YELLOW)
#define BSP_LED_FUN_LIGHT       1
#define BSP_LED_FUN_DARK        0
#define BSP_LED_FUN_FLIP        3

/*************************MPC 85xx JTAG  PIN control **********************/
#define     CPLD_JTAG_IN_ADDR   M85XX_GPINDR
#define     CPLD_JTAG_OUT_ADDR  M85XX_GPOUTDR
#define 	BIT32(x)  		 (1<<(x))

#define 	Cpld_TDI		BIT32(21)  	/* Bit address of TDI signal */
#define 	Cpld_TCK		BIT32(20)   /* Bit address of TCK signal */
#define 	Cpld_TMS		BIT32(22)   /* Bit address of TMS signal */
#define 	Cpld_PIN_ENABLE	0           /* Bit address of chip enable */
#define 	Cpld_RST		BIT32(23)  	/* Bit address of TRST signal */
#define 	Cpld_PINCE		0           /* Bit address of buffer chip enable */
#define 	Cpld_TDO		BIT32(19)  	/* Bit address of TDO signal */
#define 	CPLD_PORT_GROUP (Cpld_TDI|Cpld_TCK|Cpld_TMS|Cpld_RST|Cpld_TDO)

/******************************************************************************
*
* CS1() FLASH
*
* base address = 0xC0000000
* size = 0x10000000	(256M byte)
*
*/
#define	CS_C_BASE_ADRS		     0xc0000000
#define	CS_C_SIZE			     0x10000000

#define	OAM_FLASH_FFS1_BASE_ADRS 0xc0000000			/* oam tffs base address */ 
#define	OAM_FLASH_FFS1_SIZE		 0x04000000		 	/* oam tffs memory size */   
#define	OAM_FLASH_FFS2_BASE_ADRS 0xc4000000			/* oam tffs base address */ 
#define	OAM_FLASH_FFS2_SIZE		 0x04000000		 	/* oam tffs memory size */   

#define FLASH_TFFS_SIZE_RESERVE  0x100000           /*最后1M的空间用于bios*/

#define	FLASH_BASE_ADRS_1		 OAM_FLASH_FFS1_BASE_ADRS
#define	FLASH_SIZE_1			 OAM_FLASH_FFS1_SIZE
#define	FLASH_BASE_ADRS_2		 OAM_FLASH_FFS2_BASE_ADRS
#define	FLASH_SIZE_2			 OAM_FLASH_FFS2_SIZE

#define	TFFS_NUM   			3

#ifndef FLASH_BASE_ADRS
#define FLASH_BASE_ADRS                 0xfc000000
#endif

#define GET_FLASH_BASE_ADDR(chipno , base_addr)   \
{\
	if(chipno == 0)  \
		base_addr = FLASH_BASE_ADRS ;  \
	else if(chipno == 1)	  \
		base_addr = FLASH_BASE_ADRS_1 ;  \
	else if(chipno == 2)	  \
		base_addr = FLASH_BASE_ADRS_2 ;  \
  	else   \
		base_addr =  0xFFFFFFFF ; \
}

#define BACKUP_FILE_SYS 	LFileSystem_sys2
#define BOOT_FILE_DIR1  			"/tffs/sys/"
#define BOOT_FILE_UPGRADE_DIR  "/tffs/upgrade/"
#define BOOT_FILE_DIR2  			"/fs1/sys/"
#define BOOT_FILE_DIR3 			 "/ramdisk:0/"

#define	MAX_BIN_SIZE            0x100000
#define MAX_APP_IMG_SIZE        0x400000

#define	FLASH_BLOCK_SIZE	    0x00020000	/* 128 K bytes */
#define	FLASH_BLOCK_NUM		    0x200		/* 512 */
#define	FLASH_SIZE			    0x04000000

#define	FLASH_FFS_BASE_ADRS		FLASH_BASE_ADRS/* Flash memory base address */ 
#define	FLASH_FFS_SIZE			(FLASH_SIZE/2)	/* Flash memory size */   
#define	FLASH_BIOS_BOOT_OFF  	0x3f40000
#define	FLASH_ERASE_UNIT_SIZE	FLASH_BLOCK_SIZE
#define FLASH_IFATTR_BLOCK 		504         /*最后1M用于bios,其中第一块用于ip*/
#define BSP_IF_OFFSET_ADRS	  	(FLASH_BLOCK_SIZE*FLASH_IFATTR_BLOCK)


#define	OAM_NVRAM_BASE_ADRS 	0xd0000000			
#define	OAM_NVRAM_SIZE		0x00100000	
#define   BSP_IF_NVRAM_OFF	  	0
#define   NVRAM_ADDR(offset)        ((char*)(OAM_NVRAM_BASE_ADRS+offset))

/******************************************************************************
*
* CSa(CS3):	little CPLD
* base address = 0xa0000000
* size = 0x20000000	(256M byte)
*/
#define	CS_A_BASE_ADRS		    0xa0000000
#define	CS_A_SIZE			    0x10000000

#define	LOCAL_CPLD_BASE_ADRS	CS_A_BASE_ADRS
#define	LOCAL_CPLD_SIZE			0x10000

#define CPLD_RESET_REG1_OFF     0x11
#define CPLD_FLASH_WP 	        0x16
/*32bit mode*/
#define	CPLD_REG(off)	   ((volatile unsigned int *)(LOCAL_CPLD_BASE_ADRS + (off * 4)))
#define	FPGA_REG(off)	   ((volatile unsigned int *)(LOCAL_CPLD_BASE_ADRS + 0x800000+ (off * 4)))

/*SLOT ID REGISTERS*/
#define BSP_SLOT_ID_OFFSET		0x12 

/*内存最高的64K的存储空间内存放BIOS的版本信息*/
#define BIOS_VERSION_BASE_ADRS  (LOCAL_MEM_LOCAL_ADRS+ LOCAL_MEM_SIZE- 0X10000)
#define SRAM_BIOS_VER_OFF   	0x80

#define	NVRAM_BASE_ADRS	        0xD0000000
#define	NVRAM_SIZE	            1024 * 1024 
#define BSP_IF_OFF	    	    0x00   /*保存单板的IP地址,MAC地址等*/
#define BSP_BOOT_FLAG_OFF		0x60  /*保存上层应用的启动方式标志位*/
#define SRAM_BIOS_VER_OFF 		0x80  /*保存BIOS运行版本号 */
#define	OAM_RTC_BASE_ADRS 	    0xd2000000			
#define	OAM_RTC_SIZE			0x00010000		 	

#define DEFAULT_IP_ADRS_STRING 	 "192.168.0.254"
#define DEFAULT_IP_MASK 	     0xFFFFFF00
#define BCTL_SDRAM_128M_SIZE	 0x08000000		/* 128 Mbyte memory available */
#define BCTL_SDRAM_512M_SIZE	 0x20000000		/* 512 Mbyte memory available */

/*0x60~0x67 byte define */
#define  BSP_CRC_OFF               	BSP_BOOT_FLAG_OFF
#define  BSP_ACTIVE_STARTUP_OFF 	(BSP_CRC_OFF + 1)
#define  BSP_CUR_STARTUP_OFF 		(BSP_CRC_OFF + 2)
/*four bye or more....*/
#define  BSP_CNT_OFF    			(BSP_CRC_OFF + 3)

/******************************************************************************
*
* FPGA download
*
*/
#define	CHASS_MAX_FPGA	         8
#define BSP_CHK_FPGA_NUM_OK(num) (((num) > 0) && ((num) <= CHASS_MAX_FPGA))

#define BSP_FPGA_CTRL_ID  L_BSP_FPGA_CTRL_ID

#define	FPGA_GET_DOWN_TIMES	100

#define BSP_FPGA_DL_OAMP(chass)			(((chass) == 1) || ((chass) == 2))

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

#ifndef	_ASMLANGUAGE
typedef struct _bps_device_test_str
{
    unsigned long DeviceTestTotalNum;
    unsigned long DDRTestErrorNum;
    unsigned long Flash0TestErrorNum;    
    unsigned long Flash1TestErrorNum;    
    unsigned long Flash2TestErrorNum;        
    unsigned long PCITestErrorNum;    
    unsigned long IICTestErrorNum;    
    unsigned long SPITestErrorNum;    
    unsigned long NVRAMTestErrorNum;
    unsigned long CPLDTestErrorNum;
}BSP_DEVICE_TEST_STR;
#endif

#ifdef __cplusplus
    }
#endif

#endif /*__BSP_PROJ_CONFIG__*/



