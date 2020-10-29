/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ARM_LIB_PRIVATE_H__
#define __ARM_LIB_PRIVATE_H__

typedef union {
  struct {
    UINT32    InD           :1;
    UINT32    Level         :3;
    UINT32    TnD           :1;
    UINT32    Reserved      :27;
  } Bits;
  UINT32 Data;
} CSSELR_DATA;

typedef enum
{
  CSSELR_CACHE_TYPE_DATA_OR_UNIFIED = 0,
  CSSELR_CACHE_TYPE_INSTRUCTION = 1
} CSSELR_CACHE_TYPE;

typedef union {
  struct {
    UINT64    LineSize           :3;
    UINT64    Associativity      :10;
    UINT64    NumSets            :15;
    UINT64    Unknown            :4;
    UINT64    Reserved           :32;
  } BitsNonCcidx;
  struct {
    UINT64    LineSize           :3;
    UINT64    Associativity      :21;
    UINT64    Reserved1          :8;
    UINT64    NumSets            :24;
    UINT64    Reserved2          :8;
  } BitsCcidx;
  UINT64 Data;
} CCSIDR_DATA;

typedef union {
  struct {
    UINT32 NumSets               :24;
    UINT32 Reserved              :8;
  } Bits;
  UINT32 Data;
} CSSIDR2_DATA;

// The lower 32 bits are the same for both AARCH32 and AARCH64
// so we can use the same structure for both.
typedef union {
  struct {
    UINT32    Ctype1   : 3;
    UINT32    Ctype2   : 3;
    UINT32    Ctype3   : 3;
    UINT32    Ctype4   : 3;
    UINT32    Ctype5   : 3;
    UINT32    Ctype6   : 3;
    UINT32    Ctype7   : 3;
    UINT32    LoUIS    : 3;
    UINT32    LoC      : 3;
    UINT32    LoUU     : 3;
    UINT32    Icb      : 3;
  } Bits;
  UINT32 Data;
} CLIDR_DATA;

typedef enum {
  CLIDR_CACHE_TYPE_NONE = 0,
  CLIDR_CACHE_TYPE_INSTRUCTION_ONLY = 1,
  CLIDR_CACHE_TYPE_DATA_ONLY = 2,
  CLIDR_CACHE_TYPE_SEPARATE = 3,
  CLIDR_CACHE_TYPE_UNIFIED = 4
} CLIDR_CACHE_TYPE;

#define CLIDR_GET_CACHE_TYPE(x, level) ((x >> (3 * level)) & 0b111)

VOID
CPSRMaskInsert (
  IN  UINT32  Mask,
  IN  UINT32  Value
  );

UINT32
CPSRRead (
  VOID
  );

UINTN
ReadCCSIDR (
  IN UINT32 CSSELR
  );

UINT32
ReadCLIDR (
  VOID
  );

#endif // __ARM_LIB_PRIVATE_H__
