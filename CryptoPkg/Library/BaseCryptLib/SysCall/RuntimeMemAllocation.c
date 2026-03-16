/** @file
  Light-weight Memory Management Routines for OpenSSL-based Crypto
  Library at Runtime Phase.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CrtLibSupport.h>
#include <Library/RuntimeMemoryAllocationLib.h>

//
// -- Memory-Allocation Routines Wrapper for UEFI-OpenSSL Library --
//

/* Allocates memory blocks */
void *
malloc (
  size_t  size
  )
{
  return RuntimeAllocateMem ((UINTN)size);
}

/* Reallocate memory blocks */
void *
realloc (
  void    *ptr,
  size_t  size
  )
{
  return RuntimeReallocateMem (ptr, (UINTN)size);
}

/* Deallocates or frees a memory block */
void
free (
  void  *ptr
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
