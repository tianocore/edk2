/** @file
  Confidential Computing X64 Instruction

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CC_INSTRUCTION_H_
#define CC_INSTRUCTION_H_

#include <Base.h>
#include <Uefi.h>
#include <Register/Amd/Ghcb.h>
#include <IndustryStandard/InstructionParsing.h>
#include <Protocol/DebugSupport.h>

//
// Instruction execution mode definition
//
typedef enum {
  LongMode64Bit = 0,
  LongModeCompat32Bit,
  LongModeCompat16Bit,
} CC_INSTRUCTION_MODE;

//
// Instruction size definition (for operand and address)
//
typedef enum {
  Size8Bits = 0,
  Size16Bits,
  Size32Bits,
  Size64Bits,
} CC_INSTRUCTION_SIZE;

//
// Intruction segment definition
//
typedef enum {
  SegmentEs = 0,
  SegmentCs,
  SegmentSs,
  SegmentDs,
  SegmentFs,
  SegmentGs,
} CC_INSTRUCTION_SEGMENT;

//
// Instruction rep function definition
//
typedef enum {
  RepNone = 0,
  RepZ,
  RepNZ,
} CC_INSTRUCTION_REP;

typedef struct {
  UINT8    Rm;
  UINT8    Reg;
  UINT8    Mod;
} CC_INSTRUCTION_MODRM_EXT;

typedef struct {
  UINT8    Base;
  UINT8    Index;
  UINT8    Scale;
} CC_INSTRUCTION_SIB_EXT;

//
// Instruction opcode definition
//
typedef struct {
  CC_INSTRUCTION_MODRM_EXT    ModRm;

  CC_INSTRUCTION_SIB_EXT      Sib;

  UINTN                       RegData;
  UINTN                       RmData;
} CC_INSTRUCTION_OPCODE_EXT;

//
// Instruction parsing context definition
//
typedef struct {
  GHCB                         *Ghcb;

  CC_INSTRUCTION_MODE          Mode;
  CC_INSTRUCTION_SIZE          DataSize;
  CC_INSTRUCTION_SIZE          AddrSize;
  BOOLEAN                      SegmentSpecified;
  CC_INSTRUCTION_SEGMENT       Segment;
  CC_INSTRUCTION_REP           RepMode;

  UINT8                        *Begin;
  UINT8                        *End;

  UINT8                        *Prefixes;
  UINT8                        *OpCodes;
  UINT8                        *Displacement;
  UINT8                        *Immediate;

  INSTRUCTION_REX_PREFIX       RexPrefix;

  BOOLEAN                      ModRmPresent;
  INSTRUCTION_MODRM            ModRm;

  BOOLEAN                      SibPresent;
  INSTRUCTION_SIB              Sib;

  UINTN                        PrefixSize;
  UINTN                        OpCodeSize;
  UINTN                        DisplacementSize;
  UINTN                        ImmediateSize;

  CC_INSTRUCTION_OPCODE_EXT    Ext;
} CC_INSTRUCTION_DATA;

EFI_STATUS
CcInitInstructionData (
  IN OUT CC_INSTRUCTION_DATA     *InstructionData,
  IN     GHCB                    *Ghcb,
  IN     EFI_SYSTEM_CONTEXT_X64  *Regs
  );

/**
  Return a pointer to the contents of the specified register.

  Based upon the input register, return a pointer to the registers contents
  in the x86 processor context.

  @param[in] Regs      x64 processor context
  @param[in] Register  Register to obtain pointer for

  @return              Pointer to the contents of the requested register

**/
UINT64 *
CcGetRegisterPointer (
  IN EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN UINT8                   Register
  );

/**
  Decode a ModRM byte.

  Examine the instruction parsing context to decode a ModRM byte and the SIB
  byte, if present.

  @param[in]      Regs             x64 processor context
  @param[in, out] InstructionData  Instruction parsing context

**/
VOID
CcDecodeModRm (
  IN     EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN OUT CC_INSTRUCTION_DATA     *InstructionData
  );

/**
  Determine instruction length

  Return the total length of the parsed instruction.

  @param[in] InstructionData  Instruction parsing context

  @return                     Length of parsed instruction

**/
UINT64
CcInstructionLength (
  IN CC_INSTRUCTION_DATA  *InstructionData
  );

/**
  Initialize the instruction parsing context.

  Initialize the instruction parsing context, which includes decoding the
  instruction prefixes.

  @param[in, out] InstructionData  Instruction parsing context
  @param[in]      Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in]      Regs             x64 processor context

  @retval         EFI_SUCCESS      Successfully initialize InstructionData
  @retval         Others           Other error as indicated
**/
EFI_STATUS
CcInitInstructionData (
  IN OUT CC_INSTRUCTION_DATA     *InstructionData,
  IN     GHCB                    *Ghcb,
  IN     EFI_SYSTEM_CONTEXT_X64  *Regs
  );

#endif
