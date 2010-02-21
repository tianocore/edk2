/*++ @file
  Entrypoint of AcpiResetDxe driver.

Copyright (c) 2010, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
--*/

#include "Reset.h"

EFI_ACPI_DESCRIPTION  mAcpiDescription;

/**
  Reset the system.

  @param[in] ResetType       Warm or cold
  @param[in] ResetStatus     Possible cause of reset
  @param[in] DataSize        Size of ResetData in bytes
  @param[in] ResetData       Optional Unicode string

**/
VOID
EFIAPI
EfiAcpiResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN CHAR16           *ResetData OPTIONAL
  )
{
  AcpiResetSystem (ResetType, ResetStatus, DataSize, ResetData, &mAcpiDescription);
}

/**
  Initialize the state information for the Reset Architectural Protocol.

  @param[in] ImageHandle  Image handle of the loaded driver
  @param[in] SystemTable  Pointer to the System Table

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_UNSUPPORTED       Cannot find the info to reset system

**/
EFI_STATUS
EFIAPI
InitializeReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            Handle;
  //
  // Initialize AcpiDescription
  //
  if (!GetAcpiDescription (&mAcpiDescription)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Make sure the Reset Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiResetArchProtocolGuid);

  //
  // Hook the runtime service table
  //
  SystemTable->RuntimeServices->ResetSystem = EfiAcpiResetSystem;

  //
  // Now install the Reset RT AP on a new handle
  //
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
