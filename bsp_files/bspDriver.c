/******************************************************************************
*
* This module realize the driver we used in the bsp.
*
* It include the spi driver, the  flash driver, the dallas DS1742 nvram driver,
* and the ftp file transmit driver.
*
******************************************************************************/
/*
2007-01-1 ,xiexy  
2007-11-12 xiexy add the file download and upload interface
*/

#include <stdio.h>

#include "vxWorks.h"
#include "ftpLib.h"
#include "ioLib.h"
#include "string.h"
#include "drv/mem/m8260Siu.h"
#include "drv/sio/m8260CpmMux.h"
#include "drv/sio/m8260sio.h"
#include "drv/parallel/m8260IOPort.h"
#include "drv/timer/m8260Clock.h"
#include "taskLib.h"
#include "intLib.h"
#include "proj_config.h"
#include "bspCommon.h"
#include "pingLib.h"
#include "selectLib.h"
#include "time.h"

#if  defined(INCLUDE_BSP_FILE_OP)
/******************************************************************************
*
* This is the ftp file transmit	API. 
* 
* You must provide the ftp server's ip address,	and	told him the file name you wanna get. 
* You must to allocate the memory to restore the file got from the server, 
* told me the buffer length is nessisary. Finally, I use the last parametre to return
* the real length I get from the FTP server.
*
*/

int	bsp_ftp_download_from(char * ip , char* user, char *password , char *path , char * name, char * buffer, int len, int * reallen)
{
	char buf[8*1024] ;
	struct timeval tv; 
	int	ctrl_sock, data_sock, num=0 ,ret = 0 , status = 0  ;
	char * p_desc = buffer;
	fd_set readset ; 

	//ping 3 次，确定网络是通畅
	if(ping(ip , 3 , PING_OPT_SILENT) != OK) 
	{
		printf("\nping %s no answer , please check network link and configuration\n", ip);
		return -6;
	}
	
	if (ftpXfer(ip, user, password, "", "RETR %s", path, name, &ctrl_sock, &data_sock) == ERROR)
	{	
		printf("ftpXfer socket connection fail\n");
		return -1;
	}
	
	FD_ZERO(&readset);
	FD_SET(data_sock, &readset); 

	tv.tv_sec = 20;
	tv.tv_usec = 0; 

	*reallen = 0 ;
	while(1)
	{
		status = select(data_sock+1, &readset, NULL, NULL, &tv); 
		if(status == 0)
		{
			printf("\nselect socket timeout!(20 second)\n"); 
			close (data_sock);
			close (ctrl_sock);
			return -1 ;
		}
		else if(status == 1)
		{ 
			if(FD_ISSET(data_sock, &readset))	
			{
			num = read(data_sock , buf, sizeof(buf)) ;
			if(num <= 0)
				break ;
			if((*reallen + num) > len)
				{
				printf("Error: file length  bigger than the buffer\n"); 
				break;
				}

			memcpy(p_desc, buf,	num);
			p_desc += num;
			*reallen +=	num;
			}	
		}
		else
		{
			printf("select socket ERROR(ret = %d)\n" ,status); 
			break ;
		}
	}
	printf("\n");
	
	close (data_sock);

	if (num	< 0)
	{
		printf("ftp error num < 0\n") ;
		ret = -2;
	}

	if (ftpReplyGet(ctrl_sock, TRUE) != FTP_COMPLETE)
		ret = -3;

	if (ftpCommand(ctrl_sock, "QUIT", 0, 0, 0, 0, 0, 0) != FTP_COMPLETE)
		ret = -4;

	close (ctrl_sock);

	return ret;
}
int	bsp_ftp_file(char * ip , char* user, char *password , char * remote_full_name , char * local_full_name , int cmd) 
{
#define FTP_BUFFER_SIZE		 4096
	char *buf  = NULL , *cmdString= NULL , *ftpString= NULL  ;
	struct timeval tv; 
	int ctrl_sock, data_sock, num ,ret = 0 , status = 0 ;
	FILE * fp = NULL ;
	fd_set actionset ; 

	if(cmd == 0)  /*get file*/
	{
		cmdString = "wb" ;
		ftpString = "RETR %s" ;	
	}
	else    /*put file*/
	{
		cmdString = "rb" ;
		ftpString = "STOR %s" ;			
	}
		
	//ping 3 次，确定网络是通畅
	if(ping(ip , 3 , PING_OPT_SILENT) != OK) 
	{
		printf("\nping %s no answer , please check network link and configuration\n", ip);
		return -6;
	}

	if ((buf = malloc(FTP_BUFFER_SIZE))  == NULL)
	{	
		return -7;
	}
	
	if((fp = fopen(local_full_name , cmdString)) == NULL) 
	{	
		printf("open/create  (%s)  fail , check the path and fileName\n" , local_full_name);
		free(buf);
		return -8 ;
	}

	if (ftpXfer(ip, user, password, "", ftpString, ".", remote_full_name, &ctrl_sock, &data_sock) == ERROR)
	{	
		printf("ftpXfer socket  create connection fail , check the ftp server configrations\n");
		free(buf);
		fclose (fp) ;
		return -9;
	}



	FD_ZERO(&actionset);
	FD_SET(data_sock, &actionset); 

	tv.tv_sec = 20;
	tv.tv_usec = 0; 
	while(1)
	{
		if(cmd == 0)  /*get file*/
			status = select(data_sock+1, &actionset, NULL, NULL, &tv); 
		else   /*put file*/
			status = select(data_sock+1, NULL , &actionset, NULL, &tv); 
		
		if(status == 0)
		{
			printf("\nselect socket timeout!(20 second)\n"); 
			close (data_sock);
			ret = -2;
			goto get_close_exit ;
		}
		else if(status == 1)
		{ 
			if(FD_ISSET(data_sock, &actionset))	
			{
				if(cmd == 0)  /*get file*/
				{
					num = read(data_sock , buf ,  FTP_BUFFER_SIZE) ;
					if(num <= 0)
						break ;
					fwrite(buf , 1, num ,  fp) ;
				}
				else     /*put file*/
				{
					num = fread(buf , 1, FTP_BUFFER_SIZE ,	fp) ;
					if(num <= 0)
						break ; 		
					write(data_sock , buf ,  num) ;
				}
			}	
		}
		else
		{
			printf("select socket ERROR(ret = %d)\n" ,status); 
			ret = -5;
			break ;
		}
	}
	printf("\n");
	
	close (data_sock);

	if (ftpReplyGet(ctrl_sock, TRUE) != FTP_COMPLETE)
		ret = -3;

	if (ftpCommand(ctrl_sock, "QUIT", 0, 0, 0, 0, 0, 0) != FTP_COMPLETE)
		ret = -4;
	
get_close_exit :
	
	close (ctrl_sock);
	fclose (fp) ;
	free(buf) ;

	return ret;
}
int	bsp_ftp_get_file(char * ip , char* user, char *password , char * remote_full_name , char * local_full_name)
{
	return bsp_ftp_file(ip, user, password,  remote_full_name, local_full_name, 0) ;
}


int	bsp_ftp_put_file(char * ip , char* user, char *password , char * remote_full_name , char * local_full_name)
{
	return bsp_ftp_file(ip, user, password,  remote_full_name, local_full_name, 1) ;
}

static int load_fs_gzip_file(char * name, char * buffer, int len, int * reallen)
{
	/// first check netring_img.gz
	void * fp= NULL;
	int err ;	
	fp = gzopen(name,"rb");
	if (fp)
	{
		*reallen = gzread(fp,buffer,len) ;		
		gzerror(fp,&err);
		gzclose(fp);
		if (err == Z_STREAM_END ||err == 0) 
			return OK ;
	}
	return ERROR;
}

int bsp_fs_download(char * name, char * buffer, int len, int * reallen)
{
#define BUFFER_LEN  (16*1024)
	int num, status = 0;
	char buf[BUFFER_LEN];
	char * p_desc = buffer;
	
	int fd;

      /*LOAD a gzip bin FILE to a ram buffer*/
	if(load_fs_gzip_file( name, buffer, len, reallen)  == OK)
		return OK ;
	
	/*LOAD a normal bin FILE to a ram buffer*/	
	fd = open (name, O_RDONLY , 0);	

	*reallen = 0;
	while ((num = read (fd, buf, sizeof(buf))) > 0)
	{
		if (*reallen + num > len)
		{
			status = -5;
			break;
		}

		memcpy(p_desc, buf, num);
		p_desc += num;
		*reallen += num;
	}

	close (fd);
	
	if (num	< 0)
		status = -2;
	return status;
}

#endif  /* defined(INCLUDE_BSP_FTP_UPGRADE)*/

/******************************************************************************
*
* This is the Flash operation interface.
*
*/

#ifdef _INCLUDE_IP_SAVEIN_NVRAM_
static int bsp_ifadrs_write_to_xxx(unsigned int num, BSP_IF_ATTR * pif)
{
	char * pifhdr = (char *)pif ;
	int i ;
	
	if (pif == 0 || num >= 3)
		return -1;
	
	/*memcpy(&(pifhdr[num]) , pif , sizeof(BSP_IF_ATTR));*/
	for(i = sizeof(BSP_IF_ATTR) *num ; i < sizeof(BSP_IF_ATTR) *(num+1)  ; i++)
	{
		*NVRAM_ADDR(BSP_IF_NVRAM_OFF+i)  = *pifhdr;
		++pifhdr ;
	}	
	return OK ;
}

static int bsp_ifadrs_read_from_xxx(int num, BSP_IF_ATTR *pif)
{
	char * pifhdr = (char *)pif ;
	int i ;
	
	if (pif == 0 || num >= 3)
		return -1;	
	
	/*memcpy(pif	, &(pifhdr[num]) ,sizeof(BSP_IF_ATTR)); */
	for(i = sizeof(BSP_IF_ATTR) *num ; i < sizeof(BSP_IF_ATTR) *(num+1)  ; i++)
	{
		*pifhdr =  *NVRAM_ADDR(BSP_IF_NVRAM_OFF+i) ;
		++pifhdr ;
	}
	return OK ;
}
#endif /*_INCLUDE_IP_SAVEIN_NVRAM_*/

#ifdef _INCLUDE_IP_SAVEIN_FLASH_
static int bsp_ifadrs_write_to_xxx(unsigned int num, BSP_IF_ATTR * pif)
{
	int status = 0 ; 
	BSP_IF_ATTR oldif[3];
	if (pif == 0 || num >= 3)
		return -1;

	bsp_flash_read( 0 , BSP_IF_OFFSET_ADRS, (char *)oldif ,sizeof(BSP_IF_ATTR)*3) ;

	memcpy((char *) &(oldif[num]) , (char *)pif ,sizeof(BSP_IF_ATTR));
		
	bsp_flash_erase(0 , BSP_IF_OFFSET_ADRS/FLASH_BLOCK_SIZE, 1);/*bios ares's last block*/

	status =  bsp_flash_write( 0 , BSP_IF_OFFSET_ADRS, (char *)oldif ,sizeof(BSP_IF_ATTR)*3) ;
	
	return status ;
}

static int bsp_ifadrs_read_from_xxx(int num, BSP_IF_ATTR *pif)
{
	int status = 0 ; 
	BSP_IF_ATTR oldif[3];
	if (pif == 0 || num >= 3)
		return -1;

	status = bsp_flash_read( 0 , BSP_IF_OFFSET_ADRS, (char *)oldif ,sizeof(BSP_IF_ATTR)*3) ;

	memcpy((char *)pif ,  (char *) &(oldif[num]) ,sizeof(BSP_IF_ATTR) );
	
	return status ;
}

#endif /*_INCLUDE_IP_SAVEIN_FLASH_*/

#if defined(_INCLUDE_IP_SAVEIN_NVRAM_) ||defined(_INCLUDE_IP_SAVEIN_FLASH_)
static unsigned char bsp_byte_crc_sum_cal(unsigned char* pbuf, int len)
{
	char nAccum = 0x5a; 
	int i ;
	for (i = 0; i < len; i++) 
		nAccum = (nAccum ^ (*(char *)(pbuf + i))); 
	return nAccum; 
}

int bsp_ifadrs_check(int num)
{
	char re;
	BSP_IF_ATTR pif ;
	
	bsp_ifadrs_read_from_xxx (num ,  &pif) ;
	re = bsp_byte_crc_sum_cal((unsigned char *)&pif, (sizeof(BSP_IF_ATTR) - 2));
	
	return (pif.crc - re);	
}

/******************************************************************************/
int bsp_ifadrs_get_attr(int num, unsigned long* ip, unsigned long* mask, unsigned char* mac)
{
	BSP_IF_ATTR ifattr;

	if (ip == 0 || mask == 0 || mac == 0)
		return -1;

	if (bsp_ifadrs_check(num) != 0)
		return -4;
	
	if (bsp_ifadrs_read_from_xxx(num, &ifattr) != 0)
		return -2;
	
	*ip = ifattr.ipaddr;
	*mask = ifattr.ipmask;
	bcopy((char *)(ifattr.mac), (char *)mac, 6);
	return 0;
}

int bsp_ifadrs_set_attr(int num, unsigned long ip, unsigned long mask, unsigned char* mac)
{
	BSP_IF_ATTR ifattr;

	if (mac == 0)
		return -1;
	
	if (bsp_ifadrs_read_from_xxx(num , &ifattr) != 0)
		return -2;	
	
	ifattr.ipaddr = ip;
	ifattr.ipmask = mask;
	bcopy((char *)mac, (char *)(ifattr.mac), 6);
	ifattr.crc = bsp_byte_crc_sum_cal((unsigned char *)(&ifattr), (sizeof(BSP_IF_ATTR) - 2));
	if (bsp_ifadrs_write_to_xxx(num, &ifattr) != 0)
		return -2;
	return 0;
}

int bsp_sys_ifadrs_get_attr(unsigned long* ip, unsigned long* mask, unsigned char* mac)
{
	return (bsp_ifadrs_get_attr(0, ip, mask, mac));
}
int bsp_dcc_ifadrs_get_attr(unsigned long* ip, unsigned long* mask, unsigned char* mac)
{
	return (bsp_ifadrs_get_attr(1, ip, mask, mac));
}

int bsp_sys_ifadrs_set_attr(unsigned long ip, unsigned long mask, unsigned char* mac)
{
	return (bsp_ifadrs_set_attr(0, ip, mask, mac));
}
int bsp_dcc_ifadrs_set_attr(unsigned long ip, unsigned long mask, unsigned char* mac)
{
	return (bsp_ifadrs_set_attr(1, ip, mask, mac));
}

int bsp_ifadrs_set_ip_all(char* ip, char* mask, char* mac)
{
	char* token;
	char* pholder;
	unsigned long ipaddr, ipmask;
	unsigned char bmac[6];
	int m;

	ipaddr = inet_addr(ip);
	ipmask = inet_addr(mask);
	token = strtok_r(mac, ":", &pholder);
	for (m = 0; m < 6; m++)
	{
		if (token == 0)
			break;

		bmac[m] = strtol(token, (char **)0, 16);
		token = strtok_r(0, ":", &pholder);
	}
	if (bsp_ifadrs_set_attr(0, ipaddr, ipmask, bmac) != 0)
		return -1;
	return 0;

}

int bsp_ifadrs_set_ip(char* ip)
{
	unsigned long ipaddr, ipmask;
	unsigned char bmac[6];
	
	ipaddr = inet_addr(ip);
	ipmask = 0xffffff00;
	bmac[0] = 0x08;
	bcopy((char *)(&(ipaddr)), (char *)(&(bmac[1])), 4);
	bmac[5] = 0x01;

	if (bsp_ifadrs_set_attr(0 , ipaddr, ipmask, bmac) != 0)
		return -1;
	return 0;
}

int bsp_ifadrs_get_mac(int num , unsigned char * mac)
{
	unsigned long ip, mask;

	if (mac == 0)
		return -1;
	if (bsp_ifadrs_get_attr(num, &ip, &mask, mac) != 0)
		return -2;
	return 0;
}

int bsp_ifadrs_set_dcc_ip(char* ip, unsigned long mask)
{
	unsigned long ipaddr;
	unsigned char bmac[6];
	
	ipaddr = inet_addr(ip);
	bmac[0] = 0x08;
	bcopy((char *)(&(ipaddr)), (char *)(&(bmac[1])), 4);
	bmac[5] = 0x01;

	if (bsp_ifadrs_set_attr(1, ipaddr, mask, bmac) != 0)
		return -1;
	return 0;
}
#endif   /*#ifdef _INCLUDE_IP_SAVEIN_NVRAM_ || #ifdef _INCLUDE_IP_SAVEIN_FLASH_)*/


#if defined(INCLUDE_IO_DOWNLOAD_FPGA)
/*****************************************************************************
*
* FPGA download	io control.
*
*/

#define	__BSP_DRIVER_IO___

#define	BSP_IO_DELAY(timer,	delay)\
{\
	for	(timer = 0;	timer <	delay; timer++);\
}

#define	BSP_IO_BIT(num)		(0x80 >> (num))
#define	BSP_IO_BIT_CYCLONE(num)	(0x01 << (num))


int bsp_io_acs_pin_init(char chass)
{
	unsigned long base;
	BSP_FPGA_CTRL_ID pbctrl;

	if (bsp_chass_get_base_adrs(chass, &base) != 0)
		return -1;

	pbctrl = (BSP_FPGA_CTRL_ID)base;
	pbctrl->data |= CPLD_FPGA_CLK;
	pbctrl->data |= CPLD_FPGA_DOUT;

	return 0;
}

int bsp_io_send_data(char chass, char *adrs, unsigned int len)
{
	unsigned long base ;
	int	bytenum, bitnum	= 0;
	BSP_FPGA_CTRL_ID pbctrl;
	
	if (bsp_chass_get_base_adrs(chass, &base) != 0)
		return -1;

	pbctrl = (BSP_FPGA_CTRL_ID)base;
	for	(bytenum = 0; bytenum <	len; bytenum++)
	{
		for	(bitnum	= 0; bitnum < 8; bitnum++)
		{
			//pbctrl->data = ~CPLD_FPGA_CLK;	
			if ((adrs[bytenum] & BSP_IO_BIT(bitnum)) == BSP_IO_BIT(bitnum))
				pbctrl->data = CPLD_FPGA_DOUT;// 数据为1，同时时钟清0. hz05191
			else
				pbctrl->data = 0	;// 数据为0，同时时钟清0.
			
			pbctrl->data |= CPLD_FPGA_CLK;	
		}
	}
	pbctrl->data = CPLD_FPGA_CLK;
		
	return 0;
}

int bsp_io_send_data_cyclone(char chass, char *adrs, unsigned int len)
{
	unsigned long base;
	int	bytenum, bitnum	= 0;
	BSP_FPGA_CTRL_ID pbctrl;
	
	if (bsp_chass_get_base_adrs(chass, &base) != 0)
		return -1;

	pbctrl = (BSP_FPGA_CTRL_ID)base;
	for	(bytenum = 0; bytenum <	len; bytenum++)
	{
		for	(bitnum	= 0; bitnum < 8; bitnum++)
		{
			if ((adrs[bytenum] & BSP_IO_BIT_CYCLONE(bitnum)) == BSP_IO_BIT_CYCLONE(bitnum))
				pbctrl->data = CPLD_FPGA_DOUT;// 数据为1，同时时钟清0. hz05191
			else
				pbctrl->data = 0x0;// 数据为0，同时时钟清0.
			
			pbctrl->data |= CPLD_FPGA_CLK;	
		}
	}
	pbctrl->data = CPLD_FPGA_CLK;

	return 0;
}

#endif /*INCLUDE_IO_DOWNLOAD_FPGA*/

	
