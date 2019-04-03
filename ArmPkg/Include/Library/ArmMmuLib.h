/** @file

  Copyright (c) 2015 - 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ARM_MMU_LIB__
#define __ARM_MMU_LIB__

#include <Uefi/UefiBaseType.h>

#include <Library/ArmLib.h>

EFI_STATUS
EFIAPI
ArmConfigureMmu (
  IN  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT VOID                          **TranslationTableBase OPTIONAL,
  OUT UINTN                         *TranslationTableSize  OPTIONAL
  );

EFI_STATUS
EFIAPI
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  );

EFI_STATUS
EFIAPI
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  );

EFI_STATUS
EFIAPI
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  );

EFI_STATUS
EFIAPI
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  );

VOID
EFIAPI
ArmReplaceLiveTranslationEntry (
  IN  UINT64  *Entry,
  IN  UINT64  Value,
  IN  UINT64  RegionStart
  );

EFI_STATUS
ArmSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  );

#endif
