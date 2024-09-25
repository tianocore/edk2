/** @file
  Minimal implementation of OPENSSL_cleanse for OpensslLibSm3.inf.

  Copyright (c) 2024, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>

VOID
OPENSSL_cleanse (
  VOID   *Buffer,
  UINTN  Size
  )
{
  ZeroMem (Buffer, Size);
}
