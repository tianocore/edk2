/** @file
  This file contains code for USB Network Control Model
  binding driver

  Copyright (c) 2023, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "UsbCdcNcm.h"

EFI_DRIVER_BINDING_PROTOCOL  gUsbNcmDriverBinding = {
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

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &InterfaceDescriptor);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((InterfaceDescriptor.InterfaceClass == USB_CDC_CLASS) &&
      (InterfaceDescriptor.InterfaceSubClass == USB_CDC_NCM_SUBCLASS) &&
      (InterfaceDescriptor.InterfaceProtocol == USB_NO_CLASS_PROTOCOL))
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

  Status = gBS->OpenProtocol (
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

  Status = IsSupportedDevice (UsbIo) ? EFI_SUCCESS : EFI_UNSUPPORTED;

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
    if ((UsbEthPath->Type == ACPI_DEVICE_PATH) && (UsbEthPath->SubType == ACPI_DP)) {
      if (CompareMem ((ACPI_HID_DEVICE_PATH *)UsbCdcDataPath, (ACPI_HID_DEVICE_PATH *)UsbEthPath, sizeof (ACPI_HID_DEVICE_PATH))) {
        return EFI_NOT_FOUND;
      }
    }

    if ((UsbEthPath->Type == HARDWARE_DEVICE_PATH) && (UsbEthPath->SubType == HW_PCI_DP)) {
      if (CompareMem ((PCI_DEVICE_PATH *)UsbCdcDataPath, (PCI_DEVICE_PATH *)UsbEthPath, sizeof (PCI_DEVICE_PATH))) {
        return EFI_NOT_FOUND;
      }
    }

    if ((UsbEthPath->Type == MESSAGING_DEVICE_PATH) && (UsbEthPath->SubType == MSG_USB_DP)) {
      if (IsDevicePathEnd (NextDevicePathNode (UsbEthPath))) {
        if (((USB_DEVICE_PATH *)UsbEthPath)->ParentPortNumber ==
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

    UsbEthPath     = NextDevicePathNode (UsbEthPath);
    UsbCdcDataPath = NextDevicePathNode (UsbCdcDataPath);
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

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    ASSERT_EFI_ERROR (Status);

    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    ASSERT_EFI_ERROR (Status);

    if ((Interface.InterfaceClass == USB_CDC_DATA_CLASS) &&
        (Interface.InterfaceSubClass == USB_CDC_DATA_SUBCLASS) &&
        (Interface.InterfaceProtocol == USB_NCM_NTB_PROTOCOL))
    {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&UsbCdcDataPath
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = IsSameDevice (UsbEthPath, UsbCdcDataPath);
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

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUsbIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **)&UsbIo
                    );
    ASSERT_EFI_ERROR (Status);

    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
    ASSERT_EFI_ERROR (Status);

    if ((Interface.InterfaceClass == USB_CDC_CLASS) &&
        (Interface.InterfaceSubClass == USB_CDC_NCM_SUBCLASS) &&
        (Interface.InterfaceProtocol == USB_NO_CLASS_PROTOCOL))
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

  Status = gBS->OpenProtocol (
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

  Status = gBS->OpenProtocol (
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

  Status = IsUsbCdcData (UsbEthPath, &UsbCdcDataHandle) ? EFI_SUCCESS : EFI_UNSUPPORTED;
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, CallbackFunction, NULL, &Event);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->RegisterProtocolNotify (&gEfiUsbIoProtocolGuid, Event, &Reg);
    return Status;
  }

  UsbEthDriver = AllocateZeroPool (sizeof (USB_ETHERNET_DRIVER));
  if (!UsbEthDriver) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiUsbIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return EFI_OUT_OF_RESOURCES;
  }

  Status = LoadAllDescriptor (UsbIo, &UsbEthDriver->Config);
  ASSERT_EFI_ERROR (Status);

  GetEndpoint (UsbIo, UsbEthDriver);

  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &Interface);
  ASSERT_EFI_ERROR (Status);

  UsbEthDriver->Signature                          = USB_ETHERNET_SIGNATURE;
  UsbEthDriver->NumOfInterface                     = Interface.InterfaceNumber;
  UsbEthDriver->UsbCdcDataHandle                   = UsbCdcDataHandle;
  UsbEthDriver->UsbIo                              = UsbIo;
  UsbEthDriver->UsbEth.UsbEthReceive               = UsbEthNcmReceive;
  UsbEthDriver->UsbEth.UsbEthTransmit              = UsbEthNcmTransmit;
  UsbEthDriver->UsbEth.UsbEthInterrupt             = UsbEthNcmInterrupt;
  UsbEthDriver->UsbEth.UsbEthMacAddress            = GetUsbEthMacAddress;
  UsbEthDriver->UsbEth.UsbEthMaxBulkSize           = UsbEthNcmBulkSize;
  UsbEthDriver->UsbEth.UsbHeaderFunDescriptor      = GetUsbHeaderFunDescriptor;
  UsbEthDriver->UsbEth.UsbUnionFunDescriptor       = GetUsbUnionFunDescriptor;
  UsbEthDriver->UsbEth.UsbEthFunDescriptor         = GetUsbEthFunDescriptor;
  UsbEthDriver->UsbEth.SetUsbEthMcastFilter        = SetUsbEthMcastFilter;
  UsbEthDriver->UsbEth.SetUsbEthPowerPatternFilter = SetUsbEthPowerFilter;
  UsbEthDriver->UsbEth.GetUsbEthPowerPatternFilter = GetUsbEthPowerFilter;
  UsbEthDriver->UsbEth.SetUsbEthPacketFilter       = SetUsbEthPacketFilter;
  UsbEthDriver->UsbEth.GetUsbEthStatistic          = GetUsbEthStatistic;

  UsbEthDriver->BulkBuffer = AllocateZeroPool (USB_NCM_MAX_NTB_SIZE);

  Status = gBS->InstallProtocolInterface (
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

  Status = gBS->OpenProtocol (
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

  UsbEthDriver = USB_ETHERNET_DEV_FROM_THIS (UsbEthProtocol);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEdkIIUsbEthProtocolGuid,
                  UsbEthProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
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
  gUsbNcmDriverBinding.DriverBindingHandle = ImageHandle;
  gUsbNcmDriverBinding.ImageHandle         = ImageHandle;

  return gBS->InstallMultipleProtocolInterfaces (
                &gUsbNcmDriverBinding.DriverBindingHandle,
                &gEfiDriverBindingProtocolGuid,
                &gUsbNcmDriverBinding,
                &gEfiComponentName2ProtocolGuid,
                &gUsbNcmComponentName2,
                NULL
                );
}
