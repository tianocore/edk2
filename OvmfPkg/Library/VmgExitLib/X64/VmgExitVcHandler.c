/** @file
  X64 #VC Exception Handler functon.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VmgExitLib.h>
#include <Register/Amd/Msr.h>

#define CR4_OSXSAVE (1 << 18)

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
// Non-automatic Exit function prototype
//
typedef
UINT64
(*NAE_EXIT) (
  GHCB                     *Ghcb,
  EFI_SYSTEM_CONTEXT_X64   *Regs,
  SEV_ES_INSTRUCTION_DATA  *InstructionData
  );


/**
  Checks the GHCB to determine if the specified register has been marked valid.

  The ValidBitmap area represents the areas of the GHCB that have been marked
  valid. Return an indication of whether the area of the GHCB that holds the
  specified register has been marked valid.

  @param[in] Ghcb    Pointer to the Guest-Hypervisor Communication Block
  @param[in] Reg     Offset in the GHCB of the register to check

  @retval TRUE       Register has been marked vald in the GHCB
  @retval FALSE      Register has not been marked valid in the GHCB

**/
STATIC
BOOLEAN
GhcbIsRegValid (
  IN GHCB                *Ghcb,
  IN GHCB_REGISTER       Reg
  )
{
  UINT32  RegIndex;
  UINT32  RegBit;

  RegIndex = Reg / 8;
  RegBit   = Reg & 0x07;

  return (Ghcb->SaveArea.ValidBitmap[RegIndex] & (1 << RegBit));
}

/**
  Marks a register as valid in the GHCB.

  The ValidBitmap area represents the areas of the GHCB that have been marked
  valid. Set the area of the GHCB that holds the specified register as valid.

  @param[in, out] Ghcb    Pointer to the Guest-Hypervisor Communication Block
  @param[in] Reg          Offset in the GHCB of the register to mark valid

**/
STATIC
VOID
GhcbSetRegValid (
  IN OUT GHCB                *Ghcb,
  IN     GHCB_REGISTER       Reg
  )
{
  UINT32  RegIndex;
  UINT32  RegBit;

  RegIndex = Reg / 8;
  RegBit   = Reg & 0x07;

  Ghcb->SaveArea.ValidBitmap[RegIndex] |= (1 << RegBit);
}

/**
  Return a pointer to the contents of the specified register.

  Based upon the input register, return a pointer to the registers contents
  in the x86 processor context.

  @param[in] Regs      x64 processor context
  @param[in] Register  Register to obtain pointer for

  @retval              Pointer to the contents of the requested register

**/
STATIC
INT64 *
GetRegisterPointer (
  IN EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN UINT8                    Register
  )
{
  UINT64 *Reg;

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

  return (INT64 *) Reg;
}

/**
  Update the instruction parsing context for displacement bytes.

  @param[in, out] InstructionData  Instruction parsing context
  @param[in]      Size             The instruction displacement size

**/
STATIC
VOID
UpdateForDisplacement (
  IN OUT SEV_ES_INSTRUCTION_DATA  *InstructionData,
  IN     UINTN                    Size
  )
{
  InstructionData->DisplacementSize = Size;
  InstructionData->Immediate += Size;
  InstructionData->End += Size;
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
  IN SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  SEV_ES_INSTRUCTION_OPCODE_EXT  *Ext;

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

  @retval                     The memory operand effective address

**/
STATIC
UINTN
GetEffectiveMemoryAddress (
  IN EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  SEV_ES_INSTRUCTION_OPCODE_EXT  *Ext;
  INTN                           EffectiveAddress;

  Ext = &InstructionData->Ext;
  EffectiveAddress = 0;

  if (IsRipRelative (InstructionData)) {
    /* RIP-relative displacement is a 32-bit signed value */
    INT32 RipRelative;

    RipRelative = *(INT32 *) InstructionData->Displacement;

    UpdateForDisplacement (InstructionData, 4);
    return (UINTN) ((INTN) Regs->Rip + RipRelative);
  }

  switch (Ext->ModRm.Mod) {
  case 1:
    UpdateForDisplacement (InstructionData, 1);
    EffectiveAddress += (INT8) (*(INT8 *) (InstructionData->Displacement));
    break;
  case 2:
    switch (InstructionData->AddrSize) {
    case Size16Bits:
      UpdateForDisplacement (InstructionData, 2);
      EffectiveAddress += (INT16) (*(INT16 *) (InstructionData->Displacement));
      break;
    default:
      UpdateForDisplacement (InstructionData, 4);
      EffectiveAddress += (INT32) (*(INT32 *) (InstructionData->Displacement));
      break;
    }
    break;
  }

  if (InstructionData->SibPresent) {
    if (Ext->Sib.Index != 4) {
      EffectiveAddress += (*GetRegisterPointer (Regs, Ext->Sib.Index) << Ext->Sib.Scale);
    }

    if ((Ext->Sib.Base != 5) || Ext->ModRm.Mod) {
      EffectiveAddress += *GetRegisterPointer (Regs, Ext->Sib.Base);
    } else {
      UpdateForDisplacement (InstructionData, 4);
      EffectiveAddress += (INT32) (*(INT32 *) (InstructionData->Displacement));
    }
  } else {
    EffectiveAddress += *GetRegisterPointer (Regs, Ext->ModRm.Rm);
  }

  return (UINTN) EffectiveAddress;
}

/**
  Decode a ModRM byte.

  Examine the instruction parsing context to decode a ModRM byte and the SIB
  byte, if present.

  @param[in]      Regs             x64 processor context
  @param[in, out] InstructionData  Instruction parsing context

**/
STATIC
VOID
DecodeModRm (
  IN     EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN OUT SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  SEV_ES_INSTRUCTION_REX_PREFIX  *RexPrefix;
  SEV_ES_INSTRUCTION_OPCODE_EXT  *Ext;
  SEV_ES_INSTRUCTION_MODRM       *ModRm;
  SEV_ES_INSTRUCTION_SIB         *Sib;

  RexPrefix = &InstructionData->RexPrefix;
  Ext = &InstructionData->Ext;
  ModRm = &InstructionData->ModRm;
  Sib = &InstructionData->Sib;

  InstructionData->ModRmPresent = TRUE;
  ModRm->Uint8 = *(InstructionData->End);

  InstructionData->Displacement++;
  InstructionData->Immediate++;
  InstructionData->End++;

  Ext->ModRm.Mod = ModRm->Bits.Mod;
  Ext->ModRm.Reg = (RexPrefix->Bits.BitR << 3) | ModRm->Bits.Reg;
  Ext->ModRm.Rm  = (RexPrefix->Bits.BitB << 3) | ModRm->Bits.Rm;

  Ext->RegData = *GetRegisterPointer (Regs, Ext->ModRm.Reg);

  if (Ext->ModRm.Mod == 3) {
    Ext->RmData = *GetRegisterPointer (Regs, Ext->ModRm.Rm);
  } else {
    if (ModRm->Bits.Rm == 4) {
      InstructionData->SibPresent = TRUE;
      Sib->Uint8 = *(InstructionData->End);

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

**/
STATIC
VOID
DecodePrefixes (
  IN     EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN OUT SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  SEV_ES_INSTRUCTION_MODE  Mode;
  SEV_ES_INSTRUCTION_SIZE  ModeDataSize;
  SEV_ES_INSTRUCTION_SIZE  ModeAddrSize;
  UINT8                    *Byte;

  /*TODO: Determine current mode - 64-bit for now */
  Mode = LongMode64Bit;
  ModeDataSize = Size32Bits;
  ModeAddrSize = Size64Bits;

  InstructionData->Mode = Mode;
  InstructionData->DataSize = ModeDataSize;
  InstructionData->AddrSize = ModeAddrSize;

  InstructionData->Prefixes = InstructionData->Begin;

  Byte = InstructionData->Prefixes;
  for ( ; ; Byte++, InstructionData->PrefixSize++) {
    //
    // Check the 0x40 to 0x4F range using an if statement here since some
    // compilers don't like the "case 0x40 ... 0x4F:" syntax. This avoids
    // 16 case statements below.
    //
    if ((*Byte >= 0x40) && (*Byte <= 0x4F)) {
      InstructionData->RexPrefix.Uint8 = *Byte;
      if (*Byte & 0x08)
        InstructionData->DataSize = Size64Bits;
      continue;
    }

    switch (*Byte) {
    case 0x26:
    case 0x2E:
    case 0x36:
    case 0x3E:
      if (Mode != LongMode64Bit) {
        InstructionData->SegmentSpecified = TRUE;
        InstructionData->Segment = (*Byte >> 3) & 3;
      }
      break;

    case 0x64:
      InstructionData->SegmentSpecified = TRUE;
      InstructionData->Segment = *Byte & 7;
      break;

    case 0x66:
      if (!InstructionData->RexPrefix.Uint8) {
        InstructionData->DataSize =
          (Mode == LongMode64Bit)       ? Size16Bits :
          (Mode == LongModeCompat32Bit) ? Size16Bits :
          (Mode == LongModeCompat16Bit) ? Size32Bits : 0;
      }
      break;

    case 0x67:
      InstructionData->AddrSize =
        (Mode == LongMode64Bit)       ? Size32Bits :
        (Mode == LongModeCompat32Bit) ? Size16Bits :
        (Mode == LongModeCompat16Bit) ? Size32Bits : 0;
      break;

    case 0xF0:
      break;

    case 0xF2:
      InstructionData->RepMode = RepZ;
      break;

    case 0xF3:
      InstructionData->RepMode = RepNZ;
      break;

    default:
      InstructionData->OpCodes = Byte;
      InstructionData->OpCodeSize = (*Byte == 0x0F) ? 2 : 1;

      InstructionData->End = Byte + InstructionData->OpCodeSize;
      InstructionData->Displacement = InstructionData->End;
      InstructionData->Immediate = InstructionData->End;
      return;
    }
  }
}

/**
  Determine instruction length

  Return the total length of the parsed instruction.

  @param[in] InstructionData  Instruction parsing context

  @retval                     Length of parsed instruction

**/
STATIC
UINT64
InstructionLength (
  IN SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  return (UINT64) (InstructionData->End - InstructionData->Begin);
}

/**
  Initialize the instruction parsing context.

  Initialize the instruction parsing context, which includes decoding the
  instruction prefixes.

  @param[in, out] InstructionData  Instruction parsing context
  @param[in]      Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in]      Regs             x64 processor context

**/
STATIC
VOID
InitInstructionData (
  IN OUT SEV_ES_INSTRUCTION_DATA  *InstructionData,
  IN     GHCB                     *Ghcb,
  IN     EFI_SYSTEM_CONTEXT_X64   *Regs
  )
{
  SetMem (InstructionData, sizeof (*InstructionData), 0);
  InstructionData->Ghcb = Ghcb;
  InstructionData->Begin = (UINT8 *) Regs->Rip;
  InstructionData->End = (UINT8 *) Regs->Rip;

  DecodePrefixes (Regs, InstructionData);
}

/**
  Report an unsupported event to the hypervisor

  Use the VMGEXIT support to report an unsupported event to the hypervisor.

  @param[in] Ghcb             Pointer to the Guest-Hypervisor Communication
                              Block
  @param[in] Regs             x64 processor context
  @param[in] InstructionData  Instruction parsing context

  @retval                     New exception value to propagate

**/
STATIC
UINT64
UnsupportedExit (
  IN GHCB                     *Ghcb,
  IN EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  Status;

  Status = VmgExit (Ghcb, SVM_EXIT_UNSUPPORTED, Regs->ExceptionData, 0);
  if (Status == 0) {
    GHCB_EVENT_INJECTION  Event;

    Event.Uint64 = 0;
    Event.Elements.Vector = GP_EXCEPTION;
    Event.Elements.Type   = GHCB_EVENT_INJECTION_TYPE_EXCEPTION;
    Event.Elements.Valid  = 1;

    Status = Event.Uint64;
  }

  return Status;
}

/**
  Handle an MMIO event.

  Use the VMGEXIT instruction to handle either an MMIO read or an MMIO write.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in, out] InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
MmioExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN OUT SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  ExitInfo1, ExitInfo2, Status;
  UINTN   Bytes;
  INTN    *Register;
  UINT8   OpCode, SignByte;

  Bytes = 0;

  OpCode = *(InstructionData->OpCodes);
  if (OpCode == 0x0F) {
    OpCode = *(InstructionData->OpCodes + 1);
  }

  switch (OpCode) {
  /* MMIO write */
  case 0x88:
    Bytes = 1;
  case 0x89:
    DecodeModRm (Regs, InstructionData);
    Bytes = (Bytes) ? Bytes
                    : (InstructionData->DataSize == Size16Bits) ? 2
                    : (InstructionData->DataSize == Size32Bits) ? 4
                    : (InstructionData->DataSize == Size64Bits) ? 8
                    : 0;

    if (InstructionData->Ext.ModRm.Mod == 3) {
      /* NPF on two register operands??? */
      return UnsupportedExit (Ghcb, Regs, InstructionData);
    }

    ExitInfo1 = InstructionData->Ext.RmData;
    ExitInfo2 = Bytes;
    CopyMem (Ghcb->SharedBuffer, &InstructionData->Ext.RegData, Bytes);

    Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
    Status = VmgExit (Ghcb, SVM_EXIT_MMIO_WRITE, ExitInfo1, ExitInfo2);
    if (Status) {
      return Status;
    }
    break;

  case 0xC6:
    Bytes = 1;
  case 0xC7:
    DecodeModRm (Regs, InstructionData);
    Bytes = (Bytes) ? Bytes
                    : (InstructionData->DataSize == Size16Bits) ? 2
                    : (InstructionData->DataSize == Size32Bits) ? 4
                    : 0;

    InstructionData->ImmediateSize = Bytes;
    InstructionData->End += Bytes;

    ExitInfo1 = InstructionData->Ext.RmData;
    ExitInfo2 = Bytes;
    CopyMem (Ghcb->SharedBuffer, InstructionData->Immediate, Bytes);

    Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
    Status = VmgExit (Ghcb, SVM_EXIT_MMIO_WRITE, ExitInfo1, ExitInfo2);
    if (Status) {
      return Status;
    }
    break;

  /* MMIO read */
  case 0x8A:
    Bytes = 1;
  case 0x8B:
    DecodeModRm (Regs, InstructionData);
    Bytes = (Bytes) ? Bytes
                    : (InstructionData->DataSize == Size16Bits) ? 2
                    : (InstructionData->DataSize == Size32Bits) ? 4
                    : (InstructionData->DataSize == Size64Bits) ? 8
                    : 0;
    if (InstructionData->Ext.ModRm.Mod == 3) {
      /* NPF on two register operands??? */
      return UnsupportedExit (Ghcb, Regs, InstructionData);
    }

    ExitInfo1 = InstructionData->Ext.RmData;
    ExitInfo2 = Bytes;

    Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
    Status = VmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
    if (Status) {
      return Status;
    }

    Register = GetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
    if (Bytes == 4) {
      /* Zero-extend for 32-bit operation */
      *Register = 0;
    }
    CopyMem (Register, Ghcb->SharedBuffer, Bytes);
    break;

  /* MMIO Read w/ zero-extension */
  case 0xB6:
    Bytes = 1;
  case 0xB7:
    Bytes = (Bytes) ? Bytes : 2;

    ExitInfo1 = InstructionData->Ext.RmData;
    ExitInfo2 = Bytes;

    Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
    Status = VmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
    if (Status) {
      return Status;
    }

    Register = GetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
    SetMem (Register, InstructionData->DataSize, 0);
    CopyMem (Register, Ghcb->SharedBuffer, Bytes);
    break;

  /* MMIO Read w/ sign-extension */
  case 0xBE:
    Bytes = 1;
  case 0xBF:
    Bytes = (Bytes) ? Bytes : 2;

    ExitInfo1 = InstructionData->Ext.RmData;
    ExitInfo2 = Bytes;

    Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
    Status = VmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
    if (Status) {
      return Status;
    }

    if (Bytes == 1) {
      UINT8 *Data = (UINT8 *) Ghcb->SharedBuffer;

      SignByte = (*Data & 0x80) ? 0xFF : 0x00;
    } else {
      UINT16 *Data = (UINT16 *) Ghcb->SharedBuffer;

      SignByte = (*Data & 0x8000) ? 0xFF : 0x00;
    }

    Register = GetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
    SetMem (Register, InstructionData->DataSize, SignByte);
    CopyMem (Register, Ghcb->SharedBuffer, Bytes);
    break;

  default:
    Status = GP_EXCEPTION;
    ASSERT (FALSE);
  }

  return Status;
}

/**
  Handle a WBINVD event.

  Use the VMGEXIT instruction to handle a WBINVD event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
WbinvdExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN     SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  Status;

  Status = VmgExit (Ghcb, SVM_EXIT_WBINVD, 0, 0);
  if (Status) {
    return Status;
  }

  return 0;
}

/**
  Handle an MSR event.

  Use the VMGEXIT instruction to handle either a RDMSR or WRMSR event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
MsrExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN     SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  ExitInfo1, Status;

  ExitInfo1 = 0;

  switch (*(InstructionData->OpCodes + 1)) {
  case 0x30: // WRMSR
    ExitInfo1 = 1;
    Ghcb->SaveArea.Rax = Regs->Rax;
    GhcbSetRegValid (Ghcb, GhcbRax);
    Ghcb->SaveArea.Rdx = Regs->Rdx;
    GhcbSetRegValid (Ghcb, GhcbRdx);
    /* Fallthrough */
  case 0x32: // RDMSR
    Ghcb->SaveArea.Rcx = Regs->Rcx;
    GhcbSetRegValid (Ghcb, GhcbRcx);
    break;
  default:
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Status = VmgExit (Ghcb, SVM_EXIT_MSR, ExitInfo1, 0);
  if (Status) {
    return Status;
  }

  if (!ExitInfo1) {
    if (!GhcbIsRegValid (Ghcb, GhcbRax) ||
        !GhcbIsRegValid (Ghcb, GhcbRdx)) {
      return UnsupportedExit (Ghcb, Regs, InstructionData);
    }
    Regs->Rax = Ghcb->SaveArea.Rax;
    Regs->Rdx = Ghcb->SaveArea.Rdx;
  }

  return 0;
}

#define IOIO_TYPE_STR       (1 << 2)
#define IOIO_TYPE_IN        1
#define IOIO_TYPE_INS       (IOIO_TYPE_IN | IOIO_TYPE_STR)
#define IOIO_TYPE_OUT       0
#define IOIO_TYPE_OUTS      (IOIO_TYPE_OUT | IOIO_TYPE_STR)

#define IOIO_REP            (1 << 3)

#define IOIO_ADDR_64        (1 << 9)
#define IOIO_ADDR_32        (1 << 8)
#define IOIO_ADDR_16        (1 << 7)

#define IOIO_DATA_32        (1 << 6)
#define IOIO_DATA_16        (1 << 5)
#define IOIO_DATA_8         (1 << 4)
#define IOIO_DATA_BYTES(x)  (((x) & 0x70) >> 4)

#define IOIO_SEG_ES         (0 << 10)
#define IOIO_SEG_DS         (3 << 10)

/**
  Build the IOIO event information.

  The IOIO event information identifies the type of IO operation to be performed
  by the hypervisor. Build this information based on the instruction data.

  @param[in]       Regs             x64 processor context
  @param[in, out]  InstructionData  Instruction parsing context

  @retval Others                    IOIO event information value

**/
STATIC
UINT64
IoioExitInfo (
  IN     EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN OUT SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  ExitInfo;

  ExitInfo = 0;

  switch (*(InstructionData->OpCodes)) {
  // INS opcodes
  case 0x6C:
  case 0x6D:
    ExitInfo |= IOIO_TYPE_INS;
    ExitInfo |= IOIO_SEG_ES;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  // OUTS opcodes
  case 0x6E:
  case 0x6F:
    ExitInfo |= IOIO_TYPE_OUTS;
    ExitInfo |= IOIO_SEG_DS;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  // IN immediate opcodes
  case 0xE4:
  case 0xE5:
    InstructionData->ImmediateSize = 1;
    InstructionData->End++;
    ExitInfo |= IOIO_TYPE_IN;
    ExitInfo |= ((*(InstructionData->OpCodes + 1)) << 16);
    break;

  // OUT immediate opcodes
  case 0xE6:
  case 0xE7:
    InstructionData->ImmediateSize = 1;
    InstructionData->End++;
    ExitInfo |= IOIO_TYPE_OUT;
    ExitInfo |= ((*(InstructionData->OpCodes + 1)) << 16) | IOIO_TYPE_OUT;
    break;

  // IN register opcodes
  case 0xEC:
  case 0xED:
    ExitInfo |= IOIO_TYPE_IN;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  // OUT register opcodes
  case 0xEE:
  case 0xEF:
    ExitInfo |= IOIO_TYPE_OUT;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  default:
    return 0;
  }

  switch (*(InstructionData->OpCodes)) {
  case 0x6C:
  case 0x6E:
  case 0xE4:
  case 0xE6:
  case 0xEC:
  case 0xEE:
    // Single-byte opcodes
    ExitInfo |= IOIO_DATA_8;
    break;

  default:
    // Length determined by instruction parsing
    ExitInfo |= (InstructionData->DataSize == Size16Bits) ? IOIO_DATA_16
                                                          : IOIO_DATA_32;
  }

  switch (InstructionData->AddrSize) {
  case Size16Bits:
    ExitInfo |= IOIO_ADDR_16;
    break;

  case Size32Bits:
    ExitInfo |= IOIO_ADDR_32;
    break;

  case Size64Bits:
    ExitInfo |= IOIO_ADDR_64;
    break;

  default:
    break;
  }

  if (InstructionData->RepMode) {
    ExitInfo |= IOIO_REP;
  }

  return ExitInfo;
}

/**
  Handle an IOIO event.

  Use the VMGEXIT instruction to handle an IOIO event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
IoioExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN     SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64   ExitInfo1, ExitInfo2, Status;
  BOOLEAN  String;

  ExitInfo1 = IoioExitInfo (Regs, InstructionData);
  if (!ExitInfo1) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  String = (ExitInfo1 & IOIO_TYPE_STR) ? TRUE : FALSE;
  if (String) {
    UINTN  IoBytes, VmgExitBytes;
    UINTN  GhcbCount, OpCount;

    Status = 0;

    IoBytes = (ExitInfo1 >> 4) & 0x7;
    GhcbCount = sizeof (Ghcb->SharedBuffer) / IoBytes;

    OpCount = (ExitInfo1 & IOIO_REP) ? Regs->Rcx : 1;
    while (OpCount) {
      ExitInfo2 = MIN (OpCount, GhcbCount);
      VmgExitBytes = ExitInfo2 * IoBytes;

      if (!(ExitInfo1 & IOIO_TYPE_IN)) {
        CopyMem (Ghcb->SharedBuffer, (VOID *) Regs->Rsi, VmgExitBytes);
        Regs->Rsi += VmgExitBytes;
      }

      Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
      Status = VmgExit (Ghcb, SVM_EXIT_IOIO_PROT, ExitInfo1, ExitInfo2);
      if (Status) {
        return Status;
      }

      if (ExitInfo1 & IOIO_TYPE_IN) {
        CopyMem ((VOID *) Regs->Rdi, Ghcb->SharedBuffer, VmgExitBytes);
        Regs->Rdi += VmgExitBytes;
      }

      if (ExitInfo1 & IOIO_REP) {
        Regs->Rcx -= ExitInfo2;
      }

      OpCount -= ExitInfo2;
    }
  } else {
    if (ExitInfo1 & IOIO_TYPE_IN) {
      Ghcb->SaveArea.Rax = 0;
    } else {
      CopyMem (&Ghcb->SaveArea.Rax, &Regs->Rax, IOIO_DATA_BYTES (ExitInfo1));
    }
    GhcbSetRegValid (Ghcb, GhcbRax);

    Status = VmgExit (Ghcb, SVM_EXIT_IOIO_PROT, ExitInfo1, 0);
    if (Status) {
      return Status;
    }

    if (ExitInfo1 & IOIO_TYPE_IN) {
      if (!GhcbIsRegValid (Ghcb, GhcbRax)) {
        return UnsupportedExit (Ghcb, Regs, InstructionData);
      }
      CopyMem (&Regs->Rax, &Ghcb->SaveArea.Rax, IOIO_DATA_BYTES (ExitInfo1));
    }
  }

  return 0;
}

/**
  Handle a CPUID event.

  Use the VMGEXIT instruction to handle a CPUID event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
CpuidExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN     SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  Status;

  Ghcb->SaveArea.Rax = Regs->Rax;
  GhcbSetRegValid (Ghcb, GhcbRax);
  Ghcb->SaveArea.Rcx = Regs->Rcx;
  GhcbSetRegValid (Ghcb, GhcbRcx);
  if (Regs->Rax == 0x0000000d) {
    Ghcb->SaveArea.XCr0 = (AsmReadCr4 () & CR4_OSXSAVE) ? AsmXGetBv (0) : 1;
    GhcbSetRegValid (Ghcb, GhcbXCr0);
  }

  Status = VmgExit (Ghcb, SVM_EXIT_CPUID, 0, 0);
  if (Status) {
    return Status;
  }

  if (!GhcbIsRegValid (Ghcb, GhcbRax) ||
      !GhcbIsRegValid (Ghcb, GhcbRbx) ||
      !GhcbIsRegValid (Ghcb, GhcbRcx) ||
      !GhcbIsRegValid (Ghcb, GhcbRdx)) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }
  Regs->Rax = Ghcb->SaveArea.Rax;
  Regs->Rbx = Ghcb->SaveArea.Rbx;
  Regs->Rcx = Ghcb->SaveArea.Rcx;
  Regs->Rdx = Ghcb->SaveArea.Rdx;

  return 0;
}

/**
  Handle a RDPMC event.

  Use the VMGEXIT instruction to handle a RDPMC event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
RdpmcExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN     SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  Status;

  Ghcb->SaveArea.Rcx = Regs->Rcx;
  GhcbSetRegValid (Ghcb, GhcbRcx);

  Status = VmgExit (Ghcb, SVM_EXIT_RDPMC, 0, 0);
  if (Status) {
    return Status;
  }

  if (!GhcbIsRegValid (Ghcb, GhcbRax) ||
      !GhcbIsRegValid (Ghcb, GhcbRdx)) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }
  Regs->Rax = Ghcb->SaveArea.Rax;
  Regs->Rdx = Ghcb->SaveArea.Rdx;

  return 0;
}

/**
  Handle a RDTSC event.

  Use the VMGEXIT instruction to handle a RDTSC event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @retval Others                   New exception value to propagate

**/
STATIC
UINT64
RdtscExit (
  IN OUT GHCB                     *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64   *Regs,
  IN     SEV_ES_INSTRUCTION_DATA  *InstructionData
  )
{
  UINT64  Status;

  Status = VmgExit (Ghcb, SVM_EXIT_RDTSC, 0, 0);
  if (Status) {
    return Status;
  }

  if (!GhcbIsRegValid (Ghcb, GhcbRax) ||
      !GhcbIsRegValid (Ghcb, GhcbRdx)) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }
  Regs->Rax = Ghcb->SaveArea.Rax;
  Regs->Rdx = Ghcb->SaveArea.Rdx;

  return 0;
}

/**
  Handle a #VC exception.

  Performs the necessary processing to handle a #VC exception.

  @param[in, out]  ExceptionType  Pointer to an EFI_EXCEPTION_TYPE to be set
                                  as value to use on error.
  @param[in, out]  SystemContext  Pointer to EFI_SYSTEM_CONTEXT

  @retval  EFI_SUCCESS            Exception handled
  @retval  EFI_UNSUPPORTED        #VC not supported, (new) exception value to
                                  propagate provided
  @retval  EFI_PROTOCOL_ERROR     #VC handling failed, (new) exception value to
                                  propagate provided

**/
EFI_STATUS
EFIAPI
VmgExitHandleVc (
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  EFI_SYSTEM_CONTEXT_X64    *Regs;
  GHCB                      *Ghcb;
  NAE_EXIT                  NaeExit;
  SEV_ES_INSTRUCTION_DATA   InstructionData;
  UINT64                    ExitCode, Status;
  EFI_STATUS                VcRet;

  VcRet = EFI_SUCCESS;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  ASSERT (Msr.GhcbInfo.Function == 0);
  ASSERT (Msr.Ghcb != 0);

  Regs = SystemContext.SystemContextX64;
  Ghcb = Msr.Ghcb;

  VmgInit (Ghcb);

  ExitCode = Regs->ExceptionData;
  switch (ExitCode) {
  case SVM_EXIT_RDTSC:
    NaeExit = RdtscExit;
    break;

  case SVM_EXIT_RDPMC:
    NaeExit = RdpmcExit;
    break;

  case SVM_EXIT_CPUID:
    NaeExit = CpuidExit;
    break;

  case SVM_EXIT_IOIO_PROT:
    NaeExit = IoioExit;
    break;

  case SVM_EXIT_MSR:
    NaeExit = MsrExit;
    break;

  case SVM_EXIT_WBINVD:
    NaeExit = WbinvdExit;
    break;

  case SVM_EXIT_NPF:
    NaeExit = MmioExit;
    break;

  default:
    NaeExit = UnsupportedExit;
  }

  InitInstructionData (&InstructionData, Ghcb, Regs);

  Status = NaeExit (Ghcb, Regs, &InstructionData);
  if (Status == 0) {
    Regs->Rip += InstructionLength (&InstructionData);
  } else {
    GHCB_EVENT_INJECTION  Event;

    Event.Uint64 = Status;
    if (Event.Elements.ErrorCodeValid) {
      Regs->ExceptionData = Event.Elements.ErrorCode;
    } else {
      Regs->ExceptionData = 0;
    }

    *ExceptionType = Event.Elements.Vector;

    VcRet = EFI_PROTOCOL_ERROR;
  }

  VmgDone (Ghcb);

  return VcRet;
}
