/** @file
  x64 CPU Exception Hanlder.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuExceptionCommon.h"

/**
  Internal function to setup CPU exception handlers.

**/
VOID
InternalSetupCpuExceptionHandlers (
  VOID
  )
{
  IA32_DESCRIPTOR                     IdtDescriptor;
  UINTN                               IdtSize;
  EXCEPTION_HANDLER_TEMPLATE_MAP      TemplateMap;
  UINT16                              CodeSegment;
  IA32_IDT_GATE_DESCRIPTOR            *IdtEntry;
  UINTN                               Index;
  UINTN                               InterruptHandler;

  //
  // Read IDT descriptor and calculate IDT size
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtSize = (IdtDescriptor.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR);
  if (IdtSize > CPU_EXCEPTION_NUM) {
    //
    // CPU exeption library only setup CPU_EXCEPTION_NUM exception handler at most
    //
    IdtSize = CPU_EXCEPTION_NUM;
  }

  //
  // Use current CS as the segment selector of interrupt gate in IDT
  //
  CodeSegment = AsmReadCs ();
  IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *) IdtDescriptor.Base;

  GetTemplateAddressMap (&TemplateMap);

  for (Index = 0; Index < IdtSize; Index ++) {
    InterruptHandler = TemplateMap.ExceptionStart + Index * TemplateMap.ExceptionStubHeaderSize;
    IdtEntry[Index].Bits.OffsetLow       = (UINT16)(UINTN)InterruptHandler;
    IdtEntry[Index].Bits.OffsetHigh      = (UINT16)((UINTN)InterruptHandler >> 16);
    IdtEntry[Index].Bits.OffsetUpper     = (UINT32)((UINTN)InterruptHandler >> 32);
    IdtEntry[Index].Bits.Selector        = CodeSegment;
    IdtEntry[Index].Bits.GateType        = IA32_IDT_GATE_TYPE_INTERRUPT_32;
  }
}

/**
  Common exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
DumpCpuContent (
  IN UINTN                ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINTN                   ImageBase;
  UINTN                   EntryPoint;

  InternalPrintMessage (
    "!!!! X64 Exception Type - %016lx     CPU Apic ID - %08x !!!!\n",
    ExceptionType,
    GetApicId ()
    );
  InternalPrintMessage (
    "RIP  - %016lx, CS  - %016lx, RFLAGS - %016lx\n",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Rflags
    );
  if (mErrorCodeFlag & (1 << ExceptionType)) {
    InternalPrintMessage (
      "ExceptionData - %016lx\n",
      SystemContext.SystemContextX64->ExceptionData
      );
  }
  InternalPrintMessage (
    "RAX  - %016lx, RCX - %016lx, RDX - %016lx\n",
    SystemContext.SystemContextX64->Rax,
    SystemContext.SystemContextX64->Rcx,
    SystemContext.SystemContextX64->Rdx
    );
  InternalPrintMessage (
    "RBX  - %016lx, RSP - %016lx, RBP - %016lx\n",
    SystemContext.SystemContextX64->Rbx,
    SystemContext.SystemContextX64->Rsp,
    SystemContext.SystemContextX64->Rbp
    );
  InternalPrintMessage (
    "RSI  - %016lx, RDI - %016lx\n",
    SystemContext.SystemContextX64->Rsi,
    SystemContext.SystemContextX64->Rdi
    );
  InternalPrintMessage (
    "R8   - %016lx, R9  - %016lx, R10 - %016lx\n",
    SystemContext.SystemContextX64->R8,
    SystemContext.SystemContextX64->R9,
    SystemContext.SystemContextX64->R10
    );
  InternalPrintMessage (
    "R11  - %016lx, R12 - %016lx, R13 - %016lx\n",
    SystemContext.SystemContextX64->R11,
    SystemContext.SystemContextX64->R12,
    SystemContext.SystemContextX64->R13
    );
  InternalPrintMessage (
    "R14  - %016lx, R15 - %016lx\n",
    SystemContext.SystemContextX64->R14,
    SystemContext.SystemContextX64->R15
    );
  InternalPrintMessage (
    "DS   - %016lx, ES  - %016lx, FS  - %016lx\n",
    SystemContext.SystemContextX64->Ds,
    SystemContext.SystemContextX64->Es,
    SystemContext.SystemContextX64->Fs
    );
  InternalPrintMessage (
    "GS   - %016lx, SS  - %016lx\n",
    SystemContext.SystemContextX64->Gs,
    SystemContext.SystemContextX64->Ss
    );
  InternalPrintMessage (
    "CR0  - %016lx, CR2 - %016lx, CR3 - %016lx\n",
    SystemContext.SystemContextX64->Cr0,
    SystemContext.SystemContextX64->Cr2,
    SystemContext.SystemContextX64->Cr3
    );
  InternalPrintMessage (
    "CR4  - %016lx, CR8 - %016lx\n",
    SystemContext.SystemContextX64->Cr4,
    SystemContext.SystemContextX64->Cr8
    );
  InternalPrintMessage (
    "DR0  - %016lx, DR1 - %016lx, DR2 - %016lx\n",
    SystemContext.SystemContextX64->Dr0,
    SystemContext.SystemContextX64->Dr1,
    SystemContext.SystemContextX64->Dr2
    );
  InternalPrintMessage (
    "DR3  - %016lx, DR6 - %016lx, DR7 - %016lx\n",
    SystemContext.SystemContextX64->Dr3,
    SystemContext.SystemContextX64->Dr6,
    SystemContext.SystemContextX64->Dr7
    );
  InternalPrintMessage (
    "GDTR - %016lx %016lx, LDTR - %016lx\n",
    SystemContext.SystemContextX64->Gdtr[0],
    SystemContext.SystemContextX64->Gdtr[1],
    SystemContext.SystemContextX64->Ldtr
    );
  InternalPrintMessage (
    "IDTR - %016lx %016lx,   TR - %016lx\n",
    SystemContext.SystemContextX64->Idtr[0],
    SystemContext.SystemContextX64->Idtr[1],
    SystemContext.SystemContextX64->Tr
    );
	InternalPrintMessage (
    "FXSAVE_STATE - %016lx\n",
    &SystemContext.SystemContextX64->FxSaveState
    );

  //
  // Find module image base and module entry point by RIP
  //
  ImageBase = FindModuleImageBase (SystemContext.SystemContextX64->Rip, &EntryPoint);
  if (ImageBase != 0) {
    InternalPrintMessage (
      " (ImageBase=%016lx, EntryPoint=%016lx) !!!!\n",
      ImageBase,
      EntryPoint
      );
  }
}
