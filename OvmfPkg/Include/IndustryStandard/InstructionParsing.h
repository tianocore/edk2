/** @file
  Instruction parsing support definitions.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __INSTRUCTION_PARSING_H__
#define __INSTRUCTION_PARSING_H__

#include <Base.h>
#include <Uefi.h>

//
// Instruction REX prefix definition
//
typedef union {
  struct {
    UINT8  BitB:1;
    UINT8  BitX:1;
    UINT8  BitR:1;
    UINT8  BitW:1;
    UINT8  Rex:4;
  } Bits;

  UINT8  Uint8;
} INSTRUCTION_REX_PREFIX;

//
// Instruction ModRM definition
//
typedef union {
  struct {
    UINT8  Rm:3;
    UINT8  Reg:3;
    UINT8  Mod:2;
  } Bits;

  UINT8  Uint8;
} INSTRUCTION_MODRM;

//
// Instruction SIB definition
//
typedef union {
  struct {
    UINT8  Base:3;
    UINT8  Index:3;
    UINT8  Scale:2;
  } Bits;

  UINT8  Uint8;
} INSTRUCTION_SIB;

//
// Legacy Instruction Prefixes
//
#define OVERRIDE_SEGMENT_CS          0x2E
#define OVERRIDE_SEGMENT_DS          0x3E
#define OVERRIDE_SEGMENT_ES          0x26
#define OVERRIDE_SEGMENT_SS          0x36
#define OVERRIDE_SEGMENT_FS          0x64
#define OVERRIDE_SEGMENT_GS          0x65
#define OVERRIDE_OPERAND_SIZE        0x66
#define OVERRIDE_ADDRESS_SIZE        0x67
#define LOCK_PREFIX                  0xF0
#define REPNZ_PREFIX                 0xF2
#define REPZ_PREFIX                  0xF3

//
// REX Prefixes
//
#define REX_PREFIX_START             0x40
#define REX_PREFIX_STOP              0x4F
#define REX_64BIT_OPERAND_SIZE_MASK  0x08

//
// Two-byte Opcode Flag
//
#define TWO_BYTE_OPCODE_ESCAPE       0x0F

#endif
