/* mot85xxPci.c - Motorola CDS 8548 PCI/PCI Express Bridge functions */

/* Copyright (c) 2005-2006 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,27jan06,dtr  More comments and clean up.
01a,04jun05,dtr  File created from cds85xx/mot85xxPci.c/01f.
*/

/* PCI Host Bridge setup File */

/*
DESCRIPTION
PCI Host controller setup routines. Enables PCI windows to support the main 
pci library functionality including auto config.

INCLUDE FILES: mot85xxPci.h
*/

#include <vxWorks.h>
#include "config.h"
#include <sysLib.h>

#include <drv/pci/pciConfigLib.h> 	
#include <drv/pci/pciIntLib.h> 	
#include <drv/pci/pciAutoConfigLib.h>	
#include "sysBusPci.h"
#include "mot85xxPci.h"

#define MAX_NUM_VECTORS 4

UINT32 pciRegRead(UINT32 *adrs);
void   pciRegWrite(UINT32 *adrs,UINT32 value);

void pciConfigTest();
#define PCI_REG_READ  pciRegRead
#define PCI_REG_WRITE  pciRegWrite


/*************************************************************************
* 
* mot85xxBridgeInit - initialize the PCI bridge
*
* This function performs all the initialisation required for the 
* Bridge/Interrupts/PCI Bus to function. It does some low level processor
* initialisation which might normally be done in romInit as it is optional 
* to do use this and shows what the core changes required to bring up the 
* bridge.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*/

void mot85xxBridgeInit(void)
    {
    STATUS          result;
    volatile UINT32 valRead;

    /* Initialise Local access windows for PCI */

    *M85XX_LAWBAR4(CCSBAR) = PCI_LAW_BASE >> LAWBAR_ADRS_SHIFT;
    *M85XX_LAWAR4(CCSBAR)  = LAWAR_ENABLE | LAWAR_TGTIF_PCI | PCI_LAW_WIN_SZ;

    unsigned long volatile *pulDEVReg = (unsigned long volatile*)M85XX_DEVDISR;        
    *pulDEVReg &= 0X7FFFFFFF;  /*Æô¶¯PCI1*/    

    valRead =  *M85XX_LAWAR4(CCSBAR);
 
 
      WRS_ASM("isync");

      /* Set outbound translation window addresses */

      sysPciConfigEnable (CDS85XX_PCI_1_BUS);
   
      result = sysPciConfigWrite(0,0,0,
				 PCI_CFG_BASE_ADDRESS_0,
				 0x4,
				 PCI_BRIDGE_PIMMR_BASE_ADRS);

      /* for PCI 1 */
#if 1
      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_BASE_ADRS_REG0(CCSBAR)),
		    (CPU_PCI_MEM_ADRS>>12) & 0xfffff);

      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_TRANS_ADRS_REG0(CCSBAR)),
		    (PCI_MEM_ADRS>>12) & 0xfffff);
#if 0
      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_BASE_ADRS_REG1(CCSBAR)),
		    (CPU_PCI_MEMIO_ADRS>>12) & 0xfffff);

      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_TRANS_ADRS_REG1(CCSBAR)),
		    (PCI_MEMIO_ADRS>>12) & 0xfffff);

      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_BASE_ADRS_REG2(CCSBAR)),
		    (CPU_PCI_IO_ADRS>>12) & 0xfffff);

      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_TRANS_ADRS_REG2(CCSBAR)),
		    (PCI_IO_ADRS>>12) & 0xfffff);
#endif
      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_ATTR_REG0(CCSBAR)),
		    PCI_WINDOW_ENABLE_BIT | PCI_OUT_ATTR_RTT_MEM | \
                    PCI_OUT_ATTR_WTT_MEM | PCI_MEM_SIZE_MASK);
#if 0
      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_ATTR_REG1(CCSBAR)),
		    PCI_WINDOW_ENABLE_BIT |PCI_OUT_ATTR_RTT_MEM | \
                    PCI_OUT_ATTR_WTT_MEM | PCI_MEMIO_SIZE_MASK);

      PCI_REG_WRITE((UINT32*)(PCI_OUTBOUND_ATTR_REG2(CCSBAR)),
		    PCI_WINDOW_ENABLE_BIT |PCI_OUT_ATTR_RTT_IO | \
                    PCI_OUT_ATTR_WTT_IO | PCI_IO_SIZE_MASK);
#endif
      /* Switch on the inbound windows */

      PCI_REG_WRITE((UINT32*)(PCI_INBOUND_BASE_ADRS_REG1(CCSBAR)),
		    (PCI_MSTR_MEM_BUS>>12) & 0xfffff);

      PCI_REG_WRITE((UINT32*)(PCI_INBOUND_TRANS_ADRS_REG1(CCSBAR)),
		    (LOCAL_MEM_LOCAL_ADRS>>12) & 0xfffff);
			  
      PCI_REG_WRITE((UINT32*)(PCI_INBOUND_ATTR_REG1(CCSBAR)),
		    PCI_WINDOW_ENABLE_BIT | \
		    PCI_IN_ATTR_RTT_LM_READ_SNOOP | \
                    PCI_IN_ATTR_RTT_LM_WRITE_SNOOP | PCI_IN_ATTR_TGI_LM | 
		    PCI_LOCAL_MEM_SIZE_MASK);
#if 0
      PCI_REG_WRITE((UINT32*)(PCI_INBOUND_ATTR_REG2(CCSBAR)),
		    PCI_IN_ATTR_RTT_LM_READ_SNOOP | \
                    PCI_IN_ATTR_RTT_LM_WRITE_SNOOP | PCI_IN_ATTR_TGI_LM | PCI_LOCAL_MEM_SIZE_MASK);

      PCI_REG_WRITE((UINT32*)(PCI_INBOUND_ATTR_REG3(CCSBAR)),
		    PCI_IN_ATTR_RTT_LM_READ_SNOOP | \
                    PCI_IN_ATTR_RTT_LM_WRITE_SNOOP | PCI_IN_ATTR_TGI_LM | PCI_LOCAL_MEM_SIZE_MASK);
#endif
#endif
      /* configure the bridge as bus master */

      result = sysPciConfigWrite(0,0,0,
				 COMMAND_REGISTER_OFFSET,
				 COMMAND_REGISTER_WIDTH,
				 PCI_CMD_IO_ENABLE |
				 PCI_CMD_MEM_ENABLE | 
				 PCI_CMD_MASTER_ENABLE);

      WRS_ASM("sync;eieio");

 
     }

/************************************************************
* pciRegWrite - write to memory-map PCI registers
*
* This function write ATMU registers for PCI.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*/
 
void pciRegWrite
    (
    UINT32 *adrs,
    UINT32 value
    )
    {
    *adrs = value;
    WRS_ASM("sync;eieio");
    }

/************************************************************
* pciRegRead - read from memory-map PCI registers
*
* This function read ATMU registers for PCI.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*/

UINT32 pciRegRead
    (
    UINT32 *adrs
    )
    {
    return (*adrs);
    }

/***********************************************************
* pciConfigTest - dump on-chip PCI configuration header
*
* This function print out PCI configuration header.
* 
* RETURNS: N/A
*
* ERRNO: N/A
*/

void pciConfigTest
    (
    int pciSys
    )
    {
    int loop;
    UINT32 var;

    switch(pciSys)
	{
	case CDS85XX_PCI_2_BUS:
	case CDS85XX_PCI_1_BUS:
	case CDS85XX_PCIEX_BUS:
	    sysPciConfigEnable(pciSys);
	    break;
	default:
	    sysPciConfigEnable (CDS85XX_PCI_1_BUS);
	    break;
	}

    for(loop = 0; loop < 0x40; loop += 4)
        {
        sysPciConfigRead(0,0,0,loop,0x4,&var);
        printf("Word %d Value %x\n",loop,var);
        }

    }
int BspPciBusTest()
{
    UINT32 var;
    sysPciConfigRead(0,10,0,0,0x4,&var);
    if(var != 0xb62414e4)
    {
        return -1;
    }
    return 0;
}


