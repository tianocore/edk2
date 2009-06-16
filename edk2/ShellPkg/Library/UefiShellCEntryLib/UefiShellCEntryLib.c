/** @file
  Provides application point extension for "C" style main funciton 

Copyright (c) 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellParameters.h>

#include <Library/DebugLib.h>

INT32 
EFIAPI 
main(
  UINTN Argc, 
  CHAR16 **Argv
);

EFI_STATUS
EFIAPI
ShellCEntryLib(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  ){
  INT32                         ReturnFromMain;
  EFI_SHELL_PARAMETERS_PROTOCOL *EfiShellParametersProtocol;
  EFI_SHELL_INTERFACE           *EfiShellInterface;
  EFI_STATUS                    Status;

  ReturnFromMain = -1;
  EfiShellParametersProtocol = NULL;
  EfiShellInterface = NULL;

  Status = SystemTable->BootServices->OpenProtocol(ImageHandle, 
                             &gEfiShellParametersProtocolGuid,
                             (VOID **)&EfiShellParametersProtocol,
                             ImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL
                             );
  if (!EFI_ERROR(Status)) {
    //
    // use shell 2.0 interface
    //
    ReturnFromMain = main(EfiShellInterface->Argc, EfiShellInterface->Argv);
  } else {
    //
    // try to get shell 1.0 interface instead.
    //
    Status = SystemTable->BootServices->OpenProtocol(ImageHandle, 
                               &gEfiShellInterfaceGuid,
                               (VOID **)&EfiShellInterface,
                               ImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL
                               );
    if (!EFI_ERROR(Status)) {
      //
      // use shell 1.0 interface
      // 
      ReturnFromMain = main(EfiShellParametersProtocol->Argc, EfiShellParametersProtocol->Argv);
    } else {
      ASSERT(FALSE);
    }
  }
  if (ReturnFromMain == 0) {
    return (EFI_SUCCESS);
  } else {
    return (EFI_UNSUPPORTED);
  }
}
