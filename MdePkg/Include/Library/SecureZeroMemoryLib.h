/** @file
  Provides a library class for securely zeroing memory.

  Copyright (c), Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Securely zero a buffer.

  This function attempts to ensure the buffer is actually cleared and that the
  compiler does not optimize away the writes.

  @param  Buffer  Pointer to the buffer to clear.
  @param  Length  Number of bytes to clear.

  @return Buffer (same pointer passed in).
**/
VOID *
EFIAPI
SecureZeroMemory (
  OUT VOID   *Buffer,
  IN  UINTN  Length
  );
