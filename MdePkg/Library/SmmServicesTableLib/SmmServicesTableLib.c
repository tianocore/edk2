/** @file
  SMM Services Table Library.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>
#include <Protocol/SmmBase2.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_SMM_SYSTEM_TABLE2   *gSmst             = NULL;
EFI_SMM_BASE2_PROTOCOL  *mInternalSmmBase2 = NULL;

/**
  The constructor function caches the pointer of SMM Services Table.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Retrieve SMM Base2 Protocol,  Do not use gBS from UefiBootServicesTableLib on purpose
  // to prevent inclusion of gBS, gST, and gImageHandle from SMM Drivers unless the 
  // SMM driver explicity declares that dependency. 
  //
  Status = SystemTable->BootServices->LocateProtocol (
                                        &gEfiSmmBase2ProtocolGuid,
                                        NULL,
                                        (VOID **)&mInternalSmmBase2
                                        );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mInternalSmmBase2 != NULL);

  //
  // Check to see if we are already in SMM
  //
  if (!InSmm ()) {
    //
    // We are not in SMM, so SMST is not needed
    //
    return EFI_SUCCESS;
  }

  //
  // We are in SMM, retrieve the pointer to SMM System Table
  //
  mInternalSmmBase2->GetSmstLocation (mInternalSmmBase2, &gSmst);
  ASSERT (gSmst != NULL);

  return EFI_SUCCESS;
}

/**
  This function allows the caller to determine if the driver is executing in 
  System Management Mode(SMM).

  This function returns TRUE if the driver is executing in SMM and FALSE if the 
  driver is not executing in SMM.

  @retval  TRUE  The driver is executing in System Management Mode (SMM).
  @retval  FALSE The driver is not executing in System Management Mode (SMM). 

**/
BOOLEAN
EFIAPI
InSmm (
  VOID
  )
{
  BOOLEAN  InSmm;

  //
  // Check to see if we are already in SMM
  //
  mInternalSmmBase2->InSmm (mInternalSmmBase2, &InSmm);
  return InSmm;
}
