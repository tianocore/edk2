/** @file
  IA32 CPU Exception Handler functons.

  Copyright (c) 2012 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  OUT IA32_IDT_GATE_DESCRIPTOR  *IdtEntry,
  IN  UINTN                     InterruptHandler
  )
{
  IdtEntry->Bits.OffsetLow  = (UINT16)(UINTN)InterruptHandler;
  IdtEntry->Bits.OffsetHigh = (UINT16)((UINTN)InterruptHandler >> 16);
  IdtEntry->Bits.GateType   = IA32_IDT_GATE_TYPE_INTERRUPT_32;
}

/**
  Read IDT handler value from IDT entry.

  @param IdtEntry          Pointer to IDT entry to be read.

**/
UINTN
ArchGetIdtHandler (
  IN IA32_IDT_GATE_DESCRIPTOR  *IdtEntry
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
  IN UINTN                   ExceptionType,
  IN EFI_SYSTEM_CONTEXT      SystemContext,
  IN EXCEPTION_HANDLER_DATA  *ExceptionHandlerData
  )
{
  IA32_EFLAGS32          Eflags;
  RESERVED_VECTORS_DATA  *ReservedVectors;

  ReservedVectors = ExceptionHandlerData->ReservedVectors;
  //
  // Save Exception context in global variable in first entry of the exception handler.
  // So when original exception handler returns to the new exception handler (second entry),
  // the Eflags/Cs/Eip/ExceptionData can be used.
  //
  ReservedVectors[ExceptionType].OldFlags      = SystemContext.SystemContextIa32->Eflags;
  ReservedVectors[ExceptionType].OldCs         = SystemContext.SystemContextIa32->Cs;
  ReservedVectors[ExceptionType].OldIp         = SystemContext.SystemContextIa32->Eip;
  ReservedVectors[ExceptionType].ExceptionData = SystemContext.SystemContextIa32->ExceptionData;
  //
  // Clear IF flag to avoid old IDT handler enable interrupt by IRET
  //
  Eflags.UintN                            = SystemContext.SystemContextIa32->Eflags;
  Eflags.Bits.IF                          = 0;
  SystemContext.SystemContextIa32->Eflags = Eflags.UintN;
  //
  // Modify the EIP in stack, then old IDT handler will return to HookAfterStubBegin.
  //
  SystemContext.SystemContextIa32->Eip = (UINTN)ReservedVectors[ExceptionType].HookAfterStubHeaderCode;
}

/**
  Restore CPU exception context when handling EFI_VECTOR_HANDOFF_HOOK_AFTER case.

  @param[in] ExceptionType        Exception type.
  @param[in] SystemContext        Pointer to EFI_SYSTEM_CONTEXT.
  @param[in] ExceptionHandlerData Pointer to exception handler data.
**/
VOID
ArchRestoreExceptionContext (
  IN UINTN                   ExceptionType,
  IN EFI_SYSTEM_CONTEXT      SystemContext,
  IN EXCEPTION_HANDLER_DATA  *ExceptionHandlerData
  )
{
  RESERVED_VECTORS_DATA  *ReservedVectors;

  ReservedVectors                                = ExceptionHandlerData->ReservedVectors;
  SystemContext.SystemContextIa32->Eflags        = ReservedVectors[ExceptionType].OldFlags;
  SystemContext.SystemContextIa32->Cs            = ReservedVectors[ExceptionType].OldCs;
  SystemContext.SystemContextIa32->Eip           = ReservedVectors[ExceptionType].OldIp;
  SystemContext.SystemContextIa32->ExceptionData = ReservedVectors[ExceptionType].ExceptionData;
}

/**
  Setup separate stack for given exceptions.

  @param[in] StackSwitchData      Pointer to data required for setuping up
                                  stack switch.

  @retval EFI_SUCCESS             The exceptions have been successfully
                                  initialized with new stack.
  @retval EFI_INVALID_PARAMETER   StackSwitchData contains invalid content.

**/
EFI_STATUS
ArchSetupExceptionStack (
  IN CPU_EXCEPTION_INIT_DATA  *StackSwitchData
  )
{
  IA32_DESCRIPTOR                 Gdtr;
  IA32_DESCRIPTOR                 Idtr;
  IA32_IDT_GATE_DESCRIPTOR        *IdtTable;
  IA32_TSS_DESCRIPTOR             *TssDesc;
  IA32_TASK_STATE_SEGMENT         *Tss;
  UINTN                           StackTop;
  UINTN                           Index;
  UINTN                           Vector;
  UINTN                           TssBase;
  UINTN                           GdtSize;
  EXCEPTION_HANDLER_TEMPLATE_MAP  TemplateMap;

  if ((StackSwitchData == NULL) ||
      (StackSwitchData->Ia32.Revision != CPU_EXCEPTION_INIT_DATA_REV) ||
      (StackSwitchData->Ia32.KnownGoodStackTop == 0) ||
      (StackSwitchData->Ia32.KnownGoodStackSize == 0) ||
      (StackSwitchData->Ia32.StackSwitchExceptions == NULL) ||
      (StackSwitchData->Ia32.StackSwitchExceptionNumber == 0) ||
      (StackSwitchData->Ia32.StackSwitchExceptionNumber > CPU_EXCEPTION_NUM) ||
      (StackSwitchData->Ia32.GdtTable == NULL) ||
      (StackSwitchData->Ia32.IdtTable == NULL) ||
      (StackSwitchData->Ia32.ExceptionTssDesc == NULL) ||
      (StackSwitchData->Ia32.ExceptionTss == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The caller is responsible for that the GDT table, no matter the existing
  // one or newly allocated, has enough space to hold descriptors for exception
  // task-state segments.
  //
  if (((UINTN)StackSwitchData->Ia32.GdtTable & (IA32_GDT_ALIGNMENT - 1)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINTN)StackSwitchData->Ia32.ExceptionTssDesc < (UINTN)(StackSwitchData->Ia32.GdtTable)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINTN)StackSwitchData->Ia32.ExceptionTssDesc + StackSwitchData->Ia32.ExceptionTssDescSize >
      ((UINTN)(StackSwitchData->Ia32.GdtTable) + StackSwitchData->Ia32.GdtTableSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We need one descriptor and one TSS for current task and every exception
  // specified.
  //
  if (StackSwitchData->Ia32.ExceptionTssDescSize <
      sizeof (IA32_TSS_DESCRIPTOR) * (StackSwitchData->Ia32.StackSwitchExceptionNumber + 1))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (StackSwitchData->Ia32.ExceptionTssSize <
      sizeof (IA32_TASK_STATE_SEGMENT) * (StackSwitchData->Ia32.StackSwitchExceptionNumber + 1))
  {
    return EFI_INVALID_PARAMETER;
  }

  TssDesc = StackSwitchData->Ia32.ExceptionTssDesc;
  Tss     = StackSwitchData->Ia32.ExceptionTss;

  //
  // Initialize new GDT table and/or IDT table, if any
  //
  AsmReadIdtr (&Idtr);
  AsmReadGdtr (&Gdtr);

  GdtSize = (UINTN)TssDesc +
            sizeof (IA32_TSS_DESCRIPTOR) *
            (StackSwitchData->Ia32.StackSwitchExceptionNumber + 1) -
            (UINTN)(StackSwitchData->Ia32.GdtTable);
  if ((UINTN)StackSwitchData->Ia32.GdtTable != Gdtr.Base) {
    CopyMem (StackSwitchData->Ia32.GdtTable, (VOID *)Gdtr.Base, Gdtr.Limit + 1);
    Gdtr.Base  = (UINTN)StackSwitchData->Ia32.GdtTable;
    Gdtr.Limit = (UINT16)GdtSize - 1;
  }

  if ((UINTN)StackSwitchData->Ia32.IdtTable != Idtr.Base) {
    Idtr.Base = (UINTN)StackSwitchData->Ia32.IdtTable;
  }

  if (StackSwitchData->Ia32.IdtTableSize > 0) {
    Idtr.Limit = (UINT16)(StackSwitchData->Ia32.IdtTableSize - 1);
  }

  //
  // Fixup current task descriptor. Task-state segment for current task will
  // be filled by processor during task switching.
  //
  TssBase = (UINTN)Tss;

  TssDesc->Uint64         = 0;
  TssDesc->Bits.LimitLow  = sizeof (IA32_TASK_STATE_SEGMENT) - 1;
  TssDesc->Bits.BaseLow   = (UINT16)TssBase;
  TssDesc->Bits.BaseMid   = (UINT8)(TssBase >> 16);
  TssDesc->Bits.Type      = IA32_GDT_TYPE_TSS;
  TssDesc->Bits.P         = 1;
  TssDesc->Bits.LimitHigh = 0;
  TssDesc->Bits.BaseHigh  = (UINT8)(TssBase >> 24);

  //
  // Fixup exception task descriptor and task-state segment
  //
  AsmGetTssTemplateMap (&TemplateMap);
  StackTop = StackSwitchData->Ia32.KnownGoodStackTop - CPU_STACK_ALIGNMENT;
  StackTop = (UINTN)ALIGN_POINTER (StackTop, CPU_STACK_ALIGNMENT);
  IdtTable = StackSwitchData->Ia32.IdtTable;
  for (Index = 0; Index < StackSwitchData->Ia32.StackSwitchExceptionNumber; ++Index) {
    TssDesc += 1;
    Tss     += 1;

    //
    // Fixup TSS descriptor
    //
    TssBase = (UINTN)Tss;

    TssDesc->Uint64         = 0;
    TssDesc->Bits.LimitLow  = sizeof (IA32_TASK_STATE_SEGMENT) - 1;
    TssDesc->Bits.BaseLow   = (UINT16)TssBase;
    TssDesc->Bits.BaseMid   = (UINT8)(TssBase >> 16);
    TssDesc->Bits.Type      = IA32_GDT_TYPE_TSS;
    TssDesc->Bits.P         = 1;
    TssDesc->Bits.LimitHigh = 0;
    TssDesc->Bits.BaseHigh  = (UINT8)(TssBase >> 24);

    //
    // Fixup TSS
    //
    Vector = StackSwitchData->Ia32.StackSwitchExceptions[Index];
    if ((Vector >= CPU_EXCEPTION_NUM) ||
        (Vector >= (Idtr.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR)))
    {
      continue;
    }

    ZeroMem (Tss, sizeof (*Tss));
    Tss->EIP = (UINT32)(TemplateMap.ExceptionStart
                        + Vector * TemplateMap.ExceptionStubHeaderSize);
    Tss->EFLAGS = 0x2;
    Tss->ESP    = StackTop;
    Tss->CR3    = AsmReadCr3 ();
    Tss->ES     = AsmReadEs ();
    Tss->CS     = AsmReadCs ();
    Tss->SS     = AsmReadSs ();
    Tss->DS     = AsmReadDs ();
    Tss->FS     = AsmReadFs ();
    Tss->GS     = AsmReadGs ();

    StackTop -= StackSwitchData->Ia32.KnownGoodStackSize;

    //
    // Update IDT to use Task Gate for given exception
    //
    IdtTable[Vector].Bits.OffsetLow  = 0;
    IdtTable[Vector].Bits.Selector   = (UINT16)((UINTN)TssDesc - Gdtr.Base);
    IdtTable[Vector].Bits.Reserved_0 = 0;
    IdtTable[Vector].Bits.GateType   = IA32_IDT_GATE_TYPE_TASK;
    IdtTable[Vector].Bits.OffsetHigh = 0;
  }

  //
  // Publish GDT
  //
  AsmWriteGdtr (&Gdtr);

  //
  // Load current task
  //
  AsmWriteTr ((UINT16)((UINTN)StackSwitchData->Ia32.ExceptionTssDesc - Gdtr.Base));

  //
  // Publish IDT
  //
  AsmWriteIdtr (&Idtr);

  return EFI_SUCCESS;
}

/**
  Display processor context.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Processor context to be display.
**/
VOID
EFIAPI
DumpCpuContext (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
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
        "  I:%x R:%x U:%x W:%x P:%x PK:%x SS:%x SGX:%x",
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_ID)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_RSVD) != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_US)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_WR)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_P)    != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_PK)   != 0,
        (SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_SS)   != 0,
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
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  DumpCpuContext (ExceptionType, SystemContext);
  //
  // Dump module image base and module entry point by EIP
  //
  if ((ExceptionType == EXCEPT_IA32_PAGE_FAULT) &&
      ((SystemContext.SystemContextIa32->ExceptionData & IA32_PF_EC_ID) != 0))
  {
    //
    // The EIP in SystemContext could not be used
    // if it is page fault with I/D set.
    //
    DumpModuleImageInfo ((*(UINTN *)(UINTN)SystemContext.SystemContextIa32->Esp));
  } else {
    DumpModuleImageInfo (SystemContext.SystemContextIa32->Eip);
  }
}
