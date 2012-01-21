/** @file
  The implementation of the ARP protocol.
  
Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArpImpl.h"

//
// Global variable of EFI ARP Protocol Interface.
//
EFI_ARP_PROTOCOL  mEfiArpProtocolTemplate = {
  ArpConfigure,
  ArpAdd,
  ArpFind,
  ArpDelete,
  ArpFlush,
  ArpRequest,
  ArpCancel
};


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
  )
{
  NET_CHECK_SIGNATURE (ArpService, ARP_SERVICE_DATA_SIGNATURE);

  Instance->Signature  = ARP_INSTANCE_DATA_SIGNATURE;
  Instance->ArpService = ArpService;

  CopyMem (&Instance->ArpProto, &mEfiArpProtocolTemplate, sizeof (Instance->ArpProto));

  Instance->Configured = FALSE;
  Instance->Destroyed  = FALSE;

  InitializeListHead (&Instance->List);
}


/**
  Process the Arp packets received from Mnp, the procedure conforms to RFC826.

  @param[in]  Context            Pointer to the context data registerd to the
                                 Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameRcvdDpc (
  IN VOID       *Context
  )
{
  EFI_STATUS                            Status;
  ARP_SERVICE_DATA                      *ArpService;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *RxToken;
  EFI_MANAGED_NETWORK_RECEIVE_DATA      *RxData;
  ARP_HEAD                              *Head;
  ARP_ADDRESS                           ArpAddress;
  ARP_CACHE_ENTRY                       *CacheEntry;
  LIST_ENTRY                            *Entry;
  ARP_INSTANCE_DATA                     *Instance;
  EFI_ARP_CONFIG_DATA                   *ConfigData;
  NET_ARP_ADDRESS                       SenderAddress[2];
  BOOLEAN                               ProtoMatched;
  BOOLEAN                               IsTarget;
  BOOLEAN                               MergeFlag;

  ArpService = (ARP_SERVICE_DATA *)Context;
  NET_CHECK_SIGNATURE (ArpService, ARP_SERVICE_DATA_SIGNATURE);

  RxToken = &ArpService->RxToken;

  if (RxToken->Status == EFI_ABORTED) {
    //
    // The Token is aborted, possibly by arp itself, just return and the receiving
    // process is stopped.
    //
    return;
  }

  if (EFI_ERROR (RxToken->Status)) {
    //
    // Restart the receiving if any other error Status occurs.
    //
    goto RESTART_RECEIVE;
  }

  //
  // Status is EFI_SUCCESS, process the received frame.
  //
  RxData = RxToken->Packet.RxData;
  Head   = (ARP_HEAD *) RxData->PacketData;

  //
  // Convert the byte order of the multi-byte fields.
  //
  Head->HwType    = NTOHS (Head->HwType);
  Head->ProtoType = NTOHS (Head->ProtoType);
  Head->OpCode    = NTOHS (Head->OpCode);

  if ((Head->HwType != ArpService->SnpMode.IfType) ||
    (Head->HwAddrLen != ArpService->SnpMode.HwAddressSize) ||
    (RxData->ProtocolType != ARP_ETHER_PROTO_TYPE)) {
    //
    // The hardware type or the hardware address length doesn't match.
    // There is a sanity check for the protocol type too.
    //
    goto RECYCLE_RXDATA;
  }

  //
  // Set the pointers to the addresses contained in the arp packet.
  //
  ArpAddress.SenderHwAddr    = (UINT8 *)(Head + 1);
  ArpAddress.SenderProtoAddr = ArpAddress.SenderHwAddr + Head->HwAddrLen;
  ArpAddress.TargetHwAddr    = ArpAddress.SenderProtoAddr + Head->ProtoAddrLen;
  ArpAddress.TargetProtoAddr = ArpAddress.TargetHwAddr + Head->HwAddrLen;

  SenderAddress[Hardware].Type       = Head->HwType;
  SenderAddress[Hardware].Length     = Head->HwAddrLen;
  SenderAddress[Hardware].AddressPtr = ArpAddress.SenderHwAddr;

  SenderAddress[Protocol].Type       = Head->ProtoType;
  SenderAddress[Protocol].Length     = Head->ProtoAddrLen;
  SenderAddress[Protocol].AddressPtr = ArpAddress.SenderProtoAddr;

  //
  // First, check the denied cache table.
  //
  CacheEntry = ArpFindDeniedCacheEntry (
                 ArpService,
                 &SenderAddress[Protocol],
                 &SenderAddress[Hardware]
                 );
  if (CacheEntry != NULL) {
    //
    // This address (either hardware or protocol address, or both) is configured to
    // be a deny entry, silently skip the normal process.
    //
    goto RECYCLE_RXDATA;
  }

  ProtoMatched = FALSE;
  IsTarget     = FALSE;
  Instance     = NULL;
  NET_LIST_FOR_EACH (Entry, &ArpService->ChildrenList) {
    //
    // Iterate all the children.
    //
    Instance = NET_LIST_USER_STRUCT (Entry, ARP_INSTANCE_DATA, List);
    NET_CHECK_SIGNATURE (Instance, ARP_INSTANCE_DATA_SIGNATURE);
    ConfigData = &Instance->ConfigData;

    if ((Instance->Configured) &&
      (Head->ProtoType == ConfigData->SwAddressType) &&
      (Head->ProtoAddrLen == ConfigData->SwAddressLength)) {
      //
      // The protocol type is matched for the received arp packet.
      //
      ProtoMatched = TRUE;
      if (0 == CompareMem (
                 (VOID *)ArpAddress.TargetProtoAddr,
                 ConfigData->StationAddress,
                 ConfigData->SwAddressLength
                 )) {
        //
        // The arp driver has the target address required by the received arp packet.
        //
        IsTarget = TRUE;
        break;
      }
    }
  }

  if (!ProtoMatched) {
    //
    // Protocol type unmatchable, skip.
    //
    goto RECYCLE_RXDATA;
  }

  //
  // Check whether the sender's address information is already in the cache.
  //
  MergeFlag  = FALSE;
  CacheEntry = ArpFindNextCacheEntryInTable (
                 &ArpService->ResolvedCacheTable,
                 NULL,
                 ByProtoAddress,
                 &SenderAddress[Protocol],
                 NULL
                 );
  if (CacheEntry != NULL) {
    //
    // Update the entry with the new information.
    //
    ArpFillAddressInCacheEntry (CacheEntry, &SenderAddress[Hardware], NULL);
    CacheEntry->DecayTime = CacheEntry->DefaultDecayTime;
    MergeFlag = TRUE;
  }

  if (!IsTarget) {
    //
    // This arp packet isn't targeted to us, skip now.
    //
    goto RECYCLE_RXDATA;
  }

  if (!MergeFlag) {
    //
    // Add the triplet <protocol type, sender protocol address, sender hardware address>
    // to the translation table.
    //
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->PendingRequestTable,
                   NULL,
                   ByProtoAddress,
                   &SenderAddress[Protocol],
                   NULL
                   );
    if (CacheEntry == NULL) {
      //
      // Allocate a new CacheEntry.
      //
      CacheEntry = ArpAllocCacheEntry (NULL);
      if (CacheEntry == NULL) {
        goto RECYCLE_RXDATA;
      }
    }

    if (!IsListEmpty (&CacheEntry->List)) {
      RemoveEntryList (&CacheEntry->List);
    }

    //
    // Fill the addresses into the CacheEntry.
    //
    ArpFillAddressInCacheEntry (
      CacheEntry,
      &SenderAddress[Hardware],
      &SenderAddress[Protocol]
      );

    //
    // Inform the user.
    //
    ArpAddressResolved (CacheEntry, NULL, NULL);

    //
    // Add this entry into the ResolvedCacheTable
    //
    InsertHeadList (&ArpService->ResolvedCacheTable, &CacheEntry->List);
  }

  if (Head->OpCode == ARP_OPCODE_REQUEST) {
    //
    // Send back the ARP Reply. If we reach here, Instance is not NULL and CacheEntry
    // is not NULL.
    //
    ArpSendFrame (Instance, CacheEntry, ARP_OPCODE_REPLY);
  }

RECYCLE_RXDATA:

  //
  // Signal Mnp to recycle the RxData.
  //
  gBS->SignalEvent (RxData->RecycleEvent);

RESTART_RECEIVE:

  //
  // Continue to receive packets from Mnp.
  //
  Status = ArpService->Mnp->Receive (ArpService->Mnp, RxToken);

  DEBUG_CODE (
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "ArpOnFrameRcvd: ArpService->Mnp->Receive "
        "failed, %r\n.", Status));
    }
  );
}

/**
  Queue ArpOnFrameRcvdDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data registerd to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameRcvd (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Request ArpOnFrameRcvdDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, ArpOnFrameRcvdDpc, Context);
}

/**
  Process the already sent arp packets.
  
  @param[in]  Context                Pointer to the context data registerd to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameSentDpc (
  IN VOID       *Context
  )
{
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *TxToken;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA     *TxData;

  ASSERT (Context != NULL);

  TxToken = (EFI_MANAGED_NETWORK_COMPLETION_TOKEN *)Context;
  TxData  = TxToken->Packet.TxData;

  DEBUG_CODE (
    if (EFI_ERROR (TxToken->Status)) {
      DEBUG ((EFI_D_ERROR, "ArpOnFrameSent: TxToken->Status, %r.\n", TxToken->Status));
    }
  );

  //
  // Free the allocated memory and close the event.
  //
  FreePool (TxData->FragmentTable[0].FragmentBuffer);
  FreePool (TxData);
  gBS->CloseEvent (TxToken->Event);
  FreePool (TxToken);
}

/**
  Request ArpOnFrameSentDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data registerd to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpOnFrameSent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Request ArpOnFrameSentDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, ArpOnFrameSentDpc, Context);
}


/**
  Process the arp cache olding and drive the retrying arp requests.

  @param[in]  Event                  The Event this notify function registered to.
  @param[in]  Context                Pointer to the context data registerd to the
                                     Event.

  @return None.

**/
VOID
EFIAPI
ArpTimerHandler (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ARP_SERVICE_DATA      *ArpService;
  LIST_ENTRY            *Entry;
  LIST_ENTRY            *NextEntry;
  LIST_ENTRY            *ContextEntry;
  ARP_CACHE_ENTRY       *CacheEntry;
  USER_REQUEST_CONTEXT  *RequestContext;

  ASSERT (Context != NULL);
  ArpService = (ARP_SERVICE_DATA *)Context;

  //
  // Iterate all the pending requests to see whether a retry is needed to send out
  // or the request finally fails because the retry time reaches the limitation.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &ArpService->PendingRequestTable) {
    CacheEntry = NET_LIST_USER_STRUCT (Entry, ARP_CACHE_ENTRY, List);

    if (CacheEntry->NextRetryTime <= ARP_PERIODIC_TIMER_INTERVAL) {
      //
      // Timeout, if we can retry more, send out the request again, otherwise abort
      // this request.
      //
      if (CacheEntry->RetryCount == 0) {
        //
        // Abort this request.
        //
        ArpAddressResolved (CacheEntry, NULL, NULL);
        ASSERT (IsListEmpty (&CacheEntry->UserRequestList));

        RemoveEntryList (&CacheEntry->List);
        FreePool (CacheEntry);
      } else {
        //
        // resend the ARP request.
        //
        ASSERT (!IsListEmpty(&CacheEntry->UserRequestList));

        ContextEntry   = CacheEntry->UserRequestList.ForwardLink;
        RequestContext = NET_LIST_USER_STRUCT (ContextEntry, USER_REQUEST_CONTEXT, List);

        ArpSendFrame (RequestContext->Instance, CacheEntry, ARP_OPCODE_REQUEST);

        CacheEntry->RetryCount--;
        CacheEntry->NextRetryTime = RequestContext->Instance->ConfigData.RetryTimeOut;
      }
    } else {
      //
      // Update the NextRetryTime.
      //
      CacheEntry->NextRetryTime -= ARP_PERIODIC_TIMER_INTERVAL;
    }
  }

  //
  // Check the timeouts for the DeniedCacheTable.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &ArpService->DeniedCacheTable) {
    CacheEntry = NET_LIST_USER_STRUCT (Entry, ARP_CACHE_ENTRY, List);
    ASSERT (IsListEmpty (&CacheEntry->UserRequestList));

    if (CacheEntry->DefaultDecayTime == 0) {
      //
      // It's a static entry, skip it.
      //
      continue;
    }

    if (CacheEntry->DecayTime <= ARP_PERIODIC_TIMER_INTERVAL) {
      //
      // Time out, remove it.
      //
      RemoveEntryList (&CacheEntry->List);
      FreePool (CacheEntry);
    } else {
      //
      // Update the DecayTime.
      //
      CacheEntry->DecayTime -= ARP_PERIODIC_TIMER_INTERVAL;
    }
  }

  //
  // Check the timeouts for the ResolvedCacheTable.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &ArpService->ResolvedCacheTable) {
    CacheEntry = NET_LIST_USER_STRUCT (Entry, ARP_CACHE_ENTRY, List);
    ASSERT (IsListEmpty (&CacheEntry->UserRequestList));

    if (CacheEntry->DefaultDecayTime == 0) {
      //
      // It's a static entry, skip it.
      //
      continue;
    }

    if (CacheEntry->DecayTime <= ARP_PERIODIC_TIMER_INTERVAL) {
      //
      // Time out, remove it.
      //
      RemoveEntryList (&CacheEntry->List);
      FreePool (CacheEntry);
    } else {
      //
      // Update the DecayTime.
      //
      CacheEntry->DecayTime -= ARP_PERIODIC_TIMER_INTERVAL;
    }
  }
}


/**
  Match the two NET_ARP_ADDRESSes.

  @param[in]  AddressOne             Pointer to the first address to match.
  @param[in]  AddressTwo             Pointer to the second address to match.

  @return The two addresses match or not.

**/
BOOLEAN
ArpMatchAddress (
  IN NET_ARP_ADDRESS  *AddressOne,
  IN NET_ARP_ADDRESS  *AddressTwo
  )
{
  ASSERT (AddressOne != NULL && AddressTwo != NULL);

  if ((AddressOne->Type != AddressTwo->Type) ||
    (AddressOne->Length != AddressTwo->Length)) {
    //
    // Either Type or Length doesn't match.
    //
    return FALSE;
  }

  if ((AddressOne->AddressPtr != NULL) &&
    (CompareMem (
      AddressOne->AddressPtr,
      AddressTwo->AddressPtr,
      AddressOne->Length
      ) != 0)) {
    //
    // The address is not the same.
    //
    return FALSE;
  }

  return TRUE;
}


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
  IN LIST_ENTRY        *CacheTable,
  IN LIST_ENTRY        *StartEntry,
  IN FIND_OPTYPE       FindOpType,
  IN NET_ARP_ADDRESS   *ProtocolAddress OPTIONAL,
  IN NET_ARP_ADDRESS   *HardwareAddress OPTIONAL
  )
{
  LIST_ENTRY       *Entry;
  ARP_CACHE_ENTRY  *CacheEntry;

  if (StartEntry == NULL) {
    //
    // Start from the beginning of the table if no StartEntry is specified.
    //
    StartEntry = CacheTable;
  }

  for (Entry = StartEntry->ForwardLink; Entry != CacheTable; Entry = Entry->ForwardLink) {
    CacheEntry = NET_LIST_USER_STRUCT (Entry, ARP_CACHE_ENTRY, List);

    if ((FindOpType & MATCH_SW_ADDRESS) != 0) {
      //
      // Find by the software address.
      //
      if (!ArpMatchAddress (ProtocolAddress, &CacheEntry->Addresses[Protocol])) {
        //
        // The ProtocolAddress doesn't match, continue to the next cache entry.
        //
        continue;
      }
    }

    if ((FindOpType & MATCH_HW_ADDRESS) != 0) {
      //
      // Find by the hardware address.
      //
      if (!ArpMatchAddress (HardwareAddress, &CacheEntry->Addresses[Hardware])) {
        //
        // The HardwareAddress doesn't match, continue to the next cache entry.
        //
        continue;
      }
    }

    //
    // The CacheEntry meets the requirements now, return this entry.
    //
    return CacheEntry;
  }

  //
  // No matching.
  //
  return NULL;
}


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
  )
{
  ARP_CACHE_ENTRY  *CacheEntry;

  ASSERT ((ProtocolAddress != NULL) || (HardwareAddress != NULL));
  NET_CHECK_SIGNATURE (ArpService, ARP_SERVICE_DATA_SIGNATURE);

  CacheEntry = NULL;

  if ((ProtocolAddress != NULL) && (ProtocolAddress->AddressPtr != NULL)) {
    //
    // Find the cache entry in the DeniedCacheTable by the protocol address.
    //
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->DeniedCacheTable,
                   NULL,
                   ByProtoAddress,
                   ProtocolAddress,
                   NULL
                   );
    if (CacheEntry != NULL) {
      //
      // There is a match.
      //
      return CacheEntry;
    }
  }

  if ((HardwareAddress != NULL) && (HardwareAddress->AddressPtr != NULL)) {
    //
    // Find the cache entry in the DeniedCacheTable by the hardware address.
    //
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->DeniedCacheTable,
                   NULL,
                   ByHwAddress,
                   NULL,
                   HardwareAddress
                   );
  }

  return CacheEntry;
}


/**
  Allocate a cache entry and initialize it.

  @param[in]  Instance               Pointer to the instance context data.

  @return Pointer to the new created cache entry.

**/
ARP_CACHE_ENTRY *
ArpAllocCacheEntry (
  IN ARP_INSTANCE_DATA  *Instance
  )
{
  ARP_CACHE_ENTRY  *CacheEntry;
  NET_ARP_ADDRESS  *Address;
  UINT16           Index;

  //
  // Allocate memory for the cache entry.
  //
  CacheEntry = AllocatePool (sizeof (ARP_CACHE_ENTRY));
  if (CacheEntry == NULL) {
    return NULL;
  }

  //
  // Init the lists.
  //
  InitializeListHead (&CacheEntry->List);
  InitializeListHead (&CacheEntry->UserRequestList);

  for (Index = 0; Index < 2; Index++) {
    //
    // Init the address pointers to point to the concrete buffer.
    //
    Address = &CacheEntry->Addresses[Index];
    Address->AddressPtr = Address->Buffer.ProtoAddress;
  }

  //
  // Zero the hardware address first.
  //
  ZeroMem (CacheEntry->Addresses[Hardware].AddressPtr, ARP_MAX_HARDWARE_ADDRESS_LEN);

  if (Instance != NULL) {
    //
    // Inherit the parameters from the instance configuration.
    //
    CacheEntry->RetryCount       = Instance->ConfigData.RetryCount;
    CacheEntry->NextRetryTime    = Instance->ConfigData.RetryTimeOut;
    CacheEntry->DefaultDecayTime = Instance->ConfigData.EntryTimeOut;
    CacheEntry->DecayTime        = Instance->ConfigData.EntryTimeOut;
  } else {
    //
    // Use the default parameters if this cache entry isn't allocate in a
    // instance's  scope.
    //
    CacheEntry->RetryCount       = ARP_DEFAULT_RETRY_COUNT;
    CacheEntry->NextRetryTime    = ARP_DEFAULT_RETRY_INTERVAL;
    CacheEntry->DefaultDecayTime = ARP_DEFAULT_TIMEOUT_VALUE;
    CacheEntry->DecayTime        = ARP_DEFAULT_TIMEOUT_VALUE;
  }

  return CacheEntry;
}


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
  )
{
  LIST_ENTRY            *Entry;
  LIST_ENTRY            *NextEntry;
  USER_REQUEST_CONTEXT  *Context;
  UINTN                 Count;

  Count = 0;

  //
  // Iterate all the linked user requests to notify them.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &CacheEntry->UserRequestList) {
    Context = NET_LIST_USER_STRUCT (Entry, USER_REQUEST_CONTEXT, List);

    if (((Instance == NULL) || (Context->Instance == Instance)) &&
      ((UserEvent == NULL) || (Context->UserRequestEvent == UserEvent))) {
      //
      // Copy the address to the user-provided buffer and notify the user.
      //
      CopyMem (
        Context->UserHwAddrBuffer,
        CacheEntry->Addresses[Hardware].AddressPtr,
        CacheEntry->Addresses[Hardware].Length
        );
      gBS->SignalEvent (Context->UserRequestEvent);

      //
      // Remove this user request and free the context data.
      //
      RemoveEntryList (&Context->List);
      FreePool (Context);

      Count++;
    }
  }

  //
  // Dispatch the DPCs queued by the NotifyFunction of the Context->UserRequestEvent.
  //
  DispatchDpc ();

  return Count;
}


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
  )
{
  NET_ARP_ADDRESS  *Address[2];
  NET_ARP_ADDRESS  *CacheAddress;
  UINT32           Index;

  Address[Hardware] = HwAddr;
  Address[Protocol] = SwAddr;

  for (Index = 0; Index < 2; Index++) {
    if (Address[Index] != NULL) {
      //
      // Fill the address if the passed in pointer is not NULL.
      //
      CacheAddress = &CacheEntry->Addresses[Index];

      CacheAddress->Type   = Address[Index]->Type;
      CacheAddress->Length = Address[Index]->Length;

      if (Address[Index]->AddressPtr != NULL) {
        //
        // Copy it if the AddressPtr points to some buffer.
        //
        CopyMem (
          CacheAddress->AddressPtr,
          Address[Index]->AddressPtr,
          CacheAddress->Length
          );
      } else {
        //
        // Zero the corresponding address buffer in the CacheEntry.
        //
        ZeroMem (CacheAddress->AddressPtr, CacheAddress->Length);
      }
    }
  }
}


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
  )
{
  EFI_ARP_CONFIG_DATA  *OldConfigData;
  IP4_ADDR             Ip;

  OldConfigData = &Instance->ConfigData;

  if (ConfigData != NULL) {

    if (Instance->Configured) {
      //
      // The instance is configured, check the unchangeable fields.
      //
      if ((OldConfigData->SwAddressType != ConfigData->SwAddressType) ||
        (OldConfigData->SwAddressLength != ConfigData->SwAddressLength) ||
        (CompareMem (
           OldConfigData->StationAddress,
           ConfigData->StationAddress,
           OldConfigData->SwAddressLength
           ) != 0)) {
        //
        // Deny the unallowed changes.
        //
        return EFI_ACCESS_DENIED;
      }
    } else {
      //
      // The instance is not configured.
      //

      if (ConfigData->SwAddressType == IPV4_ETHER_PROTO_TYPE) {
        CopyMem (&Ip, ConfigData->StationAddress, sizeof (IP4_ADDR));

        if (!NetIp4IsUnicast (NTOHL (Ip), 0)) {
          //
          // The station address is not a valid IPv4 unicast address.
          //
          return EFI_INVALID_PARAMETER;
        }
      }

      //
      // Save the configuration.
      //
      CopyMem (OldConfigData, ConfigData, sizeof (*OldConfigData));

      OldConfigData->StationAddress = AllocatePool (OldConfigData->SwAddressLength);
      if (OldConfigData->StationAddress == NULL) {
        DEBUG ((EFI_D_ERROR, "ArpConfigInstance: AllocatePool for the StationAddress "
          "failed.\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Save the StationAddress.
      //
      CopyMem (
        OldConfigData->StationAddress,
        ConfigData->StationAddress,
        OldConfigData->SwAddressLength
        );

      //
      // Set the state to configured.
      //
      Instance->Configured = TRUE;
    }

    //
    // Use the implementation specific values if the following field is zero.
    //
    OldConfigData->EntryTimeOut = (ConfigData->EntryTimeOut == 0) ?
      ARP_DEFAULT_TIMEOUT_VALUE : ConfigData->EntryTimeOut;

    OldConfigData->RetryCount   = (ConfigData->RetryCount == 0) ?
      ARP_DEFAULT_RETRY_COUNT : ConfigData->RetryCount;

    OldConfigData->RetryTimeOut = (ConfigData->RetryTimeOut == 0) ?
      ARP_DEFAULT_RETRY_INTERVAL : ConfigData->RetryTimeOut;
  } else {
    //
    // Reset the configuration.
    //

    if (Instance->Configured) {
      //
      // Cancel the arp requests issued by this instance.
      //
      Instance->ArpProto.Cancel (&Instance->ArpProto, NULL, NULL);

      //
      // Free the buffer previously allocated to hold the station address.
      //
      FreePool (OldConfigData->StationAddress);
    }

    Instance->Configured = FALSE;
  }

  return EFI_SUCCESS;
}


/**
  Send out an arp frame using the CachEntry and the ArpOpCode.

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
  )
{
  EFI_STATUS                            Status;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *TxToken;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA     *TxData;
  UINT32                                TotalLength;
  UINT8                                 *Packet;
  ARP_SERVICE_DATA                      *ArpService;
  EFI_SIMPLE_NETWORK_MODE               *SnpMode;
  EFI_ARP_CONFIG_DATA                   *ConfigData;
  UINT8                                 *TmpPtr;
  ARP_HEAD                              *ArpHead;

  ASSERT ((Instance != NULL) && (CacheEntry != NULL));

  //
  // Allocate memory for the TxToken.
  //
  TxToken = AllocatePool (sizeof(EFI_MANAGED_NETWORK_COMPLETION_TOKEN));
  if (TxToken == NULL) {
    DEBUG ((EFI_D_ERROR, "ArpSendFrame: Allocate memory for TxToken failed.\n"));
    return;
  }

  TxToken->Event = NULL;
  TxData         = NULL;
  Packet         = NULL;

  //
  // Create the event for this TxToken.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ArpOnFrameSent,
                  (VOID *)TxToken,
                  &TxToken->Event
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArpSendFrame: CreateEvent failed for TxToken->Event.\n"));
    goto CLEAN_EXIT;
  }

  //
  // Allocate memory for the TxData used in the TxToken.
  //
  TxData = AllocatePool (sizeof(EFI_MANAGED_NETWORK_TRANSMIT_DATA));
  if (TxData == NULL) {
    DEBUG ((EFI_D_ERROR, "ArpSendFrame: Allocate memory for TxData failed.\n"));
    goto CLEAN_EXIT;
  }

  ArpService = Instance->ArpService;
  SnpMode    = &ArpService->SnpMode;
  ConfigData = &Instance->ConfigData;

  //
  // Calculate the buffer length for this arp frame.
  //
  TotalLength = SnpMode->MediaHeaderSize + sizeof (ARP_HEAD) +
                2 * (ConfigData->SwAddressLength + SnpMode->HwAddressSize);

  //
  // Allocate buffer for the arp frame.
  //
  Packet = AllocatePool (TotalLength);
  if (Packet == NULL) {
    DEBUG ((EFI_D_ERROR, "ArpSendFrame: Allocate memory for Packet failed.\n"));
    ASSERT (Packet != NULL);
  }

  TmpPtr = Packet;

  //
  // The destination MAC address.
  //
  if (ArpOpCode == ARP_OPCODE_REQUEST) {
    CopyMem (TmpPtr, &SnpMode->BroadcastAddress, SnpMode->HwAddressSize);
  } else {
    CopyMem (
      TmpPtr,
      CacheEntry->Addresses[Hardware].AddressPtr,
      SnpMode->HwAddressSize
      );
  }
  TmpPtr += SnpMode->HwAddressSize;

  //
  // The source MAC address.
  //
  CopyMem (TmpPtr, &SnpMode->CurrentAddress, SnpMode->HwAddressSize);
  TmpPtr += SnpMode->HwAddressSize;

  //
  // The ethernet protocol type.
  //
  *(UINT16 *)TmpPtr = HTONS (ARP_ETHER_PROTO_TYPE);
  TmpPtr            += 2;

  //
  // The ARP Head.
  //
  ArpHead               = (ARP_HEAD *) TmpPtr;
  ArpHead->HwType       = HTONS ((UINT16)SnpMode->IfType);
  ArpHead->ProtoType    = HTONS (ConfigData->SwAddressType);
  ArpHead->HwAddrLen    = (UINT8)SnpMode->HwAddressSize;
  ArpHead->ProtoAddrLen = ConfigData->SwAddressLength;
  ArpHead->OpCode       = HTONS (ArpOpCode);
  TmpPtr                += sizeof (ARP_HEAD);

  //
  // The sender hardware address.
  //
  CopyMem (TmpPtr, &SnpMode->CurrentAddress, SnpMode->HwAddressSize);
  TmpPtr += SnpMode->HwAddressSize;

  //
  // The sender protocol address.
  //
  CopyMem (TmpPtr, ConfigData->StationAddress, ConfigData->SwAddressLength);
  TmpPtr += ConfigData->SwAddressLength;

  //
  // The target hardware address.
  //
  CopyMem (
    TmpPtr,
    CacheEntry->Addresses[Hardware].AddressPtr,
    SnpMode->HwAddressSize
    );
  TmpPtr += SnpMode->HwAddressSize;

  //
  // The target protocol address.
  //
  CopyMem (
    TmpPtr,
    CacheEntry->Addresses[Protocol].AddressPtr,
    ConfigData->SwAddressLength
    );

  //
  // Set all the fields of the TxData.
  //
  TxData->DestinationAddress = NULL;
  TxData->SourceAddress      = NULL;
  TxData->ProtocolType       = 0;
  TxData->DataLength         = TotalLength - SnpMode->MediaHeaderSize;
  TxData->HeaderLength       = (UINT16) SnpMode->MediaHeaderSize;
  TxData->FragmentCount      = 1;

  TxData->FragmentTable[0].FragmentBuffer = Packet;
  TxData->FragmentTable[0].FragmentLength = TotalLength;

  //
  // Associate the TxData with the TxToken.
  //
  TxToken->Packet.TxData = TxData;
  TxToken->Status        = EFI_NOT_READY;

  //
  // Send out this arp packet by Mnp.
  //
  Status = ArpService->Mnp->Transmit (ArpService->Mnp, TxToken);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Mnp->Transmit failed, %r.\n", Status));
    goto CLEAN_EXIT;
  }

  return;

CLEAN_EXIT:

  if (Packet != NULL) {
    FreePool (Packet);
  }

  if (TxData != NULL) {
    FreePool (TxData);
  }

  if (TxToken->Event != NULL) {
    gBS->CloseEvent (TxToken->Event);
  }

  FreePool (TxToken);
}


/**
  Delete the cache entries in the specified CacheTable, using the BySwAddress,
  SwAddressType, AddressBuffer combination as the matching key, if Force is TRUE,
  the cache is deleted event it's a static entry.

  @param[in]  CacheTable             Pointer to the cache table to do the deletion.
  @param[in]  BySwAddress            Delete the cache entry by software address or by
                                     hardware address.
  @param[in]  SwAddressType          The software address used to do the deletion.
  @param[in]  AddressBuffer          Pointer to the buffer containing the address to
                                     match for the deletion.
  @param[in]  Force                  This deletion is forced or not.

  @return The count of the deleted cache entries.

**/
UINTN
ArpDeleteCacheEntryInTable (
  IN LIST_ENTRY      *CacheTable,
  IN BOOLEAN         BySwAddress,
  IN UINT16          SwAddressType,
  IN UINT8           *AddressBuffer OPTIONAL,
  IN BOOLEAN         Force
  )
{
  LIST_ENTRY       *Entry;
  LIST_ENTRY       *NextEntry;
  ARP_CACHE_ENTRY  *CacheEntry;
  UINTN            Count;

  Count = 0;

  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, CacheTable) {
    CacheEntry = NET_LIST_USER_STRUCT (Entry, ARP_CACHE_ENTRY, List);

    if ((CacheEntry->DefaultDecayTime == 0) && !Force) {
      //
      // It's a static entry and we are not forced to delete it, skip.
      //
      continue;
    }

    if (BySwAddress) {
      if (SwAddressType == CacheEntry->Addresses[Protocol].Type) {
        //
        // Protocol address type matched. Check the address.
        //
        if ((AddressBuffer == NULL) ||
          (CompareMem (
             AddressBuffer,
             CacheEntry->Addresses[Protocol].AddressPtr,
             CacheEntry->Addresses[Protocol].Length
             ) == 0)) {
          //
          // Address matched.
          //
          goto MATCHED;
        }
      }
    } else {
      if ((AddressBuffer == NULL) ||
        (CompareMem (
           AddressBuffer,
           CacheEntry->Addresses[Hardware].AddressPtr,
           CacheEntry->Addresses[Hardware].Length
           ) == 0)) {
        //
        // Address matched.
        //
        goto MATCHED;
      }
    }

    continue;

MATCHED:

    //
    // Delete this entry.
    //
    RemoveEntryList (&CacheEntry->List);
    ASSERT (IsListEmpty (&CacheEntry->UserRequestList));
    FreePool (CacheEntry);

    Count++;
  }

  return Count;
}


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
  )
{
  ARP_SERVICE_DATA  *ArpService;
  UINTN             Count;

  NET_CHECK_SIGNATURE (Instance, ARP_INSTANCE_DATA_SIGNATURE);

  ArpService = Instance->ArpService;

  //
  // Delete the cache entries in the DeniedCacheTable.
  //
  Count = ArpDeleteCacheEntryInTable (
            &ArpService->DeniedCacheTable,
            BySwAddress,
            Instance->ConfigData.SwAddressType,
            AddressBuffer,
            Force
            );

  //
  // Delete the cache entries inthe ResolvedCacheTable.
  //
  Count += ArpDeleteCacheEntryInTable (
             &ArpService->ResolvedCacheTable,
             BySwAddress,
             Instance->ConfigData.SwAddressType,
             AddressBuffer,
             Force
             );

  return Count;
}


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
  )
{
  ARP_SERVICE_DATA  *ArpService;
  LIST_ENTRY        *Entry;
  LIST_ENTRY        *NextEntry;
  ARP_CACHE_ENTRY   *CacheEntry;
  UINTN             Count;

  NET_CHECK_SIGNATURE (Instance, ARP_INSTANCE_DATA_SIGNATURE);

  ArpService = Instance->ArpService;

  Count = 0;
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &ArpService->PendingRequestTable) {
    CacheEntry = NET_LIST_USER_STRUCT (Entry, ARP_CACHE_ENTRY, List);

    if ((TargetSwAddress == NULL) ||
      (CompareMem (
         TargetSwAddress,
         CacheEntry->Addresses[Protocol].AddressPtr,
         CacheEntry->Addresses[Protocol].Length
         ) == 0)) {
      //
      // This request entry matches the TargetSwAddress or all requests are to be
      // cancelled as TargetSwAddress is NULL.
      //
      Count += ArpAddressResolved (CacheEntry, Instance, UserEvent);

      if (IsListEmpty (&CacheEntry->UserRequestList)) {
        //
        // No user requests any more, remove this request cache entry.
        //
        RemoveEntryList (&CacheEntry->List);
        FreePool (CacheEntry);
      }
    }
  }

  return Count;
}


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
  )
{
  EFI_STATUS         Status;
  ARP_SERVICE_DATA   *ArpService;
  NET_ARP_ADDRESS    MatchAddress;
  FIND_OPTYPE        FindOpType;
  LIST_ENTRY         *StartEntry;
  ARP_CACHE_ENTRY    *CacheEntry;
  NET_MAP            FoundEntries;
  UINT32             FoundCount;
  EFI_ARP_FIND_DATA  *FindData;
  LIST_ENTRY         *CacheTable;
  UINT32             FoundEntryLength;

  ArpService = Instance->ArpService;

  //
  // Init the FounEntries used to hold the found cache entries.
  //
  NetMapInit (&FoundEntries);

  //
  // Set the MatchAddress.
  //
  if (BySwAddress) {
    MatchAddress.Type   = Instance->ConfigData.SwAddressType;
    MatchAddress.Length = Instance->ConfigData.SwAddressLength;
    FindOpType          = ByProtoAddress;
  } else {
    MatchAddress.Type   = ArpService->SnpMode.IfType;
    MatchAddress.Length = (UINT8)ArpService->SnpMode.HwAddressSize;
    FindOpType          = ByHwAddress;
  }

  MatchAddress.AddressPtr = AddressBuffer;

  //
  // Search the DeniedCacheTable
  //
  StartEntry = NULL;
  while (TRUE) {
    //
    // Try to find the matched entries in the DeniedCacheTable.
    //
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->DeniedCacheTable,
                   StartEntry,
                   FindOpType,
                   &MatchAddress,
                   &MatchAddress
                   );
    if (CacheEntry == NULL) {
      //
      // Once the CacheEntry is NULL, there are no more matches.
      //
      break;
    }

    //
    // Insert the found entry into the map.
    //
    NetMapInsertTail (
      &FoundEntries,
      (VOID *)CacheEntry,
      (VOID *)&ArpService->DeniedCacheTable
      );

    //
    // Let the next search start from this cache entry.
    //
    StartEntry = &CacheEntry->List;

    if (Refresh) {
      //
      // Refresh the DecayTime if needed.
      //
      CacheEntry->DecayTime = CacheEntry->DefaultDecayTime;
    }
  }

  //
  // Search the ResolvedCacheTable
  //
  StartEntry = NULL;
  while (TRUE) {
    CacheEntry = ArpFindNextCacheEntryInTable (
                   &ArpService->ResolvedCacheTable,
                   StartEntry,
                   FindOpType,
                   &MatchAddress,
                   &MatchAddress
                   );
    if (CacheEntry == NULL) {
      //
      // Once the CacheEntry is NULL, there are no more matches.
      //
      break;
    }

    //
    // Insert the found entry into the map.
    //
    NetMapInsertTail (
      &FoundEntries,
      (VOID *)CacheEntry,
      (VOID *)&ArpService->ResolvedCacheTable
      );

    //
    // Let the next search start from this cache entry.
    //
    StartEntry = &CacheEntry->List;

    if (Refresh) {
      //
      // Refresh the DecayTime if needed.
      //
      CacheEntry->DecayTime = CacheEntry->DefaultDecayTime;
    }
  }

  Status = EFI_SUCCESS;

  FoundCount = (UINT32) NetMapGetCount (&FoundEntries);
  if (FoundCount == 0) {
    Status = EFI_NOT_FOUND;
    goto CLEAN_EXIT;
  }

  //
  // Found the entry length, make sure its 8 bytes alignment.
  //
  FoundEntryLength = (((sizeof (EFI_ARP_FIND_DATA) + Instance->ConfigData.SwAddressLength +
                       ArpService->SnpMode.HwAddressSize) + 3) & ~(0x3));

  if (EntryLength != NULL) {
    *EntryLength = FoundEntryLength;
  }

  if (EntryCount != NULL) {
    //
    // Return the found entry count.
    //
    *EntryCount = FoundCount;
  }

  if (Entries == NULL) {
    goto CLEAN_EXIT;
  }

  //
  // Allocate buffer to copy the found entries.
  //
  FindData = AllocatePool (FoundCount * FoundEntryLength);
  if (FindData == NULL) {
    DEBUG ((EFI_D_ERROR, "ArpFindCacheEntry: Failed to allocate memory.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto CLEAN_EXIT;
  }

  //
  // Return the address to the user.
  //
  *Entries = FindData;

  //
  // Dump the entries.
  //
  while (!NetMapIsEmpty (&FoundEntries)) {
    //
    // Get a cache entry from the map.
    //
    CacheEntry = NetMapRemoveHead (&FoundEntries, (VOID **)&CacheTable);

    //
    // Set the fields in FindData.
    //
    FindData->Size            = FoundEntryLength;
    FindData->DenyFlag        = (BOOLEAN)(CacheTable == &ArpService->DeniedCacheTable);
    FindData->StaticFlag      = (BOOLEAN)(CacheEntry->DefaultDecayTime == 0);
    FindData->HwAddressType   = ArpService->SnpMode.IfType;
    FindData->SwAddressType   = Instance->ConfigData.SwAddressType;
    FindData->HwAddressLength = (UINT8)ArpService->SnpMode.HwAddressSize;
    FindData->SwAddressLength = Instance->ConfigData.SwAddressLength;

    //
    // Copy the software address.
    //
    CopyMem (
      FindData + 1,
      CacheEntry->Addresses[Protocol].AddressPtr,
      FindData->SwAddressLength
      );

    //
    // Copy the hardware address.
    //
    CopyMem (
      (UINT8 *)(FindData + 1) + FindData->SwAddressLength,
      CacheEntry->Addresses[Hardware].AddressPtr,
      FindData->HwAddressLength
      );

    //
    // Slip to the next FindData.
    //
    FindData = (EFI_ARP_FIND_DATA *)((UINT8 *)FindData + FoundEntryLength);
  }

CLEAN_EXIT:

  NetMapClean (&FoundEntries);

  return Status;
}

