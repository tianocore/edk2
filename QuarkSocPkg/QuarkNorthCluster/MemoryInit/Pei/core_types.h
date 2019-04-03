/** @file
Core types used in Mrc.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

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

