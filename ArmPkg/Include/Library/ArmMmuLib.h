/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_MMU_LIB_H_
#define ARM_MMU_LIB_H_

#include <Uefi/UefiBaseType.h>

#include <Library/ArmLib.h>

EFI_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                          **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize  OPTIONAL
  );

/**
  Convert a region of memory to read-protected, by clearing the access flag.

  @param  BaseAddress           The start of the region.
  @param  Length                The size of the region.

  @retval EFI_SUCCESS           The attributes were set successfully.
  @retval EFI_OUT_OF_RESOURCES  The operation failed due to insufficient memory.

**/
EFI_STATUS
EFIAPI
ArmSetMemoryRegionNoAccess (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Convert a region of memory to read-enabled, by setting the access flag.

  @param  BaseAddress           The start of the region.
  @param  Length                The size of the region.

  @retval EFI_SUCCESS           The attributes were set successfully.
  @retval EFI_OUT_OF_RESOURCES  The operation failed due to insufficient memory.

**/
EFI_STATUS
EFIAPI
ArmClearMemoryRegionNoAccess (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
EFIAPI
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

VOID
EFIAPI
ArmReplaceLiveTranslationEntry (
  IN  UINT64   *Entry,
  IN  UINT64   Value,
  IN  UINT64   RegionStart,
  IN  BOOLEAN  DisableMmu
  );

EFI_STATUS
ArmSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  );

#endif // ARM_MMU_LIB_H_
