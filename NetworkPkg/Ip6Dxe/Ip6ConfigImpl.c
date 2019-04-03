/** @file
  The implementation of EFI IPv6 Configuration Protocol.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

LIST_ENTRY  mIp6ConfigInstanceList = {&mIp6ConfigInstanceList, &mIp6ConfigInstanceList};

/**
  The event process routine when the DHCPv6 service binding protocol is installed
  in the system.

  @param[in]     Event         Not used.
  @param[in]     Context       Pointer to the IP6 config instance data.

**/
VOID
EFIAPI
Ip6ConfigOnDhcp6SbInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Update the current policy to NewPolicy. During the transition
  period, the default router list, on-link prefix list, autonomous prefix list
  and address list in all interfaces will be released.

  @param[in]  IpSb               The IP6 service binding instance.
  @param[in]  NewPolicy          The new policy to be updated to.

**/
VOID
Ip6ConfigOnPolicyChanged (
  IN IP6_SERVICE            *IpSb,
  IN EFI_IP6_CONFIG_POLICY  NewPolicy
  )
{
  LIST_ENTRY          *Entry;
  LIST_ENTRY          *Entry2;
  LIST_ENTRY          *Next;
  IP6_INTERFACE       *IpIf;
  IP6_DAD_ENTRY       *DadEntry;
  IP6_DELAY_JOIN_LIST *DelayNode;
  IP6_ADDRESS_INFO    *AddrInfo;
  IP6_PROTOCOL        *Instance;
  BOOLEAN             Recovery;

  Recovery = FALSE;

  //
  // Currently there are only two policies: Manual and Automatic. Regardless of
  // what transition is going on, i.e., Manual -> Automatic and Automatic ->
  // Manual, we have to free default router list, on-link prefix list, autonomous
  // prefix list, address list in all the interfaces and destroy any IPv6 child
  // instance whose local IP is neither 0 nor the link-local address.
  //
  Ip6CleanDefaultRouterList (IpSb);
  Ip6CleanPrefixListTable (IpSb, &IpSb->OnlinkPrefix);
  Ip6CleanPrefixListTable (IpSb, &IpSb->AutonomousPrefix);

  //
  // It's tricky... If the LinkLocal address is O.K., add back the link-local
  // prefix to the on-link prefix table.
  //
  if (IpSb->LinkLocalOk) {
    Ip6CreatePrefixListEntry (
      IpSb,
      TRUE,
      (UINT32) IP6_INFINIT_LIFETIME,
      (UINT32) IP6_INFINIT_LIFETIME,
      IP6_LINK_LOCAL_PREFIX_LENGTH,
      &IpSb->LinkLocalAddr
      );
  }

  if (!IsListEmpty (&IpSb->DefaultInterface->AddressList) && IpSb->DefaultInterface->AddressCount > 0) {
    //
    // If any IPv6 children (Instance) in configured state and use global unicast address, it will be
    // destroyed in Ip6RemoveAddr() function later. Then, the upper layer driver's Stop() function will be
    // called, which may break the upper layer network stacks. So, the driver should take the responsibility
    // for the recovery by using ConnectController() after Ip6RemoveAddr().
    // Here, just check whether need to recover the upper layer network stacks later.
    //
    NET_LIST_FOR_EACH (Entry, &IpSb->DefaultInterface->AddressList) {
      AddrInfo = NET_LIST_USER_STRUCT_S (Entry, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);
      if (!IsListEmpty (&IpSb->Children)) {
        NET_LIST_FOR_EACH (Entry2, &IpSb->Children) {
          Instance = NET_LIST_USER_STRUCT_S (Entry2, IP6_PROTOCOL, Link, IP6_PROTOCOL_SIGNATURE);
          if ((Instance->State == IP6_STATE_CONFIGED) && EFI_IP6_EQUAL (&Instance->ConfigData.StationAddress, &AddrInfo->Address)) {
            Recovery = TRUE;
            break;
          }
        }
      }
    }

    //
    // All IPv6 children that use global unicast address as it's source address
    // should be destroyed now. The survivers are those use the link-local address
    // or the unspecified address as the source address.
    // TODO: Conduct a check here.
    Ip6RemoveAddr (
      IpSb,
      &IpSb->DefaultInterface->AddressList,
      &IpSb->DefaultInterface->AddressCount,
      NULL,
      0
      );

    if (IpSb->Controller != NULL && Recovery) {
      //
      // ConnectController() to recover the upper layer network stacks.
      //
      gBS->ConnectController (IpSb->Controller, NULL, NULL, TRUE);
    }
  }


  NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
    //
    // remove all pending delay node and DAD entries for the global addresses.
    //
    IpIf = NET_LIST_USER_STRUCT_S (Entry, IP6_INTERFACE, Link, IP6_INTERFACE_SIGNATURE);

    NET_LIST_FOR_EACH_SAFE (Entry2, Next, &IpIf->DelayJoinList) {
      DelayNode = NET_LIST_USER_STRUCT (Entry2, IP6_DELAY_JOIN_LIST, Link);
      if (!NetIp6IsLinkLocalAddr (&DelayNode->AddressInfo->Address)) {
        RemoveEntryList (&DelayNode->Link);
        FreePool (DelayNode);
      }
    }

    NET_LIST_FOR_EACH_SAFE (Entry2, Next, &IpIf->DupAddrDetectList) {
      DadEntry = NET_LIST_USER_STRUCT_S (Entry2, IP6_DAD_ENTRY, Link, IP6_DAD_ENTRY_SIGNATURE);

      if (!NetIp6IsLinkLocalAddr (&DadEntry->AddressInfo->Address)) {
        //
        // Fail this DAD entry if the address is not link-local.
        //
        Ip6OnDADFinished (FALSE, IpIf, DadEntry);
      }
    }
  }

  if (NewPolicy == Ip6ConfigPolicyAutomatic) {
    //
    // Set parameters to trigger router solicitation sending in timer handler.
    //
    IpSb->RouterAdvertiseReceived = FALSE;
    IpSb->SolicitTimer            = IP6_MAX_RTR_SOLICITATIONS;
    //
    // delay 1 second
    //
    IpSb->Ticks                   = (UINT32) IP6_GET_TICKS (IP6_ONE_SECOND_IN_MS);
  }
}

/**
  The work function to trigger the DHCPv6 process to perform a stateful autoconfiguration.

  @param[in]     Instance      Pointer to the IP6 config instance data.
  @param[in]     OtherInfoOnly If FALSE, get stateful address and other information
                               via DHCPv6. Otherwise, only get the other information.

  @retval    EFI_SUCCESS       The operation finished successfully.
  @retval    EFI_UNSUPPORTED   The DHCP6 driver is not available.

**/
EFI_STATUS
Ip6ConfigStartStatefulAutoConfig (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN BOOLEAN              OtherInfoOnly
  )
{
  EFI_STATUS                Status;
  IP6_SERVICE               *IpSb;
  EFI_DHCP6_CONFIG_DATA     Dhcp6CfgData;
  EFI_DHCP6_PROTOCOL        *Dhcp6;
  EFI_DHCP6_PACKET_OPTION   *OptList[1];
  UINT16                    OptBuf[4];
  EFI_DHCP6_PACKET_OPTION   *Oro;
  EFI_DHCP6_RETRANSMISSION  InfoReqReXmit;

  //
  // A host must not invoke stateful address configuration if it is already
  // participating in the statuful protocol as a result of an earlier advertisement.
  //
  if (Instance->Dhcp6Handle != NULL) {
    return EFI_SUCCESS;
  }

  IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);

  Instance->OtherInfoOnly = OtherInfoOnly;

  Status = NetLibCreateServiceChild (
             IpSb->Controller,
             IpSb->Image,
             &gEfiDhcp6ServiceBindingProtocolGuid,
             &Instance->Dhcp6Handle
             );

  if (Status == EFI_UNSUPPORTED) {
    //
    // No DHCPv6 Service Binding protocol, register a notify.
    //
    if (Instance->Dhcp6SbNotifyEvent == NULL) {
      Instance->Dhcp6SbNotifyEvent = EfiCreateProtocolNotifyEvent (
                                       &gEfiDhcp6ServiceBindingProtocolGuid,
                                       TPL_CALLBACK,
                                       Ip6ConfigOnDhcp6SbInstalled,
                                       (VOID *) Instance,
                                       &Instance->Registration
                                       );
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Instance->Dhcp6SbNotifyEvent != NULL) {
    gBS->CloseEvent (Instance->Dhcp6SbNotifyEvent);
  }

  Status = gBS->OpenProtocol (
                  Instance->Dhcp6Handle,
                  &gEfiDhcp6ProtocolGuid,
                  (VOID **) &Instance->Dhcp6,
                  IpSb->Image,
                  IpSb->Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  ASSERT_EFI_ERROR (Status);

  Dhcp6 = Instance->Dhcp6;
  Dhcp6->Configure (Dhcp6, NULL);

  //
  // Set the exta options to send. Here we only want the option request option
  // with DNS SERVERS.
  //
  Oro                         = (EFI_DHCP6_PACKET_OPTION *) OptBuf;
  Oro->OpCode                 = HTONS (DHCP6_OPT_ORO);
  Oro->OpLen                  = HTONS (2);
  *((UINT16 *) &Oro->Data[0]) = HTONS (DHCP6_OPT_DNS_SERVERS);
  OptList[0]                  = Oro;

  Status                      = EFI_SUCCESS;

  if (!OtherInfoOnly) {
    //
    // Get stateful address and other information via DHCPv6.
    //
    Dhcp6CfgData.Dhcp6Callback         = NULL;
    Dhcp6CfgData.CallbackContext       = NULL;
    Dhcp6CfgData.OptionCount           = 1;
    Dhcp6CfgData.OptionList            = &OptList[0];
    Dhcp6CfgData.IaDescriptor.Type     = EFI_DHCP6_IA_TYPE_NA;
    Dhcp6CfgData.IaDescriptor.IaId     = Instance->IaId;
    Dhcp6CfgData.IaInfoEvent           = Instance->Dhcp6Event;
    Dhcp6CfgData.ReconfigureAccept     = FALSE;
    Dhcp6CfgData.RapidCommit           = FALSE;
    Dhcp6CfgData.SolicitRetransmission = NULL;

    Status = Dhcp6->Configure (Dhcp6, &Dhcp6CfgData);

    if (!EFI_ERROR (Status)) {

      if (IpSb->LinkLocalOk) {
        Status = Dhcp6->Start (Dhcp6);
      } else {
        IpSb->Dhcp6NeedStart = TRUE;
      }

    }
  } else {
    //
    // Only get other information via DHCPv6, this doesn't require a config
    // action.
    //
    InfoReqReXmit.Irt = 4;
    InfoReqReXmit.Mrc = 64;
    InfoReqReXmit.Mrt = 60;
    InfoReqReXmit.Mrd = 0;

    if (IpSb->LinkLocalOk) {
      Status = Dhcp6->InfoRequest (
                        Dhcp6,
                        TRUE,
                        Oro,
                        0,
                        NULL,
                        &InfoReqReXmit,
                        Instance->Dhcp6Event,
                        Ip6ConfigOnDhcp6Reply,
                        Instance
                        );
    } else {
      IpSb->Dhcp6NeedInfoRequest = TRUE;
    }

  }

  return Status;
}

/**
  Signal the registered event. It is the callback routine for NetMapIterate.

  @param[in]  Map    Points to the list of registered event.
  @param[in]  Item   The registered event.
  @param[in]  Arg    Not used.

**/
EFI_STATUS
EFIAPI
Ip6ConfigSignalEvent (
  IN NET_MAP                *Map,
  IN NET_MAP_ITEM           *Item,
  IN VOID                   *Arg
  )
{
  gBS->SignalEvent ((EFI_EVENT) Item->Key);

  return EFI_SUCCESS;
}

/**
  Read the configuration data from variable storage according to the VarName and
  gEfiIp6ConfigProtocolGuid. It checks the integrity of variable data. If the
  data is corrupted, it clears the variable data to ZERO. Othewise, it outputs the
  configuration data to IP6_CONFIG_INSTANCE.

  @param[in]      VarName  The pointer to the variable name
  @param[in, out] Instance The pointer to the IP6 config instance data.

  @retval EFI_NOT_FOUND         The variable can not be found or already corrupted.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_SUCCESS           The configuration data was retrieved successfully.

**/
EFI_STATUS
Ip6ConfigReadConfigData (
  IN     CHAR16               *VarName,
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  )
{
  EFI_STATUS              Status;
  UINTN                   VarSize;
  IP6_CONFIG_VARIABLE     *Variable;
  IP6_CONFIG_DATA_ITEM    *DataItem;
  UINTN                   Index;
  IP6_CONFIG_DATA_RECORD  DataRecord;
  CHAR8                   *Data;

  //
  // Try to read the configuration variable.
  //
  VarSize = 0;
  Status  = gRT->GetVariable (
                   VarName,
                   &gEfiIp6ConfigProtocolGuid,
                   NULL,
                   &VarSize,
                   NULL
                   );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate buffer and read the config variable.
    //
    Variable = AllocatePool (VarSize);
    if (Variable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable (
                    VarName,
                    &gEfiIp6ConfigProtocolGuid,
                    NULL,
                    &VarSize,
                    Variable
                    );
    if (EFI_ERROR (Status) || (UINT16) (~NetblockChecksum ((UINT8 *) Variable, (UINT32) VarSize)) != 0) {
      //
      // GetVariable still error or the variable is corrupted.
      // Fall back to the default value.
      //
      FreePool (Variable);

      //
      // Remove the problematic variable and return EFI_NOT_FOUND, a new
      // variable will be set again.
      //
      gRT->SetVariable (
             VarName,
             &gEfiIp6ConfigProtocolGuid,
             IP6_CONFIG_VARIABLE_ATTRIBUTE,
             0,
             NULL
             );

      return EFI_NOT_FOUND;
    }

    //
    // Get the IAID we use.
    //
    Instance->IaId = Variable->IaId;

    for (Index = 0; Index < Variable->DataRecordCount; Index++) {

      CopyMem (&DataRecord, &Variable->DataRecord[Index], sizeof (DataRecord));

      DataItem = &Instance->DataItem[DataRecord.DataType];
      if (DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED) &&
          (DataItem->DataSize != DataRecord.DataSize)
          ) {
        //
        // Perhaps a corrupted data record...
        //
        continue;
      }

      if (!DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED)) {
        //
        // This data item has variable length data.
        //
        DataItem->Data.Ptr = AllocatePool (DataRecord.DataSize);
        if (DataItem->Data.Ptr == NULL) {
          //
          // no memory resource
          //
          continue;
        }
      }

      Data = (CHAR8 *) Variable + DataRecord.Offset;
      CopyMem (DataItem->Data.Ptr, Data, DataRecord.DataSize);

      DataItem->DataSize = DataRecord.DataSize;
      DataItem->Status   = EFI_SUCCESS;
    }

    FreePool (Variable);
    return EFI_SUCCESS;
  }

  return Status;
}

/**
  Write the configuration data from IP6_CONFIG_INSTANCE to variable storage.

  @param[in]      VarName  The pointer to the variable name.
  @param[in]      Instance The pointer to the IP6 configuration instance data.

  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_SUCCESS           The configuration data is written successfully.

**/
EFI_STATUS
Ip6ConfigWriteConfigData (
  IN CHAR16               *VarName,
  IN IP6_CONFIG_INSTANCE  *Instance
  )
{
  UINTN                   Index;
  UINTN                   VarSize;
  IP6_CONFIG_DATA_ITEM    *DataItem;
  IP6_CONFIG_VARIABLE     *Variable;
  IP6_CONFIG_DATA_RECORD  *DataRecord;
  CHAR8                   *Heap;
  EFI_STATUS              Status;

  VarSize = sizeof (IP6_CONFIG_VARIABLE) - sizeof (IP6_CONFIG_DATA_RECORD);

  for (Index = 0; Index < Ip6ConfigDataTypeMaximum; Index++) {

    DataItem = &Instance->DataItem[Index];
    if (!DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_VOLATILE) && !EFI_ERROR (DataItem->Status)) {

      VarSize += sizeof (IP6_CONFIG_DATA_RECORD) + DataItem->DataSize;
    }
  }

  Variable = AllocatePool (VarSize);
  if (Variable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Variable->IaId            = Instance->IaId;
  Heap                      = (CHAR8 *) Variable + VarSize;
  Variable->DataRecordCount = 0;

  for (Index = 0; Index < Ip6ConfigDataTypeMaximum; Index++) {

    DataItem = &Instance->DataItem[Index];
    if (!DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_VOLATILE) && !EFI_ERROR (DataItem->Status)) {

      Heap -= DataItem->DataSize;
      CopyMem (Heap, DataItem->Data.Ptr, DataItem->DataSize);

      DataRecord           = &Variable->DataRecord[Variable->DataRecordCount];
      DataRecord->DataType = (EFI_IP6_CONFIG_DATA_TYPE) Index;
      DataRecord->DataSize = (UINT32) DataItem->DataSize;
      DataRecord->Offset   = (UINT16) (Heap - (CHAR8 *) Variable);

      Variable->DataRecordCount++;
    }
  }

  Variable->Checksum = 0;
  Variable->Checksum = (UINT16) ~NetblockChecksum ((UINT8 *) Variable, (UINT32) VarSize);

  Status = gRT->SetVariable (
                  VarName,
                  &gEfiIp6ConfigProtocolGuid,
                  IP6_CONFIG_VARIABLE_ATTRIBUTE,
                  VarSize,
                  Variable
                  );

  FreePool (Variable);

  return Status;
}

/**
  The work function for EfiIp6ConfigGetData() to get the interface information
  of the communication device this IP6Config instance manages.

  @param[in]      Instance Pointer to the IP6 config instance data.
  @param[in, out] DataSize On input, in bytes, the size of Data. On output, in
                           bytes, the size of buffer required to store the specified
                           configuration data.
  @param[in]      Data     The data buffer in which the configuration data is returned.
                           Ignored if DataSize is ZERO.

  @retval EFI_BUFFER_TOO_SMALL The size of Data is too small for the specified
                               configuration data, and the required size is
                               returned in DataSize.
  @retval EFI_SUCCESS          The specified configuration data was obtained.

**/
EFI_STATUS
Ip6ConfigGetIfInfo (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN OUT UINTN            *DataSize,
  IN VOID                 *Data      OPTIONAL
  )
{
  IP6_SERVICE                    *IpSb;
  UINTN                          Length;
  IP6_CONFIG_DATA_ITEM           *Item;
  EFI_IP6_CONFIG_INTERFACE_INFO  *IfInfo;
  UINT32                         AddressCount;
  UINT32                         RouteCount;

  IpSb   = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
  Length = sizeof (EFI_IP6_CONFIG_INTERFACE_INFO);

  //
  // Calculate the required length, add the buffer size for AddressInfo and
  // RouteTable
  //
  Ip6BuildEfiAddressList (IpSb, &AddressCount, NULL);
  Ip6BuildEfiRouteTable (IpSb->RouteTable, &RouteCount, NULL);

  Length += AddressCount * sizeof (EFI_IP6_ADDRESS_INFO) + RouteCount * sizeof (EFI_IP6_ROUTE_TABLE);

  if (*DataSize < Length) {
    *DataSize = Length;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Copy the fixed size part of the interface info.
  //
  Item = &Instance->DataItem[Ip6ConfigDataTypeInterfaceInfo];
  IfInfo = (EFI_IP6_CONFIG_INTERFACE_INFO *) Data;
  CopyMem (IfInfo, Item->Data.Ptr, sizeof (EFI_IP6_CONFIG_INTERFACE_INFO));

  //
  // AddressInfo
  //
  IfInfo->AddressInfo = (EFI_IP6_ADDRESS_INFO *) (IfInfo + 1);
  Ip6BuildEfiAddressList (IpSb, &IfInfo->AddressInfoCount, &IfInfo->AddressInfo);

  //
  // RouteTable
  //
  IfInfo->RouteTable = (EFI_IP6_ROUTE_TABLE *) (IfInfo->AddressInfo + IfInfo->AddressInfoCount);
  Ip6BuildEfiRouteTable (IpSb->RouteTable, &IfInfo->RouteCount, &IfInfo->RouteTable);

  if (IfInfo->AddressInfoCount == 0) {
    IfInfo->AddressInfo = NULL;
  }

  if (IfInfo->RouteCount == 0) {
    IfInfo->RouteTable = NULL;
  }

  return EFI_SUCCESS;
}

/**
  The work function for EfiIp6ConfigSetData() to set the alternative inteface ID
  for the communication device managed by this IP6Config instance, if the link local
  IPv6 addresses generated from the interface ID based on the default source the
  EFI IPv6 Protocol uses is a duplicate address.

  @param[in]     Instance Pointer to the IP6 configuration instance data.
  @param[in]     DataSize Size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set.

  @retval EFI_BAD_BUFFER_SIZE  The DataSize does not match the size of the type,
                               8 bytes.
  @retval EFI_SUCCESS          The specified configuration data for the EFI IPv6
                               network stack was set.

**/
EFI_STATUS
Ip6ConfigSetAltIfId (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  )
{
  EFI_IP6_CONFIG_INTERFACE_ID  *OldIfId;
  EFI_IP6_CONFIG_INTERFACE_ID  *NewIfId;
  IP6_CONFIG_DATA_ITEM         *DataItem;

  if (DataSize != sizeof (EFI_IP6_CONFIG_INTERFACE_ID)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  DataItem = &Instance->DataItem[Ip6ConfigDataTypeAltInterfaceId];
  OldIfId  = DataItem->Data.AltIfId;
  NewIfId  = (EFI_IP6_CONFIG_INTERFACE_ID *) Data;

  CopyMem (OldIfId, NewIfId, DataSize);
  DataItem->Status = EFI_SUCCESS;

  return EFI_SUCCESS;
}

/**
  The work function for EfiIp6ConfigSetData() to set the general configuration
  policy for the EFI IPv6 network stack that is running on the communication device
  managed by this IP6Config instance. The policy will affect other configuration settings.

  @param[in]     Instance Pointer to the IP6 config instance data.
  @param[in]     DataSize Size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set.

  @retval EFI_INVALID_PARAMETER The to be set policy is invalid.
  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_ABORTED           The new policy equals the current policy.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set.

**/
EFI_STATUS
Ip6ConfigSetPolicy (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  )
{
  EFI_IP6_CONFIG_POLICY  NewPolicy;
  IP6_CONFIG_DATA_ITEM   *DataItem;
  IP6_SERVICE            *IpSb;

  if (DataSize != sizeof (EFI_IP6_CONFIG_POLICY)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NewPolicy = *((EFI_IP6_CONFIG_POLICY *) Data);

  if (NewPolicy > Ip6ConfigPolicyAutomatic) {
    return EFI_INVALID_PARAMETER;
  }

  if (NewPolicy == Instance->Policy) {

    return EFI_ABORTED;
  } else {
    //
    // Clean the ManualAddress, Gateway and DnsServers, shrink the variable
    // data size, and fire up all the related events.
    //
    DataItem           = &Instance->DataItem[Ip6ConfigDataTypeManualAddress];
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }
    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
    NetMapIterate (&DataItem->EventMap, Ip6ConfigSignalEvent, NULL);

    DataItem           = &Instance->DataItem[Ip6ConfigDataTypeGateway];
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }
    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
    NetMapIterate (&DataItem->EventMap, Ip6ConfigSignalEvent, NULL);

    DataItem           = &Instance->DataItem[Ip6ConfigDataTypeDnsServer];
    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
    NetMapIterate (&DataItem->EventMap, Ip6ConfigSignalEvent, NULL);

    if (NewPolicy == Ip6ConfigPolicyManual) {
      //
      // The policy is changed from automatic to manual. Stop the DHCPv6 process
      // and destroy the DHCPv6 child.
      //
      if (Instance->Dhcp6Handle != NULL) {
        Ip6ConfigDestroyDhcp6 (Instance);
      }
    }

    IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
    Ip6ConfigOnPolicyChanged (IpSb, NewPolicy);

    Instance->Policy = NewPolicy;

    return EFI_SUCCESS;
  }
}

/**
  The work function for EfiIp6ConfigSetData() to set the number of consecutive
  Neighbor Solicitation messages sent while performing Duplicate Address Detection
  on a tentative address. A value of ZERO indicates that Duplicate Address Detection
  will not be performed on a tentative address.

  @param[in]     Instance The Instance Pointer to the IP6 config instance data.
  @param[in]     DataSize Size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set.

  @retval EFI_BAD_BUFFER_SIZE  The DataSize does not match the size of the type.
  @retval EFI_ABORTED          The new transmit count equals the current configuration.
  @retval EFI_SUCCESS          The specified configuration data for the EFI IPv6
                               network stack was set.

**/
EFI_STATUS
Ip6ConfigSetDadXmits (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  )
{
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS  *OldDadXmits;

  if (DataSize != sizeof (EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  OldDadXmits = Instance->DataItem[Ip6ConfigDataTypeDupAddrDetectTransmits].Data.DadXmits;

  if ((*(UINT32 *) Data) == OldDadXmits->DupAddrDetectTransmits) {

    return EFI_ABORTED;
  } else {

    OldDadXmits->DupAddrDetectTransmits = *((UINT32 *) Data);
    return EFI_SUCCESS;
  }
}

/**
  The callback function for Ip6SetAddr. The prototype is defined
  as IP6_DAD_CALLBACK. It is called after Duplicate Address Detection is performed
  for the manual address set by Ip6ConfigSetManualAddress.

  @param[in]     IsDadPassed   If TRUE, Duplicate Address Detection passed.
  @param[in]     TargetAddress The tentative IPv6 address to be checked.
  @param[in]     Context       Pointer to the IP6 configuration instance data.

**/
VOID
Ip6ManualAddrDadCallback (
  IN BOOLEAN           IsDadPassed,
  IN EFI_IPv6_ADDRESS  *TargetAddress,
  IN VOID              *Context
  )
{
  IP6_CONFIG_INSTANCE            *Instance;
  UINTN                          Index;
  IP6_CONFIG_DATA_ITEM           *Item;
  EFI_IP6_CONFIG_MANUAL_ADDRESS  *ManualAddr;
  EFI_IP6_CONFIG_MANUAL_ADDRESS  *PassedAddr;
  UINTN                          DadPassCount;
  UINTN                          DadFailCount;
  IP6_SERVICE                    *IpSb;

  Instance   = (IP6_CONFIG_INSTANCE *) Context;
  NET_CHECK_SIGNATURE (Instance, IP6_CONFIG_INSTANCE_SIGNATURE);
  Item       = &Instance->DataItem[Ip6ConfigDataTypeManualAddress];
  ManualAddr = NULL;

  if (Item->DataSize == 0) {
    return;
  }

  for (Index = 0; Index < Item->DataSize / sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS); Index++) {
    //
    // Find the original tag used to place into the NET_MAP.
    //
    ManualAddr = Item->Data.ManualAddress + Index;
    if (EFI_IP6_EQUAL (TargetAddress, &ManualAddr->Address)) {
      break;
    }
  }

  ASSERT (Index != Item->DataSize / sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS));

  if (IsDadPassed) {
    NetMapInsertTail (&Instance->DadPassedMap, ManualAddr, NULL);
  } else {
    NetMapInsertTail (&Instance->DadFailedMap, ManualAddr, NULL);
  }

  DadPassCount = NetMapGetCount (&Instance->DadPassedMap);
  DadFailCount = NetMapGetCount (&Instance->DadFailedMap);

  if ((DadPassCount + DadFailCount) == (Item->DataSize / sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS))) {
    //
    // All addresses have finished the configuration process.
    //
    if (DadFailCount != 0) {
      //
      // There is at least one duplicate address.
      //
      FreePool (Item->Data.Ptr);

      Item->DataSize = DadPassCount * sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS);
      if (Item->DataSize == 0) {
        //
        // All failed, bad luck.
        //
        Item->Data.Ptr = NULL;
        Item->Status   = EFI_NOT_FOUND;
      } else {
        //
        // Part of addresses are detected to be duplicates, so update the
        // data with those passed.
        //
        PassedAddr = (EFI_IP6_CONFIG_MANUAL_ADDRESS *) AllocatePool (Item->DataSize);
        ASSERT (PassedAddr != NULL);

        Item->Data.Ptr = PassedAddr;
        Item->Status   = EFI_SUCCESS;

        while (!NetMapIsEmpty (&Instance->DadPassedMap)) {
          ManualAddr = (EFI_IP6_CONFIG_MANUAL_ADDRESS *) NetMapRemoveHead (&Instance->DadPassedMap, NULL);
          CopyMem (PassedAddr, ManualAddr, sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS));

          PassedAddr++;
        }

        ASSERT ((UINTN) PassedAddr - (UINTN) Item->Data.Ptr == Item->DataSize);
      }
    } else {
      //
      // All addresses are valid.
      //
      Item->Status = EFI_SUCCESS;
    }

    //
    // Remove the tags we put in the NET_MAPs.
    //
    while (!NetMapIsEmpty (&Instance->DadFailedMap)) {
      NetMapRemoveHead (&Instance->DadFailedMap, NULL);
    }

    while (!NetMapIsEmpty (&Instance->DadPassedMap)) {
      NetMapRemoveHead (&Instance->DadPassedMap, NULL);
    }

    //
    // Signal the waiting events.
    //
    NetMapIterate (&Item->EventMap, Ip6ConfigSignalEvent, NULL);
    IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
    Ip6ConfigWriteConfigData (IpSb->MacString, Instance);
  }
}

/**
  The work function for EfiIp6ConfigSetData() to set the station addresses manually
  for the EFI IPv6 network stack. It is only configurable when the policy is
  Ip6ConfigPolicyManual.

  @param[in]     Instance Pointer to the IP6 configuration instance data.
  @param[in]     DataSize Size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set.

  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_WRITE_PROTECTED   The specified configuration data cannot be set
                                under the current policy.
  @retval EFI_INVALID_PARAMETER One or more fields in Data is invalid.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_NOT_READY         An asynchrous process is invoked to set the specified
                                configuration data, and the process is not finished.
  @retval EFI_ABORTED           The manual addresses to be set equal current
                                configuration.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set.

**/
EFI_STATUS
Ip6ConfigSetManualAddress (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  )
{
  EFI_IP6_CONFIG_MANUAL_ADDRESS  *NewAddress;
  EFI_IP6_CONFIG_MANUAL_ADDRESS  *TmpAddress;
  IP6_CONFIG_DATA_ITEM           *DataItem;
  UINTN                          NewAddressCount;
  UINTN                          Index1;
  UINTN                          Index2;
  IP6_SERVICE                    *IpSb;
  IP6_ADDRESS_INFO               *CurrentAddrInfo;
  IP6_ADDRESS_INFO               *Copy;
  LIST_ENTRY                     CurrentSourceList;
  UINT32                         CurrentSourceCount;
  LIST_ENTRY                     *Entry;
  LIST_ENTRY                     *Entry2;
  IP6_INTERFACE                  *IpIf;
  IP6_PREFIX_LIST_ENTRY          *PrefixEntry;
  EFI_STATUS                     Status;
  BOOLEAN                        IsUpdated;
  LIST_ENTRY                     *Next;
  IP6_DAD_ENTRY                  *DadEntry;
  IP6_DELAY_JOIN_LIST            *DelayNode;

  NewAddress      = NULL;
  TmpAddress      = NULL;
  CurrentAddrInfo = NULL;
  Copy            = NULL;
  Entry           = NULL;
  Entry2          = NULL;
  IpIf            = NULL;
  PrefixEntry     = NULL;
  Next            = NULL;
  DadEntry        = NULL;
  DelayNode       = NULL;
  Status          = EFI_SUCCESS;

  ASSERT (Instance->DataItem[Ip6ConfigDataTypeManualAddress].Status != EFI_NOT_READY);

  if ((DataSize != 0) && ((DataSize % sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS)) != 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Instance->Policy != Ip6ConfigPolicyManual) {
    return EFI_WRITE_PROTECTED;
  }

  IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);

  DataItem = &Instance->DataItem[Ip6ConfigDataTypeManualAddress];

  if (Data != NULL && DataSize != 0) {
    NewAddressCount = DataSize / sizeof (EFI_IP6_CONFIG_MANUAL_ADDRESS);
    NewAddress      = (EFI_IP6_CONFIG_MANUAL_ADDRESS *) Data;

    for (Index1 = 0; Index1 < NewAddressCount; Index1++, NewAddress++) {

      if (NetIp6IsLinkLocalAddr (&NewAddress->Address)    ||
          !NetIp6IsValidUnicast (&NewAddress->Address)    ||
          (NewAddress->PrefixLength > 128)
          ) {
        //
        // make sure the IPv6 address is unicast and not link-local address &&
        // the prefix length is valid.
        //
        return EFI_INVALID_PARAMETER;
      }

      TmpAddress = NewAddress + 1;
      for (Index2 = Index1 + 1; Index2 < NewAddressCount; Index2++, TmpAddress++) {
        //
        // Any two addresses in the array can't be equal.
        //
        if (EFI_IP6_EQUAL (&TmpAddress->Address, &NewAddress->Address)) {

          return EFI_INVALID_PARAMETER;
        }
      }
    }

    //
    // Build the current source address list.
    //
    InitializeListHead (&CurrentSourceList);
    CurrentSourceCount = 0;

    NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
      IpIf = NET_LIST_USER_STRUCT_S (Entry, IP6_INTERFACE, Link, IP6_INTERFACE_SIGNATURE);

      NET_LIST_FOR_EACH (Entry2, &IpIf->AddressList) {
        CurrentAddrInfo = NET_LIST_USER_STRUCT_S (Entry2, IP6_ADDRESS_INFO, Link, IP6_ADDR_INFO_SIGNATURE);

        Copy            = AllocateCopyPool (sizeof (IP6_ADDRESS_INFO), CurrentAddrInfo);
        if (Copy == NULL) {
          break;
        }

        InsertTailList (&CurrentSourceList, &Copy->Link);
        CurrentSourceCount++;
      }
    }

    //
    // Update the value... a long journey starts
    //
    NewAddress = AllocateCopyPool (DataSize, Data);
    if (NewAddress == NULL) {
      Ip6RemoveAddr (NULL, &CurrentSourceList, &CurrentSourceCount, NULL, 0);

      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Store the new data, and init the DataItem status to EFI_NOT_READY because
    // we may have an asynchronous configuration process.
    //
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }
    DataItem->Data.Ptr = NewAddress;
    DataItem->DataSize = DataSize;
    DataItem->Status   = EFI_NOT_READY;

    //
    // Trigger DAD, it's an asynchronous process.
    //
    IsUpdated  = FALSE;

    for (Index1 = 0; Index1 < NewAddressCount; Index1++, NewAddress++) {
      if (Ip6IsOneOfSetAddress (IpSb, &NewAddress->Address, NULL, &CurrentAddrInfo)) {
        ASSERT (CurrentAddrInfo != NULL);
        //
        // Remove this already existing source address from the CurrentSourceList
        // built before.
        //
        Ip6RemoveAddr (
          NULL,
          &CurrentSourceList,
          &CurrentSourceCount,
          &CurrentAddrInfo->Address,
          128
          );

        //
        // If the new address's prefix length is not specified, just use the previous configured
        // prefix length for this address.
        //
        if (NewAddress->PrefixLength == 0) {
          NewAddress->PrefixLength = CurrentAddrInfo->PrefixLength;
        }

        //
        // This manual address is already in use, see whether prefix length is changed.
        //
        if (NewAddress->PrefixLength != CurrentAddrInfo->PrefixLength) {
          //
          // Remove the on-link prefix table, the route entry will be removed
          // implicitly.
          //
          PrefixEntry = Ip6FindPrefixListEntry (
                          IpSb,
                          TRUE,
                          CurrentAddrInfo->PrefixLength,
                          &CurrentAddrInfo->Address
                          );
          if (PrefixEntry != NULL) {
            Ip6DestroyPrefixListEntry (IpSb, PrefixEntry, TRUE, FALSE);
          }

          //
          // Save the prefix length.
          //
          CurrentAddrInfo->PrefixLength = NewAddress->PrefixLength;
          IsUpdated = TRUE;
        }

        //
        // create a new on-link prefix entry.
        //
        PrefixEntry = Ip6FindPrefixListEntry (
                        IpSb,
                        TRUE,
                        NewAddress->PrefixLength,
                        &NewAddress->Address
                        );
        if (PrefixEntry == NULL) {
          Ip6CreatePrefixListEntry (
            IpSb,
            TRUE,
            (UINT32) IP6_INFINIT_LIFETIME,
            (UINT32) IP6_INFINIT_LIFETIME,
            NewAddress->PrefixLength,
            &NewAddress->Address
            );
        }

        CurrentAddrInfo->IsAnycast = NewAddress->IsAnycast;
        //
        // Artificially mark this address passed DAD be'coz it is already in use.
        //
        Ip6ManualAddrDadCallback (TRUE, &NewAddress->Address, Instance);
      } else {
        //
        // A new address.
        //
        IsUpdated = TRUE;

        //
        // Set the new address, this will trigger DAD and activate the address if
        // DAD succeeds.
        //
        Ip6SetAddress (
          IpSb->DefaultInterface,
          &NewAddress->Address,
          NewAddress->IsAnycast,
          NewAddress->PrefixLength,
          (UINT32) IP6_INFINIT_LIFETIME,
          (UINT32) IP6_INFINIT_LIFETIME,
          Ip6ManualAddrDadCallback,
          Instance
          );
      }
    }

    //
    // Check the CurrentSourceList, it now contains those addresses currently in
    // use and will be removed.
    //
    IpIf = IpSb->DefaultInterface;

    while (!IsListEmpty (&CurrentSourceList)) {
      IsUpdated = TRUE;

      CurrentAddrInfo = NET_LIST_HEAD (&CurrentSourceList, IP6_ADDRESS_INFO, Link);

      //
      // This local address is going to be removed, the IP instances that are
      // currently using it will be destroyed.
      //
      Ip6RemoveAddr (
        IpSb,
        &IpIf->AddressList,
        &IpIf->AddressCount,
        &CurrentAddrInfo->Address,
        128
        );

      //
      // Remove the on-link prefix table, the route entry will be removed
      // implicitly.
      //
      PrefixEntry = Ip6FindPrefixListEntry (
                      IpSb,
                      TRUE,
                      CurrentAddrInfo->PrefixLength,
                      &CurrentAddrInfo->Address
                      );
      if (PrefixEntry != NULL) {
        Ip6DestroyPrefixListEntry (IpSb, PrefixEntry, TRUE, FALSE);
      }

      RemoveEntryList (&CurrentAddrInfo->Link);
      FreePool (CurrentAddrInfo);
    }

    if (IsUpdated) {
      if (DataItem->Status == EFI_NOT_READY) {
        //
        // If DAD is disabled on this interface, the configuration process is
        // actually synchronous, and the data item's status will be changed to
        // the final status before we reach here, just check it.
        //
        Status = EFI_NOT_READY;
      } else {
        Status = EFI_SUCCESS;
      }
    } else {
      //
      // No update is taken, reset the status to success and return EFI_ABORTED.
      //
      DataItem->Status = EFI_SUCCESS;
      Status           = EFI_ABORTED;
    }
  } else {
    //
    // DataSize is 0 and Data is NULL, clean up the manual address.
    //
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }
    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;

    Ip6CleanDefaultRouterList (IpSb);
    Ip6CleanPrefixListTable (IpSb, &IpSb->OnlinkPrefix);
    Ip6CleanPrefixListTable (IpSb, &IpSb->AutonomousPrefix);
    Ip6CleanAssembleTable (&IpSb->Assemble);

    if (IpSb->LinkLocalOk) {
      Ip6CreatePrefixListEntry (
        IpSb,
        TRUE,
        (UINT32) IP6_INFINIT_LIFETIME,
        (UINT32) IP6_INFINIT_LIFETIME,
        IP6_LINK_LOCAL_PREFIX_LENGTH,
        &IpSb->LinkLocalAddr
        );
    }

    Ip6RemoveAddr (
      IpSb,
      &IpSb->DefaultInterface->AddressList,
      &IpSb->DefaultInterface->AddressCount,
      NULL,
      0
      );

    NET_LIST_FOR_EACH (Entry, &IpSb->Interfaces) {
      //
      // Remove all pending delay node and DAD entries for the global addresses.
      //
      IpIf = NET_LIST_USER_STRUCT_S (Entry, IP6_INTERFACE, Link, IP6_INTERFACE_SIGNATURE);

      NET_LIST_FOR_EACH_SAFE (Entry2, Next, &IpIf->DelayJoinList) {
        DelayNode = NET_LIST_USER_STRUCT (Entry2, IP6_DELAY_JOIN_LIST, Link);
        if (!NetIp6IsLinkLocalAddr (&DelayNode->AddressInfo->Address)) {
          RemoveEntryList (&DelayNode->Link);
          FreePool (DelayNode);
        }
      }

      NET_LIST_FOR_EACH_SAFE (Entry2, Next, &IpIf->DupAddrDetectList) {
        DadEntry = NET_LIST_USER_STRUCT_S (Entry2, IP6_DAD_ENTRY, Link, IP6_DAD_ENTRY_SIGNATURE);

        if (!NetIp6IsLinkLocalAddr (&DadEntry->AddressInfo->Address)) {
          //
          // Fail this DAD entry if the address is not link-local.
          //
          Ip6OnDADFinished (FALSE, IpIf, DadEntry);
        }
      }
    }
  }

  return Status;
}

/**
  The work function for EfiIp6ConfigSetData() to set the gateway addresses manually
  for the EFI IPv6 network stack that is running on the communication device that
  this EFI IPv6 Configuration Protocol manages. It is not configurable when the policy is
  Ip6ConfigPolicyAutomatic. The gateway addresses must be unicast IPv6 addresses.

  @param[in]     Instance The pointer to the IP6 config instance data.
  @param[in]     DataSize The size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set. This points to an array of
                          EFI_IPv6_ADDRESS instances.

  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_WRITE_PROTECTED   The specified configuration data cannot be set
                                under the current policy.
  @retval EFI_INVALID_PARAMETER One or more fields in Data is invalid.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to complete the operation.
  @retval EFI_ABORTED           The manual gateway addresses to be set equal the
                                current configuration.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set.

**/
EFI_STATUS
Ip6ConfigSetGateway (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  )
{
  UINTN                 Index1;
  UINTN                 Index2;
  EFI_IPv6_ADDRESS      *OldGateway;
  EFI_IPv6_ADDRESS      *NewGateway;
  UINTN                 OldGatewayCount;
  UINTN                 NewGatewayCount;
  IP6_CONFIG_DATA_ITEM  *Item;
  BOOLEAN               OneRemoved;
  BOOLEAN               OneAdded;
  IP6_SERVICE           *IpSb;
  IP6_DEFAULT_ROUTER    *DefaultRouter;
  VOID                  *Tmp;

  OldGateway      = NULL;
  NewGateway      = NULL;
  Item            = NULL;
  DefaultRouter   = NULL;
  Tmp             = NULL;
  OneRemoved      = FALSE;
  OneAdded        = FALSE;

  if ((DataSize != 0) && (DataSize % sizeof (EFI_IPv6_ADDRESS) != 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Instance->Policy != Ip6ConfigPolicyManual) {
    return EFI_WRITE_PROTECTED;
  }

  IpSb            = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
  Item            = &Instance->DataItem[Ip6ConfigDataTypeGateway];
  OldGateway      = Item->Data.Gateway;
  OldGatewayCount = Item->DataSize / sizeof (EFI_IPv6_ADDRESS);

  for (Index1 = 0; Index1 < OldGatewayCount; Index1++) {
    //
    // Remove this default router.
    //
    DefaultRouter = Ip6FindDefaultRouter (IpSb, OldGateway + Index1);
    if (DefaultRouter != NULL) {
      Ip6DestroyDefaultRouter (IpSb, DefaultRouter);
      OneRemoved = TRUE;
    }
  }

  if (Data != NULL && DataSize != 0) {
    NewGateway      = (EFI_IPv6_ADDRESS *) Data;
    NewGatewayCount = DataSize / sizeof (EFI_IPv6_ADDRESS);
    for (Index1 = 0; Index1 < NewGatewayCount; Index1++) {

      if (!NetIp6IsValidUnicast (NewGateway + Index1)) {

        return EFI_INVALID_PARAMETER;
      }

      for (Index2 = Index1 + 1; Index2 < NewGatewayCount; Index2++) {
        if (EFI_IP6_EQUAL (NewGateway + Index1, NewGateway + Index2)) {
          return EFI_INVALID_PARAMETER;
        }
      }
    }

    if (NewGatewayCount != OldGatewayCount) {
      Tmp = AllocatePool (DataSize);
      if (Tmp == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      Tmp = NULL;
    }

    for (Index1 = 0; Index1 < NewGatewayCount; Index1++) {

      DefaultRouter = Ip6FindDefaultRouter (IpSb, NewGateway + Index1);
      if (DefaultRouter == NULL) {
        Ip6CreateDefaultRouter (IpSb, NewGateway + Index1, IP6_INF_ROUTER_LIFETIME);
        OneAdded = TRUE;
      }
    }

    if (!OneRemoved && !OneAdded) {
      Item->Status = EFI_SUCCESS;
      return EFI_ABORTED;
    } else {

      if (Tmp != NULL) {
        if (Item->Data.Ptr != NULL) {
          FreePool (Item->Data.Ptr);
        }
        Item->Data.Ptr = Tmp;
      }

      CopyMem (Item->Data.Ptr, Data, DataSize);
      Item->DataSize = DataSize;
      Item->Status   = EFI_SUCCESS;
      return EFI_SUCCESS;
    }
  } else {
    //
    // DataSize is 0 and Data is NULL, clean up the Gateway address.
    //
    if (Item->Data.Ptr != NULL) {
      FreePool (Item->Data.Ptr);
    }
    Item->Data.Ptr = NULL;
    Item->DataSize = 0;
    Item->Status   = EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  The work function for EfiIp6ConfigSetData() to set the DNS server list for the
  EFI IPv6 network stack running on the communication device that this EFI IPv6
  Configuration Protocol manages. It is not configurable when the policy is
  Ip6ConfigPolicyAutomatic. The DNS server addresses must be unicast IPv6 addresses.

  @param[in]     Instance The pointer to the IP6 config instance data.
  @param[in]     DataSize The size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set, points to an array of
                          EFI_IPv6_ADDRESS instances.

  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_WRITE_PROTECTED   The specified configuration data cannot be set
                                under the current policy.
  @retval EFI_INVALID_PARAMETER One or more fields in Data is invalid.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.
  @retval EFI_ABORTED           The DNS server addresses to be set equal the current
                                configuration.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set.

**/
EFI_STATUS
Ip6ConfigSetDnsServer (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  )
{
  UINTN                 OldIndex;
  UINTN                 NewIndex;
  EFI_IPv6_ADDRESS      *OldDns;
  EFI_IPv6_ADDRESS      *NewDns;
  UINTN                 OldDnsCount;
  UINTN                 NewDnsCount;
  IP6_CONFIG_DATA_ITEM  *Item;
  BOOLEAN               OneAdded;
  VOID                  *Tmp;

  OldDns = NULL;
  NewDns = NULL;
  Item   = NULL;
  Tmp    = NULL;

  if ((DataSize != 0) && (DataSize % sizeof (EFI_IPv6_ADDRESS) != 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Instance->Policy != Ip6ConfigPolicyManual) {
    return EFI_WRITE_PROTECTED;
  }

  Item = &Instance->DataItem[Ip6ConfigDataTypeDnsServer];

  if (Data != NULL && DataSize != 0) {
    NewDns      = (EFI_IPv6_ADDRESS *) Data;
    OldDns      = Item->Data.DnsServers;
    NewDnsCount = DataSize / sizeof (EFI_IPv6_ADDRESS);
    OldDnsCount = Item->DataSize / sizeof (EFI_IPv6_ADDRESS);
    OneAdded    = FALSE;

    if (NewDnsCount != OldDnsCount) {
      Tmp = AllocatePool (DataSize);
      if (Tmp == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      Tmp = NULL;
    }

    for (NewIndex = 0; NewIndex < NewDnsCount; NewIndex++) {

      if (!NetIp6IsValidUnicast (NewDns + NewIndex)) {
        //
        // The dns server address must be unicast.
        //
        if (Tmp != NULL) {
          FreePool (Tmp);
        }
        return EFI_INVALID_PARAMETER;
      }

      if (OneAdded) {
        //
        // If any address in the new setting is not in the old settings, skip the
        // comparision below.
        //
        continue;
      }

      for (OldIndex = 0; OldIndex < OldDnsCount; OldIndex++) {
        if (EFI_IP6_EQUAL (NewDns + NewIndex, OldDns + OldIndex)) {
          //
          // If found break out.
          //
          break;
        }
      }

      if (OldIndex == OldDnsCount) {
        OneAdded = TRUE;
      }
    }

    if (!OneAdded && (DataSize == Item->DataSize)) {
      //
      // No new item is added and the size is the same.
      //
      Item->Status = EFI_SUCCESS;
      return EFI_ABORTED;
    } else {
      if (Tmp != NULL) {
        if (Item->Data.Ptr != NULL) {
          FreePool (Item->Data.Ptr);
        }
        Item->Data.Ptr = Tmp;
      }

      CopyMem (Item->Data.Ptr, Data, DataSize);
      Item->DataSize = DataSize;
      Item->Status   = EFI_SUCCESS;
    }
  } else  {
    //
    // DataSize is 0 and Data is NULL, clean up the DnsServer address.
    //
    if (Item->Data.Ptr != NULL) {
      FreePool (Item->Data.Ptr);
    }
    Item->Data.Ptr = NULL;
    Item->DataSize = 0;
    Item->Status   = EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Generate the operational state of the interface this IP6 config instance manages
  and output in EFI_IP6_CONFIG_INTERFACE_INFO.

  @param[in]      IpSb     The pointer to the IP6 service binding instance.
  @param[out]     IfInfo   The pointer to the IP6 configuration interface information structure.

**/
VOID
Ip6ConfigInitIfInfo (
  IN  IP6_SERVICE                    *IpSb,
  OUT EFI_IP6_CONFIG_INTERFACE_INFO  *IfInfo
  )
{
  UnicodeSPrint (
    IfInfo->Name,
    sizeof (IfInfo->Name),
    L"eth%d",
    IpSb->Ip6ConfigInstance.IfIndex
  );

  IfInfo->IfType        = IpSb->SnpMode.IfType;
  IfInfo->HwAddressSize = IpSb->SnpMode.HwAddressSize;
  CopyMem (&IfInfo->HwAddress, &IpSb->SnpMode.CurrentAddress, IfInfo->HwAddressSize);
}

/**
  Parse DHCPv6 reply packet to get the DNS server list.
  It is the work function for Ip6ConfigOnDhcp6Reply and Ip6ConfigOnDhcp6Event.

  @param[in]      Dhcp6    The pointer to the EFI_DHCP6_PROTOCOL instance.
  @param[in, out] Instance The pointer to the IP6 configuration instance data.
  @param[in]      Reply    The pointer to the DHCPv6 reply packet.

  @retval EFI_SUCCESS      The DNS server address was retrieved from the reply packet.
  @retval EFI_NOT_READY    The reply packet does not contain the DNS server option, or
                           the DNS server address is not valid.

**/
EFI_STATUS
Ip6ConfigParseDhcpReply (
  IN     EFI_DHCP6_PROTOCOL  *Dhcp6,
  IN OUT IP6_CONFIG_INSTANCE *Instance,
  IN     EFI_DHCP6_PACKET    *Reply
  )
{
  EFI_STATUS               Status;
  UINT32                   OptCount;
  EFI_DHCP6_PACKET_OPTION  **OptList;
  UINT16                   OpCode;
  UINT16                   Length;
  UINTN                    Index;
  UINTN                    Index2;
  EFI_IPv6_ADDRESS         *DnsServer;
  IP6_CONFIG_DATA_ITEM     *Item;

  //
  // A DHCPv6 reply packet is received as the response to our InfoRequest
  // packet.
  //
  OptCount = 0;
  Status   = Dhcp6->Parse (Dhcp6, Reply, &OptCount, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_READY;
  }

  OptList = AllocatePool (OptCount * sizeof (EFI_DHCP6_PACKET_OPTION *));
  if (OptList == NULL) {
    return EFI_NOT_READY;
  }

  Status = Dhcp6->Parse (Dhcp6, Reply, &OptCount, OptList);
  if (EFI_ERROR (Status)) {
    Status = EFI_NOT_READY;
    goto ON_EXIT;
  }

  Status = EFI_SUCCESS;

  for (Index = 0; Index < OptCount; Index++) {
    //
    // Go through all the options to check the ones we are interested in.
    // The OpCode and Length are in network byte-order and may not be naturally
    // aligned.
    //
    CopyMem (&OpCode, &OptList[Index]->OpCode, sizeof (OpCode));
    OpCode = NTOHS (OpCode);

    if (OpCode == DHCP6_OPT_DNS_SERVERS) {
      CopyMem (&Length, &OptList[Index]->OpLen, sizeof (Length));
      Length = NTOHS (Length);

      if ((Length == 0) || ((Length % sizeof (EFI_IPv6_ADDRESS)) != 0)) {
        //
        // The length should be a multiple of 16 bytes.
        //
        Status = EFI_NOT_READY;
        break;
      }

      //
      // Validate the DnsServers: whether they are unicast addresses.
      //
      DnsServer = (EFI_IPv6_ADDRESS *) OptList[Index]->Data;
      for (Index2 = 0; Index2 < Length / sizeof (EFI_IPv6_ADDRESS); Index2++) {
        if (!NetIp6IsValidUnicast (DnsServer)) {
          Status = EFI_NOT_READY;
          goto ON_EXIT;
        }

        DnsServer++;
      }

      Item = &Instance->DataItem[Ip6ConfigDataTypeDnsServer];

      if (Item->DataSize != Length) {
        if (Item->Data.Ptr != NULL) {
          FreePool (Item->Data.Ptr);
        }

        Item->Data.Ptr = AllocatePool (Length);
        ASSERT (Item->Data.Ptr != NULL);
      }

      CopyMem (Item->Data.Ptr, OptList[Index]->Data, Length);
      Item->DataSize = Length;
      Item->Status   = EFI_SUCCESS;

      //
      // Signal the waiting events.
      //
      NetMapIterate (&Item->EventMap, Ip6ConfigSignalEvent, NULL);

      break;
    }
  }

ON_EXIT:

  FreePool (OptList);
  return Status;
}

/**
  The callback function for Ip6SetAddr. The prototype is defined
  as IP6_DAD_CALLBACK. It is called after Duplicate Address Detection is performed
  on the tentative address by DHCPv6 in Ip6ConfigOnDhcp6Event().

  @param[in]     IsDadPassed   If TRUE, Duplicate Address Detection passes.
  @param[in]     TargetAddress The tentative IPv6 address to be checked.
  @param[in]     Context       Pointer to the IP6 configuration instance data.

**/
VOID
Ip6ConfigSetStatefulAddrCallback (
  IN BOOLEAN           IsDadPassed,
  IN EFI_IPv6_ADDRESS  *TargetAddress,
  IN VOID              *Context
  )
{
  IP6_CONFIG_INSTANCE  *Instance;

  Instance = (IP6_CONFIG_INSTANCE *) Context;
  NET_CHECK_SIGNATURE (Instance, IP6_CONFIG_INSTANCE_SIGNATURE);

  //
  // We should record the addresses that fail the DAD, and DECLINE them.
  //
  if (IsDadPassed) {
    //
    // Decrease the count, no interests in those passed DAD.
    //
    if (Instance->FailedIaAddressCount > 0 ) {
      Instance->FailedIaAddressCount--;
    }
  } else {
    //
    // Record it.
    //
    IP6_COPY_ADDRESS (Instance->DeclineAddress + Instance->DeclineAddressCount, TargetAddress);
    Instance->DeclineAddressCount++;
  }

  if (Instance->FailedIaAddressCount == Instance->DeclineAddressCount) {
    //
    // The checking on all addresses are finished.
    //
    if (Instance->DeclineAddressCount != 0) {
      //
      // Decline those duplicates.
      //
      if (Instance->Dhcp6 != NULL) {
        Instance->Dhcp6->Decline (
                           Instance->Dhcp6,
                           Instance->DeclineAddressCount,
                           Instance->DeclineAddress
                           );
      }
    }

    if (Instance->DeclineAddress != NULL) {
      FreePool (Instance->DeclineAddress);
    }
    Instance->DeclineAddress      = NULL;
    Instance->DeclineAddressCount = 0;
  }
}

/**
  The event handle routine when DHCPv6 process is finished or is updated.

  @param[in]     Event         Not used.
  @param[in]     Context       The pointer to the IP6 configuration instance data.

**/
VOID
EFIAPI
Ip6ConfigOnDhcp6Event (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP6_CONFIG_INSTANCE      *Instance;
  EFI_DHCP6_PROTOCOL       *Dhcp6;
  EFI_STATUS               Status;
  EFI_DHCP6_MODE_DATA      Dhcp6ModeData;
  EFI_DHCP6_IA             *Ia;
  EFI_DHCP6_IA_ADDRESS     *IaAddr;
  UINT32                   Index;
  IP6_SERVICE              *IpSb;
  IP6_ADDRESS_INFO         *AddrInfo;
  IP6_INTERFACE            *IpIf;

  Instance = (IP6_CONFIG_INSTANCE *) Context;

  if ((Instance->Policy != Ip6ConfigPolicyAutomatic) || Instance->OtherInfoOnly) {
    //
    // IPv6 is not operating in the automatic policy now or
    // the DHCPv6 information request message exchange is aborted.
    //
    return ;
  }

  //
  // The stateful address autoconfiguration is done or updated.
  //
  Dhcp6 = Instance->Dhcp6;

  Status = Dhcp6->GetModeData (Dhcp6, &Dhcp6ModeData, NULL);
  if (EFI_ERROR (Status)) {
    return ;
  }

  IpSb   = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
  IpIf   = IpSb->DefaultInterface;
  Ia     = Dhcp6ModeData.Ia;
  IaAddr = Ia->IaAddress;

  if (Instance->DeclineAddress != NULL) {
    FreePool (Instance->DeclineAddress);
  }

  Instance->DeclineAddress = (EFI_IPv6_ADDRESS *) AllocatePool (Ia->IaAddressCount * sizeof (EFI_IPv6_ADDRESS));
  if (Instance->DeclineAddress == NULL) {
    goto ON_EXIT;
  }

  Instance->FailedIaAddressCount = Ia->IaAddressCount;
  Instance->DeclineAddressCount   = 0;

  for (Index = 0; Index < Ia->IaAddressCount; Index++, IaAddr++) {
    if (Ia->IaAddress[Index].ValidLifetime != 0 && Ia->State == Dhcp6Bound) {
      //
      // Set this address, either it's a new address or with updated lifetimes.
      // An appropriate prefix length will be set.
      //
      Ip6SetAddress (
        IpIf,
        &IaAddr->IpAddress,
        FALSE,
        0,
        IaAddr->ValidLifetime,
        IaAddr->PreferredLifetime,
        Ip6ConfigSetStatefulAddrCallback,
        Instance
        );
    } else {
      //
      // discard this address, artificially decrease the count as if this address
      // passed DAD.
      //
      if (Ip6IsOneOfSetAddress (IpSb, &IaAddr->IpAddress, NULL, &AddrInfo)) {
        ASSERT (AddrInfo != NULL);
        Ip6RemoveAddr (
          IpSb,
          &IpIf->AddressList,
          &IpIf->AddressCount,
          &AddrInfo->Address,
          AddrInfo->PrefixLength
          );
      }

      if (Instance->FailedIaAddressCount > 0) {
        Instance->FailedIaAddressCount--;
      }
    }
  }

  //
  // Parse the Reply packet to get the options we need.
  //
  if (Dhcp6ModeData.Ia->ReplyPacket != NULL) {
    Ip6ConfigParseDhcpReply (Dhcp6, Instance, Dhcp6ModeData.Ia->ReplyPacket);
  }

ON_EXIT:

  FreePool (Dhcp6ModeData.ClientId);
  FreePool (Dhcp6ModeData.Ia);
}

/**
  The event process routine when the DHCPv6 server is answered with a reply packet
  for an information request.

  @param[in]     This          Points to the EFI_DHCP6_PROTOCOL.
  @param[in]     Context       The pointer to the IP6 configuration instance data.
  @param[in]     Packet        The DHCPv6 reply packet.

  @retval EFI_SUCCESS      The DNS server address was retrieved from the reply packet.
  @retval EFI_NOT_READY    The reply packet does not contain the DNS server option, or
                           the DNS server address is not valid.

**/
EFI_STATUS
EFIAPI
Ip6ConfigOnDhcp6Reply (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN VOID                *Context,
  IN EFI_DHCP6_PACKET    *Packet
  )
{
  return Ip6ConfigParseDhcpReply (This, (IP6_CONFIG_INSTANCE *) Context, Packet);
}

/**
  The event process routine when the DHCPv6 service binding protocol is installed
  in the system.

  @param[in]     Event         Not used.
  @param[in]     Context       The pointer to the IP6 config instance data.

**/
VOID
EFIAPI
Ip6ConfigOnDhcp6SbInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP6_CONFIG_INSTANCE  *Instance;

  Instance = (IP6_CONFIG_INSTANCE *) Context;

  if ((Instance->Dhcp6Handle != NULL) || (Instance->Policy != Ip6ConfigPolicyAutomatic)) {
    //
    // The DHCP6 child is already created or the policy is no longer AUTOMATIC.
    //
    return ;
  }

  Ip6ConfigStartStatefulAutoConfig (Instance, Instance->OtherInfoOnly);
}

/**
  Set the configuration for the EFI IPv6 network stack running on the communication
  device this EFI IPv6 Configuration Protocol instance manages.

  This function is used to set the configuration data of type DataType for the EFI
  IPv6 network stack that is running on the communication device that this EFI IPv6
  Configuration Protocol instance manages.

  DataSize is used to calculate the count of structure instances in the Data for
  a DataType in which multiple structure instances are allowed.

  This function is always non-blocking. When setting some type of configuration data,
  an asynchronous process is invoked to check the correctness of the data, such as
  performing Duplicate Address Detection on the manually set local IPv6 addresses.
  EFI_NOT_READY is returned immediately to indicate that such an asynchronous process
  is invoked, and the process is not finished yet. The caller wanting to get the result
  of the asynchronous process is required to call RegisterDataNotify() to register an
  event on the specified configuration data. Once the event is signaled, the caller
  can call GetData() to obtain the configuration data and know the result.
  For other types of configuration data that do not require an asynchronous configuration
  process, the result of the operation is immediately returned.

  @param[in]     This           The pointer to the EFI_IP6_CONFIG_PROTOCOL instance.
  @param[in]     DataType       The type of data to set.
  @param[in]     DataSize       Size of the buffer pointed to by Data in bytes.
  @param[in]     Data           The data buffer to set. The type of the data buffer is
                                associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following are TRUE:
                                - This is NULL.
                                - One or more fields in Data and DataSizedo not match the
                                  requirement of the data type indicated by DataType.
  @retval EFI_WRITE_PROTECTED   The specified configuration data is read-only or the specified
                                configuration data cannot be set under the current policy.
  @retval EFI_ACCESS_DENIED     Another set operation on the specified configuration
                                data is already in process.
  @retval EFI_NOT_READY         An asynchronous process was invoked to set the specified
                                configuration data, and the process is not finished yet.
  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type
                                indicated by DataType.
  @retval EFI_UNSUPPORTED       This DataType is not supported.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_DEVICE_ERROR      An unexpected system error or network error occurred.

**/
EFI_STATUS
EFIAPI
EfiIp6ConfigSetData (
  IN EFI_IP6_CONFIG_PROTOCOL    *This,
  IN EFI_IP6_CONFIG_DATA_TYPE   DataType,
  IN UINTN                      DataSize,
  IN VOID                       *Data
  )
{
  EFI_TPL              OldTpl;
  EFI_STATUS           Status;
  IP6_CONFIG_INSTANCE  *Instance;
  IP6_SERVICE          *IpSb;

  if ((This == NULL) || (Data == NULL && DataSize != 0) || (Data != NULL && DataSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip6ConfigDataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }

  Instance = IP6_CONFIG_INSTANCE_FROM_PROTOCOL (This);
  IpSb     = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);
  NET_CHECK_SIGNATURE (IpSb, IP6_SERVICE_SIGNATURE);

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Status = Instance->DataItem[DataType].Status;
  if (Status != EFI_NOT_READY) {

    if (Instance->DataItem[DataType].SetData == NULL) {
      //
      // This type of data is readonly.
      //
      Status = EFI_WRITE_PROTECTED;
    } else {

      Status = Instance->DataItem[DataType].SetData (Instance, DataSize, Data);
      if (!EFI_ERROR (Status)) {
        //
        // Fire up the events registered with this type of data.
        //
        NetMapIterate (&Instance->DataItem[DataType].EventMap, Ip6ConfigSignalEvent, NULL);
        Ip6ConfigWriteConfigData (IpSb->MacString, Instance);
      } else if (Status == EFI_ABORTED) {
        //
        // The SetData is aborted because the data to set is the same with
        // the one maintained.
        //
        Status = EFI_SUCCESS;
        NetMapIterate (&Instance->DataItem[DataType].EventMap, Ip6ConfigSignalEvent, NULL);
      }
    }
  } else {
    //
    // Another asynchornous process is on the way.
    //
    Status = EFI_ACCESS_DENIED;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Get the configuration data for the EFI IPv6 network stack running on the communication
  device that this EFI IPv6 Configuration Protocol instance manages.

  This function returns the configuration data of type DataType for the EFI IPv6 network
  stack running on the communication device that this EFI IPv6 Configuration Protocol instance
  manages.

  The caller is responsible for allocating the buffer used to return the specified
  configuration data. The required size will be returned to the caller if the size of
  the buffer is too small.

  EFI_NOT_READY is returned if the specified configuration data is not ready due to an
  asynchronous configuration process already in progress. The caller can call RegisterDataNotify()
  to register an event on the specified configuration data. Once the asynchronous configuration
  process is finished, the event will be signaled, and a subsequent GetData() call will return
  the specified configuration data.

  @param[in]      This           Pointer to the EFI_IP6_CONFIG_PROTOCOL instance.
  @param[in]      DataType       The type of data to get.
  @param[in, out] DataSize       On input, in bytes, the size of Data. On output, in bytes, the
                                 size of buffer required to store the specified configuration data.
  @param[in]     Data            The data buffer in which the configuration data is returned. The
                                 type of the data buffer is associated with the DataType.
                                 This is an optional parameter that may be NULL.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER One or more of the followings are TRUE:
                                - This is NULL.
                                - DataSize is NULL.
                                - Data is NULL if *DataSize is not zero.
  @retval EFI_BUFFER_TOO_SMALL  The size of Data is too small for the specified configuration data,
                                and the required size is returned in DataSize.
  @retval EFI_NOT_READY         The specified configuration data is not ready due to an
                                asynchronous configuration process already in progress.
  @retval EFI_NOT_FOUND         The specified configuration data is not found.

**/
EFI_STATUS
EFIAPI
EfiIp6ConfigGetData (
  IN EFI_IP6_CONFIG_PROTOCOL    *This,
  IN EFI_IP6_CONFIG_DATA_TYPE   DataType,
  IN OUT UINTN                  *DataSize,
  IN VOID                       *Data   OPTIONAL
  )
{
  EFI_TPL               OldTpl;
  EFI_STATUS            Status;
  IP6_CONFIG_INSTANCE   *Instance;
  IP6_CONFIG_DATA_ITEM  *DataItem;

  if ((This == NULL) || (DataSize == NULL) || ((*DataSize != 0) && (Data == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip6ConfigDataTypeMaximum) {
    return EFI_NOT_FOUND;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = IP6_CONFIG_INSTANCE_FROM_PROTOCOL (This);
  DataItem = &Instance->DataItem[DataType];

  Status   = Instance->DataItem[DataType].Status;
  if (!EFI_ERROR (Status)) {

    if (DataItem->GetData != NULL) {

      Status = DataItem->GetData (Instance, DataSize, Data);
    } else if (*DataSize < Instance->DataItem[DataType].DataSize) {
      //
      // Update the buffer length.
      //
      *DataSize = Instance->DataItem[DataType].DataSize;
      Status    = EFI_BUFFER_TOO_SMALL;
    } else {

      *DataSize = Instance->DataItem[DataType].DataSize;
      CopyMem (Data, Instance->DataItem[DataType].Data.Ptr, *DataSize);
    }
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Register an event that is signaled whenever a configuration process on the specified
  configuration data is done.

  This function registers an event that is to be signaled whenever a configuration
  process on the specified configuration data is performed. An event can be registered
  for a different DataType simultaneously. The caller is responsible for determining
  which type of configuration data causes the signaling of the event in such an event.

  @param[in]     This           Pointer to the EFI_IP6_CONFIG_PROTOCOL instance.
  @param[in]     DataType       The type of data to unregister the event for.
  @param[in]     Event          The event to register.

  @retval EFI_SUCCESS           The notification event for the specified configuration data is
                                registered.
  @retval EFI_INVALID_PARAMETER This is NULL or Event is NULL.
  @retval EFI_UNSUPPORTED       The configuration data type specified by DataType is not
                                supported.
  @retval EFI_OUT_OF_RESOURCES  Required system resources could not be allocated.
  @retval EFI_ACCESS_DENIED     The Event is already registered for the DataType.

**/
EFI_STATUS
EFIAPI
EfiIp6ConfigRegisterDataNotify (
  IN EFI_IP6_CONFIG_PROTOCOL    *This,
  IN EFI_IP6_CONFIG_DATA_TYPE   DataType,
  IN EFI_EVENT                  Event
  )
{
  EFI_TPL              OldTpl;
  EFI_STATUS           Status;
  IP6_CONFIG_INSTANCE  *Instance;
  NET_MAP              *EventMap;
  NET_MAP_ITEM         *Item;

  if ((This == NULL) || (Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip6ConfigDataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }

  OldTpl    = gBS->RaiseTPL (TPL_CALLBACK);

  Instance  = IP6_CONFIG_INSTANCE_FROM_PROTOCOL (This);
  EventMap  = &Instance->DataItem[DataType].EventMap;

  //
  // Check whether this event is already registered for this DataType.
  //
  Item = NetMapFindKey (EventMap, Event);
  if (Item == NULL) {

    Status = NetMapInsertTail (EventMap, Event, NULL);

    if (EFI_ERROR (Status)) {

      Status = EFI_OUT_OF_RESOURCES;
    }

  } else {

    Status = EFI_ACCESS_DENIED;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Remove a previously registered event for the specified configuration data.

  @param  This                   The pointer to the EFI_IP6_CONFIG_PROTOCOL instance.
  @param  DataType               The type of data to remove from the previously
                                 registered event.
  @param  Event                  The event to be unregistered.

  @retval EFI_SUCCESS            The event registered for the specified
                                 configuration data was removed.
  @retval EFI_INVALID_PARAMETER  This is NULL or Event is NULL.
  @retval EFI_NOT_FOUND          The Event has not been registered for the
                                 specified DataType.

**/
EFI_STATUS
EFIAPI
EfiIp6ConfigUnregisterDataNotify (
  IN EFI_IP6_CONFIG_PROTOCOL    *This,
  IN EFI_IP6_CONFIG_DATA_TYPE   DataType,
  IN EFI_EVENT                  Event
  )
{
  EFI_TPL              OldTpl;
  EFI_STATUS           Status;
  IP6_CONFIG_INSTANCE  *Instance;
  NET_MAP_ITEM         *Item;

  if ((This == NULL) || (Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip6ConfigDataTypeMaximum) {
    return EFI_NOT_FOUND;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = IP6_CONFIG_INSTANCE_FROM_PROTOCOL (This);

  Item = NetMapFindKey (&Instance->DataItem[DataType].EventMap, Event);
  if (Item != NULL) {

    NetMapRemoveItem (&Instance->DataItem[DataType].EventMap, Item, NULL);
    Status = EFI_SUCCESS;
  } else {

    Status = EFI_NOT_FOUND;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Initialize an IP6_CONFIG_INSTANCE.

  @param[out]    Instance       The buffer of IP6_CONFIG_INSTANCE to be initialized.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.
  @retval EFI_SUCCESS           The IP6_CONFIG_INSTANCE initialized successfully.

**/
EFI_STATUS
Ip6ConfigInitInstance (
  OUT IP6_CONFIG_INSTANCE  *Instance
  )
{
  IP6_SERVICE           *IpSb;
  IP6_CONFIG_INSTANCE   *TmpInstance;
  LIST_ENTRY            *Entry;
  EFI_STATUS            Status;
  UINTN                 Index;
  UINT16                IfIndex;
  IP6_CONFIG_DATA_ITEM  *DataItem;

  IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);

  Instance->Signature = IP6_CONFIG_INSTANCE_SIGNATURE;

  //
  // Determine the index of this interface.
  //
  IfIndex = 0;
  NET_LIST_FOR_EACH (Entry, &mIp6ConfigInstanceList) {
    TmpInstance = NET_LIST_USER_STRUCT_S (Entry, IP6_CONFIG_INSTANCE, Link, IP6_CONFIG_INSTANCE_SIGNATURE);

    if (TmpInstance->IfIndex > IfIndex) {
      //
      // There is a sequence hole because some interface is down.
      //
      break;
    }

    IfIndex++;
  }

  Instance->IfIndex = IfIndex;
  NetListInsertBefore (Entry, &Instance->Link);

  for (Index = 0; Index < Ip6ConfigDataTypeMaximum; Index++) {
    //
    // Initialize the event map for each data item.
    //
    NetMapInit (&Instance->DataItem[Index].EventMap);
  }

  //
  // Initialize the NET_MAPs used for DAD on manually configured source addresses.
  //
  NetMapInit (&Instance->DadFailedMap);
  NetMapInit (&Instance->DadPassedMap);

  //
  // Initialize each data type: associate storage and set data size for the
  // fixed size data types, hook the SetData function, set the data attribute.
  //
  DataItem           = &Instance->DataItem[Ip6ConfigDataTypeInterfaceInfo];
  DataItem->GetData  = Ip6ConfigGetIfInfo;
  DataItem->Data.Ptr = &Instance->InterfaceInfo;
  DataItem->DataSize = sizeof (Instance->InterfaceInfo);
  SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED | DATA_ATTRIB_VOLATILE);
  Ip6ConfigInitIfInfo (IpSb, &Instance->InterfaceInfo);

  DataItem           = &Instance->DataItem[Ip6ConfigDataTypeAltInterfaceId];
  DataItem->SetData  = Ip6ConfigSetAltIfId;
  DataItem->Data.Ptr = &Instance->AltIfId;
  DataItem->DataSize = sizeof (Instance->AltIfId);
  DataItem->Status   = EFI_NOT_FOUND;
  SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED);

  DataItem           = &Instance->DataItem[Ip6ConfigDataTypePolicy];
  DataItem->SetData  = Ip6ConfigSetPolicy;
  DataItem->Data.Ptr = &Instance->Policy;
  DataItem->DataSize = sizeof (Instance->Policy);
  Instance->Policy   = Ip6ConfigPolicyManual;
  SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED);

  DataItem           = &Instance->DataItem[Ip6ConfigDataTypeDupAddrDetectTransmits];
  DataItem->SetData  = Ip6ConfigSetDadXmits;
  DataItem->Data.Ptr = &Instance->DadXmits;
  DataItem->DataSize = sizeof (Instance->DadXmits);
  Instance->DadXmits.DupAddrDetectTransmits = IP6_CONFIG_DEFAULT_DAD_XMITS;
  SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED);

  DataItem           = &Instance->DataItem[Ip6ConfigDataTypeManualAddress];
  DataItem->SetData  = Ip6ConfigSetManualAddress;
  DataItem->Status   = EFI_NOT_FOUND;

  DataItem           = &Instance->DataItem[Ip6ConfigDataTypeGateway];
  DataItem->SetData  = Ip6ConfigSetGateway;
  DataItem->Status   = EFI_NOT_FOUND;

  DataItem           = &Instance->DataItem[Ip6ConfigDataTypeDnsServer];
  DataItem->SetData  = Ip6ConfigSetDnsServer;
  DataItem->Status   = EFI_NOT_FOUND;

  //
  // Create the event used for DHCP.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ip6ConfigOnDhcp6Event,
                  Instance,
                  &Instance->Dhcp6Event
                  );
  ASSERT_EFI_ERROR (Status);

  Instance->Configured  = TRUE;

  //
  // Try to read the config data from NV variable.
  //
  Status = Ip6ConfigReadConfigData (IpSb->MacString, Instance);
  if (Status == EFI_NOT_FOUND) {
    //
    // The NV variable is not set, so generate a random IAID, and write down the
    // fresh new configuration as the NV variable now.
    //
    Instance->IaId = NET_RANDOM (NetRandomInitSeed ());

    for (Index = 0; Index < IpSb->SnpMode.HwAddressSize; Index++) {
      Instance->IaId |= (IpSb->SnpMode.CurrentAddress.Addr[Index] << ((Index << 3) & 31));
    }

    Ip6ConfigWriteConfigData (IpSb->MacString, Instance);
  } else if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance->Ip6Config.SetData              = EfiIp6ConfigSetData;
  Instance->Ip6Config.GetData              = EfiIp6ConfigGetData;
  Instance->Ip6Config.RegisterDataNotify   = EfiIp6ConfigRegisterDataNotify;
  Instance->Ip6Config.UnregisterDataNotify = EfiIp6ConfigUnregisterDataNotify;


  //
  // Publish the IP6 configuration form
  //
  return Ip6ConfigFormInit (Instance);
}

/**
  Release an IP6_CONFIG_INSTANCE.

  @param[in, out] Instance    The buffer of IP6_CONFIG_INSTANCE to be freed.

**/
VOID
Ip6ConfigCleanInstance (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  )
{
  UINTN                 Index;
  IP6_CONFIG_DATA_ITEM  *DataItem;

  if (Instance->DeclineAddress != NULL) {
    FreePool (Instance->DeclineAddress);
  }

  if (!Instance->Configured) {
    return ;
  }

  if (Instance->Dhcp6Handle != NULL) {

    Ip6ConfigDestroyDhcp6 (Instance);
  }

  //
  // Close the event.
  //
  if (Instance->Dhcp6Event != NULL) {
    gBS->CloseEvent (Instance->Dhcp6Event);
  }

  NetMapClean (&Instance->DadPassedMap);
  NetMapClean (&Instance->DadFailedMap);

  for (Index = 0; Index < Ip6ConfigDataTypeMaximum; Index++) {

    DataItem = &Instance->DataItem[Index];

    if (!DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED)) {
      if (DataItem->Data.Ptr != NULL) {
        FreePool (DataItem->Data.Ptr);
      }
      DataItem->Data.Ptr = NULL;
      DataItem->DataSize = 0;
    }

    NetMapClean (&Instance->DataItem[Index].EventMap);
  }

  Ip6ConfigFormUnload (Instance);

  RemoveEntryList (&Instance->Link);
}

/**
  Destroy the Dhcp6 child in IP6_CONFIG_INSTANCE and release the resources.

  @param[in, out] Instance    The buffer of IP6_CONFIG_INSTANCE to be freed.

  @retval EFI_SUCCESS         The child was successfully destroyed.
  @retval Others              Failed to destroy the child.

**/
EFI_STATUS
Ip6ConfigDestroyDhcp6 (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  )
{
  IP6_SERVICE                 *IpSb;
  EFI_STATUS                  Status;
  EFI_DHCP6_PROTOCOL          *Dhcp6;

  Dhcp6 = Instance->Dhcp6;
  ASSERT (Dhcp6 != NULL);

  Dhcp6->Stop (Dhcp6);
  Dhcp6->Configure (Dhcp6, NULL);
  Instance->Dhcp6 = NULL;

  IpSb = IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE (Instance);

  //
  // Close DHCPv6 protocol and destroy the child.
  //
  Status = gBS->CloseProtocol (
                  Instance->Dhcp6Handle,
                  &gEfiDhcp6ProtocolGuid,
                  IpSb->Image,
                  IpSb->Controller
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = NetLibDestroyServiceChild (
             IpSb->Controller,
             IpSb->Image,
             &gEfiDhcp6ServiceBindingProtocolGuid,
             Instance->Dhcp6Handle
             );

  Instance->Dhcp6Handle = NULL;

  return Status;
}

