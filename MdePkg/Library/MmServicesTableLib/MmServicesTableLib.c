/** @file
  MM Services Table Library.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiMm.h>
#include <Protocol/MmBase.h>
#include <Library/MmServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_MM_SYSTEM_TABLE   *gMmst             = NULL;

/**
  The constructor function caches the pointer of the MM Services Table.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MmServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  EFI_MM_BASE_PROTOCOL    *InternalMmBase;

  InternalMmBase = NULL;
  //
  // Retrieve MM Base Protocol,  Do not use gBS from UefiBootServicesTableLib on purpose
  // to prevent inclusion of gBS, gST, and gImageHandle from SMM Drivers unless the
  // MM driver explicity declares that dependency.
  //
  Status = SystemTable->BootServices->LocateProtocol (
                                        &gEfiMmBaseProtocolGuid,
                                        NULL,
                                        (VOID **)&InternalMmBase
                                        );
  ASSERT_EFI_ERROR (Status);
  ASSERT (InternalMmBase != NULL);

  //
  // We are in MM, retrieve the pointer to MM System Table
  //
  InternalMmBase->GetMmstLocation (InternalMmBase, &gMmst);
  ASSERT (gMmst != NULL);

  return EFI_SUCCESS;
}
