/** @file
  Console Platfrom DXE Driver, install Console Device Guids and update Console
  Environment Variables.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <ConPlatform.h>


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
  The user Entry Point for module ConPlatform. The user code starts with this function.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeConPlatform(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
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


  return Status;
}


/**
  Test to see if EFI Text In Protocol could be supported on the ControllerHandle. 

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
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConPlatformDriverBindingSupported (
          This,
          ControllerHandle,
          RemainingDevicePath,
          &gEfiSimpleTextInProtocolGuid
          );
}


/**
  Test to see if EFI Text Out Protocol could be supported on the ControllerHandle. 

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
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  return ConPlatformDriverBindingSupported (
          This,
          ControllerHandle,
          RemainingDevicePath,
          &gEfiSimpleTextOutProtocolGuid
          );
}


/**
  Test to see if the specified Protocol could be supported on the ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test.
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.
  @param  ProtocolGuid        The specfic protocol.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
ConPlatformDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath,
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
  Start this driver on ControllerHandle by opening Simple Text In protocol,
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
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *TextIn;

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
  // Open the Simple Input Protocol BY_DRIVER
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
  // Check the device handle, if it is a hot plug device,
  // do not put the device path into ConInDev, and install
  // gEfiConsoleInDeviceGuid to the device handle directly.
  // The policy is, make hot plug device plug in and play immediately.
  //
  if (IsHotPlugDevice (This->DriverBindingHandle, ControllerHandle)) {
    gBS->InstallMultipleProtocolInterfaces (
          &ControllerHandle,
          &gEfiConsoleInDeviceGuid,
          NULL,
          NULL
          );
  } else {
    //
    // Append the device path to the ConInDev environment variable
    //
    ConPlatformUpdateDeviceVariable (
      L"ConInDev",
      DevicePath,
      APPEND
      );

    //
    // If the device path is an instance in the ConIn environment variable,
    // then install EfiConsoleInDeviceGuid onto ControllerHandle
    //
    Status = ConPlatformUpdateDeviceVariable (
              L"ConIn",
              DevicePath,
              CHECK
              );

    if (!EFI_ERROR (Status)) {
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
  Start this driver on ControllerHandle by opening Simple Text Out protocol,
  reading Device Path, and installing Console Out Devcic GUID, Standard Error
  Device GUID on ControllerHandle.

  If this devcie is not one hot-plug devce, append its device path into the 
  console environment variables ConOutDev, StdErrDev.
  
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
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *TextOut;
  BOOLEAN                       NeedClose;

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
  // Check the device handle, if it is a hot plug device,
  // do not put the device path into ConOutDev and StdErrDev,
  // and install gEfiConsoleOutDeviceGuid to the device handle directly.
  // The policy is, make hot plug device plug in and play immediately.
  //
  if (IsHotPlugDevice (This->DriverBindingHandle, ControllerHandle)) {
    gBS->InstallMultipleProtocolInterfaces (
          &ControllerHandle,
          &gEfiConsoleOutDeviceGuid,
          NULL,
          NULL
          );
  } else {
    //
    // Append the device path to the ConOutDev environment variable
    //
    ConPlatformUpdateDeviceVariable (
      L"ConOutDev",
      DevicePath,
      APPEND
      );
    //
    // Append the device path to the StdErrDev environment variable
    //
    ConPlatformUpdateDeviceVariable (
      L"ErrOutDev",
      DevicePath,
      APPEND
      );

    //
    // If the device path is an instance in the ConOut environment variable,
    // then install EfiConsoleOutDeviceGuid onto ControllerHandle
    //
    Status = ConPlatformUpdateDeviceVariable (
              L"ConOut",
              DevicePath,
              CHECK
              );

    if (!EFI_ERROR (Status)) {
      NeedClose = FALSE;
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &ControllerHandle,
                      &gEfiConsoleOutDeviceGuid,
                      NULL,
                      NULL
                      );
    }
    //
    // If the device path is an instance in the StdErr environment variable,
    // then install EfiStandardErrorDeviceGuid onto ControllerHandle
    //
    Status = ConPlatformUpdateDeviceVariable (
              L"ErrOut",
              DevicePath,
              CHECK
              );
    if (!EFI_ERROR (Status)) {
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
  and closing the Simple Text In protocol on ControllerHandle.

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
  // hot plug device is not included into the console associated variables,
  // so no need to check variable for those hot plug devices.
  //
  if (!IsHotPlugDevice (This->DriverBindingHandle, ControllerHandle)) {
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
    if (!EFI_ERROR (Status)) {
      //
      // Remove DevicePath from ConInDev
      //
      ConPlatformUpdateDeviceVariable (
        L"ConInDev",
        DevicePath,
        DELETE
        );
    }
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
  // Close the Simple Input Protocol
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
  and closing the Simple Text Out protocol on ControllerHandle.

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
  // hot plug device is not included into the console associated variables,
  // so no need to check variable for those hot plug devices.
  //
  if (!IsHotPlugDevice (This->DriverBindingHandle, ControllerHandle)) {
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
    if (!EFI_ERROR (Status)) {
      //
      // Remove DevicePath from ConOutDev, and StdErrDev
      //
      ConPlatformUpdateDeviceVariable (
        L"ConOutDev",
        DevicePath,
        DELETE
        );
      ConPlatformUpdateDeviceVariable (
        L"ErrOutDev",
        DevicePath,
        DELETE
        );
    }
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

  @return None.

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
  Read the EFI variable (Name) and return a dynamically allocated
  buffer, and the size of the buffer. On failure return NULL.

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
  // Test to see if the variable exists.  If it doesn't, reuturn NULL.
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

  @retval EFI_SUCCESS    If the Single is contained within Multi.
  @retval EFI_NOT_FOUND  If the Single is not contained within Multi.

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
    return EFI_NOT_FOUND;
  }

  //
  // If performing Delete operation, the NewDevicePath must not be NULL.
  //
  if (Delete) {
    ASSERT (NewDevicePath != NULL);
  }

  TempDevicePath1 = NULL;

  DevicePath      = Multi;
  DevicePathInst  = GetNextDevicePathInstance (&DevicePath, &Size);

  //
  // Search for the match of 'Single' in 'Multi'
  //
  while (DevicePathInst != NULL) {
    if (CompareMem (Single, DevicePathInst, Size) == 0) {
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
        // Append the mis-matched devcie path into remaining device path.
        //
        TempDevicePath2 = AppendDevicePathInstance (
                            TempDevicePath1,
                            DevicePathInst
                            );
        SafeFreePool (TempDevicePath1);
        TempDevicePath1 = TempDevicePath2;
      }
    }

    FreePool (DevicePathInst);
    DevicePathInst = GetNextDevicePathInstance (&DevicePath, &Size);
  }

  if (Delete) {
    //
    // Return the remaining device path data structure
    //
    *NewDevicePath = TempDevicePath1;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Update console environment variables. 

  @param  VariableName    Console environment variables, ConOutDev, ConInDev
                          StdErrDev, ConIn or ConOut.
  @param  DevicePath      Console devcie's device path.
  @param  Operation       Variable operations, such as APPEND or DELETE.

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

  if (Operation != DELETE) {
    //
    // Match specified DevicePath in Console Variable.
    // 
    Status = ConPlatformMatchDevicePaths (
              VariableDevicePath,
              DevicePath,
              NULL,
              FALSE
              );

    if ((Operation == CHECK) || (!EFI_ERROR (Status))) {
      //
      // The device path is already in the variable
      //
      if (VariableDevicePath != NULL) {
        FreePool (VariableDevicePath);
      }

      return Status;
    }
    //
    // The device path is not in variable. Append DevicePath to the
    // environment variable that is a multi-instance device path.
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
    // Remove DevicePath from the environment variable that
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
  Check if the device is one hot-plug supported.

  @param  DriverBindingHandle   Protocol instance pointer.
  @param  ControllerHandle      Handle of device to check.

  @retval TRUE                  The devcie is a hot-plug device
  @retval FALSE                 The devcie is not a hot-plug device.

**/
BOOLEAN
IsHotPlugDevice (
  EFI_HANDLE    DriverBindingHandle,
  EFI_HANDLE    ControllerHandle
  )
{
  EFI_STATUS  Status;

  //
  // HotPlugDeviceGuid indicates ControllerHandle stands for a hot plug device.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiHotPlugDeviceGuid,
                  NULL,
                  DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}
