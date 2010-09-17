/** @file
  Driver entry for KbcReset driver.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/ResetSystemLib.h>

#include <Protocol/Reset.h>

//
// The handle onto which the Reset Architectural Protocol is installed
//
EFI_HANDLE  mResetHandle = NULL;

/**
  Reset the system.

  @param ResetType       warm or cold
  @param ResetStatus     possible cause of reset
  @param DataSize        Size of ResetData in bytes
  @param ResetData       Optional Unicode string

**/
VOID
EFIAPI
KbcResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
{
  switch (ResetType) {
  case EfiResetWarm:
    ResetWarm ();
    break;
  case EfiResetCold:
    ResetCold ();
    break;
  case EfiResetShutdown:
    ResetShutdown ();
    break;
  default:
    return;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
}

/**
  Initialize the state information for the Reset Architectural Protocol.

  @param ImageHandle     Handle of the loaded driver 
  @param SystemTable     Pointer to the System Table

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Cannot create the timer service

**/
EFI_STATUS
EFIAPI
InitializeReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Make sure the Reset Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiResetArchProtocolGuid);

  //
  // Hook the runtime service table
  //
  SystemTable->RuntimeServices->ResetSystem = KbcResetSystem;

  //
  // Now install the Reset RT AP on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mResetHandle,
                  &gEfiResetArchProtocolGuid, NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
