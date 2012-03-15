/** @file
  IA32 CPU Exception Hanlder functons.

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
  UINTN                               InterruptHandler;;

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
    IdtEntry[Index].Bits.OffsetLow  = (UINT16)(UINTN)InterruptHandler;
    IdtEntry[Index].Bits.OffsetHigh = (UINT16)((UINTN)InterruptHandler >> 16);
    IdtEntry[Index].Bits.Selector   = CodeSegment;
    IdtEntry[Index].Bits.GateType   = IA32_IDT_GATE_TYPE_INTERRUPT_32;
  }
}

/**
  Dump CPU content information.

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
    "!!!! IA32 Exception Type - %08x    CPU Apic ID - %08x !!!!\n",
    ExceptionType,
    GetApicId ()
    );
  InternalPrintMessage (
    "EIP  - %08x, CS  - %08x, EFLAGS - %08x\n",
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eflags
    );
  if ((mErrorCodeFlag & (1 << ExceptionType)) != 0) {
    InternalPrintMessage (
      "ExceptionData - %08x\n",
      SystemContext.SystemContextIa32->ExceptionData
      );
  }
  InternalPrintMessage (
    "EAX  - %08x, ECX - %08x, EDX - %08x, EBX - %08x\n",
    SystemContext.SystemContextIa32->Eax,
    SystemContext.SystemContextIa32->Ecx,
    SystemContext.SystemContextIa32->Edx,
    SystemContext.SystemContextIa32->Ebx
    );
  InternalPrintMessage (
    "ESP  - %08x, EBP - %08x, ESI - %08x, EDI - %08x\n",
    SystemContext.SystemContextIa32->Esp,
    SystemContext.SystemContextIa32->Ebp,
    SystemContext.SystemContextIa32->Esi,
    SystemContext.SystemContextIa32->Edi
    );
  InternalPrintMessage (
    "DS   - %08x, ES  - %08x, FS  - %08x, GS  - %08x, SS - %08x\n",
    SystemContext.SystemContextIa32->Ds,
    SystemContext.SystemContextIa32->Es,
    SystemContext.SystemContextIa32->Fs,
    SystemContext.SystemContextIa32->Gs,
    SystemContext.SystemContextIa32->Ss
    );
  InternalPrintMessage (
    "CR0  - %08x, CR2 - %08x, CR3 - %08x, CR4 - %08x\n",
    SystemContext.SystemContextIa32->Cr0,
    SystemContext.SystemContextIa32->Cr2,
    SystemContext.SystemContextIa32->Cr3,
    SystemContext.SystemContextIa32->Cr4
    );
  InternalPrintMessage (
    "DR0  - %08x, DR1 - %08x, DR2 - %08x, DR3 - %08x\n",
    SystemContext.SystemContextIa32->Dr0,
    SystemContext.SystemContextIa32->Dr1,
    SystemContext.SystemContextIa32->Dr2,
    SystemContext.SystemContextIa32->Dr3
    );
  InternalPrintMessage (
    "DR6  - %08x, DR7 - %08x\n",
    SystemContext.SystemContextIa32->Dr6,
    SystemContext.SystemContextIa32->Dr7
    );
  InternalPrintMessage (
    "GDTR - %08x %08x, IDTR - %08x %08x\n",
    SystemContext.SystemContextIa32->Gdtr[0],
    SystemContext.SystemContextIa32->Gdtr[1],
    SystemContext.SystemContextIa32->Idtr[0],
    SystemContext.SystemContextIa32->Idtr[1]
    );
  InternalPrintMessage (
    "LDTR - %08x, TR - %08x\n",
    SystemContext.SystemContextIa32->Ldtr,
    SystemContext.SystemContextIa32->Tr
    );
  InternalPrintMessage (
    "FXSAVE_STATE - %08x\n",
    &SystemContext.SystemContextIa32->FxSaveState
    );

  //
  // Find module image base and module entry point by RIP
  //
  ImageBase = FindModuleImageBase (SystemContext.SystemContextIa32->Eip, &EntryPoint);
  if (ImageBase != 0) {
    InternalPrintMessage (
      " (ImageBase=%08x, EntryPoint=%08x) !!!!\n",
      ImageBase,
      EntryPoint
      );
  }
}
