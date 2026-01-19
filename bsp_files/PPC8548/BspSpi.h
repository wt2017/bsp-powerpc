#ifndef	__BSP_SPI_H__
#define	__BSP_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "proj_config.h"

#define BSP_SPI_OUT_REG         M85XX_GPOUTDR
#define BSP_SPI_IN_REG          (CCSBAR + 0xE0050)   

#define BSP_SPI_MISO_BIT        0X00010000
#define BSP_SPI_MOSI_BIT        0X00010000
#define BSP_SPI_CLK_BIT         0X00020000
#define BSP_SPI_CS_BIT          0X00040000


#define BSP_PCI2_DISABLE        0X40000000

#define SPI_532x_READ   0x60
#define SPI_532x_WRITE  0x61

#define BCM532x_MAX_BYTE_CNT 	32 /*32 byte date buffer of BD */
#define BCM532x_STS_SPIF   		0x80
#define BCM532x_STS_RACK   		0x20



#ifdef __cplusplus
}
#endif

#endif


