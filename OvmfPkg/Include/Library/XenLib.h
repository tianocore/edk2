/** @file

  This file includes Xen common definitions, XenBus and Xen event channel.
  The XenBusLib library provides the operations for consumers to access the XenStore.

  Copyright (c) 2011-2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __XEN_LIB__
#define __XEN_LIB__

/**
  All Xen PV driver need struct and methods are defined here.

**/

///
/// Guest OS interface to Xen.
///
#define DOMID_FIRST_RESERVED (0x7FF0U)
#define DOMID_SELF           (0x7FF0U)
#define DOMID_IO             (0x7FF1U)
#define DOMID_XEN            (0x7FF2U)
#define DOMID_COW            (0x7FF3U)
#define DOMID_INVALID        (0x7FF4U)
#define DOMID_IDLE           (0x7FFFU)

///
/// For HVM-guest Xenbus implementation.
///
#define HVM_PARAM_STORE_PFN    1
#define HVM_PARAM_STORE_EVTCHN 2


///
/// For Xen Version Hypercall
///
#define XENVER_VERSION         0
#define XENVER_EXTRAVERSION    1
typedef CHAR8 XenExtraversion[16];


//
// For HVM Hypercall (HYPERVISOR_HVM_OP)
//
#define HVMOP_SET_PARAM        0
#define HVMOP_GET_PARAM        1
typedef UINT16                 DOMID;
typedef struct {
  DOMID             DomId;
  UINT32            Index;
  UINTN             Value;
} EFI_XEN_HVM_PARAM;


//
// For Sched Hypercall (HYPERVISOR_SCHED_OP)
//

#define SCHEDOP_SHUTDOWN       2
#define SCHEDOP_POLL           3
typedef struct {
  UINT32            *Ptr;
} XEN_GUEST_HANDLE;

typedef struct {
  XEN_GUEST_HANDLE  Ports;
  UINT32            NrPorts;
  UINTN             Timeout;
} SCHED_POLL;

//
// Reason codes for SCHEDOP_SHUTDOWN
//
#define SHUTDOWN_POWEROFF      0
#define SHUTDOWN_REBOOT        1
#define SHUTDOWN_SUSPEND       2
#define SHUTDOWN_CRASH         3
#define SHUTDOWN_WATCHDOG      4


//
// For Memory Hypercall (HYPERVISOR_MEMORY_OP)
//
typedef UINTN          XEN_PFN;

//
// Set the GPFN at which a particular page appears in the specified guest's
// pseudophysical address space.
// arg == A pointer of XEN_ADD_TO_PHYSMAP.
//
#define XENMEM_ADD_TO_PHYSMAP          7

#define XENMAPSPACE_SHARED_INFO        0
#define XENMAPSPACE_GRANT_TABLE        1
#define XENMAPSPACE_Gmfn               2
#define XENMAPIDX_GRANT_TABLE_STATUS   0x80000000
typedef struct {
  //
  // Which domain to change the mapping for.
  //
  DOMID          DomId;
  //
  // Source mapping space.
  //
  UINT32         Space;
  //
  // Index into source mapping space.
  //
  UINTN          Index;
  //
  // GPFN (Guset Page Frame Number) where the source mapping page should appear.
  //
  XEN_PFN        Gpfn;
} XEN_ADD_TO_PHYSMAP;



///
/// Event Channel stuff
///
typedef UINT32 EFI_EVENT_CHANNEL_PORT;

//
// EVTCHNOP_SEND: Send an event to the remote end of the channel whose 
//   local endpoint is Port.
//
#define EVTCHNOP_SEND         4
typedef struct {
  EFI_EVENT_CHANNEL_PORT  Port;
} EVENT_CHANNEL_SEND;

//
// EVTCHNOP_ALLOC_UNBOUND: Allocate a port in domain <Dom> and mark as
//   accepting interdomain bindings from domain <RemoteDom>. A fresh port
//   is allocated in <Dom> and returned as <Port>.
// NOTES:
//   1. If the caller is unprivileged then <Dom> must be DOMID_SELF.
//   2. <RemoteDom> may be DOMID_SELF, allowing loopback connections.
//
#define EVTCHNOP_ALLOC_UNBOUND     6
typedef struct {
  //
  // IN parameters
  //
  DOMID                       Dom;
  DOMID                       RemoteDom;
  //
  // OUT parameters
  //
  EFI_EVENT_CHANNEL_PORT      Port;
} EVENT_CHANNEL_ALLOC_UNBOUND;


/**
**/
EFI_STATUS
EFIAPI
MaskEventChannel (
  IN     UINT32                    Port
  );

/**
**/
EFI_STATUS
EFIAPI
UnmaskEventChannel (
  IN     UINT32                    Port
  );

/**
TODO Don't need now
**/
EFI_EVENT_CHANNEL_PORT
EFIAPI
BindEventChannel (
  IN  OUT  EFI_EVENT_CHANNEL_PORT    Port,
  //IN     EVENT_CHANNEL_HANDLER     Handler, //evtchn_handler_t handler,
  IN       VOID                      *Data
  );

/**
**/
EFI_STATUS
EFIAPI
UnbindEventChannel (
  IN     EFI_EVENT_CHANNEL_PORT    Port
  );

/**
  Create a port available to the pal for exchanging notifications.
  Returns the result of the hypervisor call.

  Unfortunate confusion of terminology: the port is unbound as far
  as Xen is concerned, but we automatically bind a handler to it
  from inside OVMF.

**/
UINTN
EFIAPI
EventChannelAllocateUnbound (
  IN     DOMID                     Pal,
  //IN     EVENT_CHANNEL_HANDLER     Handler, //evtchn_handler_t handler,
  IN     VOID                      *Data,
  IN     EFI_EVENT_CHANNEL_PORT    *Port
  );

/**
**/
UINTN
EFIAPI
NotifyRemoteViaEventChannel (
  IN     EFI_EVENT_CHANNEL_PORT    Port
  );

/**
  Initial the event channel for OVMF.
  TODO
**/
EFI_STATUS
EFIAPI
InitEventChannel (
  VOID
  );

/**
  Reset the event channel.
  TODO
**/
EFI_STATUS
EFIAPI
FiniEventChannel (
  VOID
  );






///
/// Xenbus stuff
///
#define BUFFER_SIZE 256

//
// The state of either end of the Xenbus, i.e. the current communication
// status of initialisation across the bus.  States here imply nothing about
// the state of the connection between the driver and the kernel's device
// layers.
//
typedef enum {
  XenbusStateUnknow,
  XenbusStateInitialising,
  //
  // InitWait: Finished early initialisation but waiting for information
  // from the peer or hotplug scripts.
  //
  XenbusStateInitWait,
  // 
  // Initialised: Waiting for a connection from the peer.
  //
  XenbusStateInitialised,
  XenbusStateConnected,
  //
  // Closing: The device is being closed due to an error or an unplug event.
  //
  XenbusStateClosing,
  XenbusStateClosed,
  //
  // Reconfiguring: The device is being reconfigured.
  //
  XenbusStateReconfiguring,
  XenbusStateReconfigured
} EFI_XENBUS_STATE;

//
// Xen Store socket message type between Xen Store Daemon and client library or Guest VM.
//
typedef enum {
  XSDebug,
  XSDirectory,
  XSRead,
  XSGetPerms,
  XSWatch,
  XSUnwatch,
  XSTransactionStart,
  XSTransactionEnd,
  XSIntroduce,
  XSRelease,
  XSGetDomainPath,
  XSWrite,
  XSMkdir,
  XSRm,
  XSSetPerms,
  XSWatchEvent,
  XSError,
  XSIsDomainIntroduced,
  XSResume,
  XSSetTarget,
  XSRestrict
} EFI_XENSTORE_MSG_TYPE;

//
// Xen Store message struct 
//
typedef struct {
  //
  // Xen Store socket message type
  //
  EFI_XENSTORE_MSG_TYPE    Type;
  //
  // Request identifier
  //
  UINT32                   RequestId;
  //
  // Transaction id. If don't use transactions, just set 0.
  //
  UINT32                   TransactionId;
  //
  // Length of data following this
  //
  UINT32                   Length;
} EFI_XENSTORE_MSG;

//
// Xen Store request
//
typedef struct {
  CHAR8                    *Data;
  UINTN                    Length;
} EFI_XENSTORE_REQUEST;

#define ARRAY_SIZE(Arr) (sizeof(Arr)/sizeof(Arr[0]))

//
// Inter-domain shared memory communications.
//
#define XENSTORE_RING_SIZE 1024
typedef UINT32 XENSTORE_RING_IDX;
typedef struct {
  //
  // Requests to Xenstore daemon.
  //
  CHAR8               Req[XENSTORE_RING_SIZE];
  //
  // Replies from Xenstore daemon.
  //
  CHAR8               Rsp[XENSTORE_RING_SIZE];

  XENSTORE_RING_IDX   ReqCons;
  XENSTORE_RING_IDX   ReqProd;
  XENSTORE_RING_IDX   RspCons;
  XENSTORE_RING_IDX   RspProd;
} EFI_XENSTORE_DOMAIN_INTERFACE;

#define XENSTORE_PAYLOAD_MAX  4096

/**
  Connect to the backend Xenbus daemon. 
  This method must be called first before any Xenbus operation.

  @return              EFI_STATUS.

**/
EFI_STATUS
EFIAPI
XenbusSetup (
  VOID
  );

/**
  Reset the Xenbus connection.

  @return              EFI_STATUS.

**/
EFI_STATUS
EFIAPI
XenbusShutdown (
  VOID
  );

/**
  Read a Xenstore key. Returns a nul-terminated string or NULL.

  @param IN  Path            The Xenstore key path.
  @param OUT Data            The value to of the key.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusRead (
  IN     CHAR8                    *Path,
  OUT    CHAR8                    **Data
  );

/**
  Create or modify a Xenstore key.

  @param IN  Path            The Xenstore key path.
  @param IN  Data            The value to of the key.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusWrite (
  IN     CHAR8                    *Path,
  IN     CHAR8                    *Data
  );

/**
  List a Xenstore directory.

  @param IN  Path            The Xenstore key path.
  @param OUT DataArray       A array of pointers to the key under the directory.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusLs (
  IN     CHAR8                    *Path,
  OUT    CHAR8                    ***DataArray
  );

/**
  Get domain ID from xenstore.

  @return  The ID of surrent Domain.

**/
DOMID
EFIAPI
XenbusGetDomId (
  VOID
  );

/**
  Read a Xenstore key as an integer.

  @param IN  Path            The Xenstore key path.

  @return The value to of the key as an integer.
  @return Other negative number (Error Number).

**/
UINTN
EFIAPI
XenbusReadInteger (
  IN     CHAR8                    *Path
  );

/**
  Write a Xenstore key.

  @param IN  Node            The Xenstore key to write.
  @param IN  Path            The Xenstore key path.
  @param IN  FormatString    A Null-terminated Unicode format string.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusPrintf (
  IN     CHAR8                    *Node,
  IN     CHAR8                    *Path,
  IN     CHAR8                    *FormatString,
  ...
  );

/**
  Change the Xenbus State in Xenstore.

  @param IN  Path            The Xenstore key path.
  @param IN  State           The new state.

  @return EFI_SUCCESS        Successly.
  @return Other              Error.

**/
EFI_STATUS
EFIAPI
XenbusSwitchState (
  IN     CHAR8                    *Path,
  IN     EFI_XENBUS_STATE         State
  );






//
// XebBus for PV front driver (blk-front driver)
//

//
// Watch event queue
//
typedef struct {
  CHAR8                      *Path;
  CHAR8                      *Token;
  struct XENBUS_EVENT        *Next;  
} XENBUS_EVENT;

typedef struct XENBUS_EVENT   *XENBUS_EVENT_QUEUE;







///
/// Common functions used by Xen PV mechanism
///

//
// Maximum number of virtual CPUs in multi-processor guests.
//
#define XEN_LEGACY_MAX_VCPUS 32

//
// Definitions for architecture-specific types
//
#if defined (MDE_CPU_IA32)
//
// The IA-32 architecture struct used by VCPU_INFO.
//
typedef struct {
  UINTN                 CR2;
  UINTN                 Pad[5];
} ARCH_VCPU_INFO;

#endif // defined (MDE_CPU_IA32)

#if defined (MDE_CPU_X64)
//
// The X64 architecture struct used by VCPU_INFO.
//
typedef struct {
  UINTN                 CR2;
  UINTN                 Pad;
} ARCH_VCPU_INFO;

#endif // defined (MDE_CPU_X64)

typedef struct {
  //
  // Max PFN (Physical Frame Number).
  //
  UINTN                 MaxPfn;
  //
  // Frame containing list of mfns containing list of mfns containing p2m.
  //
  XEN_PFN               P2MFrameListList;

  UINTN                 NmiReason;
  UINT64                Pad[32];
} __attribute__((__packed__)) ARCH_SHARED_INFO;

typedef struct {
  UINT32                Version;
  UINT32                Pad0;
  //
  // TSC time at last update of time value.
  //
  UINT64                TscTimestamp;
  //
  // Time, in nanosecs, since boot.
  //
  UINT64                SystemTime;
  //
  // Current system time.
  //
  UINT32                TscToSystemMul;
  INT8                  TscShift;
  INT8                  Pad1[3];
} VCPU_TIME_INFO;

typedef struct {
  UINT8                 EvtChnUpcallPending;
  UINT8                 EvtChnUpcallMask;
  UINTN                 EvtChnPendingSel;
  
  ARCH_VCPU_INFO        Arch;
  VCPU_TIME_INFO        Time;
} VCPU_INFO;

//
// EFI_XEN_SHARED_INFO: Xen shared_info_page
//
typedef struct {
  VCPU_INFO             VcpuInfo[XEN_LEGACY_MAX_VCPUS];
  //
  // Each channel is associated with two bits of information: PENDING and MASK.
  //
  UINTN                 EventChannelPending[sizeof (UINTN) * 8];
  UINTN                 EventChannelMask[sizeof (UINTN) * 8];
  //
  // Wallclock time.
  //
  UINT32                WallClockVersion;
  UINT32                WallClockSecond;
  UINT32                WallClockNanosecond;

  ARCH_SHARED_INFO      Arch;
} EFI_XEN_SHARED_INFO;

/**
  Get shared info page.

  @return A pointer to shared info page.

**/
VOID *
EFIAPI
GetSharedInfo (
  VOID
  );

/**
  Test and clear one bit at a gavin address.

  @param IN BitNum   The number of bit to operate.
  @param IN Addr     The address to operate.

  @return  The old value of the bit.

**/
UINTN
EFIAPI
SyncTestClearBit (
  IN     UINTN                    BitNum,
  IN     VOID                     *Addr
  );

//
// Xenstore protocol
//
#define XEN_IO_PROTO_ABI_X86_32     "x86_32-abi"
#define XEN_IO_PROTO_ABI_X86_64     "x86_64-abi"

#if defined (MDE_CPU_IA32)
  #define XEN_IO_PROTO_ABI_NATIVE XEN_IO_PROTO_ABI_X86_32
#endif

#if defined (MDE_CPU_X64)
  #define XEN_IO_PROTO_ABI_NATIVE XEN_IO_PROTO_ABI_X86_64
#endif







///
/// Xen shared ring related
///
/// mini-os/include/xen/io/ring.h
///TODO Need to modify...
typedef UINTN          RING_IDX;

//////////////////////////////////////////////////////////////////////////////////////////
//TODO Below!!!
/* Round a 32-bit unsigned constant down to the nearest power of two. */
#define __RD2(_x)  (((_x) & 0x00000002) ? 0x2                  : ((_x) & 0x1))
#define __RD4(_x)  (((_x) & 0x0000000c) ? __RD2((_x)>>2)<<2    : __RD2(_x))
#define __RD8(_x)  (((_x) & 0x000000f0) ? __RD4((_x)>>4)<<4    : __RD4(_x))
#define __RD16(_x) (((_x) & 0x0000ff00) ? __RD8((_x)>>8)<<8    : __RD8(_x))
#define __RD32(_x) (((_x) & 0xffff0000) ? __RD16((_x)>>16)<<16 : __RD16(_x))

/*
 * Calculate size of a shared ring, given the total available space for the
 * ring and indexes (_sz), and the name tag of the request/response structure.
 * A ring contains as many entries as will fit, rounded down to the nearest 
 * power of two (so we can mask with (size-1) to loop around).
 */
#define __CONST_RING_SIZE(_s, _sz) \
    (__RD32(((_sz) - offsetof(struct _s##_sring, Ring)) / \
	    sizeof(((struct _s##_sring *)0)->Ring[0])))
/*
 * The same for passing in an actual pointer instead of a name tag. TODO
 */
#define __RING_SIZE(_s, _sz) \
    (__RD32(((_sz) - (long)(_s)->Ring + (long)(_s)) / sizeof((_s)->Ring[0])))
//TODO above!!!
//////////////////////////////////////////////////////////////////////////////////////////
/*
 * Macros to make the correct C datatypes for a new kind of ring.
 * 
 * To make a new ring datatype, you need to have two message structures,
 * let's say request_t, and response_t already defined.
 *
 * In a header where you want the ring datatype declared, you then do:
 *
 *     DEFINE_RING_TYPES(mytag, request_t, response_t);
 *
 * These expand out to give you a set of types, as you can see below.
 * The most important of these are:
 * 
 *     mytag_sring_t      - The shared ring.
 *     mytag_front_ring_t - The 'front' half of the ring.
 *     mytag_back_ring_t  - The 'back' half of the ring.
 *
 * To initialize a ring in your code you need to know the location and size
 * of the shared memory area (PAGE_SIZE, for instance). To initialise
 * the front half:
 *
 *     mytag_front_ring_t front_ring;
 *     SHARED_RING_INIT((mytag_sring_t *)shared_page);
 *     FRONT_RING_INIT(&front_ring, (mytag_sring_t *)shared_page, PAGE_SIZE);
 *
 * Initializing the back follows similarly (note that only the front
 * initializes the shared ring):
 *
 *     mytag_back_ring_t back_ring;
 *     BACK_RING_INIT(&back_ring, (mytag_sring_t *)shared_page, PAGE_SIZE);
 */

#define DEFINE_RING_TYPES(__Name, __Req, __Rsp)                         \
                                                                        \
/* Shared ring entry */                                                 \
typedef union {                                                         \
  __Req                    Req;                                         \
  __Rsp                    Rsp;                                         \
} __Name##_SRING_ENTRY;                                                 \
                                                                        \
/* Shared ring page */                                                  \
typedef struct {                                                        \
  RING_IDX                 ReqProd, ReqEvent;                           \
  RING_IDX                 RspProd, RspEvent;                           \
  union {                                                               \
    struct {                                                            \
      UINT8 SmartpollActive;                                            \
    } Netif;                                                            \
    struct {                                                            \
      UINT8 Msg;                                                        \
    } TapifUser;                                                        \
    UINT8 PvtPad[4];                                                    \
  } Private;                                                            \
  UINT8 Pad[44];                                                        \
  __Name##_SRING_ENTRY Ring[1]; /* variable-length */                   \
} __Name##_SRING;                                                       \
                                                                        \
/* "Front" end's private variables */                                   \
typedef struct {                                                        \
  RING_IDX                 ReqProdPvt;                                  \
  RING_IDX                 RspCons;                                     \
  UINTN                    NrEvents;                                    \
  __Name##_SRING           *Sring;                                      \
} __Name##_FRONT_RING;                                                  \
                                                                        \
/*TODO deleted(backend will not be used in OVMF)*/                      \
/*"Back" end's private variables */                                     \
typedef struct {                                                        \
  RING_IDX                 RspProdPvt;                                  \
  RING_IDX                 ReqCons;                                     \
  UINTN                    NrEvents;                                    \
  __Name##_SRING           *Sring;                                      \
} __Name##_BACK_RING;

/*
 * Macros for manipulating rings.
 * 
 * FRONT_RING_whatever works on the "front end" of a ring: here 
 * requests are pushed on to the ring and responses taken off it.
 * 
 * BACK_RING_whatever works on the "back end" of a ring: here 
 * requests are taken off the ring and responses put on.
 * 
 * N.B. these macros do NO INTERLOCKS OR FLOW CONTROL. 
 * This is OK in 1-for-1 request-response situations where the 
 * requestor (front end) never has more than RING_SIZE()-1
 * outstanding requests.
 */

/* Initialising empty rings */
#define SHARED_RING_INIT(Str) do {                                      \
  (Str)->ReqProd  = (Str)->RspProd  = 0;                                \
  (Str)->ReqEvent = (Str)->RspEvent = 1;                                \
  ZeroMem ((Str)->Private.PvtPad, sizeof((Str)->Private.PvtPad));       \
  ZeroMem ((Str)->Pad, sizeof((Str)->Pad));                             \
} while(0)

#define FRONT_RING_INIT(Ring, Str, __Size) do {                         \
  (Ring)->ReqProdPvt = 0;                                               \
  (Ring)->RspCons    = 0;                                               \
  (Ring)->NrEvents   = __RING_SIZE(Str, __Size);                        \
  (Ring)->Sring      = (Str);                                           \
} while (0)

#define BACK_RING_INIT(Ring, Str, __Size) do {                          \
  (Ring)->RspProdPvt = 0;                                               \
  (Ring)->ReqCons    = 0;                                               \
  (Ring)->NrEvents   = __RING_SIZE(Str, __Size);                        \
  (Ring)->Sring      = (Str);                                           \
} while (0)






#endif //__XEN_LIB__

