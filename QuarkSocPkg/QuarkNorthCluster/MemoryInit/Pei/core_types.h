/** @file
Core types used in Mrc.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __MRC_CORE_TYPES_H
#define __MRC_CORE_TYPES_H

typedef char char_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned char bool;
typedef unsigned int size_t;

#ifdef ASM_INC
// Unfortunately h2inc has issue with long long
typedef struct uint64_s
{
  uint32_t lo;
  uint32_t hi;
}uint64_t;
#else
typedef unsigned long long uint64_t;
#endif

#ifdef SIM
// Native word length is 64bit in simulation environment
typedef uint64_t uintn_t;
#else
// Quark is 32bit
typedef uint32_t uintn_t;
#endif

#define PTR32(a)  ((volatile uint32_t*)(uintn_t)(a))

#endif

