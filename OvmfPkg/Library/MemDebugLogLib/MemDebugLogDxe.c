/** @file
 *
  Memory Debug Log Library - DXE/Smm

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/MemDebugLogLib.h>

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
