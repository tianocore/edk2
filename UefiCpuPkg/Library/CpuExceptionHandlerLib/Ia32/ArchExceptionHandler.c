/** @file
  IA32 CPU Exception Handler functons.

  Copyright (c) 2012 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuExceptionCommon.h"

/**
  Return address map of exception handler template so that C code can generate
  exception tables.

  @param IdtEntry          Pointer to IDT entry to be updated.
  @param InterruptHandler  IDT handler value.

**/
VOID
ArchUpdateIdtEntry (
  IN IA32_IDT_GATE_DESCRIPTOR        *IdtEntry,
  IN UINTN                           InterruptHandler
  )
{
  IdtEntry->Bits.OffsetLow   = (UINT16)(UINTN)InterruptHandler;
  IdtEntry->Bits.OffsetHigh  = (UINT16)((UINTN)InterruptHandler >> 16);
  IdtEntry->Bits.GateType    = IA32_IDT_GATE_TYPE_INTERRUPT_32;
}

/**
  Read IDT handler value from IDT entry.

  @param IdtEntry          Pointer to IDT entry to be read.

**/
UINTN
ArchGetIdtHandler (
  IN IA32_IDT_GATE_DESCRIPTOR        *IdtEntry
  )
{
  return (UINTN)IdtEntry->Bits.OffsetLow + (((UINTN)IdtEntry->Bits.OffsetHigh) << 16);
}

/**
  Save CPU exception context when handling EFI_VECTOR_HANDOFF_HOOK_AFTER case.

  @param[in] ExceptionType        Exception type.
  @param[in] SystemContext        Pointer to EFI_SYSTEM_CONTEXT.
  @param[in] ExceptionHandlerData Pointer to exception handler data.
**/
VOID
ArchSaveExceptionContext (
  IN UINTN                        ExceptionType,
  IN EFI_SYSTEM_CONTEXT           SystemContext,
  IN EXCEPTION_HANDLER_DATA       *ExceptionHandlerData
  )
{
  IA32_EFLAGS32           Eflags;
  RESERVED_VECTORS_DATA   *ReservedVectors;

  ReservedVectors = ExceptionHandlerData->ReservedVectors;
  //
  // Save Exception context in global variable
  //
  ReservedVectors[ExceptionType].OldFlags      = SystemContext.SystemContextIa32->Eflags;
  ReservedVectors[ExceptionType].OldCs         = SystemContext.SystemContextIa32->Cs;
  ReservedVectors[ExceptionType].OldIp         = SystemContext.SystemContextIa32->Eip;
  ReservedVectors[ExceptionType].ExceptionData = SystemContext.SystemContextIa32->ExceptionData;
  //
  // Clear IF flag to avoid old IDT handler enable interrupt by IRET
  //
  Eflags.UintN = SystemContext.SystemContextIa32->Eflags;
  Eflags.Bits.IF = 0;
  SystemContext.SystemContextIa32->Eflags = Eflags.UintN;
  //
  // Modify the EIP in stack, then old IDT handler will return to the stub code
  //
  SystemContext.SystemContextIa32->Eip    = (UINTN) ReservedVectors[ExceptionType].HookAfterStubHeaderCode;
}

/**
  Restore CPU exception context when handling EFI_VECTOR_HANDOFF_HOOK_AFTER case.

  @param[in] ExceptionType        Exception type.
  @param[in] SystemContext        Pointer to EFI_SYSTEM_CONTEXT.
  @param[in] ExceptionHandlerData Pointer to exception handler data.
**/
VOID
ArchRestoreExceptionContext (
  IN UINTN                        ExceptionType,
  IN EFI_SYSTEM_CONTEXT           SystemContext,
  IN EXCEPTION_HANDLER_DATA       *ExceptionHandlerData
  )
{
  RESERVED_VECTORS_DATA   *ReservedVectors;

  ReservedVectors = ExceptionHandlerData->ReservedVectors;
  SystemContext.SystemContextIa32->Eflags        = ReservedVectors[ExceptionType].OldFlags;
  SystemContext.SystemContextIa32->Cs            = ReservedVectors[ExceptionType].OldCs;
  SystemContext.SystemContextIa32->Eip           = ReservedVectors[ExceptionType].OldIp;
  SystemContext.SystemContextIa32->ExceptionData = ReservedVectors[ExceptionType].ExceptionData;
}

/**
  Display processor context.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Processor context to be display.
**/
VOID
EFIAPI
DumpCpuContext (
  IN EFI_EXCEPTION_TYPE   ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  InternalPrintMessage (
    "!!!! IA32 Exception Type - %02x(%a)  CPU Apic ID - %08x !!!!\n",
    ExceptionType,
    GetExceptionNameStr (ExceptionType),
    GetApicId ()
    );
  if ((mErrorCodeFlag & (1 << ExceptionType)) != 0) {
    InternalPrintMessage (
      "ExceptionData - %08x",
      SystemContext.SystemContextIa32->ExceptionData
      );
    if (ExceptionType == EXCEPT_IA32_PAGE_FAULT) {
      InternalPrintMessage (
        "  I:%x R:%x U:%x W:%x P:%x PK:%x S:%x",
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_ID)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_RSVD) != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_US)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_WR)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_P)    != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_PK)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_SGX)  != 0
        );
    }
    InternalPrintMessage ("\n");
  }
  InternalPrintMessage (
    "EIP  - %08x, CS  - %08x, EFLAGS - %08x\n",
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eflags
    );
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
}

/**
  Display CPU information.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
DumpImageAndCpuContent (
  IN EFI_EXCEPTION_TYPE   ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  DumpCpuContext (ExceptionType, SystemContext);
  //
  // Dump module image base and module entry point by EIP
  //
  DumpModuleImageInfo (SystemContext.SystemContextIa32->Eip);
}
