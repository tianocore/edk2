/** @file

    Usb bus enumeration support.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbBus.h"

/**
  Return the endpoint descriptor in this interface.

  @param  UsbIf                 The interface to search in.
  @param  EpAddr                The address of the endpoint to return.

  @return The endpoint descriptor or NULL.

**/
USB_ENDPOINT_DESC *
UsbGetEndpointDesc (
  IN USB_INTERFACE  *UsbIf,
  IN UINT8          EpAddr
  )
{
  USB_ENDPOINT_DESC  *EpDesc;
  UINT8              Index;
  UINT8              NumEndpoints;

  NumEndpoints = UsbIf->IfSetting->Desc.NumEndpoints;

  for (Index = 0; Index < NumEndpoints; Index++) {
    EpDesc = UsbIf->IfSetting->Endpoints[Index];

    if (EpDesc->Desc.EndpointAddress == EpAddr) {
      return EpDesc;
    }
  }

  return NULL;
}

/**
  Check if given Usb interface belongs to any Usb association.

  @param[in]  Device    The device may have the interface association descriptor.
  @param[in]  IfDesc    The interface descriptor to check for.
  @param[out] IfAssoc   The USB association device pointer.

  @retval EFI_SUCCESS   IfDesc is found within associations, IfAssoc has valid pointer.
  @retval EFI_NOT_FOUND IfDesc does no belong to any association.

**/
EFI_STATUS
GetInterfaceAssociation (
  IN USB_DEVICE          *Device,
  IN USB_INTERFACE_DESC  *IfDesc,
  OUT USB_ASSOCIATION    **IfAssoc
  )
{
  UINTN            Index;
  UINTN            i;
  USB_ASSOCIATION  *Ia;

  for (Index = 0; Index < Device->NumOfAssociation; Index++) {
    Ia = Device->Associations[Index];
    for (i = 0; i < Ia->IaDesc->Desc.InterfaceCount; i++) {
      if (IfDesc->Settings[0]->Desc.InterfaceNumber == Ia->IaDesc->Interfaces[i]->Settings[0]->Desc.InterfaceNumber) {
        *IfAssoc = Ia;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Free the resource used by USB interface.

  @param  UsbIf                 The USB interface to free.

  @retval EFI_ACCESS_DENIED     The interface is still occupied.
  @retval EFI_SUCCESS           The interface is freed.
**/
EFI_STATUS
UsbFreeInterface (
  IN USB_INTERFACE  *UsbIf
  )
{
  EFI_STATUS       Status;
  USB_ASSOCIATION  *UsbIa;

  Status = GetInterfaceAssociation (UsbIf->Device, UsbIf->IfDesc, &UsbIa);

  if (!EFI_ERROR (Status)) {
    //
    // Close USB Interface Association Protocol by Child
    //
    Status = gBS->CloseProtocol (
                    UsbIa->Handle,
                    &gEfiUsbIaProtocolGuid,
                    mUsbBusDriverBinding.DriverBindingHandle,
                    UsbIf->Handle
                    );
    DEBUG ((DEBUG_INFO, "UsbFreeInterface: close IAD protocol by child, %r\n", Status));
  } else {
    UsbCloseHostProtoByChild (UsbIf->Device->Bus, UsbIf->Handle);
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  UsbIf->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbIf->DevicePath,
                  &gEfiUsbIoProtocolGuid,
                  &UsbIf->UsbIo,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    if (UsbIf->DevicePath != NULL) {
      FreePool (UsbIf->DevicePath);
    }

    FreePool (UsbIf);
  } else {
    Status = GetInterfaceAssociation (UsbIf->Device, UsbIf->IfDesc, &UsbIa);

    if (!EFI_ERROR (Status)) {
      //
      // Reopen USB Interface Assiciation Protocol by Child
      //
      Status = gBS->OpenProtocol (
                      UsbIa->Handle,
                      &gEfiUsbIaProtocolGuid,
                      (VOID **)&UsbIa,
                      mUsbBusDriverBinding.DriverBindingHandle,
                      UsbIf->Handle,
                      EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                      );
      DEBUG ((DEBUG_INFO, "UsbFreeInterface: reopen IAD for child, Status = %r\n", Status));
    } else {
      //
      // Reopen USB Host Controller Protocol by Child
      //
      Status = UsbOpenHostProtoByChild (UsbIf->Device->Bus, UsbIf->Handle);
      DEBUG ((DEBUG_INFO, "UsbFreeInterface: reopen host controller for child, Status = %r\n", Status));
    }
  }

  return Status;
}

/**
  Free the resource used by USB association.

  @param  UsbIa                 The USB association to free.

  @retval EFI_ACCESS_DENIED     The Usb association resource is still occupied.
  @retval EFI_SUCCESS           The Usb association resource is freed.

**/
EFI_STATUS
UsbFreeAssociation (
  IN USB_ASSOCIATION  *UsbIa
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  UsbIa->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbIa->DevicePath,
                  &gEfiUsbIaProtocolGuid,
                  &UsbIa->UsbIa,
                  NULL
                  );

  if (!EFI_ERROR (Status)) {
    if (UsbIa->DevicePath != NULL) {
      FreePool (UsbIa->DevicePath);
    }

    FreePool (UsbIa);
  } else {
    UsbOpenHostProtoByChild (UsbIa->Device->Bus, UsbIa->Handle);
  }

  return Status;
}

/**
  Create an interface for the descriptor IfDesc. Each
  device's configuration can have several interfaces.
  Interface may belong to interface association.

  @param  Device                The device has the interface descriptor.
  @param  IfDesc                The interface descriptor.

  @return The created USB interface for the descriptor, or NULL.

**/
USB_INTERFACE *
UsbCreateInterface (
  IN USB_DEVICE          *Device,
  IN USB_INTERFACE_DESC  *IfDesc
  )
{
  USB_DEVICE_PATH           UsbNode;
  USB_INTERFACE             *UsbIf;
  USB_INTERFACE             *HubIf;
  USB_ASSOCIATION           *UsbIa;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathProtocol;
  EFI_STATUS                Status;

  UsbIf = AllocateZeroPool (sizeof (USB_INTERFACE));

  if (UsbIf == NULL) {
    return NULL;
  }

  UsbIf->Signature = USB_INTERFACE_SIGNATURE;
  UsbIf->Device    = Device;
  UsbIf->IfDesc    = IfDesc;
  ASSERT (IfDesc->ActiveIndex < USB_MAX_INTERFACE_SETTING);
  UsbIf->IfSetting = IfDesc->Settings[IfDesc->ActiveIndex];

  CopyMem (
    &(UsbIf->UsbIo),
    &mUsbIoProtocol,
    sizeof (EFI_USB_IO_PROTOCOL)
    );

  //
  // Install protocols for USBIO and device path
  //
  UsbNode.Header.Type      = MESSAGING_DEVICE_PATH;
  UsbNode.Header.SubType   = MSG_USB_DP;
  UsbNode.ParentPortNumber = Device->ParentPort;
  UsbNode.InterfaceNumber  = UsbIf->IfSetting->Desc.InterfaceNumber;

  SetDevicePathNodeLength (&UsbNode.Header, sizeof (UsbNode));

  Status = GetInterfaceAssociation (Device, IfDesc, &UsbIa);

  if (!EFI_ERROR (Status)) {
    DevicePathProtocol = UsbIa->DevicePath;
  } else {
    HubIf = Device->ParentIf;
    ASSERT (HubIf != NULL);
    DevicePathProtocol = HubIf->DevicePath;
  }

  UsbIf->DevicePath = AppendDevicePathNode (DevicePathProtocol, &UsbNode.Header);

  if (UsbIf->DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "UsbCreateInterface: failed to create device path\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &UsbIf->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbIf->DevicePath,
                  &gEfiUsbIoProtocolGuid,
                  &UsbIf->UsbIo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbCreateInterface: failed to install UsbIo - %r\n", Status));
    goto ON_ERROR;
  }

  Status = GetInterfaceAssociation (Device, IfDesc, &UsbIa);

  if (!EFI_ERROR (Status)) {
    //
    // Open USB Interface Assiciation Protocol by Child
    //
    Status = gBS->OpenProtocol (
                    UsbIa->Handle,
                    &gEfiUsbIaProtocolGuid,
                    (VOID **)&UsbIa,
                    mUsbBusDriverBinding.DriverBindingHandle,
                    UsbIf->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    DEBUG ((DEBUG_INFO, "UsbCreateInterface: open IAD for child, Status = %r\n", Status));
  } else {
    //
    // Open USB Host Controller Protocol by Child
    //
    Status = UsbOpenHostProtoByChild (Device->Bus, UsbIf->Handle);
    DEBUG ((DEBUG_INFO, "UsbCreateInterface: open host controller for child, Status = %r\n", Status));
  }

  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           UsbIf->Handle,
           &gEfiDevicePathProtocolGuid,
           UsbIf->DevicePath,
           &gEfiUsbIoProtocolGuid,
           &UsbIf->UsbIo,
           NULL
           );

    DEBUG ((DEBUG_ERROR, "UsbCreateInterface: failed to open host for child - %r\n", Status));
    goto ON_ERROR;
  }

  return UsbIf;

ON_ERROR:
  if (UsbIf->DevicePath != NULL) {
    FreePool (UsbIf->DevicePath);
  }

  FreePool (UsbIf);
  return NULL;
}

/**
  Create an interface association instance and install protocols to manage it.

  @param  Device        The Usb device that has the interface association.
  @param  Index         The interface association index within Device.
  @param  IfAssocDesc   The interface association descriptor.

  @return The created USB interface association, or NULL.

**/
USB_ASSOCIATION *
UsbCreateAssociation (
  IN USB_DEVICE                      *Device,
  IN UINT8                           Index,
  IN USB_INTERFACE_ASSOCIATION_DESC  *IfAssocDesc
  )
{
  USB_DEVICE_PATH  UsbNode;
  USB_ASSOCIATION  *UsbAssoc;
  EFI_STATUS       Status;

  UsbAssoc = AllocateZeroPool (sizeof (USB_ASSOCIATION));

  if (UsbAssoc == NULL) {
    return NULL;
  }

  UsbAssoc->Signature = USB_ASSOCIATION_SIGNATURE;
  UsbAssoc->Device    = Device;
  UsbAssoc->IaDesc    = IfAssocDesc;

  CopyMem (
    &(UsbAssoc->UsbIa),
    &mUsbIaProtocol,
    sizeof (EFI_USB_IA_PROTOCOL)
    );

  //
  // Install USB association protocols
  //
  // Device path protocol for the association has the USB node similar to the
  // one installed for USB interface.
  //

  UsbNode.Header.Type      = MESSAGING_DEVICE_PATH;
  UsbNode.Header.SubType   = MSG_USB_DP;
  UsbNode.ParentPortNumber = Device->ParentPort;
  UsbNode.InterfaceNumber  = Index;

  SetDevicePathNodeLength (&UsbNode.Header, sizeof (UsbNode));

  ASSERT (Device->ParentIf != NULL);

  UsbAssoc->DevicePath = AppendDevicePathNode (Device->ParentIf->DevicePath, &UsbNode.Header);

  if (UsbAssoc->DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "UsbCreateAssociation: failed to create device path\n"));

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &UsbAssoc->Handle,
                  &gEfiDevicePathProtocolGuid,
                  UsbAssoc->DevicePath,
                  &gEfiUsbIaProtocolGuid,
                  &UsbAssoc->UsbIa,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbCreateAssociation: failed to install UsbIa - %r\n", Status));
    goto ON_ERROR;
  }

  //
  // Open USB Host Controller Protocol by Child
  //
  Status = UsbOpenHostProtoByChild (Device->Bus, UsbAssoc->Handle);

  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           UsbAssoc->Handle,
           &gEfiDevicePathProtocolGuid,
           UsbAssoc->DevicePath,
           &gEfiUsbIaProtocolGuid,
           &UsbAssoc->UsbIa,
           NULL
           );

    DEBUG ((DEBUG_ERROR, "UsbCreateAssociation: failed to open host for child - %r\n", Status));
    goto ON_ERROR;
  }

  return UsbAssoc;

ON_ERROR:
  if (UsbAssoc->DevicePath != NULL) {
    FreePool (UsbAssoc->DevicePath);
  }

  FreePool (UsbAssoc);
  return NULL;
}

/**
  Free the resource used by this USB device.

  @param  Device                The USB device to free.

**/
VOID
UsbFreeDevice (
  IN USB_DEVICE  *Device
  )
{
  if (Device->DevDesc != NULL) {
    UsbFreeDevDesc (Device->DevDesc);
  }

  gBS->FreePool (Device);
}

/**
  Create a device which is on the parent's ParentPort port.

  @param  ParentIf              The parent HUB interface.
  @param  ParentPort            The port on the HUB this device is connected to.

  @return Created USB device, Or NULL.

**/
USB_DEVICE *
UsbCreateDevice (
  IN USB_INTERFACE  *ParentIf,
  IN UINT8          ParentPort
  )
{
  USB_DEVICE  *Device;

  ASSERT (ParentIf != NULL);

  Device = AllocateZeroPool (sizeof (USB_DEVICE));

  if (Device == NULL) {
    return NULL;
  }

  Device->Bus        = ParentIf->Device->Bus;
  Device->MaxPacket0 = 8;
  Device->ParentAddr = ParentIf->Device->Address;
  Device->ParentIf   = ParentIf;
  Device->ParentPort = ParentPort;
  Device->Tier       = (UINT8)(ParentIf->Device->Tier + 1);
  return Device;
}

/**
  Connect USB controller at TPL_CALLBACK

  This function is called in both UsbIoControlTransfer and
  the timer callback in hub enumeration. So, at least it is
  called at TPL_CALLBACK. Some driver sitting on USB has
  twisted TPL used. It should be no problem for us to connect
  or disconnect at CALLBACK.

  @param  Handle        Controller handle to be connected

**/
EFI_STATUS
UsbConnectController (
  EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;
  EFI_TPL     OldTpl;

  OldTpl = UsbGetCurrentTpl ();
  DEBUG ((DEBUG_INFO, "UsbConnectDriver: TPL before connect is %d, %p\n", (UINT32)OldTpl, Handle));

  gBS->RestoreTPL (TPL_CALLBACK);

  Status = gBS->ConnectController (Handle, NULL, NULL, TRUE);

  DEBUG ((DEBUG_INFO, "UsbConnectDriver: TPL after connect is %d\n", (UINT32)UsbGetCurrentTpl ()));
  ASSERT (UsbGetCurrentTpl () == TPL_CALLBACK);

  gBS->RaiseTPL (OldTpl);

  return Status;
}

/**
  Connect the USB interface with its driver. EFI USB bus will
  create a USB interface for each separate interface descriptor.

  @param  UsbIf             The interface to connect driver to.

  @return EFI_SUCCESS       Interface is managed by some driver.
  @return Others            Failed to locate a driver for this interface.

**/
EFI_STATUS
UsbConnectDriver (
  IN USB_INTERFACE  *UsbIf
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Hub is maintained by the USB bus driver. Otherwise try to
  // connect drivers with this interface
  //
  if (UsbIsHubInterface (UsbIf)) {
    DEBUG ((DEBUG_INFO, "UsbConnectDriver: found a hub device\n"));
    Status = mUsbHubApi.Init (UsbIf);
  } else {
    //
    // Only recursively wanted usb child device
    //
    if (UsbBusIsWantedUsbIO (UsbIf->Device->Bus, UsbIf)) {
      Status           = UsbConnectController (UsbIf->Handle);
      UsbIf->IsManaged = (BOOLEAN) !EFI_ERROR (Status);
    }
  }

  return Status;
}

/**
  Select an alternate setting for the interface.
  Each interface can have several mutually exclusive
  settings. Only one setting is active. It will
  also reset its endpoints' toggle to zero.

  @param  IfDesc                The interface descriptor to set.
  @param  Alternate             The alternate setting number to locate.

  @retval EFI_NOT_FOUND         There is no setting with this alternate index.
  @retval EFI_SUCCESS           The interface is set to Alternate setting.

**/
EFI_STATUS
UsbSelectSetting (
  IN USB_INTERFACE_DESC  *IfDesc,
  IN UINT8               Alternate
  )
{
  USB_INTERFACE_SETTING  *Setting;
  UINTN                  Index;

  //
  // Locate the active alternate setting
  //
  Setting = NULL;

  for (Index = 0; Index < IfDesc->NumOfSetting; Index++) {
    ASSERT (Index < USB_MAX_INTERFACE_SETTING);
    Setting = IfDesc->Settings[Index];

    if (Setting->Desc.AlternateSetting == Alternate) {
      break;
    }
  }

  if (Index == IfDesc->NumOfSetting) {
    return EFI_NOT_FOUND;
  }

  IfDesc->ActiveIndex = Index;

  ASSERT (Setting != NULL);
  DEBUG ((
    DEBUG_INFO,
    "UsbSelectSetting: setting %d selected for interface %d\n",
    Alternate,
    Setting->Desc.InterfaceNumber
    ));

  //
  // Reset the endpoint toggle to zero
  //
  for (Index = 0; Index < Setting->Desc.NumEndpoints; Index++) {
    Setting->Endpoints[Index]->Toggle = 0;
  }

  return EFI_SUCCESS;
}

/**
  Select a new configuration for the device. Each
  device may support several configurations.

  @param  Device                The device to select configuration.
  @param  ConfigValue           The index of the configuration ( != 0).

  @retval EFI_NOT_FOUND         There is no configuration with the index.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval EFI_SUCCESS           The configuration is selected.

**/
EFI_STATUS
UsbSelectConfig (
  IN USB_DEVICE  *Device,
  IN UINT8       ConfigValue
  )
{
  USB_DEVICE_DESC     *DevDesc;
  USB_CONFIG_DESC     *ConfigDesc;
  USB_INTERFACE_DESC  *IfDesc;
  USB_INTERFACE       *UsbIf;
  EFI_STATUS          Status;
  UINT8               Index;
  USB_ASSOCIATION     *UsbIa;

  ASSERT (Device != NULL);
  if (Device == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Locate the active config, then set the device's pointer
  //
  DevDesc    = Device->DevDesc;
  ConfigDesc = NULL;

  for (Index = 0; Index < DevDesc->Desc.NumConfigurations; Index++) {
    ConfigDesc = DevDesc->Configs[Index];

    if (ConfigDesc->Desc.ConfigurationValue == ConfigValue) {
      break;
    }
  }

  if ((Index == DevDesc->Desc.NumConfigurations) || (ConfigDesc == NULL)) {
    return EFI_NOT_FOUND;
  }

  Device->ActiveConfig = ConfigDesc;

  DEBUG ((
    DEBUG_INFO,
    "UsbSelectConfig: config %d selected for device %d\n",
    ConfigValue,
    Device->Address
    ));

  //
  // Create interfaces for each USB interface association descriptor.
  //
  Device->NumOfAssociation = ConfigDesc->NumOfIads;

  for (Index = 0; Index < ConfigDesc->NumOfIads; Index++) {
    DEBUG ((DEBUG_INFO, "UsbSelectConfig: process IAD %d\n", Index));

    UsbIa = UsbCreateAssociation (Device, Index, ConfigDesc->Iads[Index]);
    ASSERT (UsbIa != NULL);
    if (UsbIa == NULL) {
      return EFI_NOT_FOUND;
    }

    ASSERT (Index < USB_MAX_ASSOCIATION);
    Device->Associations[Index] = UsbIa;
  }

  //
  // Create interfaces for each USB interface descriptor.
  //
  for (Index = 0; Index < ConfigDesc->Desc.NumInterfaces; Index++) {
    //
    // First select the default interface setting, and reset
    // the endpoint toggles to zero for its endpoints.
    //
    IfDesc = ConfigDesc->Interfaces[Index];
    UsbSelectSetting (IfDesc, IfDesc->Settings[0]->Desc.AlternateSetting);

    //
    // Create a USB_INTERFACE and install USB_IO and other protocols
    //
    UsbIf = UsbCreateInterface (Device, ConfigDesc->Interfaces[Index]);

    if (UsbIf == NULL) {
      Device->NumOfInterface = Index;
      return EFI_OUT_OF_RESOURCES;
    }

    ASSERT (Index < USB_MAX_INTERFACE);
    Device->Interfaces[Index] = UsbIf;

    //
    // Connect the device to drivers, if it failed, ignore
    // the error. Don't let the unsupported interfaces to block
    // the supported interfaces.
    //
    Status = UsbConnectDriver (UsbIf);

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "UsbSelectConfig: failed to connect driver - %r, ignored\n",
        Status
        ));
    }
  }

  Device->NumOfInterface = Index;

  //
  // Connect association device drivers. Connection may fail if if device driver has been already
  // started for any UsbIo that belongs to the association.
  //
  for (Index = 0; Index < Device->NumOfAssociation; Index++) {
    Status                                 = gBS->ConnectController (Device->Associations[Index]->Handle, NULL, NULL, TRUE);
    Device->Associations[Index]->IsManaged = (BOOLEAN) !EFI_ERROR (Status);

    DEBUG ((DEBUG_INFO, "UsbSelectConfig: association driver connect: %r\n", Status));
  }

  return EFI_SUCCESS;
}

/**
  Disconnect USB controller at TPL_CALLBACK

  This function is called in both UsbIoControlTransfer and
  the timer callback in hub enumeration. So, at least it is
  called at TPL_CALLBACK. Some driver sitting on USB has
  twisted TPL used. It should be no problem for us to connect
  or disconnect at CALLBACK.

  @param  Handle        Controller handle to be disconnected

**/
EFI_STATUS
UsbDisconnectController (
  EFI_HANDLE  Handle
  )
{
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  OldTpl = UsbGetCurrentTpl ();
  DEBUG ((DEBUG_INFO, "UsbDisconnectDriver: old TPL is %d, %p\n", (UINT32)OldTpl, Handle));

  gBS->RestoreTPL (TPL_CALLBACK);

  Status = gBS->DisconnectController (Handle, NULL, NULL);

  DEBUG ((DEBUG_INFO, "UsbDisconnectDriver: TPL after disconnect is %d, %d\n", (UINT32)UsbGetCurrentTpl (), Status));
  ASSERT (UsbGetCurrentTpl () == TPL_CALLBACK);

  gBS->RaiseTPL (OldTpl);

  return Status;
}

/**
  Disconnect the USB interface with its driver.

  @param  UsbIf                 The interface to disconnect driver from.

**/
EFI_STATUS
UsbDisconnectDriver (
  IN USB_INTERFACE  *UsbIf
  )
{
  EFI_STATUS  Status;

  //
  // Release the hub if it's a hub controller, otherwise
  // disconnect the driver if it is managed by other drivers.
  //
  Status = EFI_SUCCESS;
  if (UsbIf->IsHub) {
    Status = UsbIf->HubApi->Release (UsbIf);
  } else if (UsbIf->IsManaged) {
    Status = UsbDisconnectController (UsbIf->Handle);

    if (!EFI_ERROR (Status)) {
      UsbIf->IsManaged = FALSE;
    }
  }

  return Status;
}

/**
  Remove the current device configuration.

  @param  Device                The USB device to remove configuration from.

**/
EFI_STATUS
UsbRemoveConfig (
  IN USB_DEVICE  *Device
  )
{
  USB_INTERFACE    *UsbIf;
  USB_ASSOCIATION  *UsbIa;
  UINTN            Index;
  EFI_STATUS       Status;
  EFI_STATUS       ReturnStatus;

  //
  // Remove each interface of the device
  //
  ReturnStatus = EFI_SUCCESS;
  for (Index = 0; Index < Device->NumOfInterface; Index++) {
    ASSERT (Index < USB_MAX_INTERFACE);
    UsbIf = Device->Interfaces[Index];

    if (UsbIf == NULL) {
      continue;
    }

    Status = UsbDisconnectDriver (UsbIf);
    if (!EFI_ERROR (Status)) {
      Status = UsbFreeInterface (UsbIf);
      if (EFI_ERROR (Status)) {
        UsbConnectDriver (UsbIf);
      }
    }

    if (!EFI_ERROR (Status)) {
      Device->Interfaces[Index] = NULL;
    } else {
      ReturnStatus = Status;
    }
  }

  Device->ActiveConfig = NULL;

  if (EFI_ERROR (ReturnStatus)) {
    return ReturnStatus;
  }

  // ReturnStatus is EFI_SUCCESS

  //
  // Remove each interface association
  //
  for (Index = 0; Index < Device->NumOfAssociation; Index++) {
    UsbIa = Device->Associations[Index];

    Status = UsbDisconnectController (UsbIa->Handle);
    if (!EFI_ERROR (Status)) {
      Status = UsbFreeAssociation (UsbIa);
      DEBUG ((DEBUG_INFO, "UsbRemoveConfig: free association: %r\n", Status));
      if (EFI_ERROR (Status)) {
        UsbConnectController (UsbIa->Handle);
      }
    }

    if (!EFI_ERROR (Status)) {
      Device->Associations[Index] = NULL;
    } else {
      ReturnStatus = Status;
    }
  }

  return ReturnStatus;
}

/**
  Remove the device and all its children from the bus.

  @param  Device                The device to remove.

  @retval EFI_SUCCESS           The device is removed.

**/
EFI_STATUS
UsbRemoveDevice (
  IN USB_DEVICE  *Device
  )
{
  USB_BUS     *Bus;
  USB_DEVICE  *Child;
  EFI_STATUS  Status;
  EFI_STATUS  ReturnStatus;
  UINTN       Index;

  Bus = Device->Bus;

  //
  // Remove all the devices on its downstream ports. Search from devices[1].
  // Devices[0] is the root hub.
  //
  ReturnStatus = EFI_SUCCESS;
  for (Index = 1; Index < Bus->MaxDevices; Index++) {
    Child = Bus->Devices[Index];

    if ((Child == NULL) || (Child->ParentAddr != Device->Address)) {
      continue;
    }

    Status = UsbRemoveDevice (Child);

    if (!EFI_ERROR (Status)) {
      Bus->Devices[Index] = NULL;
    } else {
      Bus->Devices[Index]->DisconnectFail = TRUE;
      ReturnStatus                        = Status;
      DEBUG ((DEBUG_INFO, "UsbRemoveDevice: failed to remove child %p at parent %p\n", Child, Device));
    }
  }

  if (EFI_ERROR (ReturnStatus)) {
    return ReturnStatus;
  }

  Status = UsbRemoveConfig (Device);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UsbRemoveDevice: device %d removed\n", Device->Address));

    ASSERT (Device->Address < Bus->MaxDevices);
    Bus->Devices[Device->Address] = NULL;
    UsbFreeDevice (Device);
  } else {
    Bus->Devices[Device->Address]->DisconnectFail = TRUE;
  }

  return Status;
}

/**
  Find the child device on the hub's port.

  @param  HubIf                 The hub interface.
  @param  Port                  The port of the hub this child is connected to.

  @return The device on the hub's port, or NULL if there is none.

**/
USB_DEVICE *
UsbFindChild (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port
  )
{
  USB_DEVICE  *Device;
  USB_BUS     *Bus;
  UINTN       Index;

  Bus = HubIf->Device->Bus;

  //
  // Start checking from device 1, device 0 is the root hub
  //
  for (Index = 1; Index < Bus->MaxDevices; Index++) {
    Device = Bus->Devices[Index];

    if ((Device != NULL) && (Device->ParentAddr == HubIf->Device->Address) &&
        (Device->ParentPort == Port))
    {
      return Device;
    }
  }

  return NULL;
}

/**
  Enumerate and configure the new device on the port of this HUB interface.

  @param  HubIf                 The HUB that has the device connected.
  @param  Port                  The port index of the hub (started with zero).
  @param  ResetIsNeeded         The boolean to control whether skip the reset of the port.

  @retval EFI_SUCCESS           The device is enumerated (added or removed).
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the device.
  @retval Others                Failed to enumerate the device.

**/
EFI_STATUS
UsbEnumerateNewDev (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port,
  IN BOOLEAN        ResetIsNeeded
  )
{
  USB_BUS              *Bus;
  USB_HUB_API          *HubApi;
  USB_DEVICE           *Child;
  USB_DEVICE           *Parent;
  EFI_USB_PORT_STATUS  PortState;
  UINTN                Address;
  UINT8                Config;
  EFI_STATUS           Status;

  Parent  = HubIf->Device;
  Bus     = Parent->Bus;
  HubApi  = HubIf->HubApi;
  Address = Bus->MaxDevices;

  gBS->Stall (USB_WAIT_PORT_STABLE_STALL);

  //
  // Hub resets the device for at least 10 milliseconds.
  // Host learns device speed. If device is of low/full speed
  // and the hub is a EHCI root hub, ResetPort will release
  // the device to its companion UHCI and return an error.
  //
  if (ResetIsNeeded) {
    Status = HubApi->ResetPort (HubIf, Port);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to reset port %d - %r\n", Port, Status));

      return Status;
    }

    DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: hub port %d is reset\n", Port));
  } else {
    DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: hub port %d reset is skipped\n", Port));
  }

  Child = UsbCreateDevice (HubIf, Port);

  if (Child == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // OK, now identify the device speed. After reset, hub
  // fully knows the actual device speed.
  //
  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to get speed of port %d\n", Port));
    goto ON_ERROR;
  }

  if (!USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: No device present at port %d\n", Port));
    Status = EFI_NOT_FOUND;
    goto ON_ERROR;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_SUPER_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_SUPER;
    Child->MaxPacket0 = 512;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_HIGH_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_HIGH;
    Child->MaxPacket0 = 64;
  } else if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_LOW_SPEED)) {
    Child->Speed      = EFI_USB_SPEED_LOW;
    Child->MaxPacket0 = 8;
  } else {
    Child->Speed      = EFI_USB_SPEED_FULL;
    Child->MaxPacket0 = 8;
  }

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: device is of %d speed\n", Child->Speed));

  if (((Child->Speed == EFI_USB_SPEED_LOW) || (Child->Speed == EFI_USB_SPEED_FULL)) &&
      (Parent->Speed == EFI_USB_SPEED_HIGH))
  {
    //
    // If the child is a low or full speed device, it is necessary to
    // set the transaction translator. Port TT is 1-based.
    // This is quite simple:
    //  1. if parent is of high speed, then parent is our translator
    //  2. otherwise use parent's translator.
    //
    Child->Translator.TranslatorHubAddress = Parent->Address;
    Child->Translator.TranslatorPortNumber = (UINT8)(Port + 1);
  } else {
    Child->Translator = Parent->Translator;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbEnumerateNewDev: device uses translator (%d, %d)\n",
    Child->Translator.TranslatorHubAddress,
    Child->Translator.TranslatorPortNumber
    ));

  //
  // After port is reset, hub establishes a signal path between
  // the device and host (DEFAULT state). Device's registers are
  // reset, use default address 0 (host enumerates one device at
  // a time) , and ready to respond to control transfer at EP 0.
  //

  //
  // Host assigns an address to the device. Device completes the
  // status stage with default address, then switches to new address.
  // ADDRESS state. Address zero is reserved for root hub.
  //
  ASSERT (Bus->MaxDevices <= 256);
  for (Address = 1; Address < Bus->MaxDevices; Address++) {
    if (Bus->Devices[Address] == NULL) {
      break;
    }
  }

  if (Address >= Bus->MaxDevices) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: address pool is full for port %d\n", Port));

    Status = EFI_ACCESS_DENIED;
    goto ON_ERROR;
  }

  Status                = UsbSetAddress (Child, (UINT8)Address);
  Child->Address        = (UINT8)Address;
  Bus->Devices[Address] = Child;

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to set device address - %r\n", Status));
    goto ON_ERROR;
  }

  gBS->Stall (USB_SET_DEVICE_ADDRESS_STALL);

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: device is now ADDRESSED at %d\n", Address));

  //
  // Host sends a Get_Descriptor request to learn the max packet
  // size of default pipe (only part of the device's descriptor).
  //
  Status = UsbGetMaxPacketSize0 (Child);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to get max packet for EP 0 - %r\n", Status));
    goto ON_ERROR;
  }

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: max packet size for EP 0 is %d\n", Child->MaxPacket0));

  //
  // Host learns about the device's abilities by requesting device's
  // entire descriptions.
  //
  Status = UsbBuildDescTable (Child);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to build descriptor table - %r\n", Status));
    goto ON_ERROR;
  }

  //
  // Select a default configuration: UEFI must set the configuration
  // before the driver can connect to the device.
  //
  Config = Child->DevDesc->Configs[0]->Desc.ConfigurationValue;
  Status = UsbSetConfig (Child, Config);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to set configure %d - %r\n", Config, Status));
    goto ON_ERROR;
  }

  DEBUG ((DEBUG_INFO, "UsbEnumerateNewDev: device %d is now in CONFIGED state\n", Address));

  //
  // Host assigns and loads a device driver.
  //
  Status = UsbSelectConfig (Child, Config);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumerateNewDev: failed to create interfaces - %r\n", Status));
    goto ON_ERROR;
  }

  //
  // Report Status Code to indicate USB device has been detected by hotplug
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_USB | EFI_IOB_PC_HOTPLUG),
    Bus->DevicePath
    );
  return EFI_SUCCESS;

ON_ERROR:
  //
  // If reach here, it means the enumeration process on a given port is interrupted due to error.
  // The s/w resources, including the assigned address(Address) and the allocated usb device data
  // structure(Bus->Devices[Address]), will NOT be freed here. These resources will be freed when
  // the device is unplugged from the port or DriverBindingStop() is invoked.
  //
  // This way is used to co-work with the lower layer EDKII UHCI/EHCI/XHCI host controller driver.
  // It's mainly because to keep UEFI spec unchanged EDKII XHCI driver have to maintain a state machine
  // to keep track of the mapping between actual address and request address. If the request address
  // (Address) is freed here, the Address value will be used by next enumerated device. Then EDKII XHCI
  // host controller driver will have wrong information, which will cause further transaction error.
  //
  // EDKII UHCI/EHCI doesn't get impacted as it's make sense to reserve s/w resource till it gets unplugged.
  //
  return Status;
}

/**
  Process the events on the port.

  @param  HubIf                 The HUB that has the device connected.
  @param  Port                  The port index of the hub (started with zero).

  @retval EFI_SUCCESS           The device is enumerated (added or removed).
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the device.
  @retval Others                Failed to enumerate the device.

**/
EFI_STATUS
UsbEnumeratePort (
  IN USB_INTERFACE  *HubIf,
  IN UINT8          Port
  )
{
  USB_HUB_API          *HubApi;
  USB_DEVICE           *Child;
  EFI_USB_PORT_STATUS  PortState;
  EFI_STATUS           Status;

  Child  = NULL;
  HubApi = HubIf->HubApi;

  //
  // Host learns of the new device by polling the hub for port changes.
  //
  Status = HubApi->GetPortStatus (HubIf, Port, &PortState);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: failed to get state of port %d\n", Port));
    return Status;
  }

  //
  // Only handle connection/enable/overcurrent/reset change.
  // Usb super speed hub may report other changes, such as warm reset change. Ignore them.
  //
  if ((PortState.PortChangeStatus & (USB_PORT_STAT_C_CONNECTION | USB_PORT_STAT_C_ENABLE | USB_PORT_STAT_C_OVERCURRENT | USB_PORT_STAT_C_RESET)) == 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbEnumeratePort: port %d state - %02x, change - %02x on %p\n",
    Port,
    PortState.PortStatus,
    PortState.PortChangeStatus,
    HubIf
    ));

  //
  // This driver only process two kinds of events now: over current and
  // connect/disconnect. Other three events are: ENABLE, SUSPEND, RESET.
  // ENABLE/RESET is used to reset port. SUSPEND isn't supported.
  //

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_OVERCURRENT)) {
    if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_OVERCURRENT)) {
      //
      // Case1:
      //   Both OverCurrent and OverCurrentChange set, means over current occurs,
      //   which probably is caused by short circuit. It has to wait system hardware
      //   to perform recovery.
      //
      DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: Critical Over Current (port %d)\n", Port));
      return EFI_DEVICE_ERROR;
    }

    //
    // Case2:
    //   Only OverCurrentChange set, means system has been recoveried from
    //   over current. As a result, all ports are nearly power-off, so
    //   it's necessary to detach and enumerate all ports again.
    //
    DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: 2.0 device Recovery Over Current (port %d)\n", Port));
  }

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_ENABLE)) {
    //
    // Case3:
    //   1.1 roothub port reg doesn't reflect over-current state, while its counterpart
    //   on 2.0 roothub does. When over-current has influence on 1.1 device, the port
    //   would be disabled, so it's also necessary to detach and enumerate again.
    //
    DEBUG ((DEBUG_ERROR, "UsbEnumeratePort: 1.1 device Recovery Over Current (port %d)\n", Port));
  }

  if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_CONNECTION)) {
    //
    // Case4:
    //   Device connected or disconnected normally.
    //
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: Device Connect/Disconnect Normally (port %d)\n", Port));
  }

  //
  // Following as the above cases, it's safety to remove and create again.
  //
  Child = UsbFindChild (HubIf, Port);

  if (Child != NULL) {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: device at port %d removed from root hub %p\n", Port, HubIf));
    UsbRemoveDevice (Child);
  }

  if (USB_BIT_IS_SET (PortState.PortStatus, USB_PORT_STAT_CONNECTION)) {
    //
    // Now, new device connected, enumerate and configure the device
    //
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: new device connected at port %d\n", Port));
    if (USB_BIT_IS_SET (PortState.PortChangeStatus, USB_PORT_STAT_C_RESET)) {
      Status = UsbEnumerateNewDev (HubIf, Port, FALSE);
    } else {
      Status = UsbEnumerateNewDev (HubIf, Port, TRUE);
    }
  } else {
    DEBUG ((DEBUG_INFO, "UsbEnumeratePort: device disconnected event on port %d\n", Port));
  }

  HubApi->ClearPortChange (HubIf, Port);
  return Status;
}

/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

**/
VOID
EFIAPI
UsbHubEnumeration (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  USB_INTERFACE  *HubIf;
  UINT8          Byte;
  UINT8          Bit;
  UINT8          Index;
  USB_DEVICE     *Child;

  ASSERT (Context != NULL);

  HubIf = (USB_INTERFACE *)Context;

  for (Index = 0; Index < HubIf->NumOfPort; Index++) {
    Child = UsbFindChild (HubIf, Index);
    if ((Child != NULL) && (Child->DisconnectFail == TRUE)) {
      DEBUG ((DEBUG_INFO, "UsbEnumeratePort: The device disconnect fails at port %d from hub %p, try again\n", Index, HubIf));
      UsbRemoveDevice (Child);
    }
  }

  if (HubIf->ChangeMap == NULL) {
    return;
  }

  //
  // HUB starts its port index with 1.
  //
  Byte = 0;
  Bit  = 1;

  for (Index = 0; Index < HubIf->NumOfPort; Index++) {
    if (USB_BIT_IS_SET (HubIf->ChangeMap[Byte], USB_BIT (Bit))) {
      UsbEnumeratePort (HubIf, Index);
    }

    USB_NEXT_BIT (Byte, Bit);
  }

  UsbHubAckHubStatus (HubIf->Device);

  gBS->FreePool (HubIf->ChangeMap);
  HubIf->ChangeMap = NULL;
  return;
}

/**
  Enumerate all the changed hub ports.

  @param  Event                 The event that is triggered.
  @param  Context               The context to the event.

**/
VOID
EFIAPI
UsbRootHubEnumeration (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  USB_INTERFACE  *RootHub;
  UINT8          Index;
  USB_DEVICE     *Child;

  RootHub = (USB_INTERFACE *)Context;

  for (Index = 0; Index < RootHub->NumOfPort; Index++) {
    Child = UsbFindChild (RootHub, Index);
    if ((Child != NULL) && (Child->DisconnectFail == TRUE)) {
      DEBUG ((DEBUG_INFO, "UsbEnumeratePort: The device disconnect fails at port %d from root hub %p, try again\n", Index, RootHub));
      UsbRemoveDevice (Child);
    }

    UsbEnumeratePort (RootHub, Index);
  }
}
