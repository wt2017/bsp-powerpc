/* sysMotFccEnd.c - system configuration module for motFccEnd driver */
 
/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"
 
/*
modification history
--------------------
01e,07mar02,kab  SPR 70817: *EndLoad returns NULL on failure
01d,17oct01,jrs  Upgrade to veloce
		 changed INCLUDE_MOT_FCC to INCLUDE_MOTFCCEND - SPR #33914
01c,16sep99,ms_  get miiLib.h from h/drv instead of locally
01b,15jul99,ms_  make compliant with our coding standards
01a,05apr99, cn  created from ads860/sysMotCpmEnd.c
*/
 
/*
DESCRIPTION
This is the WRS-supplied configuration module for the VxWorks
motFccEnd END driver.

It performs the dynamic parameterization of the motFccEnd driver.
This technique of 'just-in-time' parameterization allows driver
parameter values to be declared as any other defined constants rather
than as static strings.
*/

#include "vxWorks.h"
#include "config.h"
#include "vmLib.h"
#include "stdio.h"
#include "sysLib.h"
#include "logLib.h"
#include "stdlib.h"
#include "string.h"
#include "end.h"
#include "intLib.h"
#include "miiLib.h"
#include "drv/timer/m8260Clock.h"
#include "drv/parallel/m8260IOPort.h"

#ifdef INCLUDE_MII
#include "miiLib.c"
#endif /*INCLUDE_MII*/



#define MOT_FCC_NUM1		0x01    /* FCC1 being used */ 
#define MOT_FCC_NUM2		0x02    /* FCC2 being used */ 

#define MOT_FCC_BD_BASE(fcc_num)  (vxImmrGet() + FCC_BD_OFF(fcc_num))
#define  MOT_FCC_RIPTR_VAL(fcc_num)       (MOT_FCC_PTR_OFF(fcc_num) + 0x00)         /*FCC rx FIFO pointer value */
#define  MOT_FCC_TIPTR_VAL(fcc_num)       (MOT_FCC_PTR_OFF(fcc_num)  + 0x40)         /*FCC tx FIFO pointer value */
#define  MOT_FCC_PADPTR_VAL(fcc_num)     (MOT_FCC_PTR_OFF(fcc_num) + 0x60)         /*FCC padPtr pointer value */

#define MOT_FCC_FLAGS			0x00	/* auto,half duplex and monitor on*/
#define FCC_TBD_NUM		        0x40	/* transmit buffer descriptors (TBD)*/
#define FCC_RBD_NUM		        0x20	/* receive buffer descriptors (RBD)*/

#define FCC1_PHY_ADDR       	0xf	/* PHY 1 address */
#define FCC2_PHY_ADDR       	0x1	/* PHY 2 address */

#define FCC_DEF_PHY_MODE	    0x00	/* PHY's default operating mode,10M,half */

/* defines */
#ifdef INCLUDE_MOTFCCEND

#if defined (_WRS_VXWORKS_MAJOR) && (_WRS_VXWORKS_MAJOR >= 6)
#include "motFcc2End_vx6.c"
#else
#include "motFcc2End.c"
#endif

/* locals */
/*
 * this table may be customized by the user to force a
 * particular order how different technology abilities may be
 * negotiated by the PHY. Entries in this table may be freely combined
 * and even OR'd together.
 */
 
LOCAL INT16 motFccAnOrderTbl [] = {
                                MII_TECH_100BASE_TX,    /* 100Base-T */
                                MII_TECH_100BASE_T4,    /* 10Base-T */
                                MII_TECH_10BASE_T,      /* 100Base-T4 */
                                MII_TECH_10BASE_FD,     /* 100Base-T FD*/
                                MII_TECH_100BASE_TX_FD, /* 10Base-T FD */
                                -1                      /* end of table */
                               };

 
LOCAL FCC_END_FUNCS motFccEndFuncTbl = {
   (FUNCPTR) NULL, //miiPhyInit, // FUNCPTR miiPhyInit;       /* bsp MiiPhy init function */
   (FUNCPTR) NULL,  //  FUNCPTR miiPhyInt;        /* Driver function for BSP to call */
   (FUNCPTR) NULL,  //  FUNCPTR miiPhyBitRead;    /* Bit Read funtion */
   (FUNCPTR) NULL,  //  FUNCPTR miiPhyBitWrite;   /* Bit Write function */
   (FUNCPTR) NULL, //miiPhyDuplex , //  FUNCPTR miiPhyDuplex;     /* duplex status call back */
   (FUNCPTR) NULL,  //  FUNCPTR miiPhySpeed;      /* speed status call back */
   (FUNCPTR) NULL,  //  FUNCPTR hbFail;           /* heart beat fail */
   (FUNCPTR) NULL,  //  FUNCPTR intDisc;          /* disconnect Function */
   (FUNCPTR) NULL,  //  FUNCPTR dpramFree;
   (FUNCPTR) NULL,  //  FUNCPTR dpramFccMalloc;
   (FUNCPTR) NULL,  //  FUNCPTR dpramFccFree;
   {(FUNCPTR)NULL, (FUNCPTR)NULL, (FUNCPTR)NULL , (FUNCPTR)NULL }  // FUNCPTR reserved[4];      /* future use */
    };
/******************************************************************************
*
* sysMotFccEndLoad - load an istance of the motFccEnd driver
*
* This routine loads the motFccEnd driver with proper parameters. It also
* reads the BCSR3 to find out which type of processor is being used, and
* sets up the load string accordingly.
*
* The END device load string formed by this routine is in the following
* format.
* <immrVal>:<fccNum>:<bdBase>:<bdSize>:<bufBase>:<bufSize>:<fifoTxBase>
* :<fifoRxBase>:<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<pAnOrderTbl>:
* <userFlags>
*
* .IP <immrVal>
* Internal memory address
* .IP <fccNum>
* FCC number being used
* .IP <bdBase>
* buffer descriptors base address
* .IP <bdSize>
* buffer descriptors space size
* .IP <bufBase>
* data buffers base address
* .IP <bufSize>
* data buffers space size
* .IP <fifoTxBase>
* tx buffer in internal memory
* .IP <fifoRxBase>
* rx buffer in internal memory
* .IP <tbdNum>
* number of TBDs or NONE
* .IP <rbdNum>
* number of RBDs or NONE
* .IP <phyAddr>
* address of a MII-compliant PHY device
* .IP <phyDefMode>
* default operating mode of a MII-compliant PHY device
* .IP <pAnOrderTbl>
* auto-negotiation order table for a MII-compliant PHY device or NONE
<userFlags>
..........
<function table>
..........

*
* This routine only loads and initializes one instance of the device.
* If the user wishes to use more than one motFccEnd devices, this routine
* should be changed.
*
* RETURNS: pointer to END object or NULL.
*
* SEE ALSO: motFccEndLoad ()
*/
 
END_OBJ * sysMotFccEndLoad
    (
    char * pParamStr,   /* ptr to initialization parameter string */
    void * bsp_optional       /* used as motfcc Num*/
    )
    {
    /* this is some difference between OS package and this soucecode (see motFcc2End.c::motFccInitParse() fuction)
     * The motFcc2End driver END_LOAD_STRING should be:
       * <immrVal>:<fccNum>:<bdBase>:<bdSize>:<bufBase>:<bufSize>:<fifoTxBase>
     * :<fifoRxBase>:<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<pAnOrderTbl>:<userFlags>: <function table>
     */


    char * pStr = NULL;
    char paramStr [300];
    LOCAL char motFccEnd2ParamTemplate [] = 
		    "0x%x:0x%x:0x%x:0x%x:-1:-1:-1:-1:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x";

    END_OBJ * pEnd;

    bsp_clear_dog();

    if (strlen (pParamStr) == 0)
        {
        /*
         * muxDevLoad() calls us twice.  If the string is
         * zero length, then this is the first time through
         * this routine.
         */
        pEnd = (END_OBJ *) motFcc2EndLoad  (pParamStr);
        }
    else
        {
        /*
         * On the second pass through here, we actually create
         * the initialization parameter string on the fly.
         * Note that we will be handed our unit number on the
         * second pass and we need to preserve that information.
         * So we use the unit number handed from the input string.
         */		
	 pStr = strcpy (paramStr, pParamStr);
 
        /* Now, we get to the end of the string */
 
        pStr += strlen (paramStr);
 
        /* finish off the initialization parameter string */
		if((int)bsp_optional == 1)
			{
	        sprintf (pStr, motFccEnd2ParamTemplate,
				 (UINT) vxImmrGet (),
				 MOT_FCC_NUM1,
				 MOT_FCC_BD_BASE(MOT_FCC_NUM1),
				 FCC_BD_SIZE,
				 FCC_TBD_NUM,  	
				 FCC_RBD_NUM,  	
				 FCC1_PHY_ADDR,  	
				 FCC_DEF_PHY_MODE,  	
				 &motFccAnOrderTbl,
				 MOT_FCC_FLAGS,
				 motFccEndFuncTbl 
				 );
			}
		else if((int)bsp_optional == 2)
			{
	        sprintf (pStr, motFccEnd2ParamTemplate,
				 (UINT) vxImmrGet (),
				 MOT_FCC_NUM2,
				 MOT_FCC_BD_BASE(MOT_FCC_NUM2),
				 FCC_BD_SIZE,
				 FCC_TBD_NUM,  	
				 FCC_RBD_NUM,  	
				 FCC2_PHY_ADDR,  	
				 FCC_DEF_PHY_MODE,  	
				 &motFccAnOrderTbl,
				 MOT_FCC_FLAGS,
				 motFccEndFuncTbl 
				 );
			}
 
        if ((pEnd = (END_OBJ *) motFcc2EndLoad  (paramStr)) == (END_OBJ *)NULL)
            {
            logMsg ("Error: motFccEndLoad  failed to load driver\n",
		    0, 0, 0, 0, 0, 0);
            }
        }
 
    return (pEnd);
    }

    
#endif /* INCLUDE_MOT_FCC */

