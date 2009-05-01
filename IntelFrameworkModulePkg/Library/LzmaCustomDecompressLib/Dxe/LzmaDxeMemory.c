/** @file
  LZMA Memory Allocation for DXE

  Copyright (c) 2006 - 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include "Sdk/C/Types.h"

STATIC
VOID *
SzAlloc(
  void *p,
  size_t size
  )
{
  void *np;
  p = p;
  np = AllocatePool(size);
  return np;
}

STATIC
VOID
SzFree(
  void *p,
  void *address
  )
{
  p = p;
  if (address != NULL) {
    FreePool(address);
  }
}

ISzAlloc g_Alloc = { SzAlloc, SzFree };

