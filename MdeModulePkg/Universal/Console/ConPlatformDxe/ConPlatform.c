/** @file
  Console Platform DXE Driver, install Console Device Guids and update Console
  Environment Variables.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ConPlatform.h"

EFI_DRIVER_BINDING_PROTOCOL  gConPlatformTextInDriverBinding = {
  ConPlatformTextInDriverBindingSupported,
  ConPlatformTextInDriverBindingStart,
  ConPlatformTextInDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL  gConPlatformTextOutDriverBinding = {
  ConPlatformTextOutDriverBindingSupported,
  ConPlatformTextOutDriverBindingStart,
  ConPlatformTextOutDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Values from Usb Inteface Association Descriptor Device
//  Class Code and Usage Model specification (iadclasscode_r10.pdf)
//  from Usb.org
//
#define USB_BASE_CLASS_MISCELLANEOUS       0xEF
#define USB_MISCELLANEOUS_SUBCLASS_COMMON  0x02
#define USB_MISCELLANEOUS_PROTOCOL_IAD     0x01

/**
  Entrypoint of this module.

  This function is the entrypoint of this module. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
InitializeConPlatform (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConPlatformTextInDriverBinding,
             ImageHandle,
             &gConPlatformComponentName,
             &gConPlatformComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gConPlatformTextOutDriverBinding,
             NULL,
             &gConPlatformComponentName,
             &gConPlatformComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Test to see if EFI_SIMPLE_TEXT_INPUT_PROTOCOL is supported on ControllerHandle.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConPlatformTextInDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  return ConPlatformDriverBindingSupported (
           This,
           ControllerHandle,
           &gEfiSimpleTextInProtocolGuid
           );
}

/**
  Test to see if EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL is supported on ControllerHandle.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConPlatformTextOutDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  return ConPlatformDriverBindingSupported (
           This,
           ControllerHandle,
           &gEfiSimpleTextOutProtocolGuid
           );
}

/**
  Test to see if the specified protocol is supported on ControllerHandle.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  ProtocolGuid        The specfic protocol.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
ConPlatformDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_GUID                     *ProtocolGuid
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  //
  // Test to see if this is a physical device by checking if
  // it has a Device Path Protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Test to see if this device supports the specified Protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  ProtocolGuid,
                  (VOID **)&Interface,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         ControllerHandle,
         ProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}

/**
  Start this driver on the device for console input.

  Start this driver on ControllerHandle by opening Simple Text Input Protocol,
  reading Device Path, and installing Console In Devcice GUID on ControllerHandle.

  Append its device path into the console environment variables ConInDev.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ConPlatformTextInDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *TextIn;
  BOOLEAN                         IsInConInVariable;

  //
  // Get the Device Path Protocol so the environment variables can be updated
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the Simple Text Input Protocol BY_DRIVER
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleTextInProtocolGuid,
                  (VOID **)&TextIn,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if the device path is in ConIn Variable
  //
  IsInConInVariable = FALSE;
  Status            = ConPlatformUpdateDeviceVariable (
                        L"ConIn",
                        DevicePath,
                        Check
                        );
  if (!EFI_ERROR (Status)) {
    IsInConInVariable = TRUE;
  }

  //
  // Append the device path to the ConInDev environment variable
  //
  ConPlatformUpdateDeviceVariable (
    L"ConInDev",
    DevicePath,
    Append
    );

  //
  // If the device path is an instance in the ConIn environment variable,
  // then install EfiConsoleInDeviceGuid onto ControllerHandle
  //
  if (IsInConInVariable) {
    gBS->InstallMultipleProtocolInterfaces (
           &ControllerHandle,
           &gEfiConsoleInDeviceGuid,
           NULL,
           NULL
           );
  } else {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiSimpleTextInProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

  return EFI_SUCCESS;
}

/**
  Start this driver on the device for console output and standard error output.

  Start this driver on ControllerHandle by opening Simple Text Output Protocol,
  reading Device Path, and installing Console Out Devcic GUID, Standard Error
  Device GUID on ControllerHandle.

  Append its device path into the console environment variables ConOutDev, ErrOutDev.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
ConPlatformTextOutDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                       Status;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut;
  BOOLEAN                          NeedClose;
  BOOLEAN                          IsInConOutVariable;
  BOOLEAN                          IsInErrOutVariable;

  NeedClose = TRUE;

  //
  // Get the Device Path Protocol so the environment variables can be updated
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the Simple Text Output Protocol BY_DRIVER
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **)&TextOut,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if the device path is in ConOut & ErrOut Variable
  //
  IsInConOutVariable = FALSE;
  Status             = ConPlatformUpdateDeviceVariable (
                         L"ConOut",
                         DevicePath,
                         Check
                         );
  if (!EFI_ERROR (Status)) {
    IsInConOutVariable = TRUE;
  }

  IsInErrOutVariable = FALSE;
  Status             = ConPlatformUpdateDeviceVariable (
                         L"ErrOut",
                         DevicePath,
                         Check
                         );
  if (!EFI_ERROR (Status)) {
    IsInErrOutVariable = TRUE;
  }

  //
  // Append the device path to the ConOutDev and ErrOutDev environment variable.
  // For GOP device path, append the sibling device path as well.
  //
  if (!ConPlatformUpdateGopCandidate (DevicePath)) {
    ConPlatformUpdateDeviceVariable (
      L"ConOutDev",
      DevicePath,
      Append
      );
    //
    // Then append the device path to the ErrOutDev environment variable
    //
    ConPlatformUpdateDeviceVariable (
      L"ErrOutDev",
      DevicePath,
      Append
      );
  }

  //
  // If the device path is an instance in the ConOut environment variable,
  // then install EfiConsoleOutDeviceGuid onto ControllerHandle
  //
  if (IsInConOutVariable) {
    NeedClose = FALSE;
    Status    = gBS->InstallMultipleProtocolInterfaces (
                       &ControllerHandle,
                       &gEfiConsoleOutDeviceGuid,
                       NULL,
                       NULL
                       );
  }

  //
  // If the device path is an instance in the ErrOut environment variable,
  // then install EfiStandardErrorDeviceGuid onto ControllerHandle
  //
  if (IsInErrOutVariable) {
    NeedClose = FALSE;
    gBS->InstallMultipleProtocolInterfaces (
           &ControllerHandle,
           &gEfiStandardErrorDeviceGuid,
           NULL,
           NULL
           );
  }

  if (NeedClose) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiSimpleTextOutProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

  return EFI_SUCCESS;
}

/**
  Stop this driver on ControllerHandle by removing Console In Devcice GUID
  and closing the Simple Text Input protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConPlatformTextInDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // Get the Device Path Protocol firstly
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  //
  // If there is device path on ControllerHandle
  //
  if (!EFI_ERROR (Status)) {
    //
    // Remove DevicePath from ConInDev if exists.
    //
    ConPlatformUpdateDeviceVariable (
      L"ConInDev",
      DevicePath,
      Delete
      );
  }

  //
  // Uninstall the Console Device GUIDs from Controller Handle
  //
  ConPlatformUnInstallProtocol (
    This,
    ControllerHandle,
    &gEfiConsoleInDeviceGuid
    );

  //
  // Close the Simple Text Input Protocol
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiSimpleTextInProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}

/**
  Stop this driver on ControllerHandle by removing Console Out Devcice GUID
  and closing the Simple Text Output protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ConPlatformTextOutDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // Get the Device Path Protocol firstly
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Remove DevicePath from ConOutDev and ErrOutDev if exists.
    //
    ConPlatformUpdateDeviceVariable (
      L"ConOutDev",
      DevicePath,
      Delete
      );
    ConPlatformUpdateDeviceVariable (
      L"ErrOutDev",
      DevicePath,
      Delete
      );
  }

  //
  // Uninstall the Console Device GUIDs from Controller Handle
  //
  ConPlatformUnInstallProtocol (
    This,
    ControllerHandle,
    &gEfiConsoleOutDeviceGuid
    );

  ConPlatformUnInstallProtocol (
    This,
    ControllerHandle,
    &gEfiStandardErrorDeviceGuid
    );

  //
  // Close the Simple Text Output Protocol
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiSimpleTextOutProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}

/**
  Uninstall the specified protocol.

  @param This            Protocol instance pointer.
  @param Handle          Handle of device to uninstall protocol on.
  @param ProtocolGuid    The specified protocol need to be uninstalled.

**/
VOID
ConPlatformUnInstallProtocol (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_GUID                     *ProtocolGuid
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  Handle,
                  ProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Handle,
           ProtocolGuid,
           NULL,
           NULL
           );
  }

  return;
}

/**
  Get the necessary size of buffer and read the variable.

  First get the necessary size of buffer. Then read the
  EFI variable (Name) and return a dynamically allocated
  buffer. On failure return NULL.

  @param  Name             String part of EFI variable name

  @return Dynamically allocated memory that contains a copy of the EFI variable.
          Caller is repsoncible freeing the buffer. Return NULL means Variable
          was not read.

**/
VOID *
ConPlatformGetVariable (
  IN  CHAR16  *Name
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer;
  UINTN       BufferSize;

  BufferSize = 0;
  Buffer     = NULL;

  //
  // Test to see if the variable exists.  If it doesn't, return NULL.
  //
  Status = gRT->GetVariable (
                  Name,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &BufferSize,
                  Buffer
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate the buffer to return
    //
    Buffer = AllocatePool (BufferSize);
    if (Buffer == NULL) {
      return NULL;
    }

    //
    // Read variable into the allocated buffer.
    //
    Status = gRT->GetVariable (
                    Name,
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &BufferSize,
                    Buffer
                    );
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      //
      // To make sure Buffer is NULL if any error occurs.
      //
      Buffer = NULL;
    }
  }

  return Buffer;
}

/**
  Function returns TRUE when the two input device paths point to the two
  GOP child handles that have the same parent.

  @param Left    A pointer to a device path data structure.
  @param Right   A pointer to a device path data structure.

  @retval TRUE  Left and Right share the same parent.
  @retval FALSE Left and Right don't share the same parent or either of them is not
                a GOP device path.
**/
BOOLEAN
IsGopSibling (
  IN EFI_DEVICE_PATH_PROTOCOL  *Left,
  IN EFI_DEVICE_PATH_PROTOCOL  *Right
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *NodeLeft;
  EFI_DEVICE_PATH_PROTOCOL  *NodeRight;

  for (NodeLeft = Left; !IsDevicePathEndType (NodeLeft); NodeLeft = NextDevicePathNode (NodeLeft)) {
    if (((DevicePathType (NodeLeft) == ACPI_DEVICE_PATH) && (DevicePathSubType (NodeLeft) == ACPI_ADR_DP)) ||
        ((DevicePathType (NodeLeft) == HARDWARE_DEVICE_PATH) && (DevicePathSubType (NodeLeft) == HW_CONTROLLER_DP) &&
         (DevicePathType (NextDevicePathNode (NodeLeft)) == ACPI_DEVICE_PATH) && (DevicePathSubType (NextDevicePathNode (NodeLeft)) == ACPI_ADR_DP)))
    {
      break;
    }
  }

  if (IsDevicePathEndType (NodeLeft)) {
    return FALSE;
  }

  for (NodeRight = Right; !IsDevicePathEndType (NodeRight); NodeRight = NextDevicePathNode (NodeRight)) {
    if (((DevicePathType (NodeRight) == ACPI_DEVICE_PATH) && (DevicePathSubType (NodeRight) == ACPI_ADR_DP)) ||
        ((DevicePathType (NodeRight) == HARDWARE_DEVICE_PATH) && (DevicePathSubType (NodeRight) == HW_CONTROLLER_DP) &&
         (DevicePathType (NextDevicePathNode (NodeRight)) == ACPI_DEVICE_PATH) && (DevicePathSubType (NextDevicePathNode (NodeRight)) == ACPI_ADR_DP)))
    {
      break;
    }
  }

  if (IsDevicePathEndType (NodeRight)) {
    return FALSE;
  }

  if (((UINTN)NodeLeft - (UINTN)Left) != ((UINTN)NodeRight - (UINTN)Right)) {
    return FALSE;
  }

  return (BOOLEAN)(CompareMem (Left, Right, (UINTN)NodeLeft - (UINTN)Left) == 0);
}

/**
  Check whether a USB device match the specified USB Class device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbClass    The USB Class device path to match.

  @retval TRUE       The USB device match the USB Class device path.
  @retval FALSE      The USB device does not match the USB Class device path.

**/
BOOLEAN
MatchUsbClass (
  IN EFI_USB_IO_PROTOCOL    *UsbIo,
  IN USB_CLASS_DEVICE_PATH  *UsbClass
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT8                         DeviceClass;
  UINT8                         DeviceSubClass;
  UINT8                         DeviceProtocol;

  if ((DevicePathType (UsbClass) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbClass) != MSG_USB_CLASS_DP))
  {
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((UsbClass->VendorId != 0xffff) &&
      (UsbClass->VendorId != DevDesc.IdVendor))
  {
    return FALSE;
  }

  if ((UsbClass->ProductId != 0xffff) &&
      (UsbClass->ProductId != DevDesc.IdProduct))
  {
    return FALSE;
  }

  DeviceClass    = DevDesc.DeviceClass;
  DeviceSubClass = DevDesc.DeviceSubClass;
  DeviceProtocol = DevDesc.DeviceProtocol;

  if ((DeviceClass == 0) ||
      ((DeviceClass == USB_BASE_CLASS_MISCELLANEOUS) &&
       (DeviceSubClass == USB_MISCELLANEOUS_SUBCLASS_COMMON) &&
       (DeviceProtocol == USB_MISCELLANEOUS_PROTOCOL_IAD)))
  {
    //
    // If Class in Device Descriptor is set to 0 (Device), or
    // Class/SubClass/Protocol is 0xEF/0x02/0x01 (IAD), use the Class, SubClass
    // and Protocol in Interface Descriptor instead.
    //
    Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    DeviceClass    = IfDesc.InterfaceClass;
    DeviceSubClass = IfDesc.InterfaceSubClass;
    DeviceProtocol = IfDesc.InterfaceProtocol;
  }

  //
  // Check Class, SubClass and Protocol.
  //
  if ((UsbClass->DeviceClass != 0xff) &&
      (UsbClass->DeviceClass != DeviceClass))
  {
    return FALSE;
  }

  if ((UsbClass->DeviceSubClass != 0xff) &&
      (UsbClass->DeviceSubClass != DeviceSubClass))
  {
    return FALSE;
  }

  if ((UsbClass->DeviceProtocol != 0xff) &&
      (UsbClass->DeviceProtocol != DeviceProtocol))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Check whether a USB device match the specified USB WWID device path. This
  function follows "Load Option Processing" behavior in UEFI specification.

  @param UsbIo       USB I/O protocol associated with the USB device.
  @param UsbWwid     The USB WWID device path to match.

  @retval TRUE       The USB device match the USB WWID device path.
  @retval FALSE      The USB device does not match the USB WWID device path.

**/
BOOLEAN
MatchUsbWwid (
  IN EFI_USB_IO_PROTOCOL   *UsbIo,
  IN USB_WWID_DEVICE_PATH  *UsbWwid
  )
{
  EFI_STATUS                    Status;
  EFI_USB_DEVICE_DESCRIPTOR     DevDesc;
  EFI_USB_INTERFACE_DESCRIPTOR  IfDesc;
  UINT16                        *LangIdTable;
  UINT16                        TableSize;
  UINT16                        Index;
  CHAR16                        *CompareStr;
  UINTN                         CompareLen;
  CHAR16                        *SerialNumberStr;
  UINTN                         Length;

  if ((DevicePathType (UsbWwid) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (UsbWwid) != MSG_USB_WWID_DP))
  {
    return FALSE;
  }

  //
  // Check Vendor Id and Product Id.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((DevDesc.IdVendor != UsbWwid->VendorId) ||
      (DevDesc.IdProduct != UsbWwid->ProductId))
  {
    return FALSE;
  }

  //
  // Check Interface Number.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IfDesc);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (IfDesc.InterfaceNumber != UsbWwid->InterfaceNumber) {
    return FALSE;
  }

  //
  // Check Serial Number.
  //
  if (DevDesc.StrSerialNumber == 0) {
    return FALSE;
  }

  //
  // Get all supported languages.
  //
  TableSize   = 0;
  LangIdTable = NULL;
  Status      = UsbIo->UsbGetSupportedLanguages (UsbIo, &LangIdTable, &TableSize);
  if (EFI_ERROR (Status) || (TableSize == 0) || (LangIdTable == NULL)) {
    return FALSE;
  }

  //
  // Serial number in USB WWID device path is the last 64-or-less UTF-16 characters.
  //
  CompareStr = (CHAR16 *)(UINTN)(UsbWwid + 1);
  CompareLen = (DevicePathNodeLength (UsbWwid) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16);
  if (CompareStr[CompareLen - 1] == L'\0') {
    CompareLen--;
  }

  //
  // Compare serial number in each supported language.
  //
  for (Index = 0; Index < TableSize / sizeof (UINT16); Index++) {
    SerialNumberStr = NULL;
    Status          = UsbIo->UsbGetStringDescriptor (
                               UsbIo,
                               LangIdTable[Index],
                               DevDesc.StrSerialNumber,
                               &SerialNumberStr
                               );
    if (EFI_ERROR (Status) || (SerialNumberStr == NULL)) {
      continue;
    }

    Length = StrLen (SerialNumberStr);
    if ((Length >= CompareLen) &&
        (CompareMem (SerialNumberStr + Length - CompareLen, CompareStr, CompareLen * sizeof (CHAR16)) == 0))
    {
      FreePool (SerialNumberStr);
      return TRUE;
    }

    FreePool (SerialNumberStr);
  }

  return FALSE;
}

/**
  Compare whether a full console device path matches a USB shortform device path.

  @param[in] FullPath      Full console device path.
  @param[in] ShortformPath Short-form device path. Short-form device node may in the beginning or in the middle.

  @retval TRUE  The full console device path matches the short-form device path.
  @retval FALSE The full console device path doesn't match the short-form device path.
**/
BOOLEAN
MatchUsbShortformDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *FullPath,
  IN EFI_DEVICE_PATH_PROTOCOL  *ShortformPath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ShortformNode;
  UINTN                     ParentDevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_HANDLE                Handle;

  for ( ShortformNode = ShortformPath
        ; !IsDevicePathEnd (ShortformNode)
        ; ShortformNode = NextDevicePathNode (ShortformNode)
        )
  {
    if ((DevicePathType (ShortformNode) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (ShortformNode) == MSG_USB_CLASS_DP) ||
         (DevicePathSubType (ShortformNode) == MSG_USB_WWID_DP))
        )
    {
      break;
    }
  }

  //
  // Skip further compare when it's not a shortform device path.
  //
  if (IsDevicePathEnd (ShortformNode)) {
    return FALSE;
  }

  //
  // Compare the parent device path when the ShortformPath doesn't start with short-form node.
  //
  ParentDevicePathSize = (UINTN)ShortformNode - (UINTN)ShortformPath;
  RemainingDevicePath  = FullPath;
  Status               = gBS->LocateDevicePath (&gEfiUsbIoProtocolGuid, &RemainingDevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (ParentDevicePathSize != 0) {
    if ((ParentDevicePathSize > (UINTN)RemainingDevicePath - (UINTN)FullPath) ||
        (CompareMem (FullPath, ShortformPath, ParentDevicePathSize) != 0))
    {
      return FALSE;
    }
  }

  //
  // Compar the USB layer.
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo
                  );
  ASSERT_EFI_ERROR (Status);

  return MatchUsbClass (UsbIo, (USB_CLASS_DEVICE_PATH *)ShortformNode) ||
         MatchUsbWwid (UsbIo, (USB_WWID_DEVICE_PATH *)ShortformNode);
}

/**
  Function compares a device path data structure to that of all the nodes of a
  second device path instance.

  @param Multi           A pointer to a multi-instance device path data structure.
  @param Single          A pointer to a single-instance device path data structure.
  @param NewDevicePath   If Delete is TRUE, this parameter must not be null, and it
                         points to the remaining device path data structure.
                         (remaining device path = Multi - Single.)
  @param Delete          If TRUE, means removing Single from Multi.
                         If FALSE, the routine just check whether Single matches
                         with any instance in Multi.

  @retval EFI_SUCCESS           If the Single is contained within Multi.
  @retval EFI_NOT_FOUND         If the Single is not contained within Multi.
  @retval EFI_INVALID_PARAMETER Multi is NULL.
  @retval EFI_INVALID_PARAMETER Single is NULL.
  @retval EFI_INVALID_PARAMETER NewDevicePath is NULL when Delete is TRUE.

**/
EFI_STATUS
ConPlatformMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single,
  OUT EFI_DEVICE_PATH_PROTOCOL  **NewDevicePath OPTIONAL,
  IN  BOOLEAN                   Delete
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath1;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath2;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  UINTN                     Size;

  //
  // The passed in DevicePath should not be NULL
  //
  if ((Multi == NULL) || (Single == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If performing Delete operation, the NewDevicePath must not be NULL.
  //
  if (Delete) {
    if (NewDevicePath == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }

  TempDevicePath1 = NULL;

  DevicePath     = Multi;
  DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);

  //
  // Search for the match of 'Single' in 'Multi'
  //
  while (DevicePathInst != NULL) {
    if ((CompareMem (Single, DevicePathInst, Size) == 0) ||
        IsGopSibling (Single, DevicePathInst) || MatchUsbShortformDevicePath (Single, DevicePathInst))
    {
      if (!Delete) {
        //
        // If Delete is FALSE, return EFI_SUCCESS if Single is found in Multi.
        //
        FreePool (DevicePathInst);
        return EFI_SUCCESS;
      }
    } else {
      if (Delete) {
        //
        // If the node of Multi does not match Single, then added it back to the result.
        // That is, the node matching Single will be dropped and deleted from result.
        //
        TempDevicePath2 = AppendDevicePathInstance (
                            TempDevicePath1,
                            DevicePathInst
                            );
        if (TempDevicePath1 != NULL) {
          FreePool (TempDevicePath1);
        }

        TempDevicePath1 = TempDevicePath2;
      }
    }

    FreePool (DevicePathInst);
    DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);
  }

  if (Delete) {
    //
    // Return the new device path data structure with specified node deleted.
    //
    *NewDevicePath = TempDevicePath1;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Update console environment variables.

  @param  VariableName    Console environment variables, ConOutDev, ConInDev
                          ErrOutDev, ConIn ,ConOut or ErrOut.
  @param  DevicePath      Console devcie's device path.
  @param  Operation       Variable operations, including APPEND, CHECK and DELETE.

  @retval EFI_SUCCESS           Variable operates successfully.
  @retval EFI_OUT_OF_RESOURCES  If variable cannot be appended.
  @retval other                 Variable updating failed.

**/
EFI_STATUS
ConPlatformUpdateDeviceVariable (
  IN  CHAR16                     *VariableName,
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN  CONPLATFORM_VAR_OPERATION  Operation
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *VariableDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *NewVariableDevicePath;

  VariableDevicePath    = NULL;
  NewVariableDevicePath = NULL;

  //
  // Get Variable according to variable name.
  // The memory for Variable is allocated within ConPlatformGetVarible(),
  // it is the caller's responsibility to free the memory before return.
  //
  VariableDevicePath = ConPlatformGetVariable (VariableName);

  if (Operation != Delete) {
    //
    // Match specified DevicePath in Console Variable.
    //
    Status = ConPlatformMatchDevicePaths (
               VariableDevicePath,
               DevicePath,
               NULL,
               FALSE
               );

    if ((Operation == Check) || (!EFI_ERROR (Status))) {
      //
      // Branch here includes 2 cases:
      // 1. Operation is CHECK, simply return Status.
      // 2. Operation is APPEND, and device path already exists in variable, also return.
      //
      if (VariableDevicePath != NULL) {
        FreePool (VariableDevicePath);
      }

      return Status;
    }

    //
    // We reach here to append a device path that does not exist in variable.
    //
    Status                = EFI_SUCCESS;
    NewVariableDevicePath = AppendDevicePathInstance (
                              VariableDevicePath,
                              DevicePath
                              );
    if (NewVariableDevicePath == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // We reach here to remove DevicePath from the environment variable that
    // is a multi-instance device path.
    //
    Status = ConPlatformMatchDevicePaths (
               VariableDevicePath,
               DevicePath,
               &NewVariableDevicePath,
               TRUE
               );
  }

  if (VariableDevicePath != NULL) {
    FreePool (VariableDevicePath);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (NewVariableDevicePath != NULL) {
    //
    // Update Console Environment Variable.
    //
    Status = gRT->SetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    GetDevicePathSize (NewVariableDevicePath),
                    NewVariableDevicePath
                    );

    FreePool (NewVariableDevicePath);
  }

  return Status;
}

/**
  Update ConOutDev and ErrOutDev variables to add the device path of
  GOP controller itself and the sibling controllers.

  @param  DevicePath            Pointer to device's device path.

  @retval TRUE                  The devcie is a GOP device.
  @retval FALSE                 The devcie is not a GOP device.

**/
BOOLEAN
ConPlatformUpdateGopCandidate (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                PciHandle;
  EFI_HANDLE                GopHandle;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;

  //
  // Check whether it's a GOP device.
  //
  TempDevicePath = DevicePath;
  Status         = gBS->LocateDevicePath (&gEfiGraphicsOutputProtocolGuid, &TempDevicePath, &GopHandle);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Get the parent PciIo handle in order to find all the children
  //
  Status = gBS->LocateDevicePath (&gEfiPciIoProtocolGuid, &DevicePath, &PciHandle);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  TempDevicePath = EfiBootManagerGetGopDevicePath (PciHandle);
  if (TempDevicePath != NULL) {
    ConPlatformUpdateDeviceVariable (L"ConOutDev", TempDevicePath, Append);
    ConPlatformUpdateDeviceVariable (L"ErrOutDev", TempDevicePath, Append);
  }

  return TRUE;
}
