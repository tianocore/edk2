/** @file
  X64 #VC instruction parsing support definitions.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VC_INSTRUCTION_PARSING_H__
#define __VC_INSTRUCTION_PARSING_H__

#include <Base.h>
#include <Uefi.h>

//
// Instruction execution mode definition
//
typedef enum {
  LongMode64Bit        = 0,
  LongModeCompat32Bit,
  LongModeCompat16Bit,
} SEV_ES_INSTRUCTION_MODE;

//
// Instruction size definition (for operand and address)
//
typedef enum {
  Size8Bits            = 0,
  Size16Bits,
  Size32Bits,
  Size64Bits,
} SEV_ES_INSTRUCTION_SIZE;

//
// Intruction segment definition
//
typedef enum {
  SegmentEs            = 0,
  SegmentCs,
  SegmentSs,
  SegmentDs,
  SegmentFs,
  SegmentGs,
} SEV_ES_INSTRUCTION_SEGMENT;

//
// Instruction rep function definition
//
typedef enum {
  RepNone              = 0,
  RepZ,
  RepNZ,
} SEV_ES_INSTRUCTION_REP;

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
} SEV_ES_INSTRUCTION_REX_PREFIX;

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
} SEV_ES_INSTRUCTION_MODRM;

typedef struct {
  UINT8  Rm;
  UINT8  Reg;
  UINT8  Mod;
} SEV_ES_INSTRUCTION_MODRM_EXT;

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
} SEV_ES_INSTRUCTION_SIB;

typedef struct {
  UINT8  Base;
  UINT8  Index;
  UINT8  Scale;
} SEV_ES_INSTRUCTION_SIB_EXT;

//
// Instruction opcode definition
//
typedef struct {
  SEV_ES_INSTRUCTION_MODRM_EXT  ModRm;

  SEV_ES_INSTRUCTION_SIB_EXT    Sib;

  UINTN                         RegData;
  UINTN                         RmData;
} SEV_ES_INSTRUCTION_OPCODE_EXT;

//
// Instruction parsing context definition
//
typedef struct {
  GHCB                           *Ghcb;

  SEV_ES_INSTRUCTION_MODE        Mode;
  SEV_ES_INSTRUCTION_SIZE        DataSize;
  SEV_ES_INSTRUCTION_SIZE        AddrSize;
  BOOLEAN                        SegmentSpecified;
  SEV_ES_INSTRUCTION_SEGMENT     Segment;
  SEV_ES_INSTRUCTION_REP         RepMode;

  UINT8                          *Begin;
  UINT8                          *End;

  UINT8                          *Prefixes;
  UINT8                          *OpCodes;
  UINT8                          *Displacement;
  UINT8                          *Immediate;

  SEV_ES_INSTRUCTION_REX_PREFIX  RexPrefix;

  BOOLEAN                        ModRmPresent;
  SEV_ES_INSTRUCTION_MODRM       ModRm;

  BOOLEAN                        SibPresent;
  SEV_ES_INSTRUCTION_SIB         Sib;

  UINTN                          PrefixSize;
  UINTN                          OpCodeSize;
  UINTN                          DisplacementSize;
  UINTN                          ImmediateSize;

  SEV_ES_INSTRUCTION_OPCODE_EXT  Ext;
} SEV_ES_INSTRUCTION_DATA;

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
