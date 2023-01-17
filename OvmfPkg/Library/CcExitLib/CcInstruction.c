/** @file
  X64 Instruction function.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Register/Intel/Cpuid.h>
#include <IndustryStandard/InstructionParsing.h>
#include "CcInstruction.h"

#define MAX_INSTRUCTION_LENGTH  15

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
  )
{
  UINT64  *Reg;

  switch (Register) {
    case 0:
      Reg = &Regs->Rax;
      break;
    case 1:
      Reg = &Regs->Rcx;
      break;
    case 2:
      Reg = &Regs->Rdx;
      break;
    case 3:
      Reg = &Regs->Rbx;
      break;
    case 4:
      Reg = &Regs->Rsp;
      break;
    case 5:
      Reg = &Regs->Rbp;
      break;
    case 6:
      Reg = &Regs->Rsi;
      break;
    case 7:
      Reg = &Regs->Rdi;
      break;
    case 8:
      Reg = &Regs->R8;
      break;
    case 9:
      Reg = &Regs->R9;
      break;
    case 10:
      Reg = &Regs->R10;
      break;
    case 11:
      Reg = &Regs->R11;
      break;
    case 12:
      Reg = &Regs->R12;
      break;
    case 13:
      Reg = &Regs->R13;
      break;
    case 14:
      Reg = &Regs->R14;
      break;
    case 15:
      Reg = &Regs->R15;
      break;
    default:
      Reg = NULL;
  }

  ASSERT (Reg != NULL);

  return Reg;
}

/**
  Update the instruction parsing context for displacement bytes.

  @param[in, out] InstructionData  Instruction parsing context
  @param[in]      Size             The instruction displacement size

**/
STATIC
VOID
UpdateForDisplacement (
  IN OUT CC_INSTRUCTION_DATA  *InstructionData,
  IN     UINTN                Size
  )
{
  InstructionData->DisplacementSize = Size;
  InstructionData->Immediate       += Size;
  InstructionData->End             += Size;
}

/**
  Determine if an instruction address if RIP relative.

  Examine the instruction parsing context to determine if the address offset
  is relative to the instruction pointer.

  @param[in] InstructionData  Instruction parsing context

  @retval TRUE                Instruction addressing is RIP relative
  @retval FALSE               Instruction addressing is not RIP relative

**/
STATIC
BOOLEAN
IsRipRelative (
  IN CC_INSTRUCTION_DATA  *InstructionData
  )
{
  CC_INSTRUCTION_OPCODE_EXT  *Ext;

  Ext = &InstructionData->Ext;

  return ((InstructionData->Mode == LongMode64Bit) &&
          (Ext->ModRm.Mod == 0) &&
          (Ext->ModRm.Rm == 5)  &&
          (InstructionData->SibPresent == FALSE));
}

/**
  Return the effective address of a memory operand.

  Examine the instruction parsing context to obtain the effective memory
  address of a memory operand.

  @param[in] Regs             x64 processor context
  @param[in] InstructionData  Instruction parsing context

  @return                     The memory operand effective address

**/
STATIC
UINT64
GetEffectiveMemoryAddress (
  IN EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN CC_INSTRUCTION_DATA     *InstructionData
  )
{
  CC_INSTRUCTION_OPCODE_EXT  *Ext;
  UINT64                     EffectiveAddress;

  Ext              = &InstructionData->Ext;
  EffectiveAddress = 0;

  if (IsRipRelative (InstructionData)) {
    //
    // RIP-relative displacement is a 32-bit signed value
    //
    INT32  RipRelative;

    RipRelative = *(INT32 *)InstructionData->Displacement;

    UpdateForDisplacement (InstructionData, 4);

    //
    // Negative displacement is handled by standard UINT64 wrap-around.
    //
    return Regs->Rip + (UINT64)RipRelative;
  }

  switch (Ext->ModRm.Mod) {
    case 1:
      UpdateForDisplacement (InstructionData, 1);
      EffectiveAddress += (UINT64)(*(INT8 *)(InstructionData->Displacement));
      break;
    case 2:
      switch (InstructionData->AddrSize) {
        case Size16Bits:
          UpdateForDisplacement (InstructionData, 2);
          EffectiveAddress += (UINT64)(*(INT16 *)(InstructionData->Displacement));
          break;
        default:
          UpdateForDisplacement (InstructionData, 4);
          EffectiveAddress += (UINT64)(*(INT32 *)(InstructionData->Displacement));
          break;
      }

      break;
  }

  if (InstructionData->SibPresent) {
    INT64  Displacement;

    if (Ext->Sib.Index != 4) {
      CopyMem (
        &Displacement,
        CcGetRegisterPointer (Regs, Ext->Sib.Index),
        sizeof (Displacement)
        );
      Displacement *= (INT64)(1 << Ext->Sib.Scale);

      //
      // Negative displacement is handled by standard UINT64 wrap-around.
      //
      EffectiveAddress += (UINT64)Displacement;
    }

    if ((Ext->Sib.Base != 5) || Ext->ModRm.Mod) {
      EffectiveAddress += *CcGetRegisterPointer (Regs, Ext->Sib.Base);
    } else {
      UpdateForDisplacement (InstructionData, 4);
      EffectiveAddress += (UINT64)(*(INT32 *)(InstructionData->Displacement));
    }
  } else {
    EffectiveAddress += *CcGetRegisterPointer (Regs, Ext->ModRm.Rm);
  }

  return EffectiveAddress;
}

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
  )
{
  CC_INSTRUCTION_OPCODE_EXT  *Ext;
  INSTRUCTION_REX_PREFIX     *RexPrefix;
  INSTRUCTION_MODRM          *ModRm;
  INSTRUCTION_SIB            *Sib;

  RexPrefix = &InstructionData->RexPrefix;
  Ext       = &InstructionData->Ext;
  ModRm     = &InstructionData->ModRm;
  Sib       = &InstructionData->Sib;

  InstructionData->ModRmPresent = TRUE;
  ModRm->Uint8                  = *(InstructionData->End);

  InstructionData->Displacement++;
  InstructionData->Immediate++;
  InstructionData->End++;

  Ext->ModRm.Mod = ModRm->Bits.Mod;
  Ext->ModRm.Reg = (RexPrefix->Bits.BitR << 3) | ModRm->Bits.Reg;
  Ext->ModRm.Rm  = (RexPrefix->Bits.BitB << 3) | ModRm->Bits.Rm;

  Ext->RegData = *CcGetRegisterPointer (Regs, Ext->ModRm.Reg);

  if (Ext->ModRm.Mod == 3) {
    Ext->RmData = *CcGetRegisterPointer (Regs, Ext->ModRm.Rm);
  } else {
    if (ModRm->Bits.Rm == 4) {
      InstructionData->SibPresent = TRUE;
      Sib->Uint8                  = *(InstructionData->End);

      InstructionData->Displacement++;
      InstructionData->Immediate++;
      InstructionData->End++;

      Ext->Sib.Scale = Sib->Bits.Scale;
      Ext->Sib.Index = (RexPrefix->Bits.BitX << 3) | Sib->Bits.Index;
      Ext->Sib.Base  = (RexPrefix->Bits.BitB << 3) | Sib->Bits.Base;
    }

    Ext->RmData = GetEffectiveMemoryAddress (Regs, InstructionData);
  }
}

/**
  Decode instruction prefixes.

  Parse the instruction data to track the instruction prefixes that have
  been used.

  @param[in]      Regs             x64 processor context
  @param[in, out] InstructionData  Instruction parsing context

  @retval         EFI_SUCCESS      Successfully decode Prefixes
  @retval         Others           Other error as indicated
**/
STATIC
EFI_STATUS
DecodePrefixes (
  IN     EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN OUT CC_INSTRUCTION_DATA     *InstructionData
  )
{
  CC_INSTRUCTION_MODE  Mode;
  CC_INSTRUCTION_SIZE  ModeDataSize;
  CC_INSTRUCTION_SIZE  ModeAddrSize;
  UINT8                *Byte;
  UINT8                ParsedLength;

  ParsedLength = 0;

  //
  // Always in 64-bit mode
  //
  Mode         = LongMode64Bit;
  ModeDataSize = Size32Bits;
  ModeAddrSize = Size64Bits;

  InstructionData->Mode     = Mode;
  InstructionData->DataSize = ModeDataSize;
  InstructionData->AddrSize = ModeAddrSize;

  InstructionData->Prefixes = InstructionData->Begin;

  Byte = InstructionData->Prefixes;
  for ( ; ParsedLength <= MAX_INSTRUCTION_LENGTH; Byte++, InstructionData->PrefixSize++, ParsedLength++) {
    //
    // Check the 0x40 to 0x4F range using an if statement here since some
    // compilers don't like the "case 0x40 ... 0x4F:" syntax. This avoids
    // 16 case statements below.
    //
    if ((*Byte >= REX_PREFIX_START) && (*Byte <= REX_PREFIX_STOP)) {
      InstructionData->RexPrefix.Uint8 = *Byte;
      if ((*Byte & REX_64BIT_OPERAND_SIZE_MASK) != 0) {
        InstructionData->DataSize = Size64Bits;
      }

      continue;
    }

    switch (*Byte) {
      case OVERRIDE_SEGMENT_CS:
      case OVERRIDE_SEGMENT_DS:
      case OVERRIDE_SEGMENT_ES:
      case OVERRIDE_SEGMENT_SS:
        if (Mode != LongMode64Bit) {
          InstructionData->SegmentSpecified = TRUE;
          InstructionData->Segment          = (*Byte >> 3) & 3;
        }

        break;

      case OVERRIDE_SEGMENT_FS:
      case OVERRIDE_SEGMENT_GS:
        InstructionData->SegmentSpecified = TRUE;
        InstructionData->Segment          = *Byte & 7;
        break;

      case OVERRIDE_OPERAND_SIZE:
        if (InstructionData->RexPrefix.Uint8 == 0) {
          InstructionData->DataSize =
            (Mode == LongMode64Bit)       ? Size16Bits :
            (Mode == LongModeCompat32Bit) ? Size16Bits :
            (Mode == LongModeCompat16Bit) ? Size32Bits : 0;
        }

        break;

      case OVERRIDE_ADDRESS_SIZE:
        InstructionData->AddrSize =
          (Mode == LongMode64Bit)       ? Size32Bits :
          (Mode == LongModeCompat32Bit) ? Size16Bits :
          (Mode == LongModeCompat16Bit) ? Size32Bits : 0;
        break;

      case LOCK_PREFIX:
        break;

      case REPZ_PREFIX:
        InstructionData->RepMode = RepZ;
        break;

      case REPNZ_PREFIX:
        InstructionData->RepMode = RepNZ;
        break;

      default:
        InstructionData->OpCodes    = Byte;
        InstructionData->OpCodeSize = (*Byte == TWO_BYTE_OPCODE_ESCAPE) ? 2 : 1;

        InstructionData->End          = Byte + InstructionData->OpCodeSize;
        InstructionData->Displacement = InstructionData->End;
        InstructionData->Immediate    = InstructionData->End;
        return EFI_SUCCESS;
    }
  }

  return EFI_ABORTED;
}

/**
  Determine instruction length

  Return the total length of the parsed instruction.

  @param[in] InstructionData  Instruction parsing context

  @return                     Length of parsed instruction

**/
UINT64
CcInstructionLength (
  IN CC_INSTRUCTION_DATA  *InstructionData
  )
{
  return (UINT64)(InstructionData->End - InstructionData->Begin);
}

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
  )
{
  SetMem (InstructionData, sizeof (*InstructionData), 0);
  InstructionData->Ghcb  = Ghcb;
  InstructionData->Begin = (UINT8 *)Regs->Rip;
  InstructionData->End   = (UINT8 *)Regs->Rip;

  return DecodePrefixes (Regs, InstructionData);
}
