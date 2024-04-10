/** @file
  This file defines a VARIABLE_RUNTIME_CACHE_CONTEXT_HOB.
  It provides the variable runtime cache context.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VARIABLE_RUNTIME_CACHE_CONTEXT_H_
#define VARIABLE_RUNTIME_CACHE_CONTEXT_H_

#include <PiPei.h>
#include <Guid\SmmVariableCommon.h>

#define VARIABLE_RUNTIME_CACHE_CONTEXT_HOB_REVISION  1

#define VARIABLE_RUNTIME_CACHE_CONTEXT_GUID \
  { \
    0x0f472f7d, 0x6713, 0x4915, {0x96, 0x14, 0x5d, 0xda, 0x28, 0x40, 0x10, 0x56}  \
  }

typedef struct {
  UINT64    ReadLockPtr;
  UINT64    PendingUpdatePtr;
  UINT64    HobFlushCompletePtr;
  UINT64    RuntimeHobCacheBuffer;
  UINT64    MaxRuntimeHobCacheSize;
  UINT64    RuntimeNvCacheBuffer;
  UINT64    MaxRuntimeNvCacheSize;
  UINT64    RuntimeVolatileCacheBuffer;
  UINT64    MaxRuntimeVolatileCacheSize;
} VARIABLE_RUNTIME_CACHE_CONTEXT_HOB;

extern EFI_GUID  gVariableRuntimeCacheContextHobGuid;

#endif
