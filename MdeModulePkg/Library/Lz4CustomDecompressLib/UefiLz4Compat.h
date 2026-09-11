/**
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-3-Clause-Clear
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
#define LZ4_STATIC_LINKING_ONLY_DISABLE_MEMORY_ALLOCATION
#endif

/* Enable freestanding mode - no standard C library */
#ifndef LZ4_FREESTANDING
#define LZ4_FREESTANDING 1
#endif

/* Map LZ4 memory functions to UEFI BaseMemoryLib */
#ifndef LZ4_memcpy
#define LZ4_memcpy(dst, src, size)   CopyMem((dst), (src), (size))
#endif

#ifndef LZ4_memmove
#define LZ4_memmove(dst, src, size)  CopyMem((dst), (src), (size))
#endif

#ifndef LZ4_memset
#define LZ4_memset(ptr, val, size)   SetMem((ptr), (size), (val))
#endif

#endif /* __UEFI_LZ4_COMPAT_H__ */
