/* motFcc2End.c - Second Generation Motorola FCC Ethernet network interface.*/

/*
 * Copyright (c) 2003-2005 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01q,28aug05,dlk  Add section tags. Fix typo breaking polled mode (SPR#110883).
                 Fix off-by-one error in transmit cleanup-before-coalesce code.
		 Decrease phyMaxDelay from 20 to 5 seconds.
		 Fix errors in disabled INCLUDE_DUMPS code.
01p,29aug05,wap  Add support for IFF_ALLMULTI (SPR #110201)
01o,26jun05,dlk  Replace netJobAdd() with jobQueuePost().
01n,19mar05,dlk  Optimizations. Clean-up. Enable SNOOP through load string.
01m,28oct04,dtr  SPR 103066.
01l,31aug04,mdo  Documentation fixes for apigen
01k,19aug04,mdo  SPR's 99632, 98962, removed windview windview from code - left
                 event defines if needed.  Removed Graceful TX/Stop. Added dump
                 routines for debug. 
01j,23aug04,rcs  fixed SPR 100245
01i,23jul04,mdo  Base6 Enhancements
01p,09jul04,rcs  Fixed SPR 88900 motFcc2End driver does not handle netJobAdd() 
                 failure
01o,09jul04,rcs  Fixed SPR 98957 dereference of pDrvCtrl precedes check 
01n,09jul04,rcs  Added muxError call when netTupleGet fails SPR 88898
01m,09jul04,rcs  Implemented clean unload SPRs 98962, 98957, 98523, 89166, 
                 78575, 
01l,09jul04,rcs  Implemented Generic MIB Interface SPRs 96969, 72266, 70728, 
                 73830, 67057, 27788, 20618   
01k,09jul04,rcs  Implemented netPoolCreate SPR 67057
01j,08jul04,rcs  Fixed SPR 98818 removed motFccReplenish call from ISR
01i,08jul04,rcs  Added recommended enhancements SPR 90135
01h,15jun04,mdo  SPR 96809
01g,09jun04,mdo  SPR fix 97909
01f,04jun04,mil  Changed cacheArchXXX funcs to cacheXXX funcs.
01e,30mar04,mdo  SPR's 81336, 93209, 94561, 94562 and general cleanup.
01d,02dec03,rcs  fixed warnings for base6
01c,20aug03,gjc  Fixed SPR #90352, fixed compiler warnings and removed logMsg
01b,13aug03,gjc  Fixed SPRs #89689, #89649, #87812, #87749, #90135, #86434 
01a,14jan03,gjc  SPR#85164 Second Generation Motorola FCC END Driver.
*/

/*
DESCRIPTION
This module implements a Motorola Fast Communication Controller (FCC)
Ethernet network interface driver. This is a second generation driver that is
based on the original motFccEnd.c. It differs from the original in
initialization, performance, features and SPR fixes.

The driver "load string" interface differs from its predecessor. A parameter
that contains a pointer to a predefined array of function pointers was added
to the end of the load string. This array replaces multiple individual function
pointers for dual ported RAM allocation, MII access, duplex control, and
heartbeat and disconnect functionality; it is described more fully below.
The array simplifies updating the driver and BSP code independently.

Performance of the driver has been greatly enhanced. A layer of unnecessary
queuing was removed. Time-critical functions were re-written to be more fluid
and efficient. The driver's work load is distributed between the interrupt and
the net job queue. Only one netJobAdd() call is made per interrupt.
Multiple events pending are sent as a single net job.

A new Generic MIB interface has been implemented.

Several SPRs, written against the original motFccEnd driver and previous motFcc2End
versions, are fixed.

The FCC supports several communication protocols. This driver supports the FCC
operating in Ethernet mode, which is fully compliant with the IEEE 802.3u
10Base-T and 100Base-T specifications.

The FCC establishes a shared memory communication system with the CPU,
which may be divided into three parts: a set of Control/Status Registers (CSR)
and FCC-specific parameters, the buffer descriptors (BD), and the data buffers.

Both the CSRs and the internal parameters reside in the MPC8260's internal
RAM. They are used for mode control, and to extract status information
of a global nature. For instance, by programming these registers, the driver
can specify which FCC events should generate an interrupt, whether
features like the promiscuous mode or the heartbeat are enabled, and so on.
Pointers to both the Transmit Buffer Descriptors ring (TBD) and the
Receive Buffer Descriptors ring (RBD) are stored in the internal parameter
RAM. The latter also includes protocol-specific parameters, such as the
individual physical address of the station and the maximum receive frame
length.

The BDs are used to pass data buffers and related buffer information
between the hardware and the software. They may reside either on the 60x
bus, or on the CPM local bus. They include local status information, and a
pointer to the receive or transmit data buffers. These buffers are located in
external memory, and may reside on the 60x bus, or the CPM local bus (see
below).

This driver is designed to be moderately generic. Without modification, it can
operate across all the FCCs in the MPC8260, regardless of where the internal
memory base address is located. To achieve this goal, this driver must be
given several target-specific parameters and some external support routines.
These parameters, and the mechanisms used to communicate them to the driver, 
are detailed below.

BOARD LAYOUT
This device is on-board.  No jumper diagram is necessary.

EXTERNAL INTERFACE

The driver provides the standard external interface, motFccEnd2Load(), which
takes a string of parameters delineated by colons. The parameters should be
specified in hexadecimal, optionally preceded by "0x" or a minus sign "-".

The parameter string is parsed using strtok_r() and each parameter is
converted from a string representation to binary by a call to
strtoul(parameter, NULL, 16).

The format of the parameter string is:

"<unit>:<immrVal>:<fccNum>:<bdBase>:<bdSize>:<bufBase>:<bufSize>:<fifoTxBase>:
<fifoRxBase> :<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<phyAnOrderTbl>:
<userFlags>:<function table>(:<maxRxFrames>)"

TARGET-SPECIFIC PARAMETERS

\is
\i <unit>
This driver is written to support multiple individual device units.
This parameter is used to explicitly state which unit is being used.
Default is unit 0.
    
\i <immrVal>
Indicates the address at which the host processor presents its internal
memory (also known as the internal RAM base address). With this address,
and the <fccNum> value (see below), the driver is able to compute the
location of the FCC parameter RAM, and, ultimately, to program the FCC
for proper operation.

\i <fccNum>
This driver is written to support multiple individual device units.
This parameter is used to explicitly state which FCC is being used.

\i <bdBase>
The Motorola Fast Communication Controller is a DMA-type device and typically
shares access to some region of memory with the CPU. This driver is designed
for systems that directly share memory between the CPU and the FCC.

This parameter tells the driver that space for both the TBDs and the
RBDs need not be allocated, but should be taken from a cache-coherent
private memory space provided by the user at the given address. TBDs and RBDs
are both 8 bytes each, and individual descriptors must be 8-byte
aligned. The driver requires that an additional 8 bytes be provided for
alignment, even if <bdBase> is aligned to begin with.

If this parameter is "NONE", space for buffer descriptors is obtained
by calling cacheDmaMalloc() in motFccInitMem().

\i <bdSize>
The <bdSize> parameter specifies the size of the pre-allocated memory
region for the BDs. If <bdBase> is specified as NONE (-1), the driver ignores
this parameter. Otherwise, the driver checks that the size of the provided
memory region is adequate with respect to the given number of Transmit Buffer
Descriptors and Receive Buffer Descriptors (plus an additional 8 bytes for
alignment).

\i <bufBase>
This parameter tells the driver that space for data buffers
need not be allocated but should be taken from a cache-coherent
private memory space provided at the given address. The memory used for buffers
must be 32-byte aligned and non-cacheable. The FCC poses one more constraint, in
that DMA cycles may occur even when all the incoming data have already been
transferred to memory. This means at most 32 bytes of memory at the end of
each receive data buffer may be overwritten during reception. The driver
pads that area out, thus consuming some additional memory.

If this parameter is "NONE", space for buffer descriptors is obtained
by calling memalign() in motFccInitMem().

\i <bufSize>
The <bufSize> parameter specifies the size of the pre-allocated memory
region for data buffers. If <bufBase> is specified as NONE (-1), the driver
ignores this parameter. Otherwise, the driver checks that the size of the provided
memory region is adequate with respect to the given number of Receive Buffer
Descriptors and a non-configurable number of transmit buffers
 (MOT_FCC_TX_CL_NUM).  All the above should fit in the given memory space.
This area should also include room for buffer management structures.

\i <fifoTxBase>
Indicate the base location of the transmit FIFO, in internal memory.
The user does not need to initialize this parameter. The default
value (see MOT_FCC_FIFO_TX_BASE) is highly optimized for best performance.
However, if the user wishes to reserve that very area in internal RAM for
other purposes, this parameter may be set to a different value.

If <fifoTxBase> is specified as NONE (-1), the driver uses the default
value.

\i <fifoRxBase>
Indicate the base location of the receive FIFO, in internal memory.
The user does not need to initialize this parameter. The default
value (see MOT_FCC_FIFO_RX_BASE) is highly optimized for best performance.
However, if the user wishes to reserve that very area in internal RAM for
other purposes, this parameter may be set to a different value.

If <fifoRxBase> is specified as NONE (-1), the driver uses the default
value.

\i <tbdNum>
This parameter specifies the number of transmit buffer descriptors (TBDs).
Each buffer descriptor resides in 8 bytes of the processor's external
RAM space. If this parameter is less than a minimum number specified in
MOT_FCC_TBD_MIN, or if it is "NULL", a default value of MOT_FCC_TBD_DEF_NUM
is used. This parameter should always be an even number since each packet the 
driver sends may consume more than a single TBD.

\i <rbdNum>
This parameter specifies the number of receive buffer descriptors (RBDs).
Each buffer descriptor resides in 8 bytes of the processor's external
RAM space, and each one points to a buffer in external RAM. If this parameter 
is less than a minimum number specified in MOT_FCC_RBD_MIN, or if it is "NULL", 
a default value of MOT_FCC_RBD_DEF_NUM is used. This parameter should always be 
an even number.
                 
\i <phyAddr>
This parameter specifies the logical address of a MII-compliant physical
device (PHY) that is to be used as a physical media on the network.
Valid addresses are in the range 0-31. There may be more than one device
under the control of the same management interface. The default physical
layer initialization routine scans the whole range of PHY devices
starting from the one in <phyAddr>. If this parameter is
"MII_PHY_NULL", the default physical layer initialization routine finds
out the PHY actual address by scanning the whole range. The one with the lowest
address is chosen.

\i <phyDefMode>
This parameter specifies the operating mode that is set up
by the default physical layer initialization routine in case all
the attempts made to establish a valid link failed. If that happens,
the first PHY that matches the specified abilities is chosen to
work in that mode, and the physical link is not tested.

\i <phyAnOrderTbl>
This parameter may be set to the address of a table that specifies the
order how different subsets of technology abilities, if enabled, should be 
advertised by the auto-negotiation process. Unless the flag <MOT_FCC_USR_PHY_TBL>
is set in the userFlags field of the load string, the driver ignores this
parameter.

The user does not normally need to specify this parameter, since the default
behaviour enables auto-negotiation process as described in IEEE 802.3u.

\i <userFlags>
This field enables the user to give some degree of customization to the
driver.

MOT_FCC_USR_PHY_NO_AN: The default physical layer initialization
routine exploits the auto-negotiation mechanism as described in
the IEEE Std 802.3u, to bring a valid link up. According to it, all
the link partners on the media take part in the negotiation
process, and the highest priority common denominator technology ability
is chosen. To prevent auto-negotiation from occurring, set this bit in
the user flags.

MOT_FCC_USR_PHY_TBL: In the auto-negotiation process, PHYs
advertise all their technology abilities at the same time,
and the result is that the maximum common denominator is used. However,
this behaviour may be changed, and the user may affect the order how
each subset of PHY's abilities is negotiated. Hence, when the
MOT_FCC_USR_PHY_TBL bit is set, the default physical layer
initialization routine looks at the motFccAnOrderTbl[] table and
auto-negotiate a subset of abilities at a time, as suggested by the
table itself. It is worth noticing here, however, that if the
MOT_FCC_USR_PHY_NO_AN bit is on, the above table is ignored.

MOT_FCC_USR_PHY_NO_FD: The PHY may be set to operate in full duplex mode,
provided it has this ability, as a result of the negotiation with other
link partners. However, in this operating mode, the FCC ignores the
collision detect and carrier sense signals. To prevent negotiating full
duplex mode, set the MOT_FCC_USR_PHY_NO_FD bit in the user flags.

MOT_FCC_USR_PHY_NO_HD: The PHY may be set to operate in half duplex mode,
provided it has this ability, as a result of the negotiation with other link
partners. To prevent negotiating half duplex mode, set the
MOT_FCC_USR_PHY_NO_HD bit in the user flags.

MOT_FCC_USR_PHY_NO_100: The PHY may be set to operate at 100Mbit/s speed,
provided it has this ability, as a result of the negotiation with
other link partners. To prevent negotiating 100Mbit/s speed,
set the MOT_FCC_USR_PHY_NO_100 bit in the user flags.

MOT_FCC_USR_PHY_NO_10: The PHY may be set to operate at 10Mbit/s speed,
provided it has this ability, as a result of the negotiation with
other link partners. To prevent negotiating 10Mbit/s speed,
set the MOT_FCC_USR_PHY_NO_10 bit in the user flags.

MOT_FCC_USR_PHY_ISO: Some boards may have different PHYs controlled by the
same management interface. In some cases, it may be necessary to
electrically isolate some of them from the interface itself, in order
to guarantee a proper behaviour on the medium layer. If the user wishes to
electrically isolate all PHYs from the MII interface, the MOT_FCC_USR_PHY_ISO 
bit should be set. The default behaviour is to not isolate any PHY on the board.

MOT_FCC_USR_LOOP: When the MOT_FCC_USR_LOOP bit is set, the driver
configures the FCC to work in internal loopback mode, with the TX signal
directly connected to the RX. This mode should only be used for testing.

MOT_FCC_USR_RMON: When the MOT_FCC_USR_RMON bit is set, the driver 
configures the FCC to work in RMON mode, thus collecting network statistics
required for RMON support without the need to receive all packets as in
promiscuous mode.

MOT_FCC_USR_BUF_LBUS: When the MOT_FCC_USR_BUF_LBUS bit is set, the driver
configures the FCC to work as though the data buffers were located in the
CPM local bus.

MOT_FCC_USR_BD_LBUS: When the MOT_FCC_USR_BD_LBUS bit is set, the driver 
configures the FCC to work as though the buffer descriptors were located in the
CPM local bus.

MOT_FCC_USR_HBC: If the MOT_FCC_USR_HBC bit is set, the driver 
configures the FCC to perform heartbeat check following end of transmission
and the HB bit in the status field of the TBD is set if the collision
input does not assert within the heartbeat window. The user does not normally
need to set this bit.

\i <Function>
This is a pointer to the structure FCC_END_FUNCS. The structure contains mostly
FUNCPTRs that are used as a communication mechanism between the driver and the
BSP. If the pointer contains a NULL value, the driver uses system default
functions for the m82xxDpram DPRAM allocation and, obviously, the driver 
does not support BSP function calls for heartbeat errors, disconnect errors, and
PHY status changes that are hardware specific.

\cs
    FUNCPTR miiPhyInit; BSP Mii/Phy Init Function
    This function pointer is initialized by the BSP and called by the driver to
    initialize the MII driver. The driver sets up it's PHY settings and then
    calls this routine. The BSP is responsible for setting BSP specific PHY
    parameters and then calling the miiPhyInit. The BSP is responsible to set
    up any call to an interrupt. See miiPhyInt below.

\ce
\cs
    FUNCPTR miiPhyInt; Driver Function for BSP to Call on a Phy Status Change
    This function pointer is initialized by the driver and called by the BSP.
    The BSP calls this function when it handles a hardware MII specific
    interrupt. The driver initializes this to the function motFccPhyLSCInt.
    The BSP may or may not choose to call this function. It will depend if the
    BSP supports an interrupt driven PHY. The BSP can also set up the miiLib
    driver to poll. In this case the miiPhy driver calls this function. See
    miiLib for details.
    Note: Not calling this function when the PHY duplex mode changes
    results in a duplex mis-match. This causes TX errors in the driver
    and a reduction in throughput.
\ce
\cs
    FUNCPTR miiPhyBitRead; MII Bit Read Function
    This function pointer is initialized by the BSP and called by the driver.
    The driver calls this function when it needs to read a bit from the MII
    interface. The MII interface is hardware specific.
\ce
\cs
    FUNCPTR miiPhyBitWrite; MII Bit Write Function
    This function pointer is initialized by the BSP and called by the driver.
    The driver calls this function when it needs to write a bit to the MII
    interface. This MII interface is hardware specific.
\ce
\cs
    FUNCPTR miiPhyDuplex; Duplex Status Call Back
    This function pointer is initialized by the BSP and called by the driver.
    The driver calls this function to obtain the status of the duplex
    setting in the PHY.
\ce
\cs
    FUNCPTR miiPhySpeed; Speed Status Call Back
    This function pointer is initialized by the BSP and called by the driver.
    The driver calls this function to obtain the status of the speed
    setting in the PHY. This interface is hardware specific.
\ce
\cs
    FUNCPTR hbFail; HeartBeat Fail Indicator
    This function pointer is initialized by the BSP and called by the driver.
    The driver calls this function to indicate an FCC heartbeat error.
\ce
\cs
    FUNCPTR intDisc; Disconnect Function
    This function pointer is initialized by the BSP and called by the driver.
    The driver calls this function to indicate an FCC disconnect error.
\ce
\cs
    FUNCPTR dpramFree; DPRAM Free routine
    This function pointer is initialized by the BSP and called by the driver.
    The BSP allocates memory for the BDs from this pool. The Driver must
    free the BD area using this function.
\ce
\cs
    FUNCPTR dpramFccMalloc; DPRAM FCC Malloc routine
    This function pointer is initialized by the BSP and called by the driver.
    The Driver allocates memory from the FCC specific POOL using this function.
\ce
\cs
    FUNCPTR dpramFccFree; DPRAM FCC Free routine
    This function pointer is initialized by the BSP and called by the driver.
    The Driver frees memory from the FCC specific POOL using this function.
\ce

\i <maxRxFrames>
The <maxRxFrames> parameter is optional.  It limits the number 
of frames the receive handler services in one pass. It is intended to 
prevent the tNetTask from monopolizing the CPU and starving
applications. The default value is nRFDs * 2.

\ie

EXTERNAL SUPPORT REQUIREMENTS
This driver requires several external support functions.

\is
\i sysFccEnetEnable()
\cs
    STATUS sysFccEnetEnable (UINT32 immrVal, UINT8 fccNum);
\ce
This routine is expected to handle any target-specific functions needed
to enable the FCC. These functions typically include setting the Port B and C
on the MPC8260 so that the MII interface may be used. This routine is
expected to return OK on success, or ERROR. The driver calls this routine,
once per device, from the motFccStart () routine.
\i sysFccEnetDisable()
\cs
    STATUS sysFccEnetDisable (UINT32 immrVal, UINT8 fccNum);
\ce
This routine is expected to perform any target specific functions required
to disable the MII interface to the FCC.  This involves restoring the
default values for all the Port B and C signals. This routine is expected to
return OK on success, or ERROR. The driver calls this routine from the
motFccStop() routine each time a device is disabled.
\i sysFccEnetAddrGet()
\cs
    STATUS sysFccEnetAddrGet (int unit,UCHAR *address);
\ce
The driver expects this routine to provide the six-byte Ethernet hardware
address that is used by this device.  This routine must copy the six-byte
address to the space provided by <enetAddr>.  This routine is expected to
return OK on success, or ERROR.  The driver calls this routine, once per
device, from the motFccEndLoad() routine.
\cs
    STATUS sysFccMiiBitWr (UINT32 immrVal, UINT8 fccNum, INT32 bitVal);
\ce
This routine is expected to perform any target specific functions required
to write a single bit value to the MII management interface of a MII-compliant
PHY device. The MII management interface is made up of two lines: management
data clock (MDC) and management data input/output (MDIO). The former provides
the timing reference for transfer of information on the MDIO signal.
The latter is used to transfer control and status information between the
PHY and the FCC. For this transfer to be successful, the information itself
has to be encoded into a frame format, and both the MDIO and MDC signals have
to comply with certain requirements as described in the 802.3u IEEE Standard.
There is not built-in support in the FCC for the MII management interface.
This means that the clocking on the MDC line and the framing of the information
on the MDIO signal have to be done in software. Hence, this routine is
expected to write the value in <bitVal> to the MDIO line while properly
sourcing the MDC clock to a PHY, for one bit time.
\cs
    STATUS sysFccMiiBitRd (UINT32 immrVal, UINT8 fccNum, INT8 * bitVal);
\ce
This routine is expected to perform any target specific functions required
to read a single bit value from the MII management interface of a MII-compliant
PHY device. The MII management interface is made up of two lines: management
data clock (MDC) and management data input/output (MDIO). The former provides
the timing reference for transfer of information on the MDIO signal.
The latter is used to transfer control and status information between the
PHY and the FCC. For this transfer to be successful, the information itself
has to be encoded into a frame format, and both the MDIO and MDC signals have
to comply with certain requirements as described in the 802.3u IEEE Standard.
There is not built-in support in the FCC for the MII management interface.
This means that the clocking on the MDC line and the framing of the information
on the MDIO signal have to be done in software. Hence, this routine is
expected to read the value from the MDIO line in <bitVal>, while properly
sourcing the MDC clock to a PHY, for one bit time.
\ie


SYSTEM RESOURCE USAGE
If the driver allocates the memory for the BDs to share with the FCC,
it does so by calling the cacheDmaMalloc() routine. If this region is provided
by the user, it must be from non-cacheable memory. 

This driver can operate only if this memory region is non-cacheable
or if the hardware implements bus snooping.  The driver cannot maintain
cache coherency for the device because the BDs are asynchronously
modified by both the driver and the device, and these fields share
the same cache line.

If the driver allocates the memory for the data buffers to share with the FCC,
it does so by calling the memalign () routine.  The driver does not need to
use cache-safe memory for data buffers, since the host CPU and the device are
not allowed to modify buffers asynchronously. The related cache lines are
flushed or invalidated as appropriate.

TUNING HINTS

The only adjustable parameters are the number of TBDs and RBDs that are
created at run-time.  These parameters are given to the driver when
motFccEndLoad() is called.  There is one RBD associated with each received
frame, whereas a single transmit packet frequently uses more than one TBD.
For memory-limited applications, decreasing the number of RBDs may be
desirable.  Decreasing the number of TBDs below a certain point results
in substantial performance degradation, and is not recommended. Increasing
the number of buffer descriptors can boost performance.


SPECIAL CONSIDERATIONS

SEE ALSO: ifLib,
\tb MPC8260 Fast Ethernet Controller (Supplement to the MPC860 User's Manual) 
\tb Motorola MPC860 User's Manual ,

\INTERNAL
This driver contains conditional compilation switch MOT_FCC_DBG.
If defined, adds debug output routines.  Output is further
selectable at run-time via the motFccEndDbg global variable.
*/

#include "vxWorks.h"
#include "wdLib.h"
#include "iv.h"
#include "vme.h"
#include "net/mbuf.h"
#include "net/unixLib.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "errno.h"
#include "memLib.h"
#include "intLib.h"
#include "net/route.h"
#include "iosLib.h"
#include "errnoLib.h"
#include "vxLib.h"

#include "cacheLib.h"
#include "logLib.h"
#include "netLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "sysLib.h"
#include "taskLib.h"

#include "net/systm.h"
#include "net/if_subr.h"

#undef ETHER_MAP_IP_MULTICAST
#include "etherMultiLib.h"
#include "end.h"

#define    END_MACROS
#include "endLib.h"
#include "miiLib.h"

#include "lstLib.h"
#include "semLib.h"
#include "sys/times.h"

#include "net/unixLib.h"
#include "net/if_subr.h"

#ifdef WR_IPV6
#include "adv_net.h"
#endif /* WR_IPV6 */

#include "drv/mem/m82xxDpramLib.h"
#include "drv/intrCtl/m8260IntrCtl.h"
#include "drv/end/m8260Fcc.h"
#include "drv/sio/m8260Cp.h"
#include "drv/sio/m8260CpmMux.h"

#ifndef _WRS_FASTTEXT
#define _WRS_FASTTEXT
#endif

#define _WRS_VXWORKS_VNUM \
    ((_WRS_VXWORKS_MAJOR << 16)|(_WRS_VXWORKS_MINOR << 8)|(_WRS_VXWORKS_MAINT))

#if _WRS_VXWORKS_VNUM >= 0x060100

#undef netJobAdd	/* for the cases we store it in a pointer */

#define NET_JOB_ADD(f, a1, a2, a3, a4, a5)			    \
    (jobQueueStdPost (&netJobInfo, NET_TASK_QJOB_PRI,		    \
		      (VOIDFUNCPTR)(f), (void *)(a1), (void *)(a2), \
		      (void *)(a3), (void *)(a4), (void *)(a5)))

#else
#define NET_JOB_ADD(f, a1, a2, a3, a4, a5)			    \
    netJobAdd ((f), (a1), (a2), (a3), (a4), (a5))
#endif

#undef MOT_FCC_DBG
#undef MOT_FCC_STAT_MONITOR

/* define if CPM-21 errata applies */
#define MOT_FCC_CPM_21_ERRATA

#ifndef MOTFCC2END_HEADER
#define MOTFCC2END_HEADER "drv/end/motFcc2End.h"
#endif

#include MOTFCC2END_HEADER

#ifdef INCLUDE_WINDVIEW
#undef INCLUDE_WINDVIEW
#endif

#undef INCLUDE_WINDVIEW

#ifdef  INCLUDE_WINDVIEW
/* WindView Event numbers */
#define WV_INT_ENTRY(b,l)            wvEvent(1000,b,l)
#define WV_INT_EXIT(b,l)             wvEvent(1001,b,l)
#define WV_INT_RXB_ENTRY(b,l)        wvEvent(1300,b,l)
#define WV_INT_RXF_ENTRY(b,l)        wvEvent(1310,b,l)
#define WV_INT_BSY_ENTRY(b,l)        wvEvent(1320,b,l)
#define WV_INT_BSY_EXIT(b,l)         wvEvent(1321,b,l)
#define WV_INT_RX_EXIT(b,l)          wvEvent(1301,b,l)
#define WV_INT_RXC_ENTRY(b,l)        wvEvent(1400,b,l)
#define WV_INT_RXC_EXIT(b,l)         wvEvent(1401,b,l)
#define WV_INT_TXC_ENTRY(b,l)        wvEvent(1500,b,l)
#define WV_INT_TXC_EXIT(b,l)         wvEvent(1501,b,l)
#define WV_INT_TXB_ENTRY(b,l)        wvEvent(1600,b,l)
#define WV_INT_TXB_EXIT(b,l)         wvEvent(1601,b,l)
#define WV_INT_TXE_ENTRY(b,l)        wvEvent(1610,b,l)
#define WV_INT_TXE_EXIT(b,l)         wvEvent(1611,b,l)
#define WV_INT_NETJOBADD_ENTRY(b,l)  wvEvent(1800,b,l)
#define WV_INT_NETJOBADD_EXIT(b,l)   wvEvent(1801,b,l)
#define WV_HANDLER_ENTRY(b,l)        wvEvent(2000,b,l)
#define WV_HANDLER_EXIT(b,l)         wvEvent(2001,b,l)
#define WV_MUX_TX_RESTART_ENTRY(b,l) wvEvent(2100,b,l)
#define WV_MUX_TX_RESTART_EXIT(b,l)  wvEvent(2101,b,l)
#define WV_MUX_ERROR_ENTRY(b,l)      wvEvent(2200,b,l)
#define WV_MUX_ERROR_EXIT(b,l)       wvEvent(2201,b,l)
#define WV_SEND_ENTRY(b,l)           wvEvent(5000,b,l)
#define WV_SEND_EXIT(b,l)            wvEvent(5001,b,l)
#define WV_RECV_ENTRY(b,l)           wvEvent(6000,b,l)
#define WV_RECV_EXIT(b,l)            wvEvent(6001,b,l)
#define WV_CACHE_FLUSH_ENTRY(b,l)    wvEvent(8000,b,l)
#define WV_CACHE_FLUSH_EXIT(b,l)     wvEvent(8001,b,l)
#define WV_CACHE_INVAL_ENTRY(b,l)    wvEvent(8100,b,l)
#define WV_CACHE_INVAL_EXIT(b,l)     wvEvent(8101,b,l)
#define WV_CACHE_DEBUG_ENTRY(b,l)    wvEvent(9999,b,l)
#endif

/* defines */

/*
 * Shifted buffer descriptor status bits. These values are
 * shifted 16-bits left for use in the combined bdStatLen field,
 * of which the high-order half-word contains the status. We
 * assume a big endian host (almost certainly PPC) throughout
 * this driver.
 */

#define M8260_FETH_TBDS_R	((UINT32)(M8260_FETH_TBD_R << 16))
#define M8260_FETH_TBDS_PAD	(M8260_FETH_TBD_PAD << 16)
#define M8260_FETH_TBDS_W	(M8260_FETH_TBD_W << 16)
#define M8260_FETH_TBDS_I	(M8260_FETH_TBD_I << 16)
#define M8260_FETH_TBDS_L	(M8260_FETH_TBD_L << 16)
#define M8260_FETH_TBDS_TC	(M8260_FETH_TBD_TC << 16)
#define M8260_FETH_TBDS_DEF	(M8260_FETH_TBD_DEF << 16)
#define M8260_FETH_TBDS_HB	(M8260_FETH_TBD_HB << 16)
#define M8260_FETH_TBDS_LC	(M8260_FETH_TBD_LC << 16)
#define M8260_FETH_TBDS_RL	(M8260_FETH_TBD_RL << 16)
#define M8260_FETH_TBDS_RC	(M8260_FETH_TBD_RC << 16)
#define M8260_FETH_TBDS_UN	(M8260_FETH_TBD_UN << 16)
#define M8260_FETH_TBDS_CSL	(M8260_FETH_TBD_CSL << 16)

#define M8260_FETH_RBDS_E	((UINT32)(M8260_FETH_RBD_E << 16))
#define M8260_FETH_RBDS_W	(M8260_FETH_RBD_W << 16)
#define M8260_FETH_RBDS_I	(M8260_FETH_RBD_I << 16)
#define M8260_FETH_RBDS_L	(M8260_FETH_RBD_L << 16)
#define M8260_FETH_RBDS_F	(M8260_FETH_RBD_F << 16)
#define M8260_FETH_RBDS_M	(M8260_FETH_RBD_M << 16)
#define M8260_FETH_RBDS_BC	(M8260_FETH_RBD_BC << 16)
#define M8260_FETH_RBDS_MC	(M8260_FETH_RBD_MC << 16)
#define M8260_FETH_RBDS_LG	(M8260_FETH_RBD_LG << 16)
#define M8260_FETH_RBDS_NO	(M8260_FETH_RBD_NO << 16)
#define M8260_FETH_RBDS_SH	(M8260_FETH_RBD_SH << 16)
#define M8260_FETH_RBDS_CR	(M8260_FETH_RBD_CR << 16)
#define M8260_FETH_RBDS_OV	(M8260_FETH_RBD_OV << 16)
#define M8260_FETH_RBDS_CL	(M8260_FETH_RBD_CL << 16)

#define MOT_FCC_RBDS_ERR	(MOT_FCC_RBD_ERR << 16)

/* Cache and virtual/physical memory related macros */

#define MOT_FCC_CACHE_INVAL(address, len) { \
        CACHE_DRV_INVALIDATE (&pDrvCtrl->bufCacheFuncs, (address), (len));  \
    }

#define MOT_FCC_CACHE_FLUSH(address, len) { \
        CACHE_DRV_FLUSH (&pDrvCtrl->bufCacheFuncs, (address), (len)); \
    }


/* Flag Macros */

#define MOT_FCC_FLAG_CLEAR(clearBits)       \
    (pDrvCtrl->flags &= ~(clearBits))

#define MOT_FCC_FLAG_SET(setBits)           \
    (pDrvCtrl->flags |= (setBits))

#define MOT_FCC_FLAG_GET()                  \
    (pDrvCtrl->flags)

#define MOT_FCC_FLAG_ISSET(setBits)         \
    (pDrvCtrl->flags & (setBits))

#define MOT_FCC_USR_FLAG_ISSET(setBits)     \
    (pDrvCtrl->userFlags & (setBits))

#define END_FLAGS_ISSET(setBits)             \
    ((&pDrvCtrl->endObj)->flags & (setBits))

/* some BDs definitions */

/*
 * Since we now use linkBufPool, there is no cluster pool pointer overhead.
 * MII_ETH_MAX_PCK_SZ is 1518. linkBufPool imposes a cluster alignment of
 * NETBUF_ALIGN (64 bytes), however the fcc only requires MOT_FCC_BUF_ALIGN
 * (32 bytes): the RX buffer descriptor start address must be a multiple
 * of 32, and the MRBLR, maximum receive buffer length register in the
 * parameter RAM, must also be a multiple of 32.
 */
#define XXX_FCC_MAX_CL_LEN  (MII_ETH_MAX_PCK_SZ)

#define MOT_FCC_MAX_CL_LEN     ROUND_UP (XXX_FCC_MAX_CL_LEN, MOT_FCC_BUF_ALIGN)

#define MOT_FCC_RX_CL_SZ       (MOT_FCC_MAX_CL_LEN)
#define MOT_FCC_TX_CL_SZ       (MOT_FCC_MAX_CL_LEN)

/* read/write macros to access internal memory */

#define MOT_FCC_REG_LONG_WR(regAddr, regVal)                \
    MOT_FCC_LONG_WR ((UINT32 *) (regAddr), (regVal));

#define MOT_FCC_REG_LONG_RD(regAddr, regVal)                \
    MOT_FCC_LONG_RD ((UINT32 *) (regAddr), (regVal));

#define MOT_FCC_REG_WORD_WR(regAddr, regVal)                \
    MOT_FCC_WORD_WR ((UINT16 *) (regAddr), (regVal));

#define MOT_FCC_REG_WORD_RD(regAddr, regVal)                \
    MOT_FCC_WORD_RD ((UINT16 *) (regAddr), (regVal));

#define END_HADDR(pEnd)                                     \
        ((pEnd).mib2Tbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd)                                 \
        ((pEnd).mib2Tbl.ifPhysAddress.addrLength)

/* locals */

/* Function declarations not in any header files */

/* forward function declarations */

LOCAL STATUS    motFccInitParse (DRV_CTRL * pDrvCtrl, char *initString);
LOCAL STATUS    motFccInitMem (DRV_CTRL *pDrvCtrl);
_WRS_FASTTEXT
LOCAL STATUS    motFccSend (DRV_CTRL *pDrvCtrl, M_BLK *pMblk);
LOCAL STATUS    motFccPhyPreInit (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccRbdInit (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccTbdInit (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccFpsmrValSet (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccHashTblPopulate (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccHashTblAdd (DRV_CTRL * pDrvCtrl, UCHAR * pAddr);
LOCAL STATUS    motFccIramInit (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccPramInit (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccCpcrCommand (DRV_CTRL * pDrvCtrl, UINT8 command);
_WRS_FASTTEXT
LOCAL int       motFccTbdClean (DRV_CTRL * pDrvCtrl);
_WRS_FASTTEXT
LOCAL void      motFccInt (DRV_CTRL * pDrvCtrl);
_WRS_FASTTEXT
LOCAL UINT32    motFccHandleRXFrames(DRV_CTRL *pDrvCtrl, int * pMaxRxFrames);
LOCAL STATUS    motFccMiiRead (DRV_CTRL * pDrvCtrl, UINT8 phyAddr,
                   UINT8 regAddr, UINT16 *retVal);
LOCAL STATUS    motFccMiiWrite (DRV_CTRL * pDrvCtrl, UINT8 phyAddr,
                UINT8 regAddr, UINT16 writeData);
LOCAL STATUS    motFccAddrSet (DRV_CTRL * pDrvCtrl, UCHAR * pAddr,
                   UINT32 offset);
LOCAL void      motFccPhyLSCInt (DRV_CTRL *pDrvCtrl);
LOCAL STATUS    motFccPktTransmit (DRV_CTRL *, M_BLK *);
LOCAL STATUS    motFccPktCopyTransmit (DRV_CTRL *, M_BLK *);
LOCAL void      motFccHandleLSCJob (DRV_CTRL *);
LOCAL STATUS    motFccTbdFree (DRV_CTRL *);
LOCAL STATUS    motFccRbdFree (DRV_CTRL *);

/* END Specific interfaces. */

END_OBJ *       motFccEndLoad (char *initString);
LOCAL STATUS    motFccStart (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccUnload (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccMemFree (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccStop (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccIoctl (DRV_CTRL * pDrvCtrl, int cmd, caddr_t data);
LOCAL STATUS    motFccMCastAddrAdd (DRV_CTRL * pDrvCtrl, UCHAR * pAddress);
LOCAL STATUS    motFccMCastAddrDel (DRV_CTRL * pDrvCtrl, UCHAR * pAddress);
LOCAL STATUS    motFccMCastAddrGet (DRV_CTRL * pDrvCtrl,
                                        MULTI_TABLE *pTable);
LOCAL STATUS    motFccPollSend (DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    motFccPollReceive (DRV_CTRL * pDrvCtrl, M_BLK_ID pMblk);
LOCAL STATUS    motFccPollStart (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccPollStop (DRV_CTRL * pDrvCtrl);

/* globals */


#ifdef MOT_FCC_DBG

#undef  MOT_FCC_STAT_MONITOR
#define MOT_FCC_STAT_MONITOR

void motFccIramShow (DRV_CTRL *);
void motFccPramShow (DRV_CTRL *);
void motFccEramShow (DRV_CTRL *);

void motFccDrvShow (DRV_CTRL *);
void motFccMiiShow (DRV_CTRL *);
void motFccMibShow (DRV_CTRL *);

#define MOT_FCC_DBG_OFF         0x0000
#define MOT_FCC_DBG_RX          0x0001
#define MOT_FCC_DBG_TX          0x0002
#define MOT_FCC_DBG_POLL        0x0004
#define MOT_FCC_DBG_MII         0x0008
#define MOT_FCC_DBG_LOAD        0x0010
#define MOT_FCC_DBG_IOCTL       0x0020
#define MOT_FCC_DBG_INT         0x0040
#define MOT_FCC_DBG_START       0x0080
#define MOT_FCC_DBG_INT_RX_ERR  0x0100
#define MOT_FCC_DBG_INT_TX_ERR  0x0200
#define MOT_FCC_DBG_TRACE       0x0400
#define MOT_FCC_DBG_TRACE_RX    0x0800
#define MOT_FCC_DBG_TRACE_TX    0x1000
#define MOT_FCC_DBG_RX_ERR      0x2000
#define MOT_FCC_DBG_TX_ERR      0x4000
#define MOT_FCC_DBG_MONITOR     0x8000

#define MOT_FCC_DBG_ANY         0xffff

#define MOT_FCC_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)	\
    {							\
    if (motFccEndDbg & FLG)				\
        logMsg (X0, (int)(X1), (int)(X2), (int)(X3),	\
		(int)(X4), (int)(X5), (int)(X6));	\
    }


FUNCPTR _func_netJobAdd;
FUNCPTR _func_txRestart;
FUNCPTR _func_error;

/* global debug level flag */
UINT32 motFccEndDbg = MOT_FCC_DBG_MONITOR;

DRV_CTRL *  pDrvCtrlDbg_fcc0 = NULL;
DRV_CTRL *  pDrvCtrlDbg_fcc1 = NULL;

#else /* MOT_FCC_DBG */
#define MOT_FCC_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)
#endif

#ifdef MOT_FCC_STAT_MONITOR
#define MOT_FCC_STAT_INCR(i) (i++)
#else
#define MOT_FCC_STAT_INCR(i)
#endif


#ifdef MOT_FCC_DBG
LOCAL const char    *speedStr[2] = {"10","100"};
LOCAL const char     *linkStr[2] = {"Down","Up"};
LOCAL const char   *duplexStr[2] = {"Half","Full"};
#endif

LOCAL const char *pIfDescrStr = "Motorola FCC Ethernet Enhanced Network Driver 2.1";
/*
 * Define the device function table.  This is static across all driver
 * instances.
 */

LOCAL NET_FUNCS netFccFuncs =
    {
    (FUNCPTR) motFccStart,          /* start func. */
    (FUNCPTR) motFccStop,           /* stop func. */
    (FUNCPTR) motFccUnload,         /* unload func. */
    (FUNCPTR) motFccIoctl,          /* ioctl func. */
    (FUNCPTR) motFccSend,           /* send func. */
    (FUNCPTR) motFccMCastAddrAdd,   /* multicast add func. */
    (FUNCPTR) motFccMCastAddrDel,   /* multicast delete func. */
    (FUNCPTR) motFccMCastAddrGet,   /* multicast get func. */
    (FUNCPTR) motFccPollSend,       /* polling send func. */
    (FUNCPTR) motFccPollReceive,    /* polling receive func. */
    endEtherAddressForm,            /* put address info into a NET_BUFFER */
    (FUNCPTR) endEtherPacketDataGet,/* get pointer to data in NET_BUFFER */
    (FUNCPTR) endEtherPacketAddrGet /* Get packet addresses */
    };



/*******************************************************************************
*
* motFccEndLoad - initialize the driver and device
*
* This routine initializes both driver and device to an operational state
* using device specific parameters specified by <initString>.
*
* The parameter string, <initString>, is an ordered list of parameters each
* separated by a colon. The format of <initString> is:
*
* "<unit><immrVal>:<fccNum>:<bdBase>:<bdSize>:<bufBase>:<bufSize>:<fifoTxBase>:
* <fifoRxBase>:<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<phyAnOrderTbl>:
* <userFlags>:<function table>(:<maxRxFrames>)"
*
* The FCC shares a region of memory with the driver.  The caller of this
* routine can specify the address of this memory region, or can specify that
* the driver must obtain this memory region from the system resources.
*
* A default number of transmit/receive buffer descriptors of 32 can be
* selected by passing zero in the parameters <tbdNum> and <rbdNum>.
* In other cases, the number of buffers selected should be greater than two.
*
* The <bufBase> parameter is used to inform the driver about the shared
* memory region.  If this parameter is set to the constant "NONE," then this
* routine attempts to allocate the shared memory from the system.  Any
* other value for this parameter is interpreted by this routine as the address
* of the shared memory region to be used. The <bufSize> parameter is used
* to check that this region is large enough with respect to the provided
* values of both transmit/receive buffer descriptors.
*
* If the caller provides the shared memory region, then the driver assumes
* that this region does not require cache coherency operations, nor does it
* require conversions between virtual and physical addresses.
*
* If the caller indicates that this routine must allocate the shared memory
* region, then this routine uses cacheDmaMalloc() to obtain
* some  cache-safe memory.  The attributes of this memory is checked,
* and if the memory is not write coherent, this routine aborts and
* returns NULL.
*
* RETURNS: an END object pointer, or NULL on error.
*
* SEE ALSO: ifLib,
* \tb MPC8260 PowerQUICC II User's Manual 
*/

END_OBJ* motFcc2EndLoad
    (
    char *initString
    )
    {
    DRV_CTRL * pDrvCtrl;		   /* pointer to DRV_CTRL structure */
    UCHAR      enetAddr[MOT_FCC_ADDR_LEN]; /* ethernet address */
    int        retVal;

    if (initString == NULL)
        return NULL;

    if (initString[0] == 0)
        {
        bcopy ((char *)MOT_FCC_DEV_NAME, (void *)initString,MOT_FCC_DEV_NAME_LEN);
        return NULL;
        }

    /* allocate the device structure */

    pDrvCtrl = (DRV_CTRL *) calloc (sizeof (DRV_CTRL), 1);
    if (pDrvCtrl == NULL)
        return NULL;

    /* get memory for the phyInfo structure */
    if ((pDrvCtrl->phyInfo = calloc (sizeof (PHY_INFO), 1)) == NULL)
        {
        /* Release DrvCtrl resources */
        free (pDrvCtrl);
        return NULL;
        }

    /* set up function pointers */
    pDrvCtrl->netJobAdd    = (FUNCPTR) netJobAdd;
    pDrvCtrl->muxTxRestart = (FUNCPTR) muxTxRestart;
    pDrvCtrl->muxError     = (FUNCPTR) muxError;

#ifdef MOT_FCC_DBG
    /* get memory for the drivers stats structure */
    if ((pDrvCtrl->Stats = calloc (sizeof (FCC_DRIVER_STATS), 1)) == NULL)
	{
	free (pDrvCtrl->phyInfo);
	free (pDrvCtrl);
	return NULL;
	}

    /* support unit test */
    _func_netJobAdd = (FUNCPTR) netJobAdd;
    _func_txRestart = (FUNCPTR) muxTxRestart;
    _func_error     = (FUNCPTR) muxError;
#endif /* MOT_FCC_DBG */


    /* Parse InitString */

    if (motFccInitParse (pDrvCtrl, initString) == ERROR)
        goto errorExit;

#ifdef MOT_FCC_DBG
    if(pDrvCtrl->unit == 0)
    	pDrvCtrlDbg_fcc0 = pDrvCtrl;
   else
	pDrvCtrlDbg_fcc1 = pDrvCtrl;
#endif
	/*
     * Sanity check the unit number. Note that MOT_FCC_MAX_DEVS
     * is a misleading name; it's actually the maximum unit number.
     * For now we leave it, rather than risk breaking multiple BSPs.
     */

    if (pDrvCtrl->unit < 0 || pDrvCtrl->unit > MOT_FCC_MAX_DEVS)
        goto errorExit;

    /* memory initialization */

    if (motFccInitMem (pDrvCtrl) == ERROR)
        goto errorExit;

    /* get our ethernet hardware address */
    SYS_FCC_ENET_ADDR_GET (enetAddr);

    /* init miiPhy functions */

    if ( pDrvCtrl->motFccFuncs != NULL )
        {
        pDrvCtrl->hbFailFunc    = pDrvCtrl->motFccFuncs->hbFail;
        pDrvCtrl->intDiscFunc   = pDrvCtrl->motFccFuncs->intDisc;
        pDrvCtrl->phyInitFunc   = pDrvCtrl->motFccFuncs->miiPhyInit;
        pDrvCtrl->phyDuplexFunc = pDrvCtrl->motFccFuncs->miiPhyDuplex;
        pDrvCtrl->phySpeedFunc  = pDrvCtrl->motFccFuncs->miiPhySpeed;

        /* BSP call back to driver */
        pDrvCtrl->motFccFuncs->miiPhyInt = (FUNCPTR) motFccPhyLSCInt;
        }
    else
        {
        pDrvCtrl->hbFailFunc    = NULL;
        pDrvCtrl->intDiscFunc   = NULL;
        pDrvCtrl->phyInitFunc   = NULL;
        pDrvCtrl->phyDuplexFunc = NULL;
        pDrvCtrl->phySpeedFunc  = NULL;
        }
#ifdef  CPM_DPRAM_POOL  //xiexy
    /* init dpram functions */
    if (pDrvCtrl->motFccFuncs->dpramFree == NULL)
        pDrvCtrl->dpramFreeFunc = (FUNCPTR) m82xxDpramFree;
    else
        pDrvCtrl->dpramFreeFunc = pDrvCtrl->motFccFuncs->dpramFree;

    if (pDrvCtrl->motFccFuncs->dpramFccMalloc == NULL)
        pDrvCtrl->dpramFccMallocFunc = (FUNCPTR) m82xxDpramFccMalloc;
    else
        pDrvCtrl->dpramFccMallocFunc = pDrvCtrl->motFccFuncs->dpramFccMalloc;

    if (pDrvCtrl->motFccFuncs->dpramFccFree == NULL)
        pDrvCtrl->dpramFccFreeFunc = (FUNCPTR) m82xxDpramFccFree;
    else
        pDrvCtrl->dpramFccFreeFunc = pDrvCtrl->motFccFuncs->dpramFccFree;
#endif

    /* initialize some flags */

    pDrvCtrl->intrConnect = FALSE;


    MOT_FCC_LOG (MOT_FCC_DBG_START,
                 "User Flags\n"
                 "\tZero Copy : TRUE\n"
                 "\tDPRAM ALLOC : %s\n"
                 "\tDATA on LOCAL BUS : %s\n"
                 "\tBDs  on LOCAL BUS : %s\n",
                 (pDrvCtrl->userFlags&MOT_FCC_USR_DPRAM_ALOC) ? "TRUE":"FALSE",
                 (pDrvCtrl->userFlags&MOT_FCC_USR_BUF_LBUS)   ? "TRUE":"FALSE",
                 (pDrvCtrl->userFlags&MOT_FCC_USR_BD_LBUS)    ? "TRUE":"FALSE",
                 4,5,6);

    MOT_FCC_LOG (MOT_FCC_DBG_START,
                 "Buffer Management\n"
                 "\tBuffer Base : 0x%08x\n"
                 "\tBuffer Size : 0x%08x\n"
                 "\tDescriptor Base : 0x%08x\n"
                 "\tDescriptor Size : 0x%08x\n",
                 (int)pDrvCtrl->pBufBase,
                 pDrvCtrl->bufSize,
                 (int)pDrvCtrl->pBdBase,
                 pDrvCtrl->bdSize,
                 5,6);


    /* store the internal ram base address */

    pDrvCtrl->fccIramAddr = (UINT32) M8260_FGMR1 (pDrvCtrl->immrVal) +
                            ((pDrvCtrl->fccNum - 1) * M8260_FCC_IRAM_GAP);

    pDrvCtrl->fccReg = (FCC_REG_T *)pDrvCtrl->fccIramAddr;

    /* store the parameter ram base address */

    pDrvCtrl->fccPramAddr = (UINT32) M8260_FCC1_BASE (pDrvCtrl->immrVal) +
                            ((pDrvCtrl->fccNum - 1) * M8260_FCC_DPRAM_GAP);

    pDrvCtrl->fccPar    = (FCC_PARAM_T *) pDrvCtrl->fccPramAddr;
    pDrvCtrl->fccEthPar = &pDrvCtrl->fccPar->prot.e;

    /* endObj initializations */
    if (END_OBJ_INIT (&pDrvCtrl->endObj, NULL,
                      MOT_FCC_DEV_NAME, pDrvCtrl->unit, &netFccFuncs,
                      (char *) pIfDescrStr)
        == ERROR)
        goto errorExit;

    pDrvCtrl->phyInfo->phySpeed = MOT_FCC_10MBS;

    if (endM2Init(&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd,
                  (u_char *) &enetAddr[0], MOT_FCC_ADDR_LEN, ETHERMTU, 
                  pDrvCtrl->phyInfo->phySpeed, 
                  IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST)
        == ERROR)
        goto errorExit;

    /* Allocate DPRAM memory for the riptr, tiptr & pad */
#ifdef  CPM_DPRAM_POOL  //xiexy
    pDrvCtrl->riPtr = (void *) pDrvCtrl->dpramFccMallocFunc (32,32);
    if (pDrvCtrl->riPtr == NULL)
        goto errorExit;

    pDrvCtrl->tiPtr = (void *) pDrvCtrl->dpramFccMallocFunc (32,32);
    if (pDrvCtrl->tiPtr == NULL)
        goto errorExit;

    pDrvCtrl->padPtr = (void *) pDrvCtrl->dpramFccMallocFunc (32,32);
    if (pDrvCtrl->padPtr == NULL)
        goto errorExit;
#else
	/* Use hardcoded allocation */
	pDrvCtrl->riPtr = (void *)(pDrvCtrl->immrVal + MOT_FCC_RIPTR_VAL(pDrvCtrl->fccNum)); /*length == 0x20*/
	pDrvCtrl->tiPtr = (void *)(pDrvCtrl->immrVal + MOT_FCC_TIPTR_VAL(pDrvCtrl->fccNum)); /*length == 0x20*/
	pDrvCtrl->padPtr= (void *)(pDrvCtrl->immrVal + MOT_FCC_PADPTR_VAL(pDrvCtrl->fccNum)); /*length == 0x20*/
#endif
    /* connect the interrupt handler */
    SYS_FCC_INT_CONNECT (pDrvCtrl, motFccInt, (int) pDrvCtrl, retVal);
    if (retVal == ERROR)
        goto errorExit;

    pDrvCtrl->rxTxHandling = FALSE;

    pDrvCtrl->state = MOT_FCC_STATE_LOADED;

    return(&pDrvCtrl->endObj);

errorExit:
    motFccUnload (pDrvCtrl);
    
    return NULL;
    }

/*******************************************************************************
*
* motFccUnload - unload a driver from the system
*
* This routine unloads the driver pointed to by <pDrvCtrl> from the system.
*
* RETURNS: OK, always.
*
* SEE ALSO: motFccLoad()
*/
LOCAL STATUS motFccUnload
    (
    DRV_CTRL * pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int retVal;

    if (pDrvCtrl == NULL)
        return (ERROR);

    if (pDrvCtrl->state & MOT_FCC_STATE_LOADED)
	{
	/* must stop the device before unloading it */

	if ((pDrvCtrl->state & MOT_FCC_STATE_RUNNING) == MOT_FCC_STATE_RUNNING)
	    motFccStop(pDrvCtrl);

	/* disconnect the interrupt handler */

	SYS_FCC_INT_DISCONNECT (pDrvCtrl, motFccInt, (int)pDrvCtrl, retVal);

	if ( retVal == ERROR )
	    MOT_FCC_LOG (MOT_FCC_DBG_LOAD,
			 "motfcc%d : cannot disconnect interrupt!\n",
			 pDrvCtrl->unit, 0,0,0,0,0);

	/* OK even if pMib2Tbl is still zero ... */
	endM2Free (&pDrvCtrl->endObj);

	/* free misc resources - should be safe even if just partly init'ed */

	END_OBJECT_UNLOAD (&pDrvCtrl->endObj);

	pDrvCtrl->state = MOT_FCC_STATE_NOT_LOADED;
	}

    /*
     * The device is now stopped and its interrupts are disabled. It is
     * also disassociated from any protocols or timers.
     *
     * At this time all netPool resources associated with descriptors
     * will be returned to the pools they came from.
     */

    /*
     * For END_NET_POOL_INIT type pools, it's only safe to free
     * the net pool if all of the tuples are returned.
     */

    if (pDrvCtrl->initType == END_NET_POOL_INIT &&
	pDrvCtrl->endObj.pNetPool != NULL &&
	pDrvCtrl->endObj.pNetPool->pPoolStat->mTypes[MT_FREE] !=
	pDrvCtrl->endObj.pNetPool->mBlkCnt)
	return ERROR;

    if (motFccMemFree (pDrvCtrl) != OK)
	return ERROR;

    /* Indicate to MUX that pDrvCtrl is already (or will be) freed */
    return (EALREADY);
    }


/*******************************************************************************
*
* motFccMemFree - Free the driver's memory
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS motFccMemFree
    (
    DRV_CTRL *pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int         ix;
    M_BLK_ID    pMblk;
    FCC_PARAM_T     *   pParam;
    FCC_ETH_PARAM_T *   pEthPar;

    /* get to the beginning of the parameter area */
    pParam  = pDrvCtrl->fccPar;
    pEthPar = pDrvCtrl->fccEthPar;

    if (!pDrvCtrl->rxTxHandling)
        {
        /* Remove and free all mBlk chains associated with transmit queue */

        if (pDrvCtrl->tBufList)
            {
            for (ix = 0; ix < pDrvCtrl->tbdNum; ix++)
                {
                if ((pMblk = pDrvCtrl->tBufList[ix]) != NULL)
                    netMblkClChainFree (pMblk);
                }

            if (pDrvCtrl->pTxPollMblk)
                netTupleFree(pDrvCtrl->pTxPollMblk);
 
            free (pDrvCtrl->tBufList);
	    pDrvCtrl->tBufList = NULL;
            }

        /* Free Tuples in receive descriptors */


        if (pDrvCtrl->pMblkList)
            {
            for (ix = 0; ix < pDrvCtrl->rbdNum; ix++)
                {
                pMblk = pDrvCtrl->pMblkList[ix];    
                netTupleFree (pMblk);
                }
            free  (pDrvCtrl->pMblkList);
	    pDrvCtrl->pMblkList = NULL;
            }

        if ((MOT_FCC_FLAG_ISSET (MOT_FCC_OWN_BD_MEM)) && 
           (pDrvCtrl->pRawBdBase != NULL))
            {
            cacheDmaFree (pDrvCtrl->pRawBdBase);
	    pDrvCtrl->pRawBdBase = NULL;
            }
        else
            {
            /* release from the DPRAM pool */

            pDrvCtrl->dpramFreeFunc ((void *) pDrvCtrl->pBdBase);
            pDrvCtrl->pBdBase = NULL;
            }

        if (pDrvCtrl->phyInfo != NULL)
	    {
            free (pDrvCtrl->phyInfo);
	    pDrvCtrl->phyInfo = NULL;
	    }

#if defined (MOT_FCC_DBG) || defined (MOT_FCC_STAT_MONITOR)
	if (pDrvCtrl == pDrvCtrlDbg_fcc0)
	    pDrvCtrlDbg_fcc0 = NULL;

        if (pDrvCtrl->Stats)
	    {
            free (pDrvCtrl->Stats);
	    pDrvCtrl->Stats = NULL;
	    }
#endif /* MOT_FCC_DBG or MOT_FCC_STAT_MONITOR */

        /* free allocated DPRAM memory if necessary */

        if (pDrvCtrl->riPtr != NULL)
            {
            pDrvCtrl->dpramFccFreeFunc (pDrvCtrl->riPtr);
            pParam->riptr = 0;
            pDrvCtrl->riPtr = NULL;
            }

        if (pDrvCtrl->tiPtr != NULL)
            {
            pDrvCtrl->dpramFccFreeFunc (pDrvCtrl->tiPtr);
            pParam->tiptr = 0;
            pDrvCtrl->tiPtr = NULL;
            }

        if (pDrvCtrl->padPtr != NULL)
            {
            pDrvCtrl->dpramFccFreeFunc (pDrvCtrl->padPtr);
            pEthPar->pad_ptr = 0;
            pDrvCtrl->padPtr = NULL;
            }

	if (pDrvCtrl->endObj.pNetPool != NULL)
	    {
	    if (pDrvCtrl->initType == END_NET_POOL_INIT)
		{
		/* We checked earlier that all M_BLKs were returned. */

		if (pDrvCtrl->pMBlkArea != NULL)
		    {
		    free (pDrvCtrl->pMBlkArea);
		    pDrvCtrl->pMBlkArea = NULL;
		    }

		/*
		 * Note, the following doesn't free the cluster memory, which
		 * we don't own: it was passed by the BSP.
		 */
		netPoolDelete (pDrvCtrl->endObj.pNetPool);
		}
	    else
		{
		/* Schedule the pool to be freed */
		netPoolRelease (pDrvCtrl->endObj.pNetPool, NET_REL_IN_TASK);
		}

	    pDrvCtrl->endObj.pNetPool = NULL;
	    }

        free (pDrvCtrl);
        }
    else
        {
        if ((NET_JOB_ADD ((FUNCPTR) motFccMemFree, (int) pDrvCtrl,
                        0,0,0,0)) == ERROR)
            {
            logMsg("The netJobRing is full.\n",0,0,0,0,0,0);
            }
        }
    return (OK);
    }

/*******************************************************************************
*
* motFccInitParse - parse parameter values from initString
*
* This routine parses parameter values from initString and stores them in
* the related files of the driver control structure.
*
* RETURNS: OK or ERROR
*/
LOCAL STATUS motFccInitParse
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    char *  initString      /* parameter string */
    )
    {
    char *  tok;            /* an initString token */
    char *  holder = NULL;  /* points to initString fragment beyond token */

    /* unit number */
    tok = strtok_r (initString, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->unit = (int) strtoul (tok, NULL, 16);

    /* internal RAM base address */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->immrVal = (UINT32) strtoul (tok, NULL, 16);

    /* fcc number */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->fccNum = (UINT8) strtoul (tok, NULL, 16);

    /* Base Address of BDs */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->pBdBase = (char *) strtoul (tok, NULL, 16);

    /* Size of BDs */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->bdSize = strtoul (tok, NULL, 16);

    /* Memory Pool Base Address */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->pBufBase = (char *) strtoul (tok, NULL, 16);

    /* Memory Pool Size */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->bufSize = strtoul (tok, NULL, 16);

    /* TX FIFO Base Address */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->fifoTxBase = (UINT32) strtoul (tok, NULL, 16);

    /* RX FIFO Base Address */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->fifoRxBase = (UINT32) strtoul (tok, NULL, 16);

    /* Number of TX Buffer Descriptors */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->tbdNum = (UINT16) strtoul (tok, NULL, 16);

    /* Number of RX Buffer Descriptors */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->rbdNum = (UINT16) strtoul (tok, NULL, 16);

    /* Address of the PHY */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->phyInfo->phyAddr = (UINT8) strtoul (tok, NULL, 16);

    /* PHY Default Operating Mode */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->phyInfo->phyDefMode = (UINT8) strtoul (tok, NULL, 16);

    /* Auto-Negotiation Table */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->phyInfo->phyAnOrderTbl = (MII_AN_ORDER_TBL *)
                    strtoul (tok, NULL, 16);

    /* User Flags */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->userFlags = strtoul (tok, NULL, 16);

    /* BSP function structure */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->motFccFuncs = (FCC_END_FUNCS *) strtoul (tok, NULL, 16);

    /* passing maxRxFrames is optional. The default is rbdNum * 2  */
    pDrvCtrl->maxRxFrames = 0; /* mark to use default */

    tok = strtok_r (NULL, ":", &holder);
    if ((tok != NULL) && (tok != (char *)-1))
        pDrvCtrl->maxRxFrames = strtoul (tok, NULL, 16);
    /* else default to a reasonable value eg 16  but do it when number of 
     * RBDs  are known in InitMem */ 

    if (pDrvCtrl->tbdNum < MOT_FCC_TBD_MIN)
        {
        MOT_FCC_FLAG_SET (MOT_FCC_INV_TBD_NUM);
        pDrvCtrl->tbdNum = MOT_FCC_TBD_DEF_NUM;
        }

    if (pDrvCtrl->rbdNum < MOT_FCC_RBD_MIN)
        {
        MOT_FCC_FLAG_SET (MOT_FCC_INV_RBD_NUM);
        pDrvCtrl->rbdNum = MOT_FCC_RBD_DEF_NUM;
        }

    return (OK);
    }

/*******************************************************************************
* motFccInitMem - initialize memory
*
* This routine initializes all the memory needed by the driver whose control
* structure is passed in <pDrvCtrl>.
*
* RETURNS: OK or ERROR
*/
LOCAL STATUS motFccInitMem
    (
    DRV_CTRL *  pDrvCtrl
    )
    {
    UINT32  bdMemSize;   /* total BD area including Alignment */
    UINT32  rbdMemSize;  /* Receive BD area size */
    UINT32  tbdMemSize;  /* Transmit BD area size */
    UINT16  clNum;       /* a buffer number holder */
    
    /* initialize the netPool */
    if ( (pDrvCtrl->endObj.pNetPool = malloc (sizeof (NET_POOL))) == NULL )
        return ERROR;

    /*
     * Establish the memory area that we will share with the device.  This
     * area may be thought of as being divided into two parts: one is the
     * buffer descriptors' (BD) and the second one is for the data buffers.
     * Since they have different requirements as far as cache are concerned,
     * they may be addressed separately.
     * We'll deal with the BDs area first.  If the caller has provided
     * an area, then we assume it is non-cacheable and will not require
     * the use of the special cache routines. If the caller has not provided
     * an area, then we must obtain it from the system, using the cache
     * savvy allocation routine.
     */

    switch ( (int) pDrvCtrl->pBdBase )
        {

        case NONE :
            /*
             * The user has not provided a BD data area
             * This driver can't handle write incoherent caches.
             */
            if ( !CACHE_DMA_IS_WRITE_COHERENT () )
                {
                free(pDrvCtrl->endObj.pNetPool);
		pDrvCtrl->endObj.pNetPool = NULL;
                return ERROR;
                }

            rbdMemSize = MOT_FCC_RBD_SZ * pDrvCtrl->rbdNum;
            tbdMemSize = MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum;
            bdMemSize  = rbdMemSize + tbdMemSize + MOT_FCC_BD_ALIGN;

            /* Allocated from dma safe memory */
            pDrvCtrl->pRawBdBase = cacheDmaMalloc(bdMemSize);

            if ( pDrvCtrl->pRawBdBase == NULL )
                {
                free(pDrvCtrl->endObj.pNetPool);
		pDrvCtrl->endObj.pNetPool = NULL;
		return ERROR;    /* no memory available */
                }
             
            pDrvCtrl->pBdBase = pDrvCtrl->pRawBdBase;

            pDrvCtrl->bdSize = bdMemSize;
            MOT_FCC_FLAG_SET (MOT_FCC_OWN_BD_MEM);
            break;

        default :
            /* The user provided an area for the BDs  */
            if ( !pDrvCtrl->bdSize )
                {
                free(pDrvCtrl->endObj.pNetPool);
		pDrvCtrl->endObj.pNetPool = NULL;
                return ERROR;
                }

            /*
             * Check whether user provided a number for Rx and Tx BDs.
             * Fill in the blanks if we can.
             * Check whether enough memory was provided, etc.
             */

            if (MOT_FCC_FLAG_ISSET (MOT_FCC_INV_TBD_NUM) &&
                MOT_FCC_FLAG_ISSET (MOT_FCC_INV_RBD_NUM))
                {
                pDrvCtrl->tbdNum = (UINT16)
		    (pDrvCtrl->bdSize / (MOT_FCC_TBD_SZ + MOT_FCC_RBD_SZ));
                pDrvCtrl->rbdNum = pDrvCtrl->tbdNum;
                }
            else if (MOT_FCC_FLAG_ISSET (MOT_FCC_INV_TBD_NUM))
                {
                rbdMemSize = MOT_FCC_RBD_SZ * pDrvCtrl->rbdNum;
                pDrvCtrl->tbdNum = (UINT16)((pDrvCtrl->bdSize - rbdMemSize) /
					    MOT_FCC_TBD_SZ);
                }
            else if (MOT_FCC_FLAG_ISSET (MOT_FCC_INV_RBD_NUM))
                {
                tbdMemSize = MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum;
                pDrvCtrl->rbdNum = (UINT16) ((pDrvCtrl->bdSize - tbdMemSize) /
					     MOT_FCC_RBD_SZ);
                }
            else
                {
                rbdMemSize = MOT_FCC_RBD_SZ * pDrvCtrl->rbdNum;
                tbdMemSize = MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum;
                bdMemSize = rbdMemSize + tbdMemSize + MOT_FCC_BD_ALIGN;
                if ( pDrvCtrl->bdSize < bdMemSize )
                    return ERROR;
                }
            if ((pDrvCtrl->tbdNum < MOT_FCC_TBD_MIN) ||
                (pDrvCtrl->rbdNum < MOT_FCC_RBD_MIN))
                return ERROR;

            MOT_FCC_FLAG_CLEAR (MOT_FCC_OWN_BD_MEM);
            break;
        }

    if (pDrvCtrl->maxRxFrames < 1)
        pDrvCtrl->maxRxFrames = pDrvCtrl->rbdNum >> 1; /* Divide by 2 */

    /* zero the shared memory */
    memset (pDrvCtrl->pBdBase, 0, (int) pDrvCtrl->bdSize);

    /* align the shared memory */
    pDrvCtrl->pBdBase = (char *) ROUND_UP((UINT32)pDrvCtrl->pBdBase,
					  MOT_FCC_BD_ALIGN);

    /*
     * number of clusters: twice number of RBDs including a min number of 
     * transmit clusters and one transmit cluster for polling operation.
     * TODO: The ratio of clusters to RBDs should probably be made configurable.
     */

    clNum = (2 * pDrvCtrl->rbdNum) +
               MOT_FCC_TX_POLL_NUM +
               MOT_FCC_TX_CL_NUM;

    /* 
     * Now we'll deal with the data buffers.  If the caller has provided
     * an area, then we assume it is non-cacheable and will not require
     * the use of the special cache routines. If the caller has not provided
     * an area, then we must obtain it from the system. This means we will not be
     * using the cache savvy allocation routine, since we will flushing or
     * invalidate the data cache itself as appropriate. This speeds up
     * driver operation, as the network stack will be able to process data
     * in a cacheable area.
     */


    switch ( (int) pDrvCtrl->pBufBase )
        {
        case NONE :
            /*
             * The user has not provided data area for the clusters.
             * Allocate from cacheable data block.
             */

            pDrvCtrl->initType = END_NET_POOL_CREATE;

            MOT_FCC_FLAG_SET (MOT_FCC_OWN_BUF_MEM);

            /* cache functions descriptor for data buffers */
            pDrvCtrl->bufCacheFuncs = cacheUserFuncs;

            break;

        default :
            /*
             * The user provides the area for buffers. This must be from a
             * non cacheable area.
             */

            pDrvCtrl->initType = END_NET_POOL_INIT;

            MOT_FCC_FLAG_CLEAR (MOT_FCC_OWN_BUF_MEM);
            pDrvCtrl->bufCacheFuncs = cacheNullFuncs;
            break;
        }

     if (pDrvCtrl->initType == END_NET_POOL_INIT)
        {
	/* M_BLK/CL_BLK configuration */
	M_CL_CONFIG mclBlkConfig = {0, 0, NULL, 0};

	/* cluster configuration table */
	CL_DESC     clDescTbl [] = { {MOT_FCC_MAX_CL_LEN, 0, NULL, 0}};

	/* number of different clusters sizes in pool -- only 1 */
	int     clDescTblNumEnt = 1;

        /* pool of clusters */

	clDescTbl[0].clNum = clNum;
	clDescTbl[0].clSize = MOT_FCC_MAX_CL_LEN;

	/* Set memArea to the buffer base */

	clDescTbl[0].memArea = pDrvCtrl->pBufBase;

        /*
	 * There's a cluster overhead and an alignment issue. The
	 * most stringent alignment requirement is from linkBufLib.
	 * The CL_SIZE() macro rounds up to a multiple of NETBUF_ALIGN (64).
	 * The extra NETBUF_ALIGN is required by linkBufLib to guarantee
	 * alignment.
	 */

        clDescTbl[0].memSize = clNum * CL_SIZE (MOT_FCC_MAX_CL_LEN) + NETBUF_ALIGN;

	/*
	 * check the user provided enough memory with reference
	 * to the given number of receive/transmit frames, if any.
	 */
	if ( pDrvCtrl->bufSize < clDescTbl[0].memSize )
	    return ERROR;

        /* zero the shared memory */

        memset (pDrvCtrl->pBufBase, 0, (int) pDrvCtrl->bufSize);

	/* pool of mblks */

	mclBlkConfig.mBlkNum = clNum;

        /* pool of cluster blocks */

	mclBlkConfig.clBlkNum = clNum;

	/* memory requirements from linkBufPool.c. (See linkMblkCarve().) */

	mclBlkConfig.memSize = ROUND_UP ((sizeof (M_LINK) + sizeof (NET_POOL_ID)),
					 NETBUF_ALIGN) * clNum + NETBUF_ALIGN;

	/* linkBufPool takes care of the alignment */

	mclBlkConfig.memArea = (char *) malloc (mclBlkConfig.memSize);

	if (mclBlkConfig.memArea == NULL)
	    {
	    return ERROR;
	    }

	/* store the pointer to the mBlock area */
	pDrvCtrl->pMBlkArea = mclBlkConfig.memArea;
	pDrvCtrl->mBlkSize = mclBlkConfig.memSize;

        /* init the mem pool */

        if ( netPoolInit (pDrvCtrl->endObj.pNetPool, &mclBlkConfig, &clDescTbl[0], 
                          clDescTblNumEnt, _pLinkPoolFuncTbl) == ERROR )
            return ERROR;

        }
    else  /* pDrvCtrl->initType == END_NET_POOL_CREATE */
        {
	NETBUF_CFG netBufCfg = {NULL};
	char name [END_NAME_MAX + 8];
	NETBUF_CL_DESC clDesc;


        /* Initialize the pName field */

	netBufCfg.pName = name;  /* netPoolCreate() copies the name */

        sprintf (name, "%s%d%s","motFcc", pDrvCtrl->unit, "Pool");

        /*
         * Set the attributes to be Cached, Cache aligned, sharable, &
         * ISR safe
         */

        netBufCfg.attributes = ATTR_AC_SH_ISR;

        /* Set pDomain to kernel, use NULL. */
        netBufCfg.pDomain = NULL;

        /* Set ratio of mBlks to clusters */
        netBufCfg.ctrlNumber = clNum;

        /* Set memory partition of mBlks to kernel, use NULL */
        netBufCfg.ctrlPartId = NULL;

        /* Set extra memory size to zero for now */
        netBufCfg.bMemExtraSize = 0;

        /* Set cluster's memory partition to kernel, use NULL */
        netBufCfg.bMemPartId = NULL;

        netBufCfg.pClDescTbl = &clDesc;

        /* Initialize the Cluster Descriptor */
        netBufCfg.pClDescTbl->clSize = MOT_FCC_RX_CL_SZ;
        netBufCfg.pClDescTbl->clNum = clNum;
        netBufCfg.clDescTblNumEnt = 1;
        /* Call netPoolCreate() with the Link Pool Function Table */

        if ((pDrvCtrl->endObj.pNetPool =
                        netPoolCreate (&netBufCfg, _pLinkPoolFuncTbl)) == NULL)
            return (ERROR);

        }


    /* allocate receive buffer list */
    pDrvCtrl->pMblkList = (M_BLK_ID *) calloc (pDrvCtrl->rbdNum,
					       sizeof(M_BLK_ID) );
    if (pDrvCtrl->pMblkList == NULL)
        return ERROR;

    /* allocate the transmit pMblk list */
    pDrvCtrl->tBufList = (M_BLK **) calloc (pDrvCtrl->tbdNum, sizeof(M_BLK *));
    if (pDrvCtrl->tBufList == NULL)
        return ERROR;

    return OK;
    }

/**************************************************************************
*
* motFccStart - start the device
*
* This routine starts the FCC device and brings it up to an operational
* state.  The driver must have already been loaded with the motFccEndLoad()
* routine.
*
* \INTERNAL
* The speed field in the phyInfo structure is only set after the call
* to the physical layer init routine. On the other hand, the mib2
* interface is initialized in the motFccEndLoad() routine, and the default
* value of 10Mbit assumed there is not always correct. We need to
* correct it here.
*
* RETURNS: OK, or ERROR if the device could not be started.
*
*/
LOCAL STATUS motFccStart
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    int          retVal;     /* convenient holder for return value */
    char         bucket[4];  /* holder for vxMemProbe */
    FCC_REG_T  * reg = pDrvCtrl->fccReg;

    MOT_FCC_LOG (MOT_FCC_DBG_START,"motFccStart\n", 1, 2, 3, 4, 5, 6);

    if (pDrvCtrl == NULL)
	return ERROR;

    /* must have been loaded */
    if ((pDrvCtrl->state & MOT_FCC_STATE_LOADED) == MOT_FCC_STATE_NOT_LOADED)
        return ERROR;

    /* check if already running */
    if ((pDrvCtrl->state & MOT_FCC_STATE_RUNNING) == MOT_FCC_STATE_RUNNING)
        return OK;

    if (vxMemProbe ((char *) (pDrvCtrl->fccIramAddr), VX_READ, 4, &bucket[0]) != OK)
        return ERROR;


    /* enable system interrupt: set relevant bit in SIMNR_L */
    SYS_FCC_INT_ENABLE (pDrvCtrl, retVal);
    if (retVal == ERROR)
        return ERROR;

    /* call the BSP to do any other initialization (MII interface) */
    SYS_FCC_ENET_ENABLE;


    /* Initialize some fields in the PHY info structure */
    if (motFccPhyPreInit (pDrvCtrl) != OK)
        return ERROR;

    /* Initialize the Physical medium layer */
    if (pDrvCtrl->phyInitFunc != NULL)
        {
        if ( ( pDrvCtrl->phyInitFunc (pDrvCtrl->phyInfo)) != OK)
            return ERROR;
        }
	
    /* Initialize the receive buffer descriptors */
    if (motFccRbdInit(pDrvCtrl) == ERROR)
        return ERROR;

    /* Initialize the transmit buffer descriptors */
    if (motFccTbdInit(pDrvCtrl) == ERROR)
        return ERROR;

    /* Initialize the parameter RAM for Ethernet operation */
    if (motFccPramInit (pDrvCtrl) != OK)
        return ERROR;

    /* initialize the internal RAM for Ethernet operation */
    if (motFccIramInit (pDrvCtrl) != OK)
        return ERROR;


    /* correct the speed for the mib2 stuff */
    pDrvCtrl->endObj.mib2Tbl.ifSpeed = pDrvCtrl->phyInfo->phySpeed;

    /* startup the transmitter and receiver */
    reg->fcc_gfmr |= (M8260_GFMR_ENT | M8260_GFMR_ENR);
    END_FLAGS_SET (&pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING));

    /* Flush the write pipe */
    CACHE_PIPE_FLUSH ();

    pDrvCtrl->state |= MOT_FCC_STATE_RUNNING;

    return OK;
    }

/**************************************************************************
*
* motFccStop - stop the 'motfcc' interface
*
* This routine marks the interface as inactive, disables interrupts and
* the Ethernet Controller. As a result, reception is stopped immediately,
* and transmission is stopped after a bad CRC is appended to any frame
* currently being transmitted. The reception/transmission control logic
* (FIFO pointers, buffer descriptors, etc.) is reset. To bring the
* interface back up, motFccStart() must be called.
*
* RETURNS: OK, always.
*/
LOCAL STATUS motFccStop
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    int             retVal;	/* convenient holder for return value */
    FCC_REG_T     * reg;	/* an even bigger convenience */
    int             intLevel;
    volatile int    rxTxTimeout = 0;
 
    MOT_FCC_LOG ( MOT_FCC_DBG_START,"motFccStop\n", 1, 2, 3, 4, 5, 6);
    
    if (pDrvCtrl == NULL)
        return (ERROR);

    /* check if already stopped */
    if ((pDrvCtrl->state & MOT_FCC_STATE_RUNNING) != MOT_FCC_STATE_RUNNING)
        return OK;

    reg = pDrvCtrl->fccReg;

    /* mark the interface as down */
    END_FLAGS_CLR ( &pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING) );

    intLevel = intLock ();

    /* disable the transmitter and receiver */
    reg->fcc_gfmr &= ~(M8260_GFMR_ENT | M8260_GFMR_ENR);

    /* disable system interrupt: reset relevant bit in SIMNR_L */
    SYS_FCC_INT_DISABLE (pDrvCtrl, retVal);
    intUnlock (intLevel);
    if (retVal == ERROR)
        return ERROR;

    while( (pDrvCtrl->rxTxHandling == TRUE) || (rxTxTimeout++ > 100))
        taskDelay((sysClkRateGet()>>6) + 1);

    /* free Tx buffers  */
    if (motFccTbdFree(pDrvCtrl) == ERROR)
        return(ERROR);

    /* free Rx buffers  */
    if (motFccRbdFree(pDrvCtrl) == ERROR)
        return (ERROR);

    if ((pDrvCtrl->pTxPollMblk) != NULL)
        netTupleFree (pDrvCtrl->pTxPollMblk);

    if ((pDrvCtrl->phyInfo != NULL) && 
        (pDrvCtrl->phyInfo->phyFlags & MII_PHY_INIT))
        {
        if (INT_CONTEXT())
            {
            NET_JOB_ADD ((FUNCPTR) miiPhyUnInit, (int)(pDrvCtrl->phyInfo),
                       0, 0, 0, 0);
            NET_JOB_ADD ((FUNCPTR) cfree, (int)(pDrvCtrl->phyInfo),
                       0, 0, 0, 0);
	    /* FIXME - need to zero pDrvCtrl->phyInfo */
            }
        else
            {
            if (miiPhyUnInit (pDrvCtrl->phyInfo) != OK)
                {
                logMsg("miiPhyUnInit fails\n", 0,0,0,0,0,0);
                }
            free (pDrvCtrl->phyInfo);
	    pDrvCtrl->phyInfo = NULL;
            }
        }
    /* call the BSP to disable the MII interface */

    SYS_FCC_ENET_DISABLE;

    pDrvCtrl->state &= ~MOT_FCC_STATE_RUNNING;

    return OK;
    }

/*******************************************************************************
* motFccHandler - entry point for handling job events from motFccInt
*
* RETURNS: N/A
*/

_WRS_FASTTEXT
LOCAL void motFccHandler
    (
    DRV_CTRL *pDrvCtrl,
    UINT32    event
    )
    {
    FCC_REG_T * reg = pDrvCtrl->fccReg;
    int		intLevel;
    UINT32	more;
    int		retVal;
    int		maxRxFrames;

    more = 0;

again:
    maxRxFrames = pDrvCtrl->maxRxFrames;

    FOREVER
	{
	if ((event & (M8260_FEM_ETH_RXF | M8260_FEM_ETH_BSY)) != 0)
	    {
	    more = motFccHandleRXFrames (pDrvCtrl, &maxRxFrames);

#ifdef MOT_FCC_STAT_MONITOR
	    if (event & M8260_FEM_ETH_BSY)
		{
		MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numBSYInts);
		}
#endif /* MOT_FCC_STAT_MONITOR */

	    }

	if (event & (M8260_FEM_ETH_TXB | M8260_FEM_ETH_TXE))
	    {
	    UINT32 tbdFree;
	    BOOL restart = FALSE;

	    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);
	    tbdFree = motFccTbdClean (pDrvCtrl);
	    if (pDrvCtrl->txStall == TRUE)
		{
		if (tbdFree > pDrvCtrl->txUnStallThresh)
		    {
		    /* turn off stall alarm */
		    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numTxStallsCleared);

		    pDrvCtrl->txStall = FALSE;
		    restart = TRUE;
		    }
		}
	    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

	    /*
	     * We may not call muxTxRestart() with the TX semaphore
	     * held, because splSemId will be taken inside the muxTxRestart()
	     * call. Since in a normal send splSemId is taken BEFORE
	     * the END object TX semaphore, we would have a semaphore ordering
	     * problem and could get a deadlock.
	     */
	    if (restart)
		{
		pDrvCtrl->muxTxRestart (pDrvCtrl);
		}

#ifdef MOT_FCC_STAT_MONITOR
	    if (event & M8260_FEM_ETH_TXE)
		{
		MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numTXEInts);
		}
	    if (event & M8260_FEM_ETH_TXB)
		{
		MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numTXBInts);
		}
#endif /* MOT_FCC_STAT_MONITOR */
	    }

#ifdef MOT_FCC_STAT_MONITOR
	if (event & M8260_FEM_ETH_RXC)
	    {
	    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXCInts);
	    }
	if (event & M8260_FEM_ETH_TXC)
	    {
	    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numTXCInts);
	    }
#endif /* MOT_FCC_STAT_MONITOR */

	intLevel = intLock();

	event = reg->fcc_fcce;

	/*
	 * If we know there are more RX frames to process, treat that as
	 * a M8260_FEM_ETH_RXF event.
	 */
	event |= more;

	/* Don't bother looping just for RXC/TXC (flow control) statistics. */

	if ((event & ( M8260_FEM_ETH_TXE | M8260_FEM_ETH_TXB |
		       M8260_FEM_ETH_RXF | M8260_FEM_ETH_BSY )) == 0)
	    goto motFccHandler_noEvent;

	/* clear the events which we will service */

	reg->fcc_fcce = (UINT16) event;

	/*
	 * Reschedule ourselves if we've done the maximum number of
	 * local loops.
	 */

	if (maxRxFrames <= 0)
	    break;

	intUnlock (intLevel);
	}

    /*
     * Interrupts are locked. Note that <event> contains some
     * events, or we know there are more RX descriptors which
     * are ready to be processed.
     */

    retVal = pDrvCtrl->netJobAdd ((FUNCPTR) motFccHandler,
				      (int)pDrvCtrl, event);

    intUnlock (intLevel);

    /*
     * If we successfully posted the net job, exit, leaving device interrupts
     * disabled (apart from M8260_FEM_ETH_GRA).
     */

    if (retVal == OK)
	return;

    /*
     * Our netJobAdd() failed. Since we know there are events we need to
     * handle, we have no choice but to handle them in this invocation.
     */
    goto again;

motFccHandler_noEvent:

    pDrvCtrl->rxTxHandling = FALSE;

    reg->fcc_fccm = ( M8260_FEM_ETH_TXE |
		      M8260_FEM_ETH_TXB |
		      M8260_FEM_ETH_RXF |
		      M8260_FEM_ETH_BSY );

    intUnlock (intLevel);
    return;
    }

/*******************************************************************************
* motFccInt - entry point for handling interrupts from the FCC
*
* The interrupting events are acknowledged to the device. The device
* de-asserts its interrupt signal.  The amount of work done here is kept
* to a minimum; the bulk of the work is deferred to the netTask. However, the
* interrupt code takes on the responsibility to clean up the TX and the RX
* BD rings.
*
* RETURNS: N/A
*/

_WRS_FASTTEXT
LOCAL void motFccInt
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    UINT32      status;     /* status word */
    FCC_REG_T * reg;
    STATUS      retVal;

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numInts);

    /*
     * If we have already posted a net job, then interrupts are
     * disabled for this device, and this interrupt is somebody else's.
     * We should not clear events, because the task-level handler
     * uses these to decide if it needs to do more work.
     */
    
    if (pDrvCtrl->rxTxHandling)
	return;

    reg = pDrvCtrl->fccReg;

    /* Should we mask further events before reading the current events? */

    /* get pending events */
    status = reg->fcc_fcce;

    /* clear pending events */
    reg->fcc_fcce = (UINT16)status;

    /*
     * NOTE WELL: if we clear an interrupt, we MUST handle it now,
     * as it may not occur again. This is why we do not mask status
     * with fccm.
     */

    if ((status & (M8260_FEM_ETH_BSY | M8260_FEM_ETH_RXF |
		   M8260_FEM_ETH_TXE | M8260_FEM_ETH_TXB)) == 0)
	{
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numOTHERInts);
	return;
	}

    retVal = pDrvCtrl->netJobAdd (motFccHandler, pDrvCtrl, status);

    if (retVal == OK)
	{
	pDrvCtrl->rxTxHandling = TRUE;
	reg->fcc_fccm = 0;
	}
    else
	{
	MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numNetJobAddErrors);

	/* Leave interrupts enabled. Maybe start watchdog? */
#if 0
	if (!pDrvCtrl->netJobFailWdogStarted)
	    {
	    wdStart (pDrvCtrl->netJobFailWdogId, sysClkRateGet(),
		     (FUNCPTR)netJobFailWdog, (int)pDrvCtrl);
	    }
#endif
	}

    return;
    }

/*******************************************************************************
* motFccTxErrReinit - Special TX errors can only be cleared by a restart of
* the FCC transmitter. This function re-initializes the transmitter and then
* restarts. The TX BD ring is adjusted accordingly.
*
*
* RETURNS: N/A
*/

LOCAL void motFccTxErrReInit
    (
    DRV_CTRL *pDrvCtrl,
    UINT32    statLen
    )
    {
    VUINT32     cpcrVal;
    FCC_REG_T * reg = pDrvCtrl->fccReg;
    UINT32      immrVal = pDrvCtrl->immrVal; /* holder for the immr value */
    UINT8       fccNum  = pDrvCtrl->fccNum;  /* holder for the fcc number */
    UINT8       fcc     = fccNum - 1;
    UINT32      bdStatLen;

    reg->fcc_gfmr  &= ~M8260_GFMR_ENT;


    bdStatLen = pDrvCtrl->pTbdLast->bdStatLen;

    pDrvCtrl->pTbdLast->bdStatLen = bdStatLen & ~M8260_FETH_TBDS_R;

    /* issue a init tx command to the CP */

    /* wait until the CP is clear */
    cpcrVal = 0;
    do
        {
        MOT_FCC_REG_LONG_RD (M8260_CPCR (immrVal), cpcrVal);
        }
    while (cpcrVal & M8260_CPCR_FLG);

    /* issue the command to the CP */

    cpcrVal = (M8260_CPCR_OP ((UINT8)M8260_CPCR_TX_INIT)
             | M8260_CPCR_SBC (M8260_CPCR_SBC_FCC1   | (fcc * 0x1))
             | M8260_CPCR_PAGE (M8260_CPCR_PAGE_FCC1 | (fcc * 0x1))
             | M8260_CPCR_MCN (M8260_CPCR_MCN_ETH)
             | M8260_CPCR_FLG);

    MOT_FCC_REG_LONG_WR (M8260_CPCR (immrVal), cpcrVal);
    CACHE_PIPE_FLUSH ();        /* Flush the write pipe */

    /* wait until the CP is clear */
    cpcrVal = 0;
    do
        {
        MOT_FCC_REG_LONG_RD (M8260_CPCR (immrVal), cpcrVal);
        }
    while (cpcrVal & M8260_CPCR_FLG);

        /* issue the command to the CP */

    cpcrVal = (M8260_CPCR_OP ((UINT8)M8260_CPCR_TX_RESTART)
             | M8260_CPCR_SBC (M8260_CPCR_SBC_FCC1 | (fcc * 0x1))
             | M8260_CPCR_PAGE (M8260_CPCR_PAGE_FCC1 | (fcc * 0x1))
             | M8260_CPCR_MCN (M8260_CPCR_MCN_ETH)
             | M8260_CPCR_FLG);

    MOT_FCC_REG_LONG_WR (M8260_CPCR (immrVal), cpcrVal);
    CACHE_PIPE_FLUSH ();        /* Flush the write pipe */

    /* wait until the CP is clear */
    cpcrVal = 0;
    do
        {
        MOT_FCC_REG_LONG_RD (M8260_CPCR (immrVal), cpcrVal);
        }
    while (cpcrVal & M8260_CPCR_FLG);


    /* point the TBPTR to the correct bd */
    do
        {
        pDrvCtrl->fccPar->tbptr = (UINT32) pDrvCtrl->pTbdLast;
        }
    while (pDrvCtrl->fccPar->tbptr != (UINT32) pDrvCtrl->pTbdLast);


    /* enable transmitter */
    reg->fcc_gfmr  |= M8260_GFMR_ENT;

    /* mark the next bd ready to send */
    if ( bdStatLen & M8260_FETH_TBDS_R )
        pDrvCtrl->pTbdLast->bdStatLen = bdStatLen;

    MOT_FCC_LOG ( MOT_FCC_DBG_INT_TX_ERR,
        ("motFccTxErrReInit tTBDPTR 0x%08x Last 0x%08x Next 0x%08x Stat 0x%04x\n"),
         (int)pDrvCtrl->fccPar->tbptr,
         (int)pDrvCtrl->pTbdLast,
         (int)pDrvCtrl->pTbdNext, statLen>>16, 5, 6);
    }

#define MOTFCC_MAX_FRAGS 16

/******************************************************************************
*
* motFccPktTransmit - transmit a packet
*
* This routine transmits the packet described by the given parameters
* over the network, without copying the mBlk to a driver buffer.
* It also updates statistics.
*
* RETURNS: OK, or ERROR if no resources were available.
*/
LOCAL STATUS motFccPktTransmit
    (
    DRV_CTRL * pDrvCtrl,   /* pointer to DRV_CTRL structure */
    M_BLK    * pMblk      /* pointer to the mBlk */
    )
    {
    FCC_BD *pTbd = NULL;   /* pointer to the current ready TBD */
    FCC_BD *pTbdFirst;     /* pointer to the first TBD */
    M_BLK  *pCurr;         /* the current mBlk */
    int     fragNum;     /* number of fragments */
    int     ix;
 
    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numZcopySends);

    /* save the first TBD pointer */

    pTbdFirst = pDrvCtrl->pTbdNext;

    for (pCurr = pMblk, fragNum = 0; pCurr != NULL; pCurr = pCurr->m_next)
        {
        if (pCurr->m_len != 0)
            {
            if (fragNum == MOTFCC_MAX_FRAGS || pDrvCtrl->tbdFree == 0)
                break;

            /* get the current pTbd */

            pTbd = pDrvCtrl->pTbdNext;

            /* Set up len, buffer addr, descriptor flags. */

            pTbd->bdStatLen = (M8260_FETH_TBDS_PAD | pCurr->m_len);
            pTbd->bdAddr = (UINT32) pCurr->m_data;

	    MOT_FCC_CACHE_FLUSH (pCurr->m_data, pCurr->m_len);

            /*
             * Set the ready bit everywhere except the first
             * descriptor.
             */

            if (fragNum)
                pTbd->bdStatLen |= M8260_FETH_TBDS_R;

            /*
             * Set the 'wrap' bit for the last descriptor in
             * the table.
             */

	    if ((++pDrvCtrl->pTbdNext) ==
                (pDrvCtrl->pTbdBase + pDrvCtrl->tbdNum))
		{
		pTbd->bdStatLen |= M8260_FETH_TBDS_W;
                pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
		}

            fragNum++;
            pDrvCtrl->tbdFree--;
            }
        }

    if (pCurr != NULL)
        goto noDescs;

    /*
     * TODO - inline endM2Packet() for performance if possible.
     * Or better, use the "pull" statistics method.
     */

    endM2Packet(&pDrvCtrl->endObj, pMblk, M2_PACKET_OUT);

    pDrvCtrl->tBufList[pTbd - pDrvCtrl->pTbdBase] = pMblk;
    pTbd->bdStatLen |= (M8260_FETH_TBDS_I | M8260_FETH_TBDS_L |
                        M8260_FETH_TBDS_TC);
    pTbdFirst->bdStatLen |= M8260_FETH_TBDS_R;

    return (OK);

noDescs:

    /*
     * Ran out of descriptors: undo all relevant changes
     * and fall back to copying.
     */

    pTbd = pDrvCtrl->pTbdNext = pTbdFirst;
    for (ix = 0; ix < fragNum; ix++)
        {
        pDrvCtrl->tbdFree++;
        pTbd->bdStatLen  = 0;
        pTbd->bdAddr = (UINT32) NULL;
        if (++pTbd == (pDrvCtrl->pTbdBase + pDrvCtrl->tbdNum))
            pTbd = pDrvCtrl->pTbdBase;
        }

    return (ERROR);
    }

/******************************************************************************
*
* motFccPktCopyTransmit - copy and transmit a packet
*
* This routine transmits the packet described by the given parameters
* over the network, after copying the mBlk to the driver's buffer.
* It also updates statistics.
*
* RETURNS: OK, or END_ERR_BLOCK if no resources were available.
*/
LOCAL STATUS motFccPktCopyTransmit
    (
    DRV_CTRL *pDrvCtrl,  /* pointer to DRV_CTRL structure */
    M_BLK *   pMblk      /* pointer to the mBlk */
    )
    {
    FCC_BD * pTbd;      /* pointer to the current ready TBD */
    M_BLK_ID pNewMblk;
    char   * pBufCpy;   /* pointer to the location to copy to */
    int      len;	/* length of data to be sent */
    UINT32   bdStatLen;
    
    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numNonZcopySends);

    /* allocate tuple */

    if ((pNewMblk = netTupleGet (pDrvCtrl->endObj.pNetPool,
				 MOT_FCC_TX_CL_SZ,
                                 M_DONTWAIT, MT_DATA, TRUE)) == NULL)
	{
	MOT_FCC_LOG (MOT_FCC_DBG_TX_ERR,"Tx Copy Allocation Error\n",
		     1, 2, 3, 4, 5, 6);

	pDrvCtrl->lastError.errCode = END_ERR_NO_BUF;
	muxError(&pDrvCtrl->endObj, &pDrvCtrl->lastError);

	/* drop the packet on the floor */
	netMblkClChainFree (pMblk);

        endM2Packet (&pDrvCtrl->endObj, NULL, M2_PACKET_OUT_DISCARD);

	/* this is NOT a stall condition */
	return (ERROR);
	}

    endM2Packet (&pDrvCtrl->endObj, pMblk, M2_PACKET_OUT);
    
    pBufCpy =  pNewMblk->mBlkHdr.mData;

    /* gather the Mblk in one cluster */
    len = netMblkToBufCopy (pMblk, pBufCpy, NULL);

    /* this free the old Mblk Chain */
    netMblkClChainFree (pMblk);

   /* flush the cache, if necessary */
    MOT_FCC_CACHE_FLUSH (pBufCpy, len);

    /* point to the next free tx BD */
    pTbd = pDrvCtrl->pTbdNext;

    pTbd->bdAddr   = (unsigned long) pBufCpy;

    pDrvCtrl->tBufList[pTbd - pDrvCtrl->pTbdBase] = pNewMblk;

    pDrvCtrl->tbdFree--;

    bdStatLen = ( M8260_FETH_TBDS_R |
		  M8260_FETH_TBDS_L |
		  M8260_FETH_TBDS_TC |
		  M8260_FETH_TBDS_PAD |
                  M8260_FETH_TBDS_I | 
		  len );

    /* move on to the next element */

    if ( pTbd + 1 == pDrvCtrl->pTbdBase + pDrvCtrl->tbdNum )
        {
        pTbd->bdStatLen = (bdStatLen | M8260_FETH_TBDS_W);
        pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
        }
    else
        {
        pTbd->bdStatLen = bdStatLen;
        pDrvCtrl->pTbdNext = pTbd + 1;
        }

    return (OK);
    }

/*******************************************************************************
* motFccSend - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* muxSend() calls this routine each time it wants to send a packet.
*
* RETURNS: OK, or END_ERR_BLOCK, if no resources are available, or ERROR,
* if the device is currently working in polling mode.
*/

_WRS_FASTTEXT
LOCAL STATUS motFccSend
    (
    DRV_CTRL * pDrvCtrl,
    M_BLK    * pMblk
    )
    {

#ifndef _FREE_VERSION
    /* mblk sanity check */

    if (pMblk == NULL)
        {
        MOT_FCC_LOG (MOT_FCC_DBG_TX_ERR, "Zero Mblk\n", 0,0,0,0,0,0);
	goto motFccSend_inval;
        }
#endif

    /* check device mode */

    if (MOT_FCC_FLAG_ISSET (MOT_FCC_POLLING))
	goto motFccSend_free;

    END_TX_SEM_TAKE (&pDrvCtrl->endObj, WAIT_FOREVER);

    if (pDrvCtrl->tbdFree)
        {
        if (motFccPktTransmit (pDrvCtrl, pMblk) != OK)
            motFccPktCopyTransmit (pDrvCtrl, pMblk);
        END_TX_SEM_GIVE (&pDrvCtrl->endObj);
        return (OK);
        }
    
    pDrvCtrl->txStall = TRUE;

    /*
     * Note, a stall is not an error, just normal TX flow control,
     * so don't update MIB statistics.
     * However, we do increment our own stall count counter for
     * performance analysis.
     */

    ++ pDrvCtrl->txStallCount;

    MOT_FCC_LOG (MOT_FCC_DBG_TX_ERR, "TX STALL\n", 0,0,0,0,0,0);

    /* return an error indicating a stall */

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    return (END_ERR_BLOCK);

motFccSend_free:

    /* free the given mBlk chain */
    netMblkClChainFree (pMblk);

motFccSend_inval:
    
    errno = EINVAL;

    return (ERROR);
    }

/******************************************************************************
*
* motFccTbdFree - Free the transmit buffer descriptors ring.
*
* This routine may only be called after the TX and Rx are disabled.
* It scans the TXBD ring and frees any stay clusters or MBLK chains if in
* non zero copy mode.
* SEE ALSO: motFccRbdFree()
*
* RETURNS: N/A.
*/
LOCAL STATUS motFccTbdFree
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    int     ix;
    M_BLK * pMblkFree;

    for (ix = 0; ix < pDrvCtrl->tbdNum; ix++)
        {
        /* return the cluster headers to the pool */
        pMblkFree = pDrvCtrl->tBufList[ix];
        if (pMblkFree != NULL)
            {
	    netMblkClChainFree (pMblkFree);
            }

        }
    return (OK);
    }

/******************************************************************************
*
* motFccRbdFree - Free the receive buffer descriptor ring.
*
* This routine may only be called after the TX and Rx are disabled.
* It scans the RXBD ring and frees any stay clusters.
* SEE ALSO: motFccTbdFree()
*
* RETURNS: N/A.
*/
LOCAL STATUS motFccRbdFree
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    int ix;
    M_BLK_ID pMblk;
    
    for (ix = 0; ix < pDrvCtrl->rbdNum; ix++)
        {
        /* return the cluster buffers to the pool */
        pMblk = pDrvCtrl->pMblkList[ix];
        if (pMblk != NULL)
            {
             /* free the tuple */
            netTupleFree (pMblk);
            pDrvCtrl->pMblkList[ix] = NULL;
            }
        }
    return (OK);
    }

/******************************************************************************
*
* motFccTbdClean - clean the transmit buffer descriptors ring
*
* This routine may run in the netTask's context, or in the sending
* task. It cleans the transmit buffer descriptor ring. It also calls
* motFccTxErrReInit() for any transmit errors that need a restart to be
* cleared.
*
* This routine must be called with the END device transmit semaphore already
* held (except in polling mode).
*
* SEE ALSO: motFccTxErrReInit()
*
* RETURNS: The new number of free transmit descriptors.
*/

_WRS_FASTTEXT
LOCAL int motFccTbdClean
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    FCC_BD * pTbd;
    M_BLK  * pMblkFree;
    UINT32   statLen;

    while (pDrvCtrl->tbdFree < pDrvCtrl->tbdNum)
        {
        /* get the next bd to be processed */
        pTbd  = pDrvCtrl->pTbdLast;

	statLen = pTbd->bdStatLen;

        /* check if buffer is owned by the CP */
        if (statLen & M8260_FETH_TBDS_R)
            break;

#ifdef TBD_ZERO_BDADDR
	/* Can we avoid this test, and use another method to tell
	 * when we're done cleaning?
	 */

        if (pTbd->bdAddr == (UINT32)NULL)
            break;
#endif
        /* get the Mblk chain for that BD */

        pMblkFree = pDrvCtrl->tBufList [pTbd - pDrvCtrl->pTbdBase];

        /* Free the Mblk Chain */
        if (pMblkFree != NULL)
            {
	    netMblkClChainFree (pMblkFree);

            pDrvCtrl->tBufList[pTbd - pDrvCtrl->pTbdBase] = NULL;
            }

#ifdef TBD_ZERO_BDADDR
        pTbd->bdAddr = (UINT32) NULL; /* can we get rid of this? */
#endif

       /* use wrap bit to determine the end of the bd's */

        if (statLen & M8260_FETH_TBDS_W)
            pDrvCtrl->pTbdLast = pDrvCtrl->pTbdBase;
        else
            pDrvCtrl->pTbdLast = pTbd + 1;

        /* Inc the free count  */
        pDrvCtrl->tbdFree++;

        /* check for error conditions */
        if (statLen &
             ( M8260_FETH_TBDS_CSL |   /* carrier sense lost */
               M8260_FETH_TBDS_UN  |   /* under run */
               M8260_FETH_TBDS_HB  |   /* heartbeat */
               M8260_FETH_TBDS_LC  |   /* Late collision */
               M8260_FETH_TBDS_RL  |   /* Retransmission Limit reached */
               M8260_FETH_TBDS_DEF ))  /* defer indication */
            {

            /* Process mib statistics */
            endM2Packet(&pDrvCtrl->endObj, NULL, M2_PACKET_OUT_ERROR);

#ifdef MOT_FCC_STAT_MONITOR
            if (statLen & M8260_FETH_TBDS_CSL)
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxCslErr);
            
#endif
            if (statLen & M8260_FETH_TBDS_UN)
                {
                motFccTxErrReInit(pDrvCtrl, statLen);
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxUrErr);
                return(pDrvCtrl->tbdFree);
                }
            if (statLen & M8260_FETH_TBDS_HB)
                {
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccHbFailErr);
                if (pDrvCtrl->hbFailFunc != NULL)
                    (*pDrvCtrl->hbFailFunc)(pDrvCtrl);
                }

#ifdef MOT_FCC_STAT_MONITOR
            if (statLen & M8260_FETH_TBDS_LC)
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxLcErr);
            
            if (statLen & M8260_FETH_TBDS_RL)
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxRlErr);
            
            if (statLen & M8260_FETH_TBDS_DEF)
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxDefErr);

#endif /* MOT_FCC_STAT_MONITOR */

            if (statLen & (M8260_FETH_TBDS_RL|M8260_FETH_TBDS_LC))
                motFccTxErrReInit(pDrvCtrl, statLen);
            }
        }

    return (pDrvCtrl->tbdFree);
    }


/**************************************************************************
*
* motFccTbdInit - initialize the transmit buffer ring
*
* This routine initializes the transmit buffer descriptors ring.
*
* SEE ALSO: motFccRbdInit()
*
* RETURNS: OK, or ERROR.
*/
LOCAL STATUS motFccTbdInit
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    UINT32  ix;     /* a counter */
    FCC_BD *pTbd;   /* generic TBD pointer */

    /* carve up the shared-memory region */
    pTbd = (FCC_BD *) pDrvCtrl->pBdBase;

    pDrvCtrl->pTbdBase = pTbd;
    pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
    pDrvCtrl->pTbdLast = pDrvCtrl->pTbdBase;
    pDrvCtrl->tbdFree  = pDrvCtrl->tbdNum;

    for ( ix = 0; ix < pDrvCtrl->tbdNum; ix++ )
        {
        pDrvCtrl->tBufList[ix] = NULL;
        pTbd->bdStatLen = 0;
        pTbd->bdAddr = 0;
        ++pTbd;
        }

    /* set the wrap bit in last BD */
    --pTbd;
    pTbd->bdStatLen = M8260_FETH_TBDS_W;

    /* set stall variables to default values */
    pDrvCtrl->txStall     = FALSE;

    /*
     * Wait for 50% available before restart MUX. Note that
     * since the maximum number of descriptors we need for a send
     * is tbdNum / 2, if we have more TBDs free than this we are
     * sure we can send at least one packet.
     */

    pDrvCtrl->txUnStallThresh = (UINT16)
	(pDrvCtrl->tbdNum - (pDrvCtrl->tbdNum >> 1));

    /* get a cluster buffer from the pool */

    if ((pDrvCtrl->pTxPollMblk = netTupleGet (pDrvCtrl->endObj.pNetPool,
				 MOT_FCC_TX_CL_SZ,
                                 M_DONTWAIT, MT_DATA, TRUE)) == NULL)
        {
        pDrvCtrl->lastError.errCode = END_ERR_NO_BUF;
        muxError(&pDrvCtrl->endObj, &pDrvCtrl->lastError);

        return ERROR;
        }

    pDrvCtrl->pTxPollBuf = (UCHAR *)pDrvCtrl->pTxPollMblk->mBlkHdr.mData;

    return OK;
    }

/******************************************************************************
*
* motFccHandleRXFrames - task-level receive frames processor
*
* This routine is run in netTask's context.
* Returns M8260_FEM_ETH_RXF if we left knowing there were more RBDs ready
* to process (because we reached the packet limit or had a tuple allocation
* failure). Returns 0 if we left because we saw an empty tuple.
*
* RETURNS: M8260_FEM_ETH_RXF or 0
*/

_WRS_FASTTEXT
LOCAL UINT32 motFccHandleRXFrames
    (
    DRV_CTRL *pDrvCtrl,
    int * pMaxRxFrames
    )
    {
    FCC_BD * pRbd;      /* pointer to current receive bd */
    M_BLK_ID pMblk;     /* MBLK to send upstream */
    M_BLK_ID pNewMblk;  /* MBLK to send upstream */
    UINT32   rbdStatLen;
    UINT32   rbdLength;
    int      loopCounter = *pMaxRxFrames;

    MOT_FCC_LOG (MOT_FCC_DBG_TRACE_RX, "motFccHandleRXFrames <ENTER>\n",
		 1,2,3,4,5,6);

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXFHandlerEntries);

    while (TRUE)
        {
	/* get the current rx pointer */
        pRbd = pDrvCtrl->pRbdNext;

        /* Break if BD is still owned by CPM */

        rbdStatLen = pRbd->bdStatLen;

        if (rbdStatLen & M8260_FETH_RBDS_E)
            break;

	if (-- loopCounter < 0)
	    break;

#ifdef MOT_FCC_CPM_21_ERRATA
	/*
	 * CPM-21: False indication of collision in Fast Ethernet.
	 * Devices: MPC8260, MPC8255.
	 * In the Fast Ethernet a false COL will be reported whenever a
	 * collision occurs on the preamble of the previous frame.
	 * Workaround: Software should ignore COL indications when the
	 * CRC of the frame is correct.
	 */

        if (( rbdStatLen & M8260_FETH_RBDS_CL ) != 0)
            {
            MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXFHandlerFramesCollisions);

	    if ((rbdStatLen & M8260_FETH_RBDS_CR) == 0)
		{
		rbdStatLen &= ~M8260_FETH_RBDS_CL;
		}
            MOT_FCC_LOG (MOT_FCC_DBG_RX,
			 "motFccHandleRXFrames collisions Error %x\n",
			 rbdStatLen,2,3,4,5,6);
            }

#endif /* MOT_FCC_CPM_21_ERRATA */

	rbdLength = rbdStatLen & 0xffff;

	if ((rbdStatLen & MOT_FCC_RBDS_ERR) == 0 && rbdLength != 0)
	    {

	    pMblk = pDrvCtrl->pMblkList[pRbd - pDrvCtrl->pRbdBase];

	    if ((pNewMblk = netTupleGet (pDrvCtrl->endObj.pNetPool,
					 MOT_FCC_RX_CL_SZ, 
					 M_DONTWAIT, 
					 MT_DATA, TRUE)) == NULL)
		goto noTuple;

	    /* Pre-invalidate the replacement cluster */

	    MOT_FCC_CACHE_INVAL (pNewMblk->mBlkHdr.mData, MOT_FCC_RX_CL_SZ);

	    /* load the replacement M_BLK for this BD */
	    pDrvCtrl->pMblkList[pRbd - pDrvCtrl->pRbdBase] = pNewMblk;

	    pRbd->bdAddr = (unsigned long)pNewMblk->mBlkHdr.mData;

	    rbdLength -= MII_CRC_LEN;

	    /* adjust length */
	    pMblk->mBlkHdr.mLen    = rbdLength;
	    pMblk->mBlkPktHdr.len  = rbdLength;

	    /* set the packet header flag */
	    pMblk->mBlkHdr.mFlags |= M_PKTHDR;

	    /*
	     * update mib octets.
	     * XXX - we know from the BD status bits whether it's broadcast or
	     * multicast or unicast! Also, calling a function to do the
	     * increments is disagreeable.
	     */
	    endM2Packet(&pDrvCtrl->endObj, pMblk, M2_PACKET_IN);

	    /*
	     * Close the bds, pass ownership to the CP, and update
	     * pDrvCtrl->pRbdNext before calling the receive routine.
	     * (We may enter polling mode.)
	     */

	    if (rbdStatLen & M8260_FETH_RBDS_W)
		{
		pRbd->bdStatLen = (M8260_FETH_RBDS_W |
				   M8260_FETH_RBDS_E |
				   M8260_FETH_RBDS_I);
		pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
		}
	    else
		{
		pRbd->bdStatLen = M8260_FETH_RBDS_E | M8260_FETH_RBDS_I;
		pDrvCtrl->pRbdNext = pRbd + 1;
		}

	    /* send the frame to the upper layer */
	    END_RCV_RTN_CALL (&pDrvCtrl->endObj, pMblk);

	    continue;
	    }

	/* Otherwise, an error occurred. */

	/* do not process the packet. Keep the current cluster */
	MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXFHandlerFramesRejected);

	MOT_FCC_LOG (MOT_FCC_DBG_RX,
		     "motFccHandleRXFrames Error %x\n",
		     rbdStatLen,2,3,4,5,6);

	endM2Packet (&pDrvCtrl->endObj, NULL, M2_PACKET_IN_ERROR);

	/* close the bds, pass ownership to the CP */

	if (rbdStatLen & M8260_FETH_RBDS_W)
	    {
	    pRbd->bdStatLen = (M8260_FETH_RBDS_W |
			       M8260_FETH_RBDS_E |
			       M8260_FETH_RBDS_I);
	    pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
	    }
	else
	    {
	    pRbd->bdStatLen = M8260_FETH_RBDS_E | M8260_FETH_RBDS_I;
	    pDrvCtrl->pRbdNext = pRbd + 1;
	    }

	continue;

noTuple:
	/*
	 * TODO: we should moderate our calls to muxError()/END_ERR_NO_BUF
	 * somehow, as these are expensive, and repeated such calls are
	 * not much use.
	 */

	pDrvCtrl->lastError.errCode = END_ERR_NO_BUF;
	muxError(&pDrvCtrl->endObj, &pDrvCtrl->lastError);

	/* Try again to allocate another M_BLK? */

	MOT_FCC_LOG ( MOT_FCC_DBG_RX, "Alloc Zero Cluster\n", 0,0,0,0,0,0);
	MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXFHandlerNetBufAllocErrors);

#if 1
	/*
	 * Why skip this packet? Why not come back to it later when we get some
	 * tuples? For now we skip it to avoid the possibility of repeatedly
	 * rescheduling ourselves, preventing applications from getting enough
	 * CPU to free RX tuples. This way we can hope for a lull in the
	 * traffic.
	 */

	if (rbdStatLen & M8260_FETH_RBDS_W)
	    {
	    pRbd->bdStatLen = (M8260_FETH_RBDS_W |
			       M8260_FETH_RBDS_E |
			       M8260_FETH_RBDS_I);
	    pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
	    }
	else
	    {
	    pRbd->bdStatLen = M8260_FETH_RBDS_E | M8260_FETH_RBDS_I;
	    pDrvCtrl->pRbdNext = pRbd + 1;
	    }

	/* Check status of next descriptor */

	rbdStatLen = pDrvCtrl->pRbdNext->bdStatLen;
#endif

	break;

        } /* end while */

    MOT_FCC_LOG (MOT_FCC_DBG_TRACE_RX, "motFccHandleRXFrames <EXIT>\n",
                    1,2,3,4,5,6);

    *pMaxRxFrames = loopCounter;
    return (rbdStatLen & M8260_FETH_RBDS_E) ? 0 : M8260_FEM_ETH_RXF;
    }


/**************************************************************************
*
* motFccRbdInit - initialize the receive buffer ring
*
* This routine initializes the receive buffer descriptors ring.
*
* SEE ALSO: motFccTbdInit()
*
* RETURNS: OK, or ERROR if not enough clusters were available.
*/
LOCAL STATUS motFccRbdInit
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    FCC_BD  *pRbd;      /* generic rbd pointer */
    UINT32  ix;         /* a counter */

    /* locate the receive ring after the transmit ring */

    pDrvCtrl->pRbdBase = (FCC_BD *) (pDrvCtrl->pBdBase +
				     (MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum) );

    /* point allocation bd pointers to the first bd */

    pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;

    /* clear out the ring */
    pRbd = pDrvCtrl->pRbdBase;

    for (ix = 0; ix < pDrvCtrl->rbdNum; ix++)
        {
        if ((pDrvCtrl->pMblkList[ix] = netTupleGet(pDrvCtrl->endObj.pNetPool,
                                                  MOT_FCC_RX_CL_SZ,
                                                  M_DONTWAIT, MT_DATA, TRUE))
           == NULL)
            {
            pDrvCtrl->lastError.errCode = END_ERR_NO_BUF;
            muxError(&pDrvCtrl->endObj, &pDrvCtrl->lastError);

            return (ERROR);
            }

	/* Pre-invalidate clusters before giving them to the MAC */

	MOT_FCC_CACHE_INVAL (pDrvCtrl->pMblkList[ix]->mBlkHdr.mData,
			     MOT_FCC_RX_CL_SZ);
	
        pRbd->bdAddr = (UINT32)pDrvCtrl->pMblkList[ix]->mBlkHdr.mData;
	pRbd->bdStatLen = M8260_FETH_RBDS_E | M8260_FETH_RBDS_I;

        /* inc pointer to the next bd */
        ++pRbd;
        }

    /* set the last bd to wrap */
    --pRbd;
    pRbd->bdStatLen |= M8260_FETH_RBDS_W;

    /* initially replenish all */
    pDrvCtrl->rbdCnt = 0;

    return (OK);
    }

/**************************************************************************
*
* motFccPramInit - initialize the parameter RAM for this FCC
*
* This routine programs the parameter ram registers for this FCC and set it
* to work for Ethernet operation. It follows the initialization sequence
* recommended in the MPC8260 PowerQuicc II User's Manual.
*
* SEE ALSO: motFccIramInit ().
*
* RETURNS: OK, always.
*/
LOCAL STATUS motFccPramInit
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    UINT32            fcrVal;
    FCC_PARAM_T     * pParam;
    FCC_ETH_PARAM_T * pEthPar;
    char            * phyAddr;
    STATUS            retVal;

    MOT_FCC_LOG ( MOT_FCC_DBG_START,
             "motFccPramInit\n", 1, 2, 3, 4, 5, 6);

    /* get to the beginning of the parameter area */
    pParam  = pDrvCtrl->fccPar;
    pEthPar = pDrvCtrl->fccEthPar;
    memset(pParam,0,sizeof(*pParam));

    /* figure out the value for the FCR registers */
    fcrVal = MOT_FCC_FCR_DEF_VAL;

    if (MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_BD_LBUS))
        fcrVal |= M8260_FCR_BDB;

    if (MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_BUF_LBUS))
        fcrVal |= M8260_FCR_DTB;

    if (MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_SNOOP_ENABLE))
	fcrVal |= M8260_FCR_GBL;

    fcrVal <<= M8260_FCR_SHIFT;

    pParam->riptr = (VUSHORT)((UINT32)0xffff & (UINT32) pDrvCtrl->riPtr);
    pParam->tiptr = (VUSHORT)((UINT32)0xffff & (UINT32) pDrvCtrl->tiPtr);

    /* clear pad area to zero */

    memset (pDrvCtrl->padPtr, 0, 32);
    pEthPar->pad_ptr = (VUSHORT)((UINT32)0xffff & (UINT32) pDrvCtrl->padPtr);

    /* The MRBLR must be a multiple of 32. */

    pParam->mrblr   = MOT_FCC_MAX_CL_LEN;
    pParam->rstate  = fcrVal;
    pParam->rbase   = (unsigned long)pDrvCtrl->pRbdBase;
    pParam->tstate  = fcrVal;
    pParam->tbase   = (unsigned long)pDrvCtrl->pTbdBase;

    /* These need to be re-initialized if driver was re-loaded */

    pParam->tdptr     =  pParam->tbase;
    pParam->rdptr     =  pParam->rbase;

    pEthPar->c_mask  = MOT_FCC_C_MASK_VAL;
    pEthPar->c_pres  = MOT_FCC_C_PRES_VAL;
    pEthPar->ret_lim = MOT_FCC_RET_LIM_VAL;

    pEthPar->maxflr = MII_ETH_MAX_PCK_SZ;
    pEthPar->minflr = MOT_FCC_MINFLR_VAL;
    pEthPar->maxd1  = MOT_FCC_MAXD_VAL;
    pEthPar->maxd2  = MOT_FCC_MAXD_VAL;

    /* program the individual physical address */

    /* get the physical address */

    phyAddr = (char *)END_HADDR (pDrvCtrl->endObj);
    retVal = motFccAddrSet (pDrvCtrl, (UCHAR *)phyAddr, M8260_FCC_PADDR_H_OFF);

    CACHE_PIPE_FLUSH (); /* Flush the write pipe */

    return( retVal );
    }

/**************************************************************************
*
* motFccIramInit - initialize the internal RAM for this FCC
*
* This routine programs the internal ram registers for this FCC and set it
* to work for Ethernet operation. It follows the initialization sequence
* recommended in the MPC8260 PowerQuicc II User's Manual. It also issues
* a rx/tx init command to the CP.
*
* SEE ALSO: motFccPramInit ()
*
* RETURNS: OK, always.
*/
LOCAL STATUS motFccIramInit
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    FCC_REG_T * reg = pDrvCtrl->fccReg;
    UINT8       command;		/* command to issue to the CP */

    MOT_FCC_LOG ( MOT_FCC_DBG_START, "motFccIramInit\n", 1, 2, 3, 4, 5, 6);

    /*
     * program the GSMR, but do not enable the transmitter
     * or the receiver for now.
     */

#ifdef INCLUDE_GFMR_TCI
    reg->fcc_gfmr = (M8260_GFMR_ETHERNET | M8260_GFMR_NORM |
                     M8260_GFMR_RENC_NRZ | M8260_GFMR_TCI);
#else
    /* Leave TCI low to correct problem in 100Base-TX mode */
    reg->fcc_gfmr = (M8260_GFMR_ETHERNET | M8260_GFMR_NORM |
                     M8260_GFMR_RENC_NRZ);
#endif

    /* program the DSR */
    reg->fcc_dsr = MOT_FCC_DSR_VAL;

    /* zero-out the TODR for the sake of clarity */
    reg->fcc_todr = MOT_FCC_CLEAR_VAL;

    /* the value for the FPSMR */
    if (motFccFpsmrValSet (pDrvCtrl) != OK)
        return ERROR;

    /*
     * We enable the following event interrupts:
     * tx error;
     * rx frame;
     * busy condition;
     * tx buffer, although we'll only enable the last TBD in a frame
     * to generate interrupts. See motFccSend ();
     */

    pDrvCtrl->intMask = ( M8260_FEM_ETH_TXE |
			  M8260_FEM_ETH_TXB |
                          M8260_FEM_ETH_RXF |
			  M8260_FEM_ETH_BSY );

    reg->fcc_fccm = (UINT16) pDrvCtrl->intMask;

    /* clear all events */
    reg->fcc_fcce = MOT_FCC_FCCE_VAL;

    /* issue a init rx/tx command to the CP */
    command = M8260_CPCR_RT_INIT;

    return ( motFccCpcrCommand (pDrvCtrl, command) );
    }

/**************************************************************************
*
* motFccAddrSet - set an address to some internal register
*
* This routine writes the address pointed to by <pAddr> to some FCC's internal
* register. It may be used to set the individual physical address, or
* an address to be added to the hash table filter. It assumes that <offset>
* is the offset in the parameter RAM to the highest-order halfword that
* contains the address.
*
* RETURNS: OK, always.
*/

LOCAL STATUS motFccAddrSet
    (
    DRV_CTRL * pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UCHAR    * pAddr,      /* address to write to register */
    UINT32     offset      /* offset to highest-word in register */
    )
    {
    UINT16  word;
    UINT32  fccPramAddr;
    
    /* get to the beginning of the parameter area */
    fccPramAddr = pDrvCtrl->fccPramAddr;

    word = (UINT16)((pAddr[5] << 8) | pAddr [4]);
    MOT_FCC_REG_WORD_WR (fccPramAddr + offset, word);

    word = (UINT16)((pAddr[3] << 8) | pAddr[2]);
    MOT_FCC_REG_WORD_WR (fccPramAddr + offset + 2, word);

    word = (UINT16)((pAddr[1] << 8) | pAddr [0]);
    MOT_FCC_REG_WORD_WR (fccPramAddr + offset + 4, word);

    return (OK);
    }

/**************************************************************************
*
* motFccFpsmrValSet - set the proper value for the FPSMR
*
* This routine finds out the proper value to be programmed in the
* FPSMR register by looking at some of the user/driver flags. It deals
* with options like promiscuous mode, full duplex, loopback mode, RMON
* mode, and heartbeat control. It then writes to the FPSMR.
*
* SEE ALSO: motFccIramInit (), motFccPramInit ()
*
* RETURNS: OK, always.
*/

LOCAL STATUS motFccFpsmrValSet
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    UINT32  fpsmrVal = 0;

    /* use the standard CCITT CRC */
    fpsmrVal |= M8260_FPSMR_ETH_CRC_32;

    /* set the heartbeat check if the user wants it */
    if ( MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_HBC) )
        fpsmrVal |= M8260_FPSMR_ETH_HBC;
    else
        fpsmrVal &= ~M8260_FPSMR_ETH_HBC;

    /* set RMON mode if the user requests it */
    if ( MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_RMON) )
        fpsmrVal |= M8260_FPSMR_ETH_MON;
    else
        fpsmrVal &= ~M8260_FPSMR_ETH_MON;

    /* deal with promiscuous mode */
    if ( MOT_FCC_FLAG_ISSET (MOT_FCC_PROM) )
        fpsmrVal |= M8260_FPSMR_ETH_PRO;
    else
        fpsmrVal &= ~M8260_FPSMR_ETH_PRO;

    /* 
     * Handle the user's diddling with LPB first. That way, if we 
     * must set LPB due to FD it will stick.
     */

    /* if the user wishes to go in external loopback mode,
     * enable it. Do not enable internal loopback mode.
     */
    if ( MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_LOOP) )
        fpsmrVal |= M8260_FPSMR_ETH_LPB;
    else
        fpsmrVal &= ~M8260_FPSMR_ETH_LPB;

    /* enable full duplex mode if the PHY was configured to work in full duplex mode. */

    if ( pDrvCtrl->phyInfo->phyFlags & MII_PHY_FD )
        fpsmrVal |= (M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);
    else
        fpsmrVal &= ~M8260_FPSMR_ETH_FDE;

    pDrvCtrl->fccReg->fcc_psmr = fpsmrVal;

    return OK;
    }

/*******************************************************************************
* motFccIoctl - interface ioctl procedure
*
* Process an interface ioctl request.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS motFccIoctl
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int         cmd,            /* command to process */
    caddr_t     data            /* pointer to data */
    )
    {
    int         error = OK;
    INT8        savedFlags;
    long        value;
    END_OBJ *   pEndObj;

    if (pDrvCtrl == NULL)
	return ERROR;

    pEndObj = &pDrvCtrl->endObj;

    MOT_FCC_LOG (MOT_FCC_DBG_IOCTL,
         ("Ioctl unit=0x%x cmd=%d data=0x%x\n"),
         pDrvCtrl->unit, cmd, (int)data, 0, 0, 0);

    switch ((UINT32) cmd)
        {
        case EIOCSADDR:
            if (data == NULL)
                error = EINVAL;
            else
                {
                /* Copy and install the new address */

                bcopy ((char *) data,
                       (char *) END_HADDR(pDrvCtrl->endObj),
                        END_HADDR_LEN (pDrvCtrl->endObj) );

                /* stop the FCC */

                if ( motFccStop (pDrvCtrl) != OK)
                    return(ERROR);

                /* program the individual enet address */

                if ( motFccAddrSet (pDrvCtrl, (UCHAR *) data,
                                   M8260_FCC_PADDR_H_OFF)
                    != OK)
                    return(ERROR);

                /* restart the FCC */

                if ( motFccStart (pDrvCtrl) != OK)
                    return(ERROR);

                /*
                * A Start clears the GADDR and IADDR registers.
                * The hash table must be repopulated.
                */
                motFccHashTblPopulate (pDrvCtrl);
                }

            break;

        case EIOCGADDR:
            if (data == NULL)
                error = EINVAL;
            else
                bcopy ( (char *) END_HADDR (pDrvCtrl->endObj),
                        (char *) data,
                        END_HADDR_LEN (pDrvCtrl->endObj) );

            break;

        case EIOCSFLAGS:
            value = (long) data;
            if (value < 0)
                {
                value = -value;
                value--;
                END_FLAGS_CLR (pEndObj, value);
                }
            else
                END_FLAGS_SET (pEndObj, value);


            /* handle IFF_PROMISC */

            savedFlags = MOT_FCC_FLAG_GET();
            if (END_FLAGS_ISSET (IFF_PROMISC))
                {
                MOT_FCC_FLAG_SET (MOT_FCC_PROM);

                if ((MOT_FCC_FLAG_GET () != savedFlags) &&
                    (END_FLAGS_GET (pEndObj) & IFF_UP))
                    {
                    /* config down */

                    END_FLAGS_CLR (pEndObj, IFF_UP | IFF_RUNNING);

                    /* program the FPSMR to promiscuous mode */

                    if (motFccFpsmrValSet (pDrvCtrl) == ERROR)
                        return(ERROR);

                    /* config up */

                    END_FLAGS_SET (pEndObj, IFF_UP | IFF_RUNNING);
                    }
                }
            else
                MOT_FCC_FLAG_CLEAR (MOT_FCC_PROM);

            /* handle IFF_ALLMULTI */

            motFccHashTblPopulate (pDrvCtrl);

            break;

        case EIOCGFLAGS:
            if (data == NULL)
                error = EINVAL;
            else
                *(long *)data = END_FLAGS_GET(pEndObj);

            break;

        case EIOCMULTIADD:
            error = motFccMCastAddrAdd (pDrvCtrl, (UCHAR *) data);
            break;

        case EIOCMULTIDEL:
            error = motFccMCastAddrDel (pDrvCtrl, (UCHAR *) data);
            break;

        case EIOCMULTIGET:
            error = motFccMCastAddrGet (pDrvCtrl, (MULTI_TABLE *) data);
            break;

        case EIOCPOLLSTART:
            motFccPollStart (pDrvCtrl);
            break;

        case EIOCPOLLSTOP:
            motFccPollStop (pDrvCtrl);
            break;


        /* New Generic MIB interface */

        case EIOCGMIB2233:
        case EIOCGMIB2:

            error = endM2Ioctl (&pDrvCtrl->endObj, cmd, data);

            break;

        default:
            error = EINVAL;
        }

    return (error);
    }

/**************************************************************************
*
* motFccCpcrCommand - issue a command to the CP
*
* This routine issues the command specified in <command> to the Communication
* Processor (CP).
*
* RETURNS: OK, or ERROR if the command was unsuccessful.
*
*/
LOCAL STATUS motFccCpcrCommand
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UINT8       command     /* the command being issued */
    )
    {
    UINT32  immrVal;                    /* holder for the immr value */
    volatile  UINT32 cpcrVal = 0;       /* a convenience */
    UINT32  ix = 0;                     /* a counter */
    UINT8   fccNum = pDrvCtrl->fccNum;  /* holder for the fcc number */
    UINT8   fcc = fccNum - 1;           /* a convenience */

    immrVal = pDrvCtrl->immrVal;

    /* wait until the CP is clear */
    do
        {
        MOT_FCC_REG_LONG_RD (M8260_CPCR (immrVal), cpcrVal);

        if (ix++ == M8260_CPCR_LATENCY)
            break;
        }

    while (cpcrVal & M8260_CPCR_FLG) ;

    if (ix >= M8260_CPCR_LATENCY)
        {
        return(ERROR);
        }

    /* issue the command to the CP */

    cpcrVal = (M8260_CPCR_OP (command)
               | M8260_CPCR_SBC (M8260_CPCR_SBC_FCC1 | (fcc * 0x1))
               | M8260_CPCR_PAGE (M8260_CPCR_PAGE_FCC1 | (fcc * 0x1))
               | M8260_CPCR_MCN (M8260_CPCR_MCN_ETH)
               | M8260_CPCR_FLG);

    MOT_FCC_REG_LONG_WR (M8260_CPCR (immrVal),
                         cpcrVal);

    /* Flush the write pipe */

    CACHE_PIPE_FLUSH ();

    /* wait until the CP is clear */
    ix = 0;

    do
        {
        MOT_FCC_REG_LONG_RD (M8260_CPCR (immrVal), cpcrVal);

        if (ix++ == M8260_CPCR_LATENCY)
            break;
        }

    while (cpcrVal & M8260_CPCR_FLG) ;

    if (ix >= M8260_CPCR_LATENCY)
        {
        return(ERROR);
        }

    return(OK);
    }

/**************************************************************************
*
* motFccMiiRead - read the MII interface
*
* This routine reads the register specified by <phyReg> in the PHY device
* whose address is <phyAddr>. The value read is returned in the location
* pointed to by <miiData>.
*
* SEE ALSO: motFccMiiWrite()
*
* RETURNS: OK, or ERROR.
*
*/
LOCAL STATUS motFccMiiRead
    (
    DRV_CTRL * pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UINT8      phyAddr,    /* the PHY being read */
    UINT8      regAddr,    /* the PHY's register being read */
    UINT16   * miiData     /* value read from the MII interface */
    )
    {
    UINT16  temp = 0;   /* temporary variable */

    /* write the preamble pattern first */

    MOT_FCC_MII_WR (MII_MF_PREAMBLE, MII_MF_PREAMBLE_LEN);

    /* write the start of frame */

    MOT_FCC_MII_WR (MII_MF_ST, MII_MF_ST_LEN);

    /* write the operation code */

    MOT_FCC_MII_WR (MII_MF_OP_RD, MII_MF_OP_LEN);

    /* write the PHY address */

    MOT_FCC_MII_WR (phyAddr, MII_MF_ADDR_LEN);

    /* write the PHY register */

    MOT_FCC_MII_WR (regAddr, MII_MF_REG_LEN);

    /* write the turnaround pattern */

    SYS_FCC_MII_BIT_WR ((INT32) NONE);

    SYS_FCC_MII_BIT_WR (0);

    /* read the data on the MDIO */

    MOT_FCC_MII_RD (temp, MII_MF_DATA_LEN);

    /* leave it in idle state */

    SYS_FCC_MII_BIT_WR ((INT32) NONE);

    *miiData = temp;

    MOT_FCC_LOG ( MOT_FCC_DBG_MII,
                 "motFccMiiRead phy %d %d=0x%04x\n", phyAddr, regAddr, temp, 4, 5, 6);

    return (OK);
    }

/**************************************************************************
*
* motFccMiiWrite - write to the MII register
*
* This routine writes the register specified by <phyReg> in the PHY device,
* whose address is <phyAddr>, with the 16-bit value included in <miiData>.
*
* SEE ALSO: motFccMiiRead()
*
* RETURNS: OK, or ERROR.
*/
LOCAL STATUS motFccMiiWrite
    (
    DRV_CTRL * pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UINT8      phyAddr,    /* the PHY being written */
    UINT8      regAddr,    /* the PHY's register being written */
    UINT16     miiData     /* value written to the MII interface */
    )
    {
    MOT_FCC_LOG ( MOT_FCC_DBG_MII,
                 "motFccMiiWrite phy %d %d=0x%04x\n", phyAddr, regAddr, miiData, 4, 5, 6);

    /* write the preamble pattern first */

    MOT_FCC_MII_WR (MII_MF_PREAMBLE, MII_MF_PREAMBLE_LEN);

    /* write the start of frame */

    MOT_FCC_MII_WR (MII_MF_ST, MII_MF_ST_LEN);

    /* write the operation code */

    MOT_FCC_MII_WR (MII_MF_OP_WR, MII_MF_OP_LEN);

    /* write the PHY address */

    MOT_FCC_MII_WR (phyAddr, MII_MF_ADDR_LEN);

    /* write the PHY register */

    MOT_FCC_MII_WR (regAddr, MII_MF_REG_LEN);

    /* write the turnaround pattern */

    SYS_FCC_MII_BIT_WR (1);

    SYS_FCC_MII_BIT_WR (0);

    /* write the data on the MDIO */

    MOT_FCC_MII_WR ((UINT32)miiData, MII_MF_DATA_LEN);

    /* leave it in idle state @@@ */

    SYS_FCC_MII_BIT_WR ((INT32) NONE);

    return (OK);
    }


/**************************************************************************
*
* motFccPhyPreInit - initialize some fields in the phy info structure
*
* This routine initializes some fields in the phy info structure,
* for use of the phyInit() routine.
*
* RETURNS: OK, or ERROR if could not obtain memory.
*/

LOCAL STATUS motFccPhyPreInit
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    MOT_FCC_LOG ( MOT_FCC_DBG_START,
                 "motFccPhyPreInit\n", 1, 2, 3, 4, 5, 6);
    
    /* set some default values */

    pDrvCtrl->phyInfo->pDrvCtrl = (void *) pDrvCtrl;

    /* in case of link failure, set a default mode for the PHY */

    pDrvCtrl->phyInfo->phyFlags = MII_PHY_DEF_SET;

    pDrvCtrl->phyInfo->phyWriteRtn    = (FUNCPTR) motFccMiiWrite;
    pDrvCtrl->phyInfo->phyReadRtn     = (FUNCPTR) motFccMiiRead;
    pDrvCtrl->phyInfo->phyDelayRtn    = (FUNCPTR) taskDelay;
    pDrvCtrl->phyInfo->phyLinkDownRtn = (FUNCPTR) motFccHandleLSCJob;
    pDrvCtrl->phyInfo->phyMaxDelay    = sysClkRateGet() * 5;
    pDrvCtrl->phyInfo->phyDelayParm   = 1;

    /* handle some user-to-physical flags */

    if ( !(MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_NO_AN)))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_AUTO;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_AUTO;

    if ( MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_TBL))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_TBL;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_TBL;

    if ( !(MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_NO_100)))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_100;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_100;

    if ( !(MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_NO_FD)))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_FD;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_FD;

    if ( !(MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_NO_10)))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_10;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_10;

    if ( !(MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_NO_HD)))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_HD;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_HD;

    if ( MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_PHY_MON))
        pDrvCtrl->phyInfo->phyFlags |= MII_PHY_MONITOR;
    else
        pDrvCtrl->phyInfo->phyFlags &= ~MII_PHY_MONITOR;

    /* mark it as pre-initialized */

    pDrvCtrl->phyInfo->phyFlags |= MII_PHY_PRE_INIT;

    return(OK);
    }


/*******************************************************************************
*
* motFccHandleLSCJob - task-level link status event processor
*
* This routine is run in netTask's context.
*
* RETURNS: N/A
*/
LOCAL void motFccHandleLSCJob
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    int     intLevel;   /* current intr level */
    UINT16  miiStat;
    int     retVal;
    int     duplex=0, speed=1;  /* for debug only */
    UINT32  fpsmrVal;
#ifdef MOT_FCC_DBG
    int     link;
#endif

    MOT_FCC_LOG (MOT_FCC_DBG_MII, "motFccHandleLSCJob: \n", 0, 0, 0, 0, 0, 0);

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numLSCHandlerEntries);

    /* read MII status register once to unlatch link status bit */

    (void) motFccMiiRead (pDrvCtrl, pDrvCtrl->phyInfo->phyAddr,
			  MII_STAT_REG, &miiStat);

    /* read again to know it's current value */

    (void) motFccMiiRead (pDrvCtrl, pDrvCtrl->phyInfo->phyAddr,
			  MII_STAT_REG, &miiStat);

    if ( miiStat & MII_SR_LINK_STATUS ) /* if link is now up */
        {
#ifdef MOT_FCC_DBG
        link = 1;
#endif
        if (pDrvCtrl->phyDuplexFunc)
            {
            /* get duplex mode from BSP */

            retVal = pDrvCtrl->phyDuplexFunc (pDrvCtrl->phyInfo, &duplex);

            if (retVal == OK)
                {
                intLevel = intLock();

                fpsmrVal = pDrvCtrl->fccReg->fcc_psmr;

                /* TRUE when full duplex mode is active */
                if (duplex)
                    fpsmrVal |=  (M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);
                else
                    fpsmrVal &= ~(M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);

                pDrvCtrl->fccReg->fcc_psmr = fpsmrVal;
                intUnlock (intLevel);
                }
            }

        if (pDrvCtrl->phySpeedFunc)
            {
            /* get duplex mode from BSP */
            pDrvCtrl->phySpeedFunc (pDrvCtrl->phyInfo, &speed);
            }
        }
    else
        {
#ifdef MOT_FCC_DBG
        link = 0;
#endif
        }

    MOT_FCC_LOG (MOT_FCC_DBG_MONITOR,"FCC %d Unit %d Link %s @ %s BPS %s Duplex\n",
           pDrvCtrl->fccNum, pDrvCtrl->unit, (int)linkStr[link],
           (int)speedStr[speed],(int)duplexStr[duplex], 6);

    intLevel = intLock();
    pDrvCtrl->lscHandling = FALSE;
    intUnlock (intLevel);

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numLSCHandlerExits);
    }

/******************************************************************************
*
* motFccPhyLSCInt - Line Status Interrupt. This int handler adds a job when
*                   there is a change in the Phy status.
*
* RETURNS: None
*/

LOCAL void motFccPhyLSCInt
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    MOT_FCC_LOG (MOT_FCC_DBG_INT, "motFccPhyLSCInt\n", 1, 2, 3, 4, 5, 6);

    /* if no link status change job is pending */

    if (!pDrvCtrl->lscHandling)
        {
        /* and the BSP has supplied a duplex mode function */

        if (pDrvCtrl->phyDuplexFunc)
            {
            if (pDrvCtrl->netJobAdd ((FUNCPTR) motFccHandleLSCJob, 
                                     (int) pDrvCtrl, 0, 0, 0, 0)
                 == OK)
                pDrvCtrl->lscHandling = TRUE;
            }
            /*
             * If the BSP didn't supply a FUNC for getting duplex mode
             * there's no reason to schedule the link status change job.
             */
        }
    }

/*******************************************************************************
* motFccMCastAddrAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.
*
* SEE ALSO: motFccMCastAddrDel() motFccMCastAddrGet()
*
* RETURNS: OK or ERROR.
*/
LOCAL STATUS motFccMCastAddrAdd
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UCHAR *      pAddr      /* address to be added */
    )
    {
    int     retVal;     /* convenient holder for return value */

    if (pDrvCtrl == NULL)
        return (ERROR);

    retVal = etherMultiAdd (&pDrvCtrl->endObj.multiList, (char *)pAddr);

    if (retVal == ENETRESET)
        {
        pDrvCtrl->endObj.nMulti++;

        retVal = motFccHashTblPopulate (pDrvCtrl);
        }

    return ((retVal == OK) ? OK : ERROR);
    }

/*****************************************************************************
*
* motFccMCastAddrDel - delete a multicast address for the device
*
* This routine deletes a multicast address from the current list of
* multicast addresses.
*
* SEE ALSO: motFccMCastAddrAdd() motFccMCastAddrGet()
*
* RETURNS: OK or ERROR.
*/
LOCAL STATUS motFccMCastAddrDel
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UCHAR *      pAddr      /* address to be deleted */
    )
    {
    int     retVal;     /* convenient holder for return value */

    if (pDrvCtrl == NULL)
        return (ERROR);

    retVal = etherMultiDel (&pDrvCtrl->endObj.multiList, (char *)pAddr);

    if (retVal == ENETRESET)
        {
        pDrvCtrl->endObj.nMulti--;

        retVal = motFccHashTblPopulate (pDrvCtrl);
        }

    return (retVal);
    }

/*******************************************************************************
* motFccMCastAddrGet - get the current multicast address list
*
* This routine returns the current multicast address list in <pTable>
*
* SEE ALSO: motFccMCastAddrAdd() motFccMCastAddrDel()
*
* RETURNS: OK or ERROR.
*/
LOCAL STATUS motFccMCastAddrGet
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    MULTI_TABLE *pTable     /* table into which to copy addresses */
    )
    {
    return (etherMultiGet (&pDrvCtrl->endObj.multiList, pTable));
    }

/*******************************************************************************
* motFccHashTblAdd - add an entry to the hash table
*
* This routine adds an entry to the hash table for the address pointed to
* by <pAddr>.
*
* RETURNS: OK, or ERROR.
*/
LOCAL STATUS motFccHashTblAdd
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    UCHAR *     pAddr       /* address to be added */
    )
    {
    UINT8   command;        /* command to issue to the CP */
    
    /* program the group enet address */

    if (motFccAddrSet (pDrvCtrl, (UCHAR *) pAddr,M8260_FCC_TADDR_H_OFF) != OK)
        return (ERROR);

    /* issue a set group address command to the CP */

    command = M8260_CPCR_SET_GROUP;
    if (motFccCpcrCommand (pDrvCtrl, command) == ERROR)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
* motFccHashTblPopulate - populate the hash table
*
* This routine populates the hash table with the entries found in the
* multicast table. We have to reset the hash registers first, and
* populate them again, since more than one address may be mapped to
* the same bit.
*
* RETURNS: OK, or ERROR.
*/
LOCAL STATUS motFccHashTblPopulate
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    ETHER_MULTI * mCastNode;

    /*
     * If we're in ALLMULTI mode, just set all the bits in the filter.
     * This will insure we receive multicasts from all groups.
     */

    if (END_FLAGS_ISSET (IFF_ALLMULTI))
        {
        pDrvCtrl->fccEthPar->gaddr_h = 0xFFFFFFFF;
        pDrvCtrl->fccEthPar->gaddr_l = 0xFFFFFFFF;
        return(OK);
        }

    /*
     * Clear the group address registers. This flushes all the
     * multicast addresses without having to reset the whole device.
     */

    pDrvCtrl->fccEthPar->gaddr_h = 0;
    pDrvCtrl->fccEthPar->gaddr_l = 0;

    /* scan the multicast address list */

    for (mCastNode = (ETHER_MULTI *) lstFirst (&pDrvCtrl->endObj.multiList);
        mCastNode != NULL;
        mCastNode  = (ETHER_MULTI *) lstNext (&mCastNode->node))
        {
        /* add this entry */

        if (motFccHashTblAdd (pDrvCtrl, (UCHAR *)mCastNode->addr) == ERROR)
            return(ERROR);
        }

    return(OK);
    }

/*****************************************************************************
*
* motFccPollSend - transmit a packet in polled mode
*
* This routine is called by a user to try and send a packet from the
* device. It sends a packet directly on the network, without having to
* go through the normal process of queuing a packet on an output queue
* and then waiting for the device to decide to transmit it.
*
* These routine should not call any kernel functions.
*
* SEE ALSO: motFccPollReceive()
*
* RETURNS: OK or EAGAIN.
*/

LOCAL STATUS motFccPollSend
    (
    DRV_CTRL *pDrvCtrl,
    M_BLK_ID pMblk
    )
    {
    FCC_BD *pTbd;	    /* pointer to the current ready TBD */
    int     len;            /* length of data to be sent */
    UINT32  bdStatLen;

    if (pDrvCtrl == NULL)
        return (ERROR);

    if (!MOT_FCC_FLAG_ISSET (MOT_FCC_POLLING))
        return(EAGAIN);

    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollSend <ENTER>\n", 1, 2, 3, 4, 5, 6);

    if (!pMblk)
        return EAGAIN;

    motFccTbdClean (pDrvCtrl);

    if (pDrvCtrl->tbdFree)
        {
        /* use the current buffer */
        pTbd = pDrvCtrl->pTbdNext;

	bdStatLen = pTbd->bdStatLen & (M8260_FETH_TBDS_R|M8260_FETH_TBDS_W);

	if (bdStatLen & M8260_FETH_TBDS_R)
	    return EAGAIN;

#ifdef TBD_ZERO_BDADDR
        if ( pTbd->bdAddr != (UINT32) NULL)
            return (EAGAIN);
#endif

        /* process SNMP RFC */
        endM2Packet (&pDrvCtrl->endObj, pMblk, M2_PACKET_OUT);

        /* copy data but do not free the Mblk */
        len = netMblkToBufCopy (pMblk, (char *) pDrvCtrl->pTxPollBuf, NULL);

	if (len < MOT_FCC_MIN_TX_PKT_SZ)
	    len = MOT_FCC_MIN_TX_PKT_SZ;

        /* flush the cache, if necessary */
        MOT_FCC_CACHE_FLUSH (pDrvCtrl->pTxPollBuf, len);

        pDrvCtrl->tBufList[pTbd - pDrvCtrl->pTbdBase] = (M_BLK *) NULL;

        pTbd->bdAddr = (unsigned long) pDrvCtrl->pTxPollBuf;

	/* This preserves the W bit */

	bdStatLen |= (M8260_FETH_TBDS_R |
		      M8260_FETH_TBDS_L |
		      M8260_FETH_TBDS_TC |
		      M8260_FETH_TBDS_PAD | len);

	pTbd->bdStatLen = bdStatLen;

        /* Flush the write pipe */
        CACHE_PIPE_FLUSH ();

        while (pTbd->bdStatLen & M8260_FETH_TBDS_R)
	    {
	    /* Wait. */
	    }

	/* The W bit won't have changed. */

        if (bdStatLen & M8260_FETH_TBDS_W)
            pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
        else
            pDrvCtrl->pTbdNext++;

        pDrvCtrl->tbdFree--;
        }

    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollSend <EXIT>\n", 1, 2, 3, 4, 5, 6);

    return OK;
    }

/*******************************************************************************
* motFccPollReceive - receive a packet in polled mode
*
* This routine is called by a user to try and get a packet from the
* device. It returns EAGAIN if no packet is available. The caller must
* supply a M_BLK_ID with enough space to contain the received packet. If
* enough buffer is not available then EAGAIN is returned.
*
* These routine should not call any kernel functions.
* FCC events and/or ints must be masked and/or disabled before calling
* this function.
*
* SEE ALSO: motFccPollSend(), motFccPollStart(), motFccPollStop().
*
* RETURNS: OK or EAGAIN.
*/

LOCAL STATUS motFccPollReceive
    (
    DRV_CTRL * pDrvCtrl,
    M_BLK    * pMblk
    )
    {
    int      retVal;		/* holder for return value */
    FCC_BD * pRbd;		/* generic rbd pointer */
    UINT32   rbdStatLen;	/* holder for the status of this RBD */
    int	     rbdLen;
    char   * pData;		/* a rx data pointer */

    if (pDrvCtrl == NULL)
        return (ERROR);

    if (!MOT_FCC_FLAG_ISSET (MOT_FCC_POLLING))
        return(EAGAIN);

    if ( (pMblk->mBlkHdr.mFlags & M_EXT) != M_EXT )
        return EAGAIN; /* should never happen */

    /* get the next available RBD */
    pRbd = pDrvCtrl->pRbdNext;

    rbdStatLen = pRbd->bdStatLen;

    /* if it's not ready, don't touch it! */
    if (rbdStatLen & M8260_FETH_RBDS_E)
        return EAGAIN;

    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollReceive <ENTER>\n", 1, 2, 3, 4, 5, 6);


    /*
     *  Handle errata CPM-21... this code should NOT break the driver
     *  when CPM-21 is ultimately fixed.
     *
     *  The recommended workaround is:
     *  "Software should ignore COL indications when the CRC of the frame is
     *  correct."
     */

#ifdef MOT_FCC_CPM_21_ERRATA
    if ((rbdStatLen & (M8260_FETH_RBDS_CL | M8260_FETH_RBDS_CR))
	== M8260_FETH_RBDS_CL)
        {
        rbdStatLen &= ~M8260_FETH_RBDS_CL;
        }
#endif

    /*
     * Pass the packet up only if reception was OK and the
     * buffer is large enough.
     */

    rbdLen = rbdStatLen & 0xffff;
    rbdLen -= MII_CRC_LEN;

    if ((rbdStatLen & MOT_FCC_RBDS_ERR) != 0)
        {
        endM2Packet (&pDrvCtrl->endObj, NULL, M2_PACKET_IN_ERROR);
	retVal = EAGAIN;
        }
    else if (pMblk->mBlkHdr.mLen < rbdLen)   /* should never happen */
	{
	endM2Packet (&pDrvCtrl->endObj, NULL, M2_PACKET_IN_DISCARD);
        retVal = EAGAIN;
	}
    else
        {
        pData = (char*) pRbd->bdAddr;

        /* set up the mBlk properly */
        pMblk->mBlkHdr.mFlags |= M_PKTHDR;
        pMblk->mBlkHdr.mLen    = rbdLen;
        pMblk->mBlkPktHdr.len  = rbdLen;

	/* The cluster was pre-invalidated, we don't need to invalidate here */

        bcopy ((char *) pData, (char *) pMblk->mBlkHdr.mData, rbdLen);

	/* The endM2Packet() call requires the M_BLK to be set up. */

        endM2Packet (&pDrvCtrl->endObj, pMblk, M2_PACKET_IN);

        /* pre-invalidate before giving cluster back to MAC */

        MOT_FCC_CACHE_INVAL ((char *) pData, rbdLen);

        retVal = OK;
        }

    if (rbdStatLen & M8260_FETH_RBDS_W)
	{
	pRbd->bdStatLen = (M8260_FETH_RBDS_W |
			   M8260_FETH_RBDS_E |
			   M8260_FETH_RBDS_I);
	pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
	}
    else
	{
	pRbd->bdStatLen = M8260_FETH_RBDS_E | M8260_FETH_RBDS_I;
	pDrvCtrl->pRbdNext = pRbd + 1;
	}

    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollReceive <EXIT>\n", 1, 2, 3, 4, 5, 6);
    return retVal;
    }

/*******************************************************************************
* motFccPollStart - start polling mode
*
* This routine starts polling mode by disabling ethernet interrupts and
* setting the polling flag in the END_CTRL structure.
*
* SEE ALSO: motFccPollStop()
*
* RETURNS: OK, or ERROR if polling mode could not be started.
*/
LOCAL STATUS motFccPollStart
    (
    DRV_CTRL * pDrvCtrl     /* pointer to DRV_CTRL structure */
    )
    {
    int         intLevel;   /* current intr level */
    FCC_REG_T * reg;

    /*
     * XXX - this routine is called with interrupts locked, so
     * it shouldn't do any logging or make any kernel calls, or call
     * any routine which does!
     */
    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollStart <ENTER>\n", 1, 2, 3, 4, 5, 6);
    
    if (pDrvCtrl == NULL)
        return (ERROR);

#define FCC_POLL_START_TAKE_SEM   /* Bad. */

#if defined (FCC_POLL_START_TAKE_SEM)
    END_TX_SEM_TAKE(&pDrvCtrl->endObj, WAIT_FOREVER);
#endif
    
    if (MOT_FCC_FLAG_ISSET (MOT_FCC_POLLING))
        return ERROR;

    reg = pDrvCtrl->fccReg;

    intLevel = intLock();

    /* save current mask */

    pDrvCtrl->intMask = reg->fcc_fccm;

    /* mask all events */

    reg->fcc_fccm = 0;

    pDrvCtrl->txStall = FALSE;

    intUnlock (intLevel);

    MOT_FCC_FLAG_SET (MOT_FCC_POLLING);
    
    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollStart <EXIT>\n", 1, 2, 3, 4, 5, 6);
#if defined (FCC_POLL_START_TAKE_SEM)
    END_TX_SEM_GIVE (&pDrvCtrl->endObj);
#endif
    return (OK);
    }

/*******************************************************************************
* motFccPollStop - stop polling mode
*
* This routine stops polling mode by enabling ethernet interrupts and
* resetting the polling flag in the END_CTRL structure.
*
* SEE ALSO: motFccPollStart()
*
* RETURNS: OK, or ERROR if polling mode could not be stopped.
*/
 
LOCAL STATUS motFccPollStop
    (
    DRV_CTRL    * pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int         intLevel;
    FCC_REG_T * reg;

    if (pDrvCtrl == NULL)
        return (ERROR);
    
    reg = pDrvCtrl->fccReg;

    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollStop <ENTER>\n", 1, 2, 3, 4, 5, 6);

    if (!MOT_FCC_FLAG_ISSET (MOT_FCC_POLLING))
        return ERROR;

    /* restore the mask register */

    intLevel = intLock();

    reg->fcc_fccm = (UINT16) pDrvCtrl->intMask;

    /* clear any pending events */

    reg->fcc_fcce = 0xffff;

    pDrvCtrl->txStall = FALSE;
    /* leave poll mode */

    MOT_FCC_FLAG_CLEAR (MOT_FCC_POLLING);

    intUnlock (intLevel);

    MOT_FCC_LOG (MOT_FCC_DBG_POLL,
                 "motFccPollStop <EXIT>\n", 1, 2, 3, 4, 5, 6);

    return (OK);
    }

#ifdef MOT_FCC_STAT_MONITOR
FCC_DRIVER_STATS * motFccStatGet
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    return (pDrvCtrl->Stats);
    }
#endif

#ifdef INCLUDE_DUMPS
/******************************************************************************
* motFccDumpRxRing - Show the Receive Ring details
*
* This routine displays the receive ring descriptors.
*
* SEE ALSO: motFccDumpTxRing()
*
* RETURNS: N/A
*/

void motFccDumpRxRing
    (
    int fccNum 
    )
    {
    FCC_BD * pTestRbd;
    DRV_CTRL * pDrvCtrl;
    int i;
    int index;

    pDrvCtrl = (DRV_CTRL *)endFindByName ("motfcc", fccNum);
    if (pDrvCtrl == NULL)
	{
	printf ("Bad unit number.\n");
	return;
	}

    pTestRbd = pDrvCtrl->pRbdNext;

    index    = pTestRbd - pDrvCtrl->pRbdBase;

    printf("motFccDumpRxRing: Current descriptor index %d at %p status/len 0x%x\n",
            index, pTestRbd, pTestRbd->bdStatLen);

    for (i = 0; i < pDrvCtrl->rbdNum; i++)
        {
        pTestRbd = pDrvCtrl->pRbdBase + i;
 
        printf("motFccDumpRxRing: index %d pRxDescriptor %p status/len 0x%x\n", 
               i, pTestRbd, pTestRbd->bdStatLen);
        }
    }

/******************************************************************************
* motFccDumpTxRing - Show the Transmit Ring details
*
* This routine displays the transmit ring descriptors.
*
* SEE ALSO: motFccDumpRxRing()
*
* RETURNS: N/A
*/

void motFccDumpTxRing
    (
    int fccNum
    )
    {
    FCC_BD * pTestTbd;
    DRV_CTRL * pDrvCtrl;
    int i;
    int index;

    pDrvCtrl = (DRV_CTRL *)endFindByName ("motfcc", fccNum);
    if (pDrvCtrl == NULL)
	{
	printf ("Bad unit number.\n");
	return;
	}

    pTestTbd = pDrvCtrl->pTbdNext;

    index = pTestTbd - pDrvCtrl->pTbdBase;

    printf("motFccDumpTxRing: Current descriptor index %d at %p status 0x%x\n",
            index, pTestTbd, pTestTbd->bdStatLen);

    for (i = 0; i < pDrvCtrl->tbdNum; i++)
        {
        pTestTbd = pDrvCtrl->pTbdBase + i;

        printf("motFccDumpTxRing: index %d pTxDescriptor %p status/len 0x%x \n", 
               i, pTestTbd, pTestTbd->bdStatLen);
        }
    }
#endif /* INCLUDE_DUMPS */

#ifdef MOT_FCC_DBG
/******************************************************************************
*
* motFccMiiShow - Debug Function to show the Mii settings in the Phy Info
*                 structure.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccMiiShow
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    UCHAR speed [20];

    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    strcpy ((char *) speed, (pDrvCtrl->phyInfo->phySpeed == MOT_FCC_100MBS) ?
                    "100Mbit/s" : "10Mbit/s");

    MOT_FCC_LOG ( MOT_FCC_DBG_MII,
        ("\n\tphySpeed=%s\n\tphyMode=%s\n\tphyAddr=0x%x"
         "\n\tphyFlags=0x%x\n\tphyDefMode=0x%x\n"),
        (int)&speed[0],
        (int)pDrvCtrl->phyInfo->phyMode,
        pDrvCtrl->phyInfo->phyAddr,
        pDrvCtrl->phyInfo->phyFlags,
        pDrvCtrl->phyInfo->phyDefMode, 6);


   MOT_FCC_LOG ( MOT_FCC_DBG_MII,
       ("\n\tphyStatus=0x%x\n\tphyCtrl=0x%x\n\tphyAds=0x%x"
        "\n\tphyPrtn=0x%x phyExp=0x%x\n\tphyNext=0x%x\n"),
       pDrvCtrl->phyInfo->miiRegs.phyStatus,
       pDrvCtrl->phyInfo->miiRegs.phyCtrl,
       pDrvCtrl->phyInfo->miiRegs.phyAds,
       pDrvCtrl->phyInfo->miiRegs.phyPrtn,
       pDrvCtrl->phyInfo->miiRegs.phyExp,
       pDrvCtrl->phyInfo->miiRegs.phyNext );
    }

/******************************************************************************
*
* motFccMibShow - Debug Function to show MIB statistics.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccMibShow
    (
    DRV_CTRL *  pDrvCtrl
    )
    {
    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    MOT_FCC_LOG (MOT_FCC_DBG_MII,
        "\n\tifOutNUcastPkts=%d\n\tifOutUcastPkts=%d\n \
         \tifInNUcastPkts=%d\n\tifInUcastPkts=%d\n\tifOutErrors=%d\n",
        pDrvCtrl->endObj.mib2Tbl.ifOutNUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifOutUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifInNUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifInUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifOutErrors,6);

    }

/******************************************************************************
*
* motFccShow - Debug Function to show driver specific control data.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccShow
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    int i;
    FCC_BD    * bdPtr;
    FCC_REG_T * reg;    


    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    reg = pDrvCtrl->fccReg;

    MOT_FCC_LOG (MOT_FCC_DBG_TX, "\n\tTransmit stalls: %u\n",
		 pDrvCtrl->txStallCount, 0,0,0,0,0);

    MOT_FCC_LOG (MOT_FCC_DBG_TX,
        "\n\tTxBds:Base 0x%x Total %d Free %d Next %d Last %d ibdPtr 0x%08x\n",
        (int)pDrvCtrl->pTbdBase,
        pDrvCtrl->tbdNum,
        pDrvCtrl->tbdFree,
        (pDrvCtrl->pTbdNext-pDrvCtrl->pTbdBase),
        (pDrvCtrl->pTbdLast-pDrvCtrl->pTbdBase), pDrvCtrl->fccPar->tbptr);

    bdPtr = pDrvCtrl->pTbdBase;
    for ( i = 0; i < pDrvCtrl->tbdNum; i++)
       {
       MOT_FCC_LOG ( MOT_FCC_DBG_TX,
            ("TXBD:%3d Status 0x%04x Length 0x%04x Address 0x%08x Alloc 0x%08x\n"),
            i,
            bdPtr->bdStatLen >> 16,
            bdPtr->bdStatLen & 0xffff,
            bdPtr->bdAddr,
            (int)pDrvCtrl->tBufList[i],6);

       bdPtr++;
       }

    MOT_FCC_LOG (MOT_FCC_DBG_RX,
		 ("\n\tRxBds:Base 0x%x Total %d Full %d Next %d ibdPtr 0x%08x\n"),
		 (int)pDrvCtrl->pRbdBase,
		 pDrvCtrl->rbdNum,
		 pDrvCtrl->rbdCnt,
		 (pDrvCtrl->pRbdNext-pDrvCtrl->pRbdBase),
		 pDrvCtrl->fccPar->rbptr, 0);
    
   bdPtr = pDrvCtrl->pRbdBase;
   for ( i = 0; i < pDrvCtrl->rbdNum; i++)
       {
       MOT_FCC_LOG (MOT_FCC_DBG_RX,
            ("RXBD:%3d Status 0x%04x Length 0x%04x Address 0x%08x Alloc 0x%08x\n"),
            i,
            bdPtr->bdStatLen >> 16,
            bdPtr->bdStatLen & 0xffff,
            bdPtr->bdAddr,
            (int)pDrvCtrl->pMblkList[i],6);

        bdPtr++;
        }

   MOT_FCC_LOG ( MOT_FCC_DBG_INT,
       ("\n\tEvent 0x%04x --- Mask  0x%04x\n"
       "\n\tnumInts:%d"
       "\n\tTXCInts:%d"
       "\n\tRXBInts:%d\n"),
       reg->fcc_fcce,
       reg->fcc_fccm,
       pDrvCtrl->Stats->numInts,
       pDrvCtrl->Stats->numTXCInts,
       pDrvCtrl->Stats->numRXBInts,6);

    MOT_FCC_LOG ( MOT_FCC_DBG_INT,
        ("\n\tTxErrInts:%d"
        "\n\tRxBsyInts:%d"
        "\n\tTXBInts:%d"
        "\n\tRXFInts:%d"
        "\n\tRXCInts:%d\n"),
        pDrvCtrl->Stats->motFccTxErr,
        pDrvCtrl->Stats->numBSYInts,
        pDrvCtrl->Stats->numTXBInts,
        pDrvCtrl->Stats->numRXFInts,
        pDrvCtrl->Stats->numRXCInts,6);


    MOT_FCC_LOG ( MOT_FCC_DBG_INT_TX_ERR,
        ("\n\tTxHbFailErr:%d"
        "\n\tTxLcErr:%d"
        "\n\tTxUrErr:%d"
        "\n\tTxCslErr:%d"
        "\n\tTxRlErr:%d"
        "\n\tTxDefErr:%d\n"),
        pDrvCtrl->Stats->motFccHbFailErr,
        pDrvCtrl->Stats->motFccTxLcErr,
        pDrvCtrl->Stats->motFccTxUrErr,
        pDrvCtrl->Stats->motFccTxCslErr,
        pDrvCtrl->Stats->motFccTxRlErr,
        pDrvCtrl->Stats->motFccTxDefErr );

    MOT_FCC_LOG ( MOT_FCC_DBG_INT_RX_ERR,
        ("\n\tRxHandler:%d"
         "\n\tRxBsyInts:%d"
        "\n\tFramesLong:%d\n"),
        pDrvCtrl->Stats->numRXFHandlerEntries,
        pDrvCtrl->Stats->numBSYInts,
        pDrvCtrl->Stats->numRXFHandlerFramesLong,4,5,6);

    MOT_FCC_LOG ( MOT_FCC_DBG_INT_RX_ERR,
        "\n\tCollisions:%d"
        "\n\tCrcErrors:%d"
        "\n\tRejected:%d"
        "\n\tNetBufAllocErrors:%d"
        "\n\tNetCblkAllocErrors:%d"
        "\n\tNetMblkAllocErrors:%d\n",
        pDrvCtrl->Stats->numRXFHandlerFramesCollisions,
        pDrvCtrl->Stats->numRXFHandlerFramesCrcErrors,
        pDrvCtrl->Stats->numRXFHandlerFramesRejected,
        pDrvCtrl->Stats->numRXFHandlerNetBufAllocErrors,
        pDrvCtrl->Stats->numRXFHandlerNetCblkAllocErrors,
        pDrvCtrl->Stats->numRXFHandlerNetMblkAllocErrors);
    }

/******************************************************************************
*
* motFccIramShow - Debug Function to show FCC CP internal ram parameters.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccIramShow
    (
    DRV_CTRL *  pDrvCtrl
    )
    {
    FCC_REG_T * reg;

    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    reg = pDrvCtrl->fccReg;

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\tGFMR=0x%08x"
                  "\n\tFPSMR=0x%08x"
                  "\n\tDSR=0x%04x"
                  "\n\tTODR=0x%04x"
                  "\n\tFCCM=0x%04x"
                  "\n\tFCCE=0x%04x\n"),
                  reg->fcc_gfmr, reg->fcc_psmr,reg->fcc_dsr,
                  reg->fcc_todr, reg->fcc_fccm, reg->fcc_fcce);

    }

/******************************************************************************
*
* motFccPramShow - Debug Function to show FCC CP parameter ram.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccPramShow
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    FCC_PARAM_T     * pParam;

    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    pParam = pDrvCtrl->fccPar;

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                "FCC Parameter RAM Common\n",0,0,0,0,0,0);

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\triptr=0x%04x"
                  "\n\ttiptr=0x%04x"
                  "\n\trstate=0x%08x"
                  "\n\trbase=0x%08x"
                  "\n\ttstate=0x%08x"
                  "\n\ttbase=0x%08x\n"),
                   pParam->riptr, pParam->tiptr, pParam->rstate,
                   pParam->rbase, pParam->tstate, pParam->rbase);

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\trbdstat=0x%04x"
                  "\n\trbdlen=0x%04x"
                  "\n\trdptr=0x%08x"
                  "\n\ttbdstat=0x%04x"
                  "\n\ttdptr=0x%04x"
                  "\n\ttdptr=0x%08x\n"),
                   pParam->rbdstat, pParam->rbdlen, pParam->rdptr,
                   pParam->tbdstat, pParam->tbdlen, pParam->tdptr);

    MOT_FCC_LOG ( MOT_FCC_DBG_ANY,
                 ("\n\tMRBLR=0x%04x"
                  "\n\tRBPTR=0x%08x pRbdNext=0x%08x"
                  "\n\tTBPTR=0x%08x pTbdLast=0x%08x\n"
                  ),
                  pParam->mrblr,
                  pParam->rbptr, (int)pDrvCtrl->pRbdNext,
                  pParam->tbptr, (int)pDrvCtrl->pTbdLast, 6);
    }

/******************************************************************************
*
* motFccEramShow - Debug Function to show FCC CP ethernet parameter ram.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccEramShow
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    FCC_ETH_PARAM_T * pEthPar;

    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    pEthPar = pDrvCtrl->fccEthPar;

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\tretry limit=0x%04x"
                  "\n\tcrc mask=0x%08x"
                  "\n\tpreset crc=0x%08x"
                  "\n\tcrc error frames=0x%08x"
                  "\n\tmisaligned frames=0x%08x"
                  "\n\tdiscarded frames=0x%08x\n"),
                    pEthPar->ret_lim, pEthPar->c_mask, pEthPar->c_pres,
                    pEthPar->crcec, pEthPar->alec, pEthPar->disfc);

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\tgroup addr filter high=0x%08x"
                  "\n\tgroup addr filter low=0x%08x"
                  "\n\tindividual addr filter high=0x%08x"
                  "\n\tindividual addr filter low=0x%08x\n"),
                    pEthPar->gaddr_h, pEthPar->gaddr_l,
                    pEthPar->iaddr_h, pEthPar->iaddr_l, 0, 0);

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\tindividual addr %02x%02x%02x\n"),
                    pEthPar->paddr_h, pEthPar->paddr_m,
                    pEthPar->paddr_l, 0, 0, 0);

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\tmax frame  length=0x%04x"
                  "\n\tmin frame  length=0x%04x"
                  "\n\tmax DMA1   length=0x%04x"
                  "\n\tmax DMA2   length=0x%04x\n"),
                    pEthPar->maxflr, pEthPar->minflr,
                    pEthPar->maxd1,  pEthPar->maxd2, 0, 0);

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                 ("\n\tcam address pointer=0x%x"
                  "\n\tpersistence=0x%x"
                  "\n\tinternal pad pointer=0x%x\n"),
                    pEthPar->cam_ptr, pEthPar->p_per, pEthPar->pad_ptr,
                    0, 0, 0);

    }

/******************************************************************************
*
* motFccDrvShow - Debug Function to show FCC parameter ram addresses,
*                 initial BD and cluster settings.
*
* This function is only available when MOT_FCC_DBG is defined. It should be used
* for debugging purposes only.
*
* RETURNS: N/A
*/
void motFccDrvShow
    (
    DRV_CTRL * pDrvCtrl
    )
    {
    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    MOT_FCC_LOG ( MOT_FCC_DBG_ANY,
                ("\n\tpDrvCtrl=0x%08x"
                 "\n\tfccNum=%d"
                 "\n\tiramAddr=0x%08x"
                 "\n\tpramAddr=0x%08x\n"),
                (int)pDrvCtrl,
                pDrvCtrl->fccNum,
                pDrvCtrl->fccIramAddr,
                pDrvCtrl->fccPramAddr,
                0,0 );

    MOT_FCC_LOG ( MOT_FCC_DBG_ANY,
                ("\n\tbdSize=0x%08x @bdBase=0x%08x"
                 "\n\trbdNum=%d @ pRbdBase=0x%08x"
                 "\n\ttbdNum=%d @ pTbdBase=0x%08x\n"),
                 pDrvCtrl->bdSize,
                 (int)pDrvCtrl->pBdBase,
                 pDrvCtrl->rbdNum,
                 (int)pDrvCtrl->pRbdBase,
                 pDrvCtrl->tbdNum,
                 (int)pDrvCtrl->pTbdBase
                 );

    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
               ("\n\tpBufBase=0x%08x"
               "\n\tbufSize=0x%08x"
               "\n\tpMBlkArea=0x%08x"
               "\n\tmBlkSize=0x%08x\n"),
               (int)pDrvCtrl->pBufBase,
               (int)pDrvCtrl->bufSize,
               (int)pDrvCtrl->pMBlkArea,
               (int)pDrvCtrl->mBlkSize,
               0,0);


    MOT_FCC_LOG (MOT_FCC_DBG_ANY,
                ("\n\tpNetPool=0x%08x\n"),
                (int)pDrvCtrl->endObj.pNetPool, 0,0,0,0,0);
    }
#endif

