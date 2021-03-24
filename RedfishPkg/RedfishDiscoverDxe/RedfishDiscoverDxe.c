/** @file

  The implementation of EFI Redfidh Discover Protocol.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishDiscoverInternal.h"

LIST_ENTRY mRedfishDiscoverList;
LIST_ENTRY mRedfishInstanceList;
EFI_SMBIOS_PROTOCOL *mSmbios = NULL;

UINTN mNumNetworkInterface = 0;
UINTN mNumRestExInstance = 0;
LIST_ENTRY mEfiRedfishDiscoverNetworkInterface;
LIST_ENTRY mEfiRedfishDiscoverRestExInstance;

EFI_GUID mRedfishDiscoverTcp4InstanceGuid = EFI_REDFISH_DISCOVER_TCP4_INSTANCE_GUID;
EFI_GUID mRedfishDiscoverTcp6InstanceGuid = EFI_REDFISH_DISCOVER_TCP6_INSTANCE_GUID;
EFI_GUID mRedfishDiscoverRestExInstanceGuid = EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_GUID;

EFI_HANDLE EfiRedfishDiscoverProtocolHandle = NULL;

EFI_STATUS
EFIAPI
Tcp4GetSubnetInfo (
  IN EFI_HANDLE ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *Instance
);

EFI_STATUS
EFIAPI
Tcp6GetSubnetInfo (
  IN EFI_HANDLE ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *Instance
);

static REDFISH_DISCOVER_REQUIRED_PROTOCOL gRequiredProtocol[] = {
  {
    ProtocolTypeTcp4,
    L"TCP4 Service Binding Protocol",
    &gEfiTcp4ProtocolGuid,
    &gEfiTcp4ServiceBindingProtocolGuid,
    &mRedfishDiscoverTcp4InstanceGuid,
    Tcp4GetSubnetInfo
  },
  {
    ProtocolTypeTcp6,
    L"TCP6 Service Binding Protocol",
    &gEfiTcp6ProtocolGuid,
    &gEfiTcp6ServiceBindingProtocolGuid,
    &mRedfishDiscoverTcp6InstanceGuid,
    Tcp6GetSubnetInfo
  },
  {
    ProtocolTypeRestEx,
    L"REST EX Service Binding Protocol",
    &gEfiRestExProtocolGuid,
    &gEfiRestExServiceBindingProtocolGuid,
    &mRedfishDiscoverRestExInstanceGuid,
    NULL
  }
};

/**
  This function creates REST EX instance for the found Resfish service.
  by known owner handle.

  @param[in]    Instance        EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE
  @param[in]    Token           Client token.

  @retval NULL  Instance not found.
  @retval EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE The instance owned by this owner.

**/
EFI_STATUS
CreateRestExInstance (
  IN EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *Instance,
  IN EFI_REDFISH_DISCOVERED_TOKEN *Token
  )
{
  EFI_STATUS Status;

  Status = RestExLibCreateChild (
            Instance->Owner,
            FixedPcdGetBool (PcdRedfishDiscoverAccessModeInBand)? EfiRestExServiceInBandAccess: EfiRestExServiceOutOfBandAccess,
            EfiRestExConfigHttp,
            EfiRestExServiceRedfish,
            &Token->DiscoverList.RedfishInstances->Information.RedfishRestExHandle
            );
  return Status;
}

/**
  This function gets EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE
  by known owner handle.

  @param[in]    ImageHandle             Image handle owns EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE.
  @param[in]    TargetNetworkInterface  Target network interface used by this EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE.
  @param[in]    DiscoverFlags           EFI_REDFISH_DISCOVER_FLAG

  @retval NULL  Instance not found.
  @retval EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE The instance owned by this owner.

**/
EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *
GetInstanceByOwner (
  IN EFI_HANDLE ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *TargetNetworkInterface,
  IN EFI_REDFISH_DISCOVER_FLAG DiscoverFlags
  )
{
  EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *ThisInstance;

  if (IsListEmpty (&mRedfishDiscoverList)) {
    return NULL;
  }
  ThisInstance =
    (EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *)GetFirstNode (&mRedfishDiscoverList);
  while (TRUE) {
    if ((ThisInstance->Owner == ImageHandle) &&
         (ThisInstance->DiscoverFlags == DiscoverFlags) &&
         (ThisInstance->NetworkInterface == TargetNetworkInterface)) {
      return ThisInstance;
    }
    if (IsNodeAtEnd (&mRedfishDiscoverList, &ThisInstance->Entry)) {
      break;
    }
    ThisInstance =
      (EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *)GetNextNode (&mRedfishDiscoverList, &ThisInstance->Entry);
  };
  return NULL;
}

/**
  This function gets the subnet information of this TCP4 instance.

  @param[in]            ImageHandle  EFI handle with this image.
  @param[in]            Instance  Instance of Network interface.
  @retval EFI_STATUS    Get subnet information successfully.
  @retval Otherwise     Fail to get subnet information.
**/
EFI_STATUS
EFIAPI
Tcp4GetSubnetInfo (
  IN EFI_HANDLE ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *Instance
)
{
  EFI_STATUS Status;
  EFI_TCP4_PROTOCOL *Tcp4;
  EFI_TCP4_CONFIG_DATA Tcp4CfgData;
  EFI_TCP4_OPTION Tcp4Option;
  EFI_IP4_MODE_DATA IpModedata;
  UINT8 SubnetMaskIndex;
  UINT8 BitMask;
  UINT8 PrefixLength;
  BOOLEAN GotPrefixLength;

  if (Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Tcp4 = (EFI_TCP4_PROTOCOL *)Instance->NetworkInterfaceProtocolInfo.NetworkProtocolInterface;

  ZeroMem ((VOID *)&Tcp4CfgData, sizeof (EFI_TCP4_CONFIG_DATA));
  ZeroMem ((VOID *)&Tcp4Option, sizeof (EFI_TCP4_OPTION));
  // Give a local host IP address just for getting subnet information.
  Tcp4CfgData.AccessPoint.UseDefaultAddress = TRUE;
  Tcp4CfgData.AccessPoint.RemoteAddress.Addr [0] = 127;
  Tcp4CfgData.AccessPoint.RemoteAddress.Addr [1] = 0;
  Tcp4CfgData.AccessPoint.RemoteAddress.Addr [2] = 0;
  Tcp4CfgData.AccessPoint.RemoteAddress.Addr [3] = 1;
  Tcp4CfgData.AccessPoint.RemotePort  = 80;
  Tcp4CfgData.AccessPoint.ActiveFlag  = TRUE;

  Tcp4CfgData.ControlOption = &Tcp4Option;
  Tcp4Option.ReceiveBufferSize      = 65535;
  Tcp4Option.SendBufferSize         = 65535;
  Tcp4Option.MaxSynBackLog          = 5;
  Tcp4Option.ConnectionTimeout      = 60;
  Tcp4Option.DataRetries            = 12;
  Tcp4Option.FinTimeout             = 2;
  Tcp4Option.KeepAliveProbes        = 6;
  Tcp4Option.KeepAliveTime          = 7200;
  Tcp4Option.KeepAliveInterval      = 30;
  Tcp4Option.EnableNagle            = TRUE;
  Status = Tcp4->Configure (Tcp4, &Tcp4CfgData);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can't get subnet information\n", __FUNCTION__));
    return Status;
  }
  Status = Tcp4->GetModeData (Tcp4, NULL, NULL, &IpModedata, NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can't get IP mode data information\n", __FUNCTION__));
    return Status;
  }
  IP4_COPY_ADDRESS (&Instance->SubnetMask, &IpModedata.ConfigData.SubnetMask);
  Instance->SubnetAddr.v4.Addr [0] = IpModedata.ConfigData.StationAddress.Addr [0] & Instance->SubnetMask.v4.Addr [0];
  Instance->SubnetAddr.v4.Addr [1] = IpModedata.ConfigData.StationAddress.Addr [1] & Instance->SubnetMask.v4.Addr [1];
  Instance->SubnetAddr.v4.Addr [2] = IpModedata.ConfigData.StationAddress.Addr [2] & Instance->SubnetMask.v4.Addr [2];
  Instance->SubnetAddr.v4.Addr [3] = IpModedata.ConfigData.StationAddress.Addr [3] & Instance->SubnetMask.v4.Addr [3];
  //
  // Calculate the subnet mask prefix.
  //
  GotPrefixLength = FALSE;
  PrefixLength = 0;
  SubnetMaskIndex = 0;
  while (GotPrefixLength == FALSE && SubnetMaskIndex < 4) {
    BitMask = 0x80;
    while (BitMask != 0) {
      if ((Instance->SubnetMask.v4.Addr [SubnetMaskIndex] & BitMask) != 0) {
        PrefixLength ++;
      } else {
        GotPrefixLength = TRUE;
        break;
      }
      BitMask = BitMask >> 1;
    };
    SubnetMaskIndex ++;
  };
  Instance->SubnetPrefixLength = PrefixLength;
  return EFI_SUCCESS;
}

/**
  This function gets the subnet information of this TCP6 instance.

  @param[in]            ImageHandle  EFI handle with this image.
  @param[in]            Instance  Instance of Network interface.
  @retval EFI_STATUS    Get subnet information successfully.
  @retval Otherwise     Fail to get subnet information.
**/
EFI_STATUS
EFIAPI
Tcp6GetSubnetInfo (
  IN EFI_HANDLE ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *Instance
)
{
  EFI_STATUS Status;
  EFI_TCP6_PROTOCOL *Tcp6;
  EFI_IP6_MODE_DATA IpModedata;

  if (Instance == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Tcp6 = (EFI_TCP6_PROTOCOL *)Instance->NetworkInterfaceProtocolInfo.NetworkProtocolInterface;

  Status = Tcp6->GetModeData (Tcp6, NULL, NULL, &IpModedata, NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can't get IP mode data information\n"));
    return Status;
  }
  if (IpModedata.AddressCount == 0) {
    DEBUG ((DEBUG_INFO, "%a: No IPv6 address configured.\n"));
  }
  if (Instance->SubnetAddrInfoIPv6 != NULL) {
    FreePool (Instance->SubnetAddrInfoIPv6);
  }
  Instance->SubnetAddrInfoIPv6 = AllocateZeroPool (IpModedata.AddressCount * sizeof (EFI_IP6_ADDRESS_INFO));
  if (Instance->SubnetAddrInfoIPv6 == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory fir IPv6 subnet address information\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  Instance->SubnetAddrInfoIPv6Number = IpModedata.AddressCount;
  CopyMem (
    (VOID *)Instance->SubnetAddrInfoIPv6,
    (VOID *)&IpModedata.AddressList,
    IpModedata.AddressCount * sizeof (EFI_IP6_ADDRESS_INFO)
    );
  FreePool (IpModedata.AddressList);
  return EFI_SUCCESS;
}

/**
  This function searches EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL
  instance with the given  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE.

  @param[in] TargetNetworkInterface  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE.
                                     NULL for all EFI_REDFISH_DISCOVER_NETWORK_INTERFACEs.

  @retval Non-NULL  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL is returned.
  @retval NULL      Non of EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL instance is returned.
**/
EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *
GetTargetNetworkInterfaceInternal (
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE  *TargetNetworkInterface
  )
{
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface;

  ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
  while (TRUE) {
    if (CompareMem((VOID *)&ThisNetworkInterface->MacAddress, &TargetNetworkInterface->MacAddress, ThisNetworkInterface->HwAddressSize) == 0) {
      return ThisNetworkInterface;
    }
    if (IsNodeAtEnd (&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry)) {
      return NULL;
    }
    ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry);
  };
  return NULL;
}

/**
  This function validate if target network interface is ready for discovering
  Redfish service.

  @param[in] TargetNetworkInterface  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE.
                                     NULL for all EFI_REDFISH_DISCOVER_NETWORK_INTERFACEs.
  @param[in] Flags                   EFI_REDFISH_DISCOVER_FLAG

  @retval EFI_SUCCESS     Target network interface is ready to use.
  @retval EFI_UNSUPPORTED Target network interface is not ready to use.
**/
EFI_STATUS
ValidateTargetNetworkInterface (
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE  *TargetNetworkInterface,
  IN EFI_REDFISH_DISCOVER_FLAG Flags
  )
{
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface;

  if (IsListEmpty (&mEfiRedfishDiscoverNetworkInterface) && TargetNetworkInterface == NULL) {
    return EFI_UNSUPPORTED;
  }
  if (TargetNetworkInterface == NULL) {
    return EFI_SUCCESS; // Return EFI_SUCCESS if no network interface is specified.
  }

  ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode(&mEfiRedfishDiscoverNetworkInterface);
  while (TRUE) {
    if (CompareMem((VOID *)&ThisNetworkInterface->MacAddress, &TargetNetworkInterface->MacAddress, ThisNetworkInterface->HwAddressSize) == 0) {
      break;
    }
    if (IsNodeAtEnd (&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry)) {
      return EFI_UNSUPPORTED;
    }
    ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry);
  };
  if ((Flags & EFI_REDFISH_DISCOVER_SSDP) != 0) {
    // Validate if UDP4/6 is supported on the given network interface.
    // SSDP is not supported.

    return EFI_SUCCESS;
  }
  if (ThisNetworkInterface->NetworkInterfaceProtocolInfo.ProtocolControllerHandle == NULL) {
    return EFI_UNSUPPORTED; // The required protocol on this network interface is not found.
  }
  return EFI_SUCCESS;
}
/**
  This function returns number of network interface instance.

  @retval UINTN  Number of network interface instances.
**/
UINTN
NumberOfNetworkInterface (VOID)
{
  UINTN Num;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface;

  if (IsListEmpty (&mEfiRedfishDiscoverNetworkInterface)) {
    return 0;
  }

  Num = 1;
  ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
  while (TRUE) {
    if (IsNodeAtEnd (&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry)) {
      break;
    }
    ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry);
    Num ++;
  };
  return Num;
}

/**
  This function checks the  IP version supported on this
  netwoek interface.

  @param[in]    ThisNetworkInterface   EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL

  @retval TRUE  Is IPv6, otherwise IPv4.

**/
BOOLEAN
CheckIsIpVersion6 (
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface
)
{
  if (ThisNetworkInterface->NetworkProtocolType == ProtocolTypeTcp6) {
    return TRUE;
  }
  return FALSE;
}

/**
  This function discover Redfish service through SMBIOS host interface.

  @param[in]    Instance     EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE

  @retval EFI_SUCCESS        Redfish service is discovered through SMBIOS Host interface.
  @retval Others             Fail to discover Redfish service throught SMBIOS host interface

**/
EFI_STATUS
DiscoverRedfishHostInterface (IN EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *Instance)
{
  EFI_STATUS Status;
  REDFISH_OVER_IP_PROTOCOL_DATA *Data;
  REDFISH_INTERFACE_DATA  *DeviceDescriptor;
  CHAR8 UuidStr[sizeof"00000000-0000-0000-0000-000000000000" + 1];
  CHAR16 Ipv6Str [sizeof"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" + 1];
  CHAR8 RedfishServiceLocateStr [sizeof"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" + 1];
  UINTN StrSize;
  UINTN MacCompareStstus;
  BOOLEAN IsHttps;

  Data = NULL;
  DeviceDescriptor = NULL;

  if (mSmbios == NULL) {
    Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&mSmbios);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  Status = RedfishGetHostInterfaceProtocolData (mSmbios, &DeviceDescriptor, &Data); // Search for SMBIOS type 42h
  if (!EFI_ERROR (Status) && Data != NULL && DeviceDescriptor != NULL) {
    //
    // Chceck if we can reach out Redfish service using this network interface.
    // Check with MAC address using Device Descroptor Data Device Type 04 and Type 05.
    // Those two types of Redfish host interface device has MAC information.
    //
    if (DeviceDescriptor->DeviceType == REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE_V2) {
      MacCompareStstus = CompareMem(&Instance->NetworkInterface->MacAddress, &DeviceDescriptor->DeviceDescriptor.PciPcieDeviceV2.MacAddress, 6);
    } else if (DeviceDescriptor->DeviceType == REDFISH_HOST_INTERFACE_DEVICE_TYPE_USB_V2){
      MacCompareStstus = CompareMem(&Instance->NetworkInterface->MacAddress, &DeviceDescriptor->DeviceDescriptor.UsbDeviceV2.MacAddress, 6);
    } else {
      return EFI_UNSUPPORTED;
    }
    if (MacCompareStstus != 0) {
      return EFI_UNSUPPORTED;
    }

    if (Data->RedfishServiceIpAddressFormat == 1) {
      IP4_COPY_ADDRESS ((VOID *)&Instance->TargetIpAddress.v4, (VOID *)Data->RedfishServiceIpAddress);
    } else {
      IP6_COPY_ADDRESS ((VOID *)&Instance->TargetIpAddress.v6, (VOID *)Data->RedfishServiceIpAddress);
    }

    if (Instance->HostIntfValidation) {
      DEBUG ((DEBUG_ERROR,"%a:Send UPnP unicast SSDP to validate this Redfish Host Interface is not supported.\n", __FUNCTION__));
      Status = EFI_UNSUPPORTED;
    } else {
      //
      // Add this istance to list without detial information of Redfish
      // service.
      //
      IsHttps = FALSE;
      if (Data->RedfishServiceIpPort == 443) {
        IsHttps = TRUE;
      }
      StrSize = sizeof(UuidStr);
      AsciiSPrint(UuidStr, StrSize, "%g", &Data->ServiceUuid);
      //
      // Generate Redfish service location string.
      //
      if (Data->RedfishServiceIpAddressFormat == REDFISH_HOST_INTERFACE_HOST_IP_ADDRESS_FORMAT_IP6) {
        NetLibIp6ToStr((IPv6_ADDRESS *)&Data->RedfishServiceIpAddress, Ipv6Str, sizeof (Ipv6Str));
        if (Data->RedfishServiceIpPort == 0 || IsHttps == TRUE) {
            AsciiSPrintUnicodeFormat (
              RedfishServiceLocateStr,
              sizeof (RedfishServiceLocateStr),
              L"%s",
              Ipv6Str
            );
        } else {
            AsciiSPrintUnicodeFormat(
              RedfishServiceLocateStr,
              sizeof (RedfishServiceLocateStr),
              L"[%s]:%d",
              Ipv6Str,
              Data->RedfishServiceIpPort
            );
        }
      } else {
        if (Data->RedfishServiceIpPort == 0 || IsHttps == TRUE) {
          AsciiSPrint(
            RedfishServiceLocateStr,
            sizeof (RedfishServiceLocateStr),
            "%d.%d.%d.%d",
            Data->RedfishServiceIpAddress [0],
            Data->RedfishServiceIpAddress [1],
            Data->RedfishServiceIpAddress [2],
            Data->RedfishServiceIpAddress [3]
            );
        } else {
          AsciiSPrint(
            RedfishServiceLocateStr,
            sizeof (RedfishServiceLocateStr),
            "%d.%d.%d.%d:%d",
            Data->RedfishServiceIpAddress [0],
            Data->RedfishServiceIpAddress [1],
            Data->RedfishServiceIpAddress [2],
            Data->RedfishServiceIpAddress [3],
            Data->RedfishServiceIpPort
            );
        }
       }
      Status = AddAndSignalNewRedfishService (
            Instance,
            NULL,
            RedfishServiceLocateStr,
            UuidStr,
            NULL,
            NULL,
            NULL,
            NULL,
            IsHttps
            );
    }
  }
  return Status;
}

/**
  The function adds a new found Redfish service to internal list and
  notify client.

  @param[in]  Instance              EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE.
  @param[in]  RedfishVersion        Redfish version.
  @param[in]  RedfishLocation       Redfish location.
  @param[in]  Uuid                  Service UUID string.
  @param[in]  Os                    OS string.
  @param[in]  OsVer                 OS version string.
  @param[in]  Product               Product string.
  @param[in]  ProductVer            Product verison string.
  @param[in]  UseHttps              Redfish service requires secured connection.
  @retval EFI_SUCCESS               Redfish service is added to list successfully.

**/
EFI_STATUS
AddAndSignalNewRedfishService (
  IN EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *Instance,
  IN UINTN *RedfishVersion OPTIONAL,
  IN CHAR8 *RedfishLocation OPTIONAL,
  IN CHAR8 *Uuid  OPTIONAL,
  IN CHAR8 *Os  OPTIONAL,
  IN CHAR8 *OsVer  OPTIONAL,
  IN CHAR8 *Product  OPTIONAL,
  IN CHAR8 *ProductVer  OPTIONAL,
  IN BOOLEAN UseHttps
  )
{
  BOOLEAN NewFound;
  BOOLEAN InfoRefresh;
  BOOLEAN RestExOpened;
  BOOLEAN DeleteRestEx;
  EFI_STATUS Status;
  EFI_REDFISH_DISCOVERED_INTERNAL_LIST *DiscoveredList;
  EFI_REDFISH_DISCOVERED_INSTANCE *DiscoveredInstance;
  CHAR16 *Char16Uuid;
  EFI_REST_EX_PROTOCOL *RestEx;
  EFI_REST_EX_HTTP_CONFIG_DATA *RestExHttpConfigData;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *NetworkInterface;

  NewFound = TRUE;
  InfoRefresh = FALSE;
  Char16Uuid = NULL;
  RestExOpened = FALSE;
  DeleteRestEx = FALSE;

  DEBUG ((DEBUG_INFO,"%a:Add this instance to Redfish instance list.\n", __FUNCTION__));

  if (Uuid != NULL) {
    Char16Uuid = (CHAR16 *)AllocateZeroPool(AsciiStrSize((const CHAR8 *)Uuid) * sizeof(CHAR16));
    AsciiStrToUnicodeStrS ((const CHAR8 *)Uuid, Char16Uuid, AsciiStrSize((const CHAR8 *)Uuid) * sizeof(CHAR16));
  }
  DiscoveredList = NULL;
  DiscoveredInstance = NULL;
  RestExHttpConfigData = NULL;

  NetworkInterface = Instance->NetworkInterface;
  if (!IsListEmpty (&mRedfishInstanceList)) {
    //
    // Is this a duplicate redfish service.
    //
    DiscoveredList = (EFI_REDFISH_DISCOVERED_INTERNAL_LIST *)GetFirstNode (&mRedfishInstanceList);
    NewFound = FALSE;
    do {
      if (Char16Uuid == NULL || DiscoveredList->Instance->Information.Uuid == NULL) {
        //
        // Check if this Redfish instance already found using IP addrress.
        //
        if (!CheckIsIpVersion6(NetworkInterface)) {
          if (CompareMem ((VOID *)&Instance->TargetIpAddress.v4,
                      (VOID *)&DiscoveredList->Instance->Information.RedfishHostIpAddress.v4,
                      sizeof (EFI_IPv4_ADDRESS)
                      ) == 0)
          {
            DiscoveredInstance = DiscoveredList->Instance;
            if (DiscoveredList->Instance->Information.Uuid == NULL &&
                Char16Uuid != NULL) {
              InfoRefresh = TRUE;
              DiscoveredInstance = DiscoveredList->Instance;
              DEBUG((DEBUG_INFO,"*** This Redfish Service information refresh ***\n"));
            }
            break;
          }
        } else {
          if (CompareMem ((VOID *)&Instance->TargetIpAddress.v6,
                      (VOID *)&DiscoveredList->Instance->Information.RedfishHostIpAddress.v6,
                      sizeof (EFI_IPv6_ADDRESS)
                      ) == 0)
          {
            DiscoveredInstance = DiscoveredList->Instance;
            break;
          }
        }
      } else {
        //
        // Check if this Redfish instance already found using UUID.
        //
        if (StrCmp((const CHAR16 *)Char16Uuid, (const CHAR16 *)DiscoveredList->Instance->Information.Uuid) == 0) {
          DiscoveredInstance = DiscoveredList->Instance;
          break;
        }
      }
      if (IsNodeAtEnd (&mRedfishInstanceList, &DiscoveredList->NextInstance)) {
        NewFound = TRUE;
        break;
      }
      DiscoveredList = (EFI_REDFISH_DISCOVERED_INTERNAL_LIST *)GetNextNode (&mRedfishInstanceList, &DiscoveredList->NextInstance);
    } while (TRUE);
  }
  if (NewFound || InfoRefresh) {
    if (!InfoRefresh) {
      DiscoveredList = (EFI_REDFISH_DISCOVERED_INTERNAL_LIST *)AllocateZeroPool(sizeof(EFI_REDFISH_DISCOVERED_INTERNAL_LIST));
      if (DiscoveredList == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      InitializeListHead (&DiscoveredList->NextInstance);
      DiscoveredInstance = (EFI_REDFISH_DISCOVERED_INSTANCE *)AllocateZeroPool(sizeof(EFI_REDFISH_DISCOVERED_INSTANCE));
      if (DiscoveredInstance == NULL) {
        FreePool ((VOID *)DiscoveredList);
        return EFI_OUT_OF_RESOURCES;
      }
    }
    DEBUG ((DEBUG_INFO,"*** Redfish Service Information ***\n"));

    DiscoveredInstance->Information.UseHttps = UseHttps;
    if (RedfishVersion != NULL) {
      DiscoveredInstance->Information.RedfishVersion = *RedfishVersion;
      DEBUG ((DEBUG_INFO,"Redfish service version: %d.\n", DiscoveredInstance->Information.RedfishVersion));
    }
    if (RedfishLocation != NULL) {
      DiscoveredInstance->Information.Location = (CHAR16 *)AllocatePool(AsciiStrSize((const CHAR8 *)RedfishLocation) * sizeof(CHAR16));
      AsciiStrToUnicodeStrS ((const CHAR8 *)RedfishLocation, DiscoveredInstance->Information.Location, AsciiStrSize((const CHAR8 *)RedfishLocation) * sizeof(CHAR16));
      DEBUG ((DEBUG_INFO,"Redfish service location: %s.\n", DiscoveredInstance->Information.Location));
    }
    if (Uuid != NULL) {
      DiscoveredInstance->Information.Uuid = (CHAR16 *)AllocatePool(AsciiStrSize((const CHAR8 *)Uuid) * sizeof(CHAR16));
      AsciiStrToUnicodeStrS ((const CHAR8 *)Uuid, DiscoveredInstance->Information.Uuid, AsciiStrSize((const CHAR8 *)Uuid) * sizeof(CHAR16));
      DEBUG ((DEBUG_INFO,"Service UUID: %s.\n", DiscoveredInstance->Information.Uuid));
    }
    if (Os != NULL) {
      DiscoveredInstance->Information.Os = (CHAR16 *)AllocatePool(AsciiStrSize((const CHAR8 *)Os) * sizeof(CHAR16));
      AsciiStrToUnicodeStrS ((const CHAR8 *)Os, DiscoveredInstance->Information.Os, AsciiStrSize((const CHAR8 *)Os) * sizeof(CHAR16));
      DEBUG ((DEBUG_INFO,"Redfish service OS: %s, Version:%s.\n", DiscoveredInstance->Information.Os, DiscoveredInstance->Information.OsVersion));
    }
    if (OsVer != NULL) {
      DiscoveredInstance->Information.OsVersion = (CHAR16 *)AllocatePool(AsciiStrSize((const CHAR8 *)OsVer) * sizeof(CHAR16));
      AsciiStrToUnicodeStrS ((const CHAR8 *)OsVer, DiscoveredInstance->Information.OsVersion, AsciiStrSize((const CHAR8 *)OsVer) * sizeof(CHAR16));
    }
    if (Product != NULL && ProductVer != NULL) {
      DiscoveredInstance->Information.Product = (CHAR16 *)AllocatePool(AsciiStrSize((const CHAR8 *)Product) * sizeof(CHAR16));
      AsciiStrToUnicodeStrS ((const CHAR8 *)Product, DiscoveredInstance->Information.Product, AsciiStrSize((const CHAR8 *)Product) * sizeof(CHAR16));
      DiscoveredInstance->Information.ProductVer = (CHAR16 *)AllocatePool(AsciiStrSize((const CHAR8 *)ProductVer) * sizeof(CHAR16));
      AsciiStrToUnicodeStrS ((const CHAR8 *)ProductVer, DiscoveredInstance->Information.ProductVer, AsciiStrSize((const CHAR8 *)ProductVer) * sizeof(CHAR16));
      DEBUG ((DEBUG_INFO,"Redfish service product: %s, Version:%s.\n", DiscoveredInstance->Information.Product, DiscoveredInstance->Information.ProductVer));
    }

    if (RedfishLocation == NULL) {
      // This is the Redfish reported from SMBIOS 42h
      // without validation.

      IP4_COPY_ADDRESS((VOID *)&DiscoveredInstance->Information.RedfishHostIpAddress.v4, (VOID *)&Instance->TargetIpAddress.v4);
    }
    if (!InfoRefresh) {
      DiscoveredList->Instance = DiscoveredInstance;
      InsertTailList(&mRedfishInstanceList, &DiscoveredList->NextInstance);
    }
    DiscoveredInstance->Status = EFI_SUCCESS;
  } else {
    if (DiscoveredList != NULL) {
      DEBUG((DEBUG_INFO,"*** This Redfish Service was already found ***\n"));
      if (DiscoveredInstance->Information.Uuid != NULL) {
        DEBUG((DEBUG_INFO,"Service UUID: %s.\n", DiscoveredInstance->Information.Uuid));
      } else {
        DEBUG((DEBUG_INFO,"Service UUID: unknown.\n"));
      }
    }
  }
  if (Char16Uuid != NULL) {
    FreePool((VOID *)Char16Uuid);
  }

  Status = EFI_SUCCESS;
  if (NewFound || InfoRefresh) {
    //
    // Build up EFI_REDFISH_DISCOVERED_LIST in token.
    //
    Instance->DiscoverToken->DiscoverList.NumberOfServiceFound = 1;
    Instance->DiscoverToken->DiscoverList.RedfishInstances = DiscoveredInstance;
    DiscoveredInstance->Status = EFI_SUCCESS;
    if (!InfoRefresh) {
      Status = CreateRestExInstance (Instance, Instance->DiscoverToken); // Create REST EX child.
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a:Can't create REST EX child instance.\n",__FUNCTION__));
        goto ON_EXIT;
      }
      Status = gBS->OpenProtocol ( // Configure local host information.
                  Instance->DiscoverToken->DiscoverList.RedfishInstances->Information.RedfishRestExHandle,
                  &gEfiRestExProtocolGuid,
                  (VOID **)&RestEx,
                  Instance->NetworkInterface->OpenDriverAgentHandle,
                  Instance->NetworkInterface->OpenDriverControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
      if (EFI_ERROR (Status)) {
        DeleteRestEx = TRUE;
        goto ERROR_EXIT;
      }
      RestExOpened = TRUE;
      RestExHttpConfigData = AllocateZeroPool (sizeof (EFI_REST_EX_HTTP_CONFIG_DATA));
      if (RestExHttpConfigData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        DeleteRestEx = TRUE;
        goto EXIT_FREE_CONFIG_DATA;
      }
      RestExHttpConfigData->SendReceiveTimeout = 5000;
      RestExHttpConfigData->HttpConfigData.HttpVersion = HttpVersion11;
      RestExHttpConfigData->HttpConfigData.LocalAddressIsIPv6 = CheckIsIpVersion6(NetworkInterface);
      if (RestExHttpConfigData->HttpConfigData.LocalAddressIsIPv6) {
        RestExHttpConfigData->HttpConfigData.AccessPoint.IPv6Node = AllocateZeroPool (sizeof (EFI_HTTPv6_ACCESS_POINT));
        if (RestExHttpConfigData->HttpConfigData.AccessPoint.IPv6Node == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto EXIT_FREE_CONFIG_DATA;
        }
      } else {
        RestExHttpConfigData->HttpConfigData.AccessPoint.IPv4Node = AllocateZeroPool (sizeof (EFI_HTTPv4_ACCESS_POINT));
        if (RestExHttpConfigData->HttpConfigData.AccessPoint.IPv4Node == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto EXIT_FREE_CONFIG_DATA;
        }
        RestExHttpConfigData->HttpConfigData.AccessPoint.IPv4Node->UseDefaultAddress = TRUE;
      }
      Status = RestEx->Configure (
                       RestEx,
                       (EFI_REST_EX_CONFIG_DATA)(UINT8 *)RestExHttpConfigData
                       );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR,"%a:REST EX configured..\n", __FUNCTION__));
        DeleteRestEx = TRUE;
        goto EXIT_FREE_ALL;
      }
      //
      // Signal client, close REST EX before signaling client.
      //
      if (RestExOpened) {
        gBS->CloseProtocol(
             Instance->DiscoverToken->DiscoverList.RedfishInstances->Information.RedfishRestExHandle,
             &gEfiRestExProtocolGuid,
             Instance->NetworkInterface->OpenDriverAgentHandle,
             Instance->NetworkInterface->OpenDriverControllerHandle
          );
        RestExOpened = FALSE;
      }
    }
    Status = gBS->SignalEvent(Instance->DiscoverToken->Event);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,"%a:No event to signal!\n", __FUNCTION__));
    }
  }

EXIT_FREE_ALL:;
  if (RestExHttpConfigData != NULL && RestExHttpConfigData->HttpConfigData.AccessPoint.IPv4Node != NULL) {
    FreePool (RestExHttpConfigData->HttpConfigData.AccessPoint.IPv4Node);
  }

EXIT_FREE_CONFIG_DATA:;
  if (RestExHttpConfigData != NULL) {
    FreePool((VOID *)RestExHttpConfigData);
  }
  if (RestExOpened) {
    gBS->CloseProtocol(
           Instance->DiscoverToken->DiscoverList.RedfishInstances->Information.RedfishRestExHandle,
           &gEfiRestExProtocolGuid,
           Instance->NetworkInterface->OpenDriverAgentHandle,
           Instance->NetworkInterface->OpenDriverControllerHandle
        );
  }
ERROR_EXIT:;
    if (DeleteRestEx && RestExOpened) {
      gBS->CloseProtocol(
           Instance->DiscoverToken->DiscoverList.RedfishInstances->Information.RedfishRestExHandle,
           &gEfiRestExProtocolGuid,
           Instance->NetworkInterface->OpenDriverAgentHandle,
           Instance->NetworkInterface->OpenDriverControllerHandle
        );
    }
ON_EXIT:;
  return Status;
}

/**
  This function gets the subnet information of this network interface instance.
  can discover Redfish service on it.

  @param[in]    Instance     EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL instance.
  @param[in]    ImageHandle  EFI Image handle request the network interface list.

  @retval EFI_SUCCESS

**/
EFI_STATUS
NetworkInterfaceGetSubnetInfo (
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *Instance,
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS Status;
  UINT32 ProtocolType;
  UINT32 IPv6InfoIndex;
  EFI_IP6_ADDRESS_INFO *ThisSubnetAddrInfoIPv6;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *NewNetworkInterface;

  if (Instance->GotSubnetInfo) {
    return EFI_SUCCESS;
  }

  ProtocolType = Instance->NetworkProtocolType;
  if (gRequiredProtocol [ProtocolType].GetSubnetInfo != NULL && Instance->GotSubnetInfo == FALSE) {
    Status = gRequiredProtocol [ProtocolType].GetSubnetInfo (
        ImageHandle,
        Instance
        );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,"%a:Faile to get Subnet infomation.\n", __FUNCTION__));
      return Status;
    } else {
      DEBUG ((DEBUG_INFO,"%a:MAC address: %s\n", __FUNCTION__, Instance->StrMacAddr));
      if (CheckIsIpVersion6 (Instance)) {
        if (Instance->SubnetAddrInfoIPv6Number == 0) {
          DEBUG ((DEBUG_ERROR,"%a: There is no Subnet infomation for IPv6 network interface.\n", __FUNCTION__));
          return EFI_NOT_FOUND;
        }
        ThisSubnetAddrInfoIPv6 = Instance->SubnetAddrInfoIPv6; // First IPv6 address information.
        IP6_COPY_ADDRESS (&Instance->SubnetAddr.v6, &ThisSubnetAddrInfoIPv6->Address);
        Instance->SubnetPrefixLength = ThisSubnetAddrInfoIPv6->PrefixLength;
        DEBUG((DEBUG_INFO,"   IPv6 Subnet ID:%d, Prefix length: %d.\n",
               ThisSubnetAddrInfoIPv6->Address.Addr [7] + (UINT16)ThisSubnetAddrInfoIPv6->Address.Addr [6] * 256,
               ThisSubnetAddrInfoIPv6->PrefixLength)
               );
        //
        // If this is IPv6, then we may have to propagate network interface for IPv6 network scopes
        // according to the Ipv6 address information.
        //
        ThisSubnetAddrInfoIPv6 ++;
        for (IPv6InfoIndex = 0; IPv6InfoIndex < Instance->SubnetAddrInfoIPv6Number - 1; IPv6InfoIndex++) {
          //
          // Build up addtional EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL instances.
          //
          NewNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)AllocateZeroPool (sizeof (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL));
          if (NewNetworkInterface != NULL) {
            CopyMem ((VOID *)NewNetworkInterface, (VOID *)Instance, sizeof (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL)); // Clone information of first instance.
            IP6_COPY_ADDRESS (&NewNetworkInterface->SubnetAddr.v6, &ThisSubnetAddrInfoIPv6->Address);
            NewNetworkInterface->SubnetPrefixLength = ThisSubnetAddrInfoIPv6->PrefixLength;
            NewNetworkInterface->GotSubnetInfo = TRUE;
            InsertTailList (&mEfiRedfishDiscoverNetworkInterface, &NewNetworkInterface->Entry);
            ThisSubnetAddrInfoIPv6 ++;
            mNumNetworkInterface ++;
            DEBUG((DEBUG_INFO,"   IPv6 Subnet ID:%d, Prefix length: %d.\n",
                   ThisSubnetAddrInfoIPv6->Address.Addr [7] + (UINT16)ThisSubnetAddrInfoIPv6->Address.Addr [6] * 256,
                   ThisSubnetAddrInfoIPv6->PrefixLength)
                  );
          } else {
            return EFI_OUT_OF_RESOURCES;
          }
        }
      } else {
        DEBUG ((DEBUG_INFO,"   IPv4 Subnet:%d.%d.%d.%d Subnet mask: %d.%d.%d.%d.\n",
                    Instance->SubnetAddr.v4.Addr [0],
                    Instance->SubnetAddr.v4.Addr [1],
                    Instance->SubnetAddr.v4.Addr [2],
                    Instance->SubnetAddr.v4.Addr [3],
                    Instance->SubnetMask.v4.Addr [0],
                    Instance->SubnetMask.v4.Addr [1],
                    Instance->SubnetMask.v4.Addr [2],
                    Instance->SubnetMask.v4.Addr [3]
                    ));
      }
    }
  }
  Instance->GotSubnetInfo = TRUE; // Only try to get Subnet Info once.
  return EFI_SUCCESS;
}

/**
  This function gets the network interface list which Redfish discover protocol
  can discover Redfish service on it.

  @param[in]    This                  EFI_REDFISH_DISCOVER_PROTOCOL instance.
  @param[in]    ImageHandle           EFI Image handle request the network interface list,
  @param[out]   NumberOfNetworkIntfs  Number of network interfaces can do Redfish service discovery.
  @param[out]   NetworkIntfInstances  Network interface instances. It's an array of instance. The number of entries
                                      in array is indicated by NumberOfNetworkIntfs.
                                      Caller has to release the memory
                                      allocated by Redfish discover protocol.

  @retval EFI_SUCCESS        The information of network interface is returned in NumberOfNetworkIntfs and
                             NetworkIntfInstances.
  @retval Others             Fail to return the information of network interface.

**/
EFI_STATUS
EFIAPI
RedfishServiceGetNetworkInterface (
  IN EFI_REDFISH_DISCOVER_PROTOCOL   *This,
  IN EFI_HANDLE                      ImageHandle,
  OUT UINTN                          *NumberOfNetworkIntfs,
  OUT EFI_REDFISH_DISCOVER_NETWORK_INTERFACE **NetworkIntfInstances
)
{
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterfaceIntn;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE *ThisNetworkInterface;

  if (NetworkIntfInstances == NULL || NumberOfNetworkIntfs == NULL || ImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *NumberOfNetworkIntfs = 0;
  *NetworkIntfInstances = NULL;

  if (IsListEmpty ((const LIST_ENTRY*)&mEfiRedfishDiscoverNetworkInterface)) {
    return EFI_NOT_FOUND;
  }

  ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE *)AllocateZeroPool (sizeof (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE) * mNumNetworkInterface);
  if (ThisNetworkInterface == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  *NetworkIntfInstances = ThisNetworkInterface;
  ThisNetworkInterfaceIntn = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
  while (TRUE) {
    ThisNetworkInterface->IsIpv6 = FALSE;
    if (CheckIsIpVersion6 (ThisNetworkInterfaceIntn)) {
      ThisNetworkInterface->IsIpv6 = TRUE;
    }
    CopyMem((VOID *)&ThisNetworkInterface->MacAddress, &ThisNetworkInterfaceIntn->MacAddress, ThisNetworkInterfaceIntn->HwAddressSize);
    NetworkInterfaceGetSubnetInfo(ThisNetworkInterfaceIntn, ImageHandle); // Get subnet info.
    if (!ThisNetworkInterface->IsIpv6) {
      IP4_COPY_ADDRESS(&ThisNetworkInterface->SubnetId.v4, &ThisNetworkInterfaceIntn->SubnetAddr.v4); // IPv4 subnet information.
    } else {
      IP6_COPY_ADDRESS (&ThisNetworkInterface->SubnetId.v6, &ThisNetworkInterfaceIntn->SubnetAddr.v6); // IPv6 subnet information in IPv6 address information.
    }
    ThisNetworkInterface->SubnetPrefixLength = ThisNetworkInterfaceIntn->SubnetPrefixLength;
    ThisNetworkInterface->VlanId = ThisNetworkInterfaceIntn->VlanId;
    (*NumberOfNetworkIntfs) ++;
    if (IsNodeAtEnd (&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterfaceIntn->Entry)) {
      break;
    }
    ThisNetworkInterfaceIntn = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterfaceIntn->Entry);
    ThisNetworkInterface ++;
  };
  return EFI_SUCCESS;
}
/**
  This function acquires Redfish services by discovering static Redfish setting
  according to Redfish Host Interface or through SSDP. Returns a list of EFI
  handles in EFI_REDFISH_DISCOVERED_LIST. Each of EFI handle has cooresponding
  EFI REST EX instance installed on it. Each REST EX isntance is a child instance which
  created through EFI REST EX serivce protoocl for communicating with specific
  Redfish service.

  @param[in]    This                    EFI_REDFISH_DISCOVER_PROTOCOL instance.
  @param[in]    ImageHandle             EFI image owns these Redfish service instances.
  @param[in]    TargetNetworkInterface  Target network interface to do the discovery.
                                        NULL means discover Redfish service on all network interfaces on platform.
  @param[in]    Flags                   Redfish service discover flags.
  @param[in]    Token                   EFI_REDFISH_DISCOVERED_TOKEN instance.
                                        The memory of EFI_REDFISH_DISCOVERED_LIST and the strings in
                                        EFI_REDFISH_DISCOVERED_INFORMATION are all allocated by Acquire()
                                        and must be freed when caller invoke Release().

  @retval EFI_SUCCESS             REST EX instance of discovered Redfish services are returned.
  @retval EFI_INVALID_PARAMETERS  ImageHandle == NULL, Flags == 0, Token == NULL, Token->Timeout > 5,
                                  or Token->Event == NULL.
  @retval Others                  Fail acquire Redfish services.

**/
EFI_STATUS
EFIAPI
RedfishServiceAcquireService (
  IN EFI_REDFISH_DISCOVER_PROTOCOL          *This,
  IN EFI_HANDLE                             ImageHandle,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE  *TargetNetworkInterface,
  IN EFI_REDFISH_DISCOVER_FLAG              Flags,
  IN EFI_REDFISH_DISCOVERED_TOKEN           *Token
  )
{
  EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *Instance;
  EFI_STATUS Status1;
  EFI_STATUS Status2;
  BOOLEAN NewInstance;
  UINTN NumNetworkInterfaces;
  UINTN NetworkInterfacesIndex;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *TargetNetworkInterfaceInternal;

  DEBUG ((DEBUG_INFO,"%a:Entry.\n", __FUNCTION__));

  //
  // Validate parameters.
  //
  if (ImageHandle == NULL || Token == NULL || ((Flags & ~EFI_REDFISH_DISCOVER_VALIDATION) == 0)) {
    DEBUG ((DEBUG_ERROR,"%a:Invalid parameters.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }
  //
  // Validate target network interface.
  //
  if (EFI_ERROR (ValidateTargetNetworkInterface (TargetNetworkInterface, Flags))) {
      return EFI_UNSUPPORTED;
  }
  if (TargetNetworkInterface != NULL) {
    TargetNetworkInterfaceInternal = GetTargetNetworkInterfaceInternal (TargetNetworkInterface);
    NumNetworkInterfaces = 1;
  } else {
    TargetNetworkInterfaceInternal = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
    NumNetworkInterfaces = NumberOfNetworkInterface ();
    if (NumNetworkInterfaces == 0) {
      DEBUG ((DEBUG_ERROR,"%a:No network interface on platform.\n", __FUNCTION__));
      return EFI_UNSUPPORTED;
    }
  }
  for (NetworkInterfacesIndex = 0; NetworkInterfacesIndex < NumNetworkInterfaces; NetworkInterfacesIndex ++) {
    Status1 = EFI_SUCCESS;
    Status2 = EFI_SUCCESS;
    NewInstance = FALSE;
    Instance = GetInstanceByOwner (ImageHandle, TargetNetworkInterfaceInternal, Flags & ~EFI_REDFISH_DISCOVER_VALIDATION); // Check if we can re-use previous instance.
    if (Instance == NULL) {
      DEBUG ((DEBUG_INFO,"%a:Create new EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE.\n", __FUNCTION__));
      Instance = (EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE *)AllocateZeroPool(sizeof(EFI_REDFISH_DISCOVERED_INTERNAL_INSTANCE));
      if (Instance == NULL) {
        DEBUG ((DEBUG_ERROR,"%a:Memory allocation fail.\n", __FUNCTION__));
      }
      InitializeListHead (&Instance->Entry);
      Instance->Owner = ImageHandle;
      Instance->DiscoverFlags = Flags & ~EFI_REDFISH_DISCOVER_VALIDATION;
      Instance->NetworkInterface = TargetNetworkInterfaceInternal;
      //
      // Get subnet information in case subnet information is not set because
      // RedfishServiceGetNetworkInterfaces hasn't been called yet.
      //
      NetworkInterfaceGetSubnetInfo (TargetNetworkInterfaceInternal, ImageHandle);
      NewInstance = TRUE;
    }
    if (TargetNetworkInterfaceInternal->StrMacAddr != NULL) {
      DEBUG((DEBUG_INFO,"%a:Acquire Redfish service on network interface MAC address:%s.\n", __FUNCTION__, TargetNetworkInterfaceInternal->StrMacAddr));
    } else {
      DEBUG((DEBUG_INFO,"%a:WARNING: No MAC address on this network interface.\n", __FUNCTION__));
    }

    Instance->DiscoverToken = Token; // Always use the latest Token passed by caller.
    if ((Flags & EFI_REDFISH_DISCOVER_HOST_INTERFACE) != 0) {
      DEBUG ((DEBUG_INFO,"%a:Redfish HOST interface discovery.\n", __FUNCTION__));
      Instance->HostIntfValidation = FALSE;
      if ((Flags & EFI_REDFISH_DISCOVER_VALIDATION) != 0) {
        Instance->HostIntfValidation = TRUE;
      }
      Status1 = DiscoverRedfishHostInterface (Instance); // Discover Redfish service through Redfish Host Interface.
    }
    if ((Flags & EFI_REDFISH_DISCOVER_SSDP) != 0) {
      DEBUG ((DEBUG_ERROR,"%a:Redfish service discovery through SSDP is not supported\n", __FUNCTION__));
      return EFI_UNSUPPORTED;
    } else {
      if (EFI_ERROR (Status1) && EFI_ERROR (Status2)) {
        FreePool ((VOID *)Instance);
        DEBUG ((DEBUG_ERROR,"%a:Something wrong on Redfish service discovery Status1=%x, Status2=%x.\n", __FUNCTION__, Status1, Status2));
      } else {
        if (NewInstance) {
          InsertTailList(&mRedfishDiscoverList, &Instance->Entry);
        }
      }
    }
    if (TargetNetworkInterface == NULL) {
      //
      // Discover Redfish services on all of network interfaces.
      //
      TargetNetworkInterfaceInternal = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &TargetNetworkInterfaceInternal->Entry);
    }
  }
  return EFI_SUCCESS;
}

/**
  This function aborts Redfish service discovery on the given network interface.

  @param[in]    This                    EFI_REDFISH_DISCOVER_PROTOCOL instance.
  @param[in]    TargetNetworkInterface  Target network interface to do the discovery.

  @retval EFI_SUCCESS             REST EX instance of discovered Redfish services are returned.
  @retval Others                  Fail to abort Redfish service discovery.

**/
EFI_STATUS
EFIAPI
RedfishServiceAbortAcquire (
  IN EFI_REDFISH_DISCOVER_PROTOCOL      *This,
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE  *TargetNetworkInterface OPTIONAL
)
{
  // This function is used to abort Redfish service discovery through SSDP
  // on the network interface. SSDP is optionally supprted by EFI_REDFISH_DISCOVER_PROTOCOL,
  // we dont have implementation for SSDP now.

  return EFI_UNSUPPORTED;
}

/**
  This function releases Redfish services found by RedfishServiceAcquire().

  @param[in]    This         EFI_REDFISH_DISCOVER_PROTOCOL instance.
  @param[in]    InstanceList The Redfish service to release.

  @retval EFI_SUCCESS        REST EX instances of discovered Redfish are released.
  @retval Others             Fail to remove the entry

**/
EFI_STATUS
EFIAPI
RedfishServiceReleaseService (
  IN EFI_REDFISH_DISCOVER_PROTOCOL   *This,
  IN EFI_REDFISH_DISCOVERED_LIST *InstanceList
  )
{
  UINTN NumService;
  BOOLEAN AnyFailRelease;
  EFI_REDFISH_DISCOVERED_INSTANCE *ThisRedfishInstance;
  EFI_REDFISH_DISCOVERED_INTERNAL_LIST *DiscoveredRedfishInstance;

  if (IsListEmpty (&mRedfishInstanceList)) {
    DEBUG ((DEBUG_ERROR,"%a:No any discovered Redfish service.\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }
  AnyFailRelease = FALSE;
  ThisRedfishInstance = InstanceList->RedfishInstances;
  for (NumService = 0; NumService < InstanceList->NumberOfServiceFound; NumService ++) {
    DiscoveredRedfishInstance = (EFI_REDFISH_DISCOVERED_INTERNAL_LIST *)GetFirstNode(&mRedfishInstanceList);
    do {
      if (DiscoveredRedfishInstance->Instance == ThisRedfishInstance) {
        RemoveEntryList (&DiscoveredRedfishInstance->NextInstance);
        if (ThisRedfishInstance->Information.Location != NULL) {
          FreePool (ThisRedfishInstance->Information.Location);
        }
        if (ThisRedfishInstance->Information.Uuid != NULL) {
          FreePool (ThisRedfishInstance->Information.Uuid);
        }
        if (ThisRedfishInstance->Information.Os != NULL) {
          FreePool (ThisRedfishInstance->Information.Os);
        }
        if (ThisRedfishInstance->Information.OsVersion != NULL) {
          FreePool (ThisRedfishInstance->Information.OsVersion);
        }
        if (ThisRedfishInstance->Information.Product != NULL) {
          FreePool (ThisRedfishInstance->Information.Product);
        }
        if (ThisRedfishInstance->Information.ProductVer != NULL) {
          FreePool (ThisRedfishInstance->Information.ProductVer);
        }
        FreePool((VOID *)ThisRedfishInstance);
        goto ReleaseNext;
      }

      if (IsNodeAtEnd(&mRedfishInstanceList, &DiscoveredRedfishInstance->NextInstance)) {
        break;
      }
      DiscoveredRedfishInstance = (EFI_REDFISH_DISCOVERED_INTERNAL_LIST *)GetNextNode(&mRedfishInstanceList, &DiscoveredRedfishInstance->NextInstance);
    } while (TRUE);
    AnyFailRelease = TRUE;
ReleaseNext:;
    //
    // Release next discovered Redfish Service.
    //
    ThisRedfishInstance = (EFI_REDFISH_DISCOVERED_INSTANCE *)((UINT8 *)ThisRedfishInstance + sizeof (EFI_REDFISH_DISCOVERED_INSTANCE));
  }
  if (AnyFailRelease) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_SUCCESS;
  }
}

EFI_REDFISH_DISCOVER_PROTOCOL mRedfishDiscover = {
  RedfishServiceGetNetworkInterface,
  RedfishServiceAcquireService,
  RedfishServiceAbortAcquire,
  RedfishServiceReleaseService
};

/**
  This function create an EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL for the
  given network interface.


  @param[in]  ControllerHandle    MAC address of this network interface.
  @param[in]  NetworkProtocolType Network protocol type.
  @param[out] IsNewInstance       BOOLEAN means new instance or not.
  @param[out] NetworkInterface    Pointer to to EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL.

  @retval EFI_STATUS
**/
EFI_STATUS
CreateRedfishDiscoverNetworkInterface (
  IN EFI_HANDLE ControllerHandle,
  IN UINT32 NetworkProtocolType,
  OUT BOOLEAN  *IsNewInstance,
  OUT EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL **NetworkInterface
  )
{
  EFI_MAC_ADDRESS MacAddress;
  UINTN HwAddressSize;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *NewNetworkInterface;

  NetLibGetMacAddress (ControllerHandle, &MacAddress, &HwAddressSize);
  NewNetworkInterface = NULL;
  *IsNewInstance = TRUE;
  if (!IsListEmpty ((const LIST_ENTRY*)&mEfiRedfishDiscoverNetworkInterface)) {
    //
    // Check if this instance already exist.
    //
    ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
    if (ThisNetworkInterface != NULL) {
      while (TRUE) {
        if ((CompareMem ((CONST VOID *)&ThisNetworkInterface->MacAddress.Addr, (CONST VOID *)&MacAddress.Addr, HwAddressSize) == 0) &&
             (ThisNetworkInterface->NetworkProtocolType == NetworkProtocolType)){
          NewNetworkInterface = ThisNetworkInterface;
          *IsNewInstance = FALSE;
          break;
        }
        if (IsNodeAtEnd (&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry)) {
          NewNetworkInterface = NULL;
          break;
        }
        ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry);
      };
    }
  }
  if (NewNetworkInterface == NULL) {
    //
    // Create a new instance.
    //
    NewNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)AllocateZeroPool (sizeof (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL));
    if (NewNetworkInterface == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    NewNetworkInterface->HwAddressSize = HwAddressSize;
    CopyMem (&NewNetworkInterface->MacAddress.Addr, &MacAddress.Addr, NewNetworkInterface->HwAddressSize);
    NetLibGetMacString (ControllerHandle, NULL, &NewNetworkInterface->StrMacAddr);
    NewNetworkInterface->VlanId = NetLibGetVlanId (ControllerHandle);
  }
  *NetworkInterface = NewNetworkInterface;
  return EFI_SUCCESS;
}

/**
  This function destory network interface


  @param[in]  ThisNetworkInterface EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL instance.

  @retval EFI_STATUS
**/
EFI_STATUS
DestroyRedfishNetwrokInterface (
  IN EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface
  )
{
  EFI_STATUS Status;

  Status = gBS->UninstallProtocolInterface(
                  ThisNetworkInterface->OpenDriverControllerHandle,
                  gRequiredProtocol [ThisNetworkInterface->NetworkProtocolType].DiscoveredProtocolGuid,
                  &ThisNetworkInterface->NetworkInterfaceProtocolInfo.ProtocolDiscoverId
                  );
  RemoveEntryList (&ThisNetworkInterface->Entry);
  mNumNetworkInterface --;
  FreePool (ThisNetworkInterface);
  return Status;
}

/**
  Tests to see if the required protocols are provided on the given
  controller handle.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @retval EFI_SUCCESS              One of required protocol is found.
  @retval EFI_UNSUPPORTED          None of required protocol is found.
**/
EFI_STATUS
TestForRequiredProtocols (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE ControllerHandle
  )
{
  UINT32 Id;
  UINTN Index;
  EFI_STATUS Status;

  for (Index = 0; Index < (sizeof (gRequiredProtocol) / sizeof (REDFISH_DISCOVER_REQUIRED_PROTOCOL)); Index ++) {
    Status = gBS->OpenProtocol (
                  ControllerHandle,
                  gRequiredProtocol [Index].RequiredServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
    if (!EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      ControllerHandle,
                      gRequiredProtocol [Index].DiscoveredProtocolGuid,
                      (VOID **) &Id,
                      This->DriverBindingHandle,
                      ControllerHandle,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR, "%a: %s is found on this controller handle.\n", __FUNCTION__, gRequiredProtocol [Index].ProtocolName));
        return EFI_SUCCESS;
      }
    }
  }
  return EFI_UNSUPPORTED;
}

/**
  Build up network interface and create corresponding service through the given
  controller handle.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @retval EFI_SUCCESS              One of required protocol is found.
  @retval EFI_UNSUPPORTED          None of required protocol is found.
  @retval EFI_UNSUPPORTED          Failed to build up network interface.
**/
EFI_STATUS
BuildupNetworkInterface (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE ControllerHandle
  )
{
  UINT32 Id;
  UINT32 Index;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *NetworkInterface;
  BOOLEAN IsNew;
  EFI_STATUS Status;
  VOID *TempInterface;
  VOID **Interface;
  UINT32 *ProtocolDiscoverIdPtr;
  EFI_HANDLE OpenDriverAgentHandle;
  EFI_HANDLE OpenDriverControllerHandle;
  EFI_HANDLE *HandleOfProtocolInterfacePtr;
  EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL *RestExInstance;
  EFI_TPL OldTpl;
  BOOLEAN NewNetworkInterfaceInstalled;

  NewNetworkInterfaceInstalled = FALSE;
  Index = 0;
  do {
    Status = gBS->OpenProtocol ( // Already in list?
                    ControllerHandle,
                    gRequiredProtocol [Index].DiscoveredProtocolGuid,
                    (VOID **) &Id,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Index ++;
      if (Index == (sizeof(gRequiredProtocol) / sizeof(REDFISH_DISCOVER_REQUIRED_PROTOCOL))) {
        break;
      }
      continue;
    }

    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    gRequiredProtocol [Index].RequiredServiceBindingProtocolGuid,
                    &TempInterface,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      Index ++;
      if (Index == (sizeof(gRequiredProtocol) / sizeof(REDFISH_DISCOVER_REQUIRED_PROTOCOL))) {
        break;
      }
      continue;
    }
    if (gRequiredProtocol [Index].ProtocolType != ProtocolTypeRestEx) {
      OldTpl = gBS->RaiseTPL (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_TPL);
      Status = CreateRedfishDiscoverNetworkInterface(ControllerHandle, gRequiredProtocol [Index].ProtocolType, &IsNew, &NetworkInterface);
      if (EFI_ERROR (Status)) {
        gBS->RestoreTPL (OldTpl);
        return Status;
      }
      NetworkInterface->NetworkProtocolType = gRequiredProtocol [Index].ProtocolType;
      NetworkInterface->OpenDriverAgentHandle = This->DriverBindingHandle;
      NetworkInterface->OpenDriverControllerHandle = ControllerHandle;
      NetworkInterface->NetworkInterfaceProtocolInfo.ProtocolGuid = \
        *gRequiredProtocol [Index].RequiredProtocolGuid;
      NetworkInterface->NetworkInterfaceProtocolInfo.ProtocolServiceGuid = \
        *gRequiredProtocol [Index].RequiredServiceBindingProtocolGuid;
      ProtocolDiscoverIdPtr = &NetworkInterface->NetworkInterfaceProtocolInfo.ProtocolDiscoverId;
      OpenDriverAgentHandle = NetworkInterface->OpenDriverAgentHandle;
      OpenDriverControllerHandle = NetworkInterface->OpenDriverControllerHandle;
      HandleOfProtocolInterfacePtr = &NetworkInterface->NetworkInterfaceProtocolInfo.ProtocolControllerHandle;
      Interface = &NetworkInterface->NetworkInterfaceProtocolInfo.NetworkProtocolInterface;
      NewNetworkInterfaceInstalled = TRUE;
      if (IsNew) {
        InsertTailList (&mEfiRedfishDiscoverNetworkInterface, &NetworkInterface->Entry);
        mNumNetworkInterface ++;
      }
      gBS->RestoreTPL (OldTpl);
    } else {
      // Record REST_EX instance. REST_EX is created when clinet asks for Redfish service discovery.
      // Redfish Service Discover protocol will match REST EX to the corresponding EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL
      // when discovery.

      RestExInstance = (EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL *)AllocateZeroPool (sizeof (EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL));
      if (RestExInstance == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      RestExInstance->OpenDriverAgentHandle = This->DriverBindingHandle;
      RestExInstance->OpenDriverControllerHandle = ControllerHandle;
      RestExInstance->RestExControllerHandle = ControllerHandle;
      InitializeListHead (&RestExInstance->Entry);
      InsertTailList (&mEfiRedfishDiscoverRestExInstance, &RestExInstance->Entry);
      mNumRestExInstance ++;
      ProtocolDiscoverIdPtr = &RestExInstance->RestExId;
      OpenDriverAgentHandle = RestExInstance->OpenDriverAgentHandle;
      OpenDriverControllerHandle = RestExInstance->OpenDriverControllerHandle;
      HandleOfProtocolInterfacePtr = &RestExInstance->RestExChildHandle;
      Interface = (VOID **)&RestExInstance->RestExProtocolInterface;
    }
    Status = gBS->InstallProtocolInterface (
                    &ControllerHandle,
                    gRequiredProtocol [Index].DiscoveredProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    ProtocolDiscoverIdPtr
                    );
    if (EFI_ERROR (Status)) {
      Index ++;
      if (Index == (sizeof(gRequiredProtocol) / sizeof(REDFISH_DISCOVER_REQUIRED_PROTOCOL))) {
        break;
      }
      continue;
    }
    //
    // Create service binding child and open it BY_DRIVER.
    //
    Status = NetLibCreateServiceChild (
              ControllerHandle,
              This->ImageHandle,
              gRequiredProtocol [Index].RequiredServiceBindingProtocolGuid,
              HandleOfProtocolInterfacePtr
              );
    if (!EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                    *HandleOfProtocolInterfacePtr,
                    gRequiredProtocol [Index].RequiredProtocolGuid,
                    Interface,
                    OpenDriverAgentHandle,
                    OpenDriverControllerHandle,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
      if (!EFI_ERROR (Status)) {
        if (EfiRedfishDiscoverProtocolHandle == NULL &&
            (gRequiredProtocol [Index].ProtocolType == ProtocolTypeRestEx) &&
            !IsListEmpty (&mEfiRedfishDiscoverNetworkInterface)
            ) {
          // Install the fisrt Redfish Discover Protocol when EFI REST EX protcol is discovered.
          // This ensures EFI REST EX is ready while EFI_REDFISH_DISCOVER_PROTOCOL consumer acquires
          // Redfish serivce over network interface.

          Status = gBS->InstallProtocolInterface (
                          &EfiRedfishDiscoverProtocolHandle,
                          &gEfiRedfishDiscoverProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          (VOID *)&mRedfishDiscover
                          );
        } else if (EfiRedfishDiscoverProtocolHandle != NULL && NewNetworkInterfaceInstalled) {
           Status = gBS->ReinstallProtocolInterface (
                            EfiRedfishDiscoverProtocolHandle,
                            &gEfiRedfishDiscoverProtocolGuid,
                            (VOID *)&mRedfishDiscover,
                            (VOID *)&mRedfishDiscover
                            );
           NewNetworkInterfaceInstalled = FALSE;
        }
      }
      return Status;
    } else {
      Index ++;
      if (Index == (sizeof(gRequiredProtocol) / sizeof(REDFISH_DISCOVER_REQUIRED_PROTOCOL))) {
        break;
      }
      continue;
    }
  } while (Index < (sizeof(gRequiredProtocol) / sizeof(REDFISH_DISCOVER_REQUIRED_PROTOCOL)));
  return EFI_UNSUPPORTED;
}
/**
  Close the protocol opened for Redfish discovery. This function also destories
  the network services.

  @param[in]  ThisBindingProtocol     A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle        The handle of the controller to test. This handle
                                      must support a protocol interface that supplies
                                      an I/O abstraction to the driver.
  @param[in]  ThisRequiredProtocol    Pointer to the instance of REDFISH_DISCOVER_REQUIRED_PROTOCOL.
  @param[in]  DriverAgentHandle      Driver agent handle which used to open protocol earlier.
  @param[in]  DriverControllerHandle Driver controller handle which used to open protocol earlier.

  @retval EFI_SUCCESS                Prorocol is closed successfully.
  @retval Others                     Prorocol is closed unsuccessfully.

**/
EFI_STATUS
CloseProtocolService (
  IN EFI_DRIVER_BINDING_PROTOCOL  *ThisBindingProtocol,
  IN EFI_HANDLE  ControllerHandle,
  IN REDFISH_DISCOVER_REQUIRED_PROTOCOL *ThisRequiredProtocol,
  IN EFI_HANDLE DriverAgentHandle,
  IN EFI_HANDLE DriverControllerHandle
)
{
  EFI_STATUS Status;

  Status = gBS->CloseProtocol (
                   ControllerHandle,
                   ThisRequiredProtocol->RequiredProtocolGuid,
                   DriverAgentHandle,
                   DriverControllerHandle
                   );
  if (!EFI_ERROR (Status)) {
    NetLibDestroyServiceChild(
      ControllerHandle,
      ThisBindingProtocol->ImageHandle,
      ThisRequiredProtocol->RequiredServiceBindingProtocolGuid,
      ControllerHandle
      );
  }
  return Status;
}
/**
  Stop the services on network interface.

  @param[in]  ThisBindingProtocol  A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @retval EFI_SUCCESS              One of required protocol is found.
  @retval Others                   Faile to stop the services on network interface.
**/
EFI_STATUS
StopServiceOnNetworkInterface (
  IN EFI_DRIVER_BINDING_PROTOCOL  *ThisBindingProtocol,
  IN EFI_HANDLE ControllerHandle
  )
{
  UINT32 Index;
  EFI_STATUS Status;
  VOID *Interface;
  EFI_TPL OldTpl;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface;
  EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL *RestExInstance;

  for (Index = 0; Index < (sizeof (gRequiredProtocol) / sizeof (REDFISH_DISCOVER_REQUIRED_PROTOCOL)); Index ++) {
    Status = gBS->HandleProtocol (
                  ControllerHandle,
                  gRequiredProtocol [Index].RequiredProtocolGuid,
                  (VOID **)&Interface
                  );
    if (!EFI_ERROR (Status)) {
      if (gRequiredProtocol [Index].ProtocolType != ProtocolTypeRestEx) {
        if (IsListEmpty (&mEfiRedfishDiscoverNetworkInterface)) {
          return EFI_NOT_FOUND;
        }
        OldTpl = gBS->RaiseTPL (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_TPL);
        ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
        while (TRUE) {
          if (ThisNetworkInterface->NetworkInterfaceProtocolInfo.ProtocolControllerHandle == ControllerHandle) {

            Status = CloseProtocolService ( // Close protocol and destroy service.
                       ThisBindingProtocol,
                       ControllerHandle,
                       &gRequiredProtocol [Index],
                       ThisNetworkInterface->OpenDriverAgentHandle,
                       ThisNetworkInterface->OpenDriverControllerHandle
                       );
            if (!EFI_ERROR (Status)) {
              Status = DestroyRedfishNetwrokInterface (ThisNetworkInterface);
            }
            gBS->RestoreTPL (OldTpl);
            // Reinstall Redfish Discover protocol to notify network
            // interface change.

            Status = gBS->ReinstallProtocolInterface (
                            EfiRedfishDiscoverProtocolHandle,
                            &gEfiRedfishDiscoverProtocolGuid,
                            (VOID *)&mRedfishDiscover,
                            (VOID *)&mRedfishDiscover
                            );
            if (EFI_ERROR (Status)) {
              DEBUG((DEBUG_ERROR, "%a: Reinstall gEfiRedfishDiscoverProtocolGuid fail.", __FUNCTION__));
            }
            return Status;
          }
          if (IsNodeAtEnd (&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry)) {
            break;
          }
          ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverNetworkInterface, &ThisNetworkInterface->Entry);
        };
        gBS->RestoreTPL (OldTpl);
      } else {
        if (IsListEmpty (&mEfiRedfishDiscoverRestExInstance)) {
          return EFI_NOT_FOUND;
        }
        OldTpl = gBS->RaiseTPL (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_TPL);
        RestExInstance = (EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverRestExInstance);
        while (TRUE) {
          if (RestExInstance->RestExChildHandle == ControllerHandle) {
            Status = CloseProtocolService ( // Close REST_EX protocol.
                       ThisBindingProtocol,
                       ControllerHandle,
                       &gRequiredProtocol [Index],
                       RestExInstance->OpenDriverAgentHandle,
                       RestExInstance->OpenDriverControllerHandle
                       );
            RemoveEntryList (&RestExInstance->Entry);
            FreePool ((VOID *)RestExInstance);
            mNumRestExInstance --;
            gBS->RestoreTPL (OldTpl);
            return Status;
          }
          if (IsNodeAtEnd (&mEfiRedfishDiscoverRestExInstance, &RestExInstance->Entry)) {
            break;
          }
          RestExInstance = (EFI_REDFISH_DISCOVER_REST_EX_INSTANCE_INTERNAL *)GetNextNode(&mEfiRedfishDiscoverRestExInstance, &RestExInstance->Entry);
        };
        gBS->RestoreTPL (OldTpl);
      }
    }
  }
  return EFI_NOT_FOUND;
}
/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
RedfishDiscoverDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return TestForRequiredProtocols (This, ControllerHandle);
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
RedfishDiscoverDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return BuildupNetworkInterface (This, ControllerHandle);
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
RedfishDiscoverDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return StopServiceOnNetworkInterface (This, ControllerHandle);
}

EFI_DRIVER_BINDING_PROTOCOL gRedfishDiscoverDriverBinding = {
  RedfishDiscoverDriverBindingSupported,
  RedfishDiscoverDriverBindingStart,
  RedfishDiscoverDriverBindingStop,
  REDFISH_DISCOVER_VERSION,
  NULL,
  NULL
};

/**
  This is the declaration of an EFI image entry point.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RedfishDiscoverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;
  InitializeListHead (&mRedfishDiscoverList);
  InitializeListHead (&mRedfishInstanceList);
  InitializeListHead (&mEfiRedfishDiscoverNetworkInterface);
  InitializeListHead (&mEfiRedfishDiscoverRestExInstance);
  //
  // Install binding protoocl to obtain UDP and REST EX protocol.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gRedfishDiscoverDriverBinding,
             ImageHandle,
             &gRedfishDiscoverComponentName,
             &gRedfishDiscoverComponentName2
             );
  return Status;
}

/**
  This is the unload handle for Redfish discover module.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
RedfishDiscoverUnload (
  IN EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS Status;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *ThisNetworkInterface;

  Status = EFI_SUCCESS;
  // Destroy all network interfaces found by EFI Redfish Discover driver and
  // stop services created for Redfish Discover.

  while (!IsListEmpty (&mEfiRedfishDiscoverNetworkInterface)) {
    ThisNetworkInterface = (EFI_REDFISH_DISCOVER_NETWORK_INTERFACE_INTERNAL *)GetFirstNode (&mEfiRedfishDiscoverNetworkInterface);
    StopServiceOnNetworkInterface (&gRedfishDiscoverDriverBinding, ThisNetworkInterface->NetworkInterfaceProtocolInfo.ProtocolControllerHandle);
  };
  // Disconnect EFI Redfish discover driver controller to notify the
  // clinet which uses .EFI Redfish discover protocol.

  if (EfiRedfishDiscoverProtocolHandle != NULL) {
    //
    // Notify user EFI_REDFISH_DISCOVER_PROTOCOL is unloaded.
    //
    gBS->DisconnectController (EfiRedfishDiscoverProtocolHandle, NULL, NULL);
    Status = gBS->UninstallProtocolInterface(
                    EfiRedfishDiscoverProtocolHandle,
                    &gEfiRedfishDiscoverProtocolGuid,
                    (VOID *)&mRedfishDiscover
                    );
  }
  return Status;
}
