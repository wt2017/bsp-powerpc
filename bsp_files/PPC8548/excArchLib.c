/* excArchLib.c - PowerPC exception handling facilities */

/* 
 * Copyright (c) 1984-2006 Wind River Systems, Inc. 
 *
 * The right to copy, distribute, modify or otherwise make use 
 * of this software may be licensed only pursuant to the terms 
 * of an applicable Wind River license agreement. 
 */ 

/*
modification history
--------------------
05a,23mar06,ajc	 SPR 119277: update sysExcMsg pointer
04z,07feb06,gls  added faultAddr argument to rtpSigExcKill
04y,24jan06,ajc	 SPR 111763: fix logging of sysExcMsg
04x,11jan06,bpn  Move breakpoint instruction test in excExcHandle() to 
                 wdbDbgArchLib.c.
04w,17oct05,pch  SPR 113899: exc/intConnectCode must restore 32-bit mode
                 before calling exc/intEnt
04v,10oct05,pch  SPR 113560: add decrementer stub for PPC970
04u,15sep05,pch  SPR 112332: fix vector calculation when excVecBase != 0
04t,16aug05,rhe  Removed kprintf
04s,11aug05,md   Simplify number of ED&R scalability options
04r,09aug05,mil  Fixed branch displacement for relocated vectors (SPR111067)
04q,27jul05,md   rework ED&R stubs for scalability
04p,25jul05,mil  Converted _WRS_LAYER to use feature macros.
04o,13jul05,mil  Consolidated all connect funcs to excVecConnectCommon.
04n,08jun05,yvp  Reverted excExcepHook.
04m,07jun05,yvp  Updated copyright. #include now with angle-brackets.
04l,27apr05,yvp  Added _KSL_ based conditional around call to taskIdDefault
04k,18mar05,yvp  Changed excExcepHook to _func_excExcepHook.
04j,28dec04,yvp  Added check on _func_ioGlobalStdGet. Convert all printExc
                 to kprintf. Conditionalized ED&R error inject stubs.
04i,04mar05,pch  add _EXC_OFF_FPU for PPC440
04h,09feb05,pch  SPR 102772: handle interrupt stack protection
04g,22dec04,mwj  SPR 90443/105257/105500: save FPCSR before re-enabling
		 interrupts
04f,21apr04,to   PPC970 cleanup
04e,17mar04,rec  merge in T2 PPC970
04d,20oct04,pch  SPR 92598: handle exception stack overflow
		 allow for LOG_DISPATCH on all processor types
04c,13oct04,kk   updated kernelBaseLib.h to private/kernelBaseLibP.h
04b,07oct04,mil  Fixed excVecSet/Get using wrong offset for mchk type.
		 (SPR#102348)
04a,06oct04,pch  Renumber history from 03k
03z,28sep04,md   move _func_excPanicHook to ED&R stub
03y,23sep04,dtr  Dependencies. SPR 100373
03x,22sep04,pch  Add debug/trace capabilities (under #ifdef)
03w,10aug04,kk   renamed rtpBaseLib.h to kernelBaseLib.h
03v,08aug04,aeg  updated excExcHandle() comments re: _func_excBaseHook.
03u,03aug04,pch  SPR's 78780 & 100398: support use of standard exception
		 vectors in MMU and syscall
03t,09jun04,pch  add support for PPC440 with hard FP
03s,04jun04,mil  Changed cacheArchXXX funcs to cacheXXX funcs.
03r,18may04,dtr  Fix L1 Parity exception handler to use area above exception
		 stack by default.
03q,26apr04,dtr  Added excMsg to generic exception handler.
03p,19apr04,elg  Pass more information to EDR.
03o,23mar04,dtr  Add E500 machine check L1 parity error recovery support.
		 Added Vector table write protect code.
03n,17mar04,md   update parameters for _func_excInfoShow
03m,13feb04,md   reworked ED&R macros
03l,08dec03,pch  Move vector-table typedefs to new excArchLibP.h
03k,09dec03,dbs  add missing call to excExcepHook
03j,08dec03,dbs  fix ordering of signal-delivery and excInfoShow
03i,05dec03,jtp  excScConnect merged to excScALib.s; remove remnants
03h,02dec03,dbs  fix RTP signal delivery w.r.t. EDR changes
03g,02dec03,dbs  fully implement EDR policy handling
03f,21nov03,kam  fixed bug when _EXC_OFF_DECR is not defined
03e,21nov03,tam  added conditional on EXC_OFF_DECR
03d,14nov03,pch  Clean up data types in excScConnect
03c,11nov03,dbs  rework excExcHandle() for ED&R policies
03b,31oct03,yvp  Moved system call vector initialization to syscallArchLib.c
03a,29oct03,dbs  TESTING pes's kernel/user exception macro
02z,29oct03,kam  ISR Object code inspection mods
02y,21oct03,nrj  Added RTP task exception signal support
02x,25sep03,dbs  implement ED&R system policies
02w,11sep03,dbs  add error-injection for exception in early boot sequence
		 code.
02v,09sep03,kam  added check for _EXC_OFF_DECR in excIntConnect() which
		 creates an ISR object for the decrementor
02u,29aug03,jtp  Correct ppc440x5MchInit() prototype return value
02t,26aug03,pch  inspection cleanup
02s,26aug03,yvp  Added full-blown System Call Infrastructure support.
02r,26aug03,dbs  added ED&R instrumentation points
02q,31jul03,jtp  Add path to exc440x5.h
02p,21jul03,kk   defined SC macros for PPC4XX to avoid build errors
02o,14jul03,pad  Added setting r3 to sp value in system call exception stub
02n,01jul03,pad  Introduced support for system calls for RTP prototype.
02m,10jun03,pch  log recovered 440x5 Machine Check events;
		 rename AltVecBaseAdrs to VecBaseAltAdrs
02l,07jan03,pch  Add support for 440x5 core (PPC440GX).
		 Extract blCompute and blExtract into ppcBrCompute.c.
		 Clean up and rationalize data types; handle ([EI]VPR != 0)
		 SPRs 79397, 79421, 79422
02o,25aug03,mil  Fixed reg corruption of machine check stub code.
02o,25aug03,dtr  Wrap new ESF info mcesr with CPU==PPC85XX.
02n,13aug03,mil  Consolidated esr and mcsr as well as dear and mcar for e500.
02m,19nov02,mil  Updated support for PPC85XX.
02l,03aug02,pcs  Add support for PPC85XX and make it the same as PPC603 for
		 the present.
02k,22may02,mil  Added back excIntConnectTimer() for compatibility.
02j,08may02,mil  Relocated _EXC_OFF_PERF to avoid corruption by the extended
		 _EXC_ALTIVEC_UNAVAILABLE vector (SPR #76916).  Patched
		 calculation of wrong relocated vector offset (SPR #77145).
		 Added _EXC_OFF_THERMAL for PPC604 (SPR #77552).
02i,04apr02,pch  SPR 74348: Enable PPC Machine Check exception ASAP
02h,13nov01,yvp  Fix SPR 27916, 8179: Added extended (32-bit) vectors for
		 exception handling.
02g,05oct01,pch  SPR's 68093 & 68585:  handle critical vectors in excVecGet()
		 and excVecSet().  Comment changes (only) supporting rework
		 of SPR 69328 fix
02f,15aug01,pch  Add PPC440 support
02e,14jun01,kab  Fixed Altivec Unavailable Exchandler, SPR 68206
02d,30nov00,s_m  fixed bus error handling for 405
02c,25oct00,s_m  renamed PPC405 cpu types
02b,13oct00,sm   modified machine check handling for PPC405
02a,06oct00,sm   PPC405 GF & PPC405 support
01z,12mar99,zl   added PowerPC 509 and PowerPC 555 support.
01y,24aug98,cjtc intEnt logging for PPC is now performed in a single step
		 instead of the two-stage approach which gave problems with
		 out-of-order timestamps in the event log.
		 Global evtTimeStamp no longer required (SPR 21868)
01x,18aug98,tpr  added PowerPC EC 603 support.
01w,09jan98,dbt  modified for new breakpoint scheme
01v,06aug97,tam  fixed problem with CE interrupt (SPR #8964)
01u,26mar97,tam	 added test for DBG_BREAK_INST in excExcHandle() (SPR #8217).
01t,26mar97,jdi,tam doc cleanup.
01s,20mar97,tam  added function excIntCrtConnect for PPC403 Critical Intr.
01r,24feb97,tam  added support for 403GC/GCX exceptions.
01q,10feb97,tam  added support to handle floating point exceptions (SPR #7840).
01p,05feb97,tpr  reawork PPC860 support in excExcHandle() (SPR 7881).
01o,16jan97,tpr  Changed CACHE_TEXT_UPDATE() address in excVecSet(). (SPR #7754)
01n,03oct96,tpr  Reworked excGetInfoFromESF () to include DSISR and DAR
		 registers for PPC860 (SPR# 7254)
01o,11jul96,pr   cleanup windview instrumentation
01n,08jul96,pr   added windview instrumentation - conditionally compiled
01m,31may96,tpr  added PowerPC 860 support.
01l,12mar96,tam  re-worked exception handling for the PPC403 FIT and PIT
		 interrupts. Added excIntConnectTimer().
01k,28feb96,tam  added excCrtConnect() for critical exceptions on the PPC403
		 cpu.
01j,27feb96,ms   made excConnectCode use "stwu" instead of "addi".
		 fixed compiler warnings. Change logMsg to func_logMsg.
01i,23feb96,tpr  added excConnect() & excIntConnect(), renamed excCode[]
		 by excConnectCode.
01h,05oct95,tpr  changed excCode[] code.
01g,23may95,caf  fixed unterminated comment in version 01f.
01f,22may95,caf  enable PowerPC 603 MMU.
01e,09feb95,yao  changed machine check exception handler to excCrtStub for
		 PPC403.
01d,07feb95,yao  fixed excExcHandler () for PPC403.  removed _AIX_TOOL support.
01c,02feb95,yao  changed to set timer exceptions to excClkStub for PPC403.
		 changed to call vxEvpr{S,G}et for PPC403.
01b,07nov94,yao  cleanup.
01a,09sep94,yao  written.
*/

/*
This module provides architecture-dependent facilities for handling PowerPC
exceptions.  See excLib for the portions that are architecture independent.

INITIALIZATION
Initialization of exception handling facilities is in two parts.  First,
excVecInit() is called to set all the PowerPC exception, trap, and interrupt
vectors to the default handlers provided by this module.  The rest of this
package is initialized by calling excInit() (in excLib), which spawns the
exception support task, excTask(), and creates the pipe used to communicate
with it.  See the manual entry for excLib for more information.

PARAMETER TYPES
Due to properties of the hardware architecture, the PowerPC port of VxWorks
implements the "exception vector" parameters to functions in this module as
offsets within the vector table rather than as direct pointers.  To avoid
conflicts with other ports, those parameters are declared as pointers, but
with a (vecTblOffset) comment.  The value passed will normally be one of the
predefined vector table offsets from excPpcLib.h, such as _EXC_OFF_INTR.
See excVecSet for an example.

SEE ALSO: excLib,
.pG "Debugging"
*/


/* LINTLIBRARY */

#include <vxWorks.h>
#include <esf.h>
#include <iv.h>
#include <sysLib.h>
#include <intLib.h>
#include <msgQLib.h>
#include <signal.h>
#include <cacheLib.h>
#include <errnoLib.h>
#include <string.h>
#include <rebootLib.h>
#include <excLib.h>
#include <vxLib.h>
#include <isrLib.h>
#include <private/funcBindP.h>
#include <private/sigLibP.h>
#include <private/taskLibP.h>
#include <private/windLibP.h>			/* kernelState */
#include <wdb/wdbDbgLib.h>
#ifdef _WRS_ALTIVEC_SUPPORT
#include <altivecLib.h>
#endif /* _WRS_ALTIVEC_SUPPORT */
#include <edrLib.h>
#include <private/kernelBaseLibP.h>	/* IS_KERNEL_TASK() */
#include <ioLib.h>	/* ioGlobalStdGet() */
#ifdef	PPC_440x5
#include <arch/ppc/exc440x5.h>
#endif	/* PPC_440x5 */
#include <arch/ppc/private/excArchLibP.h>
#include <private/isrLibP.h>
#if	CPU == PPC970
#include <limits.h>				/* INT_MAX */
#include <arch/ppc/vxPpcLib.h>			/* vxDecSet */
#endif	/* CPU == PPC970 */

/*
 * Kernel build option:  offset fixups for relocated vectors can be handled
 * either cleanly (by calling a function), or quickly (using inline code).
 * General use of inline is now deprecated, except in excIntHandle, which
 * is called for every interrupt; elsewhere use function call.
 */
#define USE_INT_RELOC_FUNCTION	0	/* 0 => inline code in excIntHandle */


/* externals  */

IMPORT void	excEnt (void);	    /* exception entry routine */
IMPORT void	excExit (void);	    /* exception exit routine */
IMPORT void	intEnt (void);	    /* interrupt entry routine */
IMPORT void	intExit (void);	    /* interrupt exit routine */
IMPORT void	excScErrEnt (void); /* sys call exception entry routine */
#ifdef	_PPC_MSR_CE
IMPORT void	excCrtEnt (void);   /* critical exception stub  */
IMPORT void	excCrtExit (void);  /* critical exception stub  */
IMPORT void	intCrtEnt (void);   /* external critical exception stub  */
IMPORT void	intCrtExit (void);  /* external critical exception stub  */
IMPORT FUNCPTR	_dbgDsmInstRtn;
#ifdef	_PPC_MSR_MCE
IMPORT void	excMchkExit (void); /* machine check exception stub  */
IMPORT void	excMchkEnt (void);  /* machine check exception stub  */
#endif	/* _PPC_MSR_MCE */
#endif	/* _PPC_MSR_CE */
IMPORT void     excEPSet (FUNCPTR *);
IMPORT INSTR	ppcBrCompute (VOIDFUNCPTR target, INSTR * branchFrom, int link);
IMPORT INSTR *	ppcBrExtract (INSTR inst, INSTR * address);
#ifdef	PPC_440x5
IMPORT STATUS	ppc440x5MchInit (BOOL parity, BOOL tlb, BOOL dcache);
#endif	/* PPC_440x5 */

/* ISR object externals */

#ifdef INCLUDE_ISR_OBJECTS
IMPORT STATUS isrArchDecDispatcher (ESFPPC * pEsf);
IMPORT ISR_ID isrArchDecCreate (char * name, UINT isrTag, FUNCPTR handlerRtn,
                                int parameter, UINT options);
#endif  /* INCLUDE_ISR_OBJECTS */


/* globals */

FUNCPTR _func_excTrapRtn = NULL;        /* trap handling routine */

/*
 * Option to use extended (full 32-bit) vectors to jump from the vector table
 * to handler functions. Normally we use a 26-bit address, which suffices for
 * a vast majority of functions. However a 26-bit address restricts branches
 * to within 32MB which may be a problem for some systems.
 *
 * Setting excExtendedVectors to TRUE enables branching to an absolute 32-bit
 * address. This option increases interrupt latency by about 20%, but there is
 * no other choice left when the handler routine is more than 32MB away.
 */
/*ZCW,我们采用32bit模式*/
BOOL excExtendedVectors = TRUE;        /* extended exception stubs flag */

/*
 * Base address of vector table in normal code space.  Usually zero, but
 * may be set otherwise by the BSP, e.g. by changing VEC_BASE_ALT_ADRS.
 */
IMPORT codeBase  excVecBaseAltAdrs;

#ifdef	PPC_440x5
/* Component/BSP-settable options for the 440x5 machine check handler */
IMPORT BOOL	exc440x5_parity;
IMPORT BOOL	exc440x5_tlb;
IMPORT BOOL	exc440x5_dcache;
IMPORT FUNCPTR	exc440x5_logger;
#endif	/* PPC_440x5 */
#if (CPU==PPC85XX)
IMPORT void  ppcE500MchEnd();
IMPORT void  ppcE500Mch();
#endif /* (CPU==PPC85XX) */


/* macros */

/* sign-extend a 26-bit integer value */
#define SEXT_26BIT(x) (((0x03ffffff & (INSTR) (x)) ^ 0x02000000) - 0x02000000)

/* use special purpose register */
#define SPR_SET(reg) (((((reg) & 0x1f) << 5) | (((reg) & 0x3e0) >> 5)) << 11)

/* instructions to load 32-bit number into r3 */
#define INST_LOAD_R3_HI(x) (0x3c600000 | MSW((int)(x)))
#define INST_LOAD_R3_LO(x) (0x60630000 | LSW((int)(x)))


/* types */

/*
 * This structure holds the entry and exit routines for a given
 * type of exception or interrupt.
 */
typedef struct excWrappers
    {
    VOIDFUNCPTR enterRtn;
    VOIDFUNCPTR exitRtn;
    } EXC_WRAPPERS;

/*
 * These are the various types of exception or interrupts whose
 * entry and exit routines will be different, and dictate which
 * stub gets installed.
 */
typedef enum excType
    {
    V_NORM_EXC = 0,
    V_NORM_INT
#ifdef  _PPC_MSR_CE
    ,V_CRIT_EXC
    ,V_CRIT_INT
#ifdef  _PPC_MSR_MCE
    ,V_MCHK_EXC
#endif  /* _PPC_MSR_MCE */
#endif  /* _PPC_MSR_CE */
    } EXC_TYPE;                                             

/*
 * This array holds the entry and exit routines for the
 * types of exception and interrupts.  Their order _must_ match
 * exactly the enum of excType.
 */
LOCAL EXC_WRAPPERS excTypeRtnTbl[] =
    {
    {excEnt, excExit},                  /* V_NORM_EXC */
    {intEnt, intExit},                  /* V_NORM_INT */
#ifdef  _PPC_MSR_CE
    {excCrtEnt, excCrtExit},            /* V_CRIT_EXC */
    {intCrtEnt, intCrtExit},            /* V_CRIT_INT */
#ifdef  _PPC_MSR_MCE
    {excMchkEnt, excMchkExit}           /* V_MCHK_EXC */
#endif  /* _PPC_MSR_MCE */
#endif  /* _PPC_MSR_CE */
    };

typedef struct excTbl
    {
    vecTblOffset  vecOff;               /* vector offset */
    EXC_TYPE      vType;                /* exception type */
    void          (*excHandler) ();     /* exception handler routine */
    vecTblOffset  vecOffReloc;          /* relocated vector offset */
    } EXC_TBL;


/* locals */

/*
 * Base address of vector table in vector execution space.  This is
 * the address at which the vector code executes, set in the EVPR
 * or IVPR on processors so equipped.  Its value comes from the
 * VEC_BASE_ADRS parameter set in configAll.h, perhaps overridden
 * by the BSP.  Unless vectors have to be treated specially -- as
 * in the INCLUDE_440X5_TLB_RECOVERY_MAX configuration -- it will
 * typically have the same value as excVecBaseAltAdrs.
 *
 * Note that "vector execution space" refers to only the copies of
 * excConnectCode[] etc. installed by excVecInit(), and similar code
 * stubs installed by other initialization functions.  It does not
 * include {exc|int}[Crt]{Ent|Exit}, the handlers, or any other code
 * which -- although called from the stub code -- executes where it
 * was placed by the linker or loader.
 */
LOCAL vectorBase  excVecBase = (vectorBase)NULL;

/* Word offsets into exc*ConnectCode */
LOCAL int       entOffset, exitOffset, isrOffset;
#ifdef _EXC_OFF_CRTL
LOCAL int       entCrtOffset, exitCrtOffset, isrCrtOffset;
#endif /* _EXC_OFF_CRTL */

/* These are in vector execution space, used for branch computation */
LOCAL char      * hdlrBase;
/* save the Data and/or Instruction MMU selected */
LOCAL char      * hdlrCodeBase;


/* forward declarations */

vecTblOffset  vecOffRelocMatch (vecTblOffset vector);
vecTblOffset  vecOffRelocMatchRev (vecTblOffset vector);
void	      excExcHandle (ESFPPC * pEsf);
void	      excIntHandle ();
#if	CPU == PPC970
void	      excDecrHandle ();
#endif	/* CPU == PPC970 */
void          excScHandle (ESFPPC * pEsf);
LOCAL int     excGetInfoFromESF (FAST vecTblOffset vecNum, FAST ESFPPC *pEsf,
                                 EXC_INFO *pExcInfo);
LOCAL STATUS  excVecConnectCommon (vecTblOffset vector, EXC_TYPE vType,
                                   VOIDFUNCPTR handler, vecTblOffset reloc);
LOCAL BOOL    excPageUnProtect (char *newVector);
LOCAL void    excPageProtect (char *newVector);
#ifdef IVOR0
void	      excIvorInit (void);
#endif
#if (CPU==PPC85XX)
STATUS        installE500ParityErrorRecovery (void);
#endif /* (CPU==PPC85XX) */


/*
 * Exception vector table
 *
 * Each entry in the exception vector table is consists of:
 * - the vector offset (from the vector base address) of the exception type
 * - the exception or interrupt type
 * - the handler routine, which is one of:
 *   excScHandle()
 *   excExcHandle()
 *   excIntHandle()
 * - the relocated vector offset (from the vector base address)
 *
 * The original implementation of the exception handling routines do not
 * have facility to relocate vectors.  To find out the vector offset of
 * the exception that causes the instance of excEnt()/intEnt() to run, a
 * pre-computed value is subtracted from the beginning of the stub whose
 * address is put into LR by the bl excEnt or bl intEnt in the stub.
 * However, a stub which is relocated to another address will cause
 * the calculation in excEnt() and intEnt() to yield a wrong vector.
 * With SPRG0-SPRG3 all used up, the same excEnt()/intEnt() routines used
 * for both the un-relocated and relocated cases cannot find an efficient
 * way to calculate the right vector offset.
 *
 * As a temporary workaround, excEnt() and intEnt() are hard coded to
 * detect the relocated vectors and patch them up with the right
 * values before saving them to the ESF.  This means that binary 
 * customers do not have the option to choose which vector is 
 * relocated, and if relocated, where is the new vector offset.  
 * Note also that adding/removing relocated vectors require
 * change to the inline implementation, such as USE_INT_RELOC_FUNCTION.
 */

LOCAL EXC_TBL excBlTbl[] =
    {
#if	( (CPU == PPC403) || (CPU==PPC405) || (CPU==PPC405F) )
    {_EXC_OFF_CRTL,        V_CRIT_INT,  excIntHandle, 0}, /* crit int */
    {_EXC_OFF_MACH,        V_CRIT_EXC,  excExcHandle, 0}, /* machine chk */
    {_EXC_OFF_PROT,        V_NORM_EXC,  excExcHandle, 0}, /* protect viol */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
# if (CPU == PPC405F)	/* 405GF supports a FPU */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
# endif
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
#ifdef _EXC_NEW_OFF_PIT  /* prog timer */
    {_EXC_OFF_PIT,	   V_NORM_INT,  excIntHandle, _EXC_NEW_OFF_PIT},
#else
    {_EXC_OFF_PIT,	   V_NORM_INT,  excIntHandle, 0}, /* prog timer */
#endif  /* _EXC_NEW_OFF_PIT */
#ifdef _EXC_NEW_OFF_FIT  /* fixed timer */
    {_EXC_OFF_FIT,	   V_NORM_INT,  excIntHandle, _EXC_NEW_OFF_FIT},
#else
    {_EXC_OFF_FIT,	   V_NORM_INT,  excIntHandle, 0}, /* fixed timer */
#endif  /* _EXC_NEW_OFF_FIT */
    {_EXC_OFF_WD,	   V_CRIT_INT,  excIntHandle, 0}, /* watchdog */
    {_EXC_OFF_DATA_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* data TLB miss */
    {_EXC_OFF_INST_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* inst TLB miss */
    {_EXC_OFF_DBG,	   V_CRIT_EXC,  excExcHandle, 0}, /* debug events */

#elif	(CPU == PPC440)
    {_EXC_OFF_CRTL,	   V_CRIT_INT,  excIntHandle, 0}, /* critical int */
    {_EXC_OFF_MACH,	   V_CRIT_EXC,  excExcHandle, 0}, /* machine chk  */
    {_EXC_OFF_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* data storage */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
    {_EXC_OFF_APU,	   V_NORM_EXC,  excExcHandle, 0}, /* auxp unavail*/
    {_EXC_OFF_DECR,	   V_NORM_INT,  excIntHandle, 0}, /* decrementer */
    {_EXC_OFF_FIT,	   V_NORM_INT,  excIntHandle, 0}, /* fixed timer */
    {_EXC_OFF_WD,	   V_CRIT_INT,  excIntHandle, 0}, /* watchdog */
    {_EXC_OFF_DATA_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* data TLB miss */
    {_EXC_OFF_INST_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* inst TLB miss */
    {_EXC_OFF_DBG,	   V_CRIT_EXC,  excExcHandle, 0}, /* debug events */

#elif	(CPU == PPC85XX)	/* placed here to reflect on 440 */
    {_EXC_OFF_CRTL,	   V_CRIT_INT,  excIntHandle, 0}, /* critical int */
    {_EXC_OFF_MACH,	   V_MCHK_EXC,  excExcHandle, 0}, /* machine chk  */
    {_EXC_OFF_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* data storage */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
    {_EXC_OFF_APU,	   V_NORM_EXC,  excExcHandle, 0}, /* auxp unavail*/
    {_EXC_OFF_DECR,	   V_NORM_INT,  excIntHandle, 0}, /* decrementer */
    {_EXC_OFF_FIT,	   V_NORM_INT,  excIntHandle, 0}, /* fixed timer */
    {_EXC_OFF_WD,	   V_CRIT_INT,  excIntHandle, 0}, /* watchdog */
    {_EXC_OFF_DATA_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* data TLB miss */
    {_EXC_OFF_INST_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* inst TLB miss */
    {_EXC_OFF_DBG,	   V_CRIT_EXC,  excExcHandle, 0}, /* debug events */
    {_EXC_OFF_SPE,	   V_NORM_EXC,  excExcHandle, 0}, /* SPE */
    {_EXC_OFF_VEC_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* vector data */
    {_EXC_OFF_VEC_RND,	   V_NORM_EXC,  excExcHandle, 0}, /* vector round */
    {_EXC_OFF_PERF_MON,	   V_NORM_EXC,  excExcHandle, 0}, /* perf monitor */

#elif	((CPU == PPC509) || (CPU == PPC555))
    {_EXC_OFF_RESET,	   V_NORM_EXC,  excExcHandle, 0}, /* system reset */
    {_EXC_OFF_MACH,	   V_NORM_EXC,  excExcHandle, 0}, /* machine chk */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_DECR,	   V_NORM_INT,  excIntHandle, 0}, /* decrementer */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
    {_EXC_OFF_TRACE,	   V_NORM_EXC,  excExcHandle, 0}, /* trace except */
    {_EXC_OFF_FPA,	   V_NORM_EXC,  excExcHandle, 0}, /* fp assist */
    {_EXC_OFF_SW_EMUL,	   V_NORM_EXC,  excExcHandle, 0}, /* sw emul */
# if	(CPU == PPC555)
    {_EXC_OFF_IPE,	   V_NORM_EXC,  excExcHandle, 0}, /* instr prot */
    {_EXC_OFF_DPE,	   V_NORM_EXC,  excExcHandle, 0}, /* data prot */
# endif	(CPU == PPC555)
    {_EXC_OFF_DATA_BKPT,   V_NORM_EXC,  excExcHandle, 0}, /* data breakpt */
    {_EXC_OFF_INST_BKPT,   V_NORM_EXC,  excExcHandle, 0}, /* instr breakpt */
    {_EXC_OFF_PERI_BKPT,   V_NORM_EXC,  excExcHandle, 0}, /* peripheral BP */
    {_EXC_OFF_NM_DEV_PORT, V_NORM_EXC,	excExcHandle, 0}, /* non maskable */

#elif	(CPU == PPC601)
    {_EXC_OFF_RESET,	   V_NORM_EXC,  excExcHandle, 0}, /* system reset */
    {_EXC_OFF_MACH,	   V_NORM_EXC,  excExcHandle, 0}, /* machine chk */
    {_EXC_OFF_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* data access */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_DECR,	   V_NORM_INT,  excIntHandle, 0}, /* decrementer */
    {_EXC_OFF_IOERR,	   V_NORM_EXC,  excExcHandle, 0}, /* i/o ctrl err */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
    {_EXC_OFF_RUN_TRACE,   V_NORM_EXC,  excExcHandle, 0}, /* run/trace */

#elif	((CPU == PPC603) || (CPU == PPCEC603) || (CPU == PPC604))
    {_EXC_OFF_RESET,	   V_NORM_EXC,  excExcHandle, 0}, /* system reset */
    {_EXC_OFF_MACH,	   V_NORM_EXC,  excExcHandle, 0}, /* machine chk */
    {_EXC_OFF_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* data access */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_DECR,	   V_NORM_INT,  excIntHandle, 0}, /* decrementer */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
    {_EXC_OFF_TRACE,	   V_NORM_EXC,  excExcHandle, 0}, /* trace excepti */
# if	((CPU == PPC603) || (CPU == PPCEC603))
    {_EXC_OFF_INST_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* i-trsl miss */
    {_EXC_OFF_LOAD_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* d-trsl miss */
    {_EXC_OFF_STORE_MISS,  V_NORM_EXC,  excExcHandle, 0}, /* d-trsl miss */
# else	/* (CPU == PPC604) */
#ifdef _EXC_NEW_OFF_PERF  /* perf mon */
    {_EXC_OFF_PERF,	   V_NORM_EXC,  excExcHandle, _EXC_NEW_OFF_PERF},
#else
    {_EXC_OFF_PERF,	   V_NORM_EXC,  excExcHandle, 0}, /* perf mon */
#endif  /* _EXC_NEW_OFF_PERF */
#  ifdef _WRS_ALTIVEC_SUPPORT
    {_EXC_ALTIVEC_UNAVAILABLE, V_NORM_EXC,excExcHandle, 0}, /* altivec unav */
    {_EXC_ALTIVEC_ASSIST,  V_NORM_EXC, excExcHandle, 0},  /* altivec asst */
#  endif /* _WRS_ALTIVEC_SUPPORT */
    {_EXC_OFF_THERMAL,	   V_NORM_EXC,  excExcHandle, 0}, /* thermal */
# endif	/* (CPU == PPC604) */
    {_EXC_OFF_INST_BRK,	   V_NORM_EXC,  excExcHandle, 0}, /* instr BP */
    {_EXC_OFF_SYS_MNG,	   V_NORM_EXC,  excExcHandle, 0}, /* sys mgt*/

#elif	(CPU == PPC860)
    {_EXC_OFF_RESET,	   V_NORM_EXC,  excExcHandle, 0}, /* system reset */
    {_EXC_OFF_MACH,	   V_NORM_EXC,  excExcHandle, 0}, /* machine chk */
    {_EXC_OFF_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* data access */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_DECR,	   V_NORM_INT,  excIntHandle, 0}, /* decrementer */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* syscall */
    {_EXC_OFF_TRACE,	   V_NORM_EXC,  excExcHandle, 0}, /* trace except */
    {_EXC_OFF_SW_EMUL,	   V_NORM_EXC,  excExcHandle, 0}, /* sw emul */
    {_EXC_OFF_INST_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* inst TLB Miss */
    {_EXC_OFF_DATA_MISS,   V_NORM_EXC,  excExcHandle, 0}, /* data TLB Miss */
    {_EXC_OFF_INST_ERROR,  V_NORM_EXC,	excExcHandle, 0}, /* inst TLB err */
    {_EXC_OFF_DATA_ERROR,  V_NORM_EXC,	excExcHandle, 0}, /* data TLB err */
    {_EXC_OFF_DATA_BKPT,   V_NORM_EXC,  excExcHandle, 0}, /* data BP */
    {_EXC_OFF_INST_BKPT,   V_NORM_EXC,  excExcHandle, 0}, /* instr BP */
    {_EXC_OFF_PERI_BKPT,   V_NORM_EXC,  excExcHandle, 0}, /* peripheral BP */
    {_EXC_OFF_NM_DEV_PORT, V_NORM_EXC,	excExcHandle, 0}, /* non maskable */

#elif	(CPU == PPC970)
    {_EXC_OFF_RESET,	   V_NORM_EXC,  excExcHandle, 0}, /* system reset */
    {_EXC_OFF_MACH,	   V_NORM_EXC,  excExcHandle, 0}, /* machine chk */
    {_EXC_OFF_DATA,	   V_NORM_EXC,  excExcHandle, 0}, /* data access */
    {_EXC_OFF_DATA_SEG,	   V_NORM_EXC,  excExcHandle, 0}, /* data segment */
    {_EXC_OFF_INST,	   V_NORM_EXC,  excExcHandle, 0}, /* instr access */
    {_EXC_OFF_INST_SEG,	   V_NORM_EXC,  excExcHandle, 0}, /* instr segment */
    {_EXC_OFF_INTR,	   V_NORM_INT,  excIntHandle, 0}, /* ext int */
    {_EXC_OFF_ALIGN,	   V_NORM_EXC,  excExcHandle, 0}, /* alignment */
    {_EXC_OFF_PROG,	   V_NORM_EXC,  excExcHandle, 0}, /* program */
    {_EXC_OFF_FPU,	   V_NORM_EXC,  excExcHandle, 0}, /* fp unavail */
    {_EXC_OFF_DECR,	   V_NORM_INT,  excDecrHandle, 0}, /* decrementer */
    {_EXC_OFF_HYPV_DECR,   V_NORM_EXC,  excExcHandle, 0}, /* hv decrementer */
    {_EXC_OFF_SYSCALL,	   V_NORM_EXC,  excExcHandle, 0}, /* system call */
    {_EXC_OFF_TRACE,	   V_NORM_EXC,  excExcHandle, 0}, /* trace excepti */
# ifdef _EXC_NEW_OFF_PERF  /* perf mon */
    {_EXC_OFF_PERF,	   V_NORM_EXC,  excExcHandle, _EXC_NEW_OFF_PERF},
# else
    {_EXC_OFF_PERF,	   V_NORM_EXC,  excExcHandle, 0}, /* perf mon */
# endif
# ifdef _WRS_ALTIVEC_SUPPORT
    {_EXC_ALTIVEC_UNAVAILABLE,V_NORM_EXC, excExcHandle, 0}, /* altivec unav */
# endif /* _WRS_ALTIVEC_SUPPORT */
    {_EXC_OFF_INST_BRK,	   V_NORM_EXC,  excExcHandle, 0}, /* instr BP */
    {_EXC_OFF_SYS_MNG,	   V_NORM_EXC,  excExcHandle, 0}, /* sys mgt */
    {_EXC_OFF_SOFT_PATCH,  V_NORM_EXC,  excExcHandle, 0}, /* soft patch */
    {_EXC_OFF_MAINTENANCE, V_NORM_EXC,  excExcHandle, 0}, /* maintenance */
    {_EXC_OFF_VMX_ASSIST,  V_NORM_EXC,  excExcHandle, 0}, /* VMX assist */
    {_EXC_OFF_THERMAL,	   V_NORM_EXC,  excExcHandle, 0}, /* thermal */
    {_EXC_OFF_INSTRUMENT,  V_NORM_EXC,  excExcHandle, 0}, /* instrumentation*/

#endif	/* 40x : 440 : 5xx : 601 : 603 | 604 : 860 : 970 */

    {0,  0,  (void (*) ())NULL,  0},      /* end of table */
    };

/*
 * It is necessary to clear the MSR[CE] bit upon an external interrupt
 * on the PowerPC 403 architecture, to prevent this interrupt being
 * interrupted by a critical interrupt before the context is saved.
 * There's still a window of 5 instructions were the external interrupt
 * can be interrupted. This is taken care of, in the critical interrupt
 * entry code (intCrtEnt), by saving SPRG3 before using it and restoring
 * its original value later on.
 *
 * On 64-bit processors (PPC970), it is necessary to clear the 64-bit mode
 * bit before calling intEnt or excEnt, so that negative absolute or relative
 * branch offsets will be interpreted in 32-bit mode.
 *
 * In excConnectCode,
 *   xxxEnt is either intEnt or excEnt.
 *   xxxHandler is one of excIntHandle or excExcHandle
 *   xxxExit is either intExit or excExit.
 *
 * Changes here may affect code in excALib.s:excEnt(), e.g. the offsets
 * used in calculating the original vector address from the LR value,
 * and offsets defined in arch/ppc/private/excArchLibP.h
 */

LOCAL INSTR excConnectCode[]=
    {
    /*  data	      word    byte     opcode  operands		      */
    0x7c7343a6,	    /*  0     0x00     mtspr   SPRG3, p0	      */
#if	defined(_EXC_OFF_CRTL)
    0x7c6000a6,	    /*  1     0x04     mfmsr   p0		      */
    0x546303da,	    /*  2     0x08     rlwinm  p0,p0,0,15,13  clear MSR[CE] */
    0x7c600124,	    /*  3     0x0c     mtmsr   p0		      */
    0x4c00012c,	    /*  4     0x10     isync			      */
#elif	defined(_WRS_PPC_64BIT)
    0x7c6000a6,	    /*  1     0x04     mfmsr   p0		      */
    0x786300c0,	    /*  2     0x08     clrldi  p0,p0,3 clear MSR[SF]  */
    0x7c600164,	    /*  3     0x0c     mtmsrd  p0		      */
    0x4c00012c,	    /*  4     0x10     isync			      */
#endif	/* _EXC_OFF_CRTL, _WRS_PPC_64BIT */
    /* If either of the above, add 4 words/0x10 bytes to following offsets */
    0x7c6802a6,	    /*  1     0x04     mflr    p0		      */
    0x48000001,	    /*  2(6)  0x08/18  bl      xxxEnt		      */
    0x38610000,	    /*  3     0x0c     addi    r3, sp, 0	      */
    0x9421fff0,	    /*  4     0x10     stwu    sp, -FRAMEBASESZ(sp)   */
    0x48000001,	    /*  5(9)  0x14/24  bl      xxxHandler	      */
    0x38210010,	    /*  6     0x18     addi    sp, sp, FRAMEBASESZ    */
    0x48000001	    /*  7(11) 0x1c/2c  bl      xxxExit		      */
    };

/*
 * Stub code for extended-branch vectors. This stub will be installed into
 * the trap table if excExtendedVectors is TRUE. Branches to the xxxEnt,
 * xxxExit and xxxHandler functions are made via an absolute 32-bit address
 * stored in the LR.
 */

LOCAL INSTR excExtConnectCode[]=
    {
    /*  data	      word    byte     opcode  operands		      */
    0x7c7343a6,	    /*  0     0x00     mtspr	  SPRG3, p0	      */
#if	defined(_EXC_OFF_CRTL)
    0x7c6000a6,	    /*  1     0x04     mfmsr   p0		      */
    0x546303da,	    /*  2     0x08     rlwinm  p0,p0,0,15,13  clear MSR[CE] */
    0x7c600124,	    /*  3     0x0c     mtmsr   p0		      */
    0x4c00012c,	    /*  4     0x10     isync			      */
#elif	defined(_WRS_PPC_64BIT)
    0x7c6000a6,	    /*  1     0x04     mfmsr   p0		      */
    0x786300c0,	    /*  2     0x08     clrldi  p0,p0,3 clear MSR[SF]  */
    0x7c600164,	    /*  3     0x0c     mtmsrd  p0		      */
    0x4c00012c,	    /*  4     0x10     isync			      */
#endif	/* _EXC_OFF_CRTL, _WRS_PPC_64BIT */
    /* If either of the above, add 4 words/0x10 bytes to following offsets */
    0x7c6802a6,	    /*  1     0x04     mflr    p0                     */
    0x7c7043a6,	    /*  2     0x08     mtspr   SPRG0, p0              */
    0x3c600000,	    /*  3(7)  0x0c     lis     p0, HI(xxxEnt)         */
    0x60630000,	    /*  4(8)  0x10     ori     p0, p0, LO(xxxEnt)     */
    0x7c6803a6,	    /*  5     0x14     mtlr    p0                     */
    0x7c7042a6,	    /*  6     0x18     mfspr   p0, SPRG0              */
    0x4e800021,	    /*  7     0x1c     blrl                           */
    0x3c600000,	    /*  8(12) 0x20     lis     p0, HI(xxxHandler)     */
    0x60630000,	    /*  9(13) 0x24     ori     p0, p0, LO(xxxHandler) */
    0x7c6803a6,	    /* 10     0x28     mtlr    p0                     */
    0x38610000,	    /* 11     0x2c     addi    p0, sp, 0              */
    0x9421fff0,     /* 12     0x30     stwu    sp, -FRAMEBASESZ(sp)   */
    0x4e800021,	    /* 13     0x34     blrl                           */
    0x38210010,	    /* 14     0x38     addi    sp, sp, FRAMEBASESZ    */
    0x3c600000,	    /* 15(19) 0x3c     lis     p0, HI(xxxExit)        */
    0x60630000,	    /* 16(20) 0x40     ori     p0, p0, LO(xxxExit)    */
    0x7c6803a6,	    /* 17     0x44     mtlr    p0                     */
    0x4e800021	    /* 18     0x48     blrl                           */
    };

#ifdef	_EXC_OFF_CRTL
/*
 * In excCrtConnectCode,
 *   xxxCrtEnt is either intCrtEnt or excCrtEnt.
 *   xxxHandler is either excIntHandle or excExcHandle.
 *   xxxCrtExit is either intCrtExit or excCrtExit.
 *
 * Changes here may affect code in excALib.s:excCrtEnt(), e.g. the offset
 * used in calculating the original vector address from the LR value,
 * and offsets defined in arch/ppc/private/excArchLibP.h
 *
 * In order to determine which type of vector is being accessed, code
 * in excVecGet() and excVecSet() depends on excCrtConnectCode[0] and
 * excConnectCode[0] being different.
 */

LOCAL INSTR excCrtConnectCode[]=
    {
    /*  data	      word  byte  opcode  operands		     */
    0x7c7243a6,	    /*  0   0x00  mtspr   SPRG2, p0 # SPRG4 for mchk */
    0x7c6802a6,	    /*  1   0x04  mflr    p0			     */
    0x48000003,	    /*  2   0x08  bla     xxxCrtEnt		     */
    0x38610000,	    /*  3   0x0c  addi    r3, sp, 0		     */
    0x9421fff0,	    /*  4   0x10  stwu    sp, -FRAMEBASESZ(sp)	     */
    0x48000003,	    /*  5   0x14  bla     xxxHandler		     */
    0x38210010,	    /*  6   0x18  addi    sp, sp, FRAMEBASESZ	     */
    0x48000003	    /*  7   0x1c  bla     xxxCrtExit		     */
    };

LOCAL INSTR excExtCrtConnectCode[]=
    {
    /*  data	      word  byte  opcode  operands		     */
    0x7c7243a6,	    /*  0   0x00  mtspr   SPRG2, p0 # SPRG4 for mchk */
    0x7c6802a6,	    /*  1   0x00  mflr    p0                         */
    0x7c7043a6,	    /*  2   0x00  mtspr	  SPRG0, p0                  */
    0x3c600000,	    /*  3   0x00  lis	  p0, HI(xxxEnt)             */
    0x60630000,	    /*  4   0x00  ori	  p0, p0, LO(xxxEnt)         */
    0x7c6803a6,	    /*  5   0x00  mtlr	  p0                         */
    0x7c7042a6,	    /*  6   0x00  mfspr	  p0, SPRG0                  */
    0x4e800021,	    /*  7   0x00  blrl                               */
    0x3c600000,	    /*  8   0x00  lis	  p0, HI(xxxHandler)         */
    0x60630000,	    /*  9   0x00  ori	  p0, p0, LO(xxxHandler)     */
    0x7c6803a6,	    /*  10  0x00  mtlr	  p0                         */
    0x38610000,	    /*  11  0x00  addi	  p0, sp, 0                  */
    0x9421fff0,     /*  12  0x00  stwu    sp, -FRAMEBASESZ(sp)       */
    0x4e800021,	    /*  13  0x00  blrl                               */
    0x38210010,	    /*  14  0x00  addi	  sp, sp, FRAMEBASESZ        */
    0x3c600000,	    /*  15  0x00  lis	  p0, HI(xxxExit)            */
    0x60630000,	    /*  16  0x00  ori	  p0, p0, LO(xxxExit)        */
    0x7c6803a6,	    /*  17  0x00  mtlr	  p0                         */
    0x4e800021	    /*  18  0x00  blrl                               */
    };

#endif	/* _EXC_OFF_CRTL */

/*
 * Offsets formerly defined here are now in private/excArchLibP.h
 *
 * Changes affecting ENT_OFF or ENT_CRT_OFF will require corresponding
 * changes in excALib.s:excEnt() and excCrtEnt() when calculating the
 * original vector address from the LR value.
 */

#ifdef	PPC_440x5
/*******************************************************************************
*
* excMchLogger - Log recovered Machine Checks
*
* This routine is called via the kernel workQ during the next kernel exit
* following a Machine Check recovery, or from the timer ISR, to arrange for
* logging of the event.  See exc440x5.h for tradeoff of polling vs workQ.
*
* It is not intended to be either called or replaced by the application.
* However, the application may substitute a different function for logMsg
* as the setting of the mch_logger field of the MCHSA.  Although not needed
* for the default message, a pointer to the MCHSA is passed for possible
* use by such an alternate logging function.
*
* NOMANUAL
*/

void excMchLogger
    (
    UINT32 count,		/* Count of Machine Checks encountered */
    UINT32 mcsr			/* MCSR value which precipitated this event */
    )
    {
    PPC_MCHSA * mchsa = (PPC_MCHSA *)(excVecBaseAltAdrs + _EXC_OFF_MCHSA);

    /*
     * excVecInit sets mchsa->mch_logger to (FUNCPTR)1 to signify a request
     * for logging via the default function -- logMsg -- because logMsg is
     * not initialized until later.  Once we see that _func_logMsg has been
     * set, we replace the (FUNCPTR)1 with the value of _func_logMsg.
     */
    if (mchsa->mch_logger == (FUNCPTR)1)
	{
	if (_func_logMsg != NULL)
	    mchsa->mch_logger = _func_logMsg;
	else
	    return;
	}

    /*
     * Test not really needed, since handler will not schedule excMchLogger
     * if mchsa->mch_logger is NULL.
     */
    if (mchsa->mch_logger)
	{
#ifndef	_PPC_440X5_MCH_LOG_USE_POLLING
	++intCnt;   /* so logMsg will not block */
#endif	/* _PPC_440X5_MCH_LOG_USE_POLLING */
	(mchsa->mch_logger) (
	    "Recovered from Machine Check #%d, MCSR = 0x%x\n",
	    count - mchsa->mch_fatalCount,	/* count of recovered MC */
	    mcsr,
	    0, 0, 0,
	    mchsa);
#ifndef	_PPC_440X5_MCH_LOG_USE_POLLING
	--intCnt;
#endif	/* _PPC_440X5_MCH_LOG_USE_POLLING */
	}

    /*
     * Flush mchsa from cache
     *
     * See comment in exc440x5.h re using sizeof(PPC_MCHSA).  It is OK here --
     * in fact larger than necessary -- because we have not touched any of the
     * TLB area.
     */
    if (cacheLib.clearRtn != NULL)
	{
	cacheLib.clearRtn(DATA_CACHE, (void *)mchsa, sizeof(PPC_MCHSA));
	}

    }
#endif	/* PPC_440x5 */

/*******************************************************************************
*
* excVecInit - initialize the exception vectors
*
* This routine sets up PowerPC exception vectors to point to the
* appropriate default exception handlers.
*
* WHEN TO CALL
* This routine is usually called from the system start-up routine
* usrInit() in usrConfig.c, before interrupts are enabled.
*
* RETURNS: OK, or ERROR on the PPC440x5 if the Machine Check Handler's code
*	   will not fit in the available space.
*
* SEE ALSO: excLib
*
* INTERNAL:  excVecConnectCommon is now used to connect all exc types.
*/

STATUS excVecInit (void)
    {
    FAST int ix;

    /*
     * Set the values of entOffset, exitOffset and isrOffset once. These are
     * word offsets into the exception stubs where the respective ent, exit
     * and ISR function addresses are located.
     * These offsets are set once during the lifetime of the system, so they
     * cannot (and are not meant to) handle runtime changes in the value of
     * the global excExtendedVectors.
     */
    if (excExtendedVectors == TRUE)
	{
	entOffset     = EXT_ENT_OFF;
	isrOffset     = EXT_ISR_OFF;
	exitOffset    = EXT_EXIT_OFF;
#ifdef _EXC_OFF_CRTL
	entCrtOffset  = EXT_ENT_CRT_OFF;
	isrCrtOffset  = EXT_ISR_CRT_OFF;
	exitCrtOffset = EXT_EXIT_CRT_OFF;
#endif  /* _EXC_OFF_CRTL */
	}
    else
	{
	entOffset     = ENT_OFF;
	isrOffset     = ISR_OFF;
	exitOffset    = EXIT_OFF;
#ifdef _EXC_OFF_CRTL
	entCrtOffset  = ENT_CRT_OFF;
	isrCrtOffset  = ISR_CRT_OFF;
	exitCrtOffset = EXIT_CRT_OFF;
#endif  /* _EXC_OFF_CRTL */
	}

    /*
     * PowerPC "exception" and "interrupt" bases are the same (usually 0).
     *
     * usrInit() has already passed VEC_BASE_ADRS to intVecBaseSet(), but the
     * same value must also be passed to excVecBaseSet() before installing
     * vectors.
     */
    excVecBaseSet(intVecBaseGet());

    for (ix = 0; excBlTbl[ix].excHandler != (void (*)()) NULL; ix++)
	{
        excVecConnectCommon (excBlTbl[ix].vecOff,
                             excBlTbl[ix].vType,
                             excBlTbl[ix].excHandler,
                             excBlTbl[ix].vecOffReloc);
	}
#ifdef	IVOR0
    excIvorInit();
#endif	/* IVOR0 */

#ifdef	LOG_FRAMELESS
    /* Set log pointer to NULL until allocated */
    *(void **)LOG_FML_PTR = (void *)NULL;
#endif	/* LOG_FRAMELESS */
#ifdef	LOG_DISPATCH
    /* Set log pointers to NULL until allocated */
    *(void **)LOG_DISP_PTR = *(void **)LOG_DISP_SAVE = (void *)NULL;
#endif	/* LOG_DISPATCH */

#ifdef	PPC_440x5

    /* Initialize the 440x5 machine check handler */
    if (ppc440x5MchInit(exc440x5_parity, exc440x5_tlb, exc440x5_dcache) != OK)
	/* Handler too big for available space -- should not happen */
	return ERROR;

    /*
     * Because it is written in assembly, ppc440x5MchInit handles only the
     * installation and customization of the handler code.  Related setup work
     * is done here.
     */

    /*
     * exc440x5_logger may be NULL to specify no logging, or (FUNCPTR)1 to
     * use logMsg (whose address shouldn't be installed here because it isn't
     * initialized yet -- excMchLogger will fix up the pointer the first time
     * it is called following logMsg initialization).
     */
    ((PPC_MCHSA *)(excVecBaseAltAdrs + _EXC_OFF_MCHSA))->mch_logger =
	exc440x5_logger;

    /*
     * Must flush save area from cache because MC handler accesses it via an
     * uncached alias.
     */

    if(cacheLib.clearRtn!=NULL)
	{
	cacheLib.clearRtn(DATA_CACHE,
			  (void *) &((PPC_MCHSA *)
				    (excVecBaseAltAdrs + _EXC_OFF_MCHSA)) ->
                                    mch_logger,
			  sizeof(FUNCPTR));
	}

#endif	/* PPC_440x5 */

    /*
     * Now that the vectors are set up, and provided we can safely do so
     * prior to MMU setup, enable Machine Check exceptions.  (If excVecBase
     * and excVecBaseAltAdrs differ, no exceptions can be handled until the
     * MMU has been set up.)
     */
    if (excVecBaseAltAdrs == excVecBase)
	vxMsrSet (vxMsrGet() | _PPC_MSR_ME);

    /* Used for the generic layered exception handler */
    hdlrBase = excVecBase + _EXC_OFF_END;
    /* save the Data and/or Instruction MMU selected */
    hdlrCodeBase = excVecBaseAltAdrs + _EXC_OFF_END;

#if (CPU==PPC85XX)
    installE500ParityErrorRecovery();
#endif /* (CPU==PPC85XX) */

    return (OK);
    }


/***************************************************************************
*
* excIntNestLogInit - allow logging of nested interrupts
*
* Allocate buffers for dispatch and frameless-nesting loggers                   
* (internal arch debugging use only -- function is null in standard             
* builds).                                                                      
*
* NOMANUAL
*
*/

void	excIntNestLogInit(void)
    {
#if defined(_EXC_OFF_CRTL) && defined(LOG_FRAMELESS)
    /*
     * Allocate the log buffer and initialize the control area at 0x80.
     *
     * By aligning at 2x the buffer size, we ensure that the bit into which
     * the address carries when the buffer fills should always be zero,
     * hence wraparound can be done by unconditionally clearing that bit.
     */

    *(void **)LOG_FML_PTR = memalign(2 * LOG_FML_BUF_SIZE, LOG_FML_BUF_SIZE);
    *(int *)RIV_COUNT = *(int *)ESF_COUNT = *(int *)STM_COUNT = 0;
    *(int *)CRT_COUNT_ENT = *(int *)CRT_COUNT_EXIT = *(int *)CRT_COUNT_RESC = 0;
    *(int *)CRT_COUNT_061C = *(int *)CRT_COUNT_RESV = 0;
#endif	/* _EXC_OFF_CRTL && LOG_FRAMELESS */
#ifdef	LOG_DISPATCH
    /* Allocate the log buffer */
    *(void **)LOG_DISP_PTR = memalign(2 * LOG_DISP_BUF_SIZE, LOG_DISP_BUF_SIZE);
    *(void **)LOG_DISP_SAVE = (void *)NULL;
    cacheFlush (DATA_CACHE, (void *)LOG_DISP_SAVE, 2 * sizeof(void *));
#endif	/* LOG_DISPATCH */
    }

/***************************************************************************
*
* vecOffRelocMatch - lookup vector table for relocation offset
*
* This routine looks up the exception vector table excBlTbl[] for the vector
* offset specified.  If the vector offset is found to be relocated, it
* returns the relocated offset.  If it is not relocated, or if such an entry
* is not found, it returns the vector offset given to it as input.
* The status (found unrelocated, found relocated, not found) has no current
* use and thus is not returned to optimize performance.  This can be added
* in future if API not published by then.  Probably a user specified
* relocation table as well.
*
* RETURNS: The input vector offset if not relocated or if not found in
*          excBlTbl[], or the relocated vector offset value.
*
* NOMANUAL
*
*/

vecTblOffset vecOffRelocMatch
    (
    vecTblOffset vector			/* original vector to match */
    )
    {
    FAST int i = 0;

    while (excBlTbl[i].vecOff != 0)
	{
	if (excBlTbl[i].vecOff == vector)
	    {
	    if (excBlTbl[i].vecOffReloc == 0)
		return (vector);
	    else
		return (excBlTbl[i].vecOffReloc);
	    }
	i++;
	}
    return vector;
    }

/***************************************************************************
*
* vecOffRelocMatchRev - reverse lookup vector table for relocation offset
*
* This routine looks up the exception vector table excBlTbl[] for the
* relocated  vector offset specified.  If such a relocated vector offset
* is found, it returns the original offset.  If it has not been relocated,
* or if such an entry is not found, it returns the vector offset given to
* it as input.
*
* RETURNS: The input vector offset if not relocated or if not found in
*          excBlTbl[], or the relocated vector offset value.
*
* NOMANUAL
*
*/

vecTblOffset vecOffRelocMatchRev
    (
    vecTblOffset vector			/* relocated vector to match */
    )
    {
    FAST int i = 0;

    while (excBlTbl[i].vecOff != 0)
	{
	if (excBlTbl[i].vecOffReloc == vector)
		return (excBlTbl[i].vecOff);
	i++;
	}
    return vector;
    }

/*******************************************************************************
*
* excConnect - connect a C routine to an exception vector
*
* Application use of this function is deprecated on the PowerPC architecture.
* Use excVecSet instead.
*
* This routine connects a specified C routine to a specified exception
* vector.  An exception stub is created and in placed at <vector> in the
* exception table.  The address of <routine> is stored in the exception stub
* code.  When an exception occurs, the processor jumps to the exception stub
* code, saves the registers, and call the C routines.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* The registers are saved to an Exception Stack Frame (ESF) which is placed
* on the stack of the task that has produced the exception.  The ESF structure
* is defined in /h/arch/ppc/esfPpc.h.
*
* The only argument passed by the exception stub to the C routine is a pointer
* to the ESF containing the registers values.  The prototype of this C routine
* is as follows:
* .CS
*     void excHandler (ESFPPC *);
* .CE
*
* When the C routine returns, the exception stub restores the registers saved
* in the ESF and continues execution of the current task.
*
* RETURNS: OK if the routine connected successfully, or
*          ERROR if the routine was too far away for a 26-bit offset.
*
* SEE ALSO: excIntConnect(), excVecSet()
*
*/

STATUS excConnect
    (
    VOIDFUNCPTR * vector,   /* (vecTblOffset) exception vector to attach to */
    VOIDFUNCPTR   routine   /* (codePtr) routine to be called */
    )
    {
    return ( excVecConnectCommon ((vecTblOffset) vector,
                                  V_NORM_EXC,
                                  routine,
                                  (vecTblOffset) 0) );
    }

/*******************************************************************************
*
* excIntConnect - connect a C routine to an asynchronous exception vector
*
* Application use of this function is deprecated on the PowerPC architecture.
* Use excVecSet instead.
*
* This routine connects a specified C routine to a specified asynchronous
* exception vector, such as the external interrupt vector (0x500) or the
* decrementer vector (0x900).  An interrupt stub is created and placed at
* <vector> in the exception table.  The address of <routine> is stored in the
* interrupt stub code.  When the asynchronous exception occurs, the processor
* jumps to the interrupt stub code, saves only the requested registers, and
* calls the C routines.
*
* When the C routine is invoked, interrupts are still locked.  It is the
* C routine responsibility to re-enable interrupts.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* Before the requested registers are saved, the interrupt stub switches from the
* current task stack to the interrupt stack.  In the case of nested interrupts,
* no stack switching is performed, because the interrupt is already set.
*
* RETURNS: OK if the routine connected successfully, or
*          ERROR if the routine was too far away for a 26-bit offset.
*
* SEE ALSO: excConnect(), excVecSet()
*/

STATUS excIntConnect
    (
    VOIDFUNCPTR * vector,   /* (vecTblOffset) exception vector to attach to */
    VOIDFUNCPTR   routine   /* (codePtr) routine to be called */
    )
    {
#ifdef INCLUDE_ISR_OBJECTS
    ISR_ID isrId;

/* FIXME should be abstracted here. PIT & DECR are functionally the same */
#ifdef _EXC_OFF_DECR
    if ((UINT32) vector == _EXC_OFF_DECR)
#else /* !_EXC_OFF_DECR */
    /*
     * PPC403, PPC405 and PPC405F use the _EXC_OFF_PIT for the system
     * timer instead of _EXC_OFF_DECR
     */
    if ((UINT32) vector == _EXC_OFF_PIT)
#endif /* _EXC_OFF_DECR */
        {
        if ((isrId = isrArchDecCreate ("ppcDec", (int) vector,
                                       (FUNCPTR) routine, 0, 0)) != NULL)
            {
            return ( excVecConnectCommon ((vecTblOffset) vector,
                                          V_NORM_INT,
                                          (VOIDFUNCPTR) isrArchDecDispatcher,
                                          (vecTblOffset) 0) );
            };
        }
#endif  /* INCLUDE_ISR_OBJECTS */

    return ( excVecConnectCommon ((vecTblOffset) vector,
                                  V_NORM_INT,
                                  routine,
                                  (vecTblOffset) 0) );
    }

/***************************************************************************
*
* excVecConnectCommon - install exception processing wrapper for a vector
*
* The exception or interrupt stub that should reside at the vector 
* processing offset will be copied to the specified location.  The 
* vector type specifies which wrapper to use (normal, critical, or
* machine check).  The branch to the handler will be assembled and
* put in place, which differs in the short and long vector cases.
* For vectors that are relocated for various reasons, a short
* branch will be generated and the stub is installed at the relocated
* offset.
*
* INTERNAL:  excVecConnectCommon is now the generic connect function
*            and the EXC_TYPE is expandable.  excTypeRtnTbl[]
*            specified the stubs to be used for each.  
*            The following routines are maintained for backward 
*            compatibility:
*            excConnect, excIntConnect,
*            excCrtConnect, excIntCrtConnect,
*            excMchkConnect, excIntConnectTimer
* 
* RETURNS: OK or ERROR, if ERROR, no modification is made to memory.
* 
* NOMANUAL
*/

LOCAL STATUS excVecConnectCommon
    (
    vecTblOffset    vector,             /* original vector offset */
    EXC_TYPE	    vType,              /* vector type */
    VOIDFUNCPTR	    handler,            /* handler routine to be called */
    vecTblOffset    reloc               /* relocated vector offset */
    )
    {
    FAST vectorPtr  eVecOrg, eVec;
    BOOL        writeProtected;
    BOOL        writeProtectedAlt = FALSE;

    INSTR	entBranch, hdlrBranch, exitBranch, relocBranch;
    int		entOff, isrOff, exitOff;
    int		stubSize;
    INSTR *     stub;
    INSTR *	cVec;

    /* set (word) offsets into handler */
    switch (vType)
        {
#ifdef _EXC_OFF_CRTL
        case V_CRIT_EXC:
        case V_CRIT_INT:
#ifdef _PPC_MSR_MCE
        case V_MCHK_EXC:
#endif  /* _PPC_MSR_MCE */
            entOff = entCrtOffset;
            isrOff = isrCrtOffset;
            exitOff = exitCrtOffset;
            if (excExtendedVectors == TRUE)
                {
                stub = excExtCrtConnectCode;
                stubSize = sizeof(excExtCrtConnectCode);
                }
            else
                {
                stub = excCrtConnectCode;
                stubSize = sizeof(excCrtConnectCode);
                }
            break;
#endif /* _EXC_OFF_CRTL */
        case V_NORM_EXC:
        case V_NORM_INT:
            entOff = entOffset;
            isrOff = isrOffset;
            exitOff = exitOffset;
            if (excExtendedVectors == TRUE)
                {
                stub = excExtConnectCode;
                stubSize = sizeof(excExtConnectCode);
                }
            else
                {
                stub = excConnectCode;
                stubSize = sizeof(excConnectCode);
                }
            break;
        default:
            return ERROR;
	}

    /* check all error return paths first before modifying */

    eVecOrg = (vectorPtr) (excVecBase + vector);
    if (reloc != (vecTblOffset) 0)
        {
        /* use relocated offsets for branch computation */
        eVec = (vectorPtr) (excVecBase + reloc);
        relocBranch = ppcBrCompute ((VOIDFUNCPTR) eVec, (INSTR *) eVecOrg, 0);

        /* short branch only, else will need to save/restore regs */
        if (relocBranch == (INSTR) 0)
            return ERROR;

        vector = reloc;
        writeProtectedAlt = TRUE;
        }
    else
        {
        /* use original offset */
        eVec = eVecOrg;
        }

    if (excExtendedVectors != TRUE)
        {
	/*
	 * Compute branch instructions for short stub.
	 *
	 * Second parameter specifies vector execution space
	 * since that is where the branches will execute.
	 */

	entBranch  = ppcBrCompute (excTypeRtnTbl[vType].enterRtn,
                                   & ((INSTR *)eVec)[entOff], 1);
	hdlrBranch = ppcBrCompute (handler, & ((INSTR *)eVec)[isrOff], 1);
	exitBranch = ppcBrCompute (excTypeRtnTbl[vType].exitRtn,
                                   & ((INSTR *)eVec)[exitOff], 1);

	/* 0 means branch was out of range, return ERROR */

	if ( (entBranch == (INSTR) 0)  ||
	     (hdlrBranch == (INSTR) 0) ||
	     (exitBranch == (INSTR) 0) )
	    return ERROR;
        }
    
    /*
     * All error return is accounted for, so can start modifying.
     * Note that current excPageUnProtect does not flag an 
     * error state, and assumes always success.  
     * 
     * Note that api of excPageUnProtect assumes no failure.
     */
        
    writeProtected = excPageUnProtect((char *) eVec);
    if (writeProtectedAlt == TRUE)
        writeProtectedAlt = excPageUnProtect((char *) eVecOrg);

    if (reloc != (vecTblOffset) 0)
        {
        * (INSTR *)(CODE_PTR_FROM_VECTOR_PTR(eVecOrg)) = relocBranch;
        CACHE_TEXT_UPDATE ((void *) (CODE_PTR_FROM_VECTOR_PTR(eVecOrg)),
                           sizeof(INSTR));
        }

    cVec = (INSTR *)(CODE_PTR_FROM_VECTOR_PTR(eVec));

    /* copy the stub to the vector location */
    bcopy((char *)stub, (char *)cVec, stubSize);

#ifdef _PPC_MSR_MCE
    /* use SPRG4 for machine check */
    if (vType == V_MCHK_EXC)
        *cVec = (*stub & 0xffe007ff) | SPR_SET(SPRG4_W);
#endif  /* _PPC_MSR_MCE */

    if (excExtendedVectors == TRUE)
        {
    	/* fill in the two halves of the 32-bit function addresses */
	cVec[entOff] =
            (INSTR) INST_LOAD_R3_HI(excTypeRtnTbl[vType].enterRtn);
	cVec[entOff+1] =
            (INSTR) INST_LOAD_R3_LO(excTypeRtnTbl[vType].enterRtn);
	cVec[isrOff] =
            (INSTR) INST_LOAD_R3_HI(handler);
	cVec[isrOff+1] =
            (INSTR) INST_LOAD_R3_LO(handler);
	cVec[exitOff] =
            (INSTR) INST_LOAD_R3_HI(excTypeRtnTbl[vType].exitRtn);
	cVec[exitOff+1] =
            (INSTR) INST_LOAD_R3_LO(excTypeRtnTbl[vType].exitRtn);
	}
    else  /* excExtendedVectors == TRUE */
	{
	/* fill in the branch instructions */
	cVec[entOff]  = entBranch;
	cVec[isrOff]  = hdlrBranch;
	cVec[exitOff] = exitBranch;
	}

    CACHE_TEXT_UPDATE((void *)cVec, stubSize);   /* sync cache */

    /* reenable protection */
    if (writeProtected)
          excPageProtect((char*)eVec);
    if (writeProtectedAlt)
          excPageProtect((char*)eVecOrg);

    return (OK);
    }


#ifdef	_PPC_MSR_CE
# ifdef	_EXC_OFF_CRTL	/* if not, excCrtConnectCode does not exist */
/*******************************************************************************
*
* excCrtConnect - connect a C routine to a critical exception vector (PPC4xx)
*
* Application use of this function is deprecated.  Use excVecSet instead.
*
* This routine connects a specified C routine to a specified critical exception
* vector.  An exception stub is created and in placed at <vector> in the
* exception table.  The address of <routine> is stored in the exception stub
* code.  When an exception occurs, the processor jumps to the exception stub
* code, saves the registers, and call the C routines.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* The registers are saved to an Exception Stack Frame (ESF) which is placed
* on the stack of the task that has produced the exception.  The ESF structure
* is defined in h/arch/ppc/esfPpc.h.
*
* The only argument passed by the exception stub to the C routine is a pointer
* to the ESF containing the register values.  The prototype of this C routine
* is as follows:
* .CS
*     void excHandler (ESFPPC *);
* .CE
*
* When the C routine returns, the exception stub restores the registers saved
* in the ESF and continues execution of the current task.
*
* RETURNS: OK if the routine connected successfully, or
*          ERROR if the routine was too far away for a 26-bit offset.
*
* SEE ALSO: excIntConnect(), excIntCrtConnect, excVecSet()
*/

STATUS excCrtConnect
    (
    VOIDFUNCPTR * vector,   /* (vecTblOffset) exception vector to attach to */
    VOIDFUNCPTR   routine   /* (codePtr) routine to be called */
    )
    {
    return ( excVecConnectCommon ((vecTblOffset) vector,
                                  V_CRIT_EXC,
                                  routine,
                                  (vecTblOffset) 0) );
    }


/*******************************************************************************
*
* excIntCrtConnect - connect a C routine to a critical interrupt vector (PPC4xx)
*
* Application use of this function is deprecated.  Use excVecSet instead.
*
* This routine connects a specified C routine to a specified asynchronous
* critical exception vector such as the critical external interrupt vector
* (0x100), or the watchdog timer vector (0x1020).  An interrupt stub is created
* and placed at <vector> in the exception table.  The address of <routine> is
* stored in the interrupt stub code.  When the asynchronous exception occurs,
* the processor jumps to the interrupt stub code, saves only the requested
* registers, and calls the C routines.
*
* When the C routine is invoked, interrupts are still locked.  It is the
* C routine's responsibility to re-enable interrupts.
*
* The routine can be any normal C routine, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* Before the requested registers are saved, the interrupt stub switches from the
* current task stack to the interrupt stack.  In the case of nested interrupts,
* no stack switching is performed, because the interrupt stack is already set.
*
* RETURNS: OK if the routine connected successfully, or
*          ERROR if the routine was too far away for a 26-bit offset.
*
* SEE ALSO: excConnect(), excCrtConnect, excVecSet()
*/

STATUS excIntCrtConnect
    (
    VOIDFUNCPTR * vector,   /* (vecTblOffset) exception vector to attach to */
    VOIDFUNCPTR   routine   /* (codePtr) routine to be called */
    )
    {
    return ( excVecConnectCommon ((vecTblOffset) vector,
                                  V_CRIT_INT,
                                  routine,
                                  (vecTblOffset) 0) );
    }

#  ifdef _PPC_MSR_MCE   /* also need _EXC_OFF_CRTL for excCrtConnectCode */
/*******************************************************************************
*
* excMchkConnect - connect a C routine to a machine chk exception vector
*
* Application use of this function is deprecated on the PowerPC architecture.
* Use excVecSet instead.
*
* This routine connects a specified C routine to a specified mcheck exception
* vector.  An exception stub is created and in placed at <vector> in the
* exception table.  The address of <routine> is stored in the exception stub
* code.  When an exception occurs, the processor jumps to the exception stub
* code, saves the registers, and call the C routines.
*
* The routine can be any normal C code, except that it must not
* invoke certain operating system functions that may block or perform
* I/O operations.
*
* The registers are saved to an Exception Stack Frame (ESF) which is placed
* on the stack of the task that has produced the exception.  The ESF structure
* is defined in h/arch/ppc/esfPpc.h.
*
* The only argument passed by the exception stub to the C routine is a pointer
* to the ESF containing the register values.  The prototype of this C routine
* is as follows:
* .CS
*     void excHandler (ESFPPC *);
* .CE
*
* When the C routine returns, the exception stub restores the registers saved
* in the ESF and continues execution of the current task.
*
* RETURNS: OK if the routine connected successfully, or
*          ERROR if the routine was too far away for a 26-bit offset.
*
* SEE ALSO: excIntConnect(), excIntCrtConnect, excVecSet()
*/

STATUS excMchkConnect
    (
    VOIDFUNCPTR * vector,   /* (vecTblOffset) exception vector to attach to */
    VOIDFUNCPTR   routine   /* (codePtr) routine to be called */
    )
    {
    return ( excVecConnectCommon ((vecTblOffset) vector,
                                  V_MCHK_EXC,
                                  routine,
                                  (vecTblOffset) 0) );
    }
# endif	/* _PPC_MSR_MCE */
# endif	/* _EXC_OFF_CRTL */
#endif	/* _PPC_MSR_CE */


#if     defined(_EXC_NEW_OFF_PIT) || defined(_EXC_NEW_OFF_FIT)
/*******************************************************************************
*
* excIntConnectTimer - connect a C routine to the FIT or PIT interrupt vector
*
* Application use of this function is deprecated.  Use excVecSet instead.
*
*/

STATUS excIntConnectTimer
    (
    VOIDFUNCPTR * vector,   /* (vecTblOffset) exception vector to attach to */
    VOIDFUNCPTR   routine   /* (codePtr) routine to be called */
    )
    {
    vecTblOffset vectorReloc = vecOffRelocMatch ((vecTblOffset) vector);

    /*
     * not in other exc*Connect, but to preserve original behavior of
     * excIntConnectTimer(), NULL is checked here.
     */
    if (vectorReloc == 0)
	return ERROR;

    return ( excVecConnectCommon ((vecTblOffset) vector,
                                  V_NORM_INT,
                                  routine,
                                  (vecTblOffset) vectorReloc) );
    }
#endif  /* _EXC_NEW_OFF_PIT || _EXC_NEW_OFF_FIT */


/*******************************************************************************
*
* excVecSet - set a CPU exception vector
*
* This routine sets the C routine that will be called when the exception or
* interrupt corresponding to <vector> occurs.  This function doesn't create
* the exception stub; it just replaces the C routine to call in the exception
* stub.
*
* SEE ALSO: excVecGet(), excConnect(), excIntConnect()
*/

void excVecSet
    (
    FUNCPTR *	vector,		/* (vecTblOffset) vector offset */
    FUNCPTR	function	/* (codePtr) address to place in vector */
    )
    {
    INSTR *	newVector;	/* (vectorPtr) */
    INSTR *	newVectorCode;	/* (codePtr) */
    BOOL	writeProtected = FALSE;
    BOOL	writeProtectedAlt = FALSE;

    /*
     * SPR #77145: See Exception Vector Table comment near top of file.
     * Relocated vectors saves wrong vector offset in ESF, thus need to
     * be patched to get right.
     */

    vector = (FUNCPTR *) vecOffRelocMatch ((vecTblOffset) vector);

    /*
     * Although it manipulates the vector space, this function executes
     * in normal code space and hence should use normal-space addresses
     * to access the vector space; however the branch computations need to
     * be done relative to vector execution space.  Compute the vector's
     * address in both spaces by adding the offset to the corresponding
     * base addresses.
     */
    newVectorCode = (INSTR *)((vecTblOffset)vector + excVecBaseAltAdrs);
    newVector = (INSTR *)((vecTblOffset)vector + (vectorBase)excVecBase);

    /*
     * One of the connect functions (excConnect, excIntConnect, excCrtConnect,
     * or excIntCrtConnect) has previously copied the appropriate stub code
     * (excConnectCode[] or excCrtConnectCode[] or their extended versions to
     * the vector location.  We now need to change an instruction in the stub
     * to jump to the new handler function.
     *
     * If the processor supports "critical" exceptions, there are two
     * different stubs and the offset of the jump instruction within
     * the stub depends on whether it is the "critical" or the "normal"
     * stub.  Also for VEC_TYPE_MCHK, the stub code is modified to use
     * SPRG4.  To distinguish between them, we examine the first word of
     * the stub.
     */
    writeProtected = excPageUnProtect((char*)newVector);
    writeProtectedAlt = excPageUnProtect((char*)newVectorCode);

    if (excExtendedVectors == TRUE)
	{
#ifdef	ISR_CRT_OFF
	if ((excExtCrtConnectCode[0] == newVectorCode[0])
#ifdef	_PPC_MSR_MCE
            || (((excExtCrtConnectCode[0] & 0xffe007ff) | SPR_SET(SPRG4_W)) ==
                newVectorCode[0])        /* mchk uses SPRG4 instead */
#endif	/* _PPC_MSR_MCE */
	   )
	    {
	    newVectorCode[isrCrtOffset]   = (INSTR) (0x3c600000
						     | MSW((int)function));
	    newVectorCode[isrCrtOffset+1] = (INSTR) (0x60630000
						     | LSW((int)function));
	    CACHE_TEXT_UPDATE((void *) &newVectorCode[isrCrtOffset],
			      2 * sizeof(INSTR));
	    }
	else
#endif	/* ISR_CRT_OFF */
	    {
	    newVectorCode[isrOffset]   = (INSTR) (0x3c600000
						  | MSW((int)function));
	    newVectorCode[isrOffset+1] = (INSTR) (0x60630000
						  | LSW((int)function));

	    CACHE_TEXT_UPDATE((void *) &newVectorCode[isrOffset],
			      2 * sizeof(INSTR));
	    }
	}
    else    /* non-extended vectors */
	{
#ifdef	ISR_CRT_OFF
	if ((excCrtConnectCode[0] == newVectorCode[0])
#ifdef	_PPC_MSR_MCE
            || (((excExtCrtConnectCode[0] & 0xffe007ff) | SPR_SET(SPRG4_W)) ==
                newVectorCode[0])        /* mchk uses SPRG4 instead */
#endif	/* _PPC_MSR_MCE */
	   )
	    {
	    INSTR routineBranch = ppcBrCompute ( (VOIDFUNCPTR) function,
						 &newVector[isrCrtOffset], 1 );
	    /*
	     * If the function is too far for a 26-bit offset, ppcBrCompute
	     * will return 0. Could set an errno in that case.
	     */
	    if (routineBranch == 0)
		{
		if (_func_logMsg != NULL)
		    _func_logMsg ("Target %08lx for vector %x out of range\n",
				  function, &newVector[isrCrtOffset], 0,0,0,0);
		}
	    else
		{
		newVectorCode[isrCrtOffset] = routineBranch;
		CACHE_TEXT_UPDATE((void *) &newVectorCode[isrCrtOffset],
				sizeof(INSTR));
		}
	    }
	else
#endif	/* ISR_CRT_OFF */
	    {
	    INSTR routineBranch = ppcBrCompute ( (VOIDFUNCPTR) function,
						 &newVector[isrOffset], 1 );
	    /*
	     * If the function is too far for a 26-bit offset, ppcBrCompute
	     * will return 0. Could set an errno in that case.
	     */
	    if (routineBranch == 0)
		{
		if (_func_logMsg != NULL)
		    _func_logMsg ("Target %08lx for vector %x out of range\n",
				  function, &newVector[isrOffset], 0,0,0,0);
		}
	    else
		{
		newVectorCode[isrOffset] = routineBranch;
		CACHE_TEXT_UPDATE((void *) &newVectorCode[isrOffset],
				sizeof(INSTR));
		}
	    }
	}

    if (writeProtected)
	{
	  excPageProtect((char*)newVector);
	}

    if (writeProtectedAlt)
	{
	  excPageProtect((char*)newVectorCode);
	}
    }

/*******************************************************************************
*
* excVecGet - get a CPU exception vector
*
* This routine returns the address of the current C routine connected to
* the <vector>.
*
* RETURNS: the address of the C routine.
*
* SEE ALSO: excVecSet()
*/

FUNCPTR excVecGet
    (
    FUNCPTR * vector			/* (vecTblOffset) vector offset */
    )
    {
    INSTR * vec;	/* (vectorPtr) */
    INSTR * vecCode;	/* (codePtr) */
    codePtr routine;

    /*
     * SPR #77145: See Exception Vector Table comment near top of file.
     * Relocated vectors saves wrong vector offset in ESF, thus need to
     * be patched to get right.
     */

    vector = (FUNCPTR *) vecOffRelocMatch ((vecTblOffset) vector);

    /*
     * Although it examines the vector space, this function executes in
     * normal code space and hence should use normal-space addresses to
     * access the vector space; however the branch computations need to
     * be done relative to vector execution space.  Compute the vector's
     * address in both spaces by adding the offset to the corresponding
     * base addresses.
     */
    vecCode = (INSTR *)((vecTblOffset)vector + excVecBaseAltAdrs);
    vec = (INSTR *)((vecTblOffset)vector + (vectorBase)excVecBase);

    /*
     * One of the "connect" functions (excConnect, excIntConnect,
     * excCrtConnect, or excIntCrtConnect) has previously copied the
     * appropriate stub code (excConnectCode[] or excCrtConnectCode[])
     * to the vector location.  We now need to examine an instruction
     * in the stub and extract a pointer to the handler function.
     *
     * If the processor supports "critical" exceptions, there are two
     * different stubs and the offset of the jump instruction within
     * the stub depends on whether it is the "critical" or the "normal"
     * stub.  Also for VEC_TYPE_MCHK, the stub code is modified to use
     * SPRG4.  To distinguish between them, we examine the first word of
     * the stub.
     */

    /* extract the routine address from the instruction */

    if (excExtendedVectors == TRUE)
	{
	/* extract the two halves of the routine address from the stub */

#ifdef	ISR_CRT_OFF
	if ((excExtCrtConnectCode[0] == vecCode[0])
#ifdef	_PPC_MSR_MCE
            || (((excExtCrtConnectCode[0] & 0xffe007ff) | SPR_SET(SPRG4_W)) ==
                vecCode[0])        /* mchk uses SPRG4 instead */
#endif	/* _PPC_MSR_MCE */
	   )
	    routine = (codePtr)((vecCode[isrCrtOffset] << 16)
				| (vecCode[isrCrtOffset+1] & 0x0000ffff));
	else
#endif	/* ISR_CRT_OFF */

	    routine = (codePtr)((vecCode[isrOffset] << 16)
				| (vecCode[isrOffset+1] & 0x0000ffff));
	}
    else
	{
	    /* extract the routine address from the instruction */

#ifdef	ISR_CRT_OFF
	if ((excCrtConnectCode[0] == vecCode[0])
#ifdef	_PPC_MSR_MCE
            || (((excExtCrtConnectCode[0] & 0xffe007ff) | SPR_SET(SPRG4_W)) ==
                vecCode[0])        /* mchk uses SPRG4 instead */
#endif	/* _PPC_MSR_MCE */
	   )
	    routine = (codePtr) ppcBrExtract (vecCode[isrCrtOffset],
					      &vec[isrCrtOffset]);
	else
#endif	/* ISR_CRT_OFF */

	    routine = (codePtr) ppcBrExtract (vecCode[isrOffset],
					      &vec[isrOffset]);
	}

    return ((FUNCPTR) routine);
    }

/*******************************************************************************
*
* excVecBaseSet - set the exception vector base address
*
* This function sets the vector base address.  It is called by excVecInit(),
* and should not be called by the BSP or application.  To configure a system
* with a non-zero vector base address, adjust VEC_BASE_ADRS in config.h.
*
* Since the vector table is always aligned on a 64KB boundary, the less-
* significant 16 bits of the vector base address are zero.
*
* Newer PowerPC processor designs have a vector base register, referred to
* as the EVPR or IVPR, which determines the more-significant 16 bits.  Older
* designs do not have a vector base register, but the IP or EP bit in the MSR
* sets the base to 0x00000000 or 0xfff00000.  In either case, the base will
* be zero unless changed by calling this function.
*
* RETURNS: N/A
*
* SEE ALSO: excVecBaseGet(), excVecGet(), excVecSet()
*/

void excVecBaseSet
    (
    FUNCPTR * baseAddr		/* new vector base address */
    )
    {
#if	defined(EVPR) || defined(IVPR)
    excVecBase = (vectorBase)((uint32_t)baseAddr & 0x0ffff0000);
# if	defined(IVPR)
    vxIvprSet ((int) excVecBase);
# else	/* defined(IVPR) */
    vxEvprSet ((int) excVecBase);
# endif	/* defined(IVPR) */
#else	/* (EVPR || IVPR) */
    if ((int) baseAddr == _PPC_EXC_VEC_BASE_LOW ||
	(int) baseAddr == _PPC_EXC_VEC_BASE_HIGH)
	{
	/* keep the base address in a static variable */
	excVecBase = (vectorBase)baseAddr;

	excEPSet (baseAddr);	/* set the actual vector base register */
	}
#endif	/* (EVPR || IVPR) */
    }

/*******************************************************************************
*
* excVecBaseGet - get the vector base address
*
* This routine returns the current vector base address that has been set
* with the intVecBaseSet() routine.
*
* RETURNS: The current vector base address.
*
* SEE ALSO: intVecBaseSet()
*/

FUNCPTR * excVecBaseGet (void)
    {
    return (FUNCPTR *)excVecBase;
    }

/*******************************************************************************
*
* excExcHandle - interrupt level handling of exceptions
*
* This routine handles exception traps.  It should never be called except
* from the special assembly language interrupt stub routine.
*
* It prints out a bunch of pertinent information about the trap that
* occurred via excTask.
*
* Note that this routine runs in the context of the task that got the
* exception.
*
* NOMANUAL
*/

void excExcHandle
    (
    ESFPPC *	pEsf			/* pointer to exception stack frame */
    )
    {
    EXC_INFO	excInfo;
    vecTblOffset  vecNum = pEsf->vecOffset;	/* exception vector number */
    REG_SET *	pRegs = &pEsf->regSet;		/* pointer to register on esf */
    void    *   faultAddr;

#ifdef	LOG_DISPATCH
    /* stop dispatch logger, to preserve evidence */
    int key = intLock();
    void * logPtr = *(void **)LOG_DISP_PTR;
    if (logPtr != (void *)NULL)
	{
	*(void **)LOG_DISP_SAVE = logPtr;
	*(void **)LOG_DISP_PTR = (void *)NULL;
	cacheFlush (DATA_CACHE, (void *)LOG_DISP_SAVE, 2 * sizeof(void *));
	}
    intUnlock(key);
#endif	/* LOG_DISPATCH */

    /*
     * SPR #77145: See Exception Vector Table comment near top of file.
     * Relocated vectors saves wrong vector offset in ESF, thus need to
     * be patched to get right.
     */

    vecNum = vecOffRelocMatchRev (vecNum);
    pEsf->vecOffset = vecNum;

#ifdef  WV_INSTRUMENTATION
    /* windview - level 3 event logging */
    EVT_CTX_1(EVENT_EXCEPTION, vecNum);
#endif

    /* 
     * Check if there is a special exception handler installed (for software 
     * breakpoints), and check if the exception was catched by this handler.
     */

    if ((_func_excTrapRtn != NULL) && _func_excTrapRtn (pRegs, pEsf))
	return;

    excGetInfoFromESF (vecNum, pEsf, &excInfo);

    /*
     * The _func_excBaseHook is provided for Wind River components to use
     * the exception mechanism and handle exceptions in their own way.
     * Currently the only user of this feature is objVerify().  The
     * Wind object management system installs a hook during system
     * initialization time, i.e. the hook is always present, to trap
     * accesses to non-existing or protected memory when a user supplies
     * a bad object identifier.
     *
     * The function(s) hooked into _func_excBaseHook should return a
     * non-zero value to indicate that the exception has been handled,
     * and thus excExcHandle() will just return.  A zero return value
     * indicates that normal exception handling should continue.
     *
     * If an additional Wind River subsystem wishes to hook into the
     * exception handling path, the _func_excBaseHook can be daisy
     * chained.  When the subsystem initialization function executes,
     * the existing FUNCPTR value  of _func_excBaseHook should be cached.
     * Then during exception handling the cached FUNCPTR should be called
     * if the exception is not to be handled by the current hook.
     *
     * Note that the simulator arches temporarily overwrite the
     * _func_excBaseHook hook (and do not perform daisy chaining) in
     * vxMemProbeArch().  However, the entire sequence of operations
     * in vxMemProbeArch() where the _func_excBaseHook hook has been
     * absconded is protected with intLock/intUnlock.
     */

    if ((_func_excBaseHook != NULL) &&
	((* _func_excBaseHook) (vecNum, pEsf, pRegs, &excInfo)))
	return;

    /*
     * If the exception happened at interrupt level, or prior to
     * kernel initialization, the ED&R policy handler will most
     * likely reboot the system.
     */

    if (INT_CONTEXT () || (kernelState == TRUE))
	{
	EDR_INTERRUPT_INFO	intInfo;	/* information for system */
	char *		errStr;

#ifdef INCLUDE_FULL_EDR_STUBS
	/*
	 * Error cases handled here are:
	 *  INT_CONTEXT() && pEsf->_errno==S_excLib_INTERRUPT_STACK_OVERFLOW
	 *      interrupt stack overflow
	 *  INT_CONTEXT() && pEsf->_errno!=S_excLib_INTERRUPT_STACK_OVERFLOW
	 *      other interrupt-level exceptions
	 *  !INT_CONTEXT() && kernelState == TRUE
	 *      exception while in kernel state
	 */

	if (INT_CONTEXT())
	    if (pEsf->_errno == S_excLib_INTERRUPT_STACK_OVERFLOW)
		errStr = "interrupt stack overflow!";
	    else
		errStr = "interrupt-level exception!";
	else
	    errStr = "exception while in kernel state!";
#endif /* INCLUDE_FULL_EDR_STUBS */

	intInfo.vector	= vecNum;
	intInfo.pEsf	= (char *) pEsf;
	intInfo.pRegs	= pRegs;
	intInfo.pExcInfo	= &excInfo;
	intInfo.isException	= TRUE;

	/* inject to ED&R */

	EDR_INTERRUPT_FATAL_INJECT(&intInfo,
				   pRegs,
				   &excInfo,
				   (void *) pRegs->pc,
				   errStr);
	return;
	}

    if (_func_ioGlobalStdGet != (FUNCPTR) NULL) /* is I/O been setup yet */
	{
	if ((*_func_ioGlobalStdGet)(STD_ERR) == ERROR)   /* no I/O yet! */
	    {
	    EDR_INIT_INFO	initInfo;	/* information for system */
	    char           *msg = NULL;
	    
	    initInfo.vector		= vecNum;
	    initInfo.pEsf		= (char *) pEsf;
	    initInfo.pRegs		= pRegs;
	    initInfo.pExcInfo		= &excInfo;
	    initInfo.isException	= TRUE;
	    
	    /* inject to ED&R */

#ifdef INCLUDE_FULL_EDR_STUBS
	    msg = "exception before root task!";
#endif /* INCLUDE_FULL_EDR_STUBS */
	    
	    EDR_INIT_FATAL_INJECT (&initInfo,
				   pRegs,
				   &excInfo,
				   (void *) pRegs->pc,
				   msg);
	    return;
	    }
	}

    /* The exception was caused by a task. */

    taskIdCurrent->pExcRegSet = pRegs;		/* for taskRegs[GS]et */

#ifdef INCLUDE_SHELL
    taskIdDefault ((int)taskIdCurrent);		/* update default tid */
#endif  /* INCLUDE_SHELL */

    bcopy ((char *) &excInfo, (char *) &(taskIdCurrent->excInfo),
	   sizeof (EXC_INFO));			/* copy in exc info */


	/*
	 * Set the faultAddr.  This is used when signaling the faulting
	 * task or RTP.
	 */

	if (excInfo.valid & _EXC_INFO_CIA)
	    {
	    faultAddr = (void *)excInfo.cia;
	    }
	else if (excInfo.valid & _EXC_INFO_NIA)
	    {
	    faultAddr = (void *)excInfo.cia;
	    }
	else
	    {
	    faultAddr = (void *) NULL;
	    }

	/*
	 * If _EXC_INFO_DAR, _EXC_INFO_DEAR, or _EXC_INFO_BEAR  is valid 
	 * then override any faultAddr value set above.
	 */

#ifdef	_EXC_INFO_DAR
	if (excInfo.valid & _EXC_INFO_DAR)
	    {
	    faultAddr = (void *)excInfo.dar;
	    }
#endif
#ifdef	_EXC_INFO_DEAR
	if (excInfo.valid & _EXC_INFO_DEAR)
	    {
	    faultAddr = (void *)excInfo.dear;
	    }
#endif
#ifdef	_EXC_INFO_BEAR
	if (excInfo.valid & _EXC_INFO_BEAR)
	    {
# if	(CPU == PPC440 && defined(PPC_440x5))
	    faultAddr = (void *)excInfo.dear;
# else	/* CPU == PPC440 && defined(PPC_440x5) */
	    faultAddr = (void *)excInfo.bear;
# endif	/* CPU == PPC440 && defined(PPC_440x5) */
	    }
#endif	/* _EXC_INFO_BEAR */

    /* An explanation of ED&R interaction with signals:-
     *
     * We invoke the signal hook first, and then inject an
     * error. If the signal hook doesn't return, its because
     * someone has taken care of the problem, in which case its
     * okay for ED&R not to worry about it.
     *
     * This has the advantage of allowing us to merge the
     * error-inject call and the policy invocation into one,
     * potentially.
     */

#ifdef INCLUDE_RTP
    if (IS_KERNEL_TASK (taskIdCurrent))
#endif  /* INCLUDE_RTP */
	{
	if (_func_sigExcKill != NULL)
	    (* _func_sigExcKill) ((int) vecNum, vecNum, pRegs);
	}
#ifdef INCLUDE_RTP
    else
	{
	/* Do signal raise and delivery for RTP task */

	/*
	 * Make sure the exception didn't happen in the kernel code
	 * that is after the system call
	 * in such case we do not run user handlers.
	 */

	if (taskIdCurrent->excCnt == 1)
	    {
	    /* raise and deliver signal exception */

	    if (_func_rtpSigExcKill != NULL)
		{
                    (* _func_rtpSigExcKill) ((int) vecNum, vecNum, pRegs, 
					 faultAddr);

		/*
		 * If signal got delivered never here.... otherwise
		 * a) It is possible that User mode stack is full and
		 *    signal handler context for RTP task could not be
		 *    carved.  In such case apply default policy.
		 * b) It is possible that the sigaction associated with
		 *    this exception signal is SIG_IGN.  As per POSIX
		 *    the behaviour onwards is undefined.
		 *    We will continue and do the default policy.
		 */
		}

	    } /* else exception in syscall, continue */
	}
#endif  /* INCLUDE_RTP */

    /* Call the exception show routine if one has been installed */

    if (_func_excInfoShow != NULL)
	(*_func_excInfoShow) (&excInfo, TRUE, NULL, NULL);

#ifdef	LOG_DISPATCH
    if (logPtr != (void *)NULL)
	printf ("Dispatch logPtr: %#x\n", (int)logPtr);
#endif	/* LOG_DISPATCH */

    /* Invoke legacy exc-hook if installed, and in a kernel task. */

#ifdef INCLUDE_RTP
    if (IS_KERNEL_TASK (taskIdCurrent))
#endif  /* INCLUDE_RTP */
	{
	if (excExcepHook != NULL)		/* 5.0.2 hook? */
	    (* excExcepHook) (taskIdCurrent, vecNum, pEsf);
	}

    /* Now record the exception, since no signal-handler took it. */

#ifdef INCLUDE_RTP
    if (_WRS_IS_SUPV_EXC ())
#endif  /* INCLUDE_RTP */
	{
	EDR_TASK_INFO	taskInfo; /* information for task handling */
	char	       *msg = NULL;

	taskInfo.taskId		= (int) taskIdCurrent;
	taskInfo.status		= 0;
	taskInfo.vector		= vecNum;
	taskInfo.pEsf		= (char *) pEsf;
	taskInfo.pRegs		= pRegs;
	taskInfo.pExcInfo		= &excInfo;
	taskInfo.isException	= TRUE;

	/*
	 * A fatal exception in a kernel task.
	 *
	 * if pEsf->_errno != 0, it was either caused by an exception
	 * stack overflow, or one occurred while constructing the
	 * ESF.  (As of VxWorks 6.1 FCS, a non-zero pEsf->_errno
	 * can only be S_excLib_EXCEPTION_STACK_OVERFLOW or
	 * S_excLib_INTERRUPT_STACK_OVERFLOW; the latter is handled
	 * above.)
	 */

#ifdef INCLUDE_FULL_EDR_STUBS
	if (pEsf->_errno != 0)
	    msg = "fatal exception stack overflow!";
	else
	    msg = "fatal kernel task-level exception!";
#endif /* INCLUDE_FULL_EDR_STUBS */

	EDR_KERNEL_FATAL_INJECT (&taskInfo,
				 pRegs,
				 &excInfo,
				 (void *) pRegs->pc,
				 msg);
	}
#ifdef INCLUDE_RTP
    else
	{
#ifdef INCLUDE_FULL_EDR_STUBS
	EDR_RTP_INFO	rtpInfo; /* information for rtp handling */

	rtpInfo.rtpId	= taskIdCurrent->rtpId;
	rtpInfo.taskId	= (int) taskIdCurrent;
	rtpInfo.options	= 0;
	rtpInfo.status	= 0;
	rtpInfo.vector	= vecNum;
	rtpInfo.pEsf	= (char *) pEsf;
	rtpInfo.pRegs	= pRegs;
	rtpInfo.pExcInfo	= &excInfo;
	rtpInfo.isException	= TRUE;

	/* A fatal exception in an RTP task. */

	EDR_RTP_FATAL_INJECT (&rtpInfo,
			      pRegs,
			      &excInfo,
			      (void *) pRegs->pc,
			      "fatal RTP exception!");
#endif /* INCLUDE_FULL_EDR_STUBS */
	}
#endif  /* INCLUDE_RTP */
    }

/*******************************************************************************
*
* excIntHandle - interrupt level handling of interrupts
*
* This routine handles interrupts. It is never to be called except
* from the special assembly language interrupt stub routine.
*
* It prints out a bunch of pertinent information about the trap that
* occurred, via excTask.
*
* NOMANUAL
*/

void excIntHandle
    (
    ESFPPC *    pEsf                    /* pointer to exception stack frame */
    )
    {
#if USE_INT_RELOC_FUNCTION || defined(_EXC_NEW_OFF_PIT) || \
	defined(_EXC_NEW_OFF_FIT) || defined(_EXC_NEW_OFF_PERF)
    vecTblOffset  vecNum = pEsf->vecOffset;	/* exception vector number */
#endif

    /*
     * SPR #77145: See Exception Vector Table comment near top of file.
     * Relocated vectors saves wrong vector offset in ESF, thus need to
     * be patched to get right.
     */

#if USE_INT_RELOC_FUNCTION
    vecNum = vecOffRelocMatchRev (vecNum);
    pEsf->vecOffset = vecNum;
#else  /* USE_INT_RELOC_FUNCTION */

    /*
     * function call to vecOffRelocMatchRev() is cleaner but more expensive
     * can use following code instead (register compare/branch/set,
     * instead of table walk each entry compare/branch/memset)
     */

#ifdef  _EXC_NEW_OFF_PIT
    if (vecNum == _EXC_NEW_OFF_PIT)
	{
	vecNum = _EXC_OFF_PIT;
	pEsf->vecOffset = vecNum;
	}
    else
#endif  /* _EXC_NEW_OFF_PIT */
#ifdef  _EXC_NEW_OFF_FIT
	if (vecNum == _EXC_NEW_OFF_FIT)
	    {
	    vecNum = _EXC_OFF_FIT;
	    pEsf->vecOffset = vecNum;
	    }
#endif  /* _EXC_NEW_OFF_FIT */
#ifdef  _EXC_NEW_OFF_PERF
    if (vecNum == _EXC_NEW_OFF_PERF)
	{
	vecNum = _EXC_OFF_PERF;
	pEsf->vecOffset = vecNum;
	}
#endif  /* _EXC_NEW_OFF_PERF */

#endif  /* USE_INT_RELOC_FUNCTION */

#ifdef  WV_INSTRUMENTATION
    /*
     * windview - level 3 event is not logged here, since this
     * function is not used for the moment
     */

#if FALSE
    EVT_CTX_1(EVENT_EXCEPTION, vecNum);
#endif
#endif

    if (_func_excIntHook != NULL)
	(*_func_excIntHook) ();

    if (_func_logMsg != NULL)
	_func_logMsg ("Uninitialized interrupt\n", 0,0,0,0,0,0);
    }

#if	CPU == PPC970
/*******************************************************************************
*
* excDecrHandle - default interrupt level handling of decrementer interrupt
*
* This routine handles decrementer interrupts which occur before
* sysClkConnect has been called to install the fully-functional handler.
* Ideally they would be masked until then, but PPC970 cannot mask the
* decrementer interrupt other than by disabling all external interrupts.
*
* NOMANUAL
*/

void excDecrHandle
    (
    ESFPPC *    pEsf                    /* pointer to exception stack frame */
    )
    {
    vxDecSet(INT_MAX);		/* Set to maximum positive 32-bit value */
    }
#endif	/* CPU == PPC970 */

/*****************************************************************************
*
* excGetInfoFromESF - get relevent info from exception stack frame
*
* RETURNS: size of specified ESF
*
* INTERNAL: This code assumes that, for any PPC CPU type, each of bear,
*	    besr, dar, dear, dsisr, fpscr exists either in both ESFPPC
*	    and EXC_INFO, or in neither; and that the member exists in
*	    the structures iff the corresponding _EXC_INFO_* symbol is
*	    #define-d.  ESFPPC and EXC_INFO are defined in esfPpc.h and
*	    excPpcLib.h respectively.
*
* NOMANUAL
*/

LOCAL int excGetInfoFromESF
    (
    FAST vecTblOffset vecNum,
    FAST ESFPPC *pEsf,
    EXC_INFO *pExcInfo
    )
    {
    pExcInfo->vecOff = vecNum;
    pExcInfo->cia = pEsf->regSet.pc;		/* copy cia/nia */
    pExcInfo->msr = pEsf->regSet.msr;		/* copy msr */
    pExcInfo->cr = pEsf->regSet.cr;		/* copy cr */
    switch (vecNum)
	{
	case _EXC_OFF_MACH:
#if     ((CPU == PPC403) || (CPU == PPC405))
	    pExcInfo->bear = pEsf->bear;
	    pExcInfo->besr = pEsf->besr;
	    pExcInfo->valid = (_EXC_INFO_DEFAULT | _EXC_INFO_NIA |
			      _EXC_INFO_BEAR | _EXC_INFO_BESR) & ~_EXC_INFO_CIA;
#elif	(CPU == PPC405F)
	    /* there is no space in the exception info structure to store
	     * besr for 405F, so we just get the bear.
	     */
	    pExcInfo->bear = pEsf->bear;
	    pExcInfo->valid = (_EXC_INFO_DEFAULT | _EXC_INFO_NIA |
			       _EXC_INFO_BEAR) & ~_EXC_INFO_CIA;
#elif	(CPU == PPC440 && defined(PPC_440x5))
	    pExcInfo->mcsr = pEsf->mcsr;
	    pExcInfo->valid = (_EXC_INFO_DEFAULT | _EXC_INFO_NIA |
			       _EXC_INFO_MCSR) & ~_EXC_INFO_CIA;
	    /*
	     * If besr != 0, store it in pad3 and overload xer and dear with
	     * bear_h and bear_l.  Value of pad3 does not matter if besr == 0
	     * since the valid bits will not be set in that case.
	     */
	    if (pEsf->besr)
		{
		pExcInfo->pad3 = pEsf->besr;
		pExcInfo->xer = pEsf->bear_h;
		pExcInfo->dear = pEsf->bear_l;
		pExcInfo->valid |= _EXC_INFO_BESR | _EXC_INFO_BEAR;
		}
#elif	((CPU == PPC509) || (CPU == PPC555) || (CPU == PPC860))
	    pExcInfo->dsisr = pEsf->dsisr;
	    pExcInfo->dar   = pEsf->dar;
	    pExcInfo->valid = _EXC_INFO_DEFAULT | _EXC_INFO_DSISR |
			      _EXC_INFO_DAR;
#elif	(CPU == PPC85XX)
	    pExcInfo->mcesr = pEsf->esr;
	    pExcInfo->dear  = pEsf->dear;
	    pExcInfo->valid = (_EXC_INFO_DEFAULT |
                               _EXC_INFO_MCSR | _EXC_INFO_NIA) &
                              ~_EXC_INFO_CIA;
#else	/* 403 | 405 : 405F : 5xx | 860 : 85XX */
	    pExcInfo->valid = (_EXC_INFO_DEFAULT | _EXC_INFO_NIA) &
								 ~_EXC_INFO_CIA;
#endif	/* 403 | 405 : 405F : 5xx | 860 : 85XX */
	    break;

#if	((CPU == PPC603) || (CPU == PPCEC603) || (CPU == PPC604) || \
	 (CPU == PPC860) || (CPU == PPC85XX)  || (CPU == PPC970))
	case _EXC_OFF_DATA:
#if	  (CPU == PPC85XX)
	    pExcInfo->dear  = pEsf->dear;
	    pExcInfo->mcesr = pEsf->esr;
	    pExcInfo->valid = _EXC_INFO_DEFAULT | _EXC_INFO_DEAR |
			      _EXC_INFO_ESR;
#else	  /* CPU == PPC85XX */
	    pExcInfo->dsisr = pEsf->dsisr;
	    pExcInfo->dar   = pEsf->dar;
	    pExcInfo->valid = _EXC_INFO_DEFAULT | _EXC_INFO_DSISR |
			      _EXC_INFO_DAR;
#endif	  /* CPU == PPC85XX */
	    break;

	case _EXC_OFF_INST:
#if	  (CPU == PPC85XX)
	    pExcInfo->mcesr = pEsf->esr;
	    pExcInfo->valid = (_EXC_INFO_DEFAULT | _EXC_INFO_NIA |
                               _EXC_INFO_ESR) &
                              ~_EXC_INFO_CIA;
#else	  /* CPU == PPC85XX */
	    pExcInfo->valid = (_EXC_INFO_DEFAULT | _EXC_INFO_NIA) &
								 ~_EXC_INFO_CIA;
#endif	  /* CPU == PPC85XX */
	    break;
#endif	/* PPC603 : PPC604 : PPC860 : PPC85XX : PPC970 */

#if	((CPU == PPC509)   || (CPU == PPC555) || (CPU == PPC603) || \
	 (CPU == PPCEC603) || (CPU == PPC604) || (CPU == PPC860) || \
	 (CPU == PPC405F)  || (CPU == PPC970) || (CPU == PPC440))
	case _EXC_OFF_FPU:
	    break;
#endif	/* ((CPU == PPC509)   || (CPU == PPC555) || (CPU == PPC603) ||
	    (CPU == PPCEC603) || (CPU == PPC604) || (CPU == PPC860) ||
	    (CPU == PPC405F)  || (CPU == PPC970) || (CPU == PPC440)) */

	case _EXC_OFF_PROG:
#ifdef	_PPC_MSR_FP
	    if (((taskIdCurrent->options & VX_FP_TASK) != 0) &&
		((vxMsrGet() & _PPC_MSR_FP) != 0))
		{
		/* get the floating point status and control register */
# if    (CPU == PPC440)
		/*
		 * SPECIAL CASE:  440 with FP has no room in EXC_INFO for a
		 * separate fpcsr field, so shares pad3 with the x5 MC handler.
		 */
		pExcInfo->pad3  = pEsf->fpcsr;
# else  /* 440 */
		pExcInfo->fpcsr = pEsf->fpcsr;
# endif /* 440 */
		}
#endif	/* _PPC_MSR_FP */
	    pExcInfo->valid = _EXC_INFO_DEFAULT;
#if	  (CPU == PPC85XX)
	    pExcInfo->mcesr = pEsf->esr;
	    pExcInfo->valid |= _EXC_INFO_ESR;
#endif	  /* CPU == PPC85XX */
	    break;

	case _EXC_OFF_ALIGN:
#ifdef	_EXC_INFO_DEAR
	/* Processors which have a DEAR also set it on
	 * any of the following which they implement.
	 */
	/* cleanup - it is wrong within `case_EXC_OFFALIGN' to add other
	 * exception type cases inside `ifdef _EXC_INFO_DEAR'.  Keeping
	 * for now in e500 for non-e500 compatability with 2.2.1
	 */
# ifdef	_EXC_OFF_PROT
	case _EXC_OFF_PROT:
# endif	/* _EXC_OFF_PROT */
# ifdef	_EXC_OFF_DATA
#  if (CPU != PPC85XX)	/* see comment on cleanup above */
	case _EXC_OFF_DATA:
#  endif  /* CPU != PPC85XX */
# endif	/* _EXC_OFF_DATA */
# ifdef	_EXC_OFF_DATA_MISS
	case _EXC_OFF_DATA_MISS:
# endif	/* _EXC_OFF_DATA_MISS */
	    pExcInfo->dear   = pEsf->dear;
# if	(CPU == PPC85XX)
	    pExcInfo->mcesr  = pEsf->esr;
	    pExcInfo->valid = _EXC_INFO_DEFAULT | _EXC_INFO_DEAR |
                              _EXC_INFO_ESR;
# else	/* CPU == PPC85XX */
	    pExcInfo->valid = _EXC_INFO_DEFAULT | _EXC_INFO_DEAR;
# endif	/* CPU == PPC85XX */

#else	/* _EXC_INFO_DEAR */
	    pExcInfo->dsisr = pEsf->dsisr;
	    pExcInfo->dar   = pEsf->dar;
	    pExcInfo->valid = _EXC_INFO_DEFAULT | _EXC_INFO_DSISR |
			      _EXC_INFO_DAR;
#endif	/* _EXC_INFO_DEAR */
	    break;

	case _EXC_OFF_SYSCALL:
	    pExcInfo->valid = _EXC_INFO_DEFAULT;
	    break;

#if	(CPU == PPC601)
	case _EXC_OFF_IOERR:
	    break;

	case _EXC_OFF_RUN_TRACE:
#elif	((CPU == PPC509)   || (CPU == PPC555) || (CPU == PPC603) || \
	 (CPU == PPCEC603) || (CPU == PPC604) || (CPU == PPC860) || \
	 (CPU == PPC970))
	case _EXC_OFF_TRACE:
# if	((CPU == PPC509) || (CPU == PPC555))
	case _EXC_OFF_FPA:
	case _EXC_OFF_SW_EMUL:
#  if	(CPU == PPC555)
	case _EXC_OFF_IPE:
	case _EXC_OFF_DPE:
#  endif  /* (CPU == PPC555) */
	case _EXC_OFF_DATA_BKPT:
	case _EXC_OFF_INST_BKPT:
	case _EXC_OFF_PERI_BKPT:
	case _EXC_OFF_NM_DEV_PORT:
# elif	((CPU == PPC603) || (CPU == PPCEC603))
	case _EXC_OFF_INST_MISS:
	case _EXC_OFF_LOAD_MISS:
	case _EXC_OFF_STORE_MISS:
# elif	(CPU == PPC604)
	case _EXC_OFF_INST_BRK:
	case _EXC_OFF_SYS_MNG:
# elif	(CPU == PPC860)
	case _EXC_OFF_SW_EMUL:
	case _EXC_OFF_INST_MISS:
	case _EXC_OFF_DATA_MISS:
	case _EXC_OFF_INST_ERROR:
	case _EXC_OFF_DATA_ERROR:
	case _EXC_OFF_DATA_BKPT:
	case _EXC_OFF_INST_BKPT:
	case _EXC_OFF_PERI_BKPT:
	case _EXC_OFF_NM_DEV_PORT:
# endif	/* 5xx : 603 : 604 : 860 */
#elif     ( (CPU == PPC403)||(CPU==PPC405)||(CPU==PPC405F) )
	case _EXC_OFF_WD:
	case _EXC_OFF_DBG:
	case _EXC_OFF_INST:
	case _EXC_OFF_INST_MISS:
	    pExcInfo->valid = _EXC_INFO_DEFAULT;
	    break;
#endif  /* 601 : 5xx | 60x | 860 : 40x -- none needed for 440 */
	default:
	    pExcInfo->valid = _EXC_INFO_DEFAULT;
	    break;
	}

    return (sizeof (ESFPPC));
    }

/*******************************************************************************
*
* rtpExceptionReturn - RTP signal handler return after handling exception
*
* This routine is called from rtpSigHandlerReturn system call.
* The user signal handler should not return when it is installed for the
* the exception handling. In case it returns we apply default policy.
*
* NOMANUAL
*/
void rtpExceptionReturn ()
    {

    /* Do not contnue as the signal handler returned */

    while ((1))
	taskSuspend (0);

    }

/****************************************************************************
 *
 * excPageUnProtect - Unprotects the page specified by the vector address
 *
 * NOTE: To be implemented - check state for success of vmStateGet/Set.
 *       This means the BOOL return is not enough to distinglish
 *       between protected/unprotected, and a failure.
 * 
 * NOMANUAL
 */

LOCAL BOOL excPageUnProtect(char *newVector)
{
    UINT	state;
    int		pageSize = 0;
    char *	pageAddr = 0;
    BOOL        writeProtected = FALSE;

    if (vmLibInfo.vmLibInstalled)
	{
	pageSize = VM_PAGE_SIZE_GET();

	pageAddr = (char *) ((UINT) newVector / pageSize * pageSize);

	if (VM_STATE_GET (NULL, (void *) pageAddr, &state) != ERROR)
	    if ((state & VM_STATE_MASK_WRITABLE) != VM_STATE_WRITABLE)
		{
		writeProtected = TRUE;
		VM_STATE_SET (NULL, pageAddr, pageSize, VM_STATE_MASK_WRITABLE,
			      VM_STATE_WRITABLE);
		}

	}
    return (writeProtected);
}

/****************************************************************************
 *
 * excPageProtect - Unprotects the page specified by the vector address
 *
 * NOMANUAL
 */

LOCAL void excPageProtect(char *newVector)
{
    int pageSize = VM_PAGE_SIZE_GET();
    char * pageAddr = (char *) ((UINT) newVector / pageSize * pageSize);

    VM_STATE_SET (NULL, pageAddr, pageSize,
		  VM_STATE_MASK_WRITABLE, VM_STATE_WRITABLE_NOT);
}


#if (CPU==PPC85XX)
/****************************************************************************
 *
 * genericLayeredExcHdlrInstall - Installs a exception handler
 * This routine takes a exception handler
 * will either return directly from exception or in error go to alternate
 * exception handler. Example would be TLB miss handler.
 * This function receives a descriptor of the handler and exceptions invloved.
 * Not re-entrant at the moment !!
 * NOMANUAL
 */

STATUS genericLayeredExcHdlrInstall
    (
    GENERIC_LAYERED_EXC_INSTALL_DESC *installDesc
    )
    {
    char  * saveHdlrCodeBase;
    char  * saveHdlrBase;
    INSTR   vecBr;
    INSTR   errExit ;

    /* Compute branch between exception address and handler */
    vecBr = ppcBrCompute(installDesc->funcStart,
                         (INSTR *)(excVecBase + installDesc->excOffset),
			 0);
    /* Compute branch from end of handler to err exception address */
    errExit = ppcBrCompute((VOIDFUNCPTR)(excVecBase + installDesc->errExcOffset),
			   ((INSTR *)installDesc->funcEnd) - 1,
			   0);

    /*
     * if handler code base is not NULL then replace the local maintained
     * address with requested on to use if needed
     */

    if ( installDesc->hdlrCodeBase != NULL)
        {
        saveHdlrCodeBase = hdlrCodeBase;
        saveHdlrBase = hdlrBase;
        hdlrCodeBase = installDesc->hdlrCodeBase;
        hdlrBase = installDesc->hdlrBase;
        }

    if ((vecBr == (INSTR)0) || (errExit == (INSTR)0)|| installDesc->forceBase)
        {
        size_t hdlrSz = ((char *)installDesc->funcEnd -
			(char *)installDesc->funcStart);

        vecBr = ppcBrCompute((VOIDFUNCPTR)hdlrBase,
			 (INSTR *)(excVecBase + installDesc->excOffset), 0);
        errExit = ppcBrCompute((VOIDFUNCPTR)(excVecBase + installDesc->errExcOffset),
                               ((INSTR *)(hdlrBase + hdlrSz)) - 1, 0);
        if ((vecBr == (INSTR)0) || (errExit == (INSTR)0))
	    {
	    /* append to sysExcMsg */
	    strcat(sysExcMsg, installDesc->excMsg);
	    sysExcMsg += strlen (sysExcMsg);
	    return (ERROR);
	    }
        else
	    {
	    bcopy((char *)installDesc->funcStart,
		  hdlrCodeBase, hdlrSz - sizeof(INSTR));
	    *(((INSTR *)(hdlrCodeBase + hdlrSz)) - 1) = errExit;
	    CACHE_TEXT_UPDATE((void *)hdlrCodeBase, hdlrSz);
	    hdlrBase += hdlrSz;
	    hdlrCodeBase += hdlrSz;
	    }
        }
   else
        {
        *(((INSTR *)installDesc->funcEnd) - 1) = errExit;
        CACHE_TEXT_UPDATE((void *)((char *)installDesc->funcEnd - sizeof(INSTR)),
			  sizeof(INSTR));
        }

    *(INSTR *)(excVecBaseAltAdrs + installDesc->excOffset) = vecBr;
	CACHE_TEXT_UPDATE((void *)(excVecBase + installDesc->excOffset),
			  sizeof(INSTR));

    if ( installDesc->hdlrCodeBase != NULL)
        {
        installDesc->hdlrCodeBase =  hdlrCodeBase;
        installDesc->hdlrBase = hdlrBase;
        hdlrCodeBase = saveHdlrCodeBase;
        hdlrBase = saveHdlrBase;
        }

    return (OK);
    }

/*************************************************************************
 *
 * installE500ParityErrorRecovery - installs the L1 parity error handler
 * The handler code is with the E500 cache library. Handler not used until
 * IVOR1 set.
 * NOMANUAL
 */

STATUS installE500ParityErrorRecovery (void)
    {
    int key;
    STATUS status;
    GENERIC_LAYERED_EXC_INSTALL_DESC mchkParityDesc;
    static char installErrMsg[] =
        "L1 Parity exception handler install has failed";

    mchkParityDesc.funcStart = &ppcE500Mch;
    mchkParityDesc.funcEnd = &ppcE500MchEnd;
    mchkParityDesc.excOffset = _EXC_OFF_L1_PARITY;
    mchkParityDesc.errExcOffset = _EXC_OFF_MACH;
    mchkParityDesc.hdlrBase = NULL;
    mchkParityDesc.hdlrCodeBase = NULL;
    mchkParityDesc.forceBase = TRUE;
    mchkParityDesc.excMsg = &installErrMsg[0];

    /* Cautious should only be started from single thread */
    key = intLock();
    status = genericLayeredExcHdlrInstall (&mchkParityDesc);
    intUnlock(key);

    return status;
    }
#endif /* (CPU==PPC85XX) */
