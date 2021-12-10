/** @file
  RISC-V OpesbSBI header file reference.

  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef EDK2_SBI_TYPES_H_
#define EDK2_SBI_TYPES_H_

#include <Base.h>

typedef INT8    s8;
typedef UINT8   u8;
typedef UINT8   uint8_t;

typedef INT16   s16;
typedef UINT16  u16;
typedef INT16   int16_t;
typedef UINT16  uint16_t;

typedef INT32   s32;
typedef UINT32  u32;
typedef INT32   int32_t;
typedef UINT32  uint32_t;

typedef INT64   s64;
typedef UINT64  u64;
typedef INT64   int64_t;
typedef UINT64  uint64_t;

// PRILX is not used in EDK2 but we need to define it here because when
// defining our own types, this constant is not defined but used by OpenSBI.
#define PRILX   "016lx"

typedef BOOLEAN  bool;
typedef unsigned long   ulong;
typedef UINT64   uintptr_t;
typedef UINT64   size_t;
typedef INT64    ssize_t;
typedef UINT64   virtual_addr_t;
typedef UINT64   virtual_size_t;
typedef UINT64   physical_addr_t;
typedef UINT64   physical_size_t;

#define true            TRUE
#define false           FALSE

#define __packed        __attribute__((packed))
#define __noreturn      __attribute__((noreturn))
#define __aligned(x)    __attribute__((aligned(x)))

#if defined(__GNUC__) || defined(__clang__)
  #define likely(x) __builtin_expect((x), 1)
  #define unlikely(x) __builtin_expect((x), 0)
#else
  #define likely(x) (x)
  #define unlikely(x) (x)
#endif

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({            \
  const typeof(((type *)0)->member) * __mptr = (ptr); \
  (type *)((char *)__mptr - offsetof(type, member)); })

#define array_size(x)   (sizeof(x) / sizeof((x)[0]))

#define CLAMP(a, lo, hi) MIN(MAX(a, lo), hi)
#define ROUNDUP(a, b) ((((a)-1) / (b) + 1) * (b))
#define ROUNDDOWN(a, b) ((a) / (b) * (b))

/* clang-format on */

#endif
