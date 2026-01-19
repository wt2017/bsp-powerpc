/* backgrnd.c - True Flash File System */

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
#include "backgrnd.h"

#ifdef BACKGROUND

#include <setjmp.h>
#include <dos.h>

static void (*backgroundTask)(void *);

typedef struct {
  jmp_buf	registers;
  unsigned	mappingContext;
  void *	object;
  FLSocket *	socket;
} Context;

static Context foregroundContext, backgroundContext, *currentContext;

static int switchContext(Context *toContext, int sendValue)
{
  int state;

  if (toContext == currentContext)
    return 0;

  state = setjmp(currentContext->registers);	/* save our state */
  if (state == 0) {
    if (backgroundContext.socket) {
      currentContext->mappingContext = flGetMappingContext(backgroundContext.socket);
      if (toContext->mappingContext != UNDEFINED_MAPPING)
	flMap(backgroundContext.socket,(CardAddress) toContext->mappingContext << 12);
    }
    currentContext = toContext;
    longjmp(toContext->registers,sendValue);
  }
  /* We are back here when target task suspends, and 'state'
     is the 'sendValue' on suspend */
  return state;
}


int flForeground(int sendValue)
{
  return switchContext(&foregroundContext,sendValue);
}


int flBackground(int sendValue)
{
  return switchContext(&backgroundContext,sendValue);
}


static char backgroundStack[200];

int flStartBackground(unsigned volNo, void (*routine)(void *), void *object)
{
  if (currentContext != &foregroundContext)
    return 0;

  while (backgroundTask)		/* already exists */
    flBackground(BG_RESUME);		/* run it until it finishes */
  backgroundTask = routine;
  backgroundContext.object = object;
  backgroundContext.socket = flSocketOf(volNo);
  flNeedVcc(backgroundContext.socket);

  return flBackground(BG_RESUME);
}


void flCreateBackground(void)
{
  FLMutex dummyMutex;

  foregroundContext.socket = backgroundContext.socket = NULL;

  if (setjmp(foregroundContext.registers) != 0)
    return;

#ifdef __WIN32__
   _ESP = (void *) (backgroundStack + sizeof backgroundStack);
#else
  flStartCriticalSection(&dummyMutex);
  _SP = FP_OFF((void far *) (backgroundStack + sizeof backgroundStack));
  _SS = FP_SEG((void far *) (backgroundStack + sizeof backgroundStack));
  flEndCriticalSection(&dummyMutex);
#endif
  backgroundTask = NULL;
  if (setjmp(backgroundContext.registers) == 0) {
    currentContext = &foregroundContext;
    longjmp(foregroundContext.registers,1);       /* restore stack and continue */
  }

  /* We are back here with our new stack when 'background' is called */
  for (;;) {
    if (backgroundTask) {
      (*backgroundTask)(backgroundContext.object);
      flDontNeedVcc(backgroundContext.socket);
      backgroundTask = NULL;
    }
    flForeground(-1);
  }
}

#endif


