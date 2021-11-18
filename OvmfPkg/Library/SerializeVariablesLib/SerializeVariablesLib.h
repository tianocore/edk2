/** @file
  Serialize Variables Library implementation

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SERIALIZE_VARIABLES_LIB_INSTANCE__
#define __SERIALIZE_VARIABLES_LIB_INSTANCE__

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SerializeVariablesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define SV_FROM_HANDLE(a)  CR (a, SV_INSTANCE, Signature, SV_SIGNATURE)
#define SV_SIGNATURE  SIGNATURE_32 ('S', 'V', 'A', 'R')

typedef struct {
  UINT32    Signature;
  VOID      *BufferPtr;
  UINTN     BufferSize;
  UINTN     DataSize;
} SV_INSTANCE;

#endif
