/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "CcExitTd.h"
#include <Library/CcExitLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/InstructionParsing.h>
#include "CcInstruction.h"

#define TDX_MMIO_READ   0
#define TDX_MMIO_WRITE  1

typedef union {
  struct {
    UINT32    Eax;
    UINT32    Edx;
  } Regs;
  UINT64    Val;
} MSR_DATA;

typedef union {
  UINT8    Val;
  struct {
    UINT8    B : 1;
    UINT8    X : 1;
    UINT8    R : 1;
    UINT8    W : 1;
  } Bits;
} REX;

typedef union {
  UINT8    Val;
  struct {
    UINT8    Rm  : 3;
    UINT8    Reg : 3;
    UINT8    Mod : 2;
  } Bits;
} MODRM;

typedef struct {
  UINT64    Regs[4];
} CPUID_DATA;

/**
  Handle an CPUID event.

  Use the TDVMCALL instruction to handle cpuid #ve

  @param[in, out] Regs             x64 processor context
  @param[in]      Veinfo           VE Info

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate
**/
STATIC
UINT64
EFIAPI
CpuIdExit (
  IN EFI_SYSTEM_CONTEXT_X64     *Regs,
  IN TDCALL_VEINFO_RETURN_DATA  *Veinfo
  )
{
  CPUID_DATA  CpuIdData;
  UINT64      Status;

  Status = TdVmCallCpuid (Regs->Rax, Regs->Rcx, &CpuIdData);

  if (Status == 0) {
    Regs->Rax = CpuIdData.Regs[0];
    Regs->Rbx = CpuIdData.Regs[1];
    Regs->Rcx = CpuIdData.Regs[2];
    Regs->Rdx = CpuIdData.Regs[3];
  }

  return Status;
}

/**
  Handle an IO event.

  Use the TDVMCALL instruction to handle either an IO read or an IO write.

  @param[in, out] Regs             x64 processor context
  @param[in]      Veinfo           VE Info

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate
**/
STATIC
UINT64
EFIAPI
IoExit (
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN TDCALL_VEINFO_RETURN_DATA   *Veinfo
  )
{
  BOOLEAN  Write;
  UINTN    Size;
  UINTN    Port;
  UINT64   Val;
  UINT64   RepCnt;
  UINT64   Status;

  Val    = 0;
  Status = 0;
  Write  = Veinfo->ExitQualification.Io.Direction ? FALSE : TRUE;
  Size   = Veinfo->ExitQualification.Io.Size + 1;
  Port   = Veinfo->ExitQualification.Io.Port;

  if (Veinfo->ExitQualification.Io.String) {
    //
    // If REP is set, get rep-cnt from Rcx
    //
    RepCnt = Veinfo->ExitQualification.Io.Rep ? Regs->Rcx : 1;

    while (RepCnt) {
      Val = 0;
      if (Write == TRUE) {
        CopyMem (&Val, (VOID *)Regs->Rsi, Size);
        Regs->Rsi += Size;
      }

      Status = TdVmCall (EXIT_REASON_IO_INSTRUCTION, Size, Write, Port, Val, (Write ? NULL : &Val));
      if (Status != 0) {
        break;
      }

      if (Write == FALSE) {
        CopyMem ((VOID *)Regs->Rdi, &Val, Size);
        Regs->Rdi += Size;
      }

      if (Veinfo->ExitQualification.Io.Rep) {
        Regs->Rcx -= 1;
      }

      RepCnt -= 1;
    }
  } else {
    if (Write == TRUE) {
      CopyMem (&Val, (VOID *)&Regs->Rax, Size);
    }

    Status = TdVmCall (EXIT_REASON_IO_INSTRUCTION, Size, Write, Port, Val, (Write ? NULL : &Val));
    if ((Status == 0) && (Write == FALSE)) {
      CopyMem ((VOID *)&Regs->Rax, &Val, Size);
    }
  }

  return Status;
}

/**
  Handle an READ MSR event.

  Use the TDVMCALL instruction to handle msr read

  @param[in, out] Regs             x64 processor context
  @param[in]      Veinfo           VE Info

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate
**/
STATIC
UINT64
ReadMsrExit (
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN TDCALL_VEINFO_RETURN_DATA   *Veinfo
  )
{
  MSR_DATA  Data;
  UINT64    Status;

  Status = TdVmCall (EXIT_REASON_MSR_READ, Regs->Rcx, 0, 0, 0, &Data);
  if (Status == 0) {
    Regs->Rax = Data.Regs.Eax;
    Regs->Rdx = Data.Regs.Edx;
  }

  return Status;
}

/**
  Handle an WRITE MSR event.

  Use the TDVMCALL instruction to handle msr write

  @param[in, out] Regs             x64 processor context
  @param[in]      Veinfo           VE Info

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate
**/
STATIC
UINT64
WriteMsrExit (
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN TDCALL_VEINFO_RETURN_DATA   *Veinfo
  )
{
  UINT64    Status;
  MSR_DATA  Data;

  Data.Regs.Eax = (UINT32)Regs->Rax;
  Data.Regs.Edx = (UINT32)Regs->Rdx;

  Status =  TdVmCall (EXIT_REASON_MSR_WRITE, Regs->Rcx, Data.Val, 0, 0, NULL);

  return Status;
}

STATIC
VOID
EFIAPI
TdxDecodeInstruction (
  IN UINT8   *Rip,
  IN UINT32  Length
  )
{
  UINTN  i;

  DEBUG ((DEBUG_INFO, "TDX: #TD[EPT] instruction (%p):", Rip));
  for (i = 0; i < MIN (15, Length); i++) {
    DEBUG ((DEBUG_INFO, "%02x ", Rip[i]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

#define TDX_DECODER_BUG_ON(x)               \
  if ((x)) {                                \
    TdxDecodeInstruction(Rip);              \
    TdVmCall(TDVMCALL_HALT, 0, 0, 0, 0, 0); \
    CpuDeadLoop (); \
  }

/**
 * Tdx MMIO access via TdVmcall.
 *
 * @param MmioSize      Size of the MMIO access
 * @param ReadOrWrite   Read or write operation
 * @param GuestPA       Guest physical address
 * @param Val           Pointer to the value which is read or written

 * @retval EFI_SUCCESS  Successfully access the mmio
 * @retval Others       Other errors as indicated
 */
STATIC
EFI_STATUS
TdxMmioReadWrite (
  IN UINT32  MmioSize,
  IN UINT32  ReadOrWrite,
  IN UINT64  GuestPA,
  IN UINT64  *Val
  )
{
  UINT64  TdStatus;

  if ((MmioSize != 1) && (MmioSize != 2) && (MmioSize != 4) && (MmioSize != 8)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid MmioSize - %d\n", __func__, MmioSize));
    return EFI_INVALID_PARAMETER;
  }

  if (Val == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TdStatus = 0;
  if (ReadOrWrite == TDX_MMIO_READ) {
    TdStatus = TdVmCall (TDVMCALL_MMIO, MmioSize, TDX_MMIO_READ, GuestPA, 0, Val);
  } else if (ReadOrWrite == TDX_MMIO_WRITE) {
    TdStatus = TdVmCall (TDVMCALL_MMIO, MmioSize, TDX_MMIO_WRITE, GuestPA, *Val, 0);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (TdStatus != 0) {
    DEBUG ((DEBUG_ERROR, "%a: TdVmcall failed with %llx\n", __func__, TdStatus));
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

typedef struct {
  UINT8                   OpCode;
  UINT32                  Bytes;
  EFI_PHYSICAL_ADDRESS    Address;
  UINT64                  Val;
  UINT64                  *Register;
  UINT32                  ReadOrWrite;
} MMIO_EXIT_PARSED_INSTRUCTION;

/**
 * Parse the MMIO instructions.
 *
 * @param Regs              Pointer to the EFI_SYSTEM_CONTEXT_X64 which includes the instructions
 * @param InstructionData   Pointer to the CC_INSTRUCTION_DATA
 * @param ParsedInstruction Pointer to the parsed instruction data
 *
 * @retval EFI_SUCCESS      Successfully parsed the instructions
 * @retval Others           Other error as indicated
 */
STATIC
EFI_STATUS
ParseMmioExitInstructions (
  IN OUT EFI_SYSTEM_CONTEXT_X64     *Regs,
  IN OUT CC_INSTRUCTION_DATA        *InstructionData,
  OUT MMIO_EXIT_PARSED_INSTRUCTION  *ParsedInstruction
  )
{
  EFI_STATUS            Status;
  UINT8                 OpCode;
  UINT8                 SignByte;
  UINT32                Bytes;
  EFI_PHYSICAL_ADDRESS  Address;
  UINT64                Val;
  UINT64                *Register;
  UINT32                ReadOrWrite;

  Address  = 0;
  Bytes    = 0;
  Register = NULL;
  Status   = EFI_SUCCESS;
  Val      = 0;

  Status = CcInitInstructionData (InstructionData, NULL, Regs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Initialize InstructionData failed! (%r)\n", __func__, Status));
    return Status;
  }

  OpCode = *(InstructionData->OpCodes);
  if (OpCode == TWO_BYTE_OPCODE_ESCAPE) {
    OpCode = *(InstructionData->OpCodes + 1);
  }

  switch (OpCode) {
    //
    // MMIO write (MOV reg/memX, regX)
    //
    case 0x88:
      Bytes = 1;
    //
    // fall through
    //
    case 0x89:
      CcDecodeModRm (Regs, InstructionData);
      Bytes = ((Bytes != 0) ? Bytes :
               (InstructionData->DataSize == Size16Bits) ? 2 :
               (InstructionData->DataSize == Size32Bits) ? 4 :
               (InstructionData->DataSize == Size64Bits) ? 8 :
               0);

      if (InstructionData->Ext.ModRm.Mod == 3) {
        DEBUG ((DEBUG_ERROR, "%a: Parse Ext.ModRm.Mod error! (OpCode: 0x%x)\n", __func__, OpCode));
        return EFI_UNSUPPORTED;
      }

      Address     = InstructionData->Ext.RmData;
      Val         = InstructionData->Ext.RegData;
      ReadOrWrite = TDX_MMIO_WRITE;

      break;

    //
    // MMIO write (MOV moffsetX, aX)
    //
    case 0xA2:
      Bytes = 1;
    //
    // fall through
    //
    case 0xA3:
      Bytes = ((Bytes != 0) ? Bytes :
               (InstructionData->DataSize == Size16Bits) ? 2 :
               (InstructionData->DataSize == Size32Bits) ? 4 :
               (InstructionData->DataSize == Size64Bits) ? 8 :
               0);

      InstructionData->ImmediateSize = (UINTN)(1 << InstructionData->AddrSize);
      InstructionData->End          += InstructionData->ImmediateSize;
      CopyMem (&Address, InstructionData->Immediate, InstructionData->ImmediateSize);

      Val         = Regs->Rax;
      ReadOrWrite = TDX_MMIO_WRITE;
      break;

    //
    // MMIO write (MOV reg/memX, immX)
    //
    case 0xC6:
      Bytes = 1;
    //
    // fall through
    //
    case 0xC7:
      CcDecodeModRm (Regs, InstructionData);
      Bytes = ((Bytes != 0) ? Bytes :
               (InstructionData->DataSize == Size16Bits) ? 2 :
               (InstructionData->DataSize == Size32Bits) ? 4 :
               (InstructionData->DataSize == Size64Bits) ? 8 :
               0);

      InstructionData->ImmediateSize = Bytes;
      InstructionData->End          += Bytes;

      Val = 0;
      CopyMem (&Val, InstructionData->Immediate, InstructionData->ImmediateSize);

      Address     = InstructionData->Ext.RmData;
      ReadOrWrite = TDX_MMIO_WRITE;

      break;

    //
    // MMIO read (MOV regX, reg/memX)
    //
    case 0x8A:
      Bytes = 1;
    //
    // fall through
    //
    case 0x8B:
      CcDecodeModRm (Regs, InstructionData);
      Bytes = ((Bytes != 0) ? Bytes :
               (InstructionData->DataSize == Size16Bits) ? 2 :
               (InstructionData->DataSize == Size32Bits) ? 4 :
               (InstructionData->DataSize == Size64Bits) ? 8 :
               0);
      if (InstructionData->Ext.ModRm.Mod == 3) {
        //
        // NPF on two register operands???
        //
        DEBUG ((DEBUG_ERROR, "%a: Parse Ext.ModRm.Mod error! (OpCode: 0x%x)\n", __func__, OpCode));
        return EFI_UNSUPPORTED;
      }

      Address     = InstructionData->Ext.RmData;
      ReadOrWrite = TDX_MMIO_READ;

      Register = CcGetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
      if (Register == NULL) {
        return EFI_ABORTED;
      }

      if (Bytes == 4) {
        //
        // Zero-extend for 32-bit operation
        //
        *Register = 0;
      }

      break;

    //
    // MMIO read (MOV aX, moffsetX)
    //
    case 0xA0:
      Bytes = 1;
    //
    // fall through
    //
    case 0xA1:
      Bytes = ((Bytes != 0) ? Bytes :
               (InstructionData->DataSize == Size16Bits) ? 2 :
               (InstructionData->DataSize == Size32Bits) ? 4 :
               (InstructionData->DataSize == Size64Bits) ? 8 :
               0);

      InstructionData->ImmediateSize = (UINTN)(1 << InstructionData->AddrSize);
      InstructionData->End          += InstructionData->ImmediateSize;

      Address = 0;
      CopyMem (
        &Address,
        InstructionData->Immediate,
        InstructionData->ImmediateSize
        );

      if (Bytes == 4) {
        //
        // Zero-extend for 32-bit operation
        //
        Regs->Rax = 0;
      }

      Register    = &Regs->Rax;
      ReadOrWrite = TDX_MMIO_READ;

      break;

    //
    // MMIO read w/ zero-extension ((MOVZX regX, reg/memX)
    //
    case 0xB6:
      Bytes = 1;
    //
    // fall through
    //
    case 0xB7:
      CcDecodeModRm (Regs, InstructionData);
      Bytes   = (Bytes != 0) ? Bytes : 2;
      Address = InstructionData->Ext.RmData;

      Register = CcGetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
      if (Register == NULL) {
        return EFI_ABORTED;
      }

      SetMem (Register, (UINTN)(1 << InstructionData->DataSize), 0);

      ReadOrWrite = TDX_MMIO_READ;

      break;

    //
    // MMIO read w/ sign-extension (MOVSX regX, reg/memX)
    //
    case 0xBE:
      Bytes = 1;
    //
    // fall through
    //
    case 0xBF:
      CcDecodeModRm (Regs, InstructionData);
      Bytes = (Bytes != 0) ? Bytes : 2;

      Address = InstructionData->Ext.RmData;

      if (Bytes == 1) {
        UINT8  *Data;
        Data     = (UINT8 *)&Val;
        SignByte = ((*Data & BIT7) != 0) ? 0xFF : 0x00;
      } else {
        UINT16  *Data;
        Data     = (UINT16 *)&Val;
        SignByte = ((*Data & BIT15) != 0) ? 0xFF : 0x00;
      }

      Register = CcGetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
      if (Register == NULL) {
        return EFI_ABORTED;
      }

      SetMem (Register, (UINTN)(1 << InstructionData->DataSize), SignByte);

      ReadOrWrite = TDX_MMIO_READ;

      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid MMIO opcode (%x)\n", __func__, OpCode));
      Status = EFI_UNSUPPORTED;
  }

  if (!EFI_ERROR (Status)) {
    ParsedInstruction->OpCode      = OpCode;
    ParsedInstruction->Address     = Address;
    ParsedInstruction->Bytes       = Bytes;
    ParsedInstruction->Register    = Register;
    ParsedInstruction->Val         = Val;
    ParsedInstruction->ReadOrWrite = ReadOrWrite;
  }

  return Status;
}

/**
  Handle an MMIO event.

  Use the TDVMCALL instruction to handle either an mmio read or an mmio write.

  @param[in, out] Regs             x64 processor context
  @param[in]      Veinfo           VE Info

  @retval 0                        Event handled successfully
**/
STATIC
UINT64
EFIAPI
MmioExit (
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN TDCALL_VEINFO_RETURN_DATA   *Veinfo
  )
{
  UINT64                        TdStatus;
  EFI_STATUS                    Status;
  TD_RETURN_DATA                TdReturnData;
  UINT8                         Gpaw;
  UINT64                        Val;
  UINT64                        TdSharedPageMask;
  CC_INSTRUCTION_DATA           InstructionData;
  MMIO_EXIT_PARSED_INSTRUCTION  ParsedInstruction;

  TdStatus = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  if (TdStatus == TDX_EXIT_REASON_SUCCESS) {
    Gpaw             = (UINT8)(TdReturnData.TdInfo.Gpaw & 0x3f);
    TdSharedPageMask = 1ULL << (Gpaw - 1);
  } else {
    DEBUG ((DEBUG_ERROR, "%a: TDCALL failed with status=%llx\n", __func__, TdStatus));
    goto FatalError;
  }

  if ((Veinfo->GuestPA & TdSharedPageMask) == 0) {
    DEBUG ((DEBUG_ERROR, "%a: EPT-violation #VE on private memory is not allowed!", __func__));
    goto FatalError;
  }

  Status = ParseMmioExitInstructions (Regs, &InstructionData, &ParsedInstruction);
  if (EFI_ERROR (Status)) {
    goto FatalError;
  }

  if (Veinfo->GuestPA != (ParsedInstruction.Address | TdSharedPageMask)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Address is not correct! (%d: 0x%llx != 0x%llx)\n",
      __func__,
      ParsedInstruction.OpCode,
      Veinfo->GuestPA,
      ParsedInstruction.Address
      ));
    goto FatalError;
  }

  if (ParsedInstruction.ReadOrWrite == TDX_MMIO_WRITE ) {
    Status = TdxMmioReadWrite (ParsedInstruction.Bytes, TDX_MMIO_WRITE, Veinfo->GuestPA, &ParsedInstruction.Val);
  } else if (ParsedInstruction.ReadOrWrite == TDX_MMIO_READ) {
    Val    = 0;
    Status = TdxMmioReadWrite (ParsedInstruction.Bytes, TDX_MMIO_READ, Veinfo->GuestPA, &Val);
    if (!EFI_ERROR (Status)) {
      CopyMem (ParsedInstruction.Register, &Val, ParsedInstruction.Bytes);
    }
  } else {
    goto FatalError;
  }

  if (EFI_ERROR (Status)) {
    goto FatalError;
  }

  //
  // We change instruction length to reflect true size so handler can
  // bump rip
  //
  Veinfo->ExitInstructionLength =  (UINT32)(CcInstructionLength (&InstructionData));
  TdxDecodeInstruction ((UINT8 *)Regs->Rip, Veinfo->ExitInstructionLength);

  return 0;

FatalError:
  TdVmCall (TDVMCALL_HALT, 0, 0, 0, 0, 0);
  CpuDeadLoop ();
  return 0;
}

/**
  Handle a #VE exception.

  Performs the necessary processing to handle a #VE exception.

  @param[in, out]  ExceptionType  Pointer to an EFI_EXCEPTION_TYPE to be set
                                  as value to use on error.
  @param[in, out]  SystemContext  Pointer to EFI_SYSTEM_CONTEXT

  @retval  EFI_SUCCESS            Exception handled
  @retval  EFI_UNSUPPORTED        #VE not supported, (new) exception value to
                                  propagate provided
  @retval  EFI_PROTOCOL_ERROR     #VE handling failed, (new) exception value to
                                  propagate provided

**/
EFI_STATUS
EFIAPI
CcExitHandleVe (
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINT64                  Status;
  TD_RETURN_DATA          ReturnData;
  EFI_SYSTEM_CONTEXT_X64  *Regs;

  Regs   = SystemContext.SystemContextX64;
  Status = TdCall (TDCALL_TDGETVEINFO, 0, 0, 0, &ReturnData);
  ASSERT (Status == 0);
  if (Status != 0) {
    DEBUG ((DEBUG_ERROR, "#VE happened. TDGETVEINFO failed with Status = 0x%llx\n", Status));
    TdVmCall (TDVMCALL_HALT, 0, 0, 0, 0, 0);
    CpuDeadLoop ();
  }

  switch (ReturnData.VeInfo.ExitReason) {
    case EXIT_REASON_CPUID:
      Status = CpuIdExit (Regs, &ReturnData.VeInfo);
      DEBUG ((
        DEBUG_VERBOSE,
        "CPUID #VE happened, ExitReasion is %d, ExitQualification = 0x%x.\n",
        ReturnData.VeInfo.ExitReason,
        ReturnData.VeInfo.ExitQualification.Val
        ));
      break;

    case EXIT_REASON_HLT:
      Status = TdVmCall (EXIT_REASON_HLT, 0, 0, 0, 0, 0);
      break;

    case EXIT_REASON_IO_INSTRUCTION:
      Status = IoExit (Regs, &ReturnData.VeInfo);
      DEBUG ((
        DEBUG_VERBOSE,
        "IO_Instruction #VE happened, ExitReasion is %d, ExitQualification = 0x%x.\n",
        ReturnData.VeInfo.ExitReason,
        ReturnData.VeInfo.ExitQualification.Val
        ));
      break;

    case EXIT_REASON_MSR_READ:
      Status = ReadMsrExit (Regs, &ReturnData.VeInfo);
      DEBUG ((
        DEBUG_VERBOSE,
        "RDMSR #VE happened, ExitReasion is %d, ExitQualification = 0x%x. Regs->Rcx=0x%llx, Status = 0x%llx\n",
        ReturnData.VeInfo.ExitReason,
        ReturnData.VeInfo.ExitQualification.Val,
        Regs->Rcx,
        Status
        ));
      break;

    case EXIT_REASON_MSR_WRITE:
      Status = WriteMsrExit (Regs, &ReturnData.VeInfo);
      DEBUG ((
        DEBUG_VERBOSE,
        "WRMSR #VE happened, ExitReasion is %d, ExitQualification = 0x%x. Regs->Rcx=0x%llx, Status = 0x%llx\n",
        ReturnData.VeInfo.ExitReason,
        ReturnData.VeInfo.ExitQualification.Val,
        Regs->Rcx,
        Status
        ));
      break;

    case EXIT_REASON_EPT_VIOLATION:
      Status = MmioExit (Regs, &ReturnData.VeInfo);
      DEBUG ((
        DEBUG_VERBOSE,
        "MMIO #VE happened, ExitReasion is %d, ExitQualification = 0x%x.\n",
        ReturnData.VeInfo.ExitReason,
        ReturnData.VeInfo.ExitQualification.Val
        ));
      break;

    case EXIT_REASON_VMCALL:
    case EXIT_REASON_MWAIT_INSTRUCTION:
    case EXIT_REASON_MONITOR_INSTRUCTION:
    case EXIT_REASON_WBINVD:
    case EXIT_REASON_RDPMC:
    case EXIT_REASON_INVD:
      /* Handle as nops. */
      break;

    default:
      DEBUG ((
        DEBUG_ERROR,
        "Unsupported #VE happened, ExitReason is %d, ExitQualification = 0x%x.\n",
        ReturnData.VeInfo.ExitReason,
        ReturnData.VeInfo.ExitQualification.Val
        ));

      ASSERT (FALSE);
      CpuDeadLoop ();
  }

  if (Status) {
    DEBUG ((
      DEBUG_ERROR,
      "#VE Error (0x%llx) returned from host, ExitReason is %d, ExitQualification = 0x%x.\n",
      Status,
      ReturnData.VeInfo.ExitReason,
      ReturnData.VeInfo.ExitQualification.Val
      ));

    TdVmCall (TDVMCALL_HALT, 0, 0, 0, 0, 0);
    CpuDeadLoop ();
  }

  SystemContext.SystemContextX64->Rip += ReturnData.VeInfo.ExitInstructionLength;
  return EFI_SUCCESS;
}
