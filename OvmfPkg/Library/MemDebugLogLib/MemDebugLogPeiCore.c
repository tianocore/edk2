/** @file
 *
  Memory Debug Log Library - PEI Core

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
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
    // NOTE: This is expected to fail until the HOB is created.
    //
    Status = MemDebugLogAddrFromHOB (&mMemDebugLogBufAddr);
    if (EFI_ERROR (Status)) {
      mMemDebugLogBufAddr = 0;
    } else {
      mMemDebugLogBufAddrInit = TRUE;
    }
  }

  if (mMemDebugLogBufAddr) {
    Status = MemDebugLogWriteBuffer (mMemDebugLogBufAddr, Buffer, Length);
  } else {
    //
    // HOB has not yet been created, so
    // write to the early debug log buffer.
    //
    if (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase) != 0x0) {
      Status = MemDebugLogWriteBuffer (
                 (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase),
                 Buffer,
                 Length
                 );
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  return Status;
}
