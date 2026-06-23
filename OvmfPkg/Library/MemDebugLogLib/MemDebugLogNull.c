/** @file
 *
  Memory Debug Log Library - Null.

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
  // Null Instance - NOP
  return EFI_SUCCESS;
}

UINT32
EFIAPI
MemDebugLogPages (
  VOID
  )
{
  return 0;
}

BOOLEAN
EFIAPI
MemDebugLogEnabled (
  VOID
  )
{
  return FALSE;
}

EFI_STATUS
EFIAPI
MemDebugLogInit (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr,
  IN UINT32                MemDebugLogBufSize
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MemDebugLogCopy (
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufDestAddr,
  IN EFI_PHYSICAL_ADDRESS  MemDebugLogBufSrcAddr
  )
{
  return EFI_SUCCESS;
}
