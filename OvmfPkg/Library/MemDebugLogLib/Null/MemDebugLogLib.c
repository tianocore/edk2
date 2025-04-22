/** @file
 *
  Memory Debug Log Library.

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MemDebugLogLib.h>

EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN   Length
  )
{
  // Null Instance - NOP
  return EFI_SUCCESS;
}

