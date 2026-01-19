/* flflash.h - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#ifndef FLFLASH_H
#define FLFLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flsocket.h"

/* Some useful types for mapped Flash locations */

typedef volatile unsigned char FAR0 * FlashPTR;
typedef volatile unsigned short int FAR0 * FlashWPTR;
typedef volatile unsigned long FAR0 * FlashDPTR;

typedef unsigned short FlashType;	/* JEDEC id */

#define	NOT_FLASH	0

/* page characteristics flags */
#define  BIG_PAGE    0x0100             /* page size > 100H*/
#define  FULL_PAGE  0x0200          	/* no partial page programming*/

/* MTD write routine mode flags */
#define OVERWRITE	1	/* Overwriting non-erased area  */
#define EDC		2	/* Activate ECC/EDC		*/
#define EXTRA		4	/* Read/write spare area	*/


/*----------------------------------------------------------------------*/
/*	         Flash array identification structure			*/
/*									*/
/* This structure contains a description of the Flash array and		*/
/* routine pointers for the map, read, write & erase functions.		*/
/* 									*/
/* The structure is initialized by the MTD that identifies the Flash	*/
/* array.                                                               */
/* On entry to an MTD, the Flash structure contains default routines	*/
/* for all operations. This routines are sufficient forread-only access	*/
/* to NOR Flash on a memory-mapped socket. The MTD should override the	*/
/* default routines with MTD specific ones when appropriate.		*/
/*----------------------------------------------------------------------*/

/* Flash array identification structure */

typedef struct tFlash FLFlash;		/* Forward definition */

struct tFlash {
  FlashType	type;			/* Flash device type (JEDEC id) */
  long int	erasableBlockSize;	/* Smallest physically erasable size
					   (with interleaving taken in account) */
  long int	chipSize;		/* chip size */
  int		noOfChips;		/* no of chips in array */
  int		interleaving;		/* chip interleaving
					   (The interleaving is defined as
					   the address difference between
					   two consecutive bytes on a chip) */
  unsigned	flags;			/* Special capabilities & options
					   Bits 0-7 may be used by FLite.
					   Bits 8-15 are not used bt FLite
					   and may be used by MTD's for MTD-
					   specific purposes. */
  void *	mtdVars;		/* Points to MTD private area for
					   this socket.
					   This field, if used by the MTD, is
					   initialized by the MTD identification
					   routine */
  FLSocket *	socket;			/* Socket of this drive */


/* Flag bit values */

#define SUSPEND_FOR_WRITE	1	/* MTD provides suspend for write */
#define	NFTL_ENABLED		2	/* Flash can run NFTL */


/*----------------------------------------------------------------------*/
/*      	          f l a s h . m a p				*/
/*									*/
/* MTD specific map routine						*/
/*                                                                      */
/* The default routine maps by socket mapping, and is suitable for all  */
/* NOR Flash.                                                           */
/* NAND or other type Flash should use map-through-copy emulation: Read */
/* a block of Flash to an internal buffer and return a pointer to that	*/
/* buffer.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to map				*/
/*	length		: Length to map					*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to required card address				*/
/*----------------------------------------------------------------------*/
  void FAR0 *	(*map)(FLFlash *, CardAddress, int);

/*----------------------------------------------------------------------*/
/*      	          f l a s h . r e a d				*/
/*									*/
/* MTD specific Flash read routine					*/
/*									*/
/* The default routine reads by copying from a mapped window, and is	*/
/* suitable for all NOR Flash.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to read				*/
/*	buffer		: Area to read into				*/
/*	length		: Length to read				*/
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus	(*read)(FLFlash *, CardAddress, void FAR1 *, int, int);

/*----------------------------------------------------------------------*/
/*                       f l a s h . w r i t e				*/
/*									*/
/* MTD specific Flash write routine					*/
/*									*/
/* The default routine returns a write-protect error.			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Card address to write to			*/
/*      buffer		: Address of data to write			*/
/*	length		: Number of bytes to write			*/
/*	modes		: See write mode flags definition above		*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/
  FLStatus 	(*write)(FLFlash *, CardAddress, const void FAR1 *, int, int);

/*----------------------------------------------------------------------*/
/*                       f l a s h . e r a s e				*/
/*									*/
/* Erase one or more contiguous Flash erasable blocks			*/
/*									*/
/* The default routine returns a write-protect error.			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      firstErasableBlock : Number of first block to erase		*/
/*	numOfErasableBlocks: Number of blocks to erase			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/
  FLStatus 	(*erase)(FLFlash *, int, int);

/*----------------------------------------------------------------------*/
/*      	 f l a s h . s e t P o w e r O n C a l l b a c k	*/
/*									*/
/* Register power on callback routine. Default: no routine is 		*/
/* registered.								*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/
  void 	(*setPowerOnCallback)(FLFlash *);
};



/* MTD registration information */

extern int noOfMTDs;	/* No. of MTDs actually registered */

typedef FLStatus (*MTDidentifyRoutine) (FLFlash vol);

extern MTDidentifyRoutine mtdTable[];


/* See interface documentation of functions in flash.c	*/

extern void flIntelIdentify(FLFlash *, 
                            void (*)(FLFlash *, CardAddress, unsigned char, FlashPTR), 
                            CardAddress);

extern FLStatus	flIntelSize(FLFlash *, 
                            void (*)(FLFlash *, CardAddress, unsigned char, FlashPTR), 
                            CardAddress);

extern FLStatus	flIdentifyFlash(FLSocket *socket, FLFlash *flash);

#ifdef __cplusplus
}
#endif

#endif

