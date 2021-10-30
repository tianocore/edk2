/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "VmTdExitHandler.h"
#include <Library/VmgExitLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/InstructionParsing.h>

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

  Val   = 0;
  Write = Veinfo->ExitQualification.Io.Direction ? FALSE : TRUE;
  Size  = Veinfo->ExitQualification.Io.Size + 1;
  Port  = Veinfo->ExitQualification.Io.Port;

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
  IN UINT8  *Rip
  )
{
  UINTN  i;

  DEBUG ((DEBUG_INFO, "TDX: #TD[EPT] instruction (%p):", Rip));
  for (i = 0; i < 15; i++) {
    DEBUG ((DEBUG_INFO, "%02x:", Rip[i]));
  }

  DEBUG ((DEBUG_INFO, "\n"));
}

#define TDX_DECODER_BUG_ON(x)               \
  if ((x)) {                                \
    TdxDecodeInstruction(Rip);              \
    TdVmCall(TDVMCALL_HALT, 0, 0, 0, 0, 0); \
  }

STATIC
UINT64 *
EFIAPI
GetRegFromContext (
  IN EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN UINTN                   RegIndex
  )
{
  switch (RegIndex) {
    case 0: return &Regs->Rax;
      break;
    case 1: return &Regs->Rcx;
      break;
    case 2: return &Regs->Rdx;
      break;
    case 3: return &Regs->Rbx;
      break;
    case 4: return &Regs->Rsp;
      break;
    case 5: return &Regs->Rbp;
      break;
    case 6: return &Regs->Rsi;
      break;
    case 7: return &Regs->Rdi;
      break;
    case 8: return &Regs->R8;
      break;
    case 9: return &Regs->R9;
      break;
    case 10: return &Regs->R10;
      break;
    case 11: return &Regs->R11;
      break;
    case 12: return &Regs->R12;
      break;
    case 13: return &Regs->R13;
      break;
    case 14: return &Regs->R14;
      break;
    case 15: return &Regs->R15;
      break;
  }

  return NULL;
}

/**
  Handle an MMIO event.

  Use the TDVMCALL instruction to handle either an mmio read or an mmio write.

  @param[in, out] Regs             x64 processor context
  @param[in]      Veinfo           VE Info

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate
**/
STATIC
INTN
EFIAPI
MmioExit (
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN TDCALL_VEINFO_RETURN_DATA   *Veinfo
  )
{
  UINT64   Status;
  UINT32   MmioSize;
  UINT32   RegSize;
  UINT8    OpCode;
  BOOLEAN  SeenRex;
  UINT64   *Reg;
  UINT8    *Rip;
  UINT64   Val;
  UINT32   OpSize;
  MODRM    ModRm;
  REX      Rex;

  Rip     = (UINT8 *)Regs->Rip;
  Val     = 0;
  Rex.Val = 0;
  SeenRex = FALSE;

  //
  // Default to 32bit transfer
  //
  OpSize = 4;

  do {
    OpCode = *Rip++;
    if (OpCode == 0x66) {
      OpSize = 2;
    } else if ((OpCode == 0x64) || (OpCode == 0x65) || (OpCode == 0x67)) {
      continue;
    } else if ((OpCode >= 0x40) && (OpCode <= 0x4f)) {
      SeenRex = TRUE;
      Rex.Val = OpCode;
    } else {
      break;
    }
  } while (TRUE);

  //
  // We need to have at least 2 more bytes for this instruction
  //
  TDX_DECODER_BUG_ON (((UINT64)Rip - Regs->Rip) > 13);

  OpCode = *Rip++;
  //
  // Two-byte opecode, get next byte
  //
  if (OpCode == 0x0F) {
    OpCode = *Rip++;
  }

  switch (OpCode) {
    case 0x88:
    case 0x8A:
    case 0xB6:
      MmioSize = 1;
      break;
    case 0xB7:
      MmioSize = 2;
      break;
    default:
      MmioSize = Rex.Bits.W ? 8 : OpSize;
      break;
  }

  /* Punt on AH/BH/CH/DH unless it shows up. */
  ModRm.Val = *Rip++;
  TDX_DECODER_BUG_ON (MmioSize == 1 && ModRm.Bits.Reg > 4 && !SeenRex && OpCode != 0xB6);
  Reg = GetRegFromContext (Regs, ModRm.Bits.Reg | ((int)Rex.Bits.R << 3));
  TDX_DECODER_BUG_ON (!Reg);

  if (ModRm.Bits.Rm == 4) {
    ++Rip; /* SIB byte */
  }

  if ((ModRm.Bits.Mod == 2) || ((ModRm.Bits.Mod == 0) && (ModRm.Bits.Rm == 5))) {
    Rip += 4; /* DISP32 */
  } else if (ModRm.Bits.Mod == 1) {
    ++Rip;  /* DISP8 */
  }

  switch (OpCode) {
    case 0x88:
    case 0x89:
      CopyMem ((void *)&Val, Reg, MmioSize);
      Status = TdVmCall (TDVMCALL_MMIO, MmioSize, 1, Veinfo->GuestPA, Val, 0);
      break;
    case 0xC7:
      CopyMem ((void *)&Val, Rip, OpSize);
      Status = TdVmCall (TDVMCALL_MMIO, MmioSize, 1, Veinfo->GuestPA, Val, 0);
      Rip   += OpSize;
    default:
      //
      // 32-bit write registers are zero extended to the full register
      // Hence 'MOVZX r[32/64], r/m16' is
      // hardcoded to reg size 8, and the straight MOV case has a reg
      // size of 8 in the 32-bit read case.
      //
      switch (OpCode) {
        case 0xB6:
          RegSize = Rex.Bits.W ? 8 : OpSize;
          break;
        case 0xB7:
          RegSize =  8;
          break;
        default:
          RegSize = MmioSize == 4 ? 8 : MmioSize;
          break;
      }

      Status = TdVmCall (TDVMCALL_MMIO, MmioSize, 0, Veinfo->GuestPA, 0, &Val);
      if (Status == 0) {
        ZeroMem (Reg, RegSize);
        CopyMem (Reg, (void *)&Val, MmioSize);
      }
  }

  if (Status == 0) {
    TDX_DECODER_BUG_ON (((UINT64)Rip - Regs->Rip) > 15);

    //
    // We change instruction length to reflect true size so handler can
    // bump rip
    //
    Veinfo->ExitInstructionLength =  (UINT32)((UINT64)Rip - Regs->Rip);
  }

  return Status;
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
VmTdExitHandleVe (
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
  }

  SystemContext.SystemContextX64->Rip += ReturnData.VeInfo.ExitInstructionLength;
  return EFI_SUCCESS;
}
