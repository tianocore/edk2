/** @file
  Reset Architectural Protocol implementation

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ResetSystem.h"

//
// The handle onto which the Reset Architectural Protocol is installed
//
EFI_HANDLE  mResetHandle = NULL;

/**
  The driver's entry point.

  It initializes the Reset Architectural Protocol.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Cannot install ResetArch protocol.

**/
EFI_STATUS
EFIAPI
InitializeResetSystem (
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
  gRT->ResetSystem = ResetSystem;

  //
  // Now install the Reset RT AP on a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mResetHandle,
                  &gEfiResetArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Reset system for capsule update.

  @param[in] CapsuleDataPtr  Pointer to the capsule block descriptors.
                            
**/
VOID
CapsuleReset (
  IN UINTN   CapsuleDataPtr
  )
{
  //
  // This implementation assumes that we're using a variable
  // to indicate capsule updates.
  //
  gRT->SetVariable (
         EFI_CAPSULE_VARIABLE_NAME,
         &gEfiCapsuleVendorGuid,
         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
         sizeof (UINTN),
         (VOID *) &CapsuleDataPtr
         );

  EnterS3WithImmediateWake ();

  //
  // Should not return
  //
  CpuDeadLoop ();
}

/**
  Put the system into S3 power state.                            
**/
VOID
DoS3 (
  VOID
  )
{
  EnterS3WithImmediateWake ();

  //
  // Should not return
  //
  CpuDeadLoop ();
}

/**
  Resets the entire platform.

  @param[in] ResetType          The type of reset to perform.
  @param[in] ResetStatus        The status code for the reset.
  @param[in] DataSize           The size, in bytes, of WatchdogData.
  @param[in] ResetData          For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.

**/
VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN VOID             *ResetData OPTIONAL
  )
{
  EFI_STATUS    Status;
  UINTN         Size;
  UINTN         CapsuleDataPtr;
  
  //
  // Indicate reset system runtime service is called.
  //
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_SOFTWARE_EFI_RUNTIME_SERVICE | EFI_SW_RS_PC_RESET_SYSTEM));

  switch (ResetType) {
  case EfiResetWarm:

    //
    //Check if there are pending capsules to process
    //
    Size = sizeof (CapsuleDataPtr);
    Status =  EfiGetVariable (
                 EFI_CAPSULE_VARIABLE_NAME,
                 &gEfiCapsuleVendorGuid,
                 NULL,
                 &Size,
                 (VOID *) &CapsuleDataPtr
                 );

    if (Status == EFI_SUCCESS) {
      //
      //Process capsules across a system reset.
      //
      DoS3();
    }

    ResetWarm ();

    break;

 case EfiResetCold:
    ResetCold ();
    break;

  case EfiResetShutdown:
    ResetShutdown ();
    return ;

  default:
    return ;
  }

  //
  // Given we should have reset getting here would be bad
  //
  ASSERT (FALSE);
}
