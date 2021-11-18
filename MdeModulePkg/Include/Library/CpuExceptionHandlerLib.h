/** @file
  CPU Exception library provides the default CPU interrupt/exception handler.
  It also provides capability to register user interrupt/exception handler.

  Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __CPU_EXCEPTION_HANDLER_LIB_H__
#define __CPU_EXCEPTION_HANDLER_LIB_H__

#include <Ppi/VectorHandoffInfo.h>
#include <Protocol/Cpu.h>

#define CPU_EXCEPTION_INIT_DATA_REV  1

typedef union {
  struct {
    //
    // Revision number of this structure.
    //
    UINT32     Revision;
    //
    // The address of top of known good stack reserved for *ALL* exceptions
    // listed in field StackSwitchExceptions.
    //
    UINTN      KnownGoodStackTop;
    //
    // The size of known good stack for *ONE* exception only.
    //
    UINTN      KnownGoodStackSize;
    //
    // Buffer of exception vector list for stack switch.
    //
    UINT8      *StackSwitchExceptions;
    //
    // Number of exception vectors in StackSwitchExceptions.
    //
    UINTN      StackSwitchExceptionNumber;
    //
    // Buffer of IDT table. It must be type of IA32_IDT_GATE_DESCRIPTOR.
    // Normally there's no need to change IDT table size.
    //
    VOID       *IdtTable;
    //
    // Size of buffer for IdtTable.
    //
    UINTN      IdtTableSize;
    //
    // Buffer of GDT table. It must be type of IA32_SEGMENT_DESCRIPTOR.
    //
    VOID       *GdtTable;
    //
    // Size of buffer for GdtTable.
    //
    UINTN      GdtTableSize;
    //
    // Pointer to start address of descriptor of exception task gate in the
    // GDT table. It must be type of IA32_TSS_DESCRIPTOR.
    //
    VOID       *ExceptionTssDesc;
    //
    // Size of buffer for ExceptionTssDesc.
    //
    UINTN      ExceptionTssDescSize;
    //
    // Buffer of task-state segment for exceptions. It must be type of
    // IA32_TASK_STATE_SEGMENT.
    //
    VOID       *ExceptionTss;
    //
    // Size of buffer for ExceptionTss.
    //
    UINTN      ExceptionTssSize;
    //
    // Flag to indicate if default handlers should be initialized or not.
    //
    BOOLEAN    InitDefaultHandlers;
  } Ia32, X64;
} CPU_EXCEPTION_INIT_DATA;

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
  );

/**
  Initializes all CPU exceptions entries with optional extra initializations.

  By default, this method should include all functionalities implemented by
  InitializeCpuExceptionHandlers(), plus extra initialization works, if any.
  This could be done by calling InitializeCpuExceptionHandlers() directly
  in this method besides the extra works.

  InitData is optional and its use and content are processor arch dependent.
  The typical usage of it is to convey resources which have to be reserved
  elsewhere and are necessary for the extra initializations of exception.

  @param[in]  VectorInfo    Pointer to reserved vector list.
  @param[in]  InitData      Pointer to data optional for extra initializations
                            of exception.

  @retval EFI_SUCCESS             The exceptions have been successfully
                                  initialized.
  @retval EFI_INVALID_PARAMETER   VectorInfo or InitData contains invalid
                                  content.
  @retval EFI_UNSUPPORTED         This function is not supported.

**/
EFI_STATUS
EFIAPI
InitializeCpuExceptionHandlersEx (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL,
  IN CPU_EXCEPTION_INIT_DATA  *InitData OPTIONAL
  );

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
  );

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
  );

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
  );

#endif
