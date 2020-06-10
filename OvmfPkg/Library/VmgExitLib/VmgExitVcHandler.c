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
#include <Register/Intel/Cpuid.h>
#include <IndustryStandard/InstructionParsing.h>

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

typedef struct {
  UINT8  Rm;
  UINT8  Reg;
  UINT8  Mod;
} SEV_ES_INSTRUCTION_MODRM_EXT;

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

  INSTRUCTION_REX_PREFIX         RexPrefix;

  BOOLEAN                        ModRmPresent;
  INSTRUCTION_MODRM              ModRm;

  BOOLEAN                        SibPresent;
  INSTRUCTION_SIB                Sib;

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

  return ((Ghcb->SaveArea.ValidBitmap[RegIndex] & (1 << RegBit)) != 0);
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

  //
  // Always in 64-bit mode
  //
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
        InstructionData->Segment = (*Byte >> 3) & 3;
      }
      break;

    case OVERRIDE_SEGMENT_FS:
    case OVERRIDE_SEGMENT_GS:
      InstructionData->SegmentSpecified = TRUE;
      InstructionData->Segment = *Byte & 7;
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
      InstructionData->OpCodes = Byte;
      InstructionData->OpCodeSize = (*Byte == TWO_BYTE_OPCODE_ESCAPE) ? 2 : 1;

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

  @return                     Length of parsed instruction

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

  @return                     New exception value to propagate

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
  Build the IOIO event information.

  The IOIO event information identifies the type of IO operation to be performed
  by the hypervisor. Build this information based on the instruction data.

  @param[in]       Regs             x64 processor context
  @param[in, out]  InstructionData  Instruction parsing context

  @return                           IOIO event information value

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
  //
  // INS opcodes
  //
  case 0x6C:
  case 0x6D:
    ExitInfo |= IOIO_TYPE_INS;
    ExitInfo |= IOIO_SEG_ES;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  //
  // OUTS opcodes
  //
  case 0x6E:
  case 0x6F:
    ExitInfo |= IOIO_TYPE_OUTS;
    ExitInfo |= IOIO_SEG_DS;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  //
  // IN immediate opcodes
  //
  case 0xE4:
  case 0xE5:
    InstructionData->ImmediateSize = 1;
    InstructionData->End++;
    ExitInfo |= IOIO_TYPE_IN;
    ExitInfo |= ((*(InstructionData->OpCodes + 1)) << 16);
    break;

  //
  // OUT immediate opcodes
  //
  case 0xE6:
  case 0xE7:
    InstructionData->ImmediateSize = 1;
    InstructionData->End++;
    ExitInfo |= IOIO_TYPE_OUT;
    ExitInfo |= ((*(InstructionData->OpCodes + 1)) << 16) | IOIO_TYPE_OUT;
    break;

  //
  // IN register opcodes
  //
  case 0xEC:
  case 0xED:
    ExitInfo |= IOIO_TYPE_IN;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  //
  // OUT register opcodes
  //
  case 0xEE:
  case 0xEF:
    ExitInfo |= IOIO_TYPE_OUT;
    ExitInfo |= ((Regs->Rdx & 0xffff) << 16);
    break;

  default:
    return 0;
  }

  switch (*(InstructionData->OpCodes)) {
  //
  // Single-byte opcodes
  //
  case 0x6C:
  case 0x6E:
  case 0xE4:
  case 0xE6:
  case 0xEC:
  case 0xEE:
    ExitInfo |= IOIO_DATA_8;
    break;

  //
  // Length determined by instruction parsing
  //
  default:
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

  if (InstructionData->RepMode != 0) {
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
  @return                          New exception value to propagate

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
  BOOLEAN  IsString;

  ExitInfo1 = IoioExitInfo (Regs, InstructionData);
  if (ExitInfo1 == 0) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  IsString = ((ExitInfo1 & IOIO_TYPE_STR) != 0) ? TRUE : FALSE;
  if (IsString) {
    UINTN  IoBytes, VmgExitBytes;
    UINTN  GhcbCount, OpCount;

    Status = 0;

    IoBytes = IOIO_DATA_BYTES (ExitInfo1);
    GhcbCount = sizeof (Ghcb->SharedBuffer) / IoBytes;

    OpCount = ((ExitInfo1 & IOIO_REP) != 0) ? Regs->Rcx : 1;
    while (OpCount) {
      ExitInfo2 = MIN (OpCount, GhcbCount);
      VmgExitBytes = ExitInfo2 * IoBytes;

      if ((ExitInfo1 & IOIO_TYPE_IN) == 0) {
        CopyMem (Ghcb->SharedBuffer, (VOID *) Regs->Rsi, VmgExitBytes);
        Regs->Rsi += VmgExitBytes;
      }

      Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
      Status = VmgExit (Ghcb, SVM_EXIT_IOIO_PROT, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

      if ((ExitInfo1 & IOIO_TYPE_IN) != 0) {
        CopyMem ((VOID *) Regs->Rdi, Ghcb->SharedBuffer, VmgExitBytes);
        Regs->Rdi += VmgExitBytes;
      }

      if ((ExitInfo1 & IOIO_REP) != 0) {
        Regs->Rcx -= ExitInfo2;
      }

      OpCount -= ExitInfo2;
    }
  } else {
    if ((ExitInfo1 & IOIO_TYPE_IN) != 0) {
      Ghcb->SaveArea.Rax = 0;
    } else {
      CopyMem (&Ghcb->SaveArea.Rax, &Regs->Rax, IOIO_DATA_BYTES (ExitInfo1));
    }
    GhcbSetRegValid (Ghcb, GhcbRax);

    Status = VmgExit (Ghcb, SVM_EXIT_IOIO_PROT, ExitInfo1, 0);
    if (Status != 0) {
      return Status;
    }

    if ((ExitInfo1 & IOIO_TYPE_IN) != 0) {
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
  @return                          New exception value to propagate

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
  if (Regs->Rax == CPUID_EXTENDED_STATE) {
    IA32_CR4  Cr4;

    Cr4.UintN = AsmReadCr4 ();
    Ghcb->SaveArea.XCr0 = (Cr4.Bits.OSXSAVE == 1) ? AsmXGetBv (0) : 1;
    GhcbSetRegValid (Ghcb, GhcbXCr0);
  }

  Status = VmgExit (Ghcb, SVM_EXIT_CPUID, 0, 0);
  if (Status != 0) {
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
  case SVM_EXIT_CPUID:
    NaeExit = CpuidExit;
    break;

  case SVM_EXIT_IOIO_PROT:
    NaeExit = IoioExit;
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
    if (Event.Elements.ErrorCodeValid != 0) {
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
