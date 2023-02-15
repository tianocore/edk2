/** @file
  This file contains code for USB network binding driver

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "DriverBinding.h"

PXE_SW_UNDI  *gPxe =3D NULL;
NIC_DEVICE   *gLanDeviceList[MAX_LAN_INTERFACE];

EFI_DRIVER_BINDING_PROTOCOL  gNetworkCommonDriverBinding =3D {
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

  MacAddrNode.Header.Type      =3D MESSAGING_DEVICE_PATH;
  MacAddrNode.Header.SubType   =3D MSG_MAC_ADDR_DP;
  MacAddrNode.Header.Length[0] =3D (UINT8)sizeof (MacAddrNode);
  MacAddrNode.Header.Length[1] =3D 0;

  EndNode =3D BaseDev;

  while (!IsDevicePathEnd (EndNode)) {
    EndNode =3D NextDevicePathNode (EndNode);
  }

  BaseLength  =3D (UINT16)((UINTN)(EndNode) - (UINTN)(BaseDev));
  TotalLength =3D (UINT16)(BaseLength + sizeof (MacAddrNode) + sizeof (EFI_DEVICE_PATH_PROTOCOL));

  Status =3D gBS->AllocatePool (EfiBootServicesData, TotalLength, (VOID **)&DevicePath);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Dev =3D (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
  CopyMem (DevicePath, (CHAR8 *)BaseDev, BaseLength);
  DevicePath +=3D BaseLength;
  CopyMem (DevicePath, (CHAR8 *)&MacAddrNode, sizeof (MacAddrNode));
  DevicePath +=3D sizeof (MacAddrNode);
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

  Status =3D gBS->OpenProtocol (
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

  Status =3D gBS->OpenProtocol (
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

  Status =3D gBS->OpenProtocol (
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

  Status =3D UsbEth->UsbEthMacAddress (UsbEth, &MacAddress);
  ASSERT_EFI_ERROR (Status);
  Status =3D UsbEth->UsbEthMaxBulkSize (UsbEth, &BulkDataSize);

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

  NicDevice =3D AllocateZeroPool (sizeof (NIC_DEVICE) + BulkDataSize + 4096);
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
  if (gPxe =3D=3D NULL) {
    TmpPxePointer =3D NULL;
    TmpPxePointer =3D AllocateZeroPool (sizeof (PXE_SW_UNDI) + 16);
    if (!TmpPxePointer) {
      if (NicDevice !=3D NULL) {
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
      if (((UINTN)TmpPxePointer & 0x0F) !=3D 0) {
        gPxe =3D (PXE_SW_UNDI *)(TmpPxePointer + 8);
      } else {
        gPxe =3D (PXE_SW_UNDI *)TmpPxePointer;
      }

      if (!gPxe) {
        if (NicDevice !=3D NULL) {
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

  NicDevice->NiiProtocol.Id    =3D (UINT64)(UINTN)(gPxe);
  NicDevice->NiiProtocol.IfNum =3D gPxe->IFcnt | gPxe->IFcntExt << 8;

  UpdateNicNum (&NicDevice->NicInfo, gPxe);

  NicDevice->NicInfo.Signature =3D NIC_DATA_SIGNATURE;

  NicDevice->NicInfo.UsbEth         =3D UsbEth;
  NicDevice->NicInfo.MaxSegmentSize =3D (UINT16)BulkDataSize;
  NicDevice->NicInfo.CableDetect    =3D 0;
  NicDevice->ReceiveBuffer          =3D ALIGN_POINTER ((VOID *)NicDevice, 4096);

  CopyMem ((CHAR8 *)&(NicDevice->NicInfo.MacAddr), (CHAR8 *)&MacAddress, sizeof (MacAddress));

  NicDevice->NicInfo.TxBufferCount =3D 0;

  if (NicDevice->NiiProtocol.IfNum < MAX_LAN_INTERFACE) {
    gLanDeviceList[NicDevice->NiiProtocol.IfNum] =3D NicDevice;
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

    if (TmpPxePointer !=3D NULL) {
      FreePool (TmpPxePointer);
    }

    if (NicDevice !=3D NULL) {
      FreePool (NicDevice);
    }

    return EFI_DEVICE_ERROR;
  }

  Status =3D CreateMacDevicePath (
             &NicDevice->DevPath,
             UsbEthPath,
             &NicDevice->NicInfo
             );

  if (EFI_ERROR (Status)) {
    UpdateNicNum (NULL, gPxe);
    if (TmpPxePointer !=3D NULL) {
      FreePool (TmpPxePointer);
    }
  }

  NicDevice->Signature                 =3D UNDI_DEV_SIGNATURE;
  NicDevice->NiiProtocol.Revision      =3D EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION;
  NicDevice->NiiProtocol.Type          =3D EfiNetworkInterfaceUndi;
  NicDevice->NiiProtocol.MajorVer      =3D PXE_ROMID_MAJORVER;
  NicDevice->NiiProtocol.MinorVer      =3D PXE_ROMID_MINORVER;
  NicDevice->NiiProtocol.ImageSize     =3D 0;
  NicDevice->NiiProtocol.ImageAddr     =3D 0;
  NicDevice->NiiProtocol.Ipv6Supported =3D TRUE;

  NicDevice->NiiProtocol.StringId[0] =3D 'U';
  NicDevice->NiiProtocol.StringId[1] =3D 'N';
  NicDevice->NiiProtocol.StringId[2] =3D 'D';
  NicDevice->NiiProtocol.StringId[3] =3D 'I';
  NicDevice->DeviceHandle            =3D NULL;

  NicDevice->NicInfo.RateLimitingEnable      =3D PcdGetBool (EnableRateLimiting);
  NicDevice->NicInfo.RateLimitingCreditCount =3D 0;
  NicDevice->NicInfo.RateLimitingCredit      =3D PcdGet32 (RateLimitingCredit);
  NicDevice->NicInfo.RateLimiter             =3D NULL;

  ZeroMem (&NicDevice->NicInfo.Request, sizeof (EFI_USB_DEVICE_REQUEST));

  Status =3D UsbEth->UsbEthInterrupt (UsbEth, TRUE, NETWORK_COMMON_POLLING_INTERVAL, &NicDevice->NicInfo.Request);
  ASSERT_EFI_ERROR (Status);

  Status =3D gBS->InstallMultipleProtocolInterfaces (
                  &NicDevice->DeviceHandle,
                  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                  &NicDevice->NiiProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NicDevice->DevPath,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    if (NicDevice->NiiProtocol.IfNum < MAX_LAN_INTERFACE) {
      gLanDeviceList[NicDevice->NiiProtocol.IfNum] =3D NULL;
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

    if (TmpPxePointer !=3D NULL) {
      FreePool (TmpPxePointer);
    }

    if (NicDevice->DevPath !=3D NULL) {
      FreePool (NicDevice->DevPath);
    }

    if (NicDevice !=3D NULL) {
      FreePool (NicDevice);
    }

    return EFI_DEVICE_ERROR;
  }

  Status =3D gBS->OpenProtocol (
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

  if (NumberOfChildren =3D=3D 0) {
    Status =3D gBS->OpenProtocol (
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

    NicDevice =3D UNDI_DEV_FROM_THIS (NiiProtocol);
    Status    =3D gBS->UninstallMultipleProtocolInterfaces (
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

  AllChildrenStopped =3D TRUE;

  for (Index =3D 0; Index < NumberOfChildren; Index++) {
    Status =3D gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    (VOID **)&NiiProtocol,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      AllChildrenStopped =3D FALSE;
      continue;
    }

    NicDevice =3D UNDI_DEV_FROM_THIS (NiiProtocol);

    gBS->CloseProtocol (
           ControllerHandle,
           &gEdkIIUsbEthProtocolGuid,
           This->DriverBindingHandle,
           ChildHandleBuffer[Index]
           );

    Status =3D gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandleBuffer[Index],
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    &NicDevice->NiiProtocol,
                    &gEfiDevicePathProtocolGuid,
                    NicDevice->DevPath,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      Status =3D gBS->OpenProtocol (
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

  gNetworkCommonDriverBinding.DriverBindingHandle =3D ImageHandle;
  gNetworkCommonDriverBinding.ImageHandle         =3D ImageHandle;

  Status =3D gBS->InstallMultipleProtocolInterfaces (
                  &gNetworkCommonDriverBinding.DriverBindingHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gNetworkCommonDriverBinding,
                  &gEfiComponentName2ProtocolGuid,
                  &gNetworkCommonComponentName2,
                  NULL
                  );
  return Status;
}
