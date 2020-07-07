/** @file
  x64 CPU Exception Handler.

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
  OUT IA32_IDT_GATE_DESCRIPTOR       *IdtEntry,
  IN  UINTN                          InterruptHandler
  )
{
  IdtEntry->Bits.OffsetLow   = (UINT16)(UINTN)InterruptHandler;
  IdtEntry->Bits.OffsetHigh  = (UINT16)((UINTN)InterruptHandler >> 16);
  IdtEntry->Bits.OffsetUpper = (UINT32)((UINTN)InterruptHandler >> 32);
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
  return IdtEntry->Bits.OffsetLow + (((UINTN) IdtEntry->Bits.OffsetHigh)  << 16) +
                                    (((UINTN) IdtEntry->Bits.OffsetUpper) << 32);
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
  // Save Exception context in global variable in first entry of the exception handler.
  // So when original exception handler returns to the new exception handler (second entry),
  // the Eflags/Cs/Eip/ExceptionData can be used.
  //
  ReservedVectors[ExceptionType].OldSs         = SystemContext.SystemContextX64->Ss;
  ReservedVectors[ExceptionType].OldSp         = SystemContext.SystemContextX64->Rsp;
  ReservedVectors[ExceptionType].OldFlags      = SystemContext.SystemContextX64->Rflags;
  ReservedVectors[ExceptionType].OldCs         = SystemContext.SystemContextX64->Cs;
  ReservedVectors[ExceptionType].OldIp         = SystemContext.SystemContextX64->Rip;
  ReservedVectors[ExceptionType].ExceptionData = SystemContext.SystemContextX64->ExceptionData;
  //
  // Clear IF flag to avoid old IDT handler enable interrupt by IRET
  //
  Eflags.UintN = SystemContext.SystemContextX64->Rflags;
  Eflags.Bits.IF = 0;
  SystemContext.SystemContextX64->Rflags = Eflags.UintN;
  //
  // Modify the EIP in stack, then old IDT handler will return to HookAfterStubBegin.
  //
  SystemContext.SystemContextX64->Rip = (UINTN) ReservedVectors[ExceptionType].HookAfterStubHeaderCode;
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
  SystemContext.SystemContextX64->Ss            = ReservedVectors[ExceptionType].OldSs;
  SystemContext.SystemContextX64->Rsp           = ReservedVectors[ExceptionType].OldSp;
  SystemContext.SystemContextX64->Rflags        = ReservedVectors[ExceptionType].OldFlags;
  SystemContext.SystemContextX64->Cs            = ReservedVectors[ExceptionType].OldCs;
  SystemContext.SystemContextX64->Rip           = ReservedVectors[ExceptionType].OldIp;
  SystemContext.SystemContextX64->ExceptionData = ReservedVectors[ExceptionType].ExceptionData;
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
  IN CPU_EXCEPTION_INIT_DATA          *StackSwitchData
  )
{
  IA32_DESCRIPTOR                   Gdtr;
  IA32_DESCRIPTOR                   Idtr;
  IA32_IDT_GATE_DESCRIPTOR          *IdtTable;
  IA32_TSS_DESCRIPTOR               *TssDesc;
  IA32_TASK_STATE_SEGMENT           *Tss;
  UINTN                             StackTop;
  UINTN                             Index;
  UINTN                             Vector;
  UINTN                             TssBase;
  UINTN                             GdtSize;

  if (StackSwitchData == NULL ||
      StackSwitchData->Ia32.Revision != CPU_EXCEPTION_INIT_DATA_REV ||
      StackSwitchData->X64.KnownGoodStackTop == 0 ||
      StackSwitchData->X64.KnownGoodStackSize == 0 ||
      StackSwitchData->X64.StackSwitchExceptions == NULL ||
      StackSwitchData->X64.StackSwitchExceptionNumber == 0 ||
      StackSwitchData->X64.StackSwitchExceptionNumber > CPU_EXCEPTION_NUM ||
      StackSwitchData->X64.GdtTable == NULL ||
      StackSwitchData->X64.IdtTable == NULL ||
      StackSwitchData->X64.ExceptionTssDesc == NULL ||
      StackSwitchData->X64.ExceptionTss == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The caller is responsible for that the GDT table, no matter the existing
  // one or newly allocated, has enough space to hold descriptors for exception
  // task-state segments.
  //
  if (((UINTN)StackSwitchData->X64.GdtTable & (IA32_GDT_ALIGNMENT - 1)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINTN)StackSwitchData->X64.ExceptionTssDesc < (UINTN)(StackSwitchData->X64.GdtTable)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((UINTN)StackSwitchData->X64.ExceptionTssDesc + StackSwitchData->X64.ExceptionTssDescSize) >
      ((UINTN)(StackSwitchData->X64.GdtTable) + StackSwitchData->X64.GdtTableSize)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // One task gate descriptor and one task-state segment are needed.
  //
  if (StackSwitchData->X64.ExceptionTssDescSize < sizeof (IA32_TSS_DESCRIPTOR)) {
    return EFI_INVALID_PARAMETER;
  }
  if (StackSwitchData->X64.ExceptionTssSize < sizeof (IA32_TASK_STATE_SEGMENT)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Interrupt stack table supports only 7 vectors.
  //
  TssDesc = StackSwitchData->X64.ExceptionTssDesc;
  Tss     = StackSwitchData->X64.ExceptionTss;
  if (StackSwitchData->X64.StackSwitchExceptionNumber > ARRAY_SIZE (Tss->IST)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize new GDT table and/or IDT table, if any
  //
  AsmReadIdtr (&Idtr);
  AsmReadGdtr (&Gdtr);

  GdtSize = (UINTN)TssDesc + sizeof (IA32_TSS_DESCRIPTOR) -
            (UINTN)(StackSwitchData->X64.GdtTable);
  if ((UINTN)StackSwitchData->X64.GdtTable != Gdtr.Base) {
    CopyMem (StackSwitchData->X64.GdtTable, (VOID *)Gdtr.Base, Gdtr.Limit + 1);
    Gdtr.Base = (UINTN)StackSwitchData->X64.GdtTable;
    Gdtr.Limit = (UINT16)GdtSize - 1;
  }

  if ((UINTN)StackSwitchData->X64.IdtTable != Idtr.Base) {
    Idtr.Base = (UINTN)StackSwitchData->X64.IdtTable;
  }
  if (StackSwitchData->X64.IdtTableSize > 0) {
    Idtr.Limit = (UINT16)(StackSwitchData->X64.IdtTableSize - 1);
  }

  //
  // Fixup current task descriptor. Task-state segment for current task will
  // be filled by processor during task switching.
  //
  TssBase = (UINTN)Tss;

  TssDesc->Uint128.Uint64  = 0;
  TssDesc->Uint128.Uint64_1= 0;
  TssDesc->Bits.LimitLow   = sizeof(IA32_TASK_STATE_SEGMENT) - 1;
  TssDesc->Bits.BaseLow    = (UINT16)TssBase;
  TssDesc->Bits.BaseMidl   = (UINT8)(TssBase >> 16);
  TssDesc->Bits.Type       = IA32_GDT_TYPE_TSS;
  TssDesc->Bits.P          = 1;
  TssDesc->Bits.LimitHigh  = 0;
  TssDesc->Bits.BaseMidh   = (UINT8)(TssBase >> 24);
  TssDesc->Bits.BaseHigh   = (UINT32)(TssBase >> 32);

  //
  // Fixup exception task descriptor and task-state segment
  //
  ZeroMem (Tss, sizeof (*Tss));
  StackTop = StackSwitchData->X64.KnownGoodStackTop - CPU_STACK_ALIGNMENT;
  StackTop = (UINTN)ALIGN_POINTER (StackTop, CPU_STACK_ALIGNMENT);
  IdtTable = StackSwitchData->X64.IdtTable;
  for (Index = 0; Index < StackSwitchData->X64.StackSwitchExceptionNumber; ++Index) {
    //
    // Fixup IST
    //
    Tss->IST[Index] = StackTop;
    StackTop -= StackSwitchData->X64.KnownGoodStackSize;

    //
    // Set the IST field to enable corresponding IST
    //
    Vector = StackSwitchData->X64.StackSwitchExceptions[Index];
    if (Vector >= CPU_EXCEPTION_NUM ||
        Vector >= (Idtr.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR)) {
      continue;
    }
    IdtTable[Vector].Bits.Reserved_0 = (UINT8)(Index + 1);
  }

  //
  // Publish GDT
  //
  AsmWriteGdtr (&Gdtr);

  //
  // Load current task
  //
  AsmWriteTr ((UINT16)((UINTN)StackSwitchData->X64.ExceptionTssDesc - Gdtr.Base));

  //
  // Publish IDT
  //
  AsmWriteIdtr (&Idtr);

  return EFI_SUCCESS;
}

/**
  Display CPU information.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
DumpCpuContext (
  IN EFI_EXCEPTION_TYPE   ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  InternalPrintMessage (
    "!!!! X64 Exception Type - %02x(%a)  CPU Apic ID - %08x !!!!\n",
    ExceptionType,
    GetExceptionNameStr (ExceptionType),
    GetApicId ()
    );
  if ((mErrorCodeFlag & (1 << ExceptionType)) != 0) {
    InternalPrintMessage (
      "ExceptionData - %016lx",
      SystemContext.SystemContextX64->ExceptionData
      );
    if (ExceptionType == EXCEPT_IA32_PAGE_FAULT) {
      InternalPrintMessage (
        "  I:%x R:%x U:%x W:%x P:%x PK:%x SS:%x SGX:%x",
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_ID)   != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_RSVD) != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_US)   != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_WR)   != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_P)    != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_PK)   != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_SS)   != 0,
        (SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_SGX)  != 0
        );
    }
    InternalPrintMessage ("\n");
  }
  InternalPrintMessage (
    "RIP  - %016lx, CS  - %016lx, RFLAGS - %016lx\n",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Rflags
    );
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
  // Dump module image base and module entry point by RIP
  //
  if ((ExceptionType == EXCEPT_IA32_PAGE_FAULT) &&
      ((SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_ID) != 0)) {
    //
    // The RIP in SystemContext could not be used
    // if it is page fault with I/D set.
    //
    DumpModuleImageInfo ((*(UINTN *)(UINTN)SystemContext.SystemContextX64->Rsp));
  } else {
    DumpModuleImageInfo (SystemContext.SystemContextX64->Rip);
  }
}
