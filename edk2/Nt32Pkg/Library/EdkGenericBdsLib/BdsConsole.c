/*++

Copyright (c) 2006 - 2007, Intel Corporation
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

#include <EdkGenericBdsLibInternal.h>

BOOLEAN
IsNvNeed (
  IN CHAR16 *ConVarName
  )
{
  CHAR16 *Ptr;

  Ptr = ConVarName;

  //
  // If the variable includes "Dev" at last, we consider
  // it does not support NV attribute.
  //
  while (*Ptr) {
    Ptr++;
  }

  if ((*(Ptr - 3) == 'D') && (*(Ptr - 2) == 'e') && (*(Ptr - 1) == 'v')) {
    return FALSE;
  } else {
    return TRUE;
  }
}

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
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;
  UINT32                    Attributes;

  VarConsole      = NULL;
  DevicePathSize  = 0;
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

  //
  // Initialize NewDevicePath
  //
  NewDevicePath  = VarConsole;

  //
  // If ExclusiveDevicePath is even the part of the instance in VarConsole, delete it.
  // In the end, NewDevicePath is the final device path.
  //
  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
      NewDevicePath = BdsLibDelPartMatchInstance (VarConsole, ExclusiveDevicePath);
  }
  //
  // Try to append customized device path to NewDevicePath.
  //
  if (CustomizedConDevicePath != NULL) {
    if (!BdsLibMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      //
      // Check if there is part of CustomizedConDevicePath in NewDevicePath, delete it.
      //
      NewDevicePath = BdsLibDelPartMatchInstance (NewDevicePath, CustomizedConDevicePath);
      //
      // In the first check, the default console variable will be null,
      // just append current customized device path
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      BdsLibSafeFreePool(TempNewDevicePath);
    }
  }

  //
  // The attribute for ConInDev, ConOutDev and ErrOutDev does not include NV.
  //
  if (IsNvNeed(ConVarName)) {
    //
    // ConVarName has NV attribute.
    //
    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
  } else {
    //
    // ConVarName does not have NV attribute.
    //
    Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  //
  // Finally, Update the variable of the default console by NewDevicePath
  //
  gRT->SetVariable (
        ConVarName,
        &gEfiGlobalVariableGuid,
        Attributes,
        GetDevicePathSize (NewDevicePath),
        NewDevicePath
        );

  if (VarConsole == NewDevicePath) {
    BdsLibSafeFreePool(VarConsole);
  } else {
    BdsLibSafeFreePool(VarConsole);
    BdsLibSafeFreePool(NewDevicePath);
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

  CopyOfDevicePath = StartDevicePath;
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
    BdsLibSafeFreePool(Instance);
  } while (CopyOfDevicePath != NULL);

  FreePool (StartDevicePath);

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

  BdsLibSafeFreePool(HandleBuffer);

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

  BdsLibSafeFreePool(HandleBuffer);

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

  //
  // Connect all default console variables
  //

  //
  // Because possibly the platform is legacy free, in such case,
  // ConIn devices (Serial Port and PS2 Keyboard ) does not exist,
  // so we need not check the status.
  //
  BdsLibConnectConsoleVariable (L"ConIn");

  //
  // It seems impossible not to have any ConOut device on platform,
  // so we check the status here.
  //
  Status = BdsLibConnectConsoleVariable (L"ConOut");
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Special treat the err out device, becaues the null
  // err out var is legal.
  //
  BdsLibConnectConsoleVariable (L"ErrOut");

  return EFI_SUCCESS;

}
