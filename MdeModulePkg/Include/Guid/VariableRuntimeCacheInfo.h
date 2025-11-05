/** @file
  This Variable Runtime Cache Info HOB is used to store the address
  and the size of the buffer that will be used for variable runtime
  service when the PcdEnableVariableRuntimeCache is TRUE.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VARIABLE_RUNTIME_CACHE_INFO_H_
#define VARIABLE_RUNTIME_CACHE_INFO_H_

#define VARIABLE_RUNTIME_CACHE_INFO_HOB_REVISION  1

#define VARIABLE_RUNTIME_CACHE_INFO_GUID \
  { \
    0x0f472f7d, 0x6713, 0x4915, {0x96, 0x14, 0x5d, 0xda, 0x28, 0x40, 0x10, 0x56}  \
  }

typedef struct {
  ///
  /// TRUE indicates GetVariable () or GetNextVariable () is being called.
  /// When the value is FALSE, the given update (and any other pending updates)
  /// can be flushed to the runtime cache.
  ///
  BOOLEAN    ReadLock;
  ///
  /// TRUE indicates there is pending update for the given variable store needed
  /// to be flushed to the runtime cache.
  ///
  BOOLEAN    PendingUpdate;
  ///
  /// TRUE indicates all HOB variables have been flushed in flash.
  ///
  BOOLEAN    HobFlushComplete;
} CACHE_INFO_FLAG;

typedef struct {
  EFI_PHYSICAL_ADDRESS    CacheInfoFlagBuffer;
  ///
  /// Base address of the runtime Hob variable cache Buffer.
  ///
  EFI_PHYSICAL_ADDRESS    RuntimeHobCacheBuffer;
  UINT64                  RuntimeHobCachePages;
  ///
  /// Base address of the non-volatile variable runtime cache Buffer.
  ///
  EFI_PHYSICAL_ADDRESS    RuntimeNvCacheBuffer;
  UINT64                  RuntimeNvCachePages;
  ///
  /// Base address of the volatile variable runtime cache Buffer.
  ///
  EFI_PHYSICAL_ADDRESS    RuntimeVolatileCacheBuffer;
  UINT64                  RuntimeVolatileCachePages;
} VARIABLE_RUNTIME_CACHE_INFO;

extern EFI_GUID  gEdkiiVariableRuntimeCacheInfoHobGuid;

#endif
