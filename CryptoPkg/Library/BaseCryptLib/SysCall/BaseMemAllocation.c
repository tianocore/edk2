/** @file
  Base Memory Allocation Routines Wrapper for Crypto library over OpenSSL
  during PEI & DXE phases.

Copyright (c) 2009 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CrtLibSupport.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>

//
// Extra header to record the memory buffer size from malloc routine.
//
#define CRYPTMEM_HEAD_SIGNATURE  SIGNATURE_32('c','m','h','d')
typedef struct {
  UINT32    Signature;
  UINT32    Reserved;
  UINTN     Size;
} CRYPTMEM_HEAD;

#define CRYPTMEM_OVERHEAD  sizeof(CRYPTMEM_HEAD)

//
// -- Memory-Allocation Routines --
//

/* Allocates memory blocks */
void *
malloc (
  size_t  size
  )
{
  CRYPTMEM_HEAD  *PoolHdr;
  UINTN          NewSize;
  VOID           *Data;

  //
  // Adjust the size by the buffer header overhead
  //
  NewSize = (UINTN)(size) + CRYPTMEM_OVERHEAD;

  Data = AllocatePages (EFI_SIZE_TO_PAGES (NewSize));
  if (Data != NULL) {
    PoolHdr = (CRYPTMEM_HEAD *)Data;
    //
    // Record the memory brief information
    //
    PoolHdr->Signature = CRYPTMEM_HEAD_SIGNATURE;
    PoolHdr->Size      = size;

    return (VOID *)(PoolHdr + 1);
  } else {
    //
    // The buffer allocation failed.
    //
    return NULL;
  }
}

/* Reallocate memory blocks */
void *
realloc (
  void    *ptr,
  size_t  size
  )
{
  CRYPTMEM_HEAD  *OldPoolHdr;
  CRYPTMEM_HEAD  *NewPoolHdr;
  UINTN          OldSize;
  UINTN          NewSize;
  VOID           *Data;

  NewSize = (UINTN)size + CRYPTMEM_OVERHEAD;
  Data    = AllocatePages (EFI_SIZE_TO_PAGES (NewSize));
  if (Data != NULL) {
    NewPoolHdr            = (CRYPTMEM_HEAD *)Data;
    NewPoolHdr->Signature = CRYPTMEM_HEAD_SIGNATURE;
    NewPoolHdr->Size      = size;
    if (ptr != NULL) {
      //
      // Retrieve the original size from the buffer header.
      //
      OldPoolHdr = (CRYPTMEM_HEAD *)ptr - 1;
      ASSERT (OldPoolHdr->Signature == CRYPTMEM_HEAD_SIGNATURE);
      OldSize = OldPoolHdr->Size;

      //
      // Duplicate the buffer content.
      //
      CopyMem ((VOID *)(NewPoolHdr + 1), ptr, MIN (OldSize, size));
      FreePages (((VOID *)OldPoolHdr), EFI_SIZE_TO_PAGES (OldSize));
    }

    return (VOID *)(NewPoolHdr + 1);
  } else {
    //
    // The buffer allocation failed.
    //
    return NULL;
  }
}

/* De-allocates or frees a memory block */
void
free (
  void  *ptr
  )
{
  CRYPTMEM_HEAD  *PoolHdr;

  //
  // In Standard C, free() handles a null pointer argument transparently. This
  // is not true of FreePool() below, so protect it.
  //
  if (ptr != NULL) {
    PoolHdr = (CRYPTMEM_HEAD *)ptr - 1;
    ASSERT (PoolHdr->Signature == CRYPTMEM_HEAD_SIGNATURE);
    FreePages (((VOID *)PoolHdr), EFI_SIZE_TO_PAGES (PoolHdr->Size));
  }
}
