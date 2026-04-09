/** @file
 *
  Memory Debug Log Library - SEC Phase

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/MemDebugLogLib.h>

EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN  Length
  )
{
  EFI_STATUS  Status;

  if (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase) != 0x0) {
    Status = MemDebugLogWriteBuffer (
               (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase),
               Buffer,
               Length
               );
  } else {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

RETURN_STATUS
EFIAPI
MemDebugLogLibConstructor (
  VOID
  )
{
  if (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogSize) != 0) {
    MemDebugLogInit (
      (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase),
      (UINT32)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogSize)
      );
  }

  return RETURN_SUCCESS;
}
