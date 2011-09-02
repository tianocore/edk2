/** @file
  TCG MOR (Memory Overwrite Request) Control Driver.

  This driver initilize MemoryOverwriteRequestControl variable. It 
  will clear MOR_CLEAR_MEMORY_BIT bit if it is set.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TcgMor.h"

/**
  Entry Point for TCG MOR Control driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS     
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
MorDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT8       MorControl;
  UINTN       DataSize;

  ///
  /// The firmware is required to create the MemoryOverwriteRequestControl UEFI variable.
  ///

  DataSize = sizeof (MorControl);
  Status = gRT->GetVariable (
                  MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME, 
                  &gEfiMemoryOverwriteControlDataGuid, 
                  NULL, 
                  &DataSize, 
                  &MorControl
                  );
  if (EFI_ERROR (Status)) {
    //
    // Set default value to 0
    //
    MorControl = 0;
  } else {
    if (MOR_CLEAR_MEMORY_VALUE (MorControl) == 0x0) {
      //
      // MorControl is expected, directly return to avoid unnecessary variable operation
      //
      return EFI_SUCCESS;
    }
    //
    // Clear MOR_CLEAR_MEMORY_BIT
    //
    DEBUG ((EFI_D_INFO, "TcgMor: Clear MorClearMemory bit\n"));
    MorControl &= 0xFE;
  }
  
  Status = gRT->SetVariable (
                  MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME, 
                  &gEfiMemoryOverwriteControlDataGuid, 
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize, 
                  &MorControl
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}


