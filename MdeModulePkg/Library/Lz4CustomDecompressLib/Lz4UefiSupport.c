/** @file
  LZ4 UEFI support implementations.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiLz4Compat.h"
#include <Library/DebugLib.h>

/**
  Fail-safe allocator stub for the freestanding LZ4 integration.

  @param[in] Size  The requested allocation size.

  @return NULL because allocation is disabled.
**/
VOID *
Lz4DummyMalloc (
  IN size_t  Size
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Fail-safe zeroed allocator stub for the freestanding LZ4 integration.

  @param[in] Number  The number of elements to allocate.
  @param[in] Size    The size of each element.

  @return NULL because allocation is disabled.
**/
VOID *
Lz4DummyCalloc (
  IN size_t  Number,
  IN size_t  Size
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Fail-safe free stub for the freestanding LZ4 integration.

  @param[in] Ptr  The allocation to release.

  @return None.
**/
VOID
Lz4DummyFree (
  IN VOID  *Ptr
  )
{
  ASSERT (FALSE);
}
