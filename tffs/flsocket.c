/* dosformt.c - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#include "flsocket.h"

unsigned noOfDrives = 0;	/* No. of drives actually registered */

static FLSocket vols[DRIVES];

#ifndef SINGLE_BUFFER
#ifdef MALLOC_TFFS
static FLBuffer *volBuffers[DRIVES];
#else
static FLBuffer volBuffers[DRIVES];
#endif
#endif

/*----------------------------------------------------------------------*/
/*      	          f l S o c k e t N o O f 			*/
/*									*/
/* Gets the volume no. connected to a socket				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/* 	volume no. of socket						*/
/*----------------------------------------------------------------------*/

unsigned flSocketNoOf(const FLSocket vol)
{
  return vol.volNo;
}


/*----------------------------------------------------------------------*/
/*      	          f l S o c k e t O f	   			*/
/*									*/
/* Gets the socket connected to a volume no.				*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no. for which to get socket		*/
/*                                                                      */
/* Returns:                                                             */
/* 	socket of volume no.						*/
/*----------------------------------------------------------------------*/

FLSocket *flSocketOf(unsigned volNo)
{
  return &vols[volNo];
}

#ifndef SINGLE_BUFFER
/*----------------------------------------------------------------------*/
/*      	          f l B u f f e r O f	   			*/
/*									*/
/* Gets the buffer connected to a volume no.				*/
/*									*/
/* Parameters:                                                          */
/*	volNo		: Volume no. for which to get socket		*/
/*                                                                      */
/* Returns:                                                             */
/* 	buffer of volume no.						*/
/*----------------------------------------------------------------------*/

FLBuffer *flBufferOf(unsigned volNo)
{
#ifdef MALLOC_TFFS
  return volBuffers[volNo];
#else
  return &volBuffers[volNo];
#endif
}
#endif /* SINGLE_BUFFER */

/*----------------------------------------------------------------------*/
/*      	        f l W r i t e P r o t e c t e d			*/
/*									*/
/* Returns the write-protect state of the media				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	0 = not write-protected, other = write-protected		*/
/*----------------------------------------------------------------------*/

FLBoolean flWriteProtected(FLSocket vol)
{
  return vol.writeProtected(&vol);
}


#ifndef FIXED_MEDIA

/*----------------------------------------------------------------------*/
/*      	      f l R e s e t C a r d C h a n g e d		*/
/*									*/
/* Acknowledges a media-change condition and turns off the condition.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flResetCardChanged(FLSocket vol)
{
  if (vol.getAndClearCardChangeIndicator)
      vol.getAndClearCardChangeIndicator(&vol);  /* turn off the indicator */

  vol.cardChanged = FALSE;
}


/*----------------------------------------------------------------------*/
/*      	          f l M e d i a C h e c k			*/
/*									*/
/* Checks the presence and change status of the media			*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/* 	flOK		->	Media present and not changed		*/
/*	driveNotReady   ->	Media not present			*/
/*	diskChange	->	Media present but changed		*/
/*----------------------------------------------------------------------*/

FLStatus flMediaCheck(FLSocket vol)
{
  if (!vol.cardDetected(&vol)) {
    vol.cardChanged = TRUE;
    return flDriveNotReady;
  }

  if (vol.getAndClearCardChangeIndicator &&
      vol.getAndClearCardChangeIndicator(&vol))
    vol.cardChanged = TRUE;

  return vol.cardChanged ? flDiskChange : flOK;
}

#endif

/*----------------------------------------------------------------------*/
/*      	         f l I n i t S o c k e t s		        */
/*									*/
/* First call to this module: Initializes the controller and all sockets*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flInitSockets(void)
{
  unsigned volNo;
  FLSocket vol = vols;

  for (volNo = 0; volNo < noOfDrives; volNo++, pVol++) {
    flSetWindowSpeed(&vol, 250);
    flSetWindowBusWidth(&vol, 16);
    flSetWindowSize(&vol, 2);		/* make it 8 KBytes */

    vol.cardChanged = FALSE;

  #ifndef SINGLE_BUFFER
  #ifdef MALLOC_TFFS
    /* allocate buffer for this socket */
    volBuffers[volNo] = (FLBuffer *)MALLOC_TFFS(sizeof(FLBuffer));
    if (volBuffers[volNo] == NULL) {
    #ifdef DEBUG_PRINT
      DEBUG_PRINT("Debug: failed allocating sector buffer.\n");
    #endif
      return flNotEnoughMemory;
    }
  #endif
  #endif

    checkStatus(vol.initSocket(&vol));

  #ifdef SOCKET_12_VOLTS
    vol.VppOff(&vol);
    vol.VppState = PowerOff;
    vol.VppUsers = 0;
  #endif
    vol.VccOff(&vol);
    vol.VccState = PowerOff;
    vol.VccUsers = 0;
  }

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	     f l G e t M a p p i n g C o n t e x t		*/
/*									*/
/* Returns the currently mapped window page (in 4KB units)		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	unsigned int	: Current mapped page no.			*/
/*----------------------------------------------------------------------*/

unsigned flGetMappingContext(FLSocket vol)
{
  return vol.window.currentPage;
}


/*----------------------------------------------------------------------*/
/*      	                f l M a p			        */
/*									*/
/* Maps the window to a specified card address and returns a pointer to */
/* that location (some offset within the window).			*/
/*									*/
/* NOTE: Addresses over 128M are attribute memory. On PCMCIA adapters,	*/
/* subtract 128M from the address and map to attribute memory.		*/
/*                                                                      */
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      address		: Byte-address on card. NOT necessarily on a 	*/
/*			  full-window boundary.				*/
/*			  If above 128MB, address is in attribute space.*/
/*                                                                      */
/* Returns:                                                             */
/*	Pointer to a location within the window mapping the address.	*/
/*----------------------------------------------------------------------*/

void FAR0 *flMap(FLSocket vol, CardAddress address)
{
  unsigned pageToMap;

  if (vol.window.currentPage == UNDEFINED_MAPPING)
    vol.setWindow(&vol);
  pageToMap = (unsigned) ((address & -vol.window.size) >> 12);

  if (vol.window.currentPage != pageToMap) {
    vol.setMappingContext(&vol, pageToMap);
    vol.window.currentPage = pageToMap;
    vol.remapped = TRUE;	/* indicate remapping done */
  }

  return addToFarPointer(vol.window.base,address & (vol.window.size - 1));
}


/*----------------------------------------------------------------------*/
/*      	      f l S e t W i n d o w B u s W i d t h		*/
/*									*/
/* Requests to set the window bus width to 8 or 16 bits.		*/
/* Whether the request is filled depends on hardware capabilities.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      width		: Requested bus width				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowBusWidth(FLSocket vol, unsigned width)
{
  vol.window.busWidth = width;
  vol.window.currentPage = UNDEFINED_MAPPING;	/* force remapping */
}


/*----------------------------------------------------------------------*/
/*      	     f l S e t W i n d o w S p e e d			*/
/*									*/
/* Requests to set the window speed to a specified value.		*/
/* The window speed is set to a speed equal or slower than requested,	*/
/* if possible in hardware.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      nsec		: Requested window speed in nanosec.		*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowSpeed(FLSocket vol, unsigned nsec)
{
  vol.window.speed = nsec;
  vol.window.currentPage = UNDEFINED_MAPPING;	/* force remapping */
}


/*----------------------------------------------------------------------*/
/*      	      f l S e t W i n d o w S i z e			*/
/*									*/
/* Requests to set the window size to a specified value (power of 2).	*/
/* The window size is set to a size equal or greater than requested,	*/
/* if possible in hardware.						*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      sizeIn4KBUnits : Requested window size in 4 KByte units.	*/
/*			 MUST be a power of 2.				*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetWindowSize(FLSocket vol, unsigned sizeIn4KBunits)
{
  vol.window.size = (long) (sizeIn4KBunits) * 0x1000L;
	/* Size may not be possible. Actual size will be set by 'setWindow' */
  vol.window.base = physicalToPointer((long) vol.window.baseAddress << 12,
				      vol.window.size,vol.volNo);
  vol.window.currentPage = UNDEFINED_MAPPING;	/* force remapping */
}


/*----------------------------------------------------------------------*/
/*      	     f l S o c k e t S e t B u s y				*/
/*									*/
/* Notifies the start and end of a file-system operation.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      state		: TFFS_ON (1) = operation entry			*/
/*			  TFFS_OFF(0) = operation exit			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSocketSetBusy(FLSocket vol, FLBoolean state)
{
  if (state == TFFS_OFF) {
#if POLLING_INTERVAL == 0
    /* If we are not polling, activate the interval routine before exit */
    flIntervalRoutine(&vol);
#endif
  }
  else {
    vol.window.currentPage = UNDEFINED_MAPPING;	/* don't assume mapping still valid */
    vol.remapped = TRUE;
  }
}


/*----------------------------------------------------------------------*/
/*      	            f l N e e d V c c				*/
/*									*/
/* Turns on Vcc, if not on already					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

void flNeedVcc(FLSocket vol)
{
  vol.VccUsers++;
  if (vol.VccState == PowerOff) {
    vol.VccOn(&vol);
    if (vol.powerOnCallback)
      vol.powerOnCallback(vol.flash);
  }
  vol.VccState = PowerOn;
}


/*----------------------------------------------------------------------*/
/*      	         f l D o n t N e e d V c c			*/
/*									*/
/* Notifies that Vcc is no longer needed, allowing it to be turned off. */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDontNeedVcc(FLSocket vol)
{
  if (vol.VccUsers > 0)
    vol.VccUsers--;
}

#ifdef SOCKET_12_VOLTS

/*----------------------------------------------------------------------*/
/*      	            f l N e e d V p p				*/
/*									*/
/* Turns on Vpp, if not on already					*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, failed otherwise		*/
/*----------------------------------------------------------------------*/

FLStatus flNeedVpp(FLSocket vol)
{
  vol.VppUsers++;
  if (vol.VppState == PowerOff)
    checkStatus(vol.VppOn(&vol));
  vol.VppState = PowerOn;

  return flOK;
}


/*----------------------------------------------------------------------*/
/*      	         f l D o n t N e e d V p p			*/
/*									*/
/* Notifies that Vpp is no longer needed, allowing it to be turned off. */
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flDontNeedVpp(FLSocket vol)
{
  if (vol.VppUsers > 0)
    vol.VppUsers--;
  
  if (vol.VppUsers == 0)		/* ADDED. to support ss5 */
      {
      vol.VppState = PowerOff;
      vol.VppOff (&vol);
      }
}

#endif	/* SOCKET_12_VOLTS */


/*----------------------------------------------------------------------*/
/*      	    f l S e t P o w e r O n C a l l b a c k			*/
/*									*/
/* Sets a routine address to call when powering on the socket.		*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*      routine		: Routine to call when turning on power		*/
/*	flash		: Flash object of routine			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flSetPowerOnCallback(FLSocket vol, void (*routine)(void *flash), void *flash)
{
  vol.powerOnCallback = routine;
  vol.flash = flash;
}



/*----------------------------------------------------------------------*/
/*      	      f l I n t e r v a l R o u t i n e			*/
/*									*/
/* Performs periodic socket actions: Checks card presence, and handles  */
/* the Vcc & Vpp turn off mechanisms.					*/
/*                                                                      */
/* The routine may be called from the interval timer or sunchronously.	*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flIntervalRoutine(FLSocket vol)
{
#ifndef FIXED_MEDIA
  if (vol.getAndClearCardChangeIndicator == NULL &&
      !vol.cardChanged)
    if (!vol.cardDetected(&vol))	/* Check that the card is still there */
      vol.cardChanged = TRUE;
#endif

  if (vol.VppUsers == 0) {
    if (vol.VppState == PowerOn)
      vol.VppState = PowerGoingOff;
    else if (vol.VppState == PowerGoingOff) {
      vol.VppState = PowerOff;
#ifdef SOCKET_12_VOLTS
      vol.VppOff(&vol);
#endif
    }
    if (vol.VccUsers == 0) {
      if (vol.VccState == PowerOn)
	vol.VccState = PowerGoingOff;
      else if (vol.VccState == PowerGoingOff) {
	vol.VccState = PowerOff;
	vol.VccOff(&vol);
      }
    }
  }
}

#ifdef EXIT
/*----------------------------------------------------------------------*/
/*      	      f l E x i t S o c k e t				*/
/*									*/
/* Reset the socket and free resources that were allocated for this	*/
/* socket.								*/
/* This function is called when FLite exits.				*/
/*									*/
/* Parameters:                                                          */
/*	vol		: Pointer identifying drive			*/
/*                                                                      */
/*----------------------------------------------------------------------*/

void flExitSocket(FLSocket vol)
{
  flMap(&vol, 0);                           /* reset the mapping register */
  flDontNeedVcc(&vol);
  flSocketSetBusy(&vol,TFFS_OFF);
  vol.freeSocket(&vol);                     /* free allocated resources */
#ifndef SINGLE_BUFFER
#ifdef MALLOC_TFFS
  FREE_TFFS(volBuffers[vol.volNo]);
#endif
#endif
}
#endif /* EXIT */


