/* motFcc2End.c - Second Generation Motorola FCC Ethernet network interface.*/ 

/*
 * This patched driver corrects the following problems:
 *
 * - General RX and TX logic cleanup
 * - Handle IFF_ALLMULTI correctly
 * - Don't reset interface when deleting multicast addresses (it's
 *   not necessary).
 * - Wait for pending RX and TX jobs running in tNetTask to complete
 *   before finishing motFccStop() operation.
 * - Wait for all RX buffers to be reclaimed before finishing motFccStop()
 *   operation.
 */

/* Copyright 1989-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,14jan03,gjc  SPR#85164 Second Generation Motorola FCC END Driver.
*/

/*
DESCRIPTION
This module implements a Motorola Fast Communication Controller (FCC)
Ethernet network interface driver. This is a second generation driver that is
based off motFccEnd.c. It differs from the original in initialization, 
performance, features and SPR fixes.
 
The driver "load string" interface differs from its predecessor. A parameter, 
that contains a pointer to a predefined array of function pointers, was added
to the end of the load string. Adding this structure removed a layer of 
ambiguity between the driver and the BSP interface. Multiple global function 
pointers  were used previously to provide hooks for the BSP to DPRAM 
(dual port ram) allocation, mii, duplex mode, heart beat and disconnect 
functions. See the load string interface below
Trying to link a modified version of the previous driver results in link errors. 
This was due to the global funtion definitions in the driver.

Performace of the driver was greately enhanced. A layer of unnecessary queuing
was removed. Time critical functions were re-written to be more fluid and 
efficent. The drivers work load is distributed between the interrupt and the 
net job queue. Only one net job add is alowed per interrupt. Multiple events 
pending are sent as one call as an add to the job queue. TXBDs are freed in 
the interrupt code. Buffers for the RXBD ring are replenished in the interrupt.
Allocation of the Mblks and Cblks happens in the code executing on the job 
context.

RFC 2233 and IPV6 support were added as features.

SPRs, written against the original motFccEnd driver, are fixed.

The FCC supports several communication protocols. This driver supports the FCC 
to operate in Ethernet mode which is fully compliant with the IEEE 802.3u 
10Base-T and 100Base-T specifications.

The FCC establishes a shared memory communication system with the CPU,
which may be divided into three parts: a set of Control/Status Registers (CSR)
and FCC-specific parameters, the buffer descriptors (BD), and the data buffers.

Both the CSRs and the internal parameters reside in the MPC8260's internal
RAM. They are used for mode control and to extract status information
of a global nature. For instance, the types of events that should
generate an interrupt, or features like the promiscous mode or the
hearthbeat control may be set programming some of the CSRs properly.
Pointers to both the Transmit Buffer Descriptors ring (TBD) and the
Receive Buffer Descriptors ring (RBD) are stored in the internal parameter
RAM. The latter also includes protocol-specific parameters, like the
individual physical address of this station or the max receive frame length.

The BDs are used to pass data buffers and related buffer information
between the hardware and the software. They may reside either on the 60x
bus, or on the CPM local bus They include local status information and a
pointer to the incoming or outgoing data buffers. These are located again
in external memory, and the user may chose whether this is on the 60x bus,
or the CPM local bus (see below).

This driver is designed to be moderately generic. Without modification, it can
operate across all the FCCs in the MPC8260, regardless of where the internal
memory base address is located. To achieve this goal, this driver must be
given several target-specific parameters, and some external support routines
must be provided.  These parameters, and the mechanisms used to communicate
them to the driver, are detailed below.

This network interface driver does not include support for trailer protocols
or data chaining.  However, buffer loaning has been implemented in an effort
to boost performance. In addition, no copy is performed of the outgoing packet
before it is sent.

BOARD LAYOUT
This device is on-board.  No jumpering diagram is necessary.

EXTERNAL INTERFACE

The driver provides the standard external interface, motFccEnd2Load(), which
takes a string of colon-separated parameters. The parameters should be
specified in hexadecimal, optionally preceeded by "0x" or a minus sign "-".

The parameter string is parsed using strtok_r() and each parameter is
converted from a string representation to binary by a call to
strtoul(parameter, NULL, 16).

The format of the parameter string is:

"<immrVal>:<fccNum>:<bdBase>:<bdSize>:<bufBase>:<bufSize>:<fifoTxBase>:
<fifoRxBase> :<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<userFlags>:
<function table>"

TARGET-SPECIFIC PARAMETERS

.IP <immrVal>
Indicates the address at which the host processor presents its internal
memory (also known as the internal RAM base address). With this address,
and the fccNum (see below), the driver is able to compute the location of
the FCC parameter RAM, and, ultimately, to program the FCC for proper
operations.

.IP <fccNum>
This driver is written to support multiple individual device units.
This parameter is used to explicitly state which FCC is being used (on the
vads8260 board, FCC2 is wired to the Fast Ethernet tranceiver, thus this
parameter equals "2").

.IP <bdBase>
The Motorola Fast Communication Controller is a DMA-type device and typically
shares access to some region of memory with the CPU. This driver is designed
for systems that directly share memory between the CPU and the FCC.

This parameter tells the driver that space for both the TBDs and the
RBDs needs not be allocated but should be taken from a cache-coherent
private memory space provided by the user at the given address. The user
should be aware that memory used for buffers descriptors must be 8-byte
aligned and non-cacheable. Therefore, the given memory space should allow
for all the buffer descriptors and the 8-byte alignment factor.

If this parameter is "NONE", space for buffer descriptors is obtained
by calling cacheDmaMalloc() in motFccEndLoad().

.IP <bdSize>
The memory size parameter specifies the size of the pre-allocated memory
region for the BDs. If <bdBase> is specified as NONE (-1), the driver ignores
this parameter. Otherwise, the driver checks the size of the provided memory
region is adequate with respect to the given number of Transmit Buffer
Descriptors and Receive Buffer Descriptors.

.IP <bufBase>
This parameter tells the driver that space for data buffers
needs not be allocated but should be taken from a cache-coherent
private memory space provided by the user at the given address. The user
should be aware that memory used for buffers must be 32-byte
aligned and non-cacheable. The FCC poses one more constraint in that DMA
cycles may initiate even when all the incoming data have already been
transferred to memory. This means at most 32 bytes of memory at the end of
each receive data buffer, may be overwritten during reception. The driver
pads that area out, thus consuming some additional memory.

If this parameter is "NONE", space for buffer descriptors is obtained
by calling memalign() in sbcMotFccEndLoad().

.IP <bufSize>
The memory size parameter specifies the size of the pre-allocated memory
region for data buffers. If <bufBase> is specified as NONE (-1), the driver
ignores this parameter. Otherwise, the driver checks the size of the provided
memory region is adequate with respect to the given number of Receive Buffer
Descriptors and a non-configurable number of trasmit buffers
(MOT_FCC_TX_CL_NUM).  All the above should fit in the given memory space.
This area should also include room for buffer management structures.

.IP <fifoTxBase>
Indicate the base location of the transmit FIFO, in internal memory.
The user does not need to initialize this parameter, as the default
value (see MOT_FCC_FIFO_TX_BASE) is highly optimized for best performance.
However, if the user wishes to reserve that very area in internal RAM for
other purposes, he may set this parameter to a different value.

If <fifoTxBase> is specified as NONE (-1), the driver uses the default
value.

.IP <fifoRxBase>
Indicate the base location of the receive FIFO, in internal memory.
The user does not need to initialize this parameter, as the default
value (see MOT_FCC_FIFO_TX_BASE) is highly optimized for best performance.
However, if the user wishes to reserve that very area in internal RAM for
other purposes, he may set this parameter to a different value.

If <fifoRxBase> is specified as NONE (-1), the driver uses the default
value.

.IP <tbdNum>
This parameter specifies the number of transmit buffer descriptors (TBDs).
Each buffer descriptor resides in 8 bytes of the processor's external
RAM space, If this parameter is less than a minimum number specified in the
macro MOT_FCC_TBD_MIN, or if it is "NULL", a default value of 64 (see
MOT_FCC_TBD_DEF_NUM) is used. This number is kept deliberately high, since
each packet the driver sends may consume more than a single TBD. This
parameter should always equal a even number.

.IP  <rbdNum>
This parameter specifies the number of receive buffer descriptors (RBDs).
Each buffer descriptor resides in 8 bytes of the processor's external
RAM space, and each one points to a 1584-byte buffer again in external
RAM. If this parameter is less than a minimum number specified in the
macro MOT_FCC_RBD_MIN, or if it is "NULL", a default value of 32 (see
MOT_FCC_RBD_DEF_NUM) is used. This parameter should always equal a even number.

.IP  <phyAddr>
This parameter specifies the logical address of a MII-compliant physical
device (PHY) that is to be used as a physical media on the network.
Valid addresses are in the range 0-31. There may be more than one device
under the control of the same management interface. The default physical
layer initialization routine will scan the whole range of PHY devices
starting from the one in <phyAddr>. If this parameter is
"MII_PHY_NULL", the default physical layer initialization routine will find
out the PHY actual address by scanning the whole range. The one with the lowest
address will be chosen.

.IP  <phyDefMode>
This parameter specifies the operating mode that will be set up
by the default physical layer initialization routine in case all
the attempts made to establish a valid link failed. If that happens,
the first PHY that matches the specified abilities will be chosen to
work in that mode, and the physical link will not be tested.

.IP  <pAnOrderTbl>
This parameter may be set to the address of a table that specifies the
order how different subsets of technology abilities should be advertised by
the auto-negotiation process, if enabled. Unless the flag <MOT_FCC_USR_PHY_TBL>
is set in the userFlags field of the load string, the driver ignores this
parameter.

The user does not normally need to specify this parameter, since the default
behaviour enables auto-negotiation process as described in IEEE 802.3u.

.IP  <userFlags>
This field enables the user to give some degree of customization to the
driver.

MOT_FCC_USR_PHY_NO_AN: the default physical layer initialization
routine will exploit the auto-negotiation mechanism as described in
the IEEE Std 802.3u, to bring a valid link up. According to it, all
the link partners on the media will take part to the negotiation
process, and the highest priority common denominator technology ability
will be chosen. It the user wishes to prevent auto-negotiation from
occurring, he may set this bit in the user flags.

MOT_FCC_USR_PHY_TBL: in the auto-negotiation process, PHYs
advertise all their technology abilities at the same time,
and the result is that the maximum common denominator is used. However,
this behaviour may be changed, and the user may affect the order how
each subset of PHY's abilities is negotiated. Hence, when the
MOT_FCC_USR_PHY_TBL bit is set, the default physical layer
initialization routine will look at the motFccAnOrderTbl[] table and
auto-negotiate a subset of abilities at a time, as suggested by the
table itself. It is worth noticing here, however, that if the
MOT_FCC_USR_PHY_NO_AN bit is on, the above table will be ignored.

MOT_FCC_USR_PHY_NO_FD: the PHY may be set to operate in full duplex mode,
provided it has this ability, as a result of the negotiation with other
link partners. However, in this operating mode, the FCC will ignore the
collision detect and carrier sense signals. If the user wishes not to
negotiate full duplex mode, he should set the MOT_FCC_USR_PHY_NO_FD bit
in the user flags.

MOT_FCC_USR_PHY_NO_HD: the PHY may be set to operate in half duplex mode,
provided it has this ability, as a result of the negotiation with other link
partners. If the user wishes not to negotiate half duplex mode, he should
set the MOT_FCC_USR_PHY_NO_HD bit in the user flags.

MOT_FCC_USR_PHY_NO_100: the PHY may be set to operate at 100Mbit/s speed,
provided it has this ability, as a result of the negotiation with
other link partners. If the user wishes not to negotiate 100Mbit/s speed,
he should set the MOT_FCC_USR_PHY_NO_100 bit in the user flags.

MOT_FCC_USR_PHY_NO_10: the PHY may be set to operate at 10Mbit/s speed,
provided it has this ability, as a result of the negotiation with
other link partners. If the user wishes not to negotiate 10Mbit/s speed,
he should set the MOT_FCC_USR_PHY_NO_10 bit in the user flags.

MOT_FCC_USR_PHY_ISO: some boards may have different PHYs controlled by the
same management interface. In some cases, there may be the need of
electrically isolating some of them from the interface itself, in order
to guarantee a proper behaviour on the medium layer. If the user wishes to
electrically isolate all PHYs from the MII interface, he should set the
MOT_FCC_USR_PHY_ISO bit. The default behaviour is to not isolate any
PHY on the board.

MOT_FCC_USR_LOOP: when the MOT_FCC_USR_LOOP bit is set, the driver will
configure the FCC to work in internal loopback mode, with the TX signal
directly connected to the RX. This mode should only be used for testing.

MOT_FCC_USR_RMON: when the MOT_FCC_USR_RMON bit is set, the driver will
configure the FCC to work in RMON mode, thus collecting network statistics
required for RMON support without the need to receive all packets as in
promiscous mode.

MOT_FCC_USR_BUF_LBUS: when the MOT_FCC_USR_BUF_LBUS bit is set, the driver will
configure the FCC to work as though the data buffers were located in the
CPM local bus.

MOT_FCC_USR_BD_LBUS: when the MOT_FCC_USR_BD_LBUS bit is set, the driver will
configure the FCC to work as though the buffer descriptors were located in the
CPM local bus.

MOT_FCC_USR_HBC: if the MOT_FCC_USR_HBC bit is set, the driver will
configure the FCC to perform heartbeat check following end of transmisson
and the HB bit in the status field of the TBD will be set if the collision
input does not assert within the heartbeat window. The user does not normally
need to set this bit.

.IP <Function Table>
This is a pointer to the structure FCC_END_FUNCS. The stucture contains mostly 
FUNCPTRs that are used as a communication mechanism between the driver and the
BSP. If the pointer contains a NULL value, the driver will use system default
functions for the m82xxDpram DPRAM alloction and, obviously, the driver will 
not support BSP function calls for heart beat errors, disconnect errors, and 
PHY status changes tat are harware specific. 

.CS
    FUNCPTR miiPhyInit; BSP Mii/Phy Init Function
    This funtion pointer is initialized by the BSP and call by the driver to  
    initialize the mii driver. The driver sets up it's phy settings and then
    calls this routine. The BSP is responsible for setting BSP specific phy 
    parameters and then calling the miiPhyInit. The BSP is responsible to set
    up any call to an interrupt. See miiPhyInt bellow. 
    
.CE
.CS
    FUNCPTR miiPhyInt; Driver Function for BSP to Call on a Phy Status Change
    This function pointer is intialized by the driver and call by the BSP.
    The BSP calls this function when it handles a hardware mii specific 
    interrupt. The driver initalizes this to the function motFccPhyLSCInt.
    The BSP may or may not choose to call this funtion. It will depend if the 
    BSP supports an interrupt driven PHY. The BSP can also set up the miiLib 
    driver to poll. In this case the miiPhy diver calls this funtion. See the 
    miiLib for details.
    Note: Not calling this function when the phy duplex mode changes will 
    result in a duplex mis-match. This will cause TX errors in the driver 
    and a reduction in throughput. 
.CE
.CS
    FUNCPTR miiPhyBitRead; MII Bit Read Funtion
    This function pointer is intialized by the BSP and call by the driver.
    The driver calls this function when it needs to read a bit from the mii 
    interface. The mii interface is hardware specific. 
.CE
.CS
    FUNCPTR miiPhyBitWrite; MII Bit Write Function
    This function pointer is intialized by the BSP and call by the driver.
    The driver calls this function when it needs to write a bit to the mii 
    interface. This mii interface is hardware specific. 
.CE
.CS
    FUNCPTR miiPhyDuplex; Duplex Status Call Back 
    This function pointer is intialized by the BSP and call by the driver.
    The driver calls this function to obtain the status of the duplex 
    setting in the phy.  
.CE
.CS
    FUNCPTR miiPhySpeed; Speed Status Call Back
    This function pointer is intialized by the BSP and call by the driver.
    The driver calls this function to obtain the status of the speed 
    setting in the phy. This interface is hardware specific. 
.CE
.CS
    FUNCPTR hbFail; Heart Beat Fail Indicator
    This function pointer is intialized by the BSP and call by the driver.
    The driver calls this function to inticate an FCC heart beat error. 
.CE
.CS
    FUNCPTR intDisc; Disconnect Function 
    This function pointer is intialized by the BSP and call by the driver.
    The driver calls this function to inticate an FCC disconnect error. 
.CE
.CS
    FUNCPTR dpramFree; DPRAM Free routine
    This function pointer is intialized by the BSP and call by the driver.
    The BSP allocates memory for the BDs from this pool. The Driver must
    free the bd area using this function.
.CE
.CS
    FUNCPTR dpramFccMalloc; DPRAM FCC Malloc routine
    This function pointer is intialized by the BSP and call by the driver.
    The Driver allocates memory from the FCC specific POOL using this function. 
.CE
.CS
    FUNCPTR dpramFccFree; DPRAM FCC Free routine
    This function pointer is intialized by the BSP and call by the driver.
    The Driver frees memory from the FCC specific POOL using this function. 
.CE
 
.LP

EXTERNAL SUPPORT REQUIREMENTS
This driver requires several external support functions. 
Note: Funtion pointers _func_xxxxxxxx were removed and replaced with
the FCC_END_FUNCS structure located in the load string. This is a major 
differencd between the old motFccEnd driver and this one.
.IP sysFccEnetEnable()
.CS
    STATUS sysFccEnetEnable (UINT32 immrVal, UINT8 fccNum);
.CE
This routine is expected to handle any target-specific functions needed
to enable the FCC. These functions typically include setting the Port B and C
on the MPC8260 so that the MII interface may be used. This routine is
expected to return OK on success, or ERROR. The driver calls this routine,
once per device, from the motFccStart () routine.
.IP sysFccEnetDisable()
.CS
    STATUS sysFccEnetDisable (UINT32 immrVal, UINT8 fccNum);
.CE
This routine is expected to perform any target specific functions required
to disable the MII interface to the FCC.  This involves restoring the
default values for all the Port B and C signals. This routine is expected to
return OK on success, or ERROR. The driver calls this routine from the
motFccStop() routine each time a device is disabled.
.IP sysFccEnetAddrGet()
.CS
    STATUS sysFccEnetAddrGet (int unit,UCHAR *address);
.CE
The driver expects this routine to provide the six-byte Ethernet hardware
address that is used by this device.  This routine must copy the six-byte
address to the space provided by <enetAddr>.  This routine is expected to
return OK on success, or ERROR.  The driver calls this routine, once per
device, from the motFccEndLoad() routine.
.CS
    STATUS sysFccMiiBitWr (UINT32 immrVal, UINT8 fccNum, INT32 bitVal);
.CE
This routine is expected to perform any target specific functions required
to write a single bit value to the MII management interface of a MII-compliant
PHY device. The MII management interface is made up of two lines: management 
data clock (MDC) and management data input/output (MDIO). The former provides
the timing reference for transfer of information on the MDIO signal.
The latter is used to transfer control and status information between the
PHY and the FCC. For this transfer to be successful, the information itself 
has to be encoded into a frame format, and both the MDIO and MDC signals have
to comply with certain requirements as described in the 802.3u IEEE Standard.
There is not buil-in support in the FCC for the MII management interface.
This means that the clocking on the MDC line and the framing of the information
on the MDIO signal have to be done in software. Hence, this routine is 
expected to write the value in <bitVal> to the MDIO line while properly 
sourcing the MDC clock to a PHY, for one bit time.
.CS
    STATUS sysFccMiiBitRd (UINT32 immrVal, UINT8 fccNum, INT8 * bitVal);
.CE
This routine is expected to perform any target specific functions required
to read a single bit value from the MII management interface of a MII-compliant
PHY device. The MII management interface is made up of two lines: management 
data clock (MDC) and management data input/output (MDIO). The former provides
the timing reference for transfer of information on the MDIO signal.
The latter is used to transfer control and status information between the
PHY and the FCC. For this transfer to be successful, the information itself 
has to be encoded into a frame format, and both the MDIO and MDC signals have
to comply with certain requirements as described in the 802.3u IEEE Standard.
There is not buil-in support in the FCC for the MII management interface.
This means that the clocking on the MDC line and the framing of the information
on the MDIO signal have to be done in software. Hence, this routine is 
expected to read the value from the MDIO line in <bitVal>, while properly 
sourcing the MDC clock to a PHY, for one bit time.
.LP

SYSTEM RESOURCE USAGE
If the driver allocates the memory for the BDs to share with the FCC,
it does so by calling the cacheDmaMalloc() routine.  For the default case
of 64 transmit buffers and 32 receive buffers, the total size requested
is 776 bytes, and this includes the 8-byte alignment requirement of the
device.  If a non-cacheable memory region is provided by the user, the
size of this region should be this amount, unless the user has specified
a different number of transmit or receive BDs.

This driver can operate only if this memory region is non-cacheable
or if the hardware implements bus snooping.  The driver cannot maintain
cache coherency for the device because the BDs are asynchronously
modified by both the driver and the device, and these fields share
the same cache line.

If the driver allocates the memory for the data buffers to share with the FCC,
it does so by calling the memalign () routine.  The driver does not need to
use cache-safe memory for data buffers, since the host CPU and the device are
not allowed to modify buffers asynchronously. The related cache lines
are flushed or invalidated as appropriate. For the default case
of 7 transmit clusters and 32 receive clusters, the total size requested
for this memory region is 112751 bytes, and this includes the 32-byte
alignment and the 32-byte pad-out area per buffer of the device.  If a
non-cacheable memory region is provided by the user, the size of this region
should be this amount, unless the user has specified a different number
of transmit or receive BDs.

TUNING HINTS

The only adjustable parameters are the number of TBDs and RBDs that will be
created at run-time.  These parameters are given to the driver when
motFccEndLoad() is called.  There is one RBD associated with each received
frame whereas a single transmit packet normally uses more than one TBD.  For
memory-limited applications, decreasing the number of RBDs may be
desirable.  Decreasing the number of TBDs below a certain point will
provide substantial performance degradation, and is not reccomended. An
adequate number of loaning buffers are also pre-allocated to provide more
buffering before packets are dropped, but this is not configurable.

The relative priority of the netTask and of the other tasks in the system
may heavily affect performance of this driver. Usually the best performance
is achieved when the netTask priority equals that of the other
applications using the driver.

SPECIAL CONSIDERATIONS

SEE ALSO: ifLib,
.I "MPC8260 Fast Ethernet Controller (Supplement to the MPC860 User's Manual)"
.I "Motorola MPC860 User's Manual",

INTERNAL
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

#ifndef MOT_FCC_DBG
#define MOT_FCC_DBG
#endif
 

#ifndef MOTFCC2END_HEADER
#define MOTFCC2END_HEADER "motFcc2End.h"
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
#define WV_INT_GRA_ENTRY(b,l)        wvEvent(1700,b,l)
#define WV_INT_GRA_EXIT(b,l)         wvEvent(1701,b,l)
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
#else
#define WV_INT_ENTRY(b,l)            
#define WV_INT_EXIT(b,l)             
#define WV_INT_RXB_ENTRY(b,l)        
#define WV_INT_RXF_ENTRY(b,l)        
#define WV_INT_BSY_ENTRY(b,l)        
#define WV_INT_BSY_EXIT(b,l)
#define WV_INT_RX_EXIT(b,l)          
#define WV_INT_RXC_ENTRY(b,l)        
#define WV_INT_RXC_EXIT(b,l)         
#define WV_INT_TXC_ENTRY(b,l)        
#define WV_INT_TXC_EXIT(b,l)         
#define WV_INT_TXB_ENTRY(b,l)        
#define WV_INT_TXB_EXIT(b,l)         
#define WV_INT_TXE_ENTRY(b,l)        
#define WV_INT_TXE_EXIT(b,l)         
#define WV_INT_GRA_ENTRY(b,l)        
#define WV_INT_GRA_EXIT(b,l)         
#define WV_INT_NETJOBADD_ENTRY(b,l)  
#define WV_INT_NETJOBADD_EXIT(b,l)   
#define WV_HANDLER_ENTRY(b,l)        
#define WV_HANDLER_EXIT(b,l)         
#define WV_MUX_TX_RESTART_ENTRY(b,l) 
#define WV_MUX_TX_RESTART_EXIT(b,l)  
#define WV_MUX_ERROR_ENTRY(b,l)      
#define WV_MUX_ERROR_EXIT(b,l)       
#define WV_SEND_ENTRY(b,l)           
#define WV_SEND_EXIT(b,l)            
#define WV_RECV_ENTRY(b,l)           
#define WV_RECV_EXIT(b,l)            
#define WV_CACHE_FLUSH_ENTRY(b,l)    
#define WV_CACHE_FLUSH_EXIT(b,l)     
#define WV_CACHE_INVAL_ENTRY(b,l)    
#define WV_CACHE_INVAL_EXIT(b,l)     
#endif
/* defines */

/* general macros for reading/writing from/to specified locations */

/* Cache and virtual/physical memory related macros */

#define MOT_FCC_CACHE_INVAL(address, len) { \
    WV_CACHE_INVAL_ENTRY(0,0);             \
    CACHE_DRV_INVALIDATE (&pDrvCtrl->bufCacheFuncs, (address), (len));  \
    WV_CACHE_INVAL_EXIT(0,0)  \
    }

#define MOT_FCC_CACHE_FLUSH(address, len) { \
    WV_CACHE_FLUSH_ENTRY(0,0);             \
    CACHE_DRV_FLUSH (&pDrvCtrl->bufCacheFuncs, (address), (len)); \
    WV_CACHE_FLUSH_EXIT(0,0)   \
    }

/* driver flags */

#define MOT_FCC_OWN_BUF_MEM 0x01    /* internally provided memory for data*/
#define MOT_FCC_INV_TBD_NUM 0x02    /* invalid tbdNum provided */
#define MOT_FCC_INV_RBD_NUM 0x04    /* invalid rbdNum provided */
#define MOT_FCC_POLLING     0x08    /* polling mode */
#define MOT_FCC_PROM        0x20    /* promiscuous mode */
#define MOT_FCC_MCAST       0x40    /* multicast addressing mode */
#define MOT_FCC_FD          0x80    /* full duplex mode */
#define MOT_FCC_OWN_BD_MEM  0x10    /* internally provided memory for BDs */

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
 * the total is 0x630 and it accounts for the required alignment
 * of receive data buffers, and the cluster overhead.
 */
#define XXX_FCC_MAX_CL_LEN      ((MII_ETH_MAX_PCK_SZ            \
                 + (MOT_FCC_BUF_ALIGN - 1)      \
                 + MOT_FCC_BUF_ALIGN            \
                 + (CL_OVERHEAD - 1))           \
                 & (~ (CL_OVERHEAD - 1)))

#define MOT_FCC_MAX_CL_LEN     ROUND_UP(XXX_FCC_MAX_CL_LEN,MOT_FCC_BUF_ALIGN)

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

#ifdef INCLUDE_RFC_2233
#define END_HADDR(pEnd)                                                  \
        ((pEnd).pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd)                                              \
        ((pEnd).pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.addrLength)

#define END_INC_IN_UCAST(mData, mLen)  \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_IN,           \
                                    mData,   \
                                    mLen );  \
            }
                                    
#define END_INC_IN_NUCAST(mData, mLen)  \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_IN,           \
                                    mData,   \
                                    mLen );  \
            }
        
#define END_INC_IN_ERRS()  \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
            {                                    \
            pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                     M2_ctrId_ifInErrors, 1);  \
            }
            
#define END_INC_IN_DISCARDS() \
         if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
             {                                    \
             pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                             M2_ctrId_ifInDiscards, 1);  \
             }
             
#define END_INC_IN_OCTETS(mLen)
                                     
#define END_INC_OUT_UCAST(mData, mLen) \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
            {                                    \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_OUT,           \
                                    mData,   \
                                    mLen );  \
            }
                                   
#define END_INC_OUT_NUCAST(mData, mLen) \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)  \
            {                                    \
            pDrvCtrl->endObj.pMib2Tbl->m2PktCountRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_PACKET_OUT,           \
                                    mData,   \
                                    mLen );  \
            }
                                  
                                    
#define END_INC_OUT_ERRS()                         \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                     M2_ctrId_ifOutErrors, 1);  \
            }
                                  
#define END_INC_OUT_DISCARDS()                      \
        if (pDrvCtrl->endObj.pMib2Tbl != NULL)      \
            {                                       \
            pDrvCtrl->endObj.pMib2Tbl->m2CtrUpdateRtn (pDrvCtrl->endObj.pMib2Tbl, \
                                    M2_ctrId_ifOutDiscards, 1);  \
            }
            
#define END_INC_OUT_OCTETS(mLen)
            
#else

#ifdef INCLUDE_RFC_1213_OLD
#undef INCLUDE_RFC_1213_OLD
#endif
      
#undef INCLUDE_RFC_1213_OLD
/* RFC 1213 mib2 interface */

#define END_HADDR(pEnd)                                             \
        ((pEnd).mib2Tbl.ifPhysAddress.phyAddress)

#define END_HADDR_LEN(pEnd)                                         \
        ((pEnd).mib2Tbl.ifPhysAddress.addrLength)

#ifdef  INCLUDE_RFC_1213_OLD
#define END_INC_IN_ERRS()                                              \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_ERRS, +1)
#define END_INC_IN_DISCARDS()                                          \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_ERRS, +1)
#define END_INC_IN_UCAST(mData, mLen)                                  \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_UCAST, +1)

#define END_INC_IN_NUCAST(mData, mLen)                                 \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_IN_UCAST, +1)    
#define END_INC_IN_OCTETS(mLen)
            
#define END_INC_OUT_ERRS()                                             \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1)
            
#define END_INC_OUT_DISCARDS()                                         \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_ERRS, +1)
            
#define END_INC_OUT_UCAST(mData, mLen)                                 \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1)
#define END_INC_OUT_NUCAST(mData, mLen)                                \
            END_ERR_ADD(&pDrvCtrl->endObj, MIB2_OUT_UCAST, +1)
#define END_INC_OUT_OCTETS(mLen)        
#else
#define END_INC_IN_DISCARDS()           (pDrvCtrl->endObj.mib2Tbl.ifInDiscards++)
#define END_INC_IN_ERRS()               (pDrvCtrl->endObj.mib2Tbl.ifInErrors++)
#define END_INC_IN_UCAST(mData, mLen)   (pDrvCtrl->endObj.mib2Tbl.ifInUcastPkts++)
#define END_INC_IN_NUCAST(mData, mLen)  (pDrvCtrl->endObj.mib2Tbl.ifInNUcastPkts++)
#define END_INC_IN_OCTETS(mLen)         (pDrvCtrl->endObj.mib2Tbl.ifInOctets += mLen)

#define END_INC_OUT_DISCARDS()          (pDrvCtrl->endObj.mib2Tbl.ifOutDiscards++)
#define END_INC_OUT_ERRS()              (pDrvCtrl->endObj.mib2Tbl.ifOutErrors++)
#define END_INC_OUT_UCAST(mData, mLen)  (pDrvCtrl->endObj.mib2Tbl.ifOutUcastPkts++)
#define END_INC_OUT_NUCAST(mData, mLen) (pDrvCtrl->endObj.mib2Tbl.ifOutNUcastPkts++)
#define END_INC_OUT_OCTETS(mLen)        (pDrvCtrl->endObj.mib2Tbl.ifOutOctets += mLen)
#endif

#endif /* INCLUDE_RFC_2233 */
           
#define NET_TO_MOT_FCC_BUF(netBuf)                                      \
    (char *)(((UINT32) (netBuf) + MOT_FCC_BD_ALIGN - 1)                 \
      & ~(MOT_FCC_BD_ALIGN - 1))

/* locals */

/* Function declarations not in any header files */

/* forward function declarations */

LOCAL STATUS    motFccInitParse (DRV_CTRL * pDrvCtrl, char *initString);
LOCAL STATUS    motFccInitMem (DRV_CTRL *pDrvCtrl);
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
LOCAL int       motFccTbdClean (DRV_CTRL * pDrvCtrl);
LOCAL void      motFccInt (DRV_CTRL * pDrvCtrl);
LOCAL void      motFccHandleRXFrames(DRV_CTRL *pDrvCtrl);
STATUS    motFccMiiRead (DRV_CTRL * pDrvCtrl, UINT8 phyAddr,
                   UINT8 regAddr, UINT16 *retVal);
STATUS    motFccMiiWrite (DRV_CTRL * pDrvCtrl, UINT8 phyAddr,
                UINT8 regAddr, UINT16 writeData);
LOCAL STATUS    motFccAddrSet (DRV_CTRL * pDrvCtrl, UCHAR * pAddr,
                   UINT32 offset);
LOCAL void      motFccPhyLSCInt ( DRV_CTRL *pDrvCtrl );
LOCAL void      motFccHandleLSCJob (DRV_CTRL *pDrvCtrl);

/* END Specific interfaces. */

END_OBJ *       motFcc2EndLoad (char *initString);
LOCAL STATUS    motFccStart (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccUnload (DRV_CTRL * pDrvCtrl);
LOCAL STATUS    motFccStop (DRV_CTRL * pDrvCtrl);
LOCAL void      motFccTxErrReInit (DRV_CTRL * pDrvCtrl, UINT16 stat);
LOCAL int       motFccIoctl (DRV_CTRL * pDrvCtrl, int cmd, caddr_t data);
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

void motFccIramShow (DRV_CTRL *);
void motFccPramShow (DRV_CTRL *);
void motFccEramShow (DRV_CTRL *);

void motFccDrvShow (DRV_CTRL *);
void motFccMiiShow (DRV_CTRL *);
void motFccMibShow (DRV_CTRL *);

#define MOT_FCC_DBG_OFF     0x0000
#define MOT_FCC_DBG_RX      0x0001
#define MOT_FCC_DBG_TX      0x0002
#define MOT_FCC_DBG_POLL    0x0004
#define MOT_FCC_DBG_MII     0x0008
#define MOT_FCC_DBG_LOAD    0x0010
#define MOT_FCC_DBG_IOCTL   0x0020
#define MOT_FCC_DBG_INT     0x0040
#define MOT_FCC_DBG_START   0x0080
#define MOT_FCC_DBG_INT_RX_ERR  0x0100
#define MOT_FCC_DBG_INT_TX_ERR  0x0200
#define MOT_FCC_DBG_ANY     0xffff

#define MOT_FCC_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)            \
    {                                   \
    if (motFccEndDbg & FLG)                     \
        logMsg (X0, X1, X2, X3, X4, X5, X6);                    \
    }

#define MOT_FCC_STAT_INCR(i) (i++)

FUNCPTR _func_netJobAdd;
FUNCPTR _func_txRestart;
FUNCPTR _func_error;

UINT32 motFccEndDbg = 0;     /* global debug level flag */
    
DRV_CTRL *  pDrvCtrlDbg_fcc0 = NULL;
DRV_CTRL *  pDrvCtrlDbg_fcc1 = NULL;
#else /* MOT_FCC_DBG */
#define MOT_FCC_LOG(FLG, X0, X1, X2, X3, X4, X5, X6)
#define MOT_FCC_STAT_INCR(i)
#endif

#ifdef MOT_FCC_DBG
LOCAL const char    *speedStr[2]  = {"10","100"};
LOCAL const char    *linkStr[2]   = {"Down","Up"};
LOCAL const char    *duplexStr[2] = {"Half","Full"};
#endif

/*
 * Define the device function table.  This is static across all driver
 * instances.
 */

LOCAL NET_FUNCS netFccFuncs =
    {
    (FUNCPTR) motFccStart,      /* start func. */
    (FUNCPTR) motFccStop,       /* stop func. */
    (FUNCPTR) motFccUnload,     /* unload func. */
    (FUNCPTR) motFccIoctl,      /* ioctl func. */
    (FUNCPTR) motFccSend,       /* send func. */
    (FUNCPTR) motFccMCastAddrAdd,       /* multicast add func. */
    (FUNCPTR) motFccMCastAddrDel,       /* multicast delete func. */
    (FUNCPTR) motFccMCastAddrGet,       /* multicast get fun. */
    (FUNCPTR) motFccPollSend,       /* polling send func. */
    (FUNCPTR) motFccPollReceive,        /* polling receive func. */
    endEtherAddressForm,        /* put address info into a NET_BUFFER */
    (FUNCPTR) endEtherPacketDataGet,    /* get pointer to data in NET_BUFFER */
    (FUNCPTR) endEtherPacketAddrGet     /* Get packet addresses */
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
* "<immrVal>:<ivec>:<bufBase>:<bufSize>:<fifoTxBase>:<fifoRxBase>
* :<tbdNum>:<rbdNum>:<phyAddr>:<phyDefMode>:<pAnOrderTbl>:<userFlags>"
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
* routine will attempt to allocate the shared memory from the system.  Any
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
* region, then this routine will use cacheDmaMalloc() to obtain
* some  cache-safe memory.  The attributes of this memory will be checked,
* and if the memory is not write coherent, this routine will abort and
* return NULL.
*
* RETURNS: an END object pointer, or NULL on error.
*
* SEE ALSO: ifLib,
* .I "MPC8260 Power QUICC II User's Manual"
*/

END_OBJ* motFcc2EndLoad
    (
    char *initString
    )
    {
    DRV_CTRL * pDrvCtrl = NULL;            /* pointer to DRV_CTRL structure */
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
        return NULL;


    /* set up function pointers */
    pDrvCtrl->netJobAdd    = (FUNCPTR) netJobAdd;
    pDrvCtrl->muxTxRestart = (FUNCPTR) muxTxRestart; 
    pDrvCtrl->muxError     = (FUNCPTR) muxError;     

    /* Parse InitString */

    if (motFccInitParse (pDrvCtrl, initString) == ERROR)
        goto errorExit;

#ifdef MOT_FCC_DBG
	if(pDrvCtrl->unit == 0)
	    pDrvCtrlDbg_fcc0 = pDrvCtrl;
	else
		pDrvCtrlDbg_fcc1 = pDrvCtrl;

    /* get memory for the drivers stats structure */
    if ((pDrvCtrl->Stats = calloc (sizeof (FCC_DRIVER_STATS), 1)) == NULL)
        return NULL;

    /* support unit test */ 
    _func_netJobAdd = (FUNCPTR) netJobAdd;
    _func_txRestart = (FUNCPTR) muxTxRestart;
    _func_error     = (FUNCPTR) muxError;
#endif /* MOT_FCC_DBG */

    /* sanity check the unit number */
	
    if (pDrvCtrl->unit < 0 )
        goto errorExit;

    /* memory initialization */

    if (motFccInitMem (pDrvCtrl) == ERROR)
        goto errorExit;

    /* get our ethernet hardware address */

    SYS_FCC_ENET_ADDR_GET (enetAddr);

    /* init miiPhy fuctions */

    if ( pDrvCtrl->motFccFuncs == NULL )
        {
        pDrvCtrl->hbFailFunc    = NULL;   
        pDrvCtrl->intDiscFunc   = NULL;  
        pDrvCtrl->phyInitFunc   = NULL;  
        pDrvCtrl->phyDuplexFunc = NULL;
        pDrvCtrl->phySpeedFunc  = NULL;
        }
    else
        {
        pDrvCtrl->hbFailFunc    = pDrvCtrl->motFccFuncs->hbFail;   
        pDrvCtrl->intDiscFunc   = pDrvCtrl->motFccFuncs->intDisc;  
        pDrvCtrl->phyInitFunc   = pDrvCtrl->motFccFuncs->miiPhyInit;
        pDrvCtrl->phyDuplexFunc = pDrvCtrl->motFccFuncs->miiPhyDuplex;
        pDrvCtrl->phySpeedFunc  = pDrvCtrl->motFccFuncs->miiPhySpeed;

        /* BSP call back to driver */
        pDrvCtrl->motFccFuncs->miiPhyInt = (FUNCPTR) motFccPhyLSCInt;
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

    /* Set zeroBufFlag */
    if (pDrvCtrl->userFlags & MOT_FCC_USR_BUF_LBUS
        || pDrvCtrl->userFlags & MOT_FCC_USR_NO_ZCOPY)
        {
        pDrvCtrl->zeroBufFlag = FALSE;
        }
    else
        {
        pDrvCtrl->zeroBufFlag = TRUE;
        }

    /* store the internal ram base address */

    pDrvCtrl->fccIramAddr = (UINT32) M8260_FGMR1 (pDrvCtrl->immrVal) +
                            ((pDrvCtrl->fccNum - 1) * M8260_FCC_IRAM_GAP);

    pDrvCtrl->fccReg = (FCC_REG_T *)pDrvCtrl->fccIramAddr;

    /* store the parameter ram base address */

    pDrvCtrl->fccPramAddr = (UINT32) M8260_FCC1_BASE (pDrvCtrl->immrVal) +
                            ((pDrvCtrl->fccNum - 1) * M8260_FCC_DPRAM_GAP);

    pDrvCtrl->fccPar = (FCC_PARAM_T *) pDrvCtrl->fccPramAddr;
    pDrvCtrl->fccEthPar = &pDrvCtrl->fccPar->prot.e;

    /*
     * create the synchronization semaphore for graceful transmit
     * command interrupts.
     */

    if ( (pDrvCtrl->graSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY) )== NULL )                            \
        goto errorExit;

    /*
     * Because we create EMPTY semaphore we need to give it here
     * other wise the only time that it's given back is in the
     * motFccInt() and if we have two NI in the bootrom like SCC
     * and FCC the motFccStop() will be spin forever when it will
     * try to do semTake.
     */

    semGive (pDrvCtrl->graSem);

    /* endObj initializations */

    if (END_OBJ_INIT (&pDrvCtrl->endObj, /*NULL xiexy changre from*/(DEV_OBJ*)pDrvCtrl, 
                      MOT_FCC_DEV_NAME, pDrvCtrl->unit, &netFccFuncs,
                      "Motorola FCC Ethernet Enhanced Network Driver")
        == ERROR)
        goto errorExit;

    pDrvCtrl->phyInfo->phySpeed = MOT_FCC_10MBS;

#ifdef INCLUDE_RFC_2233

    /* Initialize MIB-II entries (for RFC 2233 ifXTable) */
    pDrvCtrl->endObj.pMib2Tbl = m2IfAlloc(M2_ifType_ethernet_csmacd,
                                          (UINT8*) enetAddr, 6,
                                          ETHERMTU, pDrvCtrl->phyInfo->phySpeed,
                                          MOT_FCC_DEV_NAME, pDrvCtrl->unit);

    if (pDrvCtrl->endObj.pMib2Tbl == NULL)
        {
        logMsg ("%s%d - MIB-II initializations failed\n",
                (int)MOT_FCC_DEV_NAME, pDrvCtrl->unit,0,0,0,0);
        goto errorExit;
        }
        
    /* 
     * Set the RFC2233 flag bit in the END object flags field and
     * install the counter update routines.
     */

    m2IfPktCountRtnInstall(pDrvCtrl->endObj.pMib2Tbl, m2If8023PacketCount);

    /*
     * Make a copy of the data in mib2Tbl struct as well. We do this
     * mainly for backward compatibility issues. There might be some
     * code that might be referencing the END pointer and might
     * possibly do lookups on the mib2Tbl, which will cause all sorts
     * of problems.
     */

    bcopy ((char *)&pDrvCtrl->endObj.pMib2Tbl->m2Data.mibIfTbl,
                   (char *)&pDrvCtrl->endObj.mib2Tbl, sizeof (M2_INTERFACETBL));

    /* Mark the device ready */

    END_OBJ_READY (&pDrvCtrl->endObj,
                   IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST |
                   END_MIB_2233);


#else

    /* Old RFC 1213 mib2 interface */

    if (END_MIB_INIT (&pDrvCtrl->endObj, M2_ifType_ethernet_csmacd,
                      (u_char *) &enetAddr[0], MOT_FCC_ADDR_LEN,
                      ETHERMTU, pDrvCtrl->phyInfo->phySpeed) == ERROR)
        goto errorExit;

    /* Mark the device ready */

    END_OBJ_READY (&pDrvCtrl->endObj,
                   IFF_NOTRAILERS | IFF_MULTICAST | IFF_BROADCAST);

#endif  /* INCLUDE_RFC_2233 */



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
    FCC_PARAM_T     *   pParam;
    FCC_ETH_PARAM_T *   pEthPar;
    int                 retVal;

    /* get to the beginning of the parameter area */
    pParam  = pDrvCtrl->fccPar;
    pEthPar = pDrvCtrl->fccEthPar;

    if (pDrvCtrl == NULL)
        goto errorExit;

    if ((pDrvCtrl->state & MOT_FCC_STATE_LOADED) == MOT_FCC_STATE_NOT_LOADED)
        goto errorExit;

    /* must stop the device before unloading it */

    if ((pDrvCtrl->state & MOT_FCC_STATE_RUNNING) == MOT_FCC_STATE_RUNNING)
        motFccStop(pDrvCtrl);

    /* disconnect the interrupt handler */

    SYS_FCC_INT_DISCONNECT (pDrvCtrl, motFccInt, (int)pDrvCtrl, retVal);
    if ( retVal == ERROR )
        goto errorExit;

    /* free allocated memory if necessary */

    if ((MOT_FCC_FLAG_ISSET (MOT_FCC_OWN_BUF_MEM)) && (pDrvCtrl->pBufBase != NULL))
        free (pDrvCtrl->pBufOrig);

    if ((MOT_FCC_FLAG_ISSET (MOT_FCC_OWN_BD_MEM)) && (pDrvCtrl->pBdBase != NULL))
        cacheDmaFree (pDrvCtrl->pBdOrig);
    else 
        {
        /* release from the DPRAM pool */

        pDrvCtrl->dpramFreeFunc ((void *) pDrvCtrl->pBdBase);
        pDrvCtrl->pBdBase = NULL;
        }

    /* free allocated memory if necessary */

    if ((pDrvCtrl->pMBlkArea) != NULL)
        free (pDrvCtrl->pMBlkArea);

#ifdef INCLUDE_RFC_2233
    /* Free MIB-II entries */

    m2IfFree(pDrvCtrl->endObj.pMib2Tbl);

    pDrvCtrl->endObj.pMib2Tbl = NULL;

#endif /* INCLUDE_RFC_2233 */

    /* free misc resources */

    if (pDrvCtrl->rBufList)
        free(pDrvCtrl->rBufList);

    pDrvCtrl->rBufList = NULL;

    if (pDrvCtrl->tBufList)
        free(pDrvCtrl->tBufList);

    pDrvCtrl->tBufList = NULL;

    END_OBJECT_UNLOAD (&pDrvCtrl->endObj);

    /* free the semaphores if necessary */

    if (pDrvCtrl->graSem != NULL)
        semDelete (pDrvCtrl->graSem);

    if ((char *) pDrvCtrl->phyInfo != NULL)
        cfree ((char *) pDrvCtrl->phyInfo);

#ifdef  CPM_DPRAM_POOL  //xiexy
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
#endif 

    pDrvCtrl->state = MOT_FCC_STATE_INIT;

#ifdef MOT_FCC_DBG
    pDrvCtrlDbg_fcc0 = NULL;
	pDrvCtrlDbg_fcc1 = NULL ;

    free (pDrvCtrl->Stats);
#endif /* MOT_FCC_DBG */
    
    /* free driver control structure */
    free (pDrvCtrl);

    return (OK);

    errorExit:

    return ERROR;
    }

/*******************************************************************************
*
* motFccInitParse - parse parameter values from initString
*
* This routine parses parameter values from initString and stores them in
* the related fiels of the driver control structure.
*
* RETURNS: OK or ERROR
*/
LOCAL STATUS motFccInitParse
    (
    DRV_CTRL *  pDrvCtrl,   /* pointer to DRV_CTRL structure */
    char *  initString      /* parameter string */
    )
    {
    char *  tok;        /* an initString token */
    char *  holder = NULL;  /* points to initString fragment beyond tok */

	
    tok = strtok_r (initString, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->unit = (int) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->immrVal = (UINT32) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->fccNum = (UINT32) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->pBdBase = (char *) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->bdSize = strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->pBufBase = (char *) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->bufSize = strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->fifoTxBase = (UINT32) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->fifoRxBase = (UINT32) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->tbdNum = (UINT16) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->rbdNum = (UINT16) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->phyInfo->phyAddr = (UINT8) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->phyInfo->phyDefMode = (UINT8) strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->phyInfo->phyAnOrderTbl = (MII_AN_ORDER_TBL *)
                    strtoul (tok, NULL, 16);

    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->userFlags = strtoul (tok, NULL, 16);

    /* bsp function structure */
    tok = strtok_r (NULL, ":", &holder);
    if (tok == NULL)
        return ERROR;
    pDrvCtrl->motFccFuncs = (FCC_END_FUNCS *) strtoul (tok, NULL, 16);


    if (!pDrvCtrl->tbdNum || pDrvCtrl->tbdNum <= 2)
        {
        MOT_FCC_FLAG_SET (MOT_FCC_INV_TBD_NUM);
        pDrvCtrl->tbdNum = MOT_FCC_TBD_DEF_NUM;
        }

    if (!pDrvCtrl->rbdNum || pDrvCtrl->rbdNum <= 2)
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

/* cluster blocks configuration */
    M_CL_CONFIG mclBlkConfig = {0, 0, NULL, 0};

/* cluster blocks config table */
    CL_DESC     clDescTbl [] = { {MOT_FCC_MAX_CL_LEN, 0, NULL, 0}};

/* number of different clusters sizes in pool -- only 1 */
    int     clDescTblNumEnt = 1;

    /* initialize the netPool */
    if ( (pDrvCtrl->endObj.pNetPool = malloc (sizeof (NET_POOL))) == NULL )
        {
        return ERROR;
        }
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
                return ERROR;

            rbdMemSize = MOT_FCC_RBD_SZ * pDrvCtrl->rbdNum;
            tbdMemSize = MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum;
            bdMemSize = rbdMemSize + tbdMemSize + MOT_FCC_BD_ALIGN;

            /* Allocated from dma safe memory */
            pDrvCtrl->pBdOrig = pDrvCtrl->pBdBase = cacheDmaMalloc(bdMemSize);
            if ( pDrvCtrl->pBdBase == NULL )
                return ERROR;    /* no memory available */

            pDrvCtrl->bdSize = bdMemSize;
            MOT_FCC_FLAG_SET (MOT_FCC_OWN_BD_MEM);
            break;

        default :  
            /* The user provided an area for the BDs  */
            if ( !pDrvCtrl->bdSize )
                return ERROR;

            /*
             * check whether user provided a number for Rx and Tx BDs
             * fill in the blanks if we can.
             * check whether enough memory was provided, etc.
             */

            if (MOT_FCC_FLAG_ISSET (MOT_FCC_INV_TBD_NUM) && 
                MOT_FCC_FLAG_ISSET (MOT_FCC_INV_RBD_NUM))
                {
                pDrvCtrl->tbdNum = pDrvCtrl->bdSize / (MOT_FCC_TBD_SZ + MOT_FCC_RBD_SZ);
                pDrvCtrl->rbdNum = pDrvCtrl->tbdNum;
                }
            else if (MOT_FCC_FLAG_ISSET (MOT_FCC_INV_TBD_NUM))
                {
                rbdMemSize = MOT_FCC_RBD_SZ * pDrvCtrl->rbdNum;
                pDrvCtrl->tbdNum = (pDrvCtrl->bdSize - rbdMemSize) / MOT_FCC_TBD_SZ;
                }
            else if (MOT_FCC_FLAG_ISSET (MOT_FCC_INV_RBD_NUM))
                {
                tbdMemSize = MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum;
                pDrvCtrl->rbdNum = (pDrvCtrl->bdSize - tbdMemSize) / MOT_FCC_RBD_SZ;
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

    /* zero the shared memory */
    memset (pDrvCtrl->pBdBase, 0, (int) pDrvCtrl->bdSize);

    /* align the shared memory */
    pDrvCtrl->pBdBase = (char *) ROUND_UP((UINT32)pDrvCtrl->pBdBase,MOT_FCC_BD_ALIGN);

    /*
     * number of clusters, including loaning buffers, a min number
     * of transmit clusters for copy-mode transmit, and one transmit
     * cluster for polling operation.
     */

    clNum = (2 * pDrvCtrl->rbdNum) + MOT_FCC_BD_LOAN_NUM + MOT_FCC_TX_POLL_NUM + MOT_FCC_TX_CL_NUM;

    /* pool of mblks */
    if ( mclBlkConfig.mBlkNum == 0 )
        {
        mclBlkConfig.mBlkNum = clNum * 2;
        }

    /* pool of clusters, including loaning buffers */
    if ( clDescTbl[0].clNum == 0 )
        {
        clDescTbl[0].clNum = clNum;
        clDescTbl[0].clSize = MOT_FCC_MAX_CL_LEN;
        }

    /* there's a cluster overhead and an alignment issue */
    clDescTbl[0].memSize = 
        clDescTbl[0].clNum * (clDescTbl[0].clSize + CL_OVERHEAD) + CL_ALIGNMENT;

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
             * Allocate from cachable data block.
             */ 
            clDescTbl[0].memArea = 
                (char *) memalign(CL_ALIGNMENT, clDescTbl[0].memSize);
            if ( clDescTbl[0].memArea == NULL )
                return ERROR;

            /* store the pointer to the clBlock area and its size */
            pDrvCtrl->pBufOrig = pDrvCtrl->pBufBase = clDescTbl[0].memArea;
            pDrvCtrl->bufSize = clDescTbl[0].memSize;
            MOT_FCC_FLAG_SET (MOT_FCC_OWN_BUF_MEM);

            /* cache functions descriptor for data buffers */

            pDrvCtrl->bufCacheFuncs.flushRtn = cacheFlush;
            pDrvCtrl->bufCacheFuncs.invalidateRtn = cacheInvalidate;
            pDrvCtrl->bufCacheFuncs.virtToPhysRtn = NULL;
            pDrvCtrl->bufCacheFuncs.physToVirtRtn = NULL;
            break;

        default :                
            /* 
             * The user provides the area for buffers. This must be from a 
             * non cachable area.
             */
            if ( pDrvCtrl->bufSize == 0 )
                return ERROR;

            /*
             * check the user provided enough memory with reference
             * to the given number of receive/transmit frames, if any.
             */
            if ( pDrvCtrl->bufSize < clDescTbl[0].memSize )
                return ERROR;

            /* Set memArea to the buffer base */

            clDescTbl[0].memArea = pDrvCtrl->pBufBase;
            MOT_FCC_FLAG_CLEAR (MOT_FCC_OWN_BUF_MEM);
            pDrvCtrl->bufCacheFuncs = cacheNullFuncs;
            break;
        }

    /* zero and align the shared memory */
    memset (pDrvCtrl->pBufBase, 0, (int) pDrvCtrl->bufSize);
    pDrvCtrl->pBufBase = (char *) ROUND_UP((UINT32)pDrvCtrl->pBufBase,CL_ALIGNMENT);

    /* pool of cluster blocks */
    if ( mclBlkConfig.clBlkNum == 0 )
        {
        mclBlkConfig.clBlkNum = clDescTbl[0].clNum;
        }
    /* get memory for mblks */
    if ( mclBlkConfig.memArea == NULL )
        {
        /* memory size adjusted to hold the netPool pointer at the head */
        mclBlkConfig.memSize = ((mclBlkConfig.mBlkNum * (M_BLK_SZ + MBLK_ALIGNMENT))
                                + (mclBlkConfig.clBlkNum * (CL_BLK_SZ + CL_ALIGNMENT)));
        mclBlkConfig.memArea = (char *) memalign (MBLK_ALIGNMENT, mclBlkConfig.memSize);
        if ( mclBlkConfig.memArea == NULL )
            {
            return ERROR;
            }
        /* store the pointer to the mBlock area */
        pDrvCtrl->pMBlkArea = mclBlkConfig.memArea;
        pDrvCtrl->mBlkSize = mclBlkConfig.memSize;
        }

    /* init the mem pool */

    if ( netPoolInit (pDrvCtrl->endObj.pNetPool, &mclBlkConfig, &clDescTbl[0], clDescTblNumEnt, NULL) == ERROR )
        return ERROR;

    if ( (pDrvCtrl->pClPoolId = netClPoolIdGet (pDrvCtrl->endObj.pNetPool, MOT_FCC_MAX_CL_LEN, FALSE)) == NULL )
        return ERROR;

    /* allocate receive buffer list */
    pDrvCtrl->rBufList = (char **) calloc ( pDrvCtrl->rbdNum, sizeof (char *) );
    if (pDrvCtrl->rBufList == NULL)
        return ERROR;

    /* allocate the CL_BLK buffer list */
    pDrvCtrl->tBufList = (M_BLK **) calloc( pDrvCtrl->tbdNum, sizeof(M_BLK *) );
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
* INTERNAL
* The speed field inthe phyInfo structure is only set after the call
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

    /* Initialize the transmit buffer descripters */
    if (motFccTbdInit(pDrvCtrl) == ERROR)
        return ERROR;

    /* Initialize the receive buffer descripters */
    if (motFccRbdInit(pDrvCtrl) == ERROR)
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
    int             retVal;         /* convenient holder for return value */
    UINT8           command = 0;    /* command to issue to the CP */
    FCC_REG_T     * reg;            /* an even bigger convenience */
    int             intLevel;
    MOT_FCC_RBD_ID  pRbd = NULL;   /* generic rbd pointer */
    void          * pBuf;
    UINT16          ix;            /* a counter */
    M_BLK         * pMblkFree;

    /* check if already stopped */
    if ((pDrvCtrl->state & MOT_FCC_STATE_RUNNING) != MOT_FCC_STATE_RUNNING)
        return OK;

    reg = pDrvCtrl->fccReg;

    /* clear pending event */
    reg->fcc_fcce  = M8260_FEM_ETH_GRA; 

    /* turn event on */
    reg->fcc_fccm |= M8260_FEM_ETH_GRA;  

    MOT_FCC_LOG ( MOT_FCC_DBG_START,"motFccStop\n", 1, 2, 3, 4, 5, 6);

    /* mark the interface as down */
    END_FLAGS_CLR ( &pDrvCtrl->endObj, (IFF_UP | IFF_RUNNING) );

    /* issue a graceful stop transmit command to the CP */
    command = M8260_CPCR_TX_GRSTOP;
    if ( motFccCpcrCommand (pDrvCtrl, command) == ERROR )
        return ERROR;

    /* wait for the related interrupt */
    semTake (pDrvCtrl->graSem, WAIT_FOREVER);

    intLevel = intLock ();

    /* disable the transmitter and receiver */
    reg->fcc_gfmr &= ~(M8260_GFMR_ENT | M8260_GFMR_ENR); 

    /* mask chip interrupts */
    pDrvCtrl->fccReg->fcc_fccm = 0;
    MOT_FCC_REG_WORD_WR ((UINT32) M8260_FGMR1 (pDrvCtrl->immrVal) + 
         ((pDrvCtrl->fccNum - 1) * M8260_FCC_IRAM_GAP), MOT_FCC_CLEAR_VAL);

    /* disable system interrupt: reset relevant bit in SIMNR_L */
    SYS_FCC_INT_DISABLE (pDrvCtrl, retVal);
    intUnlock (intLevel);
    if (retVal == ERROR)
        return ERROR;

    /* call the BSP to disable the MII interface */

    SYS_FCC_ENET_DISABLE;

    /*
     * Now that chip interrupts are disabled, it's impossible for
     * any instances of the RX or TX handlers being queued up in
     * tNetTask. However, there may be an existing job still
     * running/pending. We have to wait here for them to expire.
     */

    for (ix = 0; ix < 1000; ix++)
        {
        if (pDrvCtrl->rxJobPending == FALSE && pDrvCtrl->txJobPending == FALSE)
            break;
        taskDelay(1);
        }

    if (ix == 1000)
        logMsg("motfcc%d: job still pending in tNetTask",
            pDrvCtrl->unit, 0, 0, 0, 0, 0);

    /* free buffer descriptors */

    pRbd = (MOT_FCC_RBD_ID) (pDrvCtrl->pRbdBase);

    for (ix = 0; ix < pDrvCtrl->rbdNum; ix++)
        {
        /* return the cluster buffers to the pool */
        pBuf = pDrvCtrl->rBufList[ix];
        if (pBuf != NULL)
            {
            /* free the tuple */
            netMblkClChainFree ((M_BLK_ID)pBuf);
            pDrvCtrl->rBufList[ix] = NULL;
            }
        }

    for (ix = 0; ix < pDrvCtrl->tbdNum; ix++)
        {
        /* return the cluster headers to the pool */
        pMblkFree = pDrvCtrl->tBufList[ix];
        if (pMblkFree != NULL)
            {
            netMblkClChainFree (pMblkFree);
            pDrvCtrl->tBufList[ix] = NULL;
            }
        }

    if ((pDrvCtrl->pTxPollBuf) != NULL)
        netClFree (pDrvCtrl->endObj.pNetPool, pDrvCtrl->pTxPollBuf);

    /*
     * At this point, there should be 0 clusters outstanding.
     * If not, wait for them to be released by whoever owns them.
     * Note that if the WDB agent is using this interface, it will
     * have allocated some buffers from us, so this check can
     * fail if the WDB agent is running.
     */

    for (ix = 0; ix < 100; ix++)
        {
        if (pDrvCtrl->pClPoolId->clNum == pDrvCtrl->pClPoolId->clNumFree)
            break;
        taskDelay(1);
        }

    if (ix == 1000)
        logMsg("motfcc%d: still %d clusters outstanding",
            pDrvCtrl->unit, pDrvCtrl->pClPoolId->clNum -
            pDrvCtrl->pClPoolId->clNumFree, 0, 0, 0, 0);

    pDrvCtrl->state &= ~MOT_FCC_STATE_RUNNING;

    return OK;
}
                   
/*******************************************************************************
* motFccInt - entry point for handling interrupts from the FCC
*
* The interrupting events are acknowledged to the device. The device
* will de-assert its interrupt signal.  The amount of work done here is kept
* to a minimum; the bulk of the work is deferred to the netTask. However, the 
* interrupt code takes on the resposibility to clean up the TX and the RX
* BD rings. 
*
* RETURNS: N/A
*/

LOCAL void motFccInt 
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    UINT16      event;      /* event intr register */
    UINT16      status;     /* status word */
    UINT16      muxError;
    FCC_REG_T   *reg; 

    WV_INT_ENTRY(0,0);

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numInts);

    muxError = 0;

    reg = pDrvCtrl->fccReg;

    /* get pending events */
    event = reg->fcc_fcce;

    /* clear pending events */
    reg->fcc_fcce = event;  

    /* abstract current status */
    status = event & reg->fcc_fccm;

    /* NULL status event; interrupt but no events to process */
    if ( status == 0 )
        {
        WV_INT_EXIT(0,0);
        return;
        }

    /* Process a graceful stop event. */
    if ( status & M8260_FEM_ETH_GRA )
        {
        WV_INT_GRA_ENTRY(0,0);
        /* 
        * motFccStop has been called, graceful stop event.
        * sync with motFccStop 
        */
        semGive (pDrvCtrl->graSem);
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numGRAInts);

        status &= ~M8260_FEM_ETH_GRA;
        WV_INT_GRA_EXIT(0,0);
        }

    if ( status & M8260_FEM_ETH_RXC )
        {
        WV_INT_RXC_ENTRY(0,0);
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXCInts);
        status &= ~M8260_FEM_ETH_RXC;
        WV_INT_RXC_EXIT(0,0);
        }

    if ( status & M8260_FEM_ETH_TXC )
        {
        WV_INT_TXC_ENTRY(0,0);
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numTXCInts);
        status &= ~M8260_FEM_ETH_TXC;
        WV_INT_TXC_EXIT(0,0);
        }

    /* Process a RX busy or RX done event */
    if (event & (M8260_FEM_ETH_BSY|M8260_FEM_ETH_RXF) &&
        pDrvCtrl->rxJobPending == FALSE)
        {
        WV_INT_BSY_ENTRY(0,0);
        WV_INT_RXF_ENTRY(0,0);
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccRxBsyErr);
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXFInts);
        if(netJobAdd ((FUNCPTR)motFccHandleRXFrames, (int)pDrvCtrl, 0,0,0,0) == OK)
			{
			pDrvCtrl->intMask &= ~(M8260_FEM_ETH_RXF|M8260_FEM_ETH_BSY);
			pDrvCtrl->fccReg->fcc_fccm = pDrvCtrl->intMask;
			pDrvCtrl->rxJobPending = TRUE;
			}
        WV_INT_RX_EXIT(0,0);
        WV_INT_BSY_EXIT(0,0);
        }

    /* Handle tx error or TX done event */
    if (event & (M8260_FEM_ETH_TXE|M8260_FEM_ETH_TXB) &&
        pDrvCtrl->txJobPending == FALSE)
        {
        WV_INT_TXE_ENTRY(0,0);
        WV_INT_TXB_ENTRY(0,0);
        MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numTXEInts);
        if(netJobAdd ((FUNCPTR)motFccTbdClean, (int)pDrvCtrl, 0,0,0,0) == OK)
			{
			pDrvCtrl->intMask &= ~(M8260_FEM_ETH_TXB|M8260_FEM_ETH_TXE);
			pDrvCtrl->fccReg->fcc_fccm = pDrvCtrl->intMask;
			pDrvCtrl->txJobPending = TRUE;
			}		
        WV_INT_TXB_EXIT(0,0);
        WV_INT_TXE_EXIT(0,0);
        }

    WV_INT_EXIT(0,0);
    }


/*******************************************************************************
* motFccTxErrReinit - Special TX errors can only be cleared by a restart of
* the FCC transmitter. This funtion re-initializes the transmitter and then
* restarts. The TX BD ring is adjusted accordingly. 
*
*
* RETURNS: N/A
*/

LOCAL void motFccTxErrReInit
    (
    DRV_CTRL *pDrvCtrl,
    UINT16    stat
    )
    {
    VUINT32     cpcrVal;
    FCC_REG_T * reg = pDrvCtrl->fccReg;
    UINT32      immrVal = pDrvCtrl->immrVal; /* holder for the immr value */
    UINT8       fccNum  = pDrvCtrl->fccNum;  /* holder for the fcc number */
    UINT8       fcc     = fccNum - 1;
    UINT16      bdStat;

    reg->fcc_gfmr  &= ~M8260_GFMR_ENT;


    bdStat = pDrvCtrl->pTbdLast->bdStat;
    pDrvCtrl->pTbdLast->bdStat &= ~M8260_FETH_TBD_R;

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
    if ( bdStat & M8260_FETH_TBD_R )
        pDrvCtrl->pTbdLast->bdStat |= M8260_FETH_TBD_R;

    MOT_FCC_LOG ( MOT_FCC_DBG_INT_TX_ERR, 
        ("motFccTxErrReInit tTBDPTR 0x%08x Last 0x%08x Next 0x%08x Stat 0x%04x\n"),
         (int)pDrvCtrl->fccPar->tbptr,
         (int)pDrvCtrl->pTbdLast,
         (int)pDrvCtrl->pTbdNext,stat,5,6);
    }

/*******************************************************************************
*
* motFccEncap - Encapsulate a packet in the TX DMA ring
*
* This routine packages up an mBlk chain into the TX DMA ring starting
* at the first available TX descriptor. If it turns out there aren't enough
* TX descriptors to hold all the packet fragments, we revert all the
* changes we made and return error.
*
* RETURNS: OK, or ERROR, if no resources are available
*/

#define MOT_FCC_MAX_TX_FRAGS 16

#define MOT_TBUF(pRbd)			\
    pDrvCtrl->tBufList[(pTbd) - pDrvCtrl->pTbdBase]

LOCAL int motFccEncap
    (
    DRV_CTRL * pDrvCtrl,
    M_BLK    * pMblk
    )
    {
    FCC_BD    * pTbdFirst, *pTbd = NULL;
    M_BLK_ID    pCurr;
    UINT32      used;
    int         ix;

    /* save the first TBD pointer */

    pTbdFirst = pDrvCtrl->pTbdNext;

    for (pCurr = pMblk, used = 0; pCurr != NULL; pCurr = pCurr->m_next)
        {
        if (pCurr->m_len != 0)
            {
            if (used == MOT_FCC_MAX_TX_FRAGS || pDrvCtrl->tbdFree == 0)
                break;

            /* get the current pTbd */

            pTbd = pDrvCtrl->pTbdNext;

            /*
             * Sanity check: if the descriptor is still in use,
             * we're out of descriptors.
             */

            if (pTbd->bdStat & M8260_FETH_TBD_R)
                break;

            /* Set up len, buffer addr, descriptor flags. */

            pTbd->bdLen = pCurr->m_len;
            pTbd->bdAddr = (UINT32) pCurr->m_data;
            pTbd->bdStat &= M8260_FETH_TBD_W;
            pTbd->bdStat |= M8260_FETH_TBD_I|M8260_FETH_TBD_TC|M8260_FETH_TBD_PAD;

            MOT_FCC_CACHE_FLUSH (pCurr->m_data, pCurr->m_len);

            /*
             * Set the ready bit everywhere except the first
             * descriptor.
             */

            if (used)
                pTbd->bdStat |= M8260_FETH_TBD_R;

            if (pTbd->bdStat & M8260_FETH_TBD_W)
                pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
            else
                pDrvCtrl->pTbdNext++;

            used++;
            pDrvCtrl->tbdFree--;
            }
        }

    if (pCurr != NULL)
        goto noDescs;

    MOT_TBUF (pTbd) = pMblk;
    pTbd->bdStat |= M8260_FETH_TBD_I | M8260_FETH_TBD_L;
    pTbdFirst->bdStat |= M8260_FETH_TBD_R;
    pDrvCtrl->tbdCnt += used;

    CACHE_PIPE_FLUSH ();

    return (OK);

noDescs:

    /*
     * Ran out of descriptors: undo all relevant changes
     * and fall back to copying.
     */

    pTbd = pDrvCtrl->pTbdNext = pTbdFirst;
    for (ix = 0; ix < used; ix++)
        {
        pDrvCtrl->tbdFree++;
        pTbd->bdStat &= M8260_FETH_TBD_W;
        pTbd->bdAddr = (UINT32) NULL;
        if (pTbd->bdStat & M8260_FETH_TBD_W)
            pTbd = pDrvCtrl->pTbdBase;
        else
            pTbd++;
        }

    return (ERROR);
    }


/*******************************************************************************
* motFccSend - send an Ethernet packet
*
* This routine() takes a M_BLK_ID and sends off the data in the M_BLK_ID.
* The buffer must already have the addressing information properly installed
* in it. This is done by a higher layer.
*
* RETURNS: OK, or END_ERR_BLOCK, if no resources are available, or ERROR,
* if the device is currently working in polling mode.
*/

LOCAL STATUS motFccSend
    (
    DRV_CTRL * pDrvCtrl,
    M_BLK    * pMblk
    )
    {
    int rval;
    int len;
    M_BLK_ID pTmp;

    END_TX_SEM_TAKE(&pDrvCtrl->endObj, WAIT_FOREVER);

    /* check device mode */
    if (MOT_FCC_FLAG_ISSET (MOT_FCC_POLLING))
        {
        /* free the given mBlk chain */
        netMblkClChainFree (pMblk);      
        END_TX_SEM_GIVE (&pDrvCtrl->endObj);
        return (ERROR);
        }

    if (pDrvCtrl->txStall == TRUE || pDrvCtrl->tbdFree == 0)
        goto send_block;

    /*
     * First, try to do an in-place transmission, using
     * gather-write DMA.
     */

    rval = motFccEncap (pDrvCtrl, pMblk);

    /*
     * In-place DMA didn't work, coalesce everything
     * into a single buffer and try again.
     */

    if (rval == ERROR)
        {
        if ((pTmp = netTupleGet (pDrvCtrl->endObj.pNetPool,
                                  MOT_FCC_MAX_CL_LEN, M_DONTWAIT,
                                  MT_DATA, 0)) == NULL)
            goto send_block;

        len = netMblkToBufCopy (pMblk, mtod(pTmp, char *), NULL);
        pTmp->m_len = pTmp->m_pkthdr.len = len;
        pTmp->m_flags = pMblk->m_flags;
        netMblkClChainFree (pMblk);
        pMblk = pTmp;
        /* Try transmission again, must succeed this time. */
        rval = motFccEncap (pDrvCtrl, pMblk);
        }

    if (rval == ERROR)
        goto send_block;

    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    return (OK);

send_block:

    pDrvCtrl->txStall = TRUE;
    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numStallsEntered);
    /* update mib statistics */
    END_INC_OUT_ERRS();
    END_TX_SEM_GIVE (&pDrvCtrl->endObj);

    return (END_ERR_BLOCK);
    }

/******************************************************************************
*
* motFccTbdClean - clean the transmit buffer descriptors ring
*
* This routine may run in netTask's context or from an interrupt. It cleans the 
* transmit buffer descriptors ring. It also calls motFccTxErrReInit() 
* for any transmit errors that need a restart to be cleared.
*
* SEE ALSO: motFccTxErrReInit()
*
* RETURNS: N/A.
*/
LOCAL int motFccTbdClean 
    (
    DRV_CTRL *pDrvCtrl
    )
    {
    FCC_BD * pTbd;
    M_BLK  * pMblkFree;
    UINT16   stat;
    BOOL     doRestart = FALSE;

    END_TX_SEM_TAKE(&pDrvCtrl->endObj, WAIT_FOREVER);

    while (pDrvCtrl->tbdCnt)
        {
        /* get the next bd to be processed */
        pTbd  = pDrvCtrl->pTbdLast;

        /* check if buffer is owned by the CP */
        if (pTbd->bdStat & M8260_FETH_TBD_R)
            break;

        if (pTbd->bdAddr == (UINT32)NULL)
            break;

        stat = pTbd->bdStat;

        /* get the Mblk chain for that BD */

        pMblkFree = pDrvCtrl->tBufList[pDrvCtrl->pTbdLast-pDrvCtrl->pTbdBase];
        
        /* Free the Mblk Chain */
        if (pMblkFree != NULL)
            {
            netMblkClChainFree (pMblkFree); 
            pDrvCtrl->tBufList[pDrvCtrl->pTbdLast-pDrvCtrl->pTbdBase] = NULL;
            }

       /* use wrap bit to determine the end of the bd's */
        pTbd->bdLen  = 0;
        pTbd->bdAddr = (UINT32) NULL;

        if (pTbd->bdStat & M8260_FETH_TBD_W)
            pDrvCtrl->pTbdLast = pDrvCtrl->pTbdBase;
        else
            pDrvCtrl->pTbdLast++;

        /* Inc the free count  */
        pDrvCtrl->tbdFree++;
        pDrvCtrl->tbdCnt--;

        /* check for error conditions */
        if (stat & 
             ( M8260_FETH_TBD_CSL |   /* carrier sense lost */
               M8260_FETH_TBD_UN  |   /* under run */
               M8260_FETH_TBD_HB  |   /* heartbeat */
               M8260_FETH_TBD_LC  |   /* Late collision */
               M8260_FETH_TBD_RL  |   /* Retransmission Limit reached */
               M8260_FETH_TBD_DEF )) /* defer indication */
            {

            /* Process mib statistics */

            END_INC_OUT_ERRS();

            if (stat & M8260_FETH_TBD_CSL)
                {
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxCslErr);
                }
            if (stat & M8260_FETH_TBD_UN)
                {
                motFccTxErrReInit(pDrvCtrl, stat);
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxUrErr);
                }
            if (stat & M8260_FETH_TBD_HB)
                {
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccHbFailErr);
                if (pDrvCtrl->hbFailFunc != NULL)
                    (*pDrvCtrl->hbFailFunc)(pDrvCtrl);
                }
            if (stat & M8260_FETH_TBD_LC)
                {
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxLcErr);
                }
            if (stat & M8260_FETH_TBD_RL)
                {
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxRlErr);
                }
            if (stat & M8260_FETH_TBD_DEF)
                {
                MOT_FCC_STAT_INCR (pDrvCtrl->Stats->motFccTxDefErr);
                }

            if (stat & (M8260_FETH_TBD_RL|M8260_FETH_TBD_LC))
                motFccTxErrReInit(pDrvCtrl, stat);
            }
        }

    pDrvCtrl->txJobPending = FALSE;
    pDrvCtrl->intMask |= (M8260_FEM_ETH_TXB|M8260_FEM_ETH_TXE);
    pDrvCtrl->fccReg->fcc_fccm = pDrvCtrl->intMask;

    if (pDrvCtrl->txStall == TRUE && pDrvCtrl->tbdFree)
        {
        pDrvCtrl->txStall = FALSE;
        doRestart = TRUE;
        }

    END_TX_SEM_GIVE(&pDrvCtrl->endObj);

    if (doRestart == TRUE)
        muxTxRestart (&pDrvCtrl->endObj);

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
    UINT16  ix;     /* a counter */
    FCC_BD *pTbd;   /* generic TBD pointer */

    /* carve up the shared-memory region */
    pTbd = (FCC_BD *) pDrvCtrl->pBdBase;

    pDrvCtrl->pTbdBase = pTbd;
    pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
    pDrvCtrl->pTbdLast = pDrvCtrl->pTbdBase;
    pDrvCtrl->tbdFree  = pDrvCtrl->tbdNum;
    pDrvCtrl->tbdCnt   = 0;

    for ( ix = 0; ix < pDrvCtrl->tbdNum; ix++ )
        {
        pDrvCtrl->tBufList[ix] = NULL;
        pTbd->bdStat = 0;
        pTbd->bdLen  = 0;
        pTbd->bdAddr = 0;
        ++pTbd;
        }

    /* set the wrap bit in last BD */
    --pTbd;
    pTbd->bdStat = M8260_FETH_TBD_W;

    /* set stall variables to default values */
    pDrvCtrl->txStall = FALSE;

    /* wait for 75% available before restart MUX */
    pDrvCtrl->txUnStallThresh = pDrvCtrl->tbdNum-(pDrvCtrl->tbdNum/2);

    /* wait for 25% available to enabling INTS */
    pDrvCtrl->txStallThresh   =  pDrvCtrl->tbdNum/4;

    /* get a cluster buffer from the pool */
    pDrvCtrl->pTxPollBuf = 
        netClusterGet (pDrvCtrl->endObj.pNetPool, pDrvCtrl->pClPoolId);

    if ( pDrvCtrl->pTxPollBuf == NULL )
        return ERROR;

    return OK;
    }

/******************************************************************************
*
* motFccHandleRXFrames - task-level receive frames processor
*
* This routine is run in netTask's context.
*
* RETURNS: N/A
*/

#define MOT_RBUF(pRbd)			\
    pDrvCtrl->rBufList[(pRbd) - pDrvCtrl->pRbdBase]
#define MOT_FCC_MAX_RX_FRAMES 16

LOCAL void motFccHandleRXFrames(DRV_CTRL *pDrvCtrl)
    {
    FCC_BD * pRbd;      /* pointer to current receive bd */
    M_BLK  * pMblk;     /* MBLK to send upstream */
    M_BLK  * pNewMblk;
    int      loopCounter = MOT_FCC_MAX_RX_FRAMES;

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numRXFHandlerEntries);

    pRbd = pDrvCtrl->pRbdNext;

    while (loopCounter && !(pRbd->bdStat & M8260_FETH_RBD_E))
        {

        /* Handle errata CPM-21... this code should NOT break the driver
         * when CPM-21 is ultimately fixed.
         */

        if ((pRbd->bdStat & (M8260_FETH_RBD_CL | M8260_FETH_RBD_CR)) ==
            M8260_FETH_RBD_CL)
            pRbd->bdStat &= (UINT16)(~M8260_FETH_RBD_CL);

        /*
         * Check for RX errors. If any of the error bits are set in
         * the descriptor, reintialize the descriptor and move on to
         * the next one.
         */

        if (pRbd->bdStat & MOT_FCC_RBD_ERR)
            {
            END_INC_IN_ERRS();
            goto do_skip;
            }

        /*
         * Try to conjure up a new mBlk tuple to replace the one
         * we're about to loan out. If that fails, leave the
         * old one in place and reset the descriptor to allow
         * a new packet to be received.
         */

        pNewMblk = netTupleGet (pDrvCtrl->endObj.pNetPool,
            MOT_FCC_MAX_CL_LEN, M_DONTWAIT, MT_DATA, 0);

        if (pNewMblk == NULL)
            {
do_skip:
            if (pRbd->bdStat & M8260_FETH_RBD_W)
                pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
            else
                pDrvCtrl->pRbdNext++;
            pRbd->bdStat |= M8260_FETH_RBD_E | M8260_FETH_RBD_I;
            pRbd->bdLen = 0;
            goto do_next;
            }

        pNewMblk->m_data = NET_TO_MOT_FCC_BUF(pNewMblk->m_data);

        pMblk = (M_BLK_ID)MOT_RBUF(pRbd);
        MOT_RBUF(pRbd) = (char *)pNewMblk;

        pMblk->m_len        = (pRbd->bdLen & ~0xc000) - MII_CRC_LEN;
        pMblk->m_pkthdr.len = pMblk->m_len;
        pMblk->m_flags      = M_PKTHDR|M_EXT;

        MOT_FCC_CACHE_INVAL (pMblk->m_data, pMblk->m_len);

        pRbd->bdAddr = (UINT32)pNewMblk->m_data;
        pRbd->bdLen = 0;

        /* update mib Statistics */
        if (pRbd->bdStat & (M8260_FETH_RBD_MC | M8260_FETH_RBD_BC))
            {
            END_INC_IN_NUCAST (pMblk->m_data, pMblk->m_len);
            }
        else
            {
            END_INC_IN_UCAST (pMblk->m_data, pMblk->m_len);
            }

        pRbd->bdStat |= M8260_FETH_RBD_E | M8260_FETH_RBD_I;

        if (pRbd->bdStat & M8260_FETH_RBD_W)
            pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
        else
            pDrvCtrl->pRbdNext++;

        END_RCV_RTN_CALL (&pDrvCtrl->endObj, pMblk);

do_next:
        loopCounter++;
        pRbd = pDrvCtrl->pRbdNext;
        }

    /*
     * If we exceeded our work quota, queue ourselves up for
     * another round of processing. Otherwise, re-enable interrupts
     * and just bail.
     */

    if (loopCounter == 0)
        {
        netJobAdd ((FUNCPTR)motFccHandleRXFrames, (int)pDrvCtrl, 0,0,0,0);
        return;
        }

    pDrvCtrl->rxJobPending = FALSE;
    pDrvCtrl->intMask |= M8260_FEM_ETH_RXF | M8260_FEM_ETH_BSY;
    pDrvCtrl->fccReg->fcc_fccm = pDrvCtrl->intMask;

    return;
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
    UINT16  ix;         /* a counter */
    M_BLK   *pMblk;

   /*  located the receive ring after the transmit ring */
    pDrvCtrl->pRbdBase = (FCC_BD *) (pDrvCtrl->pBdBase + (MOT_FCC_TBD_SZ * pDrvCtrl->tbdNum) );

    /* point allocation bd pointers to the first bd */
    pDrvCtrl->pRbdLast = pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;

    /* clear out the ring */
    pRbd = pDrvCtrl->pRbdBase;
    for (ix = 0; ix < pDrvCtrl->rbdNum; ix++)
        {
        pMblk = netTupleGet (pDrvCtrl->endObj.pNetPool,
            MOT_FCC_MAX_CL_LEN, M_DONTWAIT, MT_DATA, 0);
        pMblk->m_data = NET_TO_MOT_FCC_BUF(pMblk->m_data);
        pDrvCtrl->rBufList[ix] = (char *)pMblk;
        pRbd->bdStat = M8260_FETH_RBD_E|M8260_FETH_RBD_I;
        pRbd->bdLen  = 0;
        pRbd->bdAddr = (UINT32)pMblk->m_data;
        /* inc pointer to the next bd */
        ++pRbd;   
        }

    /* set the last bd to wrap */
    --pRbd;
    pRbd->bdStat |= M8260_FETH_RBD_W;

    pDrvCtrl->rxStall = FALSE;
    /* wait for 75% available before restarting receiver to MUX */
    pDrvCtrl->rxUnStallThresh = pDrvCtrl->rbdNum-(pDrvCtrl->rbdNum/2); 
    /* wait for 25% available before sending MUX error */
    pDrvCtrl->rxStallThresh   = pDrvCtrl->rbdNum/2; 

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
    pParam = pDrvCtrl->fccPar;
    pEthPar = pDrvCtrl->fccEthPar;
    memset(pParam,0,sizeof(*pParam));

    /* figure out the value for the FCR registers */
    fcrVal = MOT_FCC_FCR_DEF_VAL;
    if (MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_BD_LBUS))
        {
        fcrVal |= M8260_FCR_BDB;
        }

    if (MOT_FCC_USR_FLAG_ISSET (MOT_FCC_USR_BUF_LBUS))
        {
        fcrVal |= M8260_FCR_DTB;
        }

    fcrVal <<= M8260_FCR_SHIFT;

    pParam->riptr = (VUSHORT)((UINT32)0xffff & (UINT32) pDrvCtrl->riPtr);
    pParam->tiptr = (VUSHORT)((UINT32)0xffff & (UINT32) pDrvCtrl->tiPtr);

    /* clear pad area to zero */

    memset (pDrvCtrl->padPtr, 0, 32);
    pEthPar->pad_ptr = (VUSHORT)((UINT32)0xffff & (UINT32) pDrvCtrl->padPtr);

    pParam->mrblr   = MII_ETH_MAX_PCK_SZ;
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

    phyAddr = END_HADDR (pDrvCtrl->endObj);
    retVal = motFccAddrSet (pDrvCtrl, phyAddr, M8260_FCC_PADDR_H_OFF);

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
    UINT8       command = 0;            /* command to issue to the CP */

    MOT_FCC_LOG ( MOT_FCC_DBG_START, "motFccIramInit\n", 1, 2, 3, 4, 5, 6);

    /*
     * program the GSMR, but enable neither the transmitter
     * nor the receiver for now.
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
     * we enable the following event interrupts:
     * graceful transmit command;
     * tx error;
     * rx frame;
     * busy condition;
     * tx buffer, although we'll only enable the last TBD in a frame
     * to generate interrupts. See motFccSend ();
     */

    pDrvCtrl->intMask = ( M8260_FEM_ETH_TXE | M8260_FEM_ETH_TXB |
                          M8260_FEM_ETH_RXF | M8260_FEM_ETH_BSY | 
                          M8260_FEM_ETH_GRA ); 

    reg->fcc_fccm = pDrvCtrl->intMask;

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
* will contain the address.
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

    word = (((UINT16) (pAddr [5])) << 8) | (pAddr [4]);
    MOT_FCC_REG_WORD_WR (fccPramAddr + offset,
             word);

    word = (((UINT16) (pAddr [3])) << 8) | (pAddr [2]);
    MOT_FCC_REG_WORD_WR (fccPramAddr + offset + 2,
             word);

    word = (((UINT16) (pAddr [1])) << 8) | (pAddr [0]);
    MOT_FCC_REG_WORD_WR (fccPramAddr + offset + 4,
             word);

    return (OK);
    }

/**************************************************************************
*
* motFccFpsmrValSet - set the proper value for the FPSMR
*
* This routine finds out the proper value to be programmed in the
* FPSMR register by looking at some of the user/driver flags. It deals
* with options like promiscous mode, full duplex, loopback mode, RMON
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

    /* deal with promiscous mode */
    if ( MOT_FCC_FLAG_ISSET (MOT_FCC_PROM) )
        fpsmrVal |= M8260_FPSMR_ETH_PRO;
    else
        fpsmrVal &= ~M8260_FPSMR_ETH_PRO;

    /* Handle user's diddling with LPB first. That way, if we must set LPB due to FD it will stick. */

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

LOCAL int motFccIoctl
    (
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    int         cmd,            /* command to process */
    caddr_t     data            /* pointer to data */
    )
    {
    int         error = OK;
    INT8        savedFlags;
    long        value;
    END_OBJ *   pEndObj=&pDrvCtrl->endObj;

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

                    /* program the FPSMR to promiscous mode */

                    if (motFccFpsmrValSet (pDrvCtrl) == ERROR)
                        return(ERROR);

                    /* config up */

                    END_FLAGS_SET (pEndObj, IFF_UP | IFF_RUNNING);
                    }
                }
            else
                MOT_FCC_FLAG_CLEAR (MOT_FCC_PROM);

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


#ifdef INCLUDE_RFC_2233

        case EIOCGMIB2233:
            if ((data == NULL) || (pEndObj->pMib2Tbl == NULL))
                error = EINVAL;
            else
                *((M2_ID **)data) = pEndObj->pMib2Tbl;
            break;
#else
        case EIOCGMIB2:
            if (data == NULL)
                error=EINVAL;
            else
                bcopy ((char *) &pEndObj->mib2Tbl, (char *) data,
                       sizeof (pEndObj->mib2Tbl));
            break;

#endif /* INCLUDE_RFC_2233 */


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
    UINT8   command     /* the command being issued */
    )
    {
    UINT32  immrVal = pDrvCtrl->immrVal;    /* holder for the immr value */
    volatile    UINT32  cpcrVal = 0;        /* a convenience */
    UINT32  ix = 0;             /* a counter */
    UINT8   fccNum = pDrvCtrl->fccNum;  /* holder for the fcc number */
    UINT8   fcc = fccNum - 1;       /* a convenience */

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
STATUS motFccMiiRead
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
STATUS motFccMiiWrite
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

    MOT_FCC_MII_WR (miiData, MII_MF_DATA_LEN);

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

    pDrvCtrl->phyInfo->phyWriteRtn  = (FUNCPTR) motFccMiiWrite;
    pDrvCtrl->phyInfo->phyReadRtn   = (FUNCPTR) motFccMiiRead;
    pDrvCtrl->phyInfo->phyDelayRtn  = (FUNCPTR) taskDelay;
    pDrvCtrl->phyInfo->phyLinkDownRtn = (FUNCPTR) motFccHandleLSCJob;
    pDrvCtrl->phyInfo->phyMaxDelay  = MII_PHY_DEF_DELAY;
    pDrvCtrl->phyInfo->phyDelayParm = 1;

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

    /* mark it as pre-initilized */

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
    int     link;

    MOT_FCC_LOG (MOT_FCC_DBG_ANY, "motFccHandleLSCJob: \n", 0, 0, 0, 0, 0, 0);

    MOT_FCC_STAT_INCR (pDrvCtrl->Stats->numLSCHandlerEntries);

    /* read MII status register once to unlatch link status bit */

    retVal = motFccMiiRead(pDrvCtrl,pDrvCtrl->phyInfo->phyAddr,1,&miiStat);

    /* read again to know it's current value */

    retVal = motFccMiiRead(pDrvCtrl,pDrvCtrl->phyInfo->phyAddr,1,&miiStat);
    

    if ( miiStat & 4 ) /* if link is now up */
        {
        link = 1;
        if (pDrvCtrl->phyDuplexFunc)
            {
            /* get duplex mode from BSP */

            retVal = (* pDrvCtrl->phyDuplexFunc ) ( pDrvCtrl->phyInfo, &duplex );

            if (retVal == OK)
                {
                intLevel = intLock();

                fpsmrVal = pDrvCtrl->fccReg->fcc_psmr;

                if (duplex) /* TRUE when full duplex mode is active */
                    fpsmrVal |= (M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);
                else
                    fpsmrVal &= ~(M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);

                pDrvCtrl->fccReg->fcc_psmr = fpsmrVal;

                intUnlock (intLevel);
                }
            }

        if (pDrvCtrl->phySpeedFunc)
            {
            /* get duplex mode from BSP */
            retVal = (* pDrvCtrl->phySpeedFunc ) ( pDrvCtrl->phyInfo, &speed );
            if (retVal == OK)
                {
                intLevel = intLock();

                fpsmrVal = pDrvCtrl->fccReg->fcc_psmr;
                /* TRUE when full duplex mode is active */

                if (speed) 
                    fpsmrVal |= (M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);
                else
                    fpsmrVal &= ~(M8260_FPSMR_ETH_LPB | M8260_FPSMR_ETH_FDE);

                pDrvCtrl->fccReg->fcc_psmr = fpsmrVal;

                intUnlock (intLevel);
                }
            }
        }
    else
        {
        link = 0;
        }
    
    MOT_FCC_LOG ( MOT_FCC_DBG_MII,"FCC %d Unit %d Link %s @ %s BPS %s Duplex\n",
           pDrvCtrl->fccNum, pDrvCtrl->unit, (int)linkStr[link], 
           (int)speedStr[speed],(int)duplexStr[duplex], 6);
    
    intLevel = intLock();
    pDrvCtrl->lscHandling = 0;
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
    MOT_FCC_LOG ( MOT_FCC_DBG_INT, "motFccPhyLSCInt\n", 1, 2, 3, 4, 5, 6);

    /* if no link status change job is pending */

    if ( !pDrvCtrl->lscHandling )
        {
        /* and the BSP has supplied a duplex mode function */

        if ( pDrvCtrl->phyDuplexFunc )
            {
            if (netJobAdd ( (FUNCPTR) motFccHandleLSCJob, (int) pDrvCtrl,0,0,0,0) == OK)
                {
                pDrvCtrl->lscHandling = TRUE;
                }
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
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UCHAR *      pAddr      /* address to be added */
    )
    {
    int     retVal;     /* convenient holder for return value */


    retVal = etherMultiAdd (&pDrvCtrl->endObj.multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pDrvCtrl->endObj.nMulti++;
        return (motFccHashTblAdd (pDrvCtrl, pAddr));
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
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UCHAR *      pAddr      /* address to be deleted */
    )
    {
    int     retVal;     /* convenient holder for return value */

    retVal = etherMultiDel (&pDrvCtrl->endObj.multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pDrvCtrl->endObj.nMulti--;
        retVal = motFccHashTblPopulate (pDrvCtrl);
        }

    return ((retVal == OK) ? OK : ERROR);
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
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
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
    DRV_CTRL *  pDrvCtrl,       /* pointer to DRV_CTRL structure */
    UCHAR *     pAddr       /* address to be added */
    )
    {
    UINT8   command = 0;            /* command to issue to the CP */

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
* populate them again, as more than one address may be mapped to
* the same bit.
*
* RETURNS: OK, or ERROR.
*/
LOCAL STATUS motFccHashTblPopulate
    (
    DRV_CTRL *  pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    ETHER_MULTI *   mCastNode = NULL;

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
* This routine is called by a user to try and send a packet on the
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
    int     retVal = OK;    /* holder for return value */
    char   *pBuf = NULL;    /* pointer to data to be sent */
    FCC_BD *pTbd = NULL;    /* pointer to the current ready TBD */
    int     len = 0;        /* length of data to be sent */

    WV_SEND_ENTRY(0,0);
    if ( !pMblk )
        {
        return EAGAIN;
        }

    if ( pDrvCtrl->tbdFree  ) 
        {
        /* process SNMP RFC */
        if (pMblk->mBlkHdr.mData[0] & (UINT8)0x01)
            {
            END_INC_OUT_NUCAST (pMblk->mBlkHdr.mData, pMblk->mBlkHdr.mLen);
            }
        else
            {
            END_INC_OUT_UCAST (pMblk->mBlkHdr.mData, pMblk->mBlkHdr.mLen);
            }

        pTbd = pDrvCtrl->pTbdNext;
        pBuf = pDrvCtrl->pTxPollBuf;

        /* copy data but do not free the Mblk */
        len = netMblkToBufCopy ( pMblk, (char *) pBuf, NULL );
        len = max( MOT_FCC_MIN_TX_PKT_SZ, len );

        MOT_FCC_CACHE_FLUSH (pBuf, len);            /* flush the cache, if necessary */

        pTbd->bdAddr = (unsigned long) pBuf;
        pTbd->bdLen = len;
        if ( pTbd->bdStat & M8260_FETH_TBD_W )
            {
            pTbd->bdStat = ( M8260_FETH_TBD_R  | M8260_FETH_TBD_L   | 
                             M8260_FETH_TBD_TC | M8260_FETH_TBD_PAD | M8260_FETH_TBD_W);
            pDrvCtrl->pTbdNext = pDrvCtrl->pTbdBase;
            }
        else
            {
            pTbd->bdStat = ( M8260_FETH_TBD_R | M8260_FETH_TBD_L | 
                             M8260_FETH_TBD_TC | M8260_FETH_TBD_PAD);
            pDrvCtrl->pTbdNext++;
            }

        pDrvCtrl->tbdFree--;


        CACHE_PIPE_FLUSH ();        /* Flush the write pipe */
        
        }

    motFccTbdClean(pDrvCtrl);

    WV_SEND_EXIT(0,0);
    
    return retVal;
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
* this funtion.
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
    int      retVal = OK;    /* holder for return value */
    FCC_BD * pRbd = NULL;    /* generic rbd pointer */
    UINT16   rbdStatus;      /* holder for the status of this RBD */
    UINT16   rbdLen;         /* number of bytes received */
    char   * pData = NULL;   /* a rx data pointer */

    if ( (pMblk->mBlkHdr.mFlags & M_EXT) != M_EXT )
        return EAGAIN;
    
    WV_RECV_ENTRY(0,0);

    /* get the next available RBD */
    pRbd = pDrvCtrl->pRbdNext;

    /* if it's not ready, don't touch it! */
    if ( (pRbd->bdStat & M8260_FETH_RBD_E) )
        return EAGAIN;

    rbdStatus = pRbd->bdStat;   /* read the RBD status word */
    rbdLen    = pRbd->bdLen;    /* get the actual amount of received data */


    /* Handle errata CPM-21... this code should NOT break the driver
       when CPM-21 is ultimately fixed.
    */
    if ((rbdStatus & (M8260_FETH_RBD_CL | M8260_FETH_RBD_CR)) == M8260_FETH_RBD_CL)
        {
        rbdStatus &= (UINT16)(~M8260_FETH_RBD_CL);
        }

    /* pass the packet up only if reception was Ok AND buffer is large enough,  */
    if ((rbdStatus & MOT_FCC_RBD_ERR) || pMblk->mBlkHdr.mLen < rbdLen)
        {
        END_INC_IN_ERRS();
        retVal = EAGAIN;
        }
    else
        {
        pData = (char*) pRbd->bdAddr;

        /* up-date statistics */
        if ( rbdStatus & ( M8260_FETH_RBD_MC | M8260_FETH_RBD_BC ) )
            {
            END_INC_IN_NUCAST(pData,rbdLen);
            }
        else
            {
            END_INC_IN_UCAST(pData,rbdLen);
            }


        /* set up the mBlk properly */
        pMblk->mBlkHdr.mFlags |= M_PKTHDR;
        pMblk->mBlkHdr.mLen    = (rbdLen - MII_CRC_LEN) & ~0xc000;
        pMblk->mBlkPktHdr.len  = pMblk->mBlkHdr.mLen;

        /* Make cache consistent with memory */
        MOT_FCC_CACHE_INVAL ((char *) pData, pMblk->mBlkHdr.mLen);
        bcopy ((char *) pData, (char *) pMblk->mBlkHdr.mData, ((rbdLen - MII_CRC_LEN) & ~0xc000));
        retVal = OK;
        }

    pRbd->bdLen = 0;

    /* up-date the status word: treat the last RBD in the ring differently */
    if ( pRbd->bdStat & M8260_FETH_RBD_W )
        {
        pRbd->bdStat = M8260_FETH_RBD_W | M8260_FETH_RBD_E | M8260_FETH_RBD_I;
        pDrvCtrl->pRbdNext = pDrvCtrl->pRbdBase;
        }
    else
        {
        pRbd->bdStat = M8260_FETH_RBD_E | M8260_FETH_RBD_I;
        pDrvCtrl->pRbdNext++;
        }

    WV_RECV_EXIT(0,0);

    return retVal;
    }

/*******************************************************************************
* motFccPollStart - start polling mode
*
* This routine starts polling mode by disabling ethernet interrupts and
* setting the polling flag in the END_CTRL stucture.
*
* SEE ALSO: motFccPollStop()
*
* RETURNS: OK, or ERROR if polling mode could not be started.
*/
LOCAL STATUS motFccPollStart
    (
    DRV_CTRL * pDrvCtrl       /* pointer to DRV_CTRL structure */
    )
    {
    int intLevel;   /* current intr level */
    int retVal;     /* convenient holder for return value */

    intLevel = intLock();

    /* disable system interrupt: reset relevant bit in SIMNR_L */

    SYS_FCC_INT_DISABLE (pDrvCtrl, retVal);

    if (retVal == ERROR)
        return (ERROR);

    /* mask fcc(fccNum) interrupts   */

    MOT_FCC_REG_WORD_WR ((UINT32) M8260_FGMR1 (pDrvCtrl->immrVal) + 
             ((pDrvCtrl->fccNum - 1) * M8260_FCC_IRAM_GAP), MOT_FCC_CLEAR_VAL);                \

    MOT_FCC_FLAG_SET (MOT_FCC_POLLING);

    intUnlock (intLevel);

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
    int intLevel;
    int retVal;     /* convenient holder for return value */


    intLevel = intLock();

    /* enable system interrupt: set relevant bit in SIMNR_L */

    SYS_FCC_INT_ENABLE (pDrvCtrl, retVal);
    if (retVal == ERROR)
        return (ERROR);

    /* enable fcc(fccNum) interrupts */

    MOT_FCC_REG_WORD_WR ((UINT32) M8260_FGMR1 (pDrvCtrl->immrVal) + 
        ((pDrvCtrl->fccNum - 1) * M8260_FCC_IRAM_GAP),(pDrvCtrl->intMask));

    /* set flags */

    MOT_FCC_FLAG_CLEAR (MOT_FCC_POLLING);

    intUnlock (intLevel);

    return (OK);
    }


#ifdef MOT_FCC_DBG
/******************************************************************************
*
* motFccMiiShow - Debug Function to show the Mii settings in the Phy Info 
*                 structure.
* 
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
        "\n\tifOutNUcastPkts=%d\n\tifOutUcastPkts=%d\n\tifInNUcastPkts=%d\n\tifInUcastPkts=%d\n\tifOutErrors=%d\n",
        pDrvCtrl->endObj.mib2Tbl.ifOutNUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifOutUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifInNUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifInUcastPkts,
        pDrvCtrl->endObj.mib2Tbl.ifOutErrors,6);

    }

/******************************************************************************
*
* motFccShow - Debug Function to show driver specific contol data.
* 
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
    FCC_REG_T * reg;    /* an even bigger convenience */


    if ( pDrvCtrl == NULL )
        pDrvCtrl = pDrvCtrlDbg_fcc0;

    reg = pDrvCtrl->fccReg;

    MOT_FCC_LOG (MOT_FCC_DBG_TX,
        "\n\tTxBds:Base 0x%x Total %d Free %d Next %d Last %d\n",
        (int)pDrvCtrl->pTbdBase, 
        pDrvCtrl->tbdNum, 
        pDrvCtrl->tbdFree,
        (pDrvCtrl->pTbdNext-pDrvCtrl->pTbdBase), 
        (pDrvCtrl->pTbdLast-pDrvCtrl->pTbdBase), 6);

    bdPtr = pDrvCtrl->pTbdBase;
    for ( i = 0; i < pDrvCtrl->tbdNum; i++)
       {
       MOT_FCC_LOG ( MOT_FCC_DBG_TX,
            ("TXBD:%3d Status 0x%04x Length 0x%04x Address 0x%08x Alloc 0x%08x\n"), 
            i,
            bdPtr->bdStat,
            bdPtr->bdLen,
            bdPtr->bdAddr,
            (int)pDrvCtrl->tBufList[i],6);

       bdPtr++;
       }
            
    MOT_FCC_LOG (MOT_FCC_DBG_RX,
        ("\n\tRxBds:Base 0x%x Total %d Full %d Next %d Last %d\n"),
        (int)pDrvCtrl->pRbdBase, 
        pDrvCtrl->rbdNum, 
        pDrvCtrl->rbdCnt,
        (pDrvCtrl->pRbdNext-pDrvCtrl->pRbdBase),
        (pDrvCtrl->pRbdLast-pDrvCtrl->pRbdBase),6);

   bdPtr = pDrvCtrl->pRbdBase;
   for ( i = 0; i < pDrvCtrl->rbdNum; i++)
       {
       MOT_FCC_LOG (MOT_FCC_DBG_RX,
            ("RXBD:%3d Status 0x%04x Length 0x%04x Address 0x%08x Alloc 0x%08x\n"),
            i,
            bdPtr->bdStat,
            bdPtr->bdLen,
            bdPtr->bdAddr,
            (int)pDrvCtrl->rBufList[i],6);

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
        "\n\tGRAInts:%d"
        "\n\tRXCInts:%d\n"),
        pDrvCtrl->Stats->motFccTxErr,
        pDrvCtrl->Stats->motFccRxBsyErr,
        pDrvCtrl->Stats->numTXBInts,
        pDrvCtrl->Stats->numRXFInts,
        pDrvCtrl->Stats->numGRAInts,
        pDrvCtrl->Stats->numRXCInts);


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
        "\n\tFramesLong:%d\n"), 
        pDrvCtrl->Stats->numRXFHandlerEntries,
        pDrvCtrl->Stats->numRXFHandlerFramesLong,3,4,5,6);

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
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
                  "\n\tRBPTR=0x%08x pRbdLast=0x%08x"
                  "\n\tTBPTR=0x%08x pTbdLast=0x%08x\n"
                  ), 
                  pParam->mrblr,
                  pParam->rbptr, (int)pDrvCtrl->pRbdLast, 
                  pParam->tbptr, (int)pDrvCtrl->pTbdLast, 6);
    }

/******************************************************************************
*
* motFccEramShow - Debug Function to show FCC CP ethernet parameter ram.
* 
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
                  "\n\tcrc errored frames=0x%08x"
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
* This funtion is only available when MOT_FCC_DBG is defined. It should be used
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
               ("\n\tpClBlkArea=0x%08x"
               "\n\tclBlkSize=0x%08x"
               "\n\tpMBlkArea=0x%08x"
               "\n\tmBlkSize=0x%08x\n"),
               (int)pDrvCtrl->pBufBase,
               (int)pDrvCtrl->bufSize,
               (int)pDrvCtrl->pMBlkArea,
               (int)pDrvCtrl->mBlkSize,
               0,0);


    MOT_FCC_LOG (MOT_FCC_DBG_ANY, 
                ("\n\tpNetPool=0x%08x\n\tpClPool=0x%08x\n"),
                (int)pDrvCtrl->endObj.pNetPool,
                (int)pDrvCtrl->pClPoolId, 
                0,0,0,0);
    }
#endif
