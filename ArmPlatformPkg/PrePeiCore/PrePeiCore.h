/** @file
  Main file supporting the transition to PEI Core in Normal World for Versatile Express

  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PREPEICORE_H_
#define __PREPEICORE_H_

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <PiPei.h>
#include <Ppi/TemporaryRamSupport.h>

VOID
CreatePpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  );

EFI_STATUS
EFIAPI
PrePeiCoreTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  );

VOID
SecSwitchStack (
  INTN  StackDelta
  );

// Vector Table for Pei Phase
VOID
PeiVectorTable (
  VOID
  );

VOID
EFIAPI
PrimaryMain (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  );

VOID
PeiCommonExceptionEntry (
  IN UINT32  Entry,
  IN UINTN   LR
  );

#endif
