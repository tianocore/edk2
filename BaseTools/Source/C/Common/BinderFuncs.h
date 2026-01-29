/** @file
Prototypes for binder functions that allow common code to be written which then
links to implementation of these functions which is appropriate for the specific
environment that they are running under.

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BinderFuncs_h_INCLUDED
#define BinderFuncs_h_INCLUDED

#include "Common/UefiBaseTypes.h"

//
// Binder Function Prototypes
//
// These binding functions must be implemented externally as appropriate for
// the environment that the code will be running under.
//

VOID *
CommonLibBinderAllocate (
  IN UINTN Size
  );

VOID
CommonLibBinderFree (
  IN VOID *Pointer
  );

VOID
CommonLibBinderCopyMem (
  IN VOID *Destination,
  IN VOID *Source,
  IN UINTN Length
  );

VOID
CommonLibBinderSetMem (
  IN VOID *Destination,
  IN UINTN Length,
  IN UINT8 Value
  );

INTN
CommonLibBinderCompareMem (
  IN VOID *MemOne,
  IN VOID *MemTwo,
  IN UINTN Length
  );

BOOLEAN
CommonLibBinderCompareGuid (
  IN EFI_GUID *Guid1,
  IN EFI_GUID *Guid2
  );

#endif // #ifndef CommonLibs_h_INCLUDED

