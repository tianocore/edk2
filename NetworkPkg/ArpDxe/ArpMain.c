/** @file
  Implementation of EFI Address Resolution Protocol (ARP) Protocol interface functions.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ArpImpl.h"

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
  )
{
  EFI_STATUS         Status;
  ARP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ConfigData != NULL) &&
      ((ConfigData->SwAddressLength == 0) ||
       (ConfigData->StationAddress == NULL) ||
       (ConfigData->SwAddressType <= 1500)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Configure this instance, the ConfigData has already passed the basic checks.
  //
  Status = ArpConfigureInstance (Instance, ConfigData);

  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
  )
{
  EFI_STATUS               Status;
  ARP_INSTANCE_DATA        *Instance;
  ARP_SERVICE_DATA         *ArpService;
  ARP_CACHE_ENTRY          *CacheEntry;
  EFI_SIMPLE_NETWORK_MODE  *SnpMode;
  NET_ARP_ADDRESS          MatchAddress[2];
  EFI_TPL                  OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (((!DenyFlag) && ((TargetHwAddress == NULL) || (TargetSwAddress == NULL))) ||
      (DenyFlag && (TargetHwAddress != NULL) && (TargetSwAddress != NULL)) ||
      ((TargetHwAddress == NULL) && (TargetSwAddress == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  Status     = EFI_SUCCESS;
  ArpService = Instance->ArpService;
  SnpMode    = &Instance->ArpService->SnpMode;

  //
  // Fill the hardware address part in the MatchAddress.
  //
  MatchAddress[Hardware].Type       = SnpMode->IfType;
  MatchAddress[Hardware].Length     = (UINT8)SnpMode->HwAddressSize;
  MatchAddress[Hardware].AddressPtr = TargetHwAddress;

  //
  // Fill the software address part in the MatchAddress.
  //
  MatchAddress[Protocol].Type       = Instance->ConfigData.SwAddressType;
  MatchAddress[Protocol].Length     = Instance->ConfigData.SwAddressLength;
  MatchAddress[Protocol].AddressPtr = TargetSwAddress;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // See whether the entry to add exists. Check the DeniedCacheTable first.
  //
  CacheEntry = ArpFindDeniedCacheEntry (
                 ArpService,
                 &MatchAddress[Protocol],
                 &MatchAddress[Hardware]
                 );

  if (CacheEntry == NULL) {
    //
    // Check the ResolvedCacheTable
    //
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->ResolvedCacheTable,
                   NULL,
                   ByBoth,
                   &MatchAddress[Protocol],
                   &MatchAddress[Hardware]
                   );
  }

  if ((CacheEntry != NULL) && !Overwrite) {
    //
    // The entry to add exists, if not Overwrite, deny this add request.
    //
    Status = EFI_ACCESS_DENIED;
    goto UNLOCK_EXIT;
  }

  if ((CacheEntry == NULL) && (TargetSwAddress != NULL)) {
    //
    // Check whether there are pending requests matching the entry to be added.
    //
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->PendingRequestTable,
                   NULL,
                   ByProtoAddress,
                   &MatchAddress[Protocol],
                   NULL
                   );
  }

  if (CacheEntry != NULL) {
    //
    // Remove it from the Table.
    //
    RemoveEntryList (&CacheEntry->List);
  } else {
    //
    // It's a new entry, allocate memory for the entry.
    //
    CacheEntry = ArpAllocCacheEntry (Instance);

    if (CacheEntry == NULL) {
      DEBUG ((DEBUG_ERROR, "ArpAdd: Failed to allocate pool for CacheEntry.\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto UNLOCK_EXIT;
    }
  }

  //
  // Overwrite these parameters.
  //
  CacheEntry->DefaultDecayTime = TimeoutValue;
  CacheEntry->DecayTime        = TimeoutValue;

  //
  // Fill in the addresses.
  //
  ArpFillAddressInCacheEntry (
    CacheEntry,
    &MatchAddress[Hardware],
    &MatchAddress[Protocol]
    );

  //
  // Inform the user if there is any.
  //
  ArpAddressResolved (CacheEntry, NULL, NULL);

  //
  // Add this CacheEntry to the corresponding CacheTable.
  //
  if (DenyFlag) {
    InsertHeadList (&ArpService->DeniedCacheTable, &CacheEntry->List);
  } else {
    InsertHeadList (&ArpService->ResolvedCacheTable, &CacheEntry->List);
  }

UNLOCK_EXIT:

  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
  )
{
  EFI_STATUS         Status;
  ARP_INSTANCE_DATA  *Instance;
  EFI_TPL            OldTpl;

  if ((This == NULL) ||
      (!Refresh && (EntryCount == NULL) && (EntryLength == NULL)) ||
      ((Entries != NULL) && ((EntryLength == NULL) || (EntryCount == NULL))))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // All the check passed, find the cache entries now.
  //
  Status = ArpFindCacheEntry (
             Instance,
             BySwAddress,
             AddressBuffer,
             EntryLength,
             EntryCount,
             Entries,
             Refresh
             );

  gBS->RestoreTPL (OldTpl);

  return Status;
}

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
  )
{
  ARP_INSTANCE_DATA  *Instance;
  UINTN              Count;
  EFI_TPL            OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Delete the specified cache entries.
  //
  Count = ArpDeleteCacheEntry (Instance, BySwAddress, AddressBuffer, TRUE);

  gBS->RestoreTPL (OldTpl);

  return (Count == 0) ? EFI_NOT_FOUND : EFI_SUCCESS;
}

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
  )
{
  ARP_INSTANCE_DATA  *Instance;
  UINTN              Count;
  EFI_TPL            OldTpl;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Delete the dynamic entries from the cache table.
  //
  Count = ArpDeleteCacheEntry (Instance, FALSE, NULL, FALSE);

  gBS->RestoreTPL (OldTpl);

  return (Count == 0) ? EFI_NOT_FOUND : EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS               Status;
  ARP_INSTANCE_DATA        *Instance;
  ARP_SERVICE_DATA         *ArpService;
  EFI_SIMPLE_NETWORK_MODE  *SnpMode;
  ARP_CACHE_ENTRY          *CacheEntry;
  NET_ARP_ADDRESS          HardwareAddress;
  NET_ARP_ADDRESS          ProtocolAddress;
  USER_REQUEST_CONTEXT     *RequestContext;
  EFI_TPL                  OldTpl;

  if ((This == NULL) || (TargetHwAddress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  Status     = EFI_SUCCESS;
  ArpService = Instance->ArpService;
  SnpMode    = &ArpService->SnpMode;

  if ((TargetSwAddress == NULL) ||
      ((Instance->ConfigData.SwAddressType == IPV4_ETHER_PROTO_TYPE) &&
       IP4_IS_LOCAL_BROADCAST (*((UINT32 *)TargetSwAddress))))
  {
    //
    // Return the hardware broadcast address.
    //
    CopyMem (TargetHwAddress, &SnpMode->BroadcastAddress, SnpMode->HwAddressSize);

    goto SIGNAL_USER;
  }

  if ((Instance->ConfigData.SwAddressType == IPV4_ETHER_PROTO_TYPE) &&
      IP4_IS_MULTICAST (NTOHL (*((UINT32 *)TargetSwAddress))))
  {
    //
    // If the software address is an IPv4 multicast address, invoke Mnp to
    // resolve the address.
    //
    Status = ArpService->Mnp->McastIpToMac (
                                ArpService->Mnp,
                                FALSE,
                                TargetSwAddress,
                                TargetHwAddress
                                );
    goto SIGNAL_USER;
  }

  HardwareAddress.Type       = SnpMode->IfType;
  HardwareAddress.Length     = (UINT8)SnpMode->HwAddressSize;
  HardwareAddress.AddressPtr = NULL;

  ProtocolAddress.Type       = Instance->ConfigData.SwAddressType;
  ProtocolAddress.Length     = Instance->ConfigData.SwAddressLength;
  ProtocolAddress.AddressPtr = TargetSwAddress;

  //
  // Initialize the TargetHwAddress to a zero address.
  //
  ZeroMem (TargetHwAddress, SnpMode->HwAddressSize);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Check whether the software address is in the denied table.
  //
  CacheEntry = ArpFindDeniedCacheEntry (ArpService, &ProtocolAddress, NULL);
  if (CacheEntry != NULL) {
    Status = EFI_ACCESS_DENIED;
    goto UNLOCK_EXIT;
  }

  //
  // Check whether the software address is already resolved.
  //
  CacheEntry = ArpFindNextCacheEntryInTable (
                 &ArpService->ResolvedCacheTable,
                 NULL,
                 ByProtoAddress,
                 &ProtocolAddress,
                 NULL
                 );
  if (CacheEntry != NULL) {
    //
    // Resolved, copy the address into the user buffer.
    //
    CopyMem (
      TargetHwAddress,
      CacheEntry->Addresses[Hardware].AddressPtr,
      CacheEntry->Addresses[Hardware].Length
      );

    goto UNLOCK_EXIT;
  }

  if (ResolvedEvent == NULL) {
    Status = EFI_NOT_READY;
    goto UNLOCK_EXIT;
  }

  //
  // Create a request context for this arp request.
  //
  RequestContext = AllocatePool (sizeof (USER_REQUEST_CONTEXT));
  if (RequestContext == NULL) {
    DEBUG ((DEBUG_ERROR, "ArpRequest: Allocate memory for RequestContext failed.\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto UNLOCK_EXIT;
  }

  RequestContext->Instance         = Instance;
  RequestContext->UserRequestEvent = ResolvedEvent;
  RequestContext->UserHwAddrBuffer = TargetHwAddress;
  InitializeListHead (&RequestContext->List);

  //
  // Check whether there is a same request.
  //
  CacheEntry = ArpFindNextCacheEntryInTable (
                 &ArpService->PendingRequestTable,
                 NULL,
                 ByProtoAddress,
                 &ProtocolAddress,
                 NULL
                 );
  if (CacheEntry != NULL) {
    CacheEntry->NextRetryTime = Instance->ConfigData.RetryTimeOut;
    CacheEntry->RetryCount    = Instance->ConfigData.RetryCount;
  } else {
    //
    // Allocate a cache entry for this request.
    //
    CacheEntry = ArpAllocCacheEntry (Instance);
    if (CacheEntry == NULL) {
      DEBUG ((DEBUG_ERROR, "ArpRequest: Allocate memory for CacheEntry failed.\n"));
      FreePool (RequestContext);

      Status = EFI_OUT_OF_RESOURCES;
      goto UNLOCK_EXIT;
    }

    //
    // Fill the software address.
    //
    ArpFillAddressInCacheEntry (CacheEntry, &HardwareAddress, &ProtocolAddress);

    //
    // Add this entry into the PendingRequestTable.
    //
    InsertTailList (&ArpService->PendingRequestTable, &CacheEntry->List);
  }

  //
  // Link this request context into the cache entry.
  //
  InsertHeadList (&CacheEntry->UserRequestList, &RequestContext->List);

  //
  // Send out the ARP Request frame.
  //
  ArpSendFrame (Instance, CacheEntry, ARP_OPCODE_REQUEST);
  Status = EFI_NOT_READY;

UNLOCK_EXIT:

  gBS->RestoreTPL (OldTpl);

SIGNAL_USER:

  if ((ResolvedEvent != NULL) && (Status == EFI_SUCCESS)) {
    gBS->SignalEvent (ResolvedEvent);

    //
    // Dispatch the DPC queued by the NotifyFunction of ResolvedEvent.
    //
    DispatchDpc ();
  }

  return Status;
}

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
  )
{
  ARP_INSTANCE_DATA  *Instance;
  UINTN              Count;
  EFI_TPL            OldTpl;

  if ((This == NULL) ||
      ((TargetSwAddress != NULL) && (ResolvedEvent == NULL)) ||
      ((TargetSwAddress == NULL) && (ResolvedEvent != NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (This);

  if (!Instance->Configured) {
    return EFI_NOT_STARTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Cancel the specified request.
  //
  Count = ArpCancelRequest (Instance, TargetSwAddress, ResolvedEvent);

  //
  // Dispatch the DPCs queued by the NotifyFunction of the events signaled
  // by ArpCancelRequest.
  //
  DispatchDpc ();

  gBS->RestoreTPL (OldTpl);

  return (Count == 0) ? EFI_NOT_FOUND : EFI_SUCCESS;
}
