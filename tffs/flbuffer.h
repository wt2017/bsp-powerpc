/* flbuffer.h - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#ifndef FLBUFFER_H
#define FLBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flbase.h"

typedef struct {
  unsigned char data[SECTOR_SIZE];	/* sector buffer */
  SectorNo	sectorNo;		/* current sector in buffer */
  void		*owner;			/* owner of buffer */
  FLBoolean	dirty;			/* sector in buffer was changed */
  FLBoolean	checkPoint;		/* sector in buffer must be flushed */
} FLBuffer;

#ifdef __cplusplus
}
#endif

#endif
