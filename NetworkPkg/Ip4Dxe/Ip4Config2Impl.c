/** @file
  The implementation of EFI IPv4 Configuration II Protocol.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip4Impl.h"

LIST_ENTRY  mIp4Config2InstanceList = { &mIp4Config2InstanceList, &mIp4Config2InstanceList };

/**
  The event process routine when the DHCPv4 service binding protocol is installed
  in the system.

  @param[in]     Event         Not used.
  @param[in]     Context       Pointer to the IP4 config2 instance data.

**/
VOID
EFIAPI
Ip4Config2OnDhcp4SbInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Destroy the Dhcp4 child in IP4_CONFIG2_INSTANCE and release the resources.

  @param[in, out] Instance    The buffer of IP4 config2 instance to be freed.

  @retval EFI_SUCCESS         The child was successfully destroyed.
  @retval Others              Failed to destroy the child.

**/
EFI_STATUS
Ip4Config2DestroyDhcp4 (
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  )
{
  IP4_SERVICE         *IpSb;
  EFI_STATUS          Status;
  EFI_DHCP4_PROTOCOL  *Dhcp4;

  Dhcp4 = Instance->Dhcp4;
  ASSERT (Dhcp4 != NULL);

  Dhcp4->Stop (Dhcp4);
  Dhcp4->Configure (Dhcp4, NULL);
  Instance->Dhcp4 = NULL;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  //
  // Close DHCPv4 protocol and destroy the child.
  //
  Status = gBS->CloseProtocol (
                  Instance->Dhcp4Handle,
                  &gEfiDhcp4ProtocolGuid,
                  IpSb->Image,
                  IpSb->Controller
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = NetLibDestroyServiceChild (
             IpSb->Controller,
             IpSb->Image,
             &gEfiDhcp4ServiceBindingProtocolGuid,
             Instance->Dhcp4Handle
             );

  Instance->Dhcp4Handle = NULL;

  return Status;
}

/**
  Update the current policy to NewPolicy. During the transition
  period, the default router list
  and address list in all interfaces will be released.

  @param[in]  IpSb               The IP4 service binding instance.
  @param[in]  NewPolicy          The new policy to be updated to.

**/
VOID
Ip4Config2OnPolicyChanged (
  IN IP4_SERVICE             *IpSb,
  IN EFI_IP4_CONFIG2_POLICY  NewPolicy
  )
{
  IP4_INTERFACE    *IpIf;
  IP4_ROUTE_TABLE  *RouteTable;

  //
  // Currently there are only two policies: static and dhcp. Regardless of
  // what transition is going on, i.e., static -> dhcp and dhcp ->
  // static, we have to free default router table and all addresses.
  //

  if (IpSb->DefaultInterface != NULL) {
    if (IpSb->DefaultRouteTable != NULL) {
      Ip4FreeRouteTable (IpSb->DefaultRouteTable);
      IpSb->DefaultRouteTable = NULL;
    }

    Ip4CancelReceive (IpSb->DefaultInterface);

    Ip4FreeInterface (IpSb->DefaultInterface, NULL);
    IpSb->DefaultInterface = NULL;
  }

  Ip4CleanAssembleTable (&IpSb->Assemble);

  //
  // Create new default interface and route table.
  //
  IpIf = Ip4CreateInterface (IpSb->Mnp, IpSb->Controller, IpSb->Image);
  if (IpIf == NULL) {
    return;
  }

  RouteTable = Ip4CreateRouteTable ();
  if (RouteTable == NULL) {
    Ip4FreeInterface (IpIf, NULL);
    return;
  }

  IpSb->DefaultInterface = IpIf;
  InsertHeadList (&IpSb->Interfaces, &IpIf->Link);
  IpSb->DefaultRouteTable = RouteTable;
  Ip4ReceiveFrame (IpIf, NULL, Ip4AccpetFrame, IpSb);

  if ((IpSb->State == IP4_SERVICE_CONFIGED) || (IpSb->State == IP4_SERVICE_STARTED)) {
    IpSb->State = IP4_SERVICE_UNSTARTED;
  }

  //
  // Start the dhcp configuration.
  //
  if (NewPolicy == Ip4Config2PolicyDhcp) {
    Ip4StartAutoConfig (&IpSb->Ip4Config2Instance);
  }
}

/**
  Signal the registered event. It is the callback routine for NetMapIterate.

  @param[in]  Map    Points to the list of registered event.
  @param[in]  Item   The registered event.
  @param[in]  Arg    Not used.

  @retval EFI_SUCCESS           The event was signaled successfully.
**/
EFI_STATUS
EFIAPI
Ip4Config2SignalEvent (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg
  )
{
  gBS->SignalEvent ((EFI_EVENT)Item->Key);

  return EFI_SUCCESS;
}

/**
  Read the configuration data from variable storage according to the VarName and
  gEfiIp4Config2ProtocolGuid. It checks the integrity of variable data. If the
  data is corrupted, it clears the variable data to ZERO. Otherwise, it outputs the
  configuration data to IP4_CONFIG2_INSTANCE.

  @param[in]      VarName       The pointer to the variable name
  @param[in, out] Instance      The pointer to the IP4 config2 instance data.

  @retval EFI_NOT_FOUND         The variable can not be found or already corrupted.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_SUCCESS           The configuration data was retrieved successfully.

**/
EFI_STATUS
Ip4Config2ReadConfigData (
  IN     CHAR16                *VarName,
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  )
{
  EFI_STATUS               Status;
  UINTN                    VarSize;
  IP4_CONFIG2_VARIABLE     *Variable;
  IP4_CONFIG2_DATA_ITEM    *DataItem;
  UINTN                    Index;
  IP4_CONFIG2_DATA_RECORD  DataRecord;
  CHAR8                    *Data;

  //
  // Try to read the configuration variable.
  //
  VarSize = 0;
  Status  = gRT->GetVariable (
                   VarName,
                   &gEfiIp4Config2ProtocolGuid,
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
                    &gEfiIp4Config2ProtocolGuid,
                    NULL,
                    &VarSize,
                    Variable
                    );
    if (EFI_ERROR (Status) || ((UINT16)(~NetblockChecksum ((UINT8 *)Variable, (UINT32)VarSize)) != 0)) {
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
             &gEfiIp4Config2ProtocolGuid,
             IP4_CONFIG2_VARIABLE_ATTRIBUTE,
             0,
             NULL
             );

      return EFI_NOT_FOUND;
    }

    for (Index = 0; Index < Variable->DataRecordCount; Index++) {
      CopyMem (&DataRecord, &Variable->DataRecord[Index], sizeof (DataRecord));

      DataItem = &Instance->DataItem[DataRecord.DataType];
      if (DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED) &&
          (DataItem->DataSize != DataRecord.DataSize)
          )
      {
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

      Data = (CHAR8 *)Variable + DataRecord.Offset;
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
  Write the configuration data from IP4_CONFIG2_INSTANCE to variable storage.

  @param[in]      VarName       The pointer to the variable name.
  @param[in]      Instance      The pointer to the IP4 config2 instance data.

  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_SUCCESS           The configuration data is written successfully.

**/
EFI_STATUS
Ip4Config2WriteConfigData (
  IN CHAR16                *VarName,
  IN IP4_CONFIG2_INSTANCE  *Instance
  )
{
  UINTN                    Index;
  UINTN                    VarSize;
  IP4_CONFIG2_DATA_ITEM    *DataItem;
  IP4_CONFIG2_VARIABLE     *Variable;
  IP4_CONFIG2_DATA_RECORD  *DataRecord;
  CHAR8                    *Heap;
  EFI_STATUS               Status;

  VarSize = sizeof (IP4_CONFIG2_VARIABLE) - sizeof (IP4_CONFIG2_DATA_RECORD);

  for (Index = 0; Index < Ip4Config2DataTypeMaximum; Index++) {
    DataItem = &Instance->DataItem[Index];
    if (!DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_VOLATILE) && !EFI_ERROR (DataItem->Status)) {
      VarSize += sizeof (IP4_CONFIG2_DATA_RECORD) + DataItem->DataSize;
    }
  }

  Variable = AllocatePool (VarSize);
  if (Variable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Heap                      = (CHAR8 *)Variable + VarSize;
  Variable->DataRecordCount = 0;

  for (Index = 0; Index < Ip4Config2DataTypeMaximum; Index++) {
    DataItem = &Instance->DataItem[Index];
    if (!DATA_ATTRIB_SET (DataItem->Attribute, DATA_ATTRIB_VOLATILE) && !EFI_ERROR (DataItem->Status)) {
      Heap -= DataItem->DataSize;
      CopyMem (Heap, DataItem->Data.Ptr, DataItem->DataSize);

      DataRecord           = &Variable->DataRecord[Variable->DataRecordCount];
      DataRecord->DataType = (EFI_IP4_CONFIG2_DATA_TYPE)Index;
      DataRecord->DataSize = (UINT32)DataItem->DataSize;
      DataRecord->Offset   = (UINT16)(Heap - (CHAR8 *)Variable);

      Variable->DataRecordCount++;
    }
  }

  Variable->Checksum = 0;
  Variable->Checksum = (UINT16) ~NetblockChecksum ((UINT8 *)Variable, (UINT32)VarSize);

  Status = gRT->SetVariable (
                  VarName,
                  &gEfiIp4Config2ProtocolGuid,
                  IP4_CONFIG2_VARIABLE_ATTRIBUTE,
                  VarSize,
                  Variable
                  );

  FreePool (Variable);

  return Status;
}

/**
  Build a EFI_IP4_ROUTE_TABLE to be returned to the caller of GetModeData.
  The EFI_IP4_ROUTE_TABLE is clumsy to use in the internal operation of the
  IP4 driver.

  @param[in]   IpSb        The IP4 service binding instance.
  @param[out]  Table       The built IP4 route table.

  @retval EFI_SUCCESS           The route table is successfully build
  @retval EFI_NOT_FOUND         Failed to allocate the memory for the route table.

**/
EFI_STATUS
Ip4Config2BuildDefaultRouteTable (
  IN  IP4_SERVICE          *IpSb,
  OUT EFI_IP4_ROUTE_TABLE  *Table
  )
{
  LIST_ENTRY       *Entry;
  IP4_ROUTE_ENTRY  *RtEntry;
  UINT32           Count;
  INT32            Index;

  if (IpSb->DefaultRouteTable == NULL) {
    return EFI_NOT_FOUND;
  }

  Count = IpSb->DefaultRouteTable->TotalNum;

  if (Count == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Copy the route entry to EFI route table. Keep the order of
  // route entry copied from most specific to default route. That
  // is, interlevel the route entry from the instance's route area
  // and those from the default route table's route area.
  //
  Count = 0;

  for (Index = IP4_MASK_MAX; Index >= 0; Index--) {
    NET_LIST_FOR_EACH (Entry, &(IpSb->DefaultRouteTable->RouteArea[Index])) {
      RtEntry = NET_LIST_USER_STRUCT (Entry, IP4_ROUTE_ENTRY, Link);

      EFI_IP4 (Table[Count].SubnetAddress)  = HTONL (RtEntry->Dest & RtEntry->Netmask);
      EFI_IP4 (Table[Count].SubnetMask)     = HTONL (RtEntry->Netmask);
      EFI_IP4 (Table[Count].GatewayAddress) = HTONL (RtEntry->NextHop);

      Count++;
    }
  }

  return EFI_SUCCESS;
}

/**
  The event process routine when the DHCPv4 service binding protocol is installed
  in the system.

  @param[in]     Event         Not used.
  @param[in]     Context       The pointer to the IP4 config2 instance data.

**/
VOID
EFIAPI
Ip4Config2OnDhcp4SbInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP4_CONFIG2_INSTANCE  *Instance;

  Instance = (IP4_CONFIG2_INSTANCE *)Context;

  if ((Instance->Dhcp4Handle != NULL) || (Instance->Policy != Ip4Config2PolicyDhcp)) {
    //
    // The DHCP4 child is already created or the policy is no longer DHCP.
    //
    return;
  }

  Ip4StartAutoConfig (Instance);
}

/**
  Set the station address and subnetmask for the default interface.

  @param[in]  IpSb               The pointer to the IP4 service binding instance.
  @param[in]  StationAddress     Ip address to be set.
  @param[in]  SubnetMask         Subnet to be set.

  @retval EFI_SUCCESS   Set default address successful.
  @retval Others        Some errors occur in setting.

**/
EFI_STATUS
Ip4Config2SetDefaultAddr (
  IN IP4_SERVICE  *IpSb,
  IN IP4_ADDR     StationAddress,
  IN IP4_ADDR     SubnetMask
  )
{
  EFI_STATUS        Status;
  IP4_INTERFACE     *IpIf;
  IP4_PROTOCOL      *Ip4Instance;
  EFI_ARP_PROTOCOL  *Arp;
  LIST_ENTRY        *Entry;
  IP4_ADDR          Subnet;
  IP4_ROUTE_TABLE   *RouteTable;

  IpIf = IpSb->DefaultInterface;
  ASSERT (IpIf != NULL);

  if ((IpIf->Ip == StationAddress) && (IpIf->SubnetMask == SubnetMask)) {
    IpSb->State = IP4_SERVICE_CONFIGED;
    return EFI_SUCCESS;
  }

  if (IpSb->Reconfig) {
    //
    // The default address is changed, free the previous interface first.
    //
    if (IpSb->DefaultRouteTable != NULL) {
      Ip4FreeRouteTable (IpSb->DefaultRouteTable);
      IpSb->DefaultRouteTable = NULL;
    }

    Ip4CancelReceive (IpSb->DefaultInterface);
    Ip4FreeInterface (IpSb->DefaultInterface, NULL);
    IpSb->DefaultInterface = NULL;
    //
    // Create new default interface and route table.
    //
    IpIf = Ip4CreateInterface (IpSb->Mnp, IpSb->Controller, IpSb->Image);
    if (IpIf == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    RouteTable = Ip4CreateRouteTable ();
    if (RouteTable == NULL) {
      Ip4FreeInterface (IpIf, NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    IpSb->DefaultInterface = IpIf;
    InsertHeadList (&IpSb->Interfaces, &IpIf->Link);
    IpSb->DefaultRouteTable = RouteTable;
    Ip4ReceiveFrame (IpIf, NULL, Ip4AccpetFrame, IpSb);
  }

  if (IpSb->State == IP4_SERVICE_CONFIGED) {
    IpSb->State = IP4_SERVICE_UNSTARTED;
  }

  Status = Ip4SetAddress (IpIf, StationAddress, SubnetMask);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (IpIf->Arp != NULL) {
    //
    // A non-NULL IpIf->Arp here means a new ARP child is created when setting default address,
    // but some IP children may have referenced the default interface before it is configured,
    // these IP instances also consume this ARP protocol so they need to open it BY_CHILD_CONTROLLER.
    //
    Arp = NULL;
    NET_LIST_FOR_EACH (Entry, &IpIf->IpInstances) {
      Ip4Instance = NET_LIST_USER_STRUCT_S (Entry, IP4_PROTOCOL, AddrLink, IP4_PROTOCOL_SIGNATURE);
      Status      = gBS->OpenProtocol (
                           IpIf->ArpHandle,
                           &gEfiArpProtocolGuid,
                           (VOID **)&Arp,
                           gIp4DriverBinding.DriverBindingHandle,
                           Ip4Instance->Handle,
                           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                           );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // Add a route for the connected network.
  //
  Subnet = StationAddress & SubnetMask;

  Ip4AddRoute (
    IpSb->DefaultRouteTable,
    Subnet,
    SubnetMask,
    IP4_ALLZERO_ADDRESS
    );

  IpSb->State    = IP4_SERVICE_CONFIGED;
  IpSb->Reconfig = FALSE;

  return EFI_SUCCESS;
}

/**
  Set the station address, subnetmask and gateway address for the default interface.

  @param[in]  Instance         The pointer to the IP4 config2 instance data.
  @param[in]  StationAddress   Ip address to be set.
  @param[in]  SubnetMask       Subnet to be set.
  @param[in]  GatewayAddress   Gateway to be set.

  @retval EFI_SUCCESS     Set default If successful.
  @retval Others          Errors occur as indicated.

**/
EFI_STATUS
Ip4Config2SetDefaultIf (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN IP4_ADDR              StationAddress,
  IN IP4_ADDR              SubnetMask,
  IN IP4_ADDR              GatewayAddress
  )
{
  EFI_STATUS   Status;
  IP4_SERVICE  *IpSb;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  //
  // Check whether the StationAddress/SubnetMask pair is valid.
  //
  if (!Ip4StationAddressValid (StationAddress, SubnetMask)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = Ip4Config2SetDefaultAddr (IpSb, StationAddress, SubnetMask);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create a route if there is a default router.
  //
  if (GatewayAddress != IP4_ALLZERO_ADDRESS) {
    Ip4AddRoute (
      IpSb->DefaultRouteTable,
      IP4_ALLZERO_ADDRESS,
      IP4_ALLZERO_ADDRESS,
      GatewayAddress
      );
  }

  return EFI_SUCCESS;
}

/**
  Release all the DHCP related resources.

  @param  Instance              The IP4 config2 instance.

  @return None

**/
VOID
Ip4Config2CleanDhcp4 (
  IN IP4_CONFIG2_INSTANCE  *Instance
  )
{
  IP4_SERVICE  *IpSb;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  if (Instance->Dhcp4 != NULL) {
    Instance->Dhcp4->Stop (Instance->Dhcp4);

    gBS->CloseProtocol (
           Instance->Dhcp4Handle,
           &gEfiDhcp4ProtocolGuid,
           IpSb->Image,
           IpSb->Controller
           );

    Instance->Dhcp4 = NULL;
  }

  if (Instance->Dhcp4Handle != NULL) {
    NetLibDestroyServiceChild (
      IpSb->Controller,
      IpSb->Image,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Instance->Dhcp4Handle
      );

    Instance->Dhcp4Handle = NULL;
  }

  if (Instance->Dhcp4Event != NULL) {
    gBS->CloseEvent (Instance->Dhcp4Event);
    Instance->Dhcp4Event = NULL;
  }
}

/**
  This worker function sets the DNS server list for the EFI IPv4 network
  stack running on the communication device that this EFI_IP4_CONFIG2_PROTOCOL
  manages. The DNS server addresses must be unicast IPv4 addresses.

  @param[in]     Instance        The pointer to the IP4 config2 instance data.
  @param[in]     DataSize        The size of the buffer pointed to by Data in bytes.
  @param[in]     Data            The data buffer to set, points to an array of
                                 EFI_IPv4_ADDRESS instances.

  @retval EFI_BAD_BUFFER_SIZE    The DataSize does not match the size of the type.
  @retval EFI_INVALID_PARAMETER  One or more fields in Data is invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources to complete the operation.
  @retval EFI_ABORTED            The DNS server addresses to be set equal the current
                                 configuration.
  @retval EFI_SUCCESS            The specified configuration data for the EFI IPv4
                                 network stack was set.

**/
EFI_STATUS
Ip4Config2SetDnsServerWorker (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN UINTN                 DataSize,
  IN VOID                  *Data
  )
{
  UINTN                  OldIndex;
  UINTN                  NewIndex;
  EFI_IPv4_ADDRESS       *OldDns;
  EFI_IPv4_ADDRESS       *NewDns;
  UINTN                  OldDnsCount;
  UINTN                  NewDnsCount;
  IP4_CONFIG2_DATA_ITEM  *Item;
  BOOLEAN                OneAdded;
  VOID                   *Tmp;
  IP4_ADDR               DnsAddress;

  if ((DataSize % sizeof (EFI_IPv4_ADDRESS) != 0) || (DataSize == 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Item        = &Instance->DataItem[Ip4Config2DataTypeDnsServer];
  NewDns      = (EFI_IPv4_ADDRESS *)Data;
  OldDns      = Item->Data.DnsServers;
  NewDnsCount = DataSize / sizeof (EFI_IPv4_ADDRESS);
  OldDnsCount = Item->DataSize / sizeof (EFI_IPv4_ADDRESS);
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
    CopyMem (&DnsAddress, NewDns + NewIndex, sizeof (IP4_ADDR));
    if (IP4_IS_UNSPECIFIED (NTOHL (DnsAddress)) || IP4_IS_LOCAL_BROADCAST (NTOHL (DnsAddress))) {
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
      if (EFI_IP4_EQUAL (NewDns + NewIndex, OldDns + OldIndex)) {
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
    return EFI_SUCCESS;
  }
}

/**
  Callback function when DHCP process finished. It will save the
  retrieved IP configure parameter from DHCP to the NVRam.

  @param  Event                  The callback event
  @param  Context                Opaque context to the callback

  @return None

**/
VOID
EFIAPI
Ip4Config2OnDhcp4Complete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  IP4_CONFIG2_INSTANCE     *Instance;
  EFI_DHCP4_MODE_DATA      Dhcp4Mode;
  EFI_STATUS               Status;
  IP4_ADDR                 StationAddress;
  IP4_ADDR                 SubnetMask;
  IP4_ADDR                 GatewayAddress;
  UINT32                   Index;
  UINT32                   OptionCount;
  EFI_DHCP4_PACKET_OPTION  **OptionList;

  Instance = (IP4_CONFIG2_INSTANCE *)Context;
  ASSERT (Instance->Dhcp4 != NULL);

  //
  // Get the DHCP retrieved parameters
  //
  Status = Instance->Dhcp4->GetModeData (Instance->Dhcp4, &Dhcp4Mode);

  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if (Dhcp4Mode.State == Dhcp4Bound) {
    StationAddress = EFI_NTOHL (Dhcp4Mode.ClientAddress);
    SubnetMask     = EFI_NTOHL (Dhcp4Mode.SubnetMask);
    GatewayAddress = EFI_NTOHL (Dhcp4Mode.RouterAddress);

    Status = Ip4Config2SetDefaultIf (Instance, StationAddress, SubnetMask, GatewayAddress);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Parse the ACK to get required DNS server information.
    //
    OptionCount = 0;
    OptionList  = NULL;

    Status = Instance->Dhcp4->Parse (Instance->Dhcp4, Dhcp4Mode.ReplyPacket, &OptionCount, OptionList);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      goto Exit;
    }

    OptionList = AllocateZeroPool (OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));
    if (OptionList == NULL) {
      goto Exit;
    }

    Status = Instance->Dhcp4->Parse (Instance->Dhcp4, Dhcp4Mode.ReplyPacket, &OptionCount, OptionList);
    if (EFI_ERROR (Status)) {
      FreePool (OptionList);
      goto Exit;
    }

    for (Index = 0; Index < OptionCount; Index++) {
      //
      // Look for DNS Server opcode (6).
      //
      if (OptionList[Index]->OpCode == DHCP4_TAG_DNS_SERVER) {
        if (((OptionList[Index]->Length & 0x3) != 0) || (OptionList[Index]->Length == 0)) {
          break;
        }

        Ip4Config2SetDnsServerWorker (Instance, OptionList[Index]->Length, &OptionList[Index]->Data[0]);
        break;
      }
    }

    FreePool (OptionList);

    Instance->DhcpSuccess = TRUE;
  }

Exit:
  Ip4Config2CleanDhcp4 (Instance);
  DispatchDpc ();
}

/**
  Start the DHCP configuration for this IP service instance.
  It will locates the EFI_IP4_CONFIG2_PROTOCOL, then start the
  DHCP configuration.

  @param[in]  Instance           The IP4 config2 instance to configure

  @retval EFI_SUCCESS            The auto configuration is successfully started
  @retval Others                 Failed to start auto configuration.

**/
EFI_STATUS
Ip4StartAutoConfig (
  IN IP4_CONFIG2_INSTANCE  *Instance
  )
{
  IP4_SERVICE               *IpSb;
  EFI_DHCP4_PROTOCOL        *Dhcp4;
  EFI_DHCP4_MODE_DATA       Dhcp4Mode;
  EFI_DHCP4_PACKET_OPTION   *OptionList[1];
  IP4_CONFIG2_DHCP4_OPTION  ParaList;
  EFI_STATUS                Status;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  if (IpSb->State > IP4_SERVICE_UNSTARTED) {
    return EFI_SUCCESS;
  }

  //
  // A host must not invoke DHCP configuration if it is already
  // participating in the DHCP configuration process.
  //
  if (Instance->Dhcp4Handle != NULL) {
    return EFI_SUCCESS;
  }

  Status = NetLibCreateServiceChild (
             IpSb->Controller,
             IpSb->Image,
             &gEfiDhcp4ServiceBindingProtocolGuid,
             &Instance->Dhcp4Handle
             );

  if (Status == EFI_UNSUPPORTED) {
    //
    // No DHCPv4 Service Binding protocol, register a notify.
    //
    if (Instance->Dhcp4SbNotifyEvent == NULL) {
      Instance->Dhcp4SbNotifyEvent = EfiCreateProtocolNotifyEvent (
                                       &gEfiDhcp4ServiceBindingProtocolGuid,
                                       TPL_CALLBACK,
                                       Ip4Config2OnDhcp4SbInstalled,
                                       (VOID *)Instance,
                                       &Instance->Registration
                                       );
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Instance->Dhcp4SbNotifyEvent != NULL) {
    gBS->CloseEvent (Instance->Dhcp4SbNotifyEvent);
  }

  Status = gBS->OpenProtocol (
                  Instance->Dhcp4Handle,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **)&Instance->Dhcp4,
                  IpSb->Image,
                  IpSb->Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    NetLibDestroyServiceChild (
      IpSb->Controller,
      IpSb->Image,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Instance->Dhcp4Handle
      );

    Instance->Dhcp4Handle = NULL;

    return Status;
  }

  //
  // Check the current DHCP status, if the DHCP process has
  // already finished, return now.
  //
  Dhcp4  = Instance->Dhcp4;
  Status = Dhcp4->GetModeData (Dhcp4, &Dhcp4Mode);
  if (Dhcp4Mode.State == Dhcp4Bound) {
    Ip4Config2OnDhcp4Complete (NULL, Instance);

    return EFI_SUCCESS;
  }

  //
  // Try to start the DHCP process. Use most of the current
  // DHCP configuration to avoid problems if some DHCP client
  // yields the control of this DHCP service to us.
  //
  ParaList.Head.OpCode             = DHCP4_TAG_PARA_LIST;
  ParaList.Head.Length             = 3;
  ParaList.Head.Data[0]            = DHCP4_TAG_NETMASK;
  ParaList.Route                   = DHCP4_TAG_ROUTER;
  ParaList.Dns                     = DHCP4_TAG_DNS_SERVER;
  OptionList[0]                    = &ParaList.Head;
  Dhcp4Mode.ConfigData.OptionCount = 1;
  Dhcp4Mode.ConfigData.OptionList  = OptionList;

  Status = Dhcp4->Configure (Dhcp4, &Dhcp4Mode.ConfigData);
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Instance->Dhcp4Handle,
           &gEfiDhcp4ProtocolGuid,
           IpSb->Image,
           IpSb->Controller
           );

    NetLibDestroyServiceChild (
      IpSb->Controller,
      IpSb->Image,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Instance->Dhcp4Handle
      );

    Instance->Dhcp4 = NULL;

    Instance->Dhcp4Handle = NULL;

    return Status;
  }

  //
  // Start the DHCP process
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Ip4Config2OnDhcp4Complete,
                  Instance,
                  &Instance->Dhcp4Event
                  );
  if (EFI_ERROR (Status)) {
    Ip4Config2DestroyDhcp4 (Instance);
    return Status;
  }

  Status = Dhcp4->Start (Dhcp4, Instance->Dhcp4Event);
  if (EFI_ERROR (Status)) {
    Ip4Config2DestroyDhcp4 (Instance);
    gBS->CloseEvent (Instance->Dhcp4Event);
    Instance->Dhcp4Event = NULL;

    return Status;
  }

  IpSb->State = IP4_SERVICE_STARTED;
  DispatchDpc ();

  return EFI_SUCCESS;
}

/**
  The work function is to get the interface information of the communication
  device this IP4_CONFIG2_INSTANCE manages.

  @param[in]      Instance Pointer to the IP4 config2 instance data.
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
Ip4Config2GetIfInfo (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN OUT UINTN             *DataSize,
  IN VOID                  *Data      OPTIONAL
  )
{
  IP4_SERVICE                     *IpSb;
  UINTN                           Length;
  IP4_CONFIG2_DATA_ITEM           *Item;
  EFI_IP4_CONFIG2_INTERFACE_INFO  *IfInfo;
  IP4_ADDR                        Address;

  IpSb   = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);
  Length = sizeof (EFI_IP4_CONFIG2_INTERFACE_INFO);

  if (IpSb->DefaultRouteTable != NULL) {
    Length += IpSb->DefaultRouteTable->TotalNum * sizeof (EFI_IP4_ROUTE_TABLE);
  }

  if (*DataSize < Length) {
    *DataSize = Length;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Copy the fixed size part of the interface info.
  //
  Item   = &Instance->DataItem[Ip4Config2DataTypeInterfaceInfo];
  IfInfo = (EFI_IP4_CONFIG2_INTERFACE_INFO *)Data;
  CopyMem (IfInfo, Item->Data.Ptr, sizeof (EFI_IP4_CONFIG2_INTERFACE_INFO));

  //
  // Update the address info.
  //
  if (IpSb->DefaultInterface != NULL) {
    Address = HTONL (IpSb->DefaultInterface->Ip);
    CopyMem (&IfInfo->StationAddress, &Address, sizeof (EFI_IPv4_ADDRESS));
    Address = HTONL (IpSb->DefaultInterface->SubnetMask);
    CopyMem (&IfInfo->SubnetMask, &Address, sizeof (EFI_IPv4_ADDRESS));
  }

  if (IpSb->DefaultRouteTable != NULL) {
    IfInfo->RouteTableSize = IpSb->DefaultRouteTable->TotalNum;
    IfInfo->RouteTable     = (EFI_IP4_ROUTE_TABLE *)((UINT8 *)Data + sizeof (EFI_IP4_CONFIG2_INTERFACE_INFO));

    Ip4Config2BuildDefaultRouteTable (IpSb, IfInfo->RouteTable);
  }

  return EFI_SUCCESS;
}

/**
  The work function is to set the general configuration policy for the EFI IPv4 network
  stack that is running on the communication device managed by this IP4_CONFIG2_INSTANCE.
  The policy will affect other configuration settings.

  @param[in]     Instance Pointer to the IP4 config2 instance data.
  @param[in]     DataSize Size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set.

  @retval EFI_INVALID_PARAMETER The to be set policy is invalid.
  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_ABORTED           The new policy equals the current policy.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set.

**/
EFI_STATUS
Ip4Config2SetPolicy (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN UINTN                 DataSize,
  IN VOID                  *Data
  )
{
  EFI_IP4_CONFIG2_POLICY  NewPolicy;
  IP4_CONFIG2_DATA_ITEM   *DataItem;
  IP4_SERVICE             *IpSb;

  if (DataSize != sizeof (EFI_IP4_CONFIG2_POLICY)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NewPolicy = *((EFI_IP4_CONFIG2_POLICY *)Data);

  if (NewPolicy >= Ip4Config2PolicyMax) {
    return EFI_INVALID_PARAMETER;
  }

  if (NewPolicy == Instance->Policy) {
    if ((NewPolicy != Ip4Config2PolicyDhcp) || Instance->DhcpSuccess) {
      return EFI_ABORTED;
    }
  } else {
    //
    // The policy is changed. Clean the ManualAddress, Gateway and DnsServers,
    // shrink the variable data size, and fire up all the related events.
    //
    DataItem = &Instance->DataItem[Ip4Config2DataTypeManualAddress];
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }

    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
    NetMapIterate (&DataItem->EventMap, Ip4Config2SignalEvent, NULL);

    DataItem = &Instance->DataItem[Ip4Config2DataTypeGateway];
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }

    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
    NetMapIterate (&DataItem->EventMap, Ip4Config2SignalEvent, NULL);

    DataItem = &Instance->DataItem[Ip4Config2DataTypeDnsServer];
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }

    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
    NetMapIterate (&DataItem->EventMap, Ip4Config2SignalEvent, NULL);

    if (NewPolicy == Ip4Config2PolicyDhcp) {
      SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_VOLATILE);
    } else {
      //
      // The policy is changed from dhcp to static. Stop the DHCPv4 process
      // and destroy the DHCPv4 child.
      //
      if (Instance->Dhcp4Handle != NULL) {
        Ip4Config2DestroyDhcp4 (Instance);
      }

      //
      // Close the event.
      //
      if (Instance->Dhcp4Event != NULL) {
        gBS->CloseEvent (Instance->Dhcp4Event);
        Instance->Dhcp4Event = NULL;
      }
    }
  }

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);
  Ip4Config2OnPolicyChanged (IpSb, NewPolicy);

  Instance->Policy = NewPolicy;

  return EFI_SUCCESS;
}

/**
  The work function is to set the station addresses manually for the EFI IPv4
  network stack. It is only configurable when the policy is Ip4Config2PolicyStatic.

  @param[in]     Instance Pointer to the IP4 config2 instance data.
  @param[in]     DataSize Size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set.

  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_WRITE_PROTECTED   The specified configuration data cannot be set
                                under the current policy.
  @retval EFI_INVALID_PARAMETER One or more fields in Data is invalid.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_NOT_READY         An asynchronous process is invoked to set the specified
                                configuration data, and the process is not finished.
  @retval EFI_ABORTED           The manual addresses to be set equal current
                                configuration.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set.

**/
EFI_STATUS
Ip4Config2SetManualAddress (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN UINTN                 DataSize,
  IN VOID                  *Data
  )
{
  EFI_IP4_CONFIG2_MANUAL_ADDRESS  NewAddress;
  IP4_CONFIG2_DATA_ITEM           *DataItem;
  EFI_STATUS                      Status;
  IP4_ADDR                        StationAddress;
  IP4_ADDR                        SubnetMask;
  VOID                            *Ptr;
  IP4_SERVICE                     *IpSb;
  IP4_INTERFACE                   *IpIf;
  IP4_ROUTE_TABLE                 *RouteTable;

  DataItem   = NULL;
  Status     = EFI_SUCCESS;
  Ptr        = NULL;
  IpIf       = NULL;
  RouteTable = NULL;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  ASSERT (Instance->DataItem[Ip4Config2DataTypeManualAddress].Status != EFI_NOT_READY);

  if ((DataSize != 0) && ((DataSize % sizeof (EFI_IP4_CONFIG2_MANUAL_ADDRESS)) != 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Instance->Policy != Ip4Config2PolicyStatic) {
    return EFI_WRITE_PROTECTED;
  }

  DataItem = &Instance->DataItem[Ip4Config2DataTypeManualAddress];

  if ((Data != NULL) && (DataSize != 0)) {
    NewAddress = *((EFI_IP4_CONFIG2_MANUAL_ADDRESS *)Data);

    StationAddress = EFI_NTOHL (NewAddress.Address);
    SubnetMask     = EFI_NTOHL (NewAddress.SubnetMask);

    //
    // Check whether the StationAddress/SubnetMask pair is valid.
    //
    if (!Ip4StationAddressValid (StationAddress, SubnetMask)) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Store the new data, and init the DataItem status to EFI_NOT_READY because
    // we may have an asynchronous configuration process.
    //
    Ptr = AllocateCopyPool (DataSize, Data);
    if (Ptr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }

    DataItem->Data.Ptr = Ptr;
    DataItem->DataSize = DataSize;
    DataItem->Status   = EFI_NOT_READY;

    IpSb->Reconfig = TRUE;
    Status         = Ip4Config2SetDefaultAddr (IpSb, StationAddress, SubnetMask);

    DataItem->Status = Status;

    if (EFI_ERROR (DataItem->Status) && (DataItem->Status != EFI_NOT_READY)) {
      if (Ptr != NULL) {
        FreePool (Ptr);
      }

      DataItem->Data.Ptr = NULL;
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

    //
    // Free the default router table and Interface, clean up the assemble table.
    //
    if (IpSb->DefaultInterface != NULL) {
      if (IpSb->DefaultRouteTable != NULL) {
        Ip4FreeRouteTable (IpSb->DefaultRouteTable);
        IpSb->DefaultRouteTable = NULL;
      }

      Ip4CancelReceive (IpSb->DefaultInterface);

      Ip4FreeInterface (IpSb->DefaultInterface, NULL);
      IpSb->DefaultInterface = NULL;
    }

    Ip4CleanAssembleTable (&IpSb->Assemble);

    //
    // Create new default interface and route table.
    //
    IpIf = Ip4CreateInterface (IpSb->Mnp, IpSb->Controller, IpSb->Image);
    if (IpIf == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    RouteTable = Ip4CreateRouteTable ();
    if (RouteTable == NULL) {
      Ip4FreeInterface (IpIf, NULL);
      return EFI_OUT_OF_RESOURCES;
    }

    IpSb->DefaultInterface = IpIf;
    InsertHeadList (&IpSb->Interfaces, &IpIf->Link);
    IpSb->DefaultRouteTable = RouteTable;
    Ip4ReceiveFrame (IpIf, NULL, Ip4AccpetFrame, IpSb);

    //
    // Reset the State to unstarted.
    //
    if ((IpSb->State == IP4_SERVICE_CONFIGED) || (IpSb->State == IP4_SERVICE_STARTED)) {
      IpSb->State = IP4_SERVICE_UNSTARTED;
    }
  }

  return Status;
}

/**
  The work function is to set the gateway addresses manually for the EFI IPv4
  network stack that is running on the communication device that this EFI IPv4
  Configuration Protocol manages. It is not configurable when the policy is
  Ip4Config2PolicyDhcp. The gateway addresses must be unicast IPv4 addresses.

  @param[in]     Instance The pointer to the IP4 config2 instance data.
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
Ip4Config2SetGateway (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN UINTN                 DataSize,
  IN VOID                  *Data
  )
{
  IP4_SERVICE            *IpSb;
  IP4_CONFIG2_DATA_ITEM  *DataItem;
  IP4_ADDR               Gateway;

  UINTN             Index1;
  UINTN             Index2;
  EFI_IPv4_ADDRESS  *OldGateway;
  EFI_IPv4_ADDRESS  *NewGateway;
  UINTN             OldGatewayCount;
  UINTN             NewGatewayCount;
  BOOLEAN           OneRemoved;
  BOOLEAN           OneAdded;
  VOID              *Tmp;

  OldGateway = NULL;
  NewGateway = NULL;
  OneRemoved = FALSE;
  OneAdded   = FALSE;
  Tmp        = NULL;

  if ((DataSize != 0) && (DataSize % sizeof (EFI_IPv4_ADDRESS) != 0)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Instance->Policy != Ip4Config2PolicyStatic) {
    return EFI_WRITE_PROTECTED;
  }

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  DataItem        = &Instance->DataItem[Ip4Config2DataTypeGateway];
  OldGateway      = DataItem->Data.Gateway;
  OldGatewayCount = DataItem->DataSize / sizeof (EFI_IPv4_ADDRESS);

  for (Index1 = 0; Index1 < OldGatewayCount; Index1++) {
    //
    // Remove the old route entry.
    //
    CopyMem (&Gateway, OldGateway + Index1, sizeof (IP4_ADDR));
    Ip4DelRoute (
      IpSb->DefaultRouteTable,
      IP4_ALLZERO_ADDRESS,
      IP4_ALLZERO_ADDRESS,
      NTOHL (Gateway)
      );
    OneRemoved = TRUE;
  }

  if ((Data != NULL) && (DataSize != 0)) {
    NewGateway      = (EFI_IPv4_ADDRESS *)Data;
    NewGatewayCount = DataSize / sizeof (EFI_IPv4_ADDRESS);
    for (Index1 = 0; Index1 < NewGatewayCount; Index1++) {
      CopyMem (&Gateway, NewGateway + Index1, sizeof (IP4_ADDR));

      if ((IpSb->DefaultInterface->SubnetMask != 0) &&
          !NetIp4IsUnicast (NTOHL (Gateway), IpSb->DefaultInterface->SubnetMask))
      {
        return EFI_INVALID_PARAMETER;
      }

      for (Index2 = Index1 + 1; Index2 < NewGatewayCount; Index2++) {
        if (EFI_IP4_EQUAL (NewGateway + Index1, NewGateway + Index2)) {
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
      //
      // Add the new route entry.
      //
      CopyMem (&Gateway, NewGateway + Index1, sizeof (IP4_ADDR));
      Ip4AddRoute (
        IpSb->DefaultRouteTable,
        IP4_ALLZERO_ADDRESS,
        IP4_ALLZERO_ADDRESS,
        NTOHL (Gateway)
        );

      OneAdded = TRUE;
    }

    if (!OneRemoved && !OneAdded) {
      DataItem->Status = EFI_SUCCESS;
      return EFI_ABORTED;
    } else {
      if (Tmp != NULL) {
        if (DataItem->Data.Ptr != NULL) {
          FreePool (DataItem->Data.Ptr);
        }

        DataItem->Data.Ptr = Tmp;
      }

      CopyMem (DataItem->Data.Ptr, Data, DataSize);
      DataItem->DataSize = DataSize;
      DataItem->Status   = EFI_SUCCESS;
    }
  } else {
    //
    // DataSize is 0 and Data is NULL, clean up the Gateway address.
    //
    if (DataItem->Data.Ptr != NULL) {
      FreePool (DataItem->Data.Ptr);
    }

    DataItem->Data.Ptr = NULL;
    DataItem->DataSize = 0;
    DataItem->Status   = EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  The work function is to set the DNS server list for the EFI IPv4 network
  stack running on the communication device that this EFI_IP4_CONFIG2_PROTOCOL
  manages. It is not configurable when the policy is Ip4Config2PolicyDhcp.
  The DNS server addresses must be unicast IPv4 addresses.

  @param[in]     Instance The pointer to the IP4 config2 instance data.
  @param[in]     DataSize The size of the buffer pointed to by Data in bytes.
  @param[in]     Data     The data buffer to set, points to an array of
                          EFI_IPv4_ADDRESS instances.

  @retval EFI_BAD_BUFFER_SIZE   The DataSize does not match the size of the type.
  @retval EFI_WRITE_PROTECTED   The specified configuration data cannot be set
                                under the current policy.
  @retval EFI_INVALID_PARAMETER One or more fields in Data is invalid.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.
  @retval EFI_ABORTED           The DNS server addresses to be set equal the current
                                configuration.
  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv4
                                network stack was set.

**/
EFI_STATUS
Ip4Config2SetDnsServer (
  IN IP4_CONFIG2_INSTANCE  *Instance,
  IN UINTN                 DataSize,
  IN VOID                  *Data
  )
{
  EFI_STATUS             Status;
  IP4_CONFIG2_DATA_ITEM  *Item;

  Status = EFI_SUCCESS;
  Item   = NULL;

  if (Instance->Policy != Ip4Config2PolicyStatic) {
    return EFI_WRITE_PROTECTED;
  }

  Item = &Instance->DataItem[Ip4Config2DataTypeDnsServer];

  if (DATA_ATTRIB_SET (Item->Attribute, DATA_ATTRIB_VOLATILE)) {
    REMOVE_DATA_ATTRIB (Item->Attribute, DATA_ATTRIB_VOLATILE);
  }

  if ((Data != NULL) && (DataSize != 0)) {
    Status = Ip4Config2SetDnsServerWorker (Instance, DataSize, Data);
  } else {
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

  return Status;
}

/**
  Generate the operational state of the interface this IP4 config2 instance manages
  and output in EFI_IP4_CONFIG2_INTERFACE_INFO.

  @param[in]      IpSb     The pointer to the IP4 service binding instance.
  @param[out]     IfInfo   The pointer to the IP4 config2 interface information structure.

**/
VOID
Ip4Config2InitIfInfo (
  IN  IP4_SERVICE                     *IpSb,
  OUT EFI_IP4_CONFIG2_INTERFACE_INFO  *IfInfo
  )
{
  UnicodeSPrint (
    IfInfo->Name,
    EFI_IP4_CONFIG2_INTERFACE_INFO_NAME_SIZE,
    L"eth%d",
    IpSb->Ip4Config2Instance.IfIndex
    );

  IfInfo->IfType        = IpSb->SnpMode.IfType;
  IfInfo->HwAddressSize = IpSb->SnpMode.HwAddressSize;
  CopyMem (&IfInfo->HwAddress, &IpSb->SnpMode.CurrentAddress, IfInfo->HwAddressSize);
}

/**
  Set the configuration for the EFI IPv4 network stack running on the communication
  device this EFI_IP4_CONFIG2_PROTOCOL instance manages.

  This function is used to set the configuration data of type DataType for the EFI
  IPv4 network stack that is running on the communication device that this EFI IPv4
  Configuration Protocol instance manages.

  DataSize is used to calculate the count of structure instances in the Data for
  a DataType in which multiple structure instances are allowed.

  This function is always non-blocking. When setting some type of configuration data,
  an asynchronous process is invoked to check the correctness of the data, such as
  performing Duplicate Address Detection on the manually set local IPv4 addresses.
  EFI_NOT_READY is returned immediately to indicate that such an asynchronous process
  is invoked, and the process is not finished yet. The caller wanting to get the result
  of the asynchronous process is required to call RegisterDataNotify() to register an
  event on the specified configuration data. Once the event is signaled, the caller
  can call GetData() to obtain the configuration data and know the result.
  For other types of configuration data that do not require an asynchronous configuration
  process, the result of the operation is immediately returned.

  @param[in]     This           The pointer to the EFI_IP4_CONFIG2_PROTOCOL instance.
  @param[in]     DataType       The type of data to set.
  @param[in]     DataSize       Size of the buffer pointed to by Data in bytes.
  @param[in]     Data           The data buffer to set. The type of the data buffer is
                                associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data for the EFI IPv6
                                network stack was set successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following are TRUE:
                                - This is NULL.
                                - One or more fields in Data and DataSize do not match the
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
EfiIp4Config2SetData (
  IN EFI_IP4_CONFIG2_PROTOCOL   *This,
  IN EFI_IP4_CONFIG2_DATA_TYPE  DataType,
  IN UINTN                      DataSize,
  IN VOID                       *Data
  )
{
  EFI_TPL               OldTpl;
  EFI_STATUS            Status;
  IP4_CONFIG2_INSTANCE  *Instance;
  IP4_SERVICE           *IpSb;

  if ((This == NULL) || ((Data == NULL) && (DataSize != 0)) || ((Data != NULL) && (DataSize == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip4Config2DataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }

  Instance = IP4_CONFIG2_INSTANCE_FROM_PROTOCOL (This);
  IpSb     = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);
  NET_CHECK_SIGNATURE (IpSb, IP4_SERVICE_SIGNATURE);

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
        NetMapIterate (&Instance->DataItem[DataType].EventMap, Ip4Config2SignalEvent, NULL);
        Ip4Config2WriteConfigData (IpSb->MacString, Instance);
      } else if (Status == EFI_ABORTED) {
        //
        // The SetData is aborted because the data to set is the same with
        // the one maintained.
        //
        Status = EFI_SUCCESS;
        NetMapIterate (&Instance->DataItem[DataType].EventMap, Ip4Config2SignalEvent, NULL);
      }
    }
  } else {
    //
    // Another asynchronous process is on the way.
    //
    Status = EFI_ACCESS_DENIED;
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Get the configuration data for the EFI IPv4 network stack running on the communication
  device that this EFI_IP4_CONFIG2_PROTOCOL instance manages.

  This function returns the configuration data of type DataType for the EFI IPv4 network
  stack running on the communication device that this EFI IPv4 Configuration Protocol instance
  manages.

  The caller is responsible for allocating the buffer used to return the specified
  configuration data. The required size will be returned to the caller if the size of
  the buffer is too small.

  EFI_NOT_READY is returned if the specified configuration data is not ready due to an
  asynchronous configuration process already in progress. The caller can call RegisterDataNotify()
  to register an event on the specified configuration data. Once the asynchronous configuration
  process is finished, the event will be signaled, and a subsequent GetData() call will return
  the specified configuration data.

  @param[in]      This           Pointer to the EFI_IP4_CONFIG2_PROTOCOL instance.
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
EfiIp4Config2GetData (
  IN EFI_IP4_CONFIG2_PROTOCOL   *This,
  IN EFI_IP4_CONFIG2_DATA_TYPE  DataType,
  IN OUT UINTN                  *DataSize,
  IN VOID                       *Data   OPTIONAL
  )
{
  EFI_TPL                OldTpl;
  EFI_STATUS             Status;
  IP4_CONFIG2_INSTANCE   *Instance;
  IP4_CONFIG2_DATA_ITEM  *DataItem;

  if ((This == NULL) || (DataSize == NULL) || ((*DataSize != 0) && (Data == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip4Config2DataTypeMaximum) {
    return EFI_NOT_FOUND;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = IP4_CONFIG2_INSTANCE_FROM_PROTOCOL (This);
  DataItem = &Instance->DataItem[DataType];

  Status = Instance->DataItem[DataType].Status;
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

  @param[in]     This           Pointer to the EFI_IP4_CONFIG2_PROTOCOL instance.
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
EfiIp4Config2RegisterDataNotify (
  IN EFI_IP4_CONFIG2_PROTOCOL   *This,
  IN EFI_IP4_CONFIG2_DATA_TYPE  DataType,
  IN EFI_EVENT                  Event
  )
{
  EFI_TPL               OldTpl;
  EFI_STATUS            Status;
  IP4_CONFIG2_INSTANCE  *Instance;
  NET_MAP               *EventMap;
  NET_MAP_ITEM          *Item;

  if ((This == NULL) || (Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip4Config2DataTypeMaximum) {
    return EFI_UNSUPPORTED;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = IP4_CONFIG2_INSTANCE_FROM_PROTOCOL (This);
  EventMap = &Instance->DataItem[DataType].EventMap;

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

  @param  This                   The pointer to the EFI_IP4_CONFIG2_PROTOCOL instance.
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
EfiIp4Config2UnregisterDataNotify (
  IN EFI_IP4_CONFIG2_PROTOCOL   *This,
  IN EFI_IP4_CONFIG2_DATA_TYPE  DataType,
  IN EFI_EVENT                  Event
  )
{
  EFI_TPL               OldTpl;
  EFI_STATUS            Status;
  IP4_CONFIG2_INSTANCE  *Instance;
  NET_MAP_ITEM          *Item;

  if ((This == NULL) || (Event == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataType >= Ip4Config2DataTypeMaximum) {
    return EFI_NOT_FOUND;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance = IP4_CONFIG2_INSTANCE_FROM_PROTOCOL (This);

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
  Initialize an IP4_CONFIG2_INSTANCE.

  @param[out]    Instance       The buffer of IP4_CONFIG2_INSTANCE to be initialized.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.
  @retval EFI_SUCCESS           The IP4_CONFIG2_INSTANCE initialized successfully.

**/
EFI_STATUS
Ip4Config2InitInstance (
  OUT IP4_CONFIG2_INSTANCE  *Instance
  )
{
  IP4_SERVICE            *IpSb;
  IP4_CONFIG2_INSTANCE   *TmpInstance;
  LIST_ENTRY             *Entry;
  EFI_STATUS             Status;
  UINTN                  Index;
  UINT16                 IfIndex;
  IP4_CONFIG2_DATA_ITEM  *DataItem;

  IpSb = IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE (Instance);

  Instance->Signature = IP4_CONFIG2_INSTANCE_SIGNATURE;

  //
  // Determine the index of this interface.
  //
  IfIndex = 0;
  NET_LIST_FOR_EACH (Entry, &mIp4Config2InstanceList) {
    TmpInstance = NET_LIST_USER_STRUCT_S (Entry, IP4_CONFIG2_INSTANCE, Link, IP4_CONFIG2_INSTANCE_SIGNATURE);

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

  for (Index = 0; Index < Ip4Config2DataTypeMaximum; Index++) {
    //
    // Initialize the event map for each data item.
    //
    NetMapInit (&Instance->DataItem[Index].EventMap);
  }

  //
  // Initialize each data type: associate storage and set data size for the
  // fixed size data types, hook the SetData function, set the data attribute.
  //
  DataItem           = &Instance->DataItem[Ip4Config2DataTypeInterfaceInfo];
  DataItem->GetData  = Ip4Config2GetIfInfo;
  DataItem->Data.Ptr = &Instance->InterfaceInfo;
  DataItem->DataSize = sizeof (Instance->InterfaceInfo);
  SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED | DATA_ATTRIB_VOLATILE);
  Ip4Config2InitIfInfo (IpSb, &Instance->InterfaceInfo);

  DataItem           = &Instance->DataItem[Ip4Config2DataTypePolicy];
  DataItem->SetData  = Ip4Config2SetPolicy;
  DataItem->Data.Ptr = &Instance->Policy;
  DataItem->DataSize = sizeof (Instance->Policy);
  Instance->Policy   = Ip4Config2PolicyStatic;
  SET_DATA_ATTRIB (DataItem->Attribute, DATA_ATTRIB_SIZE_FIXED);

  DataItem          = &Instance->DataItem[Ip4Config2DataTypeManualAddress];
  DataItem->SetData = Ip4Config2SetManualAddress;
  DataItem->Status  = EFI_NOT_FOUND;

  DataItem          = &Instance->DataItem[Ip4Config2DataTypeGateway];
  DataItem->SetData = Ip4Config2SetGateway;
  DataItem->Status  = EFI_NOT_FOUND;

  DataItem          = &Instance->DataItem[Ip4Config2DataTypeDnsServer];
  DataItem->SetData = Ip4Config2SetDnsServer;
  DataItem->Status  = EFI_NOT_FOUND;

  Instance->Configured = TRUE;

  //
  // Try to read the config data from NV variable.
  // If not found, write initialized config data into NV variable
  // as a default config data.
  //
  Status = Ip4Config2ReadConfigData (IpSb->MacString, Instance);
  if (Status == EFI_NOT_FOUND) {
    Status = Ip4Config2WriteConfigData (IpSb->MacString, Instance);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance->Ip4Config2.SetData              = EfiIp4Config2SetData;
  Instance->Ip4Config2.GetData              = EfiIp4Config2GetData;
  Instance->Ip4Config2.RegisterDataNotify   = EfiIp4Config2RegisterDataNotify;
  Instance->Ip4Config2.UnregisterDataNotify = EfiIp4Config2UnregisterDataNotify;

  //
  // Publish the IP4 configuration form
  //
  return Ip4Config2FormInit (Instance);
}

/**
  Release an IP4_CONFIG2_INSTANCE.

  @param[in, out] Instance    The buffer of IP4_CONFIG2_INSTANCE to be freed.

**/
VOID
Ip4Config2CleanInstance (
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  )
{
  UINTN                  Index;
  IP4_CONFIG2_DATA_ITEM  *DataItem;

  if (Instance->DeclineAddress != NULL) {
    FreePool (Instance->DeclineAddress);
  }

  if (!Instance->Configured) {
    return;
  }

  if (Instance->Dhcp4Handle != NULL) {
    Ip4Config2DestroyDhcp4 (Instance);
  }

  //
  // Close the event.
  //
  if (Instance->Dhcp4Event != NULL) {
    gBS->CloseEvent (Instance->Dhcp4Event);
    Instance->Dhcp4Event = NULL;
  }

  for (Index = 0; Index < Ip4Config2DataTypeMaximum; Index++) {
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

  Ip4Config2FormUnload (Instance);

  RemoveEntryList (&Instance->Link);
}

/**
  The event handle for IP4 auto reconfiguration. The original default
  interface and route table will be removed as the default.

  @param[in]  Context                The IP4 service binding instance.

**/
VOID
EFIAPI
Ip4AutoReconfigCallBackDpc (
  IN VOID  *Context
  )
{
  IP4_SERVICE  *IpSb;

  IpSb = (IP4_SERVICE *)Context;
  NET_CHECK_SIGNATURE (IpSb, IP4_SERVICE_SIGNATURE);

  if (IpSb->State > IP4_SERVICE_UNSTARTED) {
    IpSb->State = IP4_SERVICE_UNSTARTED;
  }

  IpSb->Reconfig = TRUE;

  Ip4StartAutoConfig (&IpSb->Ip4Config2Instance);

  return;
}

/**
  Request Ip4AutoReconfigCallBackDpc as a DPC at TPL_CALLBACK.

  @param Event     The event that is signalled.
  @param Context   The IP4 service binding instance.

**/
VOID
EFIAPI
Ip4AutoReconfigCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Request Ip4AutoReconfigCallBackDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, Ip4AutoReconfigCallBackDpc, Context);
}
