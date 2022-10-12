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
  Registers a function to be called from the processor interrupt handler.

  This function registers and enables the handler specified by InterruptHandler for a processor
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled.
  The installed handler is called once for each processor interrupt or exception.
  NOTE: This function should be invoked after InitializeCpuExceptionHandlers() is invoked,
  otherwise EFI_UNSUPPORTED returned.

  @param[in]  InterruptType     Defines which interrupt or exception to hook.
  @param[in]  InterruptHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                                when a processor interrupt occurs. If this parameter is NULL, then the handler
                                will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported,
                                or this function is not supported.
**/
EFI_STATUS
EFIAPI
RegisterCpuInterruptHandler (
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  EXCEPTION_HANDLER_DATA  *ExceptionHandlerData;

  ExceptionHandlerData = GetExceptionHandlerData ();
  return RegisterCpuInterruptHandlerWorker (InterruptType, InterruptHandler, ExceptionHandlerData);
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
  ExceptionHandlerData->ExternalInterruptHandler = AllocateZeroPool (sizeof (EFI_CPU_INTERRUPT_HANDLER) * ExceptionHandlerData->IdtEntryCount);
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
  if ((Buffer == NULL) && (BufferSize == NULL)) {
    return EFI_UNSUPPORTED;
  }

  return ArchSetupExceptionStack (Buffer, BufferSize);
}
