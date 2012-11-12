/** @file
  Boot Script Helper SMM driver.

  This driver is responsible to store BootScriptThunk from ReservedMemory to SMRAM for security consideration.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/LockBoxLib.h>

#include <Guid/BootScriptThunkData.h>

EFI_GUID mBootScriptThunkGuid = {
  0xa053f561, 0xf56b, 0x4140, {0x89, 0x1, 0xb4, 0xcb, 0x5d, 0x70, 0x92, 0x9e}
};

/**
  Entry point function of the Boot Script Thunk Helper SMM driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
BootScriptThunkHelperMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  BOOT_SCRIPT_THUNK_DATA        *BootScriptThunkData;
  EFI_STATUS                    Status;

  //
  // Get BootScriptThunk variable
  //
  BootScriptThunkData = (BOOT_SCRIPT_THUNK_DATA *)(UINTN)PcdGet64(BootScriptThunkDataPtr);
  ASSERT (BootScriptThunkData != NULL);
  if (BootScriptThunkData == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Save BootScriptThunk image
  //
  Status = SaveLockBox (
             &mBootScriptThunkGuid,
             (VOID *)(UINTN)BootScriptThunkData->BootScriptThunkBase,
             (UINTN)BootScriptThunkData->BootScriptThunkLength
             );
  ASSERT_EFI_ERROR (Status);

  Status = SetLockBoxAttributes (&mBootScriptThunkGuid, LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
