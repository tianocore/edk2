/** @file
  EFI Address Resolution Protocol (ARP) Protocol interface header file.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ARP_IMPL_H_
#define _ARP_IMPL_H_

#include <Uefi.h>

#include <Protocol/Arp.h>
#include <Protocol/ManagedNetwork.h>
#include <Protocol/ServiceBinding.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DpcLib.h>

//
// Ethernet protocol type definitions.
//
#define ARP_ETHER_PROTO_TYPE   0x0806
#define IPV4_ETHER_PROTO_TYPE  0x0800
#define IPV6_ETHER_PROTO_TYPE  0x86DD

//
// ARP opcode definitions.
//
#define ARP_OPCODE_REQUEST  0x0001
#define ARP_OPCODE_REPLY    0x0002

//
// ARP timeout, retry count and interval definitions.
//
#define ARP_DEFAULT_TIMEOUT_VALUE    (400 * TICKS_PER_SECOND)
#define ARP_DEFAULT_RETRY_COUNT      2
#define ARP_DEFAULT_RETRY_INTERVAL   (5   * TICKS_PER_MS)
#define ARP_PERIODIC_TIMER_INTERVAL  (500 * TICKS_PER_MS)

//
// ARP packet head definition.
//
#pragma pack(1)
typedef struct {
  UINT16    HwType;
  UINT16    ProtoType;
  UINT8     HwAddrLen;
  UINT8     ProtoAddrLen;
  UINT16    OpCode;
} ARP_HEAD;
#pragma pack()

//
// ARP Address definition for internal use.
//
typedef struct {
  UINT8    *SenderHwAddr;
  UINT8    *SenderProtoAddr;
  UINT8    *TargetHwAddr;
  UINT8    *TargetProtoAddr;
} ARP_ADDRESS;

#define MATCH_SW_ADDRESS  0x1
#define MATCH_HW_ADDRESS  0x2

//
// Enumeration for the search type. A search type is specified as the keyword to find
// a cache entry in the cache table.
//
typedef enum {
  ByNone         = 0,
  ByProtoAddress = MATCH_SW_ADDRESS,
  ByHwAddress    = MATCH_HW_ADDRESS,
  ByBoth         = MATCH_SW_ADDRESS | MATCH_HW_ADDRESS
} FIND_OPTYPE;

#define ARP_INSTANCE_DATA_SIGNATURE  SIGNATURE_32('A', 'R', 'P', 'I')

/**
  Returns a pointer to the ARP_INSTANCE_DATA structure from the input a.

  If the signatures matches, then a pointer to the data structure that contains
  a specified field of that data structure is returned.

  @param  a              Pointer to the field specified by ArpProto within a data
                         structure of type ARP_INSTANCE_DATA.

**/
#define ARP_INSTANCE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  ARP_INSTANCE_DATA, \
  ArpProto, \
  ARP_INSTANCE_DATA_SIGNATURE \
  )

typedef struct _ARP_SERVICE_DATA ARP_SERVICE_DATA;

//
// ARP instance context data structure.
//
typedef struct {
  UINT32                 Signature;
  ARP_SERVICE_DATA       *ArpService;
  EFI_HANDLE             Handle;
  EFI_ARP_PROTOCOL       ArpProto;
  LIST_ENTRY             List;
  EFI_ARP_CONFIG_DATA    ConfigData;
  BOOLEAN                Configured;
  BOOLEAN                InDestroy;
} ARP_INSTANCE_DATA;

#define ARP_SERVICE_DATA_SIGNATURE  SIGNATURE_32('A', 'R', 'P', 'S')

/**
  Returns a pointer to the ARP_SERVICE_DATA structure from the input a.

  If the signatures matches, then a pointer to the data structure that contains
  a specified field of that data structure is returned.

  @param  a              Pointer to the field specified by ServiceBinding within
                         a data structure of type ARP_SERVICE_DATA.

**/
#define ARP_SERVICE_DATA_FROM_THIS(a) \
  CR ( \
  (a), \
  ARP_SERVICE_DATA, \
  ServiceBinding, \
  ARP_SERVICE_DATA_SIGNATURE \
  )

//
// ARP service data structure.
//
struct _ARP_SERVICE_DATA {
  UINT32                                  Signature;
  EFI_SERVICE_BINDING_PROTOCOL            ServiceBinding;

  EFI_HANDLE                              MnpChildHandle;
  EFI_HANDLE                              ImageHandle;
  EFI_HANDLE                              ControllerHandle;

  EFI_MANAGED_NETWORK_PROTOCOL            *Mnp;
  EFI_MANAGED_NETWORK_CONFIG_DATA         MnpConfigData;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN    RxToken;

  EFI_SIMPLE_NETWORK_MODE                 SnpMode;

  UINTN                                   ChildrenNumber;
  LIST_ENTRY                              ChildrenList;

  LIST_ENTRY                              PendingRequestTable;
  LIST_ENTRY                              DeniedCacheTable;
  LIST_ENTRY                              ResolvedCacheTable;

  EFI_EVENT                               PeriodicTimer;
};

//
// User request context structure.
//
typedef struct {
  LIST_ENTRY           List;
  ARP_INSTANCE_DATA    *Instance;
  EFI_EVENT            UserRequestEvent;
  VOID                 *UserHwAddrBuffer;
} USER_REQUEST_CONTEXT;

#define ARP_MAX_PROTOCOL_ADDRESS_LEN  sizeof(EFI_IP_ADDRESS)
#define ARP_MAX_HARDWARE_ADDRESS_LEN  sizeof(EFI_MAC_ADDRESS)

typedef union {
  UINT8    ProtoAddress[ARP_MAX_PROTOCOL_ADDRESS_LEN];
  UINT8    HwAddress[ARP_MAX_HARDWARE_ADDRESS_LEN];
} NET_ARP_ADDRESS_UNION;

//
// ARP address structure in an ARP packet.
//
typedef struct {
  UINT16                   Type;
  UINT8                    Length;
  UINT8                    *AddressPtr;
  NET_ARP_ADDRESS_UNION    Buffer;
} NET_ARP_ADDRESS;

//
// Enumeration for ARP address type.
//
typedef enum {
  Hardware,
  Protocol
} ARP_ADDRESS_TYPE;

//
// ARP cache entry definition.
//
typedef struct {
  LIST_ENTRY         List;

  UINT32             RetryCount;
  UINT32             DefaultDecayTime;
  UINT32             DecayTime;
  UINT32             NextRetryTime;

  NET_ARP_ADDRESS    Addresses[2];

  LIST_ENTRY         UserRequestList;
} ARP_CACHE_ENTRY;

/**
  This function is used to assign a station address to the ARP cache for this instance
  of the ARP driver.

  Each ARP instance has one station address. The EFI_ARP_PROTOCOL driver will
  respond to ARP requests that match this registered station address. A call to
  this function with the ConfigData field set to NULL will reset this ARP instance.

  Once a protocol type and station address have been assigned to this ARP instance,
  all the following ARP functions will use this information. Attempting to change
  the protocol type or station address to a configured ARP instance will result in errors.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.
  @param  ConfigData             Pointer to the EFI_ARP_CONFIG_DATA structure.

  @retval EFI_SUCCESS            The new station address was successfully
                                 registered.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. SwAddressLength is zero when
                                 ConfigData is not NULL. StationAddress is NULL
                                 when ConfigData is not NULL.
  @retval EFI_ACCESS_DENIED      The SwAddressType, SwAddressLength, or
                                 StationAddress is different from the one that is
                                 already registered.
  @retval EFI_OUT_OF_RESOURCES   Storage for the new StationAddress could not be
                                 allocated.

**/
EFI_STATUS
EFIAPI
ArpConfigure (
  IN EFI_ARP_PROTOCOL     *This,
  IN EFI_ARP_CONFIG_DATA  *ConfigData OPTIONAL
  );

/**
  This function is used to insert entries into the ARP cache.

  ARP cache entries are typically inserted and updated by network protocol drivers
  as network traffic is processed. Most ARP cache entries will time out and be
  deleted if the network traffic stops. ARP cache entries that were inserted
  by the Add() function may be static (will not time out) or dynamic (will time out).
  Default ARP cache timeout values are not covered in most network protocol
  specifications (although RFC 1122 comes pretty close) and will only be
  discussed in general in this specification. The timeout values that are
  used in the EFI Sample Implementation should be used only as a guideline.
  Final product implementations of the EFI network stack should be tuned for
  their expected network environments.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.
  @param  DenyFlag               Set to TRUE if this entry is a deny entry. Set to
                                 FALSE if this  entry is a normal entry.
  @param  TargetSwAddress        Pointer to a protocol address to add (or deny).
                                 May be set to NULL if DenyFlag is TRUE.
  @param  TargetHwAddress        Pointer to a hardware address to add (or deny).
                                 May be set to NULL if DenyFlag is TRUE.
  @param  TimeoutValue           Time in 100-ns units that this entry will remain
                                 in the ARP cache. A value of zero means that the
                                 entry is permanent. A nonzero value will override
                                 the one given by Configure() if the entry to be
                                 added is a dynamic entry.
  @param  Overwrite              If TRUE, the matching cache entry will be
                                 overwritten with the supplied parameters. If
                                 FALSE, EFI_ACCESS_DENIED is returned if the
                                 corresponding cache entry already exists.

  @retval EFI_SUCCESS            The entry has been added or updated.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. DenyFlag is FALSE and
                                 TargetHwAddress is NULL. DenyFlag is FALSE and
                                 TargetSwAddress is NULL. TargetHwAddress is NULL
                                 and TargetSwAddress is NULL. Both TargetSwAddress
                                 and TargetHwAddress are not NULL when DenyFlag is
                                 TRUE.
  @retval EFI_OUT_OF_RESOURCES   The new ARP cache entry could not be allocated.
  @retval EFI_ACCESS_DENIED      The ARP cache entry already exists and Overwrite
                                 is not true.
  @retval EFI_NOT_STARTED        The ARP driver instance has not been configured.

**/
EFI_STATUS
EFIAPI
ArpAdd (
  IN EFI_ARP_PROTOCOL  *This,
  IN BOOLEAN           DenyFlag,
  IN VOID              *TargetSwAddress OPTIONAL,
  IN VOID              *TargetHwAddress OPTIONAL,
  IN UINT32            TimeoutValue,
  IN BOOLEAN           Overwrite
  );

/**
  This function searches the ARP cache for matching entries and allocates a buffer into
  which those entries are copied.

  The first part of the allocated buffer is EFI_ARP_FIND_DATA, following which
  are protocol address pairs and hardware address pairs.
  When finding a specific protocol address (BySwAddress is TRUE and AddressBuffer
  is not NULL), the ARP cache timeout for the found entry is reset if Refresh is
  set to TRUE. If the found ARP cache entry is a permanent entry, it is not
  affected by Refresh.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.
  @param  BySwAddress            Set to TRUE to look for matching software protocol
                                 addresses. Set to FALSE to look for matching
                                 hardware protocol addresses.
  @param  AddressBuffer          Pointer to address buffer. Set to NULL to match
                                 all addresses.
  @param  EntryLength            The size of an entry in the entries buffer.
  @param  EntryCount             The number of ARP cache entries that are found by
                                 the specified criteria.
  @param  Entries                Pointer to the buffer that will receive the ARP
                                 cache entries.
  @param  Refresh                Set to TRUE to refresh the timeout value of the
                                 matching ARP cache entry.

  @retval EFI_SUCCESS            The requested ARP cache entries were copied into
                                 the buffer.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. Both EntryCount and EntryLength are
                                 NULL, when Refresh is FALSE.
  @retval EFI_NOT_FOUND          No matching entries were found.
  @retval EFI_NOT_STARTED        The ARP driver instance has not been configured.

**/
EFI_STATUS
EFIAPI
ArpFind (
  IN EFI_ARP_PROTOCOL    *This,
  IN BOOLEAN             BySwAddress,
  IN VOID                *AddressBuffer OPTIONAL,
  OUT UINT32             *EntryLength   OPTIONAL,
  OUT UINT32             *EntryCount    OPTIONAL,
  OUT EFI_ARP_FIND_DATA  **Entries      OPTIONAL,
  IN BOOLEAN             Refresh
  );

/**
  This function removes specified ARP cache entries.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.
  @param  BySwAddress            Set to TRUE to delete matching protocol addresses.
                                 Set to FALSE to delete matching hardware
                                 addresses.
  @param  AddressBuffer          Pointer to the address buffer that is used as a
                                 key to look for the cache entry. Set to NULL to
                                 delete all entries.

  @retval EFI_SUCCESS            The entry was removed from the ARP cache.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_FOUND          The specified deletion key was not found.
  @retval EFI_NOT_STARTED        The ARP driver instance has not been configured.

**/
EFI_STATUS
EFIAPI
ArpDelete (
  IN EFI_ARP_PROTOCOL  *This,
  IN BOOLEAN           BySwAddress,
  IN VOID              *AddressBuffer OPTIONAL
  );

/**
  This function delete all dynamic entries from the ARP cache that match the specified
  software protocol type.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.

  @retval EFI_SUCCESS            The cache has been flushed.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_FOUND          There are no matching dynamic cache entries.
  @retval EFI_NOT_STARTED        The ARP driver instance has not been configured.

**/
EFI_STATUS
EFIAPI
ArpFlush (
  IN EFI_ARP_PROTOCOL  *This
  );

/**
  This function tries to resolve the TargetSwAddress and optionally returns a
  TargetHwAddress if it already exists in the ARP cache.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.
  @param  TargetSwAddress        Pointer to the protocol address to resolve.
  @param  ResolvedEvent          Pointer to the event that will be signaled when
                                 the address is resolved or some error occurs.
  @param  TargetHwAddress        Pointer to the buffer for the resolved hardware
                                 address in network byte order.

  @retval EFI_SUCCESS            The data is copied from the ARP cache into the
                                 TargetHwAddress buffer.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. TargetHwAddress is NULL.
  @retval EFI_ACCESS_DENIED      The requested address is not present in the normal
                                 ARP cache but is present in the deny address list.
                                 Outgoing traffic to that address is forbidden.
  @retval EFI_NOT_STARTED        The ARP driver instance has not been configured.
  @retval EFI_NOT_READY          The request has been started and is not finished.

**/
EFI_STATUS
EFIAPI
ArpRequest (
  IN EFI_ARP_PROTOCOL  *This,
  IN VOID              *TargetSwAddress OPTIONAL,
  IN EFI_EVENT         ResolvedEvent    OPTIONAL,
  OUT VOID             *TargetHwAddress
  );

/**
  This function aborts the previous ARP request (identified by This,  TargetSwAddress
  and ResolvedEvent) that is issued by EFI_ARP_PROTOCOL.Request().

  If the request is in the internal ARP request queue, the request is aborted
  immediately and its ResolvedEvent is signaled. Only an asynchronous address
  request needs to be canceled. If TargetSwAddress and ResolvedEvent are both
  NULL, all the pending asynchronous requests that have been issued by This
  instance will be cancelled and their corresponding events will be signaled.

  @param  This                   Pointer to the EFI_ARP_PROTOCOL instance.
  @param  TargetSwAddress        Pointer to the protocol address in previous
                                 request session.
  @param  ResolvedEvent          Pointer to the event that is used as the
                                 notification event in previous request session.

  @retval EFI_SUCCESS            The pending request session(s) is/are aborted and
                                 corresponding event(s) is/are signaled.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 This is NULL. TargetSwAddress is not NULL and
                                 ResolvedEvent is NULL. TargetSwAddress is NULL and
                                 ResolvedEvent is not NULL.
  @retval EFI_NOT_STARTED        The ARP driver instance has not been configured.
  @retval EFI_NOT_FOUND          The request is not issued by
                                 EFI_ARP_PROTOCOL.Request().

**/
EFI_STATUS
EFIAPI
ArpCancel (
  IN EFI_ARP_PROTOCOL  *This,
  IN VOID              *TargetSwAddress OPTIONAL,
  IN EFI_EVENT         ResolvedEvent    OPTIONAL
  );

/**
  Configure the instance using the ConfigData. ConfigData is already validated.

  @param[in]  Instance           Pointer to the instance context data to be
                                 configured.
  @param[in]  ConfigData         Pointer to the configuration data used to
                                 configure the instance.

  @retval EFI_SUCCESS            The instance is configured with the ConfigData.
  @retval EFI_ACCESS_DENIED      The instance is already configured and the
                                 ConfigData tries to reset some unchangeable
                                 fields.
  @retval EFI_INVALID_PARAMETER  The ConfigData provides a non-unicast IPv4 address
                                 when the SwAddressType is IPv4.
  @retval EFI_OUT_OF_RESOURCES   The instance fails to configure due to memory
                                 limitation.

**/
EFI_STATUS
ArpConfigureInstance (
  IN ARP_INSTANCE_DATA    *Instance,
  IN EFI_ARP_CONFIG_DATA  *ConfigData OPTIONAL
  );

/**
  Find the CacheEntry, using ProtocolAddress or HardwareAddress or both, as the keyword,
  in the DeniedCacheTable.

  @param[in]  ArpService             Pointer to the arp service context data.
  @param[in]  ProtocolAddress        Pointer to the protocol address.
  @param[in]  HardwareAddress        Pointer to the hardware address.

  @return Pointer to the matched cache entry, if NULL no match is found.

**/
ARP_CACHE_ENTRY *
ArpFindDeniedCacheEntry (
  IN ARP_SERVICE_DATA  *ArpService,
  IN NET_ARP_ADDRESS   *ProtocolAddress OPTIONAL,
  IN NET_ARP_ADDRESS   *HardwareAddress OPTIONAL
  );

/**
  Find the CacheEntry which matches the requirements in the specified CacheTable.

  @param[in]  CacheTable             Pointer to the arp cache table.
  @param[in]  StartEntry             Pointer to the start entry this search begins with
                                     in the cache table.
  @param[in]  FindOpType             The search type.
  @param[in]  ProtocolAddress        Pointer to the protocol address to match.
  @param[in]  HardwareAddress        Pointer to the hardware address to match.

  @return Pointer to the matched arp cache entry, if NULL, no match is found.

**/
ARP_CACHE_ENTRY *
ArpFindNextCacheEntryInTable (
  IN LIST_ENTRY       *CacheTable,
  IN LIST_ENTRY       *StartEntry,
  IN FIND_OPTYPE      FindOpType,
  IN NET_ARP_ADDRESS  *ProtocolAddress OPTIONAL,
  IN NET_ARP_ADDRESS  *HardwareAddress OPTIONAL
  );

/**
  Allocate a cache entry and initialize it.

  @param[in]  Instance               Pointer to the instance context data.

  @return Pointer to the new created cache entry.

**/
ARP_CACHE_ENTRY *
ArpAllocCacheEntry (
  IN ARP_INSTANCE_DATA  *Instance
  );

/**
  Fill the addresses in the CacheEntry using the information passed in by
  HwAddr and SwAddr.

  @param[in]  CacheEntry             Pointer to the cache entry.
  @param[in]  HwAddr                 Pointer to the software address.
  @param[in]  SwAddr                 Pointer to the hardware address.

  @return None.

**/
VOID
ArpFillAddressInCacheEntry (
  IN ARP_CACHE_ENTRY  *CacheEntry,
  IN NET_ARP_ADDRESS  *HwAddr OPTIONAL,
  IN NET_ARP_ADDRESS  *SwAddr OPTIONAL
  );

/**
  Turn the CacheEntry into the resolved status.

  @param[in]  CacheEntry             Pointer to the resolved cache entry.
  @param[in]  Instance               Pointer to the instance context data.
  @param[in]  UserEvent              Pointer to the UserEvent to notify.

  @return The count of notifications sent to the instance.

**/
UINTN
ArpAddressResolved (
  IN ARP_CACHE_ENTRY    *CacheEntry,
  IN ARP_INSTANCE_DATA  *Instance OPTIONAL,
  IN EFI_EVENT          UserEvent OPTIONAL
  );

/**
  Delete cache entries in all the cache tables.

  @param[in]  Instance               Pointer to the instance context data.
  @param[in]  BySwAddress            Delete the cache entry by software address or by
                                     hardware address.
  @param[in]  AddressBuffer          Pointer to the buffer containing the address to
                                     match for the deletion.
  @param[in]  Force                  This deletion is forced or not.

  @return The count of the deleted cache entries.

**/
UINTN
ArpDeleteCacheEntry (
  IN ARP_INSTANCE_DATA  *Instance,
  IN BOOLEAN            BySwAddress,
  IN UINT8              *AddressBuffer OPTIONAL,
  IN BOOLEAN            Force
  );

/**
  Send out an arp frame using the CacheEntry and the ArpOpCode.

  @param[in]  Instance               Pointer to the instance context data.
  @param[in]  CacheEntry             Pointer to the configuration data used to
                                     configure the instance.
  @param[in]  ArpOpCode              The opcode used to send out this Arp frame, either
                                     request or reply.

  @return None.

**/
VOID
ArpSendFrame (
  IN ARP_INSTANCE_DATA  *Instance,
  IN ARP_CACHE_ENTRY    *CacheEntry,
  IN UINT16             ArpOpCode
  );

/**
  Initialize the instance context data.

  @param[in]   ArpService        Pointer to the arp service context data this
                                 instance belongs to.
  @param[out]  Instance          Pointer to the instance context data.

  @return None.

**/
VOID
ArpInitInstance (
  IN  ARP_SERVICE_DATA   *ArpService,
  OUT ARP_INSTANCE_DATA  *Instance
  );

/**
  Process the Arp packets received from Mnp, the procedure conforms to RFC826.

  @param[in]  Context            Pointer to the context data registered to the
                                 Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameRcvdDpc (
  IN VOID  *Context
  );

/**
  Queue ArpOnFrameRcvdDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data registered to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameRcvd (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Process the already sent arp packets.

  @param[in]  Context                Pointer to the context data registered to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameSentDpc (
  IN VOID  *Context
  );

/**
  Request ArpOnFrameSentDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data registered to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameSent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Process the arp cache olding and drive the retrying arp requests.

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data registered to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpTimerHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Cancel the arp request.

  @param[in]  Instance               Pointer to the instance context data.
  @param[in]  TargetSwAddress        Pointer to the buffer containing the target
                                     software address to match the arp request.
  @param[in]  UserEvent              The user event used to notify this request
                                     cancellation.

  @return The count of the cancelled requests.

**/
UINTN
ArpCancelRequest (
  IN ARP_INSTANCE_DATA  *Instance,
  IN VOID               *TargetSwAddress OPTIONAL,
  IN EFI_EVENT          UserEvent        OPTIONAL
  );

/**
  Find the cache entry in the cache table.

  @param[in]  Instance           Pointer to the instance context data.
  @param[in]  BySwAddress        Set to TRUE to look for matching software protocol
                                 addresses. Set to FALSE to look for matching
                                 hardware protocol addresses.
  @param[in]  AddressBuffer      Pointer to address buffer. Set to NULL to match
                                 all addresses.
  @param[out] EntryLength        The size of an entry in the entries buffer.
  @param[out] EntryCount         The number of ARP cache entries that are found by
                                 the specified criteria.
  @param[out] Entries            Pointer to the buffer that will receive the ARP
                                 cache entries.
  @param[in]  Refresh            Set to TRUE to refresh the timeout value of the
                                 matching ARP cache entry.

  @retval EFI_SUCCESS            The requested ARP cache entries are copied into
                                 the buffer.
  @retval EFI_NOT_FOUND          No matching entries found.
  @retval EFI_OUT_OF_RESOURCE    There is a memory allocation failure.

**/
EFI_STATUS
ArpFindCacheEntry (
  IN ARP_INSTANCE_DATA   *Instance,
  IN BOOLEAN             BySwAddress,
  IN VOID                *AddressBuffer OPTIONAL,
  OUT UINT32             *EntryLength   OPTIONAL,
  OUT UINT32             *EntryCount    OPTIONAL,
  OUT EFI_ARP_FIND_DATA  **Entries      OPTIONAL,
  IN BOOLEAN             Refresh
  );

#endif
