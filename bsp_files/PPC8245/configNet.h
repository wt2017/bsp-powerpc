/* configNet.h - network configuration header */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
2005-06-16 bjm  add second motfcc to endDevTbl(BBTN710) 
01d,12jun02,kab  SPR 74987: cplusplus protection
01c,17oct01,jrs  Upgrade to veloce
		 changed INCLUDE_MOT_FCC to INCLUDE_MOTFCCEND - SPR #33914
01b,15jul99,ms_  make compliant with our coding standards
01a,05apr99,cn 	written from ads860/configNet.h, 01f.

*/
 
#ifndef INCconfigNeth
#define INCconfigNeth

#ifdef __cplusplus
    extern "C" {
#endif

#include "vxWorks.h"
#include "end.h"
#include "proj_config.h"

/* defines */

#ifdef INCLUDE_END_FEI_82557
/*
 * SENS Device for FEI
 */
#define	FEI_82557_FUNC		fei82557EndLoad
#define FEI_82557_STRING	"-1:0:64:128:0"   /* <memBase>:<memSize>:<nTfds>:<nRfds>:<flags> */
IMPORT END_OBJ* FEI_82557_FUNC(char*, void*);
#endif /* INCLUDE_END_FEI_82557 */

#ifdef INCLUDE_MOTFCCEND
/* Motorola Fast Communication Controller */
#define MOT_FCC_LOAD_FUNC	sysMotFccEndLoad
#define MOT_FCC_LOAD_STRING ""

IMPORT END_OBJ* MOT_FCC_LOAD_FUNC (char *, void*);

#endif /* INCLUDE_FEI_END */

/* max number of SENS ipAttachments we can have */
 
END_TBL_ENTRY endDevTbl [] =
{
#ifdef INCLUDE_MOTFCCEND
#ifdef INCLUDE_FCC1
    { 0, MOT_FCC_LOAD_FUNC, MOT_FCC_LOAD_STRING, 1, (void *)1 , FALSE},
#endif /*INCLUDE_FCC1 */
#ifdef INCLUDE_FCC2
    { 1, MOT_FCC_LOAD_FUNC, MOT_FCC_LOAD_STRING, 1, (void *)2 , FALSE},
#endif /* INCLUDE_FCC2 */
#endif /* INCLUDE_MOTFCCEND */

#ifdef INCLUDE_END_FEI_82557
   { 0, FEI_82557_FUNC, FEI_82557_STRING, 0, NULL, FALSE},
#endif /* INCLUDE_END_FEI_82557*/
    { 0, END_TBL_END, NULL, 0, NULL, FALSE},
};

#ifdef __cplusplus
    }
#endif

#endif /* INCconfigNeth */
