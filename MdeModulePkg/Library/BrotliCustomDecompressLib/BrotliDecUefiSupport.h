/** @file
  BROTLI UEFI header file for definitions

  Allows BROTLI code to build under UEFI (edk2) build environment

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BROTLI_DECOMPRESS_UEFI_SUP_H__
#define __BROTLI_DECOMPRESS_UEFI_SUP_H__

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#define memcpy                      CopyMem
#define memmove                     CopyMem
#define memset(dest,ch,count)       SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define malloc                      BrDummyMalloc
#define free                        BrDummyFree

typedef INT8     int8_t;
typedef INT16    int16_t;
typedef INT32    int32_t;
typedef INT64    int64_t;
typedef UINT8    uint8_t;
typedef UINT16   uint16_t;
typedef UINT32   uint32_t;
typedef UINT64   uint64_t;
typedef UINTN    size_t;

VOID *
BrDummyMalloc (
  IN size_t   Size
  );

VOID
BrDummyFree (
  IN VOID *   Ptr
  );

#endif
