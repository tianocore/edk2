/** @file
 *
  Memory Debug Log Library - Runtime

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/HobLib.h>
#include <Library/MemDebugLogLib.h>
#include <Library/UefiRuntimeLib.h>

EFI_PHYSICAL_ADDRESS  mMemDebugLogBufAddr;
BOOLEAN               mMemDebugLogBufAddrInit;

EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN  Length
  )
{
  EFI_HOB_GUID_TYPE       *GuidHob;
  MEM_DEBUG_LOG_HOB_DATA  *HobData;
  EFI_STATUS              Status;

  //
  // Stop logging after we have switched to virtual mode
  // to avoid potential problems (such as crashes accessing
  // physical pointers).
  //
  if (EfiGoneVirtual ()) {
    return EFI_SUCCESS;
  }

  //
  // Init debug log buffer addr on first write
  //
  if (!mMemDebugLogBufAddrInit) {
    //
    // Obtain the Memory Debug Log buffer addr from PEI HOB
    //
    GuidHob = GetFirstGuidHob (&gMemDebugLogHobGuid);
    if (GuidHob == NULL) {
      mMemDebugLogBufAddr = 0;
    } else {
      //
      // Populate the Mem Debug Log Buffer from the HOB
      //
      HobData             = (MEM_DEBUG_LOG_HOB_DATA *)GET_GUID_HOB_DATA (GuidHob);
      mMemDebugLogBufAddr = HobData->MemDebugLogBufAddr;
    }

    mMemDebugLogBufAddrInit = TRUE;
  }

  if (mMemDebugLogBufAddr) {
    Status = MemDebugLogWriteBuffer (mMemDebugLogBufAddr, Buffer, Length);
  } else {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}
