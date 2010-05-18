/** @file

Copyright (c) 1999 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BinderFuncs.h

Abstract:

  Prototypes for binder functions that allow common code to be
  written which then links to implementation of these functions
  which is appropriate for the specific environment that they
  are running under.

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

