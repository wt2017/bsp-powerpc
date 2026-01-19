/* vmGlobalMap.c - virtual memory global mapping library */

/* 
 * Copyright (c) 1984-2005 Wind River Systems, Inc. 
 *
 * The right to copy, distribute, modify or otherwise make use 
 * of this software may be licensed only pursuant to the terms 
 * of an applicable Wind River license agreement. 
 */ 

/*
modification history
--------------------
01c,10oct05,h_k  added missing cacheMmuAvailable initialization. (SPR #113421)
01b,15aug05,rhe  Replaced ksprintf with sprintf
01a,08aug05,zl   moved out from vmBaseLib.
*/

/* 
DESCRIPTION
This library provides the minimal virtual memory functionality for initializing
global MMU mappings for the kernel. These mappings are created at system startup
based on the system memory descriptor table - sysPhysmemDesc[] - of the BSP.
This functionality is enabled with the INCLUDE_VM_GLOBAL_MAP component.

The mappings can be modified at runtime only if the vmBaseLib API is included
in the system with the INCLUDE_MMU_BASIC component.

The physical memory descriptor contains information used to initialize 
the MMU attribute information in the translation table. The following state 
bits may be or'ed together:

Supervisor access:
\ts
MMU_ATTR_PROT_SUP_READ | read access in supervisor mode
MMU_ATTR_PROT_SUP_WRITE | write access in supervisor mode
MMU_ATTR_PROT_SUP_EXE | executable access in supervisor mode
\te
User access:
\ts
MMU_ATTR_PROT_USR_READ | read access in user mode
MMU_ATTR_PROT_USR_WRITE | write access in user mode
MMU_ATTR_PROT_USR_EXE | executable access in user mode
\te
Validity attribute:
\ts
MMU_ATTR_VALID | page is valid.
\te
Cache attributes:
\ts
MMU_ATTR_CACHE_OFF | cache turned off
MMU_ATTR_CACHE_COPYBACK | cache in copy-back mode
MMU_ATTR_CACHE_WRITETHRU | cache set in writethrough mode
MMU_ATTR_CACHE_GUARDED | page access set to guarded. 
MMU_ATTR_CACHE_COHERENCY | page access set to cache coherent. 
MMU_ATTR_CACHE_DEFAULT | default cache value, set with USER_D_CACHE_MODE
\te

Additionally, mask bits are or'ed together in the `initialStateMask' structure
element to describe which state bits are being specified in the `initialState'
structure element:

 MMU_ATTR_PROT_MSK
 MMU_ATTR_CACHE_MSK
 MMU_ATTR_VALID_MSK

If there is an error when mappings are initialized, the reason for the failure 
is recorded in sysExcMsg area and the system is rebooted. The most common reasons
for failures are the invalid combination of state/stateMask, or overlapping 
and conflicting entries in sysPhysmemDec[].

SEE ALSO: vmBaseLib
*/

#include <vxWorks.h>

#if !defined (_WRS_DISABLE_BASEVM)

#include <mmuLib.h>
#include <private/vmLibP.h>
#include <sysLib.h>
#include <stdio.h>
#include <cacheLib.h>

/* imports */

extern MMU_LIB_FUNCS	mmuLibFuncs;		/* initialized by mmuLib.c */
extern int		mmuPhysAddrShift;	/* see vmData.c */

extern VOIDFUNCPTR      _func_cacheFuncsSet;


/* globals */

FUNCPTR			pVmInvPageMapRtn = NULL;/* for vmInvTblLib.c */
VOIDFUNCPTR		pVmBaseFuncsSet = NULL; /* for vmbaseLib */

UINT32			mmuInvalidState;	/* initialized by mmuLib.c */
UINT *			mmuProtStateTransTbl;	/* initialized by mmuLib.c */
UINT *			mmuCacheStateTransTbl;	/* initialized by mmuLib.c */
UINT *			mmuValidStateTransTbl;	/* initialized by mmuLib.c */
UINT *			mmuMaskTransTbl;	/* initialized by mmuLib.c */
VM_CONTEXT		sysVmContext;		/* kernel vm context */

/* locals */

LOCAL int		vmPageSize;



/* forward declarations */

LOCAL STATUS	vmBaseGlobalMap (VIRT_ADDR virtualAddr, 
				 PHYS_ADDR physicalAddr,
				 UINT len);
STATUS		convertAttrFromArchIndepToDep (UINT32 indepStateMask, 
					       UINT32 indepState,
					       UINT32 *pDepStateMask, 
					       UINT32 *pDepState);

/******************************************************************************
*
* vmGlobalMapInit - initialize global mapping
*
* This routine creates and installs a virtual memory context with mappings
* defined for each contiguous memory segment defined in <pMemDescArray>.
* In the standard VxWorks configuration, an instance of PHYS_MEM_DESC (called
* `sysPhysMemDesc') is defined in sysLib.c; the variable is passed to
* vmGlobalMapInit() by the system configuration mechanism.
*
* This routine is called only once during system initialization. It
* should never be called by application code.
*
* If <enable> is TRUE, the MMU is enabled upon return.
*
* RETURNS: A pointer to a newly created virtual memory context, or NULL if
* memory cannot be mapped.
*
* SEE ALSO: vmBaseLibInit()
*/

VM_CONTEXT_ID vmGlobalMapInit
    (
    PHYS_MEM_DESC * pMemDescArray, 	  /* pointer to array of mem descs */
    int		    numDescArrayElements, /* no. of elements in pMemDescArray */
    BOOL	    enable,		  /* enable virtual memory */
    int		    cacheDefault	  /* default data cache mode */
    )
    {
    int		    ix;
    PHYS_MEM_DESC * thisDesc;
    UINT	    archDepStateMask; 
    UINT	    archDepState;
    UINT	    mmuDefaultCacheIndex;

    vmPageSize = MMU_PAGE_SIZE_GET();

    /* 
     * Initialize the cache default entry.
     * The index to the cache is what is stored.
     */

    if (cacheDefault & CACHE_WRITETHROUGH)
	{
        mmuDefaultCacheIndex = STATE_TO_CACHE_INDEX (MMU_ATTR_CACHE_WRITETHRU);
	}
    else if (cacheDefault & CACHE_COPYBACK)
	{
        mmuDefaultCacheIndex = STATE_TO_CACHE_INDEX (MMU_ATTR_CACHE_COPYBACK);
	}
    else
	{
	mmuDefaultCacheIndex = STATE_TO_CACHE_INDEX (MMU_ATTR_CACHE_OFF);
	}


    mmuCacheStateTransTbl [STATE_TO_CACHE_INDEX(MMU_ATTR_CACHE_DEFAULT)] =
			   mmuDefaultCacheIndex;

    /* create translation table for default virtual memory context */

    sysVmContext.mmuTransTbl = MMU_TRANS_TBL_CREATE ();

    if (sysVmContext.mmuTransTbl == NULL)
	return (NULL);

    /* set up kernel mapping of physical to virtual memory */

    for (ix = 0; ix < numDescArrayElements; ix++)
        {
        thisDesc = &pMemDescArray[ix];

	/* if physical address is NONE, ignore entry */

	if (thisDesc->physicalAddr == (PHYS_ADDR) NONE)
	    continue;

        /* map physical directly to virtual and set initial state */

        if (vmBaseGlobalMap (thisDesc->virtualAddr,
			     thisDesc->physicalAddr,
			     thisDesc->len) == ERROR)
            {
            sysExcMsg += sprintf (sysExcMsg, "vmGlobalMapInit: mapping "
                                  "failed - sysPhysMemDesc[%d]\n", ix);
	    return (NULL);
            }

	/* set the state of the global mapping we just created */

	if (convertAttrFromArchIndepToDep (thisDesc->initialStateMask,
					   thisDesc->initialState,
    				           &archDepStateMask,
				           &archDepState) == ERROR)
	    {
	    sysExcMsg += sprintf (sysExcMsg, "vmGlobalMapInit: invalid "
				  "attributes - sysPhysMemDesc[%d]\n", ix);
	    return (NULL);
	    }


        if (MMU_STATE_SET (sysVmContext.mmuTransTbl, 
			   thisDesc->virtualAddr,
                           thisDesc->len / vmPageSize, 
			   archDepStateMask, archDepState, 0) == ERROR)
	    {
	    sysExcMsg += sprintf (sysExcMsg, "vmGlobalMapInit: state setting "
				  "failed - sysPhysMemDesc[%d]\n", ix);
            return (NULL);
	    }
        }

    MMU_CURRENT_SET (sysVmContext.mmuTransTbl);

    /* update cache function pointers */

    cacheMmuAvailable             = TRUE;

    if (_func_cacheFuncsSet != NULL)
	_func_cacheFuncsSet ();

    if (pVmBaseFuncsSet != NULL)
	{
	pVmBaseFuncsSet();
	}

    /* enable MMU if needed */

    if (enable)
	if (MMU_ENABLE (TRUE) == ERROR)
	    return (NULL);

    return (&sysVmContext);
    }

/******************************************************************************
*
* vmBaseGlobalMapInit - initialize global mapping (obsolete)
*
* This function will be replaced by vmGlobalMapInit()
*
* RETURNS: A pointer to a newly created virtual memory context, or NULL if
* memory cannot be mapped.
*
* SEE ALSO: vmBaseLibInit(), vmGlobalMapInit()
*/

VM_CONTEXT_ID vmBaseGlobalMapInit
    (
    PHYS_MEM_DESC *pMemDescArray,       /* pointer to array of mem descs */
    int numDescArrayElements,           /* no. of elements in pMemDescArray */
    BOOL enable,                        /* enable virtual memory */
    int	cacheDefault	  		/* default data cache mode */
    )
    {
    return (vmGlobalMapInit (pMemDescArray, numDescArrayElements, enable,
			     cacheDefault));
    }

/****************************************************************************
*
* vmBaseGlobalMap - map physical to virtual in shared global virtual memory
*
* vmBaseGlobalMap maps physical pages to virtual space that is shared by all
* virtual memory contexts.  Calls to vmBaseGlobalMap should be made before any
* virtual memory contexts are created to insure that the shared global mappings
* will be included in all virtual memory contexts.  Mappings created with 
* vmBaseGlobalMap after virtual memory contexts are created are not guaranteed 
* to appear in all virtual memory contexts.
*
* RETURNS: OK, or ERROR if <virtualAddr> or physical page addresses are not on
* page boundaries, <len> is not a multiple of page size, or mapping failed.
*
* NOMANUAL
*/

LOCAL STATUS vmBaseGlobalMap 
    (
    VIRT_ADDR  virtualAddr, 
    PHYS_ADDR  physicalAddr, 
    UINT len
    )
    {
    /* check parameters */

    if ((NOT_PAGE_ALIGNED (virtualAddr)) || 
	(NOT_PAGE_ALIGNED (physicalAddr) && !mmuPhysAddrShift) ||
	(NOT_PAGE_ALIGNED (len)))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    /* map pages */

    if (MMU_GLOBAL_PAGE_MAP (virtualAddr, physicalAddr,
			     len / vmPageSize, 0) == ERROR)
	{
	return (ERROR);
	}

    /* increment the physical page mapping in the inverse page table */

    if ((pVmInvPageMapRtn != NULL) &&
	(pVmInvPageMapRtn (physicalAddr, len, FALSE) == ERROR))
	{
	return (ERROR);
	}

    return (OK);
    }

/****************************************************************************
*
* convertAttrFromArchIndepToDep - convert MMU state from architecture
*                                 independent value to architecture dependent
*
* This routine convert the architecture-independent MMU state value to its
* corresponding architecture-dependent  value.
*
* RETURNS: sets <*pDepStateMask> and <*pDepState> to the architecture-dependent
*          MMU mask and state, respectively.
*
* ERRNO:
* \is
* \i S_vmLib_BAD_STATE_PARAM 
* <state> is not a valid combination of MMU states.
* \i S_vmLib_BAD_MASK_PARAM 
* <stateMask> is not a valid combination of MMU state masks.
* \ie
*
* \NOMANUAL
*/  

STATUS convertAttrFromArchIndepToDep 
    (
    UINT32 indepStateMask,   /* Arch independant MMU state Mask.             */
    UINT32 indepState,       /* Arch independant MMU state.                  */
    UINT32 *pDepStateMask,   /* place to return arch depentant state Mask.   */
    UINT32 *pDepState        /* place to return arch depentant state.        */ 
    )
    {

    UINT archDepStateMask  = 0;
    UINT archDepState      = 0;
    UINT archDepProtState  = 0;
    UINT archDepCacheState = 0;
    UINT archDepValidState = 0;
    BOOL validMask	   = FALSE;
    UINT32 defaultIndex    = 0;
    UINT32 index           = 0;

    if ((indepStateMask & MMU_ATTR_PROT_MSK) == MMU_ATTR_PROT_MSK)
	{
	validMask = TRUE;

	archDepProtState = mmuProtStateTransTbl[STATE_TO_PROT_INDEX(indepState)];

	/* if returned an invalid state, or SUP READ is not set return error */

	if ((archDepProtState == mmuInvalidState) ||
	    (~indepState & MMU_ATTR_PROT_SUP_READ))
	    {
	    errno = S_vmLib_BAD_STATE_PARAM;
	    return (ERROR);
	    }
	else
	    archDepState = archDepProtState;
	}

    /*
     * if necessary get architecture dependent cache state and
     * check its validity
     */

    if ((indepStateMask & MMU_ATTR_CACHE_MSK) == MMU_ATTR_CACHE_MSK)
	{
	validMask = TRUE;

	/*
	 * MMU_ATRR_CACHE_DEFAULT must not be set in conjunction with any
	 * other cache settings
	 */

	if ((indepState & MMU_ATTR_CACHE_DEFAULT) &&
	    (indepState & (MMU_ATTR_CACHE_OFF | MMU_ATTR_CACHE_COPYBACK)))
	    {
	    errno = S_vmLib_BAD_STATE_PARAM;
	    return (ERROR);
	    }

            defaultIndex = 0;

        if (indepState & MMU_ATTR_CACHE_DEFAULT)
            defaultIndex =
            mmuCacheStateTransTbl [STATE_TO_CACHE_INDEX(MMU_ATTR_CACHE_DEFAULT)];

        if(defaultIndex == mmuInvalidState)
            {
            errno = S_vmLib_BAD_STATE_PARAM;
            return (ERROR);
            }

        if ((indepState & MMU_ATTR_CACHE_MSK) & ~(MMU_ATTR_CACHE_DEFAULT))
            index = defaultIndex 
                    + STATE_TO_CACHE_INDEX(((indepState & MMU_ATTR_CACHE_MSK) &
                      ~(MMU_ATTR_CACHE_DEFAULT)));
        else
            index = defaultIndex;

        archDepCacheState = mmuCacheStateTransTbl [index];


	if(archDepCacheState == mmuInvalidState)
	    {
	    errno = S_vmLib_BAD_STATE_PARAM;
	    return (ERROR);
	    }

	else
	    archDepState |= archDepCacheState;
	}
    /*
     * if necessary get architecture dependent valid state and
     * check its validity
     */

    if ((indepStateMask & MMU_ATTR_VALID_MSK) == MMU_ATTR_VALID_MSK)
	{
	validMask = TRUE;

	archDepValidState =
		    mmuValidStateTransTbl [STATE_TO_VALID_INDEX(indepState)];

	/* if returned an invalid state, or SUP READ is not set return error */

	if (archDepValidState == mmuInvalidState)
	    {
	    errno = S_vmLib_BAD_MASK_PARAM;
	    return (ERROR);
	    }

	else
	    archDepState |= archDepValidState;
	}

    /* get the architecture dependent state mask */

    if (validMask == TRUE)
        {
        archDepStateMask = mmuMaskTransTbl [STATEMASK_TO_INDEX(indepStateMask)];
        }

    /*
     * If this architecture uses special purpose attributes then use
     * the architecture specific translation mechanism.
     */

    if ( !( ((indepStateMask & MMU_ATTR_SPL_MSK) != MMU_ATTR_SPL_MSK)
        && (validMask == FALSE) ))
        {
        if (mmuLibFuncs.mmuAttrTranslate != NULL)
	    {
	    if (MMU_ATTR_TRANSLATE(indepState, &archDepState, indepStateMask,
	     		   &archDepStateMask, TRUE) == ERROR)
	        {
	        errno = S_vmLib_BAD_STATE_PARAM;
	        return (ERROR);
	        }
	    }
        }
    else if (validMask != TRUE)
	{
	errno = S_vmLib_BAD_STATE_PARAM;
	return (ERROR);
	}

    *pDepStateMask = archDepStateMask;
    *pDepState     = archDepState;

    return (OK);
    }
#endif	/* !_WRS_DISABLE_BASEVM */
