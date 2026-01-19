/* fltl.h - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#ifndef FLTL_H
#define FLTL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flflash.h"

/* See interface documentation of functions in ftllite.c	*/

typedef struct tTL TL;		/* Forward definition */
typedef struct tTLrec TLrec; 	/* Defined by translation layer */

struct tTL {
  TLrec		*rec;

  const void FAR0 *(*mapSector)(TLrec *, SectorNo sectorNo, CardAddress *physAddr);
  FLStatus	(*writeSector)(TLrec *, SectorNo sectorNo, void FAR1 *fromAddress);
  FLStatus	(*deleteSector)(TLrec *, SectorNo sectorNo, int noOfSectors);
  void		(*tlSetBusy)(TLrec *, FLBoolean);
  void		(*dismount)(TLrec *);

  #if defined(DEFRAGMENT_VOLUME) || defined(SINGLE_BUFFER)
  FLStatus	(*defragment)(TLrec *, long FAR2 *bytesNeeded);
  #endif

  #ifdef FORMAT_VOLUME
  SectorNo 	(*sectorsInVolume)(TLrec *);
  #endif
};


#ifdef FORMAT_VOLUME
#include "dosformt.h"
#endif

/* Translation layer registration information */

extern int noOfTLs;	/* No. of translation layers actually registered */

typedef struct {
  FLStatus (*mountRoutine) (FLFlash *flash, TL *tl, FLFlash **volForCallback);
#ifdef FORMAT_VOLUME
  FLStatus (*formatRoutine) (FLFlash *flash, FormatParams FAR1 *formatParams);
#endif
} TLentry;

extern TLentry tlTable[];


extern FLStatus	flMount(unsigned volNo, TL *, FLFlash *);

#ifdef FORMAT_VOLUME
extern FLStatus	flFormat(unsigned, FormatParams FAR1 *formatParams);
#endif

#ifdef __cplusplus
}
#endif

#endif

