/** @file
  This file contains code for USB Network Control Model
  binding driver

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UsbCdcNcm.h"

EFI_DRIVER_BINDING_PROTOCOL  gUsbNcmDriverBinding =3D {
  UsbNcmDriverSupported,
  UsbNcmDriverStart,
  UsbNcmDriverStop,
  USB_NCM_DRIVER_VERSION,
  NULL,
  NULL
};

/**
  Check if this interface is USB NCM SubType

  @param[in]  UsbIo   A pointer to the EFI_USB_IO_PROTOCOL instance.

  @retval TRUE        USB NCM SubType.
  @retval FALSE       Not USB NCM SubType.

**/
BOOLEAN
IsSupportedDevice (
  IN EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  Status =3D UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((InterfaceDescriptor.InterfaceClass =3D=3D USB_CDC_CLASS) &&
      (InterfaceDescriptor.InterfaceSubClass =3D=3D USB_CDC_NCM_SUBCLASS) &&
      (InterfaceDescriptor.InterfaceProtocol =3D=3D USB_NO_CLASS_PROTOCOL))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  USB NCM Driver Binding Support.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  ControllerHandle      Handle of device to test.
  @param[in]  RemainingDevicePath   Optional parameter use to pick a specific child
                                    device to start.

  @retval EFI_SUCCESS               This driver supports this device.
  @retval EFI_ALREADY_STARTED       This driver is already running on this device.
  @retval other                     This driver does not support this device.

**/
EFI_STATUS
EFIAPI
UsbNcmDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_USB_IO_PROTOCOL  *UsbIo;

  Status =3D gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status =3D IsSupportedDevice (UsbIo) ? EFI_SUCCESS : EFI_UNSUPPORTED;

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

/**
  Check if the USB NCM and USB CDC Data interfaces are from the same device.

  @param[in]  UsbEthPath                  A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.
  @param[in]  UsbCdcDataPath              A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.

  @retval EFI_SUCCESS                     Is the same device.
  @retval EFI_NOT_FOUND                   Is not the same device.

**/
EFI_STATUS
IsSameDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL  *UsbEthPath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *UsbCdcDataPath
  )
{
  while (1) {
    if ((UsbEthPath->Type =3D=3D ACPI_DEVICE_PATH) && (UsbEthPath->SubType =3D=3D ACPI_DP)) {
      if (CompareMem ((ACPI_HID_DEVICE_PATH *)UsbCdcDataPath, (ACPI_HID_DEVICE_PATH *)UsbEthPath, sizeof (ACPI_HID_DEVICE_PATH))) {
        return EFI_NOT_FOUND;
      }
    }

    if ((UsbEthPath->Type =3D=3D HARDWARE_DEVICE_PATH) && (UsbEthPath->SubType =3D=3D HW_PCI_DP)) {
      if (CompareMem ((PCI_DEVICE_PATH *)UsbCdcDataPath, (PCI_DEVICE_PATH *)UsbEthPath, sizeof (PCI_DEVICE_PATH))) {
        return EFI_NOT_FOUND;
      }
    }

    if ((UsbEthPath->Type =3D=3D MESSAGING_DEVICE_PATH) && (UsbEthPath->SubType =3D=3D MSG_USB_DP)) {
      if (IsDevicePathEnd (NextDevicePathNode (UsbEthPath))) {
        if (((USB_DEVICE_PATH *)UsbEthPath)->ParentPortNumber =3D=3D
            ((USB_DEVICE_PATH *)UsbCdcDataPath)->ParentPortNumber)
        {
          return EFI_SUCCESS;
        } else {
          return EFI_NOT_FOUND;
        }
      } else {
        if (CompareMem ((USB_DEVICE_PATH *)UsbCdcDataPath, (USB_DEVICE_PATH *)UsbEthPath, sizeof (USB_DEVICE_PATH))) {
          return EFI_NOT_FOUND;
        }
      }
    }

    UsbEthPath     =3D NextDevicePathNode (UsbEthPath);
    UsbCdcDataPath =3D NextDevicePathNode (UsbCdcDataPath);
  }
}

/**
  Check if the USB CDC Data(UsbIo) installed and return USB CDC Data Handle.

  @param[in]      UsbEthPath          A pointer to the EFI_DEVICE_PATH_PROTOCOL instance.
  @param[in, out] UsbCdcDataHandle    A pointer to the EFI_HANDLE for USB CDC Data.

  @retval TRUE                USB CDC Data(UsbIo) installed.
  @retval FALSE               USB CDC Data(UsbIo) did not installed.

**/
BOOLEAN
IsUsbCdcData (
  IN      EFI_DEVICE_PATH_PROTOCOL  *UsbEthPath,
  IN OUT  EFI_HANDLE                *UsbCdcDataHandle
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_DEVICE_PATH_PROTOCOL      *UsbCdcDataPath;

  Status =3D gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  for (Index =3D 0; Index < HandleCount; Index++) {
    Status =3D gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    ASSERT_EFI_ERROR (Status);

    Status =3D UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    ASSERT_EFI_ERROR (Status);

    if ((Interface.InterfaceClass =3D=3D USB_CDC_DATA_CLASS) &&
        (Interface.InterfaceSubClass =3D=3D USB_CDC_DATA_SUBCLASS) &&
        (Interface.InterfaceProtocol =3D=3D USB_NCM_NTB_PROTOCOL))
    {
      Status =3D gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&UsbCdcDataPath
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status =3D IsSameDevice (UsbEthPath, UsbCdcDataPath);
      if (!EFI_ERROR (Status)) {
        CopyMem (UsbCdcDataHandle, &HandleBuffer[Index], sizeof (EFI_HANDLE));
        FreePool (HandleBuffer);
        return TRUE;
      }
    }
  }

  FreePool (HandleBuffer);
  return FALSE;
}

/**
  Call Back Function.

  @param[in]  Event       Event whose notification function is being invoked.
  @param[in]  Context     The pointer to the notification function's context,
                          which is implementation-dependent.

**/
VOID
EFIAPI
CallbackFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;

  Status =3D gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index =3D 0; Index < HandleCount; Index++) {
    Status =3D gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    ASSERT_EFI_ERROR (Status);

    Status =3D UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    ASSERT_EFI_ERROR (Status);

    if ((Interface.InterfaceClass =3D=3D USB_CDC_CLASS) &&
        (Interface.InterfaceSubClass =3D=3D USB_CDC_NCM_SUBCLASS) &&
        (Interface.InterfaceProtocol =3D=3D USB_NO_CLASS_PROTOCOL))
    {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }
  }

  FreePool (HandleBuffer);
  gBS->CloseEvent (Event);
}

/**
  USB NCM Driver Binding Start.

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
UsbNcmDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  VOID                          *Reg;
  EFI_EVENT                     Event;
  USB_ETHERNET_DRIVER           *UsbEthDriver;
  EFI_DEVICE_PATH_PROTOCOL      *UsbEthPath;
  EFI_HANDLE                    UsbCdcDataHandle;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;

  Status =3D gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo,
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
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return Status;
  }

  Status =3D IsUsbCdcData (UsbEthPath, &UsbCdcDataHandle) ? EFI_SUCCESS : EFI_UNSUPPORTED;
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    Status =3D gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, CallbackFunction, NULL, &Event);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status =3D gBS->RegisterProtocolNotify (&gEfiUsbIoProtocolGuid, Event, &Reg);
    return Status;
  }

  UsbEthDriver =3D AllocateZeroPool (sizeof (USB_ETHERNET_DRIVER));
  if (!UsbEthDriver) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return EFI_OUT_OF_RESOURCES;
  }

  Status =3D LoadAllDescriptor (UsbIo, &UsbEthDriver->Config);
  ASSERT_EFI_ERROR (Status);

  GetEndpoint (UsbIo, UsbEthDriver);

  Status =3D UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
  ASSERT_EFI_ERROR (Status);

  UsbEthDriver->Signature                          =3D USB_ETHERNET_SIGNATURE;
  UsbEthDriver->NumOfInterface                     =3D Interface.InterfaceNumber;
  UsbEthDriver->UsbCdcDataHandle                   =3D UsbCdcDataHandle;
  UsbEthDriver->UsbIo                              =3D UsbIo;
  UsbEthDriver->UsbEth.UsbEthReceive               =3D UsbEthReceive;
  UsbEthDriver->UsbEth.UsbEthTransmit              =3D UsbEthTransmit;
  UsbEthDriver->UsbEth.UsbEthInterrupt             =3D UsbEthInterrupt;
  UsbEthDriver->UsbEth.UsbEthMacAddress            =3D GetUsbEthMacAddress;
  UsbEthDriver->UsbEth.UsbEthMaxBulkSize           =3D UsbEthBulkSize;
  UsbEthDriver->UsbEth.UsbHeaderFunDescriptor      =3D GetUsbHeaderFunDescriptor;
  UsbEthDriver->UsbEth.UsbUnionFunDescriptor       =3D GetUsbUnionFunDescriptor;
  UsbEthDriver->UsbEth.UsbEthFunDescriptor         =3D GetUsbEthFunDescriptor;
  UsbEthDriver->UsbEth.SetUsbEthMcastFilter        =3D SetUsbEthMcastFilter;
  UsbEthDriver->UsbEth.SetUsbEthPowerPatternFilter =3D SetUsbEthPowerFilter;
  UsbEthDriver->UsbEth.GetUsbEthPowerPatternFilter =3D GetUsbEthPowerFilter;
  UsbEthDriver->UsbEth.SetUsbEthPacketFilter       =3D SetUsbEthPacketFilter;
  UsbEthDriver->UsbEth.GetUsbEthStatistic          =3D GetUsbEthStatistic;

  UsbEthDriver->BulkBuffer =3D AllocateZeroPool (USB_NCM_MAX_NTB_SIZE);

  Status =3D gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &(UsbEthDriver->UsbEth)
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    FreePool (UsbEthDriver);
    return Status;
  }

  return Status;
}

/**
  USB NCM Driver Binding Stop.

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
UsbNcmDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  EDKII_USB_ETHERNET_PROTOCOL  *UsbEthProtocol;
  USB_ETHERNET_DRIVER          *UsbEthDriver;

  Status =3D gBS->OpenProtocol (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  (VOID **)&UsbEthProtocol,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UsbEthDriver =3D USB_ETHERNET_DEV_FROM_THIS (UsbEthProtocol);

  Status =3D gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  UsbEthProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status =3D gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiUsbIoProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );
  FreePool (UsbEthDriver->Config);
  FreePool (UsbEthDriver->BulkBuffer);
  FreePool (UsbEthDriver);
  return Status;
}

/**
  Entrypoint of NCM Driver.

  This function is the entrypoint of NCM Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
UsbNcmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gUsbNcmDriverBinding.DriverBindingHandle =3D ImageHandle;
  gUsbNcmDriverBinding.ImageHandle         =3D ImageHandle;

  return gBS->InstallMultipleProtocolInterfaces (
                &gUsbNcmDriverBinding.DriverBindingHandle,
                &gEfiDriverBindingProtocolGuid,
                &gUsbNcmDriverBinding,
                &gEfiComponentName2ProtocolGuid,
                &gUsbNcmComponentName2,
                NULL
                );
}
