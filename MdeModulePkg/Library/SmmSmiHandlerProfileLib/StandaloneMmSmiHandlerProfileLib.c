/** @file
  Standalone MM driver instance of SmiHandlerProfile Library.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include "MmSmiHandlerProfileLib.h"

/**
  The constructor function for standalone MM SMI handler profile.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
StandaloneMmSmiHandlerProfileLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return MmSmiHandlerProfileLibInitialization ();
}
