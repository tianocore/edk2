/** @file
  LZMA UEFI header file

  Allows LZMA code to build under UEFI (edk2) build environment

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFILZMA_H__
#define __UEFILZMA_H__

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>

#ifdef _WIN32
#undef _WIN32
#endif

#ifndef _SIZE_T_DEFINED
#if !defined(_WIN64) || defined(__GNUC__)
typedef unsigned int size_t;
#endif
#endif

#ifdef _WIN64
#undef _WIN64
#endif

#ifndef _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#endif

#define memcpy CopyMem
#define memmove CopyMem

#define _LZMA_SIZE_OPT

#endif // __UEFILZMA_H__

