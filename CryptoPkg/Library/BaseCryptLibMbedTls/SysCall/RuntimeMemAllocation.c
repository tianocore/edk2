/** @file
  Light-weight Memory Management Routines for MbedTLS-based Crypto
  Library at Runtime Phase.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CrtLibSupport.h>
#include <Library/RuntimeMemoryAllocationLib.h>

//
// -- Memory-Allocation Routines Wrapper for UEFI-MbedTLS Library --
//

/** Allocates memory blocks. **/
VOID *
malloc (
  size_t  size
  )
{
  return RuntimeAllocateMem ((UINTN)size);
}

/** Reallocate memory blocks. **/
VOID *
realloc (
  VOID    *ptr,
  size_t  size
  )
{
  return RuntimeReallocateMem (ptr, (UINTN)size);
}

/** Deallocates or frees a memory block. **/
VOID
free (
  VOID  *ptr
  )
{
  //
  // In Standard C, free() handles a null pointer argument transparently. This
  // is not true of RuntimeFreeMem() below, so protect it.
  //
  if (ptr != NULL) {
    RuntimeFreeMem (ptr);
  }
}
