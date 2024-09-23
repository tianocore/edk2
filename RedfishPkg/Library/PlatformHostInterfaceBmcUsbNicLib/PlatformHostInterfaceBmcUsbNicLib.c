/** @file
  Source file to provide the platform Redfish Host Interface information
  of USB NIC Device exposed by BMC.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformHostInterfaceBmcUsbNicLib.h"

static EFI_GUID  mPlatformHostInterfaceBmcUsbNicReadinessGuid =
  BMC_USB_NIC_HOST_INTERFASCE_READINESS_GUID;
static EFI_EVENT  mPlatformHostInterfaceSnpEvent         = NULL;
static VOID       *mPlatformHostInterfaceSnpRegistration = NULL;

static LIST_ENTRY  mBmcUsbNic;
static LIST_ENTRY  mBmcIpmiLan;

/**
  Probe if the system supports Redfish Host Interface Credentail
  Bootstrapping.

  @retval TRUE   Yes, it is supported.
          FALSE  No, it is not supported.

**/
BOOLEAN
ProbeRedfishCredentialBootstrap (
  VOID
  )
{
  EDKII_REDFISH_AUTH_METHOD           AuthMethod;
  EDKII_REDFISH_CREDENTIAL2_PROTOCOL  *CredentialProtocol;
  CHAR8                               *UserName;
  CHAR8                               *Password;
  BOOLEAN                             ReturnBool;
  EFI_STATUS                          Status;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry\n", __func__));

  ReturnBool = FALSE;
  //
  // Locate HII credential protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEdkIIRedfishCredential2ProtocolGuid,
                  NULL,
                  (VOID **)&CredentialProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return FALSE;
  }

  Status = CredentialProtocol->GetAuthInfo (
                                 CredentialProtocol,
                                 &AuthMethod,
                                 &UserName,
                                 &Password
                                 );
  if (!EFI_ERROR (Status)) {
    ZeroMem (Password, AsciiStrSize (Password));
    FreePool (Password);
    ZeroMem (UserName, AsciiStrSize (UserName));
    FreePool (UserName);
    ReturnBool = TRUE;
  } else {
    if (Status == EFI_ACCESS_DENIED) {
      // bootstrap credential support was disabled
      ReturnBool = TRUE;
    }
  }

  DEBUG ((
    DEBUG_REDFISH_HOST_INTERFACE,
    "    Redfish Credential Bootstrapping is %a\n",
    ReturnBool ? "supported" : "not supported"
    ));
  return ReturnBool;
}

/**
  Get platform Redfish host interface device descriptor.

  @param[in] DeviceType         Pointer to retrieve device type.
  @param[out] DeviceDescriptor  Pointer to retrieve REDFISH_INTERFACE_DATA, caller has to free
                                this memory using FreePool().

  @retval EFI_NOT_FOUND   No Redfish host interface descriptor provided on this platform.

**/
EFI_STATUS
RedfishPlatformHostInterfaceDeviceDescriptor (
  IN UINT8                    *DeviceType,
  OUT REDFISH_INTERFACE_DATA  **DeviceDescriptor
  )
{
  HOST_INTERFACE_BMC_USB_NIC_INFO  *ThisInstance;
  REDFISH_INTERFACE_DATA           *InterfaceData;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry\n", __func__));

  if (IsListEmpty (&mBmcUsbNic)) {
    return EFI_NOT_FOUND;
  }

  // Check if BMC exposed USB NIC is found and ready for using.
  ThisInstance = (HOST_INTERFACE_BMC_USB_NIC_INFO *)GetFirstNode (&mBmcUsbNic);
  while (TRUE) {
    if (ThisInstance->IsExposedByBmc && ThisInstance->IsSuppportedHostInterface) {
      *DeviceType = REDFISH_HOST_INTERFACE_DEVICE_TYPE_USB_V2;

      // Fill up REDFISH_INTERFACE_DATA defined in Redfish host interface spec v1.3
      InterfaceData = (REDFISH_INTERFACE_DATA *)AllocateZeroPool (USB_INTERFACE_DEVICE_DESCRIPTOR_V2_SIZE_1_3);
      if (InterfaceData == NULL) {
        DEBUG ((DEBUG_ERROR, "Failed to allocate memory for REDFISH_INTERFACE_DATA\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      InterfaceData->DeviceType                                   = REDFISH_HOST_INTERFACE_DEVICE_TYPE_USB_V2;
      InterfaceData->DeviceDescriptor.UsbDeviceV2.Length          = USB_INTERFACE_DEVICE_DESCRIPTOR_V2_SIZE_1_3;
      InterfaceData->DeviceDescriptor.UsbDeviceV2.IdVendor        = ThisInstance->UsbVendorId;
      InterfaceData->DeviceDescriptor.UsbDeviceV2.IdProduct       = ThisInstance->UsbProductId;
      InterfaceData->DeviceDescriptor.UsbDeviceV2.SerialNumberStr = 0;
      CopyMem (
        (VOID *)&InterfaceData->DeviceDescriptor.UsbDeviceV2.MacAddress,
        (VOID *)ThisInstance->MacAddress,
        sizeof (InterfaceData->DeviceDescriptor.UsbDeviceV2.MacAddress)
        );
      InterfaceData->DeviceDescriptor.UsbDeviceV2.Characteristics              |= (UINT16)ThisInstance->CredentialBootstrapping;
      InterfaceData->DeviceDescriptor.UsbDeviceV2.CredentialBootstrappingHandle = 0;
      *DeviceDescriptor                                                         = InterfaceData;
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    REDFISH_INTERFACE_DATA is returned successfully.\n"));
      return EFI_SUCCESS;
    }

    if (IsNodeAtEnd (&mBmcUsbNic, &ThisInstance->NextInstance)) {
      break;
    }

    ThisInstance = (HOST_INTERFACE_BMC_USB_NIC_INFO *)
                   GetNextNode (&mBmcUsbNic, &ThisInstance->NextInstance);
  }

  return EFI_NOT_FOUND;
}

/**
  Get platform Redfish host interface protocol data.
  Caller should pass NULL in ProtocolRecord to retrive the first protocol record.
  Then continuously pass previous ProtocolRecord for retrieving the next ProtocolRecord.

  @param[in, out] ProtocolRecord  Pointer to retrieve the first or the next protocol record.
                                  caller has to free the new protocol record returned from
                                  this function using FreePool().
  @param[in] IndexOfProtocolData  The index of protocol data.

  @retval EFI_NOT_FOUND   No more protocol records.

**/
EFI_STATUS
RedfishPlatformHostInterfaceProtocolData (
  IN OUT MC_HOST_INTERFACE_PROTOCOL_RECORD  **ProtocolRecord,
  IN UINT8                                  IndexOfProtocolData
  )
{
  HOST_INTERFACE_BMC_USB_NIC_INFO    *ThisInstance;
  MC_HOST_INTERFACE_PROTOCOL_RECORD  *ThisProtocolRecord;
  REDFISH_OVER_IP_PROTOCOL_DATA      *RedfishOverIpData;
  UINT8                              HostNameLength;
  CHAR8                              *HostNameString;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry\n", __func__));

  if (IsListEmpty (&mBmcUsbNic) || (IndexOfProtocolData > 0)) {
    return EFI_NOT_FOUND;
  }

  ThisInstance = (HOST_INTERFACE_BMC_USB_NIC_INFO *)GetFirstNode (&mBmcUsbNic);
  while (TRUE) {
    if (ThisInstance->IsExposedByBmc  && ThisInstance->IsSuppportedHostInterface) {
      // Get the host name before allocating memory.
      HostNameString     = (CHAR8 *)PcdGetPtr (PcdRedfishHostName);
      HostNameLength     = (UINT8)AsciiStrSize (HostNameString);
      ThisProtocolRecord = (MC_HOST_INTERFACE_PROTOCOL_RECORD *)AllocateZeroPool (
                                                                  sizeof (MC_HOST_INTERFACE_PROTOCOL_RECORD) - 1 +
                                                                  sizeof (REDFISH_OVER_IP_PROTOCOL_DATA) - 1 +
                                                                  HostNameLength
                                                                  );
      if (ThisProtocolRecord == NULL) {
        DEBUG ((DEBUG_ERROR, "    Allocate memory fail for MC_HOST_INTERFACE_PROTOCOL_RECORD.\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      ThisProtocolRecord->ProtocolType        = MCHostInterfaceProtocolTypeRedfishOverIP;
      ThisProtocolRecord->ProtocolTypeDataLen = sizeof (REDFISH_OVER_IP_PROTOCOL_DATA) -1 + HostNameLength;
      RedfishOverIpData                       = (REDFISH_OVER_IP_PROTOCOL_DATA *)&ThisProtocolRecord->ProtocolTypeData[0];
      //
      // Fill up REDFISH_OVER_IP_PROTOCOL_DATA
      //

      // Service UUID
      ZeroMem ((VOID *)&RedfishOverIpData->ServiceUuid, sizeof (EFI_GUID));
      if (StrLen ((CONST CHAR16 *)PcdGetPtr (PcdRedfishServiceUuid)) != 0) {
        StrToGuid ((CONST CHAR16 *)PcdGetPtr (PcdRedfishServiceUuid), &RedfishOverIpData->ServiceUuid);
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, " Service UUID: %g", &RedfishOverIpData->ServiceUuid));
      }

      // HostIpAddressFormat and RedfishServiceIpDiscoveryType
      RedfishOverIpData->HostIpAssignmentType          = RedfishHostIpAssignmentUnknown;
      RedfishOverIpData->RedfishServiceIpDiscoveryType = RedfishHostIpAssignmentUnknown;
      if (ThisInstance->IpAssignedType == IpmiStaticAddrsss) {
        RedfishOverIpData->HostIpAssignmentType          = RedfishHostIpAssignmentStatic;
        RedfishOverIpData->RedfishServiceIpDiscoveryType = RedfishHostIpAssignmentStatic;
      } else if (ThisInstance->IpAssignedType == IpmiDynamicAddressBmcDhcp) {
        RedfishOverIpData->HostIpAssignmentType          = RedfishHostIpAssignmentDhcp;
        RedfishOverIpData->RedfishServiceIpDiscoveryType = RedfishHostIpAssignmentDhcp;
      }

      // HostIpAddressFormat and RedfishServiceIpAddressFormat, only support IPv4 for now.
      RedfishOverIpData->HostIpAddressFormat           = REDFISH_HOST_INTERFACE_HOST_IP_ADDRESS_FORMAT_IP4;
      RedfishOverIpData->RedfishServiceIpAddressFormat = REDFISH_HOST_INTERFACE_HOST_IP_ADDRESS_FORMAT_IP4;

      // HostIpAddress
      CopyMem (
        (VOID *)RedfishOverIpData->HostIpAddress,
        (VOID *)ThisInstance->HostIpAddressIpv4,
        sizeof (ThisInstance->HostIpAddressIpv4)
        );

      // HostIpMask and RedfishServiceIpMask
      CopyMem (
        (VOID *)RedfishOverIpData->HostIpMask,
        (VOID *)ThisInstance->SubnetMaskIpv4,
        sizeof (ThisInstance->SubnetMaskIpv4)
        );
      CopyMem (
        (VOID *)RedfishOverIpData->RedfishServiceIpMask,
        (VOID *)ThisInstance->SubnetMaskIpv4,
        sizeof (ThisInstance->SubnetMaskIpv4)
        );

      // RedfishServiceIpAddress
      CopyMem (
        (VOID *)RedfishOverIpData->RedfishServiceIpAddress,
        (VOID *)ThisInstance->RedfishIpAddressIpv4,
        sizeof (ThisInstance->RedfishIpAddressIpv4)
        );

      // RedfishServiceIpPort
      RedfishOverIpData->RedfishServiceIpPort = PcdGet16 (PcdRedfishServicePort);

      // RedfishServiceVlanId
      RedfishOverIpData->RedfishServiceVlanId = ThisInstance->VLanId;

      // RedfishServiceHostnameLength
      RedfishOverIpData->RedfishServiceHostnameLength = HostNameLength;

      // Redfish host name.
      CopyMem (
        (VOID *)&RedfishOverIpData->RedfishServiceHostname,
        (VOID *)HostNameString,
        HostNameLength
        );

      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    MC_HOST_INTERFACE_PROTOCOL_RECORD is returned successfully.\n"));
      *ProtocolRecord = ThisProtocolRecord;
      return EFI_SUCCESS;
    }

    if (IsNodeAtEnd (&mBmcUsbNic, &ThisInstance->NextInstance)) {
      break;
    }

    ThisInstance = (HOST_INTERFACE_BMC_USB_NIC_INFO *)
                   GetNextNode (&mBmcUsbNic, &ThisInstance->NextInstance);
  }

  return EFI_NOT_FOUND;
}

/**
  This function retrieve the information of BMC USB NIC.

  @retval EFI_SUCCESS      All necessary information is retrieved.
  @retval EFI_NOT_FOUND    There is no BMC exposed USB NIC.
  @retval Others           Other errors.

**/
EFI_STATUS
RetrievedBmcUsbNicInfo (
  VOID
  )
{
  EFI_STATUS                                      Status;
  UINT32                                          ResponseDataSize;
  HOST_INTERFACE_BMC_USB_NIC_INFO                 *ThisInstance;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   GetLanConfigReq;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetLanConfigReps;
  IPMI_LAN_IP_ADDRESS_SRC                         *IpAddressSrc;
  IPMI_LAN_IP_ADDRESS                             *DestIpAddress;
  IPMI_LAN_SUBNET_MASK                            *SubnetMask;
  IPMI_LAN_DEFAULT_GATEWAY                        *DefaultGateway;
  IPMI_LAN_VLAN_ID                                *LanVlanId;
  EFI_USB_DEVICE_DESCRIPTOR                       UsbDeviceDescriptor;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry\n", __func__));

  if (IsListEmpty (&mBmcUsbNic)) {
    return EFI_NOT_FOUND;
  }

  ThisInstance = (HOST_INTERFACE_BMC_USB_NIC_INFO *)GetFirstNode (&mBmcUsbNic);
  while (TRUE) {
    if (ThisInstance->IsExposedByBmc) {
      ThisInstance->IsSuppportedHostInterface = FALSE;

      // Probe if Redfish Host Interface Credential Bootstrapping is supported.
      ThisInstance->CredentialBootstrapping = ProbeRedfishCredentialBootstrap ();

      // Get IP address source
      GetLanConfigReq.SetSelector                     = 0;
      GetLanConfigReq.BlockSelector                   = 0;
      GetLanConfigReq.ChannelNumber.Bits.ChannelNo    = ThisInstance->IpmiLanChannelNumber;
      GetLanConfigReq.ChannelNumber.Bits.GetParameter = 0;
      GetLanConfigReq.ChannelNumber.Bits.Reserved     = 0;
      GetLanConfigReq.ParameterSelector               = IpmiLanIpAddressSource;
      ResponseDataSize                                = sizeof (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE) + sizeof (IPMI_LAN_IP_ADDRESS_SRC);
      GetLanConfigReps                                = (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE *)AllocateZeroPool (ResponseDataSize);
      GetLanConfigReps->CompletionCode                = IPMI_COMP_CODE_UNSPECIFIED;
      Status                                          = IpmiGetLanConfigurationParameters (
                                                          &GetLanConfigReq,
                                                          GetLanConfigReps,
                                                          &ResponseDataSize
                                                          );
      if (EFI_ERROR (Status) || (GetLanConfigReps->CompletionCode != IPMI_COMP_CODE_NORMAL)) {
        DEBUG ((DEBUG_ERROR, "    Failed to get IP address source at channel %d: %r, 0x%02x.\n", ThisInstance->IpmiLanChannelNumber, Status, GetLanConfigReps->CompletionCode));
        FreePool (GetLanConfigReps);
        return Status;
      }

      IpAddressSrc = (IPMI_LAN_IP_ADDRESS_SRC *)(GetLanConfigReps + 1);
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    IP address source at channel %d: %x\n", ThisInstance->IpmiLanChannelNumber, IpAddressSrc->Bits.AddressSrc));
      ThisInstance->IpAssignedType = IpAddressSrc->Bits.AddressSrc;
      FreePool (GetLanConfigReps);

      // Get LAN IPv4 IP address
      GetLanConfigReq.ParameterSelector = IpmiLanIpAddress;
      ResponseDataSize                  = sizeof (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE) + sizeof (IPMI_LAN_IP_ADDRESS);
      GetLanConfigReps                  = (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE *)AllocateZeroPool (ResponseDataSize);
      GetLanConfigReps->CompletionCode  = IPMI_COMP_CODE_UNSPECIFIED;
      Status                            = IpmiGetLanConfigurationParameters (
                                            &GetLanConfigReq,
                                            GetLanConfigReps,
                                            &ResponseDataSize
                                            );
      if (EFI_ERROR (Status) || (GetLanConfigReps->CompletionCode != IPMI_COMP_CODE_NORMAL)) {
        DEBUG ((DEBUG_ERROR, "    Failed to get Dest IP address at channel %d: %r, 0x%02x.\n", ThisInstance->IpmiLanChannelNumber, Status, GetLanConfigReps->CompletionCode));
        FreePool (GetLanConfigReps);
        return Status;
      }

      DestIpAddress = (IPMI_LAN_IP_ADDRESS *)(GetLanConfigReps + 1);
      DEBUG ((
        DEBUG_REDFISH_HOST_INTERFACE,
        "    Dest IP address at channel %d: %d.%d.%d.%d\n",
        ThisInstance->IpmiLanChannelNumber,
        DestIpAddress->IpAddress[0],
        DestIpAddress->IpAddress[1],
        DestIpAddress->IpAddress[2],
        DestIpAddress->IpAddress[3]
        ));
      CopyMem ((VOID *)&ThisInstance->RedfishIpAddressIpv4, (VOID *)&DestIpAddress->IpAddress, sizeof (DestIpAddress->IpAddress));
      //
      // According to the design spec:
      // https://github.com/tianocore/edk2/tree/master/RedfishPkg#platform-with-bmc-and-the-bmc-exposed-usb-network-device
      // The IP address at BMC USB NIC host end is the IP address at BMC end minus 1.
      //
      CopyMem ((VOID *)&ThisInstance->HostIpAddressIpv4, (VOID *)&DestIpAddress->IpAddress, sizeof (DestIpAddress->IpAddress));
      ThisInstance->HostIpAddressIpv4[sizeof (ThisInstance->HostIpAddressIpv4) - 1] -= 1;
      FreePool (GetLanConfigReps);
      DEBUG ((
        DEBUG_REDFISH_HOST_INTERFACE,
        "    Host IP address at channel %d: %d.%d.%d.%d\n",
        ThisInstance->IpmiLanChannelNumber,
        ThisInstance->HostIpAddressIpv4[0],
        ThisInstance->HostIpAddressIpv4[1],
        ThisInstance->HostIpAddressIpv4[2],
        ThisInstance->HostIpAddressIpv4[3]
        ));

      // Get IPv4 subnet mask
      GetLanConfigReq.ParameterSelector = IpmiLanSubnetMask;
      ResponseDataSize                  = sizeof (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE) + sizeof (IPMI_LAN_SUBNET_MASK);
      GetLanConfigReps                  = (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE *)AllocateZeroPool (ResponseDataSize);
      GetLanConfigReps->CompletionCode  = IPMI_COMP_CODE_UNSPECIFIED;
      Status                            = IpmiGetLanConfigurationParameters (
                                            &GetLanConfigReq,
                                            GetLanConfigReps,
                                            &ResponseDataSize
                                            );
      if ((EFI_ERROR (Status)) || (GetLanConfigReps->CompletionCode != IPMI_COMP_CODE_NORMAL)) {
        DEBUG ((DEBUG_ERROR, "    Failed to get subnet mask at channel %d: %r, 0x%02x.\n", ThisInstance->IpmiLanChannelNumber, Status, GetLanConfigReps->CompletionCode));
        FreePool (GetLanConfigReps);
        return Status;
      }

      SubnetMask = (IPMI_LAN_SUBNET_MASK *)(GetLanConfigReps + 1);
      DEBUG ((
        DEBUG_REDFISH_HOST_INTERFACE,
        "    Subnet mask at channel %d: %d.%d.%d.%d\n",
        ThisInstance->IpmiLanChannelNumber,
        SubnetMask->IpAddress[0],
        SubnetMask->IpAddress[1],
        SubnetMask->IpAddress[2],
        SubnetMask->IpAddress[3]
        ));
      CopyMem ((VOID *)&ThisInstance->SubnetMaskIpv4, (VOID *)&SubnetMask->IpAddress, sizeof (SubnetMask->IpAddress));
      FreePool (GetLanConfigReps);

      // Get Gateway IP address.
      GetLanConfigReq.ParameterSelector = IpmiLanDefaultGateway;
      ResponseDataSize                  = sizeof (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE) + sizeof (IPMI_LAN_DEFAULT_GATEWAY);
      GetLanConfigReps                  = (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE *)AllocateZeroPool (ResponseDataSize);
      GetLanConfigReps->CompletionCode  = IPMI_COMP_CODE_UNSPECIFIED;
      Status                            = IpmiGetLanConfigurationParameters (
                                            &GetLanConfigReq,
                                            GetLanConfigReps,
                                            &ResponseDataSize
                                            );
      if ((EFI_ERROR (Status)) || (GetLanConfigReps->CompletionCode != IPMI_COMP_CODE_NORMAL)) {
        DEBUG ((DEBUG_ERROR, "    Failed to get default gateway at channel %d: %r, 0x%02x.\n", ThisInstance->IpmiLanChannelNumber, Status, GetLanConfigReps->CompletionCode));
        FreePool (GetLanConfigReps);
        return Status;
      }

      DefaultGateway = (IPMI_LAN_DEFAULT_GATEWAY *)(GetLanConfigReps + 1);
      DEBUG ((
        DEBUG_REDFISH_HOST_INTERFACE,
        "    Gateway at channel %d: %d.%d.%d.%d\n",
        ThisInstance->IpmiLanChannelNumber,
        DefaultGateway->IpAddress[0],
        DefaultGateway->IpAddress[1],
        DefaultGateway->IpAddress[2],
        DefaultGateway->IpAddress[3]
        ));
      CopyMem ((VOID *)&ThisInstance->GatewayIpv4, (VOID *)&DefaultGateway->IpAddress, sizeof (DefaultGateway->IpAddress));
      FreePool (GetLanConfigReps);

      // Get VLAN ID
      GetLanConfigReq.ParameterSelector = IpmiLanVlanId;
      ResponseDataSize                  = sizeof (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE) + sizeof (IPMI_LAN_VLAN_ID);
      GetLanConfigReps                  = (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE *)AllocateZeroPool (ResponseDataSize);
      GetLanConfigReps->CompletionCode  = IPMI_COMP_CODE_UNSPECIFIED;
      Status                            = IpmiGetLanConfigurationParameters (
                                            &GetLanConfigReq,
                                            GetLanConfigReps,
                                            &ResponseDataSize
                                            );
      if ((EFI_ERROR (Status)) || (GetLanConfigReps->CompletionCode != IPMI_COMP_CODE_NORMAL)) {
        DEBUG ((DEBUG_ERROR, "    Failed to get VLAN ID at channel %d: %r, 0x%02x.\n", ThisInstance->IpmiLanChannelNumber, Status, GetLanConfigReps->CompletionCode));
        FreePool (GetLanConfigReps);
        return Status;
      }

      LanVlanId            = (IPMI_LAN_VLAN_ID *)(GetLanConfigReps + 1);
      ThisInstance->VLanId = 0;
      if (LanVlanId->Data2.Bits.Enabled == 1) {
        ThisInstance->VLanId = LanVlanId->Data1.VanIdLowByte | (LanVlanId->Data2.Bits.VanIdHighByte << 8);
      }

      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    VLAN ID %x\n", ThisInstance->VLanId));

      FreePool (GetLanConfigReps);

      //
      // Read USB device information.
      //
      if (ThisInstance->ThisUsbIo != NULL) {
        Status = ThisInstance->ThisUsbIo->UsbGetDeviceDescriptor (ThisInstance->ThisUsbIo, &UsbDeviceDescriptor);
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    USB NIC Vendor ID: 0x%04x, Device ID: 0x%04x\n", UsbDeviceDescriptor.IdVendor, UsbDeviceDescriptor.IdProduct));
          ThisInstance->UsbVendorId  = UsbDeviceDescriptor.IdVendor;
          ThisInstance->UsbProductId = UsbDeviceDescriptor.IdProduct;
        } else {
          DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    Fail to get USB device descriptor.\n"));
        }
      }

      // All information is retrieved.
      ThisInstance->IsSuppportedHostInterface = TRUE;
      return EFI_SUCCESS;
    }

    if (IsNodeAtEnd (&mBmcUsbNic, &ThisInstance->NextInstance)) {
      break;
    }

    ThisInstance = (HOST_INTERFACE_BMC_USB_NIC_INFO *)
                   GetNextNode (&mBmcUsbNic, &ThisInstance->NextInstance);
  }

  return EFI_NOT_FOUND;
}

/**
  This function caches the found IPMI LAN channel. So we
  don't have to sedn IPMI commands again if the USB NIC is
  connected later.

  @param[in] ChannelNum                The IPMI channel number.
  @param[in] IpmiLanChannelMacAddress  Pointer to EFI_MAC_ADDRESS.
  @param[in] IpmiLanMacAddressSize     The MAC address size.

  @retval EFI_SUCCESS          IPMI LAN channel is cached.
  @retval EFI_OUT_OF_RESOURCE  Memory allocated failed.
  @retval Others               Other errors.

**/
EFI_STATUS
CacheIpmiLanMac (
  IN UINT8            ChannelNum,
  IN EFI_MAC_ADDRESS  *IpmiLanChannelMacAddress,
  IN UINT8            IpmiLanMacAddressSize
  )
{
  BMC_IPMI_LAN_CHANNEL_INFO  *ChannelInfo;

  ChannelInfo = (BMC_IPMI_LAN_CHANNEL_INFO *)AllocateZeroPool (sizeof (BMC_IPMI_LAN_CHANNEL_INFO));
  if (ChannelInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ChannelInfo->Channel = ChannelNum;
  CopyMem ((VOID *)&ChannelInfo->MacAddress.Addr, (VOID *)IpmiLanChannelMacAddress->Addr, IpmiLanMacAddressSize);
  ChannelInfo->MacAddressSize = IpmiLanMacAddressSize;
  InitializeListHead (&ChannelInfo->NextInstance);
  InsertTailList (&mBmcIpmiLan, &ChannelInfo->NextInstance);
  return EFI_SUCCESS;
}

/**
  This function checks if the IPMI channel already identified
  previously.

  @param[in]  ChannelNum            The IPMI channel number.
  @param[out] CachedIpmiLanChannel  Pointer to retrieve the cached
                                    BMC_IPMI_LAN_CHANNEL_INFO.

  @retval EFI_SUCCESS          IPMI LAN channel is found.
  @retval Others               Other errors.

**/
EFI_STATUS
CheckCachedIpmiLanMac (
  IN UINT8                       ChannelNum,
  OUT BMC_IPMI_LAN_CHANNEL_INFO  **CachedIpmiLanChannel
  )
{
  BMC_IPMI_LAN_CHANNEL_INFO  *ThisInstance;

  if (IsListEmpty (&mBmcIpmiLan)) {
    return EFI_NOT_FOUND;
  }

  ThisInstance = (BMC_IPMI_LAN_CHANNEL_INFO *)GetFirstNode (&mBmcIpmiLan);
  while (TRUE) {
    if (ThisInstance->Channel == ChannelNum) {
      *CachedIpmiLanChannel = ThisInstance;
      return EFI_SUCCESS;
    }

    if (IsNodeAtEnd (&mBmcIpmiLan, &ThisInstance->NextInstance)) {
      break;
    }

    ThisInstance = (BMC_IPMI_LAN_CHANNEL_INFO *)
                   GetNextNode (&mBmcIpmiLan, &ThisInstance->NextInstance);
  }

  return EFI_NOT_FOUND;
}

/**
  This function goes through IPMI channels to find the
  mactched MAC addrss of BMC USB NIC endpoint.

  @param[in] UsbNicInfo  The instance of HOST_INTERFACE_BMC_USB_NIC_INFO.

  @retval EFI_SUCCESS          Yes, USB NIC exposed by BMC is found.
  @retval EFI_NOT_FOUND        No, USB NIC exposed by BMC is not found
                               on the existing SNP handle.
  @retval Others               Other errors.

**/
EFI_STATUS
HostInterfaceIpmiCheckMacAddress (
  IN HOST_INTERFACE_BMC_USB_NIC_INFO  *UsbNicInfo
  )
{
  EFI_STATUS                                      Status;
  EFI_STATUS                                      ExitStatus;
  UINTN                                           ChannelNum;
  UINT32                                          ResponseDataSize;
  IPMI_GET_CHANNEL_INFO_REQUEST                   GetChanelInfoRequest;
  IPMI_GET_CHANNEL_INFO_RESPONSE                  GetChanelInfoResponse;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_REQUEST   GetLanConfigReq;
  IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE  *GetLanConfigReps;
  BMC_IPMI_LAN_CHANNEL_INFO                       *CachedIpmiLanChannel;
  UINT8                                           IpmiLanMacAddressSize;
  EFI_MAC_ADDRESS                                 IpmiLanChannelMacAddress;
  BOOLEAN                                         AlreadyCached;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry.\n", __func__));

  GetLanConfigReps = NULL;
  AlreadyCached    = FALSE;
  if (!IsListEmpty (&mBmcIpmiLan)) {
    AlreadyCached = TRUE;
  }

  // Initial the get MAC address request.
  GetLanConfigReq.ChannelNumber.Uint8 = 0;
  GetLanConfigReq.SetSelector         = 0;
  GetLanConfigReq.BlockSelector       = 0;
  GetLanConfigReq.ParameterSelector   = IpmiLanMacAddress;

  ExitStatus = EFI_NOT_FOUND;
  for (ChannelNum = IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_1;
       ChannelNum <= IPMI_CHANNEL_NUMBER_IMPLEMENTATION_SPECIFIC_11;
       ChannelNum++)
  {
    IpmiLanMacAddressSize = 0;

    // Check if the IPMI channel information is already cached.
    Status = EFI_NOT_FOUND;
    if (AlreadyCached) {
      Status = CheckCachedIpmiLanMac ((UINT8)ChannelNum, &CachedIpmiLanChannel);
    }

    if (Status == EFI_SUCCESS) {
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "  Got cached IPMI LAN info.\n"));
      IpmiLanMacAddressSize = sizeof (IPMI_LAN_MAC_ADDRESS);
      CopyMem ((VOID *)&IpmiLanChannelMacAddress.Addr, (VOID *)&CachedIpmiLanChannel->MacAddress.Addr, IpmiLanMacAddressSize);
    } else {
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "  No cached IPMI LAN info\n"));
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "  Send NetFn = App, Command = 0x42 to channel %d\n", ChannelNum));
      GetChanelInfoRequest.ChannelNumber.Uint8          = 0;
      GetChanelInfoRequest.ChannelNumber.Bits.ChannelNo = (UINT8)ChannelNum;
      Status                                            = IpmiGetChannelInfo (
                                                            &GetChanelInfoRequest,
                                                            &GetChanelInfoResponse,
                                                            &ResponseDataSize
                                                            );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "    - Channel %d fails to send command.\n", ChannelNum));
        continue;
      }

      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    - Response data size = 0x%x\n", ResponseDataSize));
      if ((GetChanelInfoResponse.CompletionCode != IPMI_COMP_CODE_NORMAL) || (ResponseDataSize == 0)) {
        DEBUG ((DEBUG_ERROR, "    - Command returned fail: 0x%x.\n", GetChanelInfoResponse.CompletionCode));
        continue;
      }

      DEBUG ((
        DEBUG_REDFISH_HOST_INTERFACE,
        "    - Channel protocol = 0x%x, Media = 0x%x\n",
        GetChanelInfoResponse.ProtocolType.Bits.ChannelProtocolType,
        GetChanelInfoResponse.MediumType.Bits.ChannelMediumType
        ));

      if (GetChanelInfoResponse.ChannelNumber.Bits.ChannelNo != ChannelNum) {
        DEBUG ((
          DEBUG_ERROR,
          "    - ChannelNumber = %d in the response which is not macthed to the request.\n",
          GetChanelInfoResponse.ChannelNumber.Bits.ChannelNo
          ));
        continue;
      }

      if ((GetChanelInfoResponse.MediumType.Bits.ChannelMediumType == IPMI_CHANNEL_MEDIA_TYPE_802_3_LAN) &&
          (GetChanelInfoResponse.ProtocolType.Bits.ChannelProtocolType == IPMI_CHANNEL_PROTOCOL_TYPE_IPMB_1_0))
      {
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    - Channel %d is a LAN device!\n", ChannelNum));

        ResponseDataSize = sizeof (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE) +
                           sizeof (IPMI_LAN_MAC_ADDRESS);
        if (GetLanConfigReps == NULL) {
          GetLanConfigReps =
            (IPMI_GET_LAN_CONFIGURATION_PARAMETERS_RESPONSE *)AllocateZeroPool (ResponseDataSize);
          if (GetLanConfigReps == NULL) {
            DEBUG ((DEBUG_ERROR, "    Allocate memory failed for getting MAC address.\n"));
            continue;
          }
        }

        GetLanConfigReq.ChannelNumber.Bits.ChannelNo = (UINT8)ChannelNum;
        GetLanConfigReps->CompletionCode             = IPMI_COMP_CODE_UNSPECIFIED;
        Status                                       = IpmiGetLanConfigurationParameters (
                                                         &GetLanConfigReq,
                                                         GetLanConfigReps,
                                                         &ResponseDataSize
                                                         );
        if (EFI_ERROR (Status) || (GetLanConfigReps->CompletionCode != IPMI_COMP_CODE_NORMAL)) {
          DEBUG ((
            DEBUG_ERROR,
            "    Fails to get MAC address of channel %d, CompletionCode = %02x.\n",
            ChannelNum,
            GetLanConfigReps->CompletionCode
            ));
          continue;
        } else {
          DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    The MAC address of channel %d.\n", ChannelNum));
          DEBUG ((
            DEBUG_REDFISH_HOST_INTERFACE,
            "      %02x:%02x:%02x:%02x:%02x:%02x\n",
            *((UINT8 *)(GetLanConfigReps + 1) + 0),
            *((UINT8 *)(GetLanConfigReps + 1) + 1),
            *((UINT8 *)(GetLanConfigReps + 1) + 2),
            *((UINT8 *)(GetLanConfigReps + 1) + 3),
            *((UINT8 *)(GetLanConfigReps + 1) + 4),
            *((UINT8 *)(GetLanConfigReps + 1) + 5)
            ));
          IpmiLanMacAddressSize = sizeof (IPMI_LAN_MAC_ADDRESS);
          CopyMem ((VOID *)&IpmiLanChannelMacAddress.Addr, (VOID *)(GetLanConfigReps + 1), IpmiLanMacAddressSize);
        }
      }
    }

    if (IpmiLanMacAddressSize != 0) {
      if (!AlreadyCached) {
        // Cache this IPMI LAN channel.
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    Cache this IPMI LAN channel.\n"));
        CacheIpmiLanMac ((UINT8)ChannelNum, &IpmiLanChannelMacAddress, IpmiLanMacAddressSize);
      }

      //
      // According to design spec in Readme file under RedfishPkg.
      // https://github.com/tianocore/edk2/tree/master/RedfishPkg#platform-with-bmc-and-the-bmc-exposed-usb-network-device
      // Compare the first five elements of MAC address and the 6th element of MAC address.
      // The 6th element of MAC address must be the 6th element of
      // IPMI channel MAC address minus 1.
      //
      if ((IpmiLanMacAddressSize != UsbNicInfo->MacAddressSize) ||
          (CompareMem (
             (VOID *)UsbNicInfo->MacAddress,
             (VOID *)&IpmiLanChannelMacAddress.Addr,
             IpmiLanMacAddressSize - 1
             ) != 0) ||
          ((IpmiLanChannelMacAddress.Addr[IpmiLanMacAddressSize - 1] - 1) !=
           *(UsbNicInfo->MacAddress + IpmiLanMacAddressSize - 1))
          )
      {
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    MAC address is not matched.\n"));
        continue;
      }

      // This is the NIC exposed by BMC.
      UsbNicInfo->IpmiLanChannelNumber = (UINT8)ChannelNum;
      UsbNicInfo->IsExposedByBmc       = TRUE;
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    MAC address is matched.\n"));
      ExitStatus = EFI_SUCCESS;
      break;
    }
  }

  if (GetLanConfigReps != NULL) {
    FreePool (GetLanConfigReps);
  }

  return ExitStatus;
}

/**
  This function searches the next MSG_USB_DP device path node.

  @param[in]  ThisDevicePath    Device path to search.

  @retval NULL          MSG_USB_DP is not found.
          Otherwise     MSG_USB_DP is found.

**/
EFI_DEVICE_PATH_PROTOCOL *
UsbNicGetNextMsgUsbDp (
  IN EFI_DEVICE_PATH_PROTOCOL  *ThisDevicePath
  )
{
  if (ThisDevicePath == NULL) {
    return NULL;
  }

  while (TRUE) {
    ThisDevicePath = NextDevicePathNode (ThisDevicePath);
    if (IsDevicePathEnd (ThisDevicePath)) {
      return NULL;
    }

    if ((ThisDevicePath->Type == MESSAGING_DEVICE_PATH) && (ThisDevicePath->SubType == MSG_USB_DP)) {
      return ThisDevicePath;
    }
  }

  return NULL;
}

/**
  This function search the UsbIo handle that matches the UsbDevicePath.

  @param[in]  UsbDevicePath    Device path of this SNP handle.
  @param[out] UsbIo            Return the UsbIo protocol.

  @retval EFI_SUCCESS          Yes, UsbIo protocl is found.
  @retval EFI_NOT_FOUND        No, UsbIo protocl is not found
  @retval Others               Other errors.

**/
EFI_STATUS
UsbNicSearchUsbIo (
  IN   EFI_DEVICE_PATH_PROTOCOL  *UsbDevicePath,
  OUT  EFI_USB_IO_PROTOCOL       **UsbIo
  )
{
  EFI_STATUS                Status;
  UINTN                     BufferSize;
  EFI_HANDLE                *HandleBuffer;
  UINT16                    Length;
  UINTN                     Index;
  CHAR16                    *DevicePathStr;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ThisDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ThisDevicePathEnd;
  EFI_DEVICE_PATH_PROTOCOL  *ThisUsbDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ThisUsbDevicePathEnd;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry.\n", __func__));
  DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "Device path on the EFI handle which has UsbIo and SNP instaleld on it.\n"));
  DevicePathStr = ConvertDevicePathToText (UsbDevicePath, FALSE, FALSE);
  if (DevicePathStr != NULL) {
    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "%s\n", DevicePathStr));
    FreePool (DevicePathStr);
  } else {
    DEBUG ((DEBUG_ERROR, "Failed to convert device path.\n"));
    return EFI_INVALID_PARAMETER;
  }

  BufferSize   = 0;
  HandleBuffer = NULL;
  *UsbIo       = NULL;
  Status       = gBS->LocateHandle (
                        ByProtocol,
                        &gEfiUsbIoProtocolGuid,
                        NULL,
                        &BufferSize,
                        NULL
                        );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "  %d UsbIo protocol instances.\n", BufferSize/sizeof (EFI_HANDLE)));
    HandleBuffer = AllocateZeroPool (BufferSize);
    if (HandleBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "    Falied to allocate buffer for the handles.\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->LocateHandle (
                    ByProtocol,
                    &gEfiUsbIoProtocolGuid,
                    NULL,
                    &BufferSize,
                    HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "  Falied to locate UsbIo protocol handles.\n"));
      FreePool (HandleBuffer);
      return Status;
    }
  } else {
    return Status;
  }

  for (Index = 0; Index < (BufferSize/sizeof (EFI_HANDLE)); Index++) {
    Status = gBS->HandleProtocol (
                    *(HandleBuffer + Index),
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&ThisDevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "Device path on #%d instance of UsbIo.\n", Index));
    DevicePathStr = ConvertDevicePathToText (ThisDevicePath, FALSE, FALSE);
    if (DevicePathStr != NULL) {
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "%s\n", DevicePathStr));
      FreePool (DevicePathStr);
    } else {
      DEBUG ((DEBUG_ERROR, "Failed to convert device path on #%d instance of UsbIo.\n", Index));
      continue;
    }

    Status = EFI_NOT_FOUND;

    // Search for the starting MSG_USB_DP node.
    ThisUsbDevicePath = UsbDevicePath;
    if ((DevicePathType (ThisUsbDevicePath) != MESSAGING_DEVICE_PATH) ||
        (DevicePathSubType (ThisUsbDevicePath) != MSG_USB_DP))
    {
      ThisUsbDevicePath = UsbNicGetNextMsgUsbDp (ThisUsbDevicePath);
      if (ThisUsbDevicePath == NULL) {
        continue;
      }
    }

    if ((DevicePathType (ThisDevicePath) != MESSAGING_DEVICE_PATH) ||
        (DevicePathSubType (ThisDevicePath) != MSG_USB_DP))
    {
      ThisDevicePath = UsbNicGetNextMsgUsbDp (ThisDevicePath);
      if (ThisDevicePath == NULL) {
        continue;
      }
    }

    // Search for the ending MSG_USB_DP node.
    ThisDevicePathEnd    = ThisDevicePath;
    ThisUsbDevicePathEnd = ThisUsbDevicePath;
    while (TRUE) {
      TempDevicePath = UsbNicGetNextMsgUsbDp (ThisDevicePathEnd);
      if (TempDevicePath == NULL) {
        break;
      }

      ThisDevicePathEnd = TempDevicePath;
    }

    while (TRUE) {
      TempDevicePath = UsbNicGetNextMsgUsbDp (ThisUsbDevicePathEnd);
      if (TempDevicePath == NULL) {
        break;
      }

      ThisUsbDevicePathEnd = TempDevicePath;
    }

    // Compare these two device paths
    Length = (UINT16)((UINTN)(UINT8 *)ThisDevicePathEnd + DevicePathNodeLength (ThisDevicePathEnd) - (UINTN)(UINT8 *)ThisDevicePath);
    if (Length != ((UINTN)(UINT8 *)ThisUsbDevicePathEnd + DevicePathNodeLength (ThisUsbDevicePathEnd) - (UINTN)(UINT8 *)ThisUsbDevicePath)) {
      continue;
    }

    if (CompareMem (
          (VOID *)ThisDevicePath,
          (VOID *)ThisUsbDevicePath,
          Length
          ) == 0)
    {
      Status = EFI_SUCCESS;
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "EFI handle with the correct UsbIo is found at #%d instance of UsbIo.\n", Index));
      break;
    }
  }

  if (Status == EFI_SUCCESS) {
    // Locate UsbIo from this handle.
    Status = gBS->HandleProtocol (
                    *(HandleBuffer + Index),
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)UsbIo
                    );
    return Status;
  }

  return EFI_NOT_FOUND;
}

/**
  This function identifies if the USB NIC has MAC address and internet
  protocol device path installed. (Only support IPv4)

  @param[in] UsbDevicePath     USB device path.

  @retval EFI_SUCCESS          Yes, this is IPv4 SNP handle
  @retval EFI_NOT_FOUND        No, this is not IPv4 SNP handle

**/
EFI_STATUS
IdentifyNetworkMessageDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *UsbDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = UsbDevicePath;
  while (TRUE) {
    DevicePath = NextDevicePathNode (DevicePath);
    if (IsDevicePathEnd (DevicePath)) {
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "MAC address device path is not found on this handle.\n"));
      break;
    }

    if ((DevicePath->Type == MESSAGING_DEVICE_PATH) && (DevicePath->SubType == MSG_MAC_ADDR_DP)) {
      DevicePath = NextDevicePathNode (DevicePath); // Advance to next device path protocol.
      if (IsDevicePathEnd (DevicePath)) {
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "IPv4 device path is not found on this handle.\n"));
        break;
      }

      if ((DevicePath->Type == MESSAGING_DEVICE_PATH) && (DevicePath->SubType == MSG_IPv4_DP)) {
        return EFI_SUCCESS;
      }

      break;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  This function identifies if the USB NIC is exposed by BMC as
  the host-BMC channel.

  @param[in] Handle          This is the EFI handle with SNP installed.
  @param[in] UsbDevicePath   USB device path.

  @retval EFI_SUCCESS          Yes, USB NIC exposed by BMC is found.
  @retval EFI_NOT_FOUND        No, USB NIC exposed by BMC is not found
                               on the existing SNP handle.
  @retval Others               Other errors.

**/
EFI_STATUS
IdentifyUsbNicBmcChannel (
  IN EFI_HANDLE                Handle,
  IN EFI_DEVICE_PATH_PROTOCOL  *UsbDevicePath
  )
{
  UINTN                            Index;
  EFI_STATUS                       Status;
  EFI_SIMPLE_NETWORK_PROTOCOL      *Snp;
  EFI_USB_IO_PROTOCOL              *UsbIo;
  HOST_INTERFACE_BMC_USB_NIC_INFO  *BmcUsbNic;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry.\n", __func__));
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **)&Snp
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "    Failed to locate SNP.\n"));
    return Status;
  }

  Status = UsbNicSearchUsbIo (UsbDevicePath, &UsbIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "    Failed to find USBIO.\n"));
    return Status;
  }

  // Get the MAC address of this SNP instance.
  BmcUsbNic = AllocateZeroPool (sizeof (HOST_INTERFACE_BMC_USB_NIC_INFO));
  if (BmcUsbNic == NULL) {
    DEBUG ((DEBUG_ERROR, "    Failed to allocate memory for HOST_INTERFACE_BMC_USB_NIC_INFO.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&BmcUsbNic->NextInstance);
  BmcUsbNic->MacAddressSize = Snp->Mode->HwAddressSize;
  BmcUsbNic->MacAddress     = AllocatePool (BmcUsbNic->MacAddressSize);
  if (BmcUsbNic->MacAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "    Failed to allocate memory for HW MAC addresss.\n"));
    FreePool (BmcUsbNic);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (
    (VOID *)BmcUsbNic->MacAddress,
    (VOID *)&Snp->Mode->CurrentAddress,
    BmcUsbNic->MacAddressSize
    );
  DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    MAC address (in size %d) for this SNP instance:\n", BmcUsbNic->MacAddressSize));
  for (Index = 0; Index < BmcUsbNic->MacAddressSize; Index++) {
    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "%02x ", *(BmcUsbNic->MacAddress + Index)));
  }

  DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "\n"));
  BmcUsbNic->ThisSnp   = Snp;
  BmcUsbNic->ThisUsbIo = UsbIo;

  Status = HostInterfaceIpmiCheckMacAddress (BmcUsbNic);
  if (Status == EFI_SUCCESS) {
    BmcUsbNic->IsExposedByBmc = TRUE;
    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    BMC exposed USB NIC is found.\n"));
  } else {
    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    BMC exposed USB NIC is not found.\n"));
  }

  InsertTailList (&mBmcUsbNic, &BmcUsbNic->NextInstance);
  return Status;
}

/**
  This function checks if the USB NIC exposed by BMC
  on each handle has SNP protocol installed on it.

  @param[in] HandleNumer    Number of handles to check.
  @param[in] HandleBuffer   Handles buffer.

  @retval EFI_SUCCESS          Yes, USB NIC exposed by BMC is found.
  @retval EFI_NOT_FOUND        No, USB NIC exposed by BMC is not found
                               on the existing SNP handle.
  @retval Others               Other errors.

**/
EFI_STATUS
CheckBmcUsbNicOnHandles (
  IN  UINTN       HandleNumer,
  IN  EFI_HANDLE  *HandleBuffer
  )
{
  UINTN                     Index;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   GotBmcUsbNic;
  CHAR16                    *DevicePathStr;

  if ((HandleNumer == 0) || (HandleBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry, #%d SNP handle\n", __func__, HandleNumer));

  GotBmcUsbNic = FALSE;
  for (Index = 0; Index < HandleNumer; Index++) {
    DEBUG ((DEBUG_MANAGEABILITY, "    Locate device path on handle 0x%08x\n", *(HandleBuffer + Index)));
    Status = gBS->HandleProtocol (
                    *(HandleBuffer + Index),
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "    Failed to locate device path on %d handle.\n", Index));
      continue;
    }

    DevicePathStr = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
    if (DevicePathStr != NULL) {
      DEBUG ((DEBUG_MANAGEABILITY, "    Device path: %s\n", DevicePathStr));
      FreePool (DevicePathStr);
    }

    // Check if this is an BMC exposed USB NIC device.
    while (TRUE) {
      if ((DevicePath->Type == MESSAGING_DEVICE_PATH) && (DevicePath->SubType == MSG_USB_DP)) {
        Status = IdentifyNetworkMessageDevicePath (DevicePath);
        if (!EFI_ERROR (Status)) {
          Status = IdentifyUsbNicBmcChannel (*(HandleBuffer + Index), DevicePath);
          if (!EFI_ERROR (Status)) {
            GotBmcUsbNic = TRUE;
          }
        }

        break; // Advance to next SNP handle.
      }

      DevicePath = NextDevicePathNode (DevicePath);
      if (IsDevicePathEnd (DevicePath)) {
        break;
      }
    }
  }

  if (GotBmcUsbNic) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_MANAGEABILITY, "No BMC USB NIC found on SNP handles\n"));
  return EFI_NOT_FOUND;
}

/**
  This function checks if the USB NIC exposed by BMC
  is already connected.

  @param[in] Registration      Locate SNP protocol from the notification
                               registeration key.
                               NULL means locate SNP protocol from the existing
                               handles.

  @retval EFI_SUCCESS          Yes, USB NIC exposed by BMC is found.
  @retval EFI_NOT_FOUND        No, USB NIC exposed by BMC is not found
                               on the existing SNP handle.
  @retval Others               Other errors.

**/
EFI_STATUS
CheckBmcUsbNic (
  VOID  *Registration
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  UINTN       BufferSize;
  EFI_HANDLE  *HandleBuffer;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry, the registration key - 0x%08x.\n", __func__, Registration));

  Handle       = NULL;
  HandleBuffer = NULL;
  Status       = EFI_SUCCESS;

  do {
    BufferSize = 0;
    Status     = gBS->LocateHandle (
                        Registration == NULL ? ByProtocol : ByRegisterNotify,
                        &gEfiSimpleNetworkProtocolGuid,
                        Registration,
                        &BufferSize,
                        NULL
                        );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    %d SNP protocol instance(s).\n", BufferSize/sizeof (EFI_HANDLE)));
      HandleBuffer = AllocateZeroPool (BufferSize);
      if (HandleBuffer == NULL) {
        DEBUG ((DEBUG_ERROR, "    Falied to allocate buffer for the handles.\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      Status = gBS->LocateHandle (
                      Registration == NULL ? ByProtocol : ByRegisterNotify,
                      &gEfiSimpleNetworkProtocolGuid,
                      Registration,
                      &BufferSize,
                      HandleBuffer
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "    Falied to locate SNP protocol handles.\n"));
        FreePool (HandleBuffer);
        return Status;
      }
    } else if (EFI_ERROR (Status)) {
      if (Registration != NULL) {
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    No more newly installed SNP protocol for this registration - %r.\n", Status));
        return EFI_SUCCESS;
      }

      return Status;
    }

    // Check USB NIC on handles.
    Status = CheckBmcUsbNicOnHandles (BufferSize/sizeof (EFI_HANDLE), HandleBuffer);
    if (!EFI_ERROR (Status)) {
      // Retrieve the rest of BMC USB NIC information for Redfish over IP information
      // and USB Network Interface V2.
      Status = RetrievedBmcUsbNicInfo ();
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "    Install protocol to notify the platform Redfish Host Interface information is ready.\n"));
        Status = gBS->InstallProtocolInterface (
                        &Handle,
                        &mPlatformHostInterfaceBmcUsbNicReadinessGuid,
                        EFI_NATIVE_INTERFACE,
                        NULL
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "    Install protocol fail %r.\n", Status));
        }
      }
    }

    FreePool (HandleBuffer);
  } while (Registration != NULL);

  return Status;
}

/**
  Notification event of SNP readiness.

  @param[in]  Event                 Event whose notification function is being invoked.
  @param[in]  Context               The pointer to the notification function's context,
                                    which is implementation-dependent.

**/
VOID
EFIAPI
PlatformHostInterfaceSnpCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry.\n", __func__));

  CheckBmcUsbNic (mPlatformHostInterfaceSnpRegistration);
  return;
}

/**
  Get the EFI protocol GUID installed by platform library which
  indicates the necessary information is ready for building
  SMBIOS 42h record.

  @param[out] InformationReadinessGuid  Pointer to retrive the protocol
                                        GUID.

  @retval EFI_SUCCESS          Notification is required for building up
                               SMBIOS type 42h record.
  @retval EFI_UNSUPPORTED      Notification is not required for building up
                               SMBIOS type 42h record.
  @retval EFI_ALREADY_STARTED  Platform host information is already ready.
  @retval Others               Other errors.
**/
EFI_STATUS
RedfishPlatformHostInterfaceNotification (
  OUT EFI_GUID  **InformationReadinessGuid
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: Entry\n", __func__));

  *InformationReadinessGuid = NULL;
  InitializeListHead (&mBmcUsbNic);
  InitializeListHead (&mBmcIpmiLan);

  //
  // Check if USB NIC exposed by BMC is already
  // connected.
  //
  Status = CheckBmcUsbNic (NULL);
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_REDFISH_HOST_INTERFACE, "%a: BMC USB NIC is not found. Register the notification.\n", __func__));

    // Register the notification of SNP installation.
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    PlatformHostInterfaceSnpCallback,
                    NULL,
                    &mPlatformHostInterfaceSnpEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to create event for the installation of SNP protocol.", __func__));
      return Status;
    }

    Status = gBS->RegisterProtocolNotify (
                    &gEfiSimpleNetworkProtocolGuid,
                    mPlatformHostInterfaceSnpEvent,
                    &mPlatformHostInterfaceSnpRegistration
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to register event for the installation of SNP protocol.", __func__));
      return Status;
    }

    *InformationReadinessGuid = &mPlatformHostInterfaceBmcUsbNicReadinessGuid;
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_ERROR, "%a: Something wrong when look for BMC USB NIC.\n", __func__));
  return Status;
}
