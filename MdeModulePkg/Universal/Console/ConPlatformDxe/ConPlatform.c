/** @file
  Console Platform DXE Driver, install Console Device Guids and update Console
  Environment Variables.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ConPlatform.h"


EFI_DRIVER_BINDING_PROTOCOL gConPlatformTextInDriverBinding = {
  ConPlatformTextInDriverBindingSupported,
  ConPlatformTextInDriverBindingStart,
  ConPlatformTextInDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL gConPlatformTextOutDriverBinding = {
  ConPlatformTextOutDriverBindingSupported,
  ConPlatformTextOutDriverBindingStart,
  ConPlatformTextOutDriverBindingStop,
  0xa,
  NULL,
  NULL
};

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
InitializeConPlatform(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

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
                  (VOID **) &Interface,
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

  If this devcie is not one hot-plug devce, append its device path into the 
  console environment variables ConInDev.
  
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
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *TextIn;
  BOOLEAN                        IsInConInVariable;

  //
  // Get the Device Path Protocol so the environment variables can be updated
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
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
                  (VOID **) &TextIn,
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
  Status = ConPlatformUpdateDeviceVariable (
             L"ConIn",
             DevicePath,
             Check
             );
  if (!EFI_ERROR (Status)) {
    IsInConInVariable = TRUE;
  }

  //
  // Check the device path, if it is a hot plug device,
  // do not put the device path into ConInDev, and install
  // gEfiConsoleInDeviceGuid to the device handle directly.
  // The policy is, make hot plug device plug in and play immediately.
  //
  if (IsHotPlugDevice (DevicePath)) {
    gBS->InstallMultipleProtocolInterfaces (
           &ControllerHandle,
           &gEfiConsoleInDeviceGuid,
           NULL,
           NULL
           );
    //
    // Append the device path to ConInDev only if it is in ConIn variable.
    //
    if (IsInConInVariable) {
      ConPlatformUpdateDeviceVariable (
        L"ConInDev",
        DevicePath,
        Append
        );
    }
  } else {
    //
    // If it is not a hot-plug device, append the device path to the
    // ConInDev environment variable
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
  }

  return EFI_SUCCESS;
}

/**
  Start this driver on the device for console output and standard error output.

  Start this driver on ControllerHandle by opening Simple Text Output Protocol,
  reading Device Path, and installing Console Out Devcic GUID, Standard Error
  Device GUID on ControllerHandle.

  If this devcie is not one hot-plug devce, append its device path into the 
  console environment variables ConOutDev, ErrOutDev.
  
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
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
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
                  (VOID **) &DevicePath,
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
                  (VOID **) &TextOut,
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
  Status = ConPlatformUpdateDeviceVariable (
             L"ConOut",
             DevicePath,
             Check
             );
  if (!EFI_ERROR (Status)) {
    IsInConOutVariable = TRUE;
  }

  IsInErrOutVariable = FALSE;
  Status = ConPlatformUpdateDeviceVariable (
             L"ErrOut",
             DevicePath,
             Check
             );
  if (!EFI_ERROR (Status)) {
    IsInErrOutVariable = TRUE;
  }

  //
  // Check the device path, if it is a hot plug device,
  // do not put the device path into ConOutDev and ErrOutDev,
  // and install gEfiConsoleOutDeviceGuid to the device handle directly.
  // The policy is, make hot plug device plug in and play immediately.
  //
  if (IsHotPlugDevice (DevicePath)) {
    gBS->InstallMultipleProtocolInterfaces (
           &ControllerHandle,
           &gEfiConsoleOutDeviceGuid,
           NULL,
           NULL
           );
    //
    // Append the device path to ConOutDev only if it is in ConOut variable.
    //
    if (IsInConOutVariable) {
      ConPlatformUpdateDeviceVariable (
        L"ConOutDev",
        DevicePath,
        Append
        );
    }
    //
    // Append the device path to ErrOutDev only if it is in ErrOut variable.
    //
    if (IsInErrOutVariable) {
      ConPlatformUpdateDeviceVariable (
        L"ErrOutDev",
        DevicePath,
        Append
        );
    }
  } else {
    //
    // If it is not a hot-plug device, append the device path to 
    // the ConOutDev and ErrOutDev environment variable.
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
      Status = gBS->InstallMultipleProtocolInterfaces (
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
                  (VOID **) &DevicePath,
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
                  (VOID **) &DevicePath,
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

  return ;
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
  IN  CHAR16    *Name
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer;
  UINTN       BufferSize;

  BufferSize  = 0;
  Buffer      = NULL;

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
    if ((DevicePathType (NodeLeft) == ACPI_DEVICE_PATH && DevicePathSubType (NodeLeft) == ACPI_ADR_DP) ||
        (DevicePathType (NodeLeft) == HARDWARE_DEVICE_PATH && DevicePathSubType (NodeLeft) == HW_CONTROLLER_DP &&
         DevicePathType (NextDevicePathNode (NodeLeft)) == ACPI_DEVICE_PATH && DevicePathSubType (NextDevicePathNode (NodeLeft)) == ACPI_ADR_DP)) {
      break;
    }
  }

  if (IsDevicePathEndType (NodeLeft)) {
    return FALSE;
  }

  for (NodeRight = Right; !IsDevicePathEndType (NodeRight); NodeRight = NextDevicePathNode (NodeRight)) {
    if ((DevicePathType (NodeRight) == ACPI_DEVICE_PATH && DevicePathSubType (NodeRight) == ACPI_ADR_DP) ||
        (DevicePathType (NodeRight) == HARDWARE_DEVICE_PATH && DevicePathSubType (NodeRight) == HW_CONTROLLER_DP &&
         DevicePathType (NextDevicePathNode (NodeRight)) == ACPI_DEVICE_PATH && DevicePathSubType (NextDevicePathNode (NodeRight)) == ACPI_ADR_DP)) {
      break;
    }
  }

  if (IsDevicePathEndType (NodeRight)) {
    return FALSE;
  }

  if (((UINTN) NodeLeft - (UINTN) Left) != ((UINTN) NodeRight - (UINTN) Right)) {
    return FALSE;
  }

  return (BOOLEAN) (CompareMem (Left, Right, (UINTN) NodeLeft - (UINTN) Left) == 0);
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

  DevicePath      = Multi;
  DevicePathInst  = GetNextDevicePathInstance (&DevicePath, &Size);

  //
  // Search for the match of 'Single' in 'Multi'
  //
  while (DevicePathInst != NULL) {
    if ((CompareMem (Single, DevicePathInst, Size) == 0) || IsGopSibling (Single, DevicePathInst)) {
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
  IN  CHAR16                    *VariableName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN  CONPLATFORM_VAR_OPERATION Operation
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
    Status = EFI_SUCCESS;
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
  Check if the device supports hot-plug through its device path.

  This function could be updated to check more types of Hot Plug devices.
  Currently, it checks USB and PCCard device.

  @param  DevicePath            Pointer to device's device path.

  @retval TRUE                  The devcie is a hot-plug device
  @retval FALSE                 The devcie is not a hot-plug device.

**/
BOOLEAN
IsHotPlugDevice (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL     *CheckDevicePath;

  CheckDevicePath = DevicePath;
  while (!IsDevicePathEnd (CheckDevicePath)) {
    //
    // Check device whether is hot plug device or not throught Device Path
    // 
    if ((DevicePathType (CheckDevicePath) == MESSAGING_DEVICE_PATH) &&
        (DevicePathSubType (CheckDevicePath) == MSG_USB_DP ||
         DevicePathSubType (CheckDevicePath) == MSG_USB_CLASS_DP ||
         DevicePathSubType (CheckDevicePath) == MSG_USB_WWID_DP)) {
      //
      // If Device is USB device
      //
      return TRUE;
    }
    if ((DevicePathType (CheckDevicePath) == HARDWARE_DEVICE_PATH) &&
        (DevicePathSubType (CheckDevicePath) == HW_PCCARD_DP)) {
      //
      // If Device is PCCard
      //
      return TRUE;
    }
  
    CheckDevicePath = NextDevicePathNode (CheckDevicePath);
  }

  return FALSE;
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
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  EFI_STATUS                           Status;
  EFI_HANDLE                           PciHandle;
  EFI_HANDLE                           GopHandle;
  EFI_DEVICE_PATH_PROTOCOL             *TempDevicePath;

  //
  // Check whether it's a GOP device.
  //
  TempDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiGraphicsOutputProtocolGuid, &TempDevicePath, &GopHandle);
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
