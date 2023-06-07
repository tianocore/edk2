/** @file
Library fills out gMmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiMm.h>

/**
  Abstraction layer for library constructor of Standalone MM and SMM instances.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MmMemoryProtectionHobLibConstructorCommon (
  VOID
  );

/**
  Library constructor of Standalone MM instance.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI MM System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
StandaloneMmMemoryProtectionHobLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  return MmMemoryProtectionHobLibConstructorCommon ();
}
