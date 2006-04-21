/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BdsConsole.c

Abstract:

  BDS Lib functions which contain all the code to connect console device

--*/

EFI_STATUS
BdsLibUpdateConsoleVariable (
  IN  CHAR16                    *ConVarName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
/*++

Routine Description:

  This function update console variable based on ConVarName, it can 
  add or remove one specific console device path from the variable

Arguments:

  ConVarName   - Console related variable name, ConIn, ConOut, ErrOut.

  CustomizedConDevicePath - The console device path which will be added to
                            the console variable ConVarName, this parameter
                            can not be multi-instance.

  ExclusiveDevicePath     - The console device path which will be removed
                            from the console variable ConVarName, this
                            parameter can not be multi-instance.

Returns:

  EFI_UNSUPPORTED         - Add or remove the same device path.
  
  EFI_SUCCESS             - Success add or remove the device path from 
                            the console variable.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;

  VarConsole      = NULL;
  DevicePathSize  = 0;
  NewDevicePath   = NULL;
  Status          = EFI_UNSUPPORTED;

  //
  // Notes: check the device path point, here should check
  // with compare memory
  //
  if (CustomizedConDevicePath == ExclusiveDevicePath) {
    return EFI_UNSUPPORTED;
  }
  //
  // Delete the ExclusiveDevicePath from current default console
  //
  VarConsole = BdsLibGetVariableAndSize (
                ConVarName,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );

  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
    if (BdsLibMatchDevicePaths (VarConsole, ExclusiveDevicePath)) {

      Instance = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);

      while (VarConsole != NULL) {
        if (CompareMem (
              Instance,
              ExclusiveDevicePath,
              DevicePathSize - sizeof (EFI_DEVICE_PATH_PROTOCOL)
              ) == 0) {
          //
          // Remove the match part
          //
          NewDevicePath = AppendDevicePathInstance (NewDevicePath, VarConsole);
          break;
        } else {
          //
          // Continue the next instance
          //
          NewDevicePath = AppendDevicePathInstance (NewDevicePath, Instance);
        }

        Instance = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);
      }
      //
      // Reset the console variable with new device path
      //
      gRT->SetVariable (
            ConVarName,
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
            GetDevicePathSize (NewDevicePath),
            NewDevicePath
            );
    }
  }
  //
  // Try to append customized device path
  //
  VarConsole = BdsLibGetVariableAndSize (
                ConVarName,
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );

  if (CustomizedConDevicePath != NULL) {
    if (!BdsLibMatchDevicePaths (VarConsole, CustomizedConDevicePath)) {
      //
      // In the first check, the default console variable will be null,
      // just append current customized device path
      //
      VarConsole = AppendDevicePathInstance (VarConsole, CustomizedConDevicePath);

      //
      // Update the variable of the default console
      //
      gRT->SetVariable (
            ConVarName,
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
            GetDevicePathSize (VarConsole),
            VarConsole
            );
    }
  }

  return EFI_SUCCESS;

}

EFI_STATUS
BdsLibConnectConsoleVariable (
  IN  CHAR16                 *ConVarName
  )
/*++

Routine Description:

  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path, if
  anyone of the instances is connected success, then this function
  will return success.

Arguments:

  ConVarName   - Console related variable name, ConIn, ConOut, ErrOut.

Returns:

  EFI_NOT_FOUND           - There is not any console devices connected success
  
  EFI_SUCCESS             - Success connect any one instance of the console
                            device path base on the variable ConVarName.

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;
  BOOLEAN                   DeviceExist;

  Status      = EFI_SUCCESS;
  DeviceExist = FALSE;

  //
  // Check if the console variable exist
  //
  StartDevicePath = BdsLibGetVariableAndSize (
                      ConVarName,
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if (StartDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyOfDevicePath = DuplicateDevicePath (StartDevicePath);
  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&CopyOfDevicePath, &Size);
    Next      = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);

    //
    // Connect the instance device path
    //
    Status = BdsLibConnectDevicePath (Instance);
    if (EFI_ERROR (Status)) {
      //
      // Delete the instance from the console varialbe
      //
      BdsLibUpdateConsoleVariable (ConVarName, NULL, Instance);
    } else {
      DeviceExist = TRUE;
    }

  } while (CopyOfDevicePath != NULL);

  gBS->FreePool (StartDevicePath);

  if (DeviceExist == FALSE) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

VOID
BdsLibConnectAllConsoles (
  VOID
  )
/*++

Routine Description:

  This function will search every simpletxt devive in current system,
  and make every simpletxt device as pertantial console device.

Arguments:

  None

Returns:

  None

--*/
{
  EFI_STATUS                Status;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *ConDevicePath;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;

  Index         = 0;
  HandleCount   = 0;
  HandleBuffer  = NULL;
  ConDevicePath = NULL;

  //
  // Update all the console varables
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextInProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &ConDevicePath
                    );
    BdsLibUpdateConsoleVariable (L"ConIn", ConDevicePath, NULL);
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &ConDevicePath
                    );
    BdsLibUpdateConsoleVariable (L"ConOut", ConDevicePath, NULL);
    BdsLibUpdateConsoleVariable (L"ErrOut", ConDevicePath, NULL);
  }
  //
  // Connect all console variables
  //
  BdsLibConnectAllDefaultConsoles ();

}

EFI_STATUS
BdsLibConnectAllDefaultConsoles (
  VOID
  )
/*++

Routine Description:

  This function will connect console device base on the console 
  device variable ConIn, ConOut and ErrOut.

Arguments:

  None

Returns:

  EFI_SUCCESS      - At least one of the ConIn and ConOut device have
                     been connected success.
                     
  EFI_STATUS       - Return the status of BdsLibConnectConsoleVariable ().

--*/
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *VarErrout;
  UINTN                     DevicePathSize;

  //
  // Connect all default console variables
  //
  Status = BdsLibConnectConsoleVariable (L"ConIn");
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BdsLibConnectConsoleVariable (L"ConOut");
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Special treat the err out device, becaues the null
  // err out var is legal.
  //
  VarErrout = BdsLibGetVariableAndSize (
                L"ErrOut",
                &gEfiGlobalVariableGuid,
                &DevicePathSize
                );
  if (VarErrout != NULL) {
    BdsLibConnectConsoleVariable (L"ErrOut");
  }

  return EFI_SUCCESS;

}
