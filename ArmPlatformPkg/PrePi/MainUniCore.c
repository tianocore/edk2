/** @file

  Copyright (c) 2011, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrePi.h"

VOID
PrimaryMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StacksBase,
  IN  UINT64  StartTimeStamp
  )
{
  PrePiMain (UefiMemoryBase, StacksBase, StartTimeStamp);

  // We must never return
  ASSERT (FALSE);
}
