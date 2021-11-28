/** @file
  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/TdxStartupLib.h>

VOID
EFIAPI
TdxStartup (
  IN VOID  *Context
  )
{
  ASSERT (FALSE);
  CpuDeadLoop ();
}
