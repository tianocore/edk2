/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CACHE_LIB_H_
#define _CACHE_LIB_H_

//
// EFI_MEMORY_CACHE_TYPE
//
typedef INT32 EFI_MEMORY_CACHE_TYPE;

#define EFI_CACHE_UNCACHEABLE                 0
#define EFI_CACHE_WRITECOMBINING              1
#define EFI_CACHE_WRITETHROUGH                4
#define EFI_CACHE_WRITEPROTECTED              5
#define EFI_CACHE_WRITEBACK                   6

/**
 Reset all the MTRRs to a known state.

  @retval  EFI_SUCCESS All MTRRs have been reset successfully.

**/
EFI_STATUS
EFIAPI
ResetCacheAttributes (
  VOID
  );

/**
  Given the memory range and cache type, programs the MTRRs.

  @param[in] MemoryAddress           Base Address of Memory to program MTRR.
  @param[in] MemoryLength            Length of Memory to program MTRR.
  @param[in] MemoryCacheType         Cache Type.

  @retval EFI_SUCCESS            Mtrr are set successfully.
  @retval EFI_LOAD_ERROR         No empty MTRRs to use.
  @retval EFI_INVALID_PARAMETER  The input parameter is not valid.
  @retval others                 An error occurs when setting MTTR.

**/
EFI_STATUS
EFIAPI
SetCacheAttributes (
  IN  EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN  UINT64                    MemoryLength,
  IN  EFI_MEMORY_CACHE_TYPE     MemoryCacheType
  );

#endif

