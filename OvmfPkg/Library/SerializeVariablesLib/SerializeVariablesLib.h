/** @file
  Serialize Variables Library implementation

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

#define SV_FROM_HANDLE(a)       CR (a, SV_INSTANCE, Signature, SV_SIGNATURE)
#define SV_SIGNATURE            SIGNATURE_32 ('S', 'V', 'A', 'R')

typedef struct {
  UINT32                              Signature;
  VOID                                *BufferPtr;
  UINTN                               BufferSize;
  UINTN                               DataSize;
} SV_INSTANCE;

#endif

