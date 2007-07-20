/** @file
  UEFI Runtime Services Table Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiDxe.h>


#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>

#include "UefiRuntimeServicesTableLibInternal.h"

EFI_RUNTIME_SERVICES  *gRT = NULL;

/**
  The constructor function caches the pointer of Runtime Services Table.

  The constructor function caches the pointer of Runtime Services Table.
  It will ASSERT() if the pointer of Runtime Services Table is NULL.
  It will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UefiRuntimeServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Cache pointer to the EFI Runtime Services Table
  //
  gRT = SystemTable->RuntimeServices;
  ASSERT (gRT != NULL);
  return EFI_SUCCESS;
}
