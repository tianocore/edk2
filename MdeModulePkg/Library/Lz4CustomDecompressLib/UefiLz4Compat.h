/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent
  @file
  UEFI compatibility definitions for upstream LZ4.
  This header must be included before lz4.h and is intentionally limited to
  freestanding and memory-operation compatibility.
**/

#ifndef __UEFI_LZ4_COMPAT_H__
#define __UEFI_LZ4_COMPAT_H__

#include <Base.h>
#include <Library/BaseMemoryLib.h>

/**
  LZ4 requires these macros when LZ4_FREESTANDING=1 is defined.
  They are mapped to UEFI BaseMemoryLib functions.
**/

/* Disable memory allocation - LZ4 decompression doesn't need malloc/free */
#ifndef LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION
#define LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION  1
#endif

/* Enable freestanding mode - no standard C library */
#ifndef LZ4_FREESTANDING
#define LZ4_FREESTANDING  1
#endif

//
// C types required by the freestanding LZ4 sources.
//
typedef UINT8   uint8_t;
typedef INT8    int8_t;
typedef UINT16  uint16_t;
typedef INT16   int16_t;
typedef UINT32  uint32_t;
typedef INT32   int32_t;
typedef UINT64  uint64_t;
typedef INT64   int64_t;
typedef UINTN   uintptr_t;
typedef UINTN   size_t;
typedef INTN    ptrdiff_t;

//
// LZ4 decompression must not allocate memory.  Provide the allocator symbols
// required by compiler headers as fail-safe stubs
//
#define malloc  Lz4DummyMalloc
#define calloc  Lz4DummyCalloc
#define free    Lz4DummyFree

VOID *
Lz4DummyMalloc (
  IN size_t  Size
  );

VOID *
Lz4DummyCalloc (
  IN size_t  Number,
  IN size_t  Size
  );

VOID
Lz4DummyFree (
  IN VOID  *Ptr
  );

#ifndef UINT_MAX
#define UINT_MAX  0xFFFFFFFFU
#endif

/* Map LZ4 memory functions to UEFI BaseMemoryLib */
#ifndef LZ4_memcpy
#define LZ4_memcpy(dst, src, size)  CopyMem((dst), (src), (size))
#endif

#ifndef LZ4_memmove
#define LZ4_memmove(dst, src, size)  CopyMem((dst), (src), (size))
#endif

#ifndef LZ4_memset
#define LZ4_memset(ptr, val, size)  SetMem((ptr), (size), (val))
#endif

#endif /* __UEFI_LZ4_COMPAT_H__ */
