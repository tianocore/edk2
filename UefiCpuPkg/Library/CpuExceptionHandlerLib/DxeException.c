/** @file
  CPU exception handler library implemenation for DXE modules.

  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include "CpuExceptionCommon.h"
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

CONST UINTN    mDoFarReturnFlag  = 0;

extern SPIN_LOCK                   mDisplayMessageSpinLock;
extern EFI_CPU_INTERRUPT_HANDLER   *mExternalInterruptHandler;

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
  IN EFI_VECTOR_HANDOFF_INFO       *VectorInfo OPTIONAL
  )
{
  return InitializeCpuExceptionHandlersWorker (VectorInfo);
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
  IN EFI_VECTOR_HANDOFF_INFO       *VectorInfo OPTIONAL
  )
{
  EFI_STATUS                         Status;
  IA32_IDT_GATE_DESCRIPTOR           *IdtTable;
  IA32_DESCRIPTOR                    IdtDescriptor;
  UINTN                              IdtEntryCount;
  EXCEPTION_HANDLER_TEMPLATE_MAP     TemplateMap;
  UINTN                              Index;
  UINTN                              InterruptEntry;
  UINT8                              *InterruptEntryCode;

  mReservedVectors = AllocatePool (sizeof (RESERVED_VECTORS_DATA) * CPU_INTERRUPT_NUM);
  ASSERT (mReservedVectors != NULL);
  SetMem ((VOID *) mReservedVectors, sizeof (RESERVED_VECTORS_DATA) * CPU_INTERRUPT_NUM, 0xff);
  if (VectorInfo != NULL) {
    Status = ReadAndVerifyVectorInfo (VectorInfo, mReservedVectors, CPU_INTERRUPT_NUM);
    if (EFI_ERROR (Status)) {
      FreePool (mReservedVectors);
      return EFI_INVALID_PARAMETER;
    }
  }
  InitializeSpinLock (&mDisplayMessageSpinLock);
  mExternalInterruptHandler = AllocateZeroPool (sizeof (EFI_CPU_INTERRUPT_HANDLER) * CPU_INTERRUPT_NUM);
  ASSERT (mExternalInterruptHandler != NULL);

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
  InterruptEntryCode = AllocatePool (TemplateMap.ExceptionStubHeaderSize * CPU_INTERRUPT_NUM);
  ASSERT (InterruptEntryCode != NULL);
  
  InterruptEntry = (UINTN) InterruptEntryCode;
  for (Index = 0; Index < CPU_INTERRUPT_NUM; Index ++) {
    CopyMem (
      (VOID *) InterruptEntry,
      (VOID *) TemplateMap.ExceptionStart,
      TemplateMap.ExceptionStubHeaderSize
      );
    AsmVectorNumFixup ((VOID *) InterruptEntry,  (UINT8) Index, (VOID *) TemplateMap.ExceptionStart);
    InterruptEntry += TemplateMap.ExceptionStubHeaderSize;
  }

  TemplateMap.ExceptionStart = (UINTN) InterruptEntryCode;
  UpdateIdtTable (IdtTable, &TemplateMap, CPU_INTERRUPT_NUM);

  //
  // Load Interrupt Descriptor Table
  //
  IdtDescriptor.Base  = (UINTN) IdtTable;
  IdtDescriptor.Limit = (UINT16) (sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM - 1);
  AsmWriteIdtr ((IA32_DESCRIPTOR *) &IdtDescriptor);

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
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  )
{
  return RegisterCpuInterruptHandlerWorker (InterruptType, InterruptHandler);
}
