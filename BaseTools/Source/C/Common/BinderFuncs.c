/** @file
Binder function implementations for ANSI C libraries.

Copyright (c) 1999 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BinderFuncs.h"
#include "CommonLib.h"
#include <stdlib.h>
#include <string.h>

//
// Binder Function Implementations
//

VOID *
CommonLibBinderAllocate (
  IN UINTN Size
  )
{
  return (VOID *) malloc (Size);
}

VOID
CommonLibBinderFree (
  IN VOID *Pointer
  )
{
  free (Pointer);
}

VOID
CommonLibBinderCopyMem (
  IN VOID *Destination,
  IN VOID *Source,
  IN UINTN Length
  )
{
  memmove (Destination, Source, Length);
}

VOID
CommonLibBinderSetMem (
  IN VOID *Destination,
  IN UINTN Length,
  IN UINT8 Value
  )
{
  memset (Destination, Value, Length);
}

INTN
CommonLibBinderCompareMem (
  IN VOID *MemOne,
  IN VOID *MemTwo,
  IN UINTN Length
  )
{
  return memcmp (MemOne, MemTwo, Length);
}

BOOLEAN
CommonLibBinderCompareGuid (
  IN EFI_GUID *Guid1,
  IN EFI_GUID *Guid2
  )
{
  return CompareGuid (Guid1, Guid2) ? FALSE : TRUE;
}



