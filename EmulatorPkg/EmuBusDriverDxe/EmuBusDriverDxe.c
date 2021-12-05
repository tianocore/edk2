/** @file
 Emu Bus driver

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "EmuBusDriverDxe.h"

//
// DriverBinding protocol global
//
EFI_DRIVER_BINDING_PROTOCOL  gEmuBusDriverBinding = {
  EmuBusDriverBindingSupported,
  EmuBusDriverBindingStart,
  EmuBusDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
EmuBusDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EMU_THUNK_PROTOCOL        *EmuThunk;

  //
  // Check the contents of the first Device Path Node of RemainingDevicePath to make sure
  // it is a legal Device Path Node for this bus driver's children.
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      if ((RemainingDevicePath->Type != HARDWARE_DEVICE_PATH) ||
          (RemainingDevicePath->SubType != HW_VENDOR_DP) ||
          (DevicePathNodeLength (RemainingDevicePath) != sizeof (EMU_VENDOR_DEVICE_PATH_NODE)))
      {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuThunkProtocolGuid,
                  (VOID **)&EmuThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEmuThunkProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  //
  // Open the EFI Device Path protocol needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close protocol, don't use device path protocol in the Support() function
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}

EFI_STATUS
EFIAPI
EmuBusDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   InstallStatus;
  EMU_THUNK_PROTOCOL           *EmuThunk;
  EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath;
  EMU_IO_DEVICE                *EmuDevice;
  EMU_BUS_DEVICE               *EmuBusDevice;
  EMU_IO_THUNK_PROTOCOL        *EmuIoThunk;
  UINT16                       ComponentName[512];
  EMU_VENDOR_DEVICE_PATH_NODE  *Node;
  BOOLEAN                      CreateDevice;

  InstallStatus = EFI_UNSUPPORTED;
  Status        = EFI_UNSUPPORTED;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuThunkProtocolGuid,
                  (VOID **)&EmuThunk,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  if (Status != EFI_ALREADY_STARTED) {
    EmuBusDevice = AllocatePool (sizeof (EMU_BUS_DEVICE));
    if (EmuBusDevice == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    EmuBusDevice->Signature           = EMU_BUS_DEVICE_SIGNATURE;
    EmuBusDevice->ControllerNameTable = NULL;

    AddUnicodeString2 (
      "eng",
      gEmuBusDriverComponentName.SupportedLanguages,
      &EmuBusDevice->ControllerNameTable,
      L"Emulator Bus Controller",
      TRUE
      );
    AddUnicodeString2 (
      "en",
      gEmuBusDriverComponentName2.SupportedLanguages,
      &EmuBusDevice->ControllerNameTable,
      L"Emulator Bus Controller",
      FALSE
      );

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gEfiCallerIdGuid,
                    EmuBusDevice,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      FreeUnicodeStringTable (EmuBusDevice->ControllerNameTable);
      gBS->FreePool (EmuBusDevice);
      return Status;
    }
  }

  for (Status = EFI_SUCCESS, EmuIoThunk = NULL; !EFI_ERROR (Status); ) {
    Status = EmuThunk->GetNextProtocol (TRUE, &EmuIoThunk);
    if (EFI_ERROR (Status)) {
      break;
    }

    CreateDevice = TRUE;
    if (RemainingDevicePath != NULL) {
      CreateDevice = FALSE;
      //
      // Check if RemainingDevicePath is the End of Device Path Node,
      // if yes, don't create any child device
      //
      if (!IsDevicePathEnd (RemainingDevicePath)) {
        //
        // If RemainingDevicePath isn't the End of Device Path Node,
        // check its validation
        //
        Node = (EMU_VENDOR_DEVICE_PATH_NODE *)RemainingDevicePath;
        if ((Node->VendorDevicePath.Header.Type == HARDWARE_DEVICE_PATH) &&
            (Node->VendorDevicePath.Header.SubType == HW_VENDOR_DP) &&
            (DevicePathNodeLength (&Node->VendorDevicePath.Header) == sizeof (EMU_VENDOR_DEVICE_PATH_NODE))
            )
        {
          if (CompareGuid (&Node->VendorDevicePath.Guid, EmuIoThunk->Protocol) && (Node->Instance == EmuIoThunk->Instance)) {
            CreateDevice = TRUE;
          }
        }
      }
    }

    if (CreateDevice) {
      //
      // Allocate instance structure, and fill in parent information.
      //
      EmuDevice = AllocatePool (sizeof (EMU_IO_DEVICE));
      if (EmuDevice == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      EmuDevice->Handle           = NULL;
      EmuDevice->ControllerHandle = ControllerHandle;
      EmuDevice->ParentDevicePath = ParentDevicePath;
      CopyMem (&EmuDevice->EmuIoThunk, EmuIoThunk, sizeof (EMU_IO_THUNK_PROTOCOL));

      EmuDevice->ControllerNameTable = NULL;

      StrnCpyS (
        ComponentName,
        sizeof (ComponentName) / sizeof (CHAR16),
        EmuIoThunk->ConfigString,
        sizeof (ComponentName) / sizeof (CHAR16)
        );

      EmuDevice->DevicePath = EmuBusCreateDevicePath (
                                ParentDevicePath,
                                EmuIoThunk->Protocol,
                                EmuIoThunk->Instance
                                );
      if (EmuDevice->DevicePath == NULL) {
        gBS->FreePool (EmuDevice);
        return EFI_OUT_OF_RESOURCES;
      }

      AddUnicodeString (
        "eng",
        gEmuBusDriverComponentName.SupportedLanguages,
        &EmuDevice->ControllerNameTable,
        ComponentName
        );

      EmuDevice->Signature = EMU_IO_DEVICE_SIGNATURE;

      InstallStatus = gBS->InstallMultipleProtocolInterfaces (
                             &EmuDevice->Handle,
                             &gEfiDevicePathProtocolGuid,
                             EmuDevice->DevicePath,
                             &gEmuIoThunkProtocolGuid,
                             &EmuDevice->EmuIoThunk,
                             NULL
                             );
      if (EFI_ERROR (InstallStatus)) {
        FreeUnicodeStringTable (EmuDevice->ControllerNameTable);
        gBS->FreePool (EmuDevice);
      } else {
        //
        // Open For Child Device
        //
        Status = gBS->OpenProtocol (
                        ControllerHandle,
                        &gEmuThunkProtocolGuid,
                        (VOID **)&EmuThunk,
                        This->DriverBindingHandle,
                        EmuDevice->Handle,
                        EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                        );
        if (!EFI_ERROR (Status)) {
          InstallStatus = EFI_SUCCESS;
        }
      }
    }
  }

  return InstallStatus;
}

EFI_STATUS
EFIAPI
EmuBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS             Status;
  UINTN                  Index;
  BOOLEAN                AllChildrenStopped;
  EMU_IO_THUNK_PROTOCOL  *EmuIoThunk;
  EMU_BUS_DEVICE         *EmuBusDevice;
  EMU_IO_DEVICE          *EmuDevice;
  EMU_THUNK_PROTOCOL     *EmuThunk;

  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiCallerIdGuid,
                    (VOID **)&EmuBusDevice,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    gBS->UninstallMultipleProtocolInterfaces (
           ControllerHandle,
           &gEfiCallerIdGuid,
           EmuBusDevice,
           NULL
           );

    FreeUnicodeStringTable (EmuBusDevice->ControllerNameTable);

    gBS->FreePool (EmuBusDevice);

    gBS->CloseProtocol (
           ControllerHandle,
           &gEmuThunkProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    return EFI_SUCCESS;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEmuIoThunkProtocolGuid,
                    (VOID **)&EmuIoThunk,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      EmuDevice = EMU_IO_DEVICE_FROM_THIS (EmuIoThunk);

      Status = gBS->CloseProtocol (
                      ControllerHandle,
                      &gEmuThunkProtocolGuid,
                      This->DriverBindingHandle,
                      EmuDevice->Handle
                      );

      Status = gBS->UninstallMultipleProtocolInterfaces (
                      EmuDevice->Handle,
                      &gEfiDevicePathProtocolGuid,
                      EmuDevice->DevicePath,
                      &gEmuIoThunkProtocolGuid,
                      &EmuDevice->EmuIoThunk,
                      NULL
                      );

      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
               ControllerHandle,
               &gEmuThunkProtocolGuid,
               (VOID **)&EmuThunk,
               This->DriverBindingHandle,
               EmuDevice->Handle,
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      } else {
        //
        // Close the child handle
        //
        FreeUnicodeStringTable (EmuDevice->ControllerNameTable);
        FreePool (EmuDevice);
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/*++

Routine Description:
  Create a device path node using Guid and InstanceNumber and append it to
  the passed in RootDevicePath

Arguments:
  RootDevicePath - Root of the device path to return.

  Guid           - GUID to use in vendor device path node.

  InstanceNumber - Instance number to use in the vendor device path. This
                    argument is needed to make sure each device path is unique.

Returns:

  EFI_DEVICE_PATH_PROTOCOL

**/
EFI_DEVICE_PATH_PROTOCOL *
EmuBusCreateDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDevicePath,
  IN  EFI_GUID                  *Guid,
  IN  UINT16                    InstanceNumber
  )
{
  EMU_VENDOR_DEVICE_PATH_NODE  DevicePath;

  DevicePath.VendorDevicePath.Header.Type    = HARDWARE_DEVICE_PATH;
  DevicePath.VendorDevicePath.Header.SubType = HW_VENDOR_DP;
  SetDevicePathNodeLength (&DevicePath.VendorDevicePath.Header, sizeof (EMU_VENDOR_DEVICE_PATH_NODE));

  //
  // The GUID defines the Class
  //
  CopyMem (&DevicePath.VendorDevicePath.Guid, Guid, sizeof (EFI_GUID));

  //
  // Add an instance number so we can make sure there are no Device Path
  // duplication.
  //
  DevicePath.Instance = InstanceNumber;

  return AppendDevicePathNode (
           RootDevicePath,
           (EFI_DEVICE_PATH_PROTOCOL *)&DevicePath
           );
}

/**
  The user Entry Point for module EmuBusDriver. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeEmuBusDriver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallAllDriverProtocols (
             ImageHandle,
             SystemTable,
             &gEmuBusDriverBinding,
             ImageHandle,
             &gEmuBusDriverComponentName,
             NULL,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
