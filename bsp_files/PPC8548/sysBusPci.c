/* sysBusPci.c - Motorola CDS8548 platform-specific PCI bus support */

/* Copyright (c) 2005-2006 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,30aug06,dtr  Modify for PCI Express but manual config still required for
                 now.
01b,27jan06,dtr  Tidyup - coding conventions.
01a,04jun05,dtr  created from cds85xx/sysBusPci.c/01b
*/

/*
DESCRIPTION
This is the  platform specific pciAutoConfigLib information.
*/


/* includes */

#include <vxWorks.h>
#include "config.h"
#include <sysLib.h>

#include <drv/pci/pciConfigLib.h> 	
#include <drv/pci/pciIntLib.h> 	
#include <drv/pci/pciAutoConfigLib.h>	
#include "sysBusPci.h"

/* static file scope locals */

IMPORT void sysPciOutLong(UINT32*,UINT32);
IMPORT UINT32 sysPciInLong (UINT32*); 

LOCAL PCI_SYSTEM sysParams ;

#ifdef INCLUDE_CDS85XX_SECONDARY_PCI
LOCAL PCI_SYSTEM sysParams2;
#endif /* INCLUDE_CDS85XX_SECONDARY_PCI */

#ifdef INCLUDE_CDS85XX_PCIEX
LOCAL PCI_SYSTEM sysParams3;
#endif /* INCLUDE_CDS85XX_PCIEX */

/* INT LINE TO IRQ assignment for MPC85xxADS-PCI board. */

LOCAL UCHAR sysPci1IntRoute [NUM_PCI1_SLOTS][4] = {
    {PCI_XINT1_LVL, PCI_XINT2_LVL, PCI_XINT3_LVL, PCI_XINT4_LVL}, /* slot 1 */
    {PCI_XINT2_LVL, PCI_XINT3_LVL, PCI_XINT4_LVL, PCI_XINT1_LVL}, /* slot 2 */
    {PCI_XINT3_LVL, PCI_XINT4_LVL, PCI_XINT1_LVL, PCI_XINT2_LVL}, /* slot 3 */
    {PCI_XINT4_LVL, PCI_XINT1_LVL, PCI_XINT2_LVL, PCI_XINT3_LVL}, /* slot 4 */
};


LOCAL UCHAR sysPci2IntRoute [4] = 
    {PCI2_XINT1_LVL, PCI_XINT2_LVL, PCI_XINT3_LVL, PCI_XINT4_LVL};

LOCAL UCHAR sysPci3IntRoute [NUM_PCIEX_SLOTS][4] = {
    {PCI_XINT1_LVL, PCI_XINT2_LVL, PCI_XINT3_LVL, PCI_XINT4_LVL} /* slot 1 */
};


/*******************************************************************************
*
* sysPciAutoConfig - PCI autoconfig support routine
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void sysPciAutoConfig (void)
    {
    void * pCookie;

    /* PCI 1 Auto configuration */

    sysPciConfigEnable (CDS85XX_PCI_1_BUS);

    /* 32-bit Prefetchable Memory Space */

    sysParams.pciMem32 	        = PCI_MEM_ADRS;
    sysParams.pciMem32Size 	= PCI_MEM_SIZE;

    /* 32-bit Non-prefetchable Memory Space */

    sysParams.pciMemIo32 	= PCI_MEMIO_ADRS;
    sysParams.pciMemIo32Size 	= PCI_MEMIO_SIZE;

    /* 32-bit PCI I/O Space */

    sysParams.pciIo32 		= PCI_IO_ADRS;
    sysParams.pciIo32Size 	= PCI_IO_SIZE;

    /* Configuration space parameters */

    sysParams.maxBus 		= 0;
    sysParams.cacheSize 	= ( _CACHE_ALIGN_SIZE / 4 );
    sysParams.maxLatency 	= PCI_LAT_TIMER;

    /*
     * Interrupt routing strategy
     * across PCI-to-PCI Bridges
     */

    sysParams.autoIntRouting 	= TRUE;

    /* Device inclusion and interrupt routing routines */

    sysParams.includeRtn 	= sysPci1AutoconfigInclude;
    sysParams.intAssignRtn 	= sysPci1AutoconfigIntrAssign;

    /*
     * PCI-to-PCI Bridge Pre-
     * and Post-enumeration init
     * routines
     */

    sysParams.bridgePreConfigInit = NULL;
			/* sysPciAutoconfigPreEnumBridgeInit; */
    sysParams.bridgePostConfigInit = NULL;
			/* sysPciAutoconfigPostEnumBridgeInit; */
    /*
     * Perform any needed PCI Host Bridge
     * Initialization that needs to be done
     * before pciAutoConfig is invoked here 
     * utilizing the information in the 
     * newly-populated sysParams structure. 
     */
 
    pCookie = pciAutoConfigLibInit (NULL);

    pciAutoCfgCtl (pCookie, PCI_PSYSTEM_STRUCT_COPY, &sysParams);
    pciAutoCfg (pCookie);

    /*
     * Perform any needed post-enumeration
     * PCI Host Bridge Initialization here
     * utilizing the information in the 
     * sysParams structure that has been 
     * updated as a result of the scan 
     * and configuration passes. 
     */


#ifdef INCLUDE_CDS85XX_SECONDARY_PCI

    /* PCI 2 Auto configuration */

    sysPciConfigEnable (CDS85XX_PCI_2_BUS);

    /* 32-bit Prefetchable Memory Space */

    sysParams2.pciMem32 	= PCI_MEM_ADRS2;
    sysParams2.pciMem32Size 	= PCI_MEM_SIZE;

    /* 32-bit Non-prefetchable Memory Space */

    sysParams2.pciMemIo32 	= PCI_MEMIO_ADRS2;
    sysParams2.pciMemIo32Size 	= PCI_MEMIO_SIZE;

    /* 32-bit PCI I/O Space */

    sysParams2.pciIo32 		= PCI_IO_ADRS2;
    sysParams2.pciIo32Size 	= PCI_IO_SIZE;

    /* Configuration space parameters */

    sysParams2.maxBus 		= 0;
    sysParams2.cacheSize 	= ( _CACHE_ALIGN_SIZE / 4 );
    sysParams2.maxLatency 	= PCI_LAT_TIMER;

    /*
     * Interrupt routing strategy
     * across PCI-to-PCI Bridges
     */

    sysParams2.autoIntRouting 	= TRUE;

    /* Device inclusion and interrupt routing routines */

    sysParams2.includeRtn 	= sysPci2AutoconfigInclude;
    sysParams2.intAssignRtn 	= sysPci2AutoconfigIntrAssign;

    /*
     * PCI-to-PCI Bridge Pre-
     * and Post-enumeration init
     * routines
     */

    sysParams2.bridgePreConfigInit = NULL;
			/* sysPciAutoconfigPreEnumBridgeInit; */
    sysParams2.bridgePostConfigInit = NULL;
			/* sysPciAutoconfigPostEnumBridgeInit; */
    /*
     * Perform any needed PCI Host Bridge
     * Initialization that needs to be done
     * before pciAutoConfig is invoked here 
     * utilizing the information in the 
     * newly-populated sysParams structure. 
     */

    pCookie = pciAutoConfigLibInit (NULL);
    pciAutoCfgCtl (pCookie, PCI_PSYSTEM_STRUCT_COPY, &sysParams2);
    pciAutoCfg (pCookie);


#endif /* INCLUDE_CDS85XX_SECONDARY_PCI */

#ifdef INCLUDE_CDS85XX_PCIEX

    /* PCI 2 Auto configuration */

    sysPciConfigEnable (CDS85XX_PCIEX_BUS);

    /* 32-bit Prefetchable Memory Space */

    sysParams3.pciMem32 	= PCI_MEM_ADRS3;
    sysParams3.pciMem32Size 	= PCI_MEM_SIZE;

    /* 32-bit Non-prefetchable Memory Space */

    sysParams3.pciMemIo32 	= PCI_MEMIO_ADRS3;
    sysParams3.pciMemIo32Size 	= PCI_MEMIO_SIZE;

    /* 32-bit PCI I/O Space */

    sysParams3.pciIo32 		= PCI_IO_ADRS3;
    sysParams3.pciIo32Size 	= PCI_IO_SIZE;

    /* Configuration space parameters */

    sysParams3.maxBus 		= 0;
    sysParams3.cacheSize 	= ( _CACHE_ALIGN_SIZE / 4 );
    sysParams3.maxLatency 	= PCI_LAT_TIMER;

    /*
     * Interrupt routing strategy
     * across PCI-to-PCI Bridges
     */

    sysParams3.autoIntRouting 	= TRUE;

    /* Device inclusion and interrupt routing routines */

    sysParams3.includeRtn 	= sysPci3AutoconfigInclude;
    sysParams3.intAssignRtn 	= sysPci3AutoconfigIntrAssign;

    /*
     * PCI-to-PCI Bridge Pre-
     * and Post-enumeration init
     * routines
     */

    sysParams3.bridgePreConfigInit = NULL;
			/* sysPciAutoconfigPreEnumBridgeInit; */
    sysParams3.bridgePostConfigInit = NULL;
			/* sysPciAutoconfigPostEnumBridgeInit; */
    /*
     * Perform any needed PCI Host Bridge
     * Initialization that needs to be done
     * before pciAutoConfig is invoked here 
     * utilizing the information in the 
     * newly-populated sysParams structure. 
     */

    pCookie = pciAutoConfigLibInit (NULL);
    pciAutoCfgCtl (pCookie, PCI_PSYSTEM_STRUCT_COPY, &sysParams3);
    pciAutoCfg (pCookie);


#endif /* INCLUDE_CDS85XX_PCIEX */

    }

/*******************************************************************************
*
* sysPci1AutoconfigInclude - PCI 1 autoconfig support routine
*
* RETURNS: OK or ERROR
*/

STATUS sysPci1AutoconfigInclude
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UINT devVend			/* deviceID/vendorID of device */
    )
    {
    
    /* 
     * Only support BUS 0;
     * Host controller itself (device number is 0) won't be configured;
     * Bridge on the Arcadia board (device number 17) won't be configured;
     */ 

    if ((pLoc->bus > 0)                                               ||
        (pLoc->bus == 0 && pLoc->device == 0 && pLoc->function == 0)  
#ifndef ARCADIA_X31
	|| (pLoc->bus == 0 && pLoc->device == 18 && pLoc->function == 0) ||
        (devVend == PCI_ARCADIA_BRIDGE_DEV_ID)
#endif
	) 

        return(ERROR);

    
    return (OK); /* Autoconfigure all devices */
    }

/*******************************************************************************
*
* sysPci2AutoconfigInclude - PCI 2 autoconfig support routine
*
* RETURNS: OK or ERROR
*/

STATUS sysPci2AutoconfigInclude
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UINT devVend			/* deviceID/vendorID of device */
    )
    {
    
    /* 
     * Only support BUS 0;
     * Host controller itself (device number is 0) won't be configured;
     */ 

    if ((pLoc->bus > 0) ||
        (pLoc->bus == 0 && pLoc->device == 0 && pLoc->function == 0))  
        return(ERROR);

    return OK; /* Autoconfigure all devices */
    }

/*******************************************************************************
*
* sysPci3AutoconfigInclude - PCI Express autoconfig support routine
*
* RETURNS: OK or ERROR
*/

STATUS sysPci3AutoconfigInclude
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UINT devVend			/* deviceID/vendorID of device */
    )
    {
    
    /* 
     * Only support BUS 0;
     * Host controller itself (device number is 0) won't be configured;
     */ 

    if ((pLoc->bus > 2) ||
        (pLoc->bus == 0 && pLoc->device == 0 && pLoc->function == 0) ||
	(pLoc->device > 0))  
        return(ERROR);

    return OK; /* Autoconfigure all devices */
    }

/*******************************************************************************
*
* sysPci1AutoconfigIntrAssign - PCI 1 autoconfig support routine
*
* RETURNS: PCI interrupt line number given pin mask
*/

UCHAR sysPci1AutoconfigIntrAssign
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UCHAR pin				/* contents of PCI int pin register */
    )
    {
    UCHAR tmpChar = 0xff;

    /* 
     * Ensure this is a reasonable value for bus zero.
     * If OK, return INT level, else we return 0xff.
     */
    if (((pin > 0) && (pin < 5))       				&& 
	(((pLoc->device) - PCI_SLOT1_DEVNO) < NUM_PCI1_SLOTS) 	&&
	(((pLoc->device) - PCI_SLOT1_DEVNO) >= 0))
	{
	tmpChar = 
	    sysPci1IntRoute [((pLoc->device) - PCI_SLOT1_DEVNO)][(pin-1)];
	}

    /* return the value to be assigned to the pin */

    return (tmpChar);
    }

/*******************************************************************************
*
* sysPci2AutoconfigIntrAssign - PCI 2 autoconfig support routine
*
* RETURNS: PCI interrupt line number given pin mask
*/

UCHAR sysPci2AutoconfigIntrAssign
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UCHAR pin				/* contents of PCI int pin register */
    )
    {
    UCHAR tmpChar = 0xff;

    /* 
     * Ensure this is a reasonable value for bus zero.
     * If OK, return INT level, else we return 0xff.
     */
    if ((pin > 0) && (pin < 5))	
	tmpChar = sysPci2IntRoute [(pin-1)];

    /* return the value to be assigned to the pin */

    return (tmpChar);
    }

/*******************************************************************************
*
* sysPci3AutoconfigIntrAssign - PCI Express autoconfig support routine
*
* RETURNS: PCI interrupt line number given pin mask
*/

UCHAR sysPci3AutoconfigIntrAssign
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UCHAR pin				/* contents of PCI int pin register */
    )
    {
    UCHAR tmpChar = 0xff;

    /* 
     * Ensure this is a reasonable value for bus zero.
     * If OK, return INT level, else we return 0xff.
     */
    if ((pin > 0) && (pin < 5))
	{
	tmpChar = 
	    sysPci3IntRoute [0][(pin-1)];
	}

    /* return the value to be assigned to the pin */

    return (tmpChar);
    }

/*******************************************************************************
*
* sysPciAutoconfigPreEnumBridgeInit - PCI autoconfig support routine
*
* RETURNS: N/A
*/


void sysPciAutoconfigPreEnumBridgeInit
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UINT devVend			/* deviceID/vendorID of device */
    )
    {
    return;
    }


/*******************************************************************************
*
* sysPciAutoconfigPostEnumBridgeInit - PCI autoconfig support routine
*
* RETURNS: N/A
*/

void sysPciAutoconfigPostEnumBridgeInit
    (
    PCI_SYSTEM * pSys,			/* PCI_SYSTEM structure pointer */
    PCI_LOC * pLoc,			/* pointer to function in question */
    UINT devVend			/* deviceID/vendorID of device */
    )
    {
    return;
    }








