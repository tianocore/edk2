/** @file
  CPU exception handler library implemenation for DXE modules.

  Copyright (c) 2013 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include "CpuExceptionCommon.h"
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

CONST UINTN  mDoFarReturnFlag = 0;

RESERVED_VECTORS_DATA      mReservedVectorsData[CPU_EXCEPTION_NUM];
EFI_CPU_INTERRUPT_HANDLER  mExternalInterruptHandlerTable[CPU_EXCEPTION_NUM];
UINTN                      mEnabledInterruptNum = 0;

EXCEPTION_HANDLER_DATA  mExceptionHandlerData;

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
  mExceptionHandlerData.ReservedVectors          = mReservedVectorsData;
  mExceptionHandlerData.ExternalInterruptHandler = mExternalInterruptHandlerTable;
  InitializeSpinLock (&mExceptionHandlerData.DisplayMessageSpinLock);
  return InitializeCpuExceptionHandlersWorker (VectorInfo, &mExceptionHandlerData);
}

/**
  Initializes all CPU interrupt/exceptions entries and provides the default interrupt/exception handlers.

  Caller should try to get an array of interrupt and/or exception vectors that are in use and need to
  persist by EFI_VECTOR_HANDOFF_INFO defined in PI 1.3 specification.
  If caller cannot get reserved vector list or it does not exists, set VectorInfo to NULL.
  If VectorInfo is not NULL, the exception vectors will be initialized per vector attribute accordingly.

  @param[in]  VectorInfo    Pointer to reserved vector list.

  @retval EFI_SUCCESS           All CPU interrupt/exception entries have been successfully initialized
                                with default interrupt/exception handlers.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
EFIAPI
InitializeCpuInterruptHandlers (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL
  )
{
  EFI_STATUS                      Status;
  IA32_IDT_GATE_DESCRIPTOR        *IdtTable;
  IA32_DESCRIPTOR                 IdtDescriptor;
  UINTN                           IdtEntryCount;
  EXCEPTION_HANDLER_TEMPLATE_MAP  TemplateMap;
  UINTN                           Index;
  UINTN                           InterruptEntry;
  UINT8                           *InterruptEntryCode;
  RESERVED_VECTORS_DATA           *ReservedVectors;
  EFI_CPU_INTERRUPT_HANDLER       *ExternalInterruptHandler;

  Status = gBS->AllocatePool (
                  EfiBootServicesCode,
                  sizeof (RESERVED_VECTORS_DATA) * CPU_INTERRUPT_NUM,
                  (VOID **)&ReservedVectors
                  );
  ASSERT (!EFI_ERROR (Status) && ReservedVectors != NULL);
  SetMem ((VOID *)ReservedVectors, sizeof (RESERVED_VECTORS_DATA) * CPU_INTERRUPT_NUM, 0xff);
  if (VectorInfo != NULL) {
    Status = ReadAndVerifyVectorInfo (VectorInfo, ReservedVectors, CPU_INTERRUPT_NUM);
    if (EFI_ERROR (Status)) {
      FreePool (ReservedVectors);
      return EFI_INVALID_PARAMETER;
    }
  }

  ExternalInterruptHandler = AllocateZeroPool (sizeof (EFI_CPU_INTERRUPT_HANDLER) * CPU_INTERRUPT_NUM);
  ASSERT (ExternalInterruptHandler != NULL);

  //
  // Read IDT descriptor and calculate IDT size
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtEntryCount = (IdtDescriptor.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR);
  if (IdtEntryCount > CPU_INTERRUPT_NUM) {
    IdtEntryCount = CPU_INTERRUPT_NUM;
  }

  //
  // Create Interrupt Descriptor Table and Copy the old IDT table in
  //
  IdtTable = AllocateZeroPool (sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM);
  ASSERT (IdtTable != NULL);
  CopyMem (IdtTable, (VOID *)IdtDescriptor.Base, sizeof (IA32_IDT_GATE_DESCRIPTOR) * IdtEntryCount);

  AsmGetTemplateAddressMap (&TemplateMap);
  ASSERT (TemplateMap.ExceptionStubHeaderSize <= HOOKAFTER_STUB_SIZE);

  Status = gBS->AllocatePool (
                  EfiBootServicesCode,
                  TemplateMap.ExceptionStubHeaderSize * CPU_INTERRUPT_NUM,
                  (VOID **)&InterruptEntryCode
                  );
  ASSERT (!EFI_ERROR (Status) && InterruptEntryCode != NULL);

  InterruptEntry = (UINTN)InterruptEntryCode;
  for (Index = 0; Index < CPU_INTERRUPT_NUM; Index++) {
    CopyMem (
      (VOID *)InterruptEntry,
      (VOID *)TemplateMap.ExceptionStart,
      TemplateMap.ExceptionStubHeaderSize
      );
    AsmVectorNumFixup ((VOID *)InterruptEntry, (UINT8)Index, (VOID *)TemplateMap.ExceptionStart);
    InterruptEntry += TemplateMap.ExceptionStubHeaderSize;
  }

  TemplateMap.ExceptionStart                     = (UINTN)InterruptEntryCode;
  mExceptionHandlerData.IdtEntryCount            = CPU_INTERRUPT_NUM;
  mExceptionHandlerData.ReservedVectors          = ReservedVectors;
  mExceptionHandlerData.ExternalInterruptHandler = ExternalInterruptHandler;
  InitializeSpinLock (&mExceptionHandlerData.DisplayMessageSpinLock);

  UpdateIdtTable (IdtTable, &TemplateMap, &mExceptionHandlerData);

  //
  // Load Interrupt Descriptor Table
  //
  IdtDescriptor.Base  = (UINTN)IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM - 1);
  AsmWriteIdtr ((IA32_DESCRIPTOR *)&IdtDescriptor);

  return EFI_SUCCESS;
}

/**
  Registers a function to be called from the processor interrupt handler.

  This function registers and enables the handler specified by InterruptHandler for a processor
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled.
  The installed handler is called once for each processor interrupt or exception.
  NOTE: This function should be invoked after InitializeCpuExceptionHandlers() or
  InitializeCpuInterruptHandlers() invoked, otherwise EFI_UNSUPPORTED returned.

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
  Initializes CPU exceptions entries and setup stack switch for given exceptions.

  This method will call InitializeCpuExceptionHandlers() to setup default
  exception handlers unless indicated not to do it explicitly.

  If InitData is passed with NULL, this method will use the resource reserved
  by global variables to initialize it; Otherwise it will use data in InitData
  to setup stack switch. This is for the different use cases in DxeCore and
  Cpu MP exception initialization.

  @param[in]  VectorInfo    Pointer to reserved vector list.
  @param[in]  InitData      Pointer to data required to setup stack switch for
                            given exceptions.

  @retval EFI_SUCCESS             The exceptions have been successfully
                                  initialized.
  @retval EFI_INVALID_PARAMETER   VectorInfo or InitData contains invalid
                                  content.

**/
EFI_STATUS
EFIAPI
InitializeCpuExceptionHandlersEx (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL,
  IN CPU_EXCEPTION_INIT_DATA  *InitData OPTIONAL
  )
{
  EFI_STATUS               Status;
  CPU_EXCEPTION_INIT_DATA  EssData;
  IA32_DESCRIPTOR          Idtr;
  IA32_DESCRIPTOR          Gdtr;

  //
  // To avoid repeat initialization of default handlers, the caller should pass
  // an extended init data with InitDefaultHandlers set to FALSE. There's no
  // need to call this method to just initialize default handlers. Call non-ex
  // version instead; or this method must be implemented as a simple wrapper of
  // non-ex version of it, if this version has to be called.
  //
  if ((InitData == NULL) || InitData->X64.InitDefaultHandlers) {
    Status = InitializeCpuExceptionHandlers (VectorInfo);
  } else {
    Status = EFI_SUCCESS;
  }

  if (!EFI_ERROR (Status)) {
    //
    // Initializing stack switch is only necessary for Stack Guard functionality.
    //
    if (PcdGetBool (PcdCpuStackGuard)) {
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

      Status = ArchSetupExceptionStack (InitData);
    }
  }

  return Status;
}
