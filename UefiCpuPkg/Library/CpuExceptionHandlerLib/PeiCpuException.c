/** @file
  CPU exception handler library implementation for PEIM module.

Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include "CpuExceptionCommon.h"
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

CONST UINTN  mDoFarReturnFlag = 0;

typedef struct {
  UINT8                     ExceptionStubHeader[HOOKAFTER_STUB_SIZE];
  EXCEPTION_HANDLER_DATA    *ExceptionHandlerData;
} EXCEPTION0_STUB_HEADER;

/**
  Get exception handler data pointer from IDT[0].

  The exception #0 stub header is duplicated in an allocated pool with extra 4-byte/8-byte to store the
  exception handler data. The new allocated memory layout follows structure EXCEPTION0_STUB_HEADER.
  The code assumes that all processors uses the same exception handler for #0 exception.

  @return  pointer to exception handler data.
**/
EXCEPTION_HANDLER_DATA *
GetExceptionHandlerData (
  VOID
  )
{
  IA32_DESCRIPTOR           IdtDescriptor;
  IA32_IDT_GATE_DESCRIPTOR  *IdtTable;
  EXCEPTION0_STUB_HEADER    *Exception0StubHeader;

  AsmReadIdtr (&IdtDescriptor);
  IdtTable = (IA32_IDT_GATE_DESCRIPTOR *)IdtDescriptor.Base;

  Exception0StubHeader = (EXCEPTION0_STUB_HEADER *)ArchGetIdtHandler (&IdtTable[0]);
  return Exception0StubHeader->ExceptionHandlerData;
}

/**
  Set exception handler data pointer to IDT[0].

  The exception #0 stub header is duplicated in an allocated pool with extra 4-byte/8-byte to store the
  exception handler data. The new allocated memory layout follows structure EXCEPTION0_STUB_HEADER.
  The code assumes that all processors uses the same exception handler for #0 exception.

  @param ExceptionHandlerData  pointer to exception handler data.
**/
VOID
SetExceptionHandlerData (
  IN EXCEPTION_HANDLER_DATA  *ExceptionHandlerData
  )
{
  EXCEPTION0_STUB_HEADER    *Exception0StubHeader;
  IA32_DESCRIPTOR           IdtDescriptor;
  IA32_IDT_GATE_DESCRIPTOR  *IdtTable;

  //
  // Duplicate the exception #0 stub header in pool and cache the ExceptionHandlerData just after the stub header.
  // So AP can get the ExceptionHandlerData by reading the IDT[0].
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtTable = (IA32_IDT_GATE_DESCRIPTOR *)IdtDescriptor.Base;

  Exception0StubHeader = AllocatePool (sizeof (*Exception0StubHeader));
  ASSERT (Exception0StubHeader != NULL);
  CopyMem (
    Exception0StubHeader->ExceptionStubHeader,
    (VOID *)ArchGetIdtHandler (&IdtTable[0]),
    sizeof (Exception0StubHeader->ExceptionStubHeader)
    );
  Exception0StubHeader->ExceptionHandlerData = ExceptionHandlerData;
  ArchUpdateIdtEntry (&IdtTable[0], (UINTN)Exception0StubHeader->ExceptionStubHeader);
}

/**
  Common exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
CommonExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EXCEPTION_HANDLER_DATA  *ExceptionHandlerData;

  ExceptionHandlerData = GetExceptionHandlerData ();
  CommonExceptionHandlerWorker (ExceptionType, SystemContext, ExceptionHandlerData);
}

/**
  Initializes all CPU exceptions entries and provides the default exception handlers.

  Caller should try to get an array of interrupt and/or exception vectors that are in use and need to
  persist by EFI_VECTOR_HANDOFF_INFO defined in PI 1.3 specification.
  If caller cannot get reserved vector list or it does not exists, set VectorInfo to NULL.
  If VectorInfo is not NULL, the exception vectors will be initialized per vector attribute accordingly.
  Note: Before invoking this API, caller must allocate memory for IDT table and load
        IDTR by AsmWriteIdtr().

  @param[in]  VectorInfo    Pointer to reserved vector list.

  @retval EFI_SUCCESS           CPU Exception Entries have been successfully initialized
                                with default exception handlers.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
InitializeCpuExceptionHandlers (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL
  )
{
  EFI_STATUS              Status;
  EXCEPTION_HANDLER_DATA  *ExceptionHandlerData;
  RESERVED_VECTORS_DATA   *ReservedVectors;

  ReservedVectors = AllocatePool (sizeof (RESERVED_VECTORS_DATA) * CPU_EXCEPTION_NUM);
  ASSERT (ReservedVectors != NULL);

  ExceptionHandlerData = AllocatePool (sizeof (EXCEPTION_HANDLER_DATA));
  ASSERT (ExceptionHandlerData != NULL);
  ExceptionHandlerData->IdtEntryCount            = CPU_EXCEPTION_NUM;
  ExceptionHandlerData->ReservedVectors          = ReservedVectors;
  ExceptionHandlerData->ExternalInterruptHandler = NULL;
  InitializeSpinLock (&ExceptionHandlerData->DisplayMessageSpinLock);

  Status = InitializeCpuExceptionHandlersWorker (VectorInfo, ExceptionHandlerData);
  if (EFI_ERROR (Status)) {
    FreePool (ReservedVectors);
    FreePool (ExceptionHandlerData);
    return Status;
  }

  SetExceptionHandlerData (ExceptionHandlerData);
  return EFI_SUCCESS;
}

/**
  Setup separate stacks for certain exception handlers.
  If the input Buffer and BufferSize are both NULL, use global variable if possible.

  @param[in]       Buffer        Point to buffer used to separate exception stack.
  @param[in, out]  BufferSize    On input, it indicates the byte size of Buffer.
                                 If the size is not enough, the return status will
                                 be EFI_BUFFER_TOO_SMALL, and output BufferSize
                                 will be the size it needs.

  @retval EFI_SUCCESS             The stacks are assigned successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.
  @retval EFI_BUFFER_TOO_SMALL    This BufferSize is too small.
**/
EFI_STATUS
EFIAPI
InitializeSeparateExceptionStacks (
  IN     VOID   *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  CPU_EXCEPTION_INIT_DATA  EssData;
  IA32_DESCRIPTOR          Idtr;
  IA32_DESCRIPTOR          Gdtr;
  UINTN                    NeedBufferSize;
  UINTN                    StackTop;
  UINT8                    *NewGdtTable;

  //
  // X64 needs only one TSS of current task working for all exceptions
  // because of its IST feature. IA32 needs one TSS for each exception
  // in addition to current task. To simplify the code, we report the
  // needed memory for IA32 case to cover both IA32 and X64 exception
  // stack switch.
  //
  // Layout of memory needed for each processor:
  //    --------------------------------
  //    |            Alignment         |  (just in case)
  //    --------------------------------
  //    |                              |
  //    |        Original GDT          |
  //    |                              |
  //    --------------------------------
  //    |    Current task descriptor   |
  //    --------------------------------
  //    |                              |
  //    |  Exception task descriptors  |  X ExceptionNumber
  //    |                              |
  //    --------------------------------
  //    |  Current task-state segment  |
  //    --------------------------------
  //    |                              |
  //    | Exception task-state segment |  X ExceptionNumber
  //    |                              |
  //    --------------------------------
  //

  if ((Buffer == NULL) && (BufferSize == NULL)) {
    return EFI_UNSUPPORTED;
  }

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsmReadGdtr (&Gdtr);
  //
  // Total needed size includes stack size, new GDT table size, TSS size.
  // Add another DESCRIPTOR size for alignment requiremet.
  //
  NeedBufferSize = CPU_STACK_SWITCH_EXCEPTION_NUMBER * CPU_KNOWN_GOOD_STACK_SIZE +
                   CPU_TSS_DESC_SIZE + Gdtr.Limit + 1 +
                   CPU_TSS_SIZE +
                   sizeof (IA32_TSS_DESCRIPTOR);
  if (*BufferSize < NeedBufferSize) {
    *BufferSize = NeedBufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StackTop    = (UINTN)Buffer + CPU_STACK_SWITCH_EXCEPTION_NUMBER * CPU_KNOWN_GOOD_STACK_SIZE;
  NewGdtTable = ALIGN_POINTER (StackTop, sizeof (IA32_TSS_DESCRIPTOR));

  AsmReadIdtr (&Idtr);
  EssData.KnownGoodStackTop          = StackTop;
  EssData.KnownGoodStackSize         = CPU_KNOWN_GOOD_STACK_SIZE;
  EssData.StackSwitchExceptions      = CPU_STACK_SWITCH_EXCEPTION_LIST;
  EssData.StackSwitchExceptionNumber = CPU_STACK_SWITCH_EXCEPTION_NUMBER;
  EssData.IdtTable                   = (VOID *)Idtr.Base;
  EssData.IdtTableSize               = Idtr.Limit + 1;
  EssData.GdtTable                   = NewGdtTable;
  EssData.GdtTableSize               = CPU_TSS_DESC_SIZE + Gdtr.Limit + 1;
  EssData.ExceptionTssDesc           = NewGdtTable + Gdtr.Limit + 1;
  EssData.ExceptionTssDescSize       = CPU_TSS_DESC_SIZE;
  EssData.ExceptionTss               = NewGdtTable + Gdtr.Limit + 1 + CPU_TSS_DESC_SIZE;
  EssData.ExceptionTssSize           = CPU_TSS_SIZE;

  return ArchSetupExceptionStack (&EssData);
}
