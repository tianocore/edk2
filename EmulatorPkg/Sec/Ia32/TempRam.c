/*++ @file
  Temp RAM PPI

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include <Ppi/TemporaryRamSupport.h>

VOID
EFIAPI
SecSwitchStack (
  UINT32  TemporaryMemoryBase,
  UINT32  PermenentMemoryBase
  );

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  )
{
  //
  // Migrate the whole temporary memory to permanent memory.
  //
  CopyMem (
    (VOID *)(UINTN)PermanentMemoryBase,
    (VOID *)(UINTN)TemporaryMemoryBase,
    CopySize
    );

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediately, also we need fixup the stack change caused by new call into
  // permanent memory.
  //
  SecSwitchStack ((UINT32)TemporaryMemoryBase, (UINT32)PermanentMemoryBase);

  //
  // We need *not* fix the return address because currently,
  // The PeiCore is executed in flash.
  //

  //
  // Simulate to invalid temporary memory, terminate temporary memory
  //
  // ZeroMem ((VOID*)(UINTN)TemporaryMemoryBase, CopySize);

  return EFI_SUCCESS;
}
