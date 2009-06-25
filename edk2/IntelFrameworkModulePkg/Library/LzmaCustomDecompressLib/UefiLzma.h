/** @file
  LZMA UEFI header file

  Allows LZMA code to build under UEFI (edk2) build environment

  Copyright (c) 2009, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFILZMA_H__
#define __UEFILZMA_H__

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>

#ifdef _WIN32
#undef _WIN32
#endif

#if !defined(_WIN64) || defined(__GNUC__)
typedef unsigned int size_t;
#endif

#ifdef _WIN64
#undef _WIN64
#endif

typedef int ptrdiff_t;

#define memcpy CopyMem
#define memmove CopyMem

#define _LZMA_SIZE_OPT

#endif // __UEFILZMA_H__

