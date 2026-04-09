/** @file
 *
  Memory Debug Log Library - Runtime

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
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
  EFI_STATUS  Status;

  //
  // Stop logging after we have switched to virtual mode
  // to avoid potential problems (such as crashes accessing
  // physical pointers).
  //
  if (EfiGoneVirtual ()) {
    return EFI_SUCCESS;
  }

  if (!mMemDebugLogBufAddrInit) {
    //
    // Obtain the Memory Debug Log buffer addr from HOB
    //
    Status = MemDebugLogAddrFromHOB (&mMemDebugLogBufAddr);
    if (EFI_ERROR (Status)) {
      mMemDebugLogBufAddr = 0;
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
