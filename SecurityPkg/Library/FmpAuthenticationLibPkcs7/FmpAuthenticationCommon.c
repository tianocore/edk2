/** @file
  Common utility functions for FMP Authentication PKCS7 library.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RuntimeMemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "FmpAuthenticationCommon.h"

BOOLEAN  mFmpAuthenticationExitBootServiceSignalled = FALSE;

/**
  Allocates a temp buffer for FMP authentication

  @param[in]  AllocationSize    Bytes to be allocated.

  @return  A pointer to the allocated buffer or NULL if allocation fails.

 **/
VOID *
EFIAPI
AllocateTempBuffer (
  IN  UINTN  AllocationSize
  )
{
  if (!mFmpAuthenticationExitBootServiceSignalled) {
    return AllocateZeroPool (AllocationSize);
  }

  return RuntimeAllocateMem (AllocationSize);
}

/**
  Frees a temp buffer allocated with AllocateTempBuffer().

  @param[in]  Buffer  Pointer to the buffer to free.

 **/
VOID
EFIAPI
FreeTempBuffer (
  IN  VOID  *Buffer
  )
{
  if (Buffer == NULL) {
    return;
  }

  if (!mFmpAuthenticationExitBootServiceSignalled) {
    FreePool (Buffer);
  } else {
    RuntimeFreeMem (Buffer);
  }
}
