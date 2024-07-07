/** @file
  X64 #VC Exception Handler functon.

  Copyright (C) 2020 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/LocalApicLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/CcExitLib.h>
#include <Library/AmdSvsmLib.h>
#include <Register/Amd/Msr.h>
#include <Register/Intel/Cpuid.h>
#include <IndustryStandard/InstructionParsing.h>

#include "CcExitVcHandler.h"
#include "CcInstruction.h"

//
// Non-automatic Exit function prototype
//
typedef
UINT64
(*NAE_EXIT) (
  GHCB                    *Ghcb,
  EFI_SYSTEM_CONTEXT_X64  *Regs,
  CC_INSTRUCTION_DATA     *InstructionData
  );

//
// SEV-SNP Cpuid table entry/function
//
typedef PACKED struct {
  UINT32    EaxIn;
  UINT32    EcxIn;
  UINT64    Unused;
  UINT64    Unused2;
  UINT32    Eax;
  UINT32    Ebx;
  UINT32    Ecx;
  UINT32    Edx;
  UINT64    Reserved;
} SEV_SNP_CPUID_FUNCTION;

//
// SEV-SNP Cpuid page format
//
typedef PACKED struct {
  UINT32                    Count;
  UINT32                    Reserved1;
  UINT64                    Reserved2;
  SEV_SNP_CPUID_FUNCTION    function[0];
} SEV_SNP_CPUID_INFO;

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
  IN GHCB                    *Ghcb,
  IN EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN CC_INSTRUCTION_DATA     *InstructionData
  )
{
  UINT64  Status;

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_UNSUPPORTED, Regs->ExceptionData, 0);
  if (Status == 0) {
    GHCB_EVENT_INJECTION  Event;

    Event.Uint64          = 0;
    Event.Elements.Vector = GP_EXCEPTION;
    Event.Elements.Type   = GHCB_EVENT_INJECTION_TYPE_EXCEPTION;
    Event.Elements.Valid  = 1;

    Status = Event.Uint64;
  }

  return Status;
}

/**
  Validate that the MMIO memory access is not to encrypted memory.

  Examine the pagetable entry for the memory specified. MMIO should not be
  performed against encrypted memory.

  @param[in] Ghcb           Pointer to the Guest-Hypervisor Communication Block
  @param[in] MemoryAddress  Memory address to validate
  @param[in] MemoryLength   Memory length to validate

  @retval 0          Memory is not encrypted
  @return            New exception value to propogate

**/
STATIC
UINT64
ValidateMmioMemory (
  IN GHCB   *Ghcb,
  IN UINTN  MemoryAddress,
  IN UINTN  MemoryLength
  )
{
  MEM_ENCRYPT_SEV_ADDRESS_RANGE_STATE  State;
  GHCB_EVENT_INJECTION                 GpEvent;

  State = MemEncryptSevGetAddressRangeState (
            0,
            MemoryAddress,
            MemoryLength
            );
  if (State == MemEncryptSevAddressRangeUnencrypted) {
    return 0;
  }

  //
  // Any state other than unencrypted is an error, issue a #GP.
  //
  DEBUG ((
    DEBUG_ERROR,
    "MMIO using encrypted memory: %lx\n",
    (UINT64)MemoryAddress
    ));
  GpEvent.Uint64          = 0;
  GpEvent.Elements.Vector = GP_EXCEPTION;
  GpEvent.Elements.Type   = GHCB_EVENT_INJECTION_TYPE_EXCEPTION;
  GpEvent.Elements.Valid  = 1;

  return GpEvent.Uint64;
}

/**
  Handle an MMIO event.

  Use the VMGEXIT instruction to handle either an MMIO read or an MMIO write.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in, out] InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
MmioExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN OUT CC_INSTRUCTION_DATA     *InstructionData
  )
{
  UINT64  ExitInfo1, ExitInfo2, Status;
  UINTN   Bytes;
  UINT64  *Register;
  UINT8   OpCode, SignByte;
  UINTN   Address;

  Bytes = 0;

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
        //
        // NPF on two register operands???
        //
        return UnsupportedExit (Ghcb, Regs, InstructionData);
      }

      Status = ValidateMmioMemory (Ghcb, InstructionData->Ext.RmData, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = InstructionData->Ext.RmData;
      ExitInfo2 = Bytes;
      CopyMem (Ghcb->SharedBuffer, &InstructionData->Ext.RegData, Bytes);

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_WRITE, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

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

      //
      // This code is X64 only, so a possible 8-byte copy to a UINTN is ok.
      // Use a STATIC_ASSERT to be certain the code is being built as X64.
      //
      STATIC_ASSERT (
        sizeof (UINTN) == sizeof (UINT64),
        "sizeof (UINTN) != sizeof (UINT64), this file must be built as X64"
        );

      Address = 0;
      CopyMem (
        &Address,
        InstructionData->Immediate,
        InstructionData->ImmediateSize
        );

      Status = ValidateMmioMemory (Ghcb, Address, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = Address;
      ExitInfo2 = Bytes;
      CopyMem (Ghcb->SharedBuffer, &Regs->Rax, Bytes);

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_WRITE, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

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
               0);

      InstructionData->ImmediateSize = Bytes;
      InstructionData->End          += Bytes;

      Status = ValidateMmioMemory (Ghcb, InstructionData->Ext.RmData, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = InstructionData->Ext.RmData;
      ExitInfo2 = Bytes;
      CopyMem (Ghcb->SharedBuffer, InstructionData->Immediate, Bytes);

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_WRITE, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

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
        return UnsupportedExit (Ghcb, Regs, InstructionData);
      }

      Status = ValidateMmioMemory (Ghcb, InstructionData->Ext.RmData, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = InstructionData->Ext.RmData;
      ExitInfo2 = Bytes;

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

      Register = CcGetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
      if (Bytes == 4) {
        //
        // Zero-extend for 32-bit operation
        //
        *Register = 0;
      }

      CopyMem (Register, Ghcb->SharedBuffer, Bytes);
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

      //
      // This code is X64 only, so a possible 8-byte copy to a UINTN is ok.
      // Use a STATIC_ASSERT to be certain the code is being built as X64.
      //
      STATIC_ASSERT (
        sizeof (UINTN) == sizeof (UINT64),
        "sizeof (UINTN) != sizeof (UINT64), this file must be built as X64"
        );

      Address = 0;
      CopyMem (
        &Address,
        InstructionData->Immediate,
        InstructionData->ImmediateSize
        );

      Status = ValidateMmioMemory (Ghcb, Address, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = Address;
      ExitInfo2 = Bytes;

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

      if (Bytes == 4) {
        //
        // Zero-extend for 32-bit operation
        //
        Regs->Rax = 0;
      }

      CopyMem (&Regs->Rax, Ghcb->SharedBuffer, Bytes);
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
      Bytes = (Bytes != 0) ? Bytes : 2;

      Status = ValidateMmioMemory (Ghcb, InstructionData->Ext.RmData, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = InstructionData->Ext.RmData;
      ExitInfo2 = Bytes;

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

      Register = CcGetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
      SetMem (Register, (UINTN)(1 << InstructionData->DataSize), 0);
      CopyMem (Register, Ghcb->SharedBuffer, Bytes);
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

      Status = ValidateMmioMemory (Ghcb, InstructionData->Ext.RmData, Bytes);
      if (Status != 0) {
        return Status;
      }

      ExitInfo1 = InstructionData->Ext.RmData;
      ExitInfo2 = Bytes;

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_MMIO_READ, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

      if (Bytes == 1) {
        UINT8  *Data;

        Data     = (UINT8 *)Ghcb->SharedBuffer;
        SignByte = ((*Data & BIT7) != 0) ? 0xFF : 0x00;
      } else {
        UINT16  *Data;

        Data     = (UINT16 *)Ghcb->SharedBuffer;
        SignByte = ((*Data & BIT15) != 0) ? 0xFF : 0x00;
      }

      Register = CcGetRegisterPointer (Regs, InstructionData->Ext.ModRm.Reg);
      SetMem (Register, (UINTN)(1 << InstructionData->DataSize), SignByte);
      CopyMem (Register, Ghcb->SharedBuffer, Bytes);
      break;

    default:
      DEBUG ((DEBUG_ERROR, "Invalid MMIO opcode (%x)\n", OpCode));
      Status = GP_EXCEPTION;
      ASSERT (FALSE);
  }

  return Status;
}

/**
  Handle a MWAIT event.

  Use the VMGEXIT instruction to handle a MWAIT event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
MwaitExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  Ghcb->SaveArea.Rax = Regs->Rax;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRax);
  Ghcb->SaveArea.Rcx = Regs->Rcx;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRcx);

  return CcExitVmgExit (Ghcb, SVM_EXIT_MWAIT, 0, 0);
}

/**
  Handle a MONITOR event.

  Use the VMGEXIT instruction to handle a MONITOR event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
MonitorExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  Ghcb->SaveArea.Rax = Regs->Rax;  // Identity mapped, so VA = PA
  CcExitVmgSetOffsetValid (Ghcb, GhcbRax);
  Ghcb->SaveArea.Rcx = Regs->Rcx;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRcx);
  Ghcb->SaveArea.Rdx = Regs->Rdx;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRdx);

  return CcExitVmgExit (Ghcb, SVM_EXIT_MONITOR, 0, 0);
}

/**
  Handle a WBINVD event.

  Use the VMGEXIT instruction to handle a WBINVD event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
WbinvdExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  return CcExitVmgExit (Ghcb, SVM_EXIT_WBINVD, 0, 0);
}

/**
  Handle a RDTSCP event.

  Use the VMGEXIT instruction to handle a RDTSCP event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
RdtscpExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  UINT64  Status;

  CcDecodeModRm (Regs, InstructionData);

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_RDTSCP, 0, 0);
  if (Status != 0) {
    return Status;
  }

  if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRcx) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRdx))
  {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Regs->Rax = Ghcb->SaveArea.Rax;
  Regs->Rcx = Ghcb->SaveArea.Rcx;
  Regs->Rdx = Ghcb->SaveArea.Rdx;

  return 0;
}

/**
  Handle a VMMCALL event.

  Use the VMGEXIT instruction to handle a VMMCALL event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
VmmCallExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  UINT64  Status;

  Ghcb->SaveArea.Rax = Regs->Rax;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRax);
  Ghcb->SaveArea.Cpl = (UINT8)(Regs->Cs & 0x3);
  CcExitVmgSetOffsetValid (Ghcb, GhcbCpl);

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_VMMCALL, 0, 0);
  if (Status != 0) {
    return Status;
  }

  if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax)) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Regs->Rax = Ghcb->SaveArea.Rax;

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
  @return                          New exception value to propagate

**/
STATIC
UINT64
MsrExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  MSR_SVSM_CAA_REGISTER  Msr;
  UINT64                 ExitInfo1;
  UINT64                 Status;

  ExitInfo1 = 0;

  //
  // The SVSM CAA MSR is a software implemented MSR and not supported
  // by the hardware, handle it directly.
  //
  if (Regs->Rax == MSR_SVSM_CAA) {
    // Writes to the SVSM CAA MSR are ignored
    if (*(InstructionData->OpCodes + 1) == 0x30) {
      return 0;
    }

    Msr.Uint64 = AmdSvsmSnpGetCaa ();
    Regs->Rax  = Msr.Bits.Lower32Bits;
    Regs->Rdx  = Msr.Bits.Upper32Bits;

    return 0;
  }

  switch (*(InstructionData->OpCodes + 1)) {
    case 0x30: // WRMSR
      ExitInfo1          = 1;
      Ghcb->SaveArea.Rax = Regs->Rax;
      CcExitVmgSetOffsetValid (Ghcb, GhcbRax);
      Ghcb->SaveArea.Rdx = Regs->Rdx;
      CcExitVmgSetOffsetValid (Ghcb, GhcbRdx);
    //
    // fall through
    //
    case 0x32: // RDMSR
      Ghcb->SaveArea.Rcx = Regs->Rcx;
      CcExitVmgSetOffsetValid (Ghcb, GhcbRcx);
      break;
    default:
      return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_MSR, ExitInfo1, 0);
  if (Status != 0) {
    return Status;
  }

  if (ExitInfo1 == 0) {
    if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax) ||
        !CcExitVmgIsOffsetValid (Ghcb, GhcbRdx))
    {
      return UnsupportedExit (Ghcb, Regs, InstructionData);
    }

    Regs->Rax = Ghcb->SaveArea.Rax;
    Regs->Rdx = Ghcb->SaveArea.Rdx;
  }

  return 0;
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
  IN     EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN OUT CC_INSTRUCTION_DATA     *InstructionData
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
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
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

    IoBytes   = IOIO_DATA_BYTES (ExitInfo1);
    GhcbCount = sizeof (Ghcb->SharedBuffer) / IoBytes;

    OpCount = ((ExitInfo1 & IOIO_REP) != 0) ? Regs->Rcx : 1;
    while (OpCount != 0) {
      ExitInfo2    = MIN (OpCount, GhcbCount);
      VmgExitBytes = ExitInfo2 * IoBytes;

      if ((ExitInfo1 & IOIO_TYPE_IN) == 0) {
        CopyMem (Ghcb->SharedBuffer, (VOID *)Regs->Rsi, VmgExitBytes);
        Regs->Rsi += VmgExitBytes;
      }

      Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
      CcExitVmgSetOffsetValid (Ghcb, GhcbSwScratch);
      Status = CcExitVmgExit (Ghcb, SVM_EXIT_IOIO_PROT, ExitInfo1, ExitInfo2);
      if (Status != 0) {
        return Status;
      }

      if ((ExitInfo1 & IOIO_TYPE_IN) != 0) {
        CopyMem ((VOID *)Regs->Rdi, Ghcb->SharedBuffer, VmgExitBytes);
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

    CcExitVmgSetOffsetValid (Ghcb, GhcbRax);

    Status = CcExitVmgExit (Ghcb, SVM_EXIT_IOIO_PROT, ExitInfo1, 0);
    if (Status != 0) {
      return Status;
    }

    if ((ExitInfo1 & IOIO_TYPE_IN) != 0) {
      if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax)) {
        return UnsupportedExit (Ghcb, Regs, InstructionData);
      }

      CopyMem (&Regs->Rax, &Ghcb->SaveArea.Rax, IOIO_DATA_BYTES (ExitInfo1));
    }
  }

  return 0;
}

/**
  Handle a INVD event.

  Use the VMGEXIT instruction to handle a INVD event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
InvdExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  return CcExitVmgExit (Ghcb, SVM_EXIT_INVD, 0, 0);
}

/**
  Fetch CPUID leaf/function via hypervisor/VMGEXIT.

  @param[in, out] Ghcb         Pointer to the Guest-Hypervisor Communication
                               Block
  @param[in]      EaxIn        EAX input for cpuid instruction
  @param[in]      EcxIn        ECX input for cpuid instruction
  @param[in]      Xcr0In       XCR0 at time of cpuid instruction
  @param[in, out] Eax          Pointer to store leaf's EAX value
  @param[in, out] Ebx          Pointer to store leaf's EBX value
  @param[in, out] Ecx          Pointer to store leaf's ECX value
  @param[in, out] Edx          Pointer to store leaf's EDX value
  @param[in, out] Status       Pointer to store status from VMGEXIT (always 0
                               unless return value indicates failure)
  @param[in, out] Unsupported  Pointer to store indication of unsupported
                               VMGEXIT (always false unless return value
                               indicates failure)

  @retval TRUE                 CPUID leaf fetch successfully.
  @retval FALSE                Error occurred while fetching CPUID leaf. Callers
                               should Status and Unsupported and handle
                               accordingly if they indicate a more precise
                               error condition.

**/
STATIC
BOOLEAN
GetCpuidHyp (
  IN OUT GHCB     *Ghcb,
  IN     UINT32   EaxIn,
  IN     UINT32   EcxIn,
  IN     UINT64   XCr0,
  IN OUT UINT32   *Eax,
  IN OUT UINT32   *Ebx,
  IN OUT UINT32   *Ecx,
  IN OUT UINT32   *Edx,
  IN OUT UINT64   *Status,
  IN OUT BOOLEAN  *UnsupportedExit
  )
{
  *UnsupportedExit   = FALSE;
  Ghcb->SaveArea.Rax = EaxIn;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRax);
  Ghcb->SaveArea.Rcx = EcxIn;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRcx);
  if (EaxIn == CPUID_EXTENDED_STATE) {
    Ghcb->SaveArea.XCr0 = XCr0;
    CcExitVmgSetOffsetValid (Ghcb, GhcbXCr0);
  }

  *Status = CcExitVmgExit (Ghcb, SVM_EXIT_CPUID, 0, 0);
  if (*Status != 0) {
    return FALSE;
  }

  if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRbx) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRcx) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRdx))
  {
    *UnsupportedExit = TRUE;
    return FALSE;
  }

  if (Eax) {
    *Eax = (UINT32)(UINTN)Ghcb->SaveArea.Rax;
  }

  if (Ebx) {
    *Ebx = (UINT32)(UINTN)Ghcb->SaveArea.Rbx;
  }

  if (Ecx) {
    *Ecx = (UINT32)(UINTN)Ghcb->SaveArea.Rcx;
  }

  if (Edx) {
    *Edx = (UINT32)(UINTN)Ghcb->SaveArea.Rdx;
  }

  return TRUE;
}

/**
  Check if SEV-SNP enabled.

  @retval TRUE      SEV-SNP is enabled.
  @retval FALSE     SEV-SNP is disabled.

**/
STATIC
BOOLEAN
SnpEnabled (
  VOID
  )
{
  MSR_SEV_STATUS_REGISTER  Msr;

  Msr.Uint32 = AsmReadMsr32 (MSR_SEV_STATUS);

  return !!Msr.Bits.SevSnpBit;
}

/**
  Calculate the total XSAVE area size for enabled XSAVE areas

  @param[in]      XFeaturesEnabled  Bit-mask of enabled XSAVE features/areas as
                                    indicated by XCR0/MSR_IA32_XSS bits
  @param[in, out] XSaveSize         Pointer to storage for calculated XSAVE area
                                    size
  @param[in]      Compacted         Whether or not the calculation is for the
                                    normal XSAVE area size (leaf 0xD,0x0,EBX) or
                                    compacted XSAVE area size (leaf 0xD,0x1,EBX)


  @retval TRUE                      XSAVE size calculation was successful.
  @retval FALSE                     XSAVE size calculation was unsuccessful.
**/
STATIC
BOOLEAN
GetCpuidXSaveSize (
  IN     UINT64   XFeaturesEnabled,
  IN OUT UINT32   *XSaveSize,
  IN     BOOLEAN  Compacted
  )
{
  SEV_SNP_CPUID_INFO  *CpuidInfo;
  UINT64              XFeaturesFound = 0;
  UINT32              Idx;

  //
  // The base/legacy XSave size is documented to be 0x240 in the APM.
  //
  *XSaveSize = 0x240;
  CpuidInfo  = (SEV_SNP_CPUID_INFO *)(UINT64)PcdGet32 (PcdOvmfCpuidBase);

  for (Idx = 0; Idx < CpuidInfo->Count; Idx++) {
    SEV_SNP_CPUID_FUNCTION  *CpuidFn = &CpuidInfo->function[Idx];

    if (!((CpuidFn->EaxIn == 0xD) && (CpuidFn->EcxIn > 1))) {
      continue;
    }

    if (XFeaturesFound & (1ULL << CpuidFn->EcxIn) ||
        !(XFeaturesEnabled & (1ULL << CpuidFn->EcxIn)))
    {
      continue;
    }

    XFeaturesFound |= (1ULL << CpuidFn->EcxIn);
    if (Compacted) {
      *XSaveSize += CpuidFn->Eax;
    } else {
      *XSaveSize = MAX (*XSaveSize, CpuidFn->Eax + CpuidFn->Ebx);
    }
  }

  /*
   * Either the guest set unsupported XCR0/XSS bits, or the corresponding
   * entries in the CPUID table were not present. This is an invalid state.
   */
  if (XFeaturesFound != (XFeaturesEnabled & ~3UL)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check if a CPUID leaf/function is indexed via ECX sub-leaf/sub-function

  @param[in]      EaxIn        EAX input for cpuid instruction

  @retval FALSE                cpuid leaf/function is not indexed by ECX input
  @retval TRUE                 cpuid leaf/function is indexed by ECX input

**/
STATIC
BOOLEAN
IsFunctionIndexed (
  IN     UINT32  EaxIn
  )
{
  switch (EaxIn) {
    case CPUID_CACHE_PARAMS:
    case CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS:
    case CPUID_EXTENDED_TOPOLOGY:
    case CPUID_EXTENDED_STATE:
    case CPUID_INTEL_RDT_MONITORING:
    case CPUID_INTEL_RDT_ALLOCATION:
    case CPUID_INTEL_SGX:
    case CPUID_INTEL_PROCESSOR_TRACE:
    case CPUID_DETERMINISTIC_ADDRESS_TRANSLATION_PARAMETERS:
    case CPUID_V2_EXTENDED_TOPOLOGY:
    case 0x8000001D: /* Cache Topology Information */
      return TRUE;
  }

  return FALSE;
}

/**
  Fetch CPUID leaf/function via SEV-SNP CPUID table.

  @param[in, out] Ghcb         Pointer to the Guest-Hypervisor Communication
                               Block
  @param[in]      EaxIn        EAX input for cpuid instruction
  @param[in]      EcxIn        ECX input for cpuid instruction
  @param[in]      Xcr0In       XCR0 at time of cpuid instruction
  @param[in, out] Eax          Pointer to store leaf's EAX value
  @param[in, out] Ebx          Pointer to store leaf's EBX value
  @param[in, out] Ecx          Pointer to store leaf's ECX value
  @param[in, out] Edx          Pointer to store leaf's EDX value
  @param[in, out] Status       Pointer to store status from VMGEXIT (always 0
                               unless return value indicates failure)
  @param[in, out] Unsupported  Pointer to store indication of unsupported
                               VMGEXIT (always false unless return value
                               indicates failure)

  @retval TRUE                 CPUID leaf fetch successfully.
  @retval FALSE                Error occurred while fetching CPUID leaf. Callers
                               should Status and Unsupported and handle
                               accordingly if they indicate a more precise
                               error condition.

**/
STATIC
BOOLEAN
GetCpuidFw (
  IN OUT GHCB     *Ghcb,
  IN     UINT32   EaxIn,
  IN     UINT32   EcxIn,
  IN     UINT64   XCr0,
  IN OUT UINT32   *Eax,
  IN OUT UINT32   *Ebx,
  IN OUT UINT32   *Ecx,
  IN OUT UINT32   *Edx,
  IN OUT UINT64   *Status,
  IN OUT BOOLEAN  *Unsupported
  )
{
  SEV_SNP_CPUID_INFO  *CpuidInfo;
  BOOLEAN             Found;
  UINT32              Idx;

  CpuidInfo = (SEV_SNP_CPUID_INFO *)(UINT64)PcdGet32 (PcdOvmfCpuidBase);
  Found     = FALSE;

  for (Idx = 0; Idx < CpuidInfo->Count; Idx++) {
    SEV_SNP_CPUID_FUNCTION  *CpuidFn = &CpuidInfo->function[Idx];

    if (CpuidFn->EaxIn != EaxIn) {
      continue;
    }

    if (IsFunctionIndexed (CpuidFn->EaxIn) && (CpuidFn->EcxIn != EcxIn)) {
      continue;
    }

    *Eax = CpuidFn->Eax;
    *Ebx = CpuidFn->Ebx;
    *Ecx = CpuidFn->Ecx;
    *Edx = CpuidFn->Edx;

    Found = TRUE;
    break;
  }

  if (!Found) {
    *Eax = *Ebx = *Ecx = *Edx = 0;
    goto Out;
  }

  if (EaxIn == CPUID_VERSION_INFO) {
    IA32_CR4  Cr4;
    UINT32    Ebx2;
    UINT32    Edx2;

    if (!GetCpuidHyp (
           Ghcb,
           EaxIn,
           EcxIn,
           XCr0,
           NULL,
           &Ebx2,
           NULL,
           &Edx2,
           Status,
           Unsupported
           ))
    {
      return FALSE;
    }

    /* initial APIC ID */
    *Ebx = (*Ebx & 0x00FFFFFF) | (Ebx2 & 0xFF000000);
    /* APIC enabled bit */
    *Edx = (*Edx & ~BIT9) | (Edx2 & BIT9);
    /* OSXSAVE enabled bit */
    Cr4.UintN = AsmReadCr4 ();
    *Ecx      = (Cr4.Bits.OSXSAVE) ? (*Ecx & ~BIT27) | (*Ecx & BIT27)
                              : (*Ecx & ~BIT27);
  } else if (EaxIn == CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS) {
    IA32_CR4  Cr4;

    Cr4.UintN = AsmReadCr4 ();
    /* OSPKE enabled bit */
    *Ecx = (Cr4.Bits.PKE) ? (*Ecx | BIT4) : (*Ecx & ~BIT4);
  } else if (EaxIn == CPUID_EXTENDED_TOPOLOGY) {
    if (!GetCpuidHyp (
           Ghcb,
           EaxIn,
           EcxIn,
           XCr0,
           NULL,
           NULL,
           NULL,
           Edx,
           Status,
           Unsupported
           ))
    {
      return FALSE;
    }
  } else if ((EaxIn == CPUID_EXTENDED_STATE) && ((EcxIn == 0) || (EcxIn == 1))) {
    MSR_IA32_XSS_REGISTER  XssMsr;
    BOOLEAN                Compacted;
    UINT32                 XSaveSize;

    XssMsr.Uint64 = 0;
    Compacted     = FALSE;
    if (EcxIn == 1) {
      /*
       * The PPR and APM aren't clear on what size should be encoded in
       * 0xD:0x1:EBX when compaction is not enabled by either XSAVEC or
       * XSAVES, as these are generally fixed to 1 on real CPUs. Report
       * this undefined case as an error.
       */
      if (!(*Eax & (BIT3 | BIT1))) {
        /* (XSAVES | XSAVEC) */
        return FALSE;
      }

      Compacted     = TRUE;
      XssMsr.Uint64 = AsmReadMsr64 (MSR_IA32_XSS);
    }

    if (!GetCpuidXSaveSize (
           XCr0 | XssMsr.Uint64,
           &XSaveSize,
           Compacted
           ))
    {
      return FALSE;
    }

    *Ebx = XSaveSize;
  } else if (EaxIn == 0x8000001E) {
    UINT32  Ebx2;
    UINT32  Ecx2;

    /* extended APIC ID */
    if (!GetCpuidHyp (
           Ghcb,
           EaxIn,
           EcxIn,
           XCr0,
           Eax,
           &Ebx2,
           &Ecx2,
           NULL,
           Status,
           Unsupported
           ))
    {
      return FALSE;
    }

    /* compute ID */
    *Ebx = (*Ebx & 0xFFFFFF00) | (Ebx2 & 0x000000FF);
    /* node ID */
    *Ecx = (*Ecx & 0xFFFFFF00) | (Ecx2 & 0x000000FF);
  } else if (EaxIn == 0x8000001F) {
    /* Set the SVSM feature bit if running under an SVSM */
    if (AmdSvsmIsSvsmPresent ()) {
      *Eax |= BIT28;
    }
  }

Out:
  *Status      = 0;
  *Unsupported = FALSE;
  return TRUE;
}

/**
  Handle a CPUID event.

  Use VMGEXIT instruction or CPUID table to handle a CPUID event.

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
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  BOOLEAN  Unsupported;
  UINT64   Status;
  UINT32   EaxIn;
  UINT32   EcxIn;
  UINT64   XCr0;
  UINT32   Eax;
  UINT32   Ebx;
  UINT32   Ecx;
  UINT32   Edx;

  EaxIn = (UINT32)(UINTN)Regs->Rax;
  EcxIn = (UINT32)(UINTN)Regs->Rcx;

  if (EaxIn == CPUID_EXTENDED_STATE) {
    IA32_CR4  Cr4;

    Cr4.UintN           = AsmReadCr4 ();
    Ghcb->SaveArea.XCr0 = (Cr4.Bits.OSXSAVE == 1) ? AsmXGetBv (0) : 1;
    XCr0                = (Cr4.Bits.OSXSAVE == 1) ? AsmXGetBv (0) : 1;
  }

  if (SnpEnabled ()) {
    if (!GetCpuidFw (
           Ghcb,
           EaxIn,
           EcxIn,
           XCr0,
           &Eax,
           &Ebx,
           &Ecx,
           &Edx,
           &Status,
           &Unsupported
           ))
    {
      goto CpuidFail;
    }
  } else {
    if (!GetCpuidHyp (
           Ghcb,
           EaxIn,
           EcxIn,
           XCr0,
           &Eax,
           &Ebx,
           &Ecx,
           &Edx,
           &Status,
           &Unsupported
           ))
    {
      goto CpuidFail;
    }
  }

  Regs->Rax = Eax;
  Regs->Rbx = Ebx;
  Regs->Rcx = Ecx;
  Regs->Rdx = Edx;

  return 0;

CpuidFail:
  if (Unsupported) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  return Status;
}

/**
  Handle a RDPMC event.

  Use the VMGEXIT instruction to handle a RDPMC event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
RdpmcExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  UINT64  Status;

  Ghcb->SaveArea.Rcx = Regs->Rcx;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRcx);

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_RDPMC, 0, 0);
  if (Status != 0) {
    return Status;
  }

  if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRdx))
  {
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
  @return                          New exception value to propagate

**/
STATIC
UINT64
RdtscExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  UINT64  Status;

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_RDTSC, 0, 0);
  if (Status != 0) {
    return Status;
  }

  if (!CcExitVmgIsOffsetValid (Ghcb, GhcbRax) ||
      !CcExitVmgIsOffsetValid (Ghcb, GhcbRdx))
  {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Regs->Rax = Ghcb->SaveArea.Rax;
  Regs->Rdx = Ghcb->SaveArea.Rdx;

  return 0;
}

/**
  Handle a DR7 register write event.

  Use the VMGEXIT instruction to handle a DR7 write event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully
  @return                          New exception value to propagate

**/
STATIC
UINT64
Dr7WriteExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  CC_INSTRUCTION_OPCODE_EXT  *Ext;
  SEV_ES_PER_CPU_DATA        *SevEsData;
  UINT64                     *Register;
  UINT64                     Status;

  if (MemEncryptSevEsDebugVirtualizationIsEnabled ()) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Ext       = &InstructionData->Ext;
  SevEsData = (SEV_ES_PER_CPU_DATA *)(Ghcb + 1);

  //
  // MOV DRn always treats MOD == 3 no matter how encoded
  //
  Register = CcGetRegisterPointer (Regs, Ext->ModRm.Rm);

  //
  // Using a value of 0 for ExitInfo1 means RAX holds the value
  //
  Ghcb->SaveArea.Rax = *Register;
  CcExitVmgSetOffsetValid (Ghcb, GhcbRax);

  Status = CcExitVmgExit (Ghcb, SVM_EXIT_DR7_WRITE, 0, 0);
  if (Status != 0) {
    return Status;
  }

  SevEsData->Dr7       = *Register;
  SevEsData->Dr7Cached = 1;

  return 0;
}

/**
  Handle a DR7 register read event.

  Use the VMGEXIT instruction to handle a DR7 read event.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in]      InstructionData  Instruction parsing context

  @retval 0                        Event handled successfully

**/
STATIC
UINT64
Dr7ReadExit (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN     CC_INSTRUCTION_DATA     *InstructionData
  )
{
  CC_INSTRUCTION_OPCODE_EXT  *Ext;
  SEV_ES_PER_CPU_DATA        *SevEsData;
  UINT64                     *Register;

  if (MemEncryptSevEsDebugVirtualizationIsEnabled ()) {
    return UnsupportedExit (Ghcb, Regs, InstructionData);
  }

  Ext       = &InstructionData->Ext;
  SevEsData = (SEV_ES_PER_CPU_DATA *)(Ghcb + 1);

  //
  // MOV DRn always treats MOD == 3 no matter how encoded
  //
  Register = CcGetRegisterPointer (Regs, Ext->ModRm.Rm);

  //
  // If there is a cached valued for DR7, return that. Otherwise return the
  // DR7 standard reset value of 0x400 (no debug breakpoints set).
  //
  *Register = (SevEsData->Dr7Cached == 1) ? SevEsData->Dr7 : 0x400;

  return 0;
}

/**
  Check that the opcode matches the exit code for a #VC.

  Each exit code should only be raised while executing certain instructions.
  Verify that rIP points to a correct instruction based on the exit code to
  protect against maliciously injected interrupts via the hypervisor. If it does
  not, report an unsupported event to the hypervisor.

  Decodes the ModRm byte into InstructionData if necessary.

  @param[in, out] Ghcb             Pointer to the Guest-Hypervisor Communication
                                   Block
  @param[in, out] Regs             x64 processor context
  @param[in, out] InstructionData  Instruction parsing context
  @param[in]      ExitCode         Exit code given by #VC.

  @retval 0                        No problems detected.
  @return                          New exception value to propagate


**/
STATIC
UINT64
VcCheckOpcodeBytes (
  IN OUT GHCB                    *Ghcb,
  IN OUT EFI_SYSTEM_CONTEXT_X64  *Regs,
  IN OUT CC_INSTRUCTION_DATA     *InstructionData,
  IN     UINT64                  ExitCode
  )
{
  UINT8  OpCode;

  //
  // Expected opcodes are either 1 or 2 bytes. If they are 2 bytes, they always
  // start with TWO_BYTE_OPCODE_ESCAPE (0x0f), so skip over that.
  //
  OpCode = *(InstructionData->OpCodes);
  if (OpCode == TWO_BYTE_OPCODE_ESCAPE) {
    OpCode = *(InstructionData->OpCodes + 1);
  }

  switch (ExitCode) {
    case SVM_EXIT_IOIO_PROT:
    case SVM_EXIT_NPF:
      /* handled separately */
      return 0;

    case SVM_EXIT_CPUID:
      if (OpCode == 0xa2) {
        return 0;
      }

      break;

    case SVM_EXIT_INVD:
      if (OpCode == 0x08) {
        return 0;
      }

      break;

    case SVM_EXIT_MONITOR:
      CcDecodeModRm (Regs, InstructionData);

      if ((OpCode == 0x01) &&
          (  (InstructionData->ModRm.Uint8 == 0xc8)   /* MONITOR */
          || (InstructionData->ModRm.Uint8 == 0xfa))) /* MONITORX */
      {
        return 0;
      }

      break;

    case SVM_EXIT_MWAIT:
      CcDecodeModRm (Regs, InstructionData);

      if ((OpCode == 0x01) &&
          (  (InstructionData->ModRm.Uint8 == 0xc9)   /* MWAIT */
          || (InstructionData->ModRm.Uint8 == 0xfb))) /* MWAITX */
      {
        return 0;
      }

      break;

    case SVM_EXIT_MSR:
      /* RDMSR */
      if ((OpCode == 0x32) ||
          /* WRMSR */
          (OpCode == 0x30))
      {
        return 0;
      }

      break;

    case SVM_EXIT_RDPMC:
      if (OpCode == 0x33) {
        return 0;
      }

      break;

    case SVM_EXIT_RDTSC:
      if (OpCode == 0x31) {
        return 0;
      }

      break;

    case SVM_EXIT_RDTSCP:
      CcDecodeModRm (Regs, InstructionData);

      if ((OpCode == 0x01) && (InstructionData->ModRm.Uint8 == 0xf9)) {
        return 0;
      }

      break;

    case SVM_EXIT_DR7_READ:
      CcDecodeModRm (Regs, InstructionData);

      if ((OpCode == 0x21) &&
          (InstructionData->Ext.ModRm.Reg == 7))
      {
        return 0;
      }

      break;

    case SVM_EXIT_VMMCALL:
      CcDecodeModRm (Regs, InstructionData);

      if ((OpCode == 0x01) && (InstructionData->ModRm.Uint8 == 0xd9)) {
        return 0;
      }

      break;

    case SVM_EXIT_DR7_WRITE:
      CcDecodeModRm (Regs, InstructionData);

      if ((OpCode == 0x23) &&
          (InstructionData->Ext.ModRm.Reg == 7))
      {
        return 0;
      }

      break;

    case SVM_EXIT_WBINVD:
      if (OpCode == 0x9) {
        return 0;
      }

      break;

    default:
      break;
  }

  return UnsupportedExit (Ghcb, Regs, InstructionData);
}

/**
  Handle a #VC exception.

  Performs the necessary processing to handle a #VC exception.

  @param[in, out]  Ghcb           Pointer to the GHCB
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
InternalVmgExitHandleVc (
  IN OUT GHCB                *Ghcb,
  IN OUT EFI_EXCEPTION_TYPE  *ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_SYSTEM_CONTEXT_X64  *Regs;
  NAE_EXIT                NaeExit;
  CC_INSTRUCTION_DATA     InstructionData;
  UINT64                  ExitCode, Status;
  EFI_STATUS              VcRet;
  BOOLEAN                 InterruptState;

  VcRet = EFI_SUCCESS;

  Regs = SystemContext.SystemContextX64;

  CcExitVmgInit (Ghcb, &InterruptState);

  ExitCode = Regs->ExceptionData;
  switch (ExitCode) {
    case SVM_EXIT_DR7_READ:
      NaeExit = Dr7ReadExit;
      break;

    case SVM_EXIT_DR7_WRITE:
      NaeExit = Dr7WriteExit;
      break;

    case SVM_EXIT_RDTSC:
      NaeExit = RdtscExit;
      break;

    case SVM_EXIT_RDPMC:
      NaeExit = RdpmcExit;
      break;

    case SVM_EXIT_CPUID:
      NaeExit = CpuidExit;
      break;

    case SVM_EXIT_INVD:
      NaeExit = InvdExit;
      break;

    case SVM_EXIT_IOIO_PROT:
      NaeExit = IoioExit;
      break;

    case SVM_EXIT_MSR:
      NaeExit = MsrExit;
      break;

    case SVM_EXIT_VMMCALL:
      NaeExit = VmmCallExit;
      break;

    case SVM_EXIT_RDTSCP:
      NaeExit = RdtscpExit;
      break;

    case SVM_EXIT_WBINVD:
      NaeExit = WbinvdExit;
      break;

    case SVM_EXIT_MONITOR:
      NaeExit = MonitorExit;
      break;

    case SVM_EXIT_MWAIT:
      NaeExit = MwaitExit;
      break;

    case SVM_EXIT_NPF:
      NaeExit = MmioExit;
      break;

    default:
      NaeExit = UnsupportedExit;
  }

  CcInitInstructionData (&InstructionData, Ghcb, Regs);

  Status = VcCheckOpcodeBytes (Ghcb, Regs, &InstructionData, ExitCode);

  //
  // If the opcode does not match the exit code, do not process the exception
  //
  if (Status == 0) {
    Status = NaeExit (Ghcb, Regs, &InstructionData);
  }

  if (Status == 0) {
    Regs->Rip += CcInstructionLength (&InstructionData);
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

  CcExitVmgDone (Ghcb, InterruptState);

  return VcRet;
}

/**
  Routine to allow ASSERT from within #VC.

  @param[in, out]  SevEsData  Pointer to the per-CPU data

**/
VOID
EFIAPI
VmgExitIssueAssert (
  IN OUT SEV_ES_PER_CPU_DATA  *SevEsData
  )
{
  //
  // Progress will be halted, so set VcCount to allow for ASSERT output
  // to be seen.
  //
  SevEsData->VcCount = 0;

  ASSERT (FALSE);
  CpuDeadLoop ();
}
