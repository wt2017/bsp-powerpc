# Makefile - makefile for bsp/config/ads82xx
#
# Copyright 1984-2003 Wind River Systems, Inc.
#
# modification history
# --------------------
# 01b,05nov04,dtr  Add VXWORKS define for security engine drivers.
# 01a,19dec03,dtr  created from ads826x Makefile
#
# DESCRIPTION
# This file contains rules for building VxWorks for the
# Motorola Voyager Application Development Support Board with a PowerPC 8272
#
# INCLUDES
#     makeTarget
#*/

CPU              = PPC603
TOOL             = gnu

TGT_DIR = $(WIND_BASE)/target
include $(TGT_DIR)/h/make/defs.bsp

## Only redefine make definitions below this point, or your definitions will
## be overwritten by the makefile stubs above.

TARGET_DIR	= ads82xx
VENDOR		= Motorola
BOARD		= ads82xx

USR_ENTRY   = usrInit

RELEASE		+= bootrom_uncmp.hex

## The constants ROM_TEXT_ADRS, ROM_SIZE, and RAM_HIGH_ADRS are defined
## in config.h and Makefile. All definitions for these constants must be
## identical.

ROM_TEXT_ADRS    = 10000100 # ROM entry address
ROM_SIZE         = 01000000 # number of bytes of ROM space
ROM_WARM_ADRS    = 10000104 # ROM warm entry address 
ROM_LINK_ADRS	 = 10000000

RAM_LOW_ADRS     = 00010000 # RAM text/data address
RAM_HIGH_ADRS    = 01B00000 # RAM text/data address

# uncomment the following line to enable debugging
#ADDED_CFLAGS +=  -g -O
EXTRA_DEFINE   = -DVXWORKS
MACH_EXTRA      = 
EXTRA_MODULES   =

## Only redefine make definitions above this point, or the expansion of 
## makefile target dependencies may be incorrect.

include $(TGT_DIR)/h/make/rules.bsp

# Additional objects needed by romInit



