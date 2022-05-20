/** @file
  CPU exception handler library implemenation for DXE modules.

  Copyright (c) 2013 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include "CpuExceptionCommon.h"
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

CONST UINTN  mDoFarReturnFlag = 0;

RESERVED_VECTORS_DATA      mReservedVectorsData[CPU_INTERRUPT_NUM];
EFI_CPU_INTERRUPT_HANDLER  mExternalInterruptHandlerTable[CPU_INTERRUPT_NUM];
EXCEPTION_HANDLER_DATA     mExceptionHandlerData = {
  CPU_INTERRUPT_NUM,
  0,                     // To be fixed
  mReservedVectorsData,
  mExternalInterruptHandlerTable
};

UINT8  mNewStack[CPU_STACK_SWITCH_EXCEPTION_NUMBER *
                 CPU_KNOWN_GOOD_STACK_SIZE];
UINT8  mNewGdt[CPU_TSS_GDT_SIZE];

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
  CommonExceptionHandlerWorker (ExceptionType, SystemContext, &mExceptionHandlerData);
}

/**
  Initializes all CPU exceptions entries and provides the default exception handlers.

  Caller should try to get an array of interrupt and/or exception vectors that are in use and need to
  persist by EFI_VECTOR_HANDOFF_INFO defined in PI 1.3 specification.
  If caller cannot get reserved vector list or it does not exists, set VectorInfo to NULL.
  If VectorInfo is not NULL, the exception vectors will be initialized per vector attribute accordingly.

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
  InitializeSpinLock (&mExceptionHandlerData.DisplayMessageSpinLock);
  return InitializeCpuExceptionHandlersWorker (VectorInfo, &mExceptionHandlerData);
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
  return RegisterCpuInterruptHandlerWorker (InterruptType, InterruptHandler, &mExceptionHandlerData);
}

/**
  Setup separate stacks for certain exception handlers.

  InitData is optional and processor arch dependent.

  @param[in]  InitData      Pointer to data optional for information about how
                            to assign stacks for certain exception handlers.

  @retval EFI_SUCCESS             The stacks are assigned successfully.
  @retval EFI_UNSUPPORTED         This function is not supported.

**/
EFI_STATUS
EFIAPI
InitializeSeparateExceptionStacks (
  IN CPU_EXCEPTION_INIT_DATA  *InitData OPTIONAL
  )
{
  CPU_EXCEPTION_INIT_DATA  EssData;
  IA32_DESCRIPTOR          Idtr;
  IA32_DESCRIPTOR          Gdtr;

  if (InitData == NULL) {
    SetMem (mNewGdt, sizeof (mNewGdt), 0);

    AsmReadIdtr (&Idtr);
    AsmReadGdtr (&Gdtr);

    EssData.X64.Revision                   = CPU_EXCEPTION_INIT_DATA_REV;
    EssData.X64.KnownGoodStackTop          = (UINTN)mNewStack + sizeof (mNewStack);
    EssData.X64.KnownGoodStackSize         = CPU_KNOWN_GOOD_STACK_SIZE;
    EssData.X64.StackSwitchExceptions      = CPU_STACK_SWITCH_EXCEPTION_LIST;
    EssData.X64.StackSwitchExceptionNumber = CPU_STACK_SWITCH_EXCEPTION_NUMBER;
    EssData.X64.IdtTable                   = (VOID *)Idtr.Base;
    EssData.X64.IdtTableSize               = Idtr.Limit + 1;
    EssData.X64.GdtTable                   = mNewGdt;
    EssData.X64.GdtTableSize               = sizeof (mNewGdt);
    EssData.X64.ExceptionTssDesc           = mNewGdt + Gdtr.Limit + 1;
    EssData.X64.ExceptionTssDescSize       = CPU_TSS_DESC_SIZE;
    EssData.X64.ExceptionTss               = mNewGdt + Gdtr.Limit + 1 + CPU_TSS_DESC_SIZE;
    EssData.X64.ExceptionTssSize           = CPU_TSS_SIZE;

    InitData = &EssData;
  }

  return ArchSetupExceptionStack (InitData);
}
