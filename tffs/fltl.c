/* fltl.c - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#include "flflash.h"
#include "fltl.h"


extern int noOfTLs;	/* No. of translation layers actually registered */

extern TLentry tlTable[];


/*----------------------------------------------------------------------*/
/*      	             m o u n t 					*/
/*									*/
/* Mount a translation layer						*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	tl		: Where to store translation layer methods	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flMount(unsigned volNo, TL *tl, FLFlash *flash)
{
  FLFlash *volForCallback;
  FLSocket *socket = flSocketOf(volNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  checkStatus(flIdentifyFlash(socket,flash));

  for (iTL = 0; iTL < noOfTLs && status != flOK; iTL++)
    status = tlTable[iTL].mountRoutine(flash,tl, &volForCallback);

  volForCallback->setPowerOnCallback(volForCallback);

  return status;
}


#ifdef FORMAT_VOLUME

/*----------------------------------------------------------------------*/
/*      	             f o r m a t 				*/
/*									*/
/* Formats the Flash volume						*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no.					*/
/*	formatParams	: Address of FormatParams structure to use	*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flFormat(unsigned volNo, FormatParams FAR1 *formatParams)
{
  FLFlash flash;
  FLSocket *socket = flSocketOf(volNo);
  FLStatus status = flUnknownMedia;
  int iTL;

  checkStatus(flIdentifyFlash(socket,&flash));

  for (iTL = 0; iTL < noOfTLs && status == flUnknownMedia; iTL++)
    status = tlTable[iTL].formatRoutine(&flash,formatParams);

  return status;
}

#endif


