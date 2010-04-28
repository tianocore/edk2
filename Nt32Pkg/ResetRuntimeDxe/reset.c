/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 Reset.c

Abstract:

  Reset Architectural Protocol as defined in Tiano under NT Emulation

**/

#include <Uefi.h>
#include <WinNtDxe.h>
#include <Protocol/Reset.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/WinNtLib.h>
#include <Library/UefiBootServicesTableLib.h>


EFI_STATUS
EFIAPI
InitializeNtReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

VOID
EFIAPI
WinNtResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  );


EFI_STATUS
EFIAPI
InitializeNtReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:


Arguments:

  ImageHandle of the loaded driver
  Pointer to the System Table

Returns:

  Status
--*/
// TODO:    SystemTable - add argument and description to function comment
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  SystemTable->RuntimeServices->ResetSystem = WinNtResetSystem;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiResetArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

VOID
EFIAPI
WinNtResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ResetType   - TODO: add argument description
  ResetStatus - TODO: add argument description
  DataSize    - TODO: add argument description
  ResetData   - TODO: add argument description

Returns:

  EFI_SUCCESS - TODO: Add description for return value

--*/
{
  //
  // BUGBUG Need to kill all console windows later
  //
  //
  // Discard ResetType, always return 0 as exit code
  //
  gWinNt->ExitProcess (0);

  //
  // Should never go here
  //
  ASSERT (FALSE);
}
