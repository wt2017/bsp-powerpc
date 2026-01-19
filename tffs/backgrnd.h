/* backgrnd.h - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#ifndef BACKGRND_H
#define BACKGRND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flsocket.h"

#define BG_RESUME	1
#define	BG_SUSPEND	2

#ifdef BACKGROUND

extern int flForeground(int sendValue);
extern int flBackground(int sendValue);
extern int flStartBackground(unsigned volNo, void (*routine)(void *), void *object);
extern void flCreateBackground(void);

#else

#define flForeground(n)   BG_RESUME
#define flBackground(n)

#endif

#ifdef __cplusplus
}
#endif

#endif

