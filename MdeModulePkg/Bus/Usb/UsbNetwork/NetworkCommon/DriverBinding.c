/** @file
  This file contains code for USB network binding driver

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DriverBinding.h"

PXE_SW_UNDI  *gPxe = NULL;
NIC_DEVICE   *gLanDeviceList[MAX_LAN_INTERFACE];
UINT32       gRateLimitingCredit;
UINT32       gRateLimitingPollTimer;
BOOLEAN      gRateLimitingEnable;

EFI_DRIVER_BINDING_PROTOCOL  gNetworkCommonDriverBinding = {
  NetworkCommonSupported,
  NetworkCommonDriverStart,
  NetworkCommonDriverStop,
  NETWORK_COMMON_DRIVER_VERSION,
  NULL,
  NULL
};

/**
  Create MAC Device Path

  @param[in, out] Dev             A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.
  @param[in]      BaseDev         A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.
  @param[in]      Nic             A pointer to the Network interface controller data.

  @retval EFI_OUT_OF_RESOURCES    The device path could not be created successfully due to a lack of resources.
  @retval EFI_SUCCESS             MAC device path created successfully.

**/
EFI_STATUS
CreateMacDevicePath (
  IN OUT  EFI_DEVICE_PATH_PROTOCOL  **Dev,
  IN      EFI_DEVICE_PATH_PROTOCOL  *BaseDev,
  IN      NIC_DATA                  *Nic
  )
{
  EFI_STATUS                Status;
  MAC_ADDR_DEVICE_PATH      MacAddrNode;
  EFI_DEVICE_PATH_PROTOCOL  *EndNode;
  UINT8                     *DevicePath;
  UINT16                    TotalLength;
  UINT16                    BaseLength;

  ZeroMem (&MacAddrNode, sizeof (MAC_ADDR_DEVICE_PATH));
  CopyMem (&MacAddrNode.MacAddress, &Nic->MacAddr, sizeof (EFI_MAC_ADDRESS));

  MacAddrNode.Header.Type      = MESSAGING_DEVICE_PATH;
  MacAddrNode.Header.SubType   = MSG_MAC_ADDR_DP;
  MacAddrNode.Header.Length[0] = (UINT8)sizeof (MacAddrNode);
  MacAddrNode.Header.Length[1] = 0;

  EndNode = BaseDev;

  while (!IsDevicePathEnd (EndNode)) {
    EndNode = NextDevicePathNode (EndNode);
  }

  BaseLength  = (UINT16)((UINTN)(EndNode) - (UINTN)(BaseDev));
  TotalLength = (UINT16)(BaseLength + sizeof (MacAddrNode) + sizeof (EFI_DEVICE_PATH_PROTOCOL));

  Status = gBS->AllocatePool (EfiBootServicesData, TotalLength, (VOID **)&DevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Dev = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
  CopyMem (DevicePath, (CHAR8 *)BaseDev, BaseLength);
  DevicePath += BaseLength;
  CopyMem (DevicePath, (CHAR8 *)&MacAddrNode, sizeof (MacAddrNode));
  DevicePath += sizeof (MacAddrNode);
  CopyMem (DevicePath, (CHAR8 *)EndNode, sizeof (EFI_DEVICE_PATH_PROTOCOL));

  return EFI_SUCCESS;
}

/**
  Network Common Driver Binding Support.

  @param[in]  This                    Protocol instance pointer.
  @param[in]  ControllerHandle        Handle of device to test.
  @param[in]  RemainingDevicePath     Optional parameter use to pick a specific child
                                      device to start.

  @retval EFI_SUCCESS                 This driver supports this device.
  @retval EFI_ALREADY_STARTED         This driver is already running on this device.
  @retval other                       This driver does not support this device.

**/
EFI_STATUS
EFIAPI
NetworkCommonSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEth;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  (VOID **)&UsbEth,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         ControllerHandle,
         &gEdkIIUsbEthProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

/**
  Network Common Driver Binding Start.

  @param[in]  This                    Protocol instance pointer.
  @param[in]  ControllerHandle        Handle of device to bind driver to.
  @param[in]  RemainingDevicePath     Optional parameter use to pick a specific child
                                      device to start.

  @retval EFI_SUCCESS                 This driver is added to ControllerHandle
  @retval EFI_DEVICE_ERROR            This driver could not be started due to a device error
  @retval EFI_OUT_OF_RESOURCES        The driver could not install successfully due to a lack of resources.
  @retval other                       This driver does not support this device

**/
EFI_STATUS
EFIAPI
NetworkCommonDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_DEVICE_PATH_PROTOCOL     *UsbEthPath;
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEth;
  EFI_MAC_ADDRESS              MacAddress;
  UINTN                        BulkDataSize;
  NIC_DEVICE                   *NicDevice;
  UINT8                        *TmpPxePointer;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  (VOID **)&UsbEth,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&UsbEthPath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return Status;
  }

  ZeroMem (&MacAddress, sizeof (EFI_MAC_ADDRESS));

  Status = UsbEth->UsbEthMacAddress (UsbEth, &MacAddress);
  ASSERT_EFI_ERROR (Status);
  Status = UsbEth->UsbEthMaxBulkSize (UsbEth, &BulkDataSize);

  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return Status;
  }

  NicDevice = AllocateZeroPool (sizeof (NIC_DEVICE) + BulkDataSize + 4096);
  if (!NicDevice) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return EFI_OUT_OF_RESOURCES;
  }

  // for alignment adjustment
  if (gPxe == NULL) {
    TmpPxePointer = NULL;
    TmpPxePointer = AllocateZeroPool (sizeof (PXE_SW_UNDI) + 16);
    if (!TmpPxePointer) {
      if (NicDevice != NULL) {
        FreePool (NicDevice);
      }

      gBS->CloseProtocol (
             ControllerHandle,
             &gEfiDevicePathProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );
      gBS->CloseProtocol (
             ControllerHandle,
             &gEdkIIUsbEthProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );

      return EFI_OUT_OF_RESOURCES;
    } else {
      // check for paragraph alignment here
      if (((UINTN)TmpPxePointer & 0x0F) != 0) {
        gPxe = (PXE_SW_UNDI *)(TmpPxePointer + 8);
      } else {
        gPxe = (PXE_SW_UNDI *)TmpPxePointer;
      }

      if (gPxe == NULL) {
        if (NicDevice != NULL) {
          FreePool (NicDevice);
        }

        gBS->CloseProtocol (
               ControllerHandle,
               &gEfiDevicePathProtocolGuid,
               This->DriverBindingHandle,
               ControllerHandle
               );
        gBS->CloseProtocol (
               ControllerHandle,
               &gEdkIIUsbEthProtocolGuid,
               This->DriverBindingHandle,
               ControllerHandle
               );
        return EFI_OUT_OF_RESOURCES;
      }

      PxeStructInit (gPxe);
    }
  }

  NicDevice->NiiProtocol.Id    = (UINT64)(UINTN)(gPxe);
  NicDevice->NiiProtocol.IfNum = gPxe->IFcnt | gPxe->IFcntExt << 8;

  UpdateNicNum (&NicDevice->NicInfo, gPxe);

  NicDevice->NicInfo.Signature = NIC_DATA_SIGNATURE;

  NicDevice->NicInfo.UsbEth         = UsbEth;
  NicDevice->NicInfo.MaxSegmentSize = (UINT16)BulkDataSize;
  NicDevice->NicInfo.CableDetect    = 0;
  NicDevice->ReceiveBuffer          = ALIGN_POINTER ((VOID *)NicDevice, 4096);

  CopyMem ((CHAR8 *)&(NicDevice->NicInfo.MacAddr), (CHAR8 *)&MacAddress, sizeof (MacAddress));

  NicDevice->NicInfo.TxBufferCount = 0;

  if (NicDevice->NiiProtocol.IfNum < MAX_LAN_INTERFACE) {
    gLanDeviceList[NicDevice->NiiProtocol.IfNum] = NicDevice;
  } else {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    if (TmpPxePointer != NULL) {
      FreePool (TmpPxePointer);
    }

    if (NicDevice != NULL) {
      FreePool (NicDevice);
    }

    return EFI_DEVICE_ERROR;
  }

  Status = CreateMacDevicePath (
             &NicDevice->DevPath,
             UsbEthPath,
             &NicDevice->NicInfo
             );

  if (EFI_ERROR (Status)) {
    UpdateNicNum (NULL, gPxe);
    if (TmpPxePointer != NULL) {
      FreePool (TmpPxePointer);
    }
  }

  NicDevice->Signature                 = UNDI_DEV_SIGNATURE;
  NicDevice->NiiProtocol.Revision      = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION;
  NicDevice->NiiProtocol.Type          = EfiNetworkInterfaceUndi;
  NicDevice->NiiProtocol.MajorVer      = PXE_ROMID_MAJORVER;
  NicDevice->NiiProtocol.MinorVer      = PXE_ROMID_MINORVER;
  NicDevice->NiiProtocol.ImageSize     = 0;
  NicDevice->NiiProtocol.ImageAddr     = 0;
  NicDevice->NiiProtocol.Ipv6Supported = TRUE;

  NicDevice->NiiProtocol.StringId[0] = 'U';
  NicDevice->NiiProtocol.StringId[1] = 'N';
  NicDevice->NiiProtocol.StringId[2] = 'D';
  NicDevice->NiiProtocol.StringId[3] = 'I';
  NicDevice->DeviceHandle            = NULL;

  NicDevice->NicInfo.RateLimitingEnable      = gRateLimitingEnable;
  NicDevice->NicInfo.RateLimitingCreditCount = 0;
  NicDevice->NicInfo.RateLimitingCredit      = gRateLimitingCredit;
  NicDevice->NicInfo.RateLimitingPollTimer   = gRateLimitingPollTimer;
  NicDevice->NicInfo.RateLimiter             = NULL;

  ZeroMem (&NicDevice->NicInfo.Request, sizeof (EFI_USB_DEVICE_REQUEST));

  Status = UsbEth->UsbEthInterrupt (UsbEth, TRUE, NETWORK_COMMON_POLLING_INTERVAL, &NicDevice->NicInfo.Request);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NicDevice->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  &NicDevice->NiiProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NicDevice->DevPath,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    if (NicDevice->NiiProtocol.IfNum < MAX_LAN_INTERFACE) {
      gLanDeviceList[NicDevice->NiiProtocol.IfNum] = NULL;
    }

    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    if (TmpPxePointer != NULL) {
      FreePool (TmpPxePointer);
    }

    if (NicDevice->DevPath != NULL) {
      FreePool (NicDevice->DevPath);
    }

    if (NicDevice != NULL) {
      FreePool (NicDevice);
    }

    return EFI_DEVICE_ERROR;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  (VOID **)&UsbEth,
                  This->DriverBindingHandle,
                  NicDevice->DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

  return Status;
}

/**
  Network Common Driver Binding Stop.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  ControllerHandle      Handle of device to stop driver on
  @param[in]  NumberOfChildren      Number of Handles in ChildHandleBuffer. If number of
                                    children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer     List of Child Handles to Stop.

  @retval EFI_SUCCESS               This driver is removed ControllerHandle
  @retval other                     This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
NetworkCommonDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                                 Status;
  BOOLEAN                                    AllChildrenStopped;
  UINTN                                      Index;
  EDKII_USB_ETHERNET_PROTOCOL                *UsbEth;
  NIC_DEVICE                                 *NicDevice;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL  *NiiProtocol;

  if (NumberOfChildren == 0) {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    (VOID **)&NiiProtocol,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      gBS->CloseProtocol (
             ControllerHandle,
             &gEfiDevicePathProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );
      gBS->CloseProtocol (
             ControllerHandle,
             &gEdkIIUsbEthProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );
      return EFI_SUCCESS;
    }

    NicDevice = UNDI_DEV_FROM_THIS (NiiProtocol);
    Status    = gBS->UninstallMultipleProtocolInterfaces (
                       ControllerHandle,
                       &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                       &NicDevice->NiiProtocol,
                       &gEfiDevicePathProtocolGuid,
                       NicDevice->DevPath,
                       NULL
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FreePool (NicDevice->DevPath);
    FreePool (NicDevice);

    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    (VOID **)&NiiProtocol,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
      continue;
    }

    NicDevice = UNDI_DEV_FROM_THIS (NiiProtocol);

    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ChildHandleBuffer[Index]
           );

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    &NicDevice->NiiProtocol,
                    &gEfiDevicePathProtocolGuid,
                    NicDevice->DevPath,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      ControllerHandle,
                      &gEdkIIUsbEthProtocolGuid,
                      (VOID **)&UsbEth,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index],
                      EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                      );
    } else {
      FreePool (NicDevice->DevPath);
      FreePool (NicDevice);
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Entrypoint of Network Common Driver.

  This function is the entrypoint of Network Common Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
NetworkCommonEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  gNetworkCommonDriverBinding.DriverBindingHandle = ImageHandle;
  gNetworkCommonDriverBinding.ImageHandle         = ImageHandle;
  gRateLimitingEnable                             = PcdGetBool (PcdEnableUsbNetworkRateLimiting);
  gRateLimitingCredit                             = PcdGet32 (PcdUsbNetworkRateLimitingCredit);
  gRateLimitingPollTimer                          = PcdGet32 (PcdUsbNetworkRateLimitingFactor);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gNetworkCommonDriverBinding.DriverBindingHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gNetworkCommonDriverBinding,
                  &gEfiComponentName2ProtocolGuid,
                  &gNetworkCommonComponentName2,
                  NULL
                  );
  return Status;
}
