/** @file
  x64 CPU Exception Handler.

  Copyright (c) 2012 - 2022, Intel Corporation. All rights reserved.<BR>
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
  IN IA32_IDT_GATE_DESCRIPTOR  *IdtEntry
  )
{
  return IdtEntry->Bits.OffsetLow + (((UINTN)IdtEntry->Bits.OffsetHigh)  << 16) +
         (((UINTN)IdtEntry->Bits.OffsetUpper) << 32);
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
  ReservedVectors[ExceptionType].OldSs         = SystemContext.SystemContextX64->Ss;
  ReservedVectors[ExceptionType].OldSp         = SystemContext.SystemContextX64->Rsp;
  ReservedVectors[ExceptionType].OldFlags      = SystemContext.SystemContextX64->Rflags;
  ReservedVectors[ExceptionType].OldCs         = SystemContext.SystemContextX64->Cs;
  ReservedVectors[ExceptionType].OldIp         = SystemContext.SystemContextX64->Rip;
  ReservedVectors[ExceptionType].ExceptionData = SystemContext.SystemContextX64->ExceptionData;
  //
  // Clear IF flag to avoid old IDT handler enable interrupt by IRET
  //
  Eflags.UintN                           = SystemContext.SystemContextX64->Rflags;
  Eflags.Bits.IF                         = 0;
  SystemContext.SystemContextX64->Rflags = Eflags.UintN;
  //
  // Modify the EIP in stack, then old IDT handler will return to HookAfterStubBegin.
  //
  SystemContext.SystemContextX64->Rip = (UINTN)ReservedVectors[ExceptionType].HookAfterStubHeaderCode;
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

  ReservedVectors                               = ExceptionHandlerData->ReservedVectors;
  SystemContext.SystemContextX64->Ss            = ReservedVectors[ExceptionType].OldSs;
  SystemContext.SystemContextX64->Rsp           = ReservedVectors[ExceptionType].OldSp;
  SystemContext.SystemContextX64->Rflags        = ReservedVectors[ExceptionType].OldFlags;
  SystemContext.SystemContextX64->Cs            = ReservedVectors[ExceptionType].OldCs;
  SystemContext.SystemContextX64->Rip           = ReservedVectors[ExceptionType].OldIp;
  SystemContext.SystemContextX64->ExceptionData = ReservedVectors[ExceptionType].ExceptionData;
}

/**
  Setup separate stacks for certain exception handlers.

  @param[in]       Buffer        Point to buffer used to separate exception stack.
  @param[in, out]  BufferSize    On input, it indicates the byte size of Buffer.
                                 If the size is not enough, the return status will
                                 be EFI_BUFFER_TOO_SMALL, and output BufferSize
                                 will be the size it needs.

  @retval EFI_SUCCESS             The stacks are assigned successfully.
  @retval EFI_BUFFER_TOO_SMALL    This BufferSize is too small.
  @retval EFI_UNSUPPORTED         This function is not supported.
**/
EFI_STATUS
ArchSetupExceptionStack (
  IN     VOID   *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  IA32_DESCRIPTOR           Gdtr;
  IA32_DESCRIPTOR           Idtr;
  IA32_IDT_GATE_DESCRIPTOR  *IdtTable;
  IA32_TSS_DESCRIPTOR       *TssDesc;
  IA32_TASK_STATE_SEGMENT   *Tss;
  VOID                      *NewGdtTable;
  UINTN                     StackTop;
  UINTN                     Index;
  UINTN                     Vector;
  UINTN                     TssBase;
  UINT8                     *StackSwitchExceptions;
  UINTN                     NeedBufferSize;

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Interrupt stack table supports only 7 vectors.
  //
  if (CPU_STACK_SWITCH_EXCEPTION_NUMBER > ARRAY_SIZE (Tss->IST)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Total needed size includes stack size, new GDT table size, TSS size.
  // Add another DESCRIPTOR size for alignment requiremet.
  //
  // Layout of memory needed for each processor:
  //    --------------------------------
  //    |                              |
  //    |          Stack Size          |  X ExceptionNumber
  //    |                              |
  //    --------------------------------
  //    |          Alignment           |  (just in case)
  //    --------------------------------
  //    |                              |
  //    |         Original GDT         |
  //    |                              |
  //    --------------------------------
  //    |                              |
  //    |  Exception task descriptors  |  X 1
  //    |                              |
  //    --------------------------------
  //    |                              |
  //    | Exception task-state segment |  X 1
  //    |                              |
  //    --------------------------------
  //
  AsmReadGdtr (&Gdtr);
  NeedBufferSize = CPU_STACK_SWITCH_EXCEPTION_NUMBER * CPU_KNOWN_GOOD_STACK_SIZE +
                   sizeof (IA32_TSS_DESCRIPTOR) +
                   Gdtr.Limit + 1 + CPU_TSS_DESC_SIZE +
                   CPU_TSS_SIZE;

  if (*BufferSize < NeedBufferSize) {
    *BufferSize = NeedBufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsmReadIdtr (&Idtr);
  StackSwitchExceptions = CPU_STACK_SWITCH_EXCEPTION_LIST;
  StackTop              = (UINTN)Buffer + CPU_STACK_SWITCH_EXCEPTION_NUMBER * CPU_KNOWN_GOOD_STACK_SIZE;
  NewGdtTable           = ALIGN_POINTER (StackTop, sizeof (IA32_TSS_DESCRIPTOR));
  TssDesc               = (IA32_TSS_DESCRIPTOR *)((UINTN)NewGdtTable + Gdtr.Limit + 1);
  Tss                   = (IA32_TASK_STATE_SEGMENT *)((UINTN)TssDesc + CPU_TSS_DESC_SIZE);

  CopyMem (NewGdtTable, (VOID *)Gdtr.Base, Gdtr.Limit + 1);
  Gdtr.Base  = (UINTN)NewGdtTable;
  Gdtr.Limit = (UINT16)(Gdtr.Limit + CPU_TSS_DESC_SIZE);

  //
  // Fixup current task descriptor. Task-state segment for current task will
  // be filled by processor during task switching.
  //
  TssBase = (UINTN)Tss;

  TssDesc->Uint128.Uint64   = 0;
  TssDesc->Uint128.Uint64_1 = 0;
  TssDesc->Bits.LimitLow    = sizeof (IA32_TASK_STATE_SEGMENT) - 1;
  TssDesc->Bits.BaseLow     = (UINT16)TssBase;
  TssDesc->Bits.BaseMidl    = (UINT8)(TssBase >> 16);
  TssDesc->Bits.Type        = IA32_GDT_TYPE_TSS;
  TssDesc->Bits.P           = 1;
  TssDesc->Bits.LimitHigh   = 0;
  TssDesc->Bits.BaseMidh    = (UINT8)(TssBase >> 24);
  TssDesc->Bits.BaseHigh    = (UINT32)(TssBase >> 32);

  //
  // Fixup exception task descriptor and task-state segment
  //
  ZeroMem (Tss, sizeof (*Tss));
  //
  // Plus 1 byte is for compact stack layout in case StackTop is already aligned.
  //
  StackTop = StackTop - CPU_STACK_ALIGNMENT + 1;
  StackTop = (UINTN)ALIGN_POINTER (StackTop, CPU_STACK_ALIGNMENT);
  IdtTable = (IA32_IDT_GATE_DESCRIPTOR  *)Idtr.Base;
  for (Index = 0; Index < CPU_STACK_SWITCH_EXCEPTION_NUMBER; ++Index) {
    //
    // Fixup IST
    //
    Tss->IST[Index] = StackTop;
    StackTop       -= CPU_KNOWN_GOOD_STACK_SIZE;

    //
    // Set the IST field to enable corresponding IST
    //
    Vector = StackSwitchExceptions[Index];
    if ((Vector >= CPU_EXCEPTION_NUM) ||
        (Vector >= (Idtr.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR)))
    {
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
  AsmWriteTr ((UINT16)((UINTN)TssDesc - Gdtr.Base));

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
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
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
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  DumpCpuContext (ExceptionType, SystemContext);
  //
  // Dump module image base and module entry point by RIP
  //
  if ((ExceptionType == EXCEPT_IA32_PAGE_FAULT) &&
      ((SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_ID) != 0))
  {
    //
    // The RIP in SystemContext could not be used
    // if it is page fault with I/D set.
    //
    DumpModuleImageInfo ((*(UINTN *)(UINTN)SystemContext.SystemContextX64->Rsp));
  } else {
    DumpModuleImageInfo (SystemContext.SystemContextX64->Rip);
  }
}
