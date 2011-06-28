/*++ @file
  Temp RAM PPI

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include <Ppi/TemporaryRamSupport.h>

VOID
EFIAPI
SecSwitchStack (
  UINT32   TemporaryMemoryBase,
  UINT32   PermenentMemoryBase
  );


EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  //
  // Migrate the whole temporary memory to permenent memory.
  //
  CopyMem (
    (VOID*)(UINTN)PermanentMemoryBase,
    (VOID*)(UINTN)TemporaryMemoryBase,
    CopySize
    );

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediatly, also we need fixup the stack change caused by new call into
  // permenent memory.
  //
  SecSwitchStack ((UINT32) TemporaryMemoryBase, (UINT32) PermanentMemoryBase);

  //
  // We need *not* fix the return address because currently,
  // The PeiCore is excuted in flash.
  //

  //
  // Simulate to invalid temporary memory, terminate temporary memory
  //
  //ZeroMem ((VOID*)(UINTN)TemporaryMemoryBase, CopySize);

  return EFI_SUCCESS;
}
