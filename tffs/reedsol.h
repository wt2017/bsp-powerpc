/* reedsol.h - True Flash File System */

/* Copyright 1984-2004 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* FAT-FTL Lite Software Development Kit
 * Copyright (C) M-Systems Ltd. 1995-1997	*/

/*
modification history
--------------------
01a,29jul04,alr  modified file header, restarted history
*/

#ifndef FLEDC_H
#define FLEDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flbase.h"

typedef enum { NO_EDC_ERROR, CORRECTABLE_ERROR, UNCORRECTABLE_ERROR, EDC_ERROR } EDCstatus;

EDCstatus flCheckAndFixEDC(char FAR1 *block, char *syndrom, FLBoolean byteSwap);

#ifdef __cplusplus
}
#endif

#endif
