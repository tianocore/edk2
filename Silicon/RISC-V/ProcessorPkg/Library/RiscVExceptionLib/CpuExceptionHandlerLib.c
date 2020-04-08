/** @file
  RISC-V Exception Handler library implementition.

  Copyright (c) 2016 - 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/RiscVCpuLib.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_types.h>

#include "CpuExceptionHandlerLib.h"

STATIC EFI_CPU_INTERRUPT_HANDLER mInterruptHandlers[2];

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
  return EFI_SUCCESS;
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

  DEBUG ((DEBUG_INFO, "%a: Type:%x Handler: %x\n", __FUNCTION__, InterruptType, InterruptHandler));
  mInterruptHandlers[InterruptType] = InterruptHandler;
  return EFI_SUCCESS;
}
/**
  Machine mode trap handler.

  @param[in]  SmodeTrapReg     Registers before trap occurred.

**/
VOID
RiscVSupervisorModeTrapHandler (
  SMODE_TRAP_REGISTERS *SmodeTrapReg
  )
{
  UINTN SCause;
  EFI_SYSTEM_CONTEXT RiscVSystemContext;

  RiscVSystemContext.SystemContextRiscV64 = (EFI_SYSTEM_CONTEXT_RISCV64 *)SmodeTrapReg;
  //
  // Check scasue register.
  //
  SCause = (UINTN)csr_read(RISCV_CSR_SUPERVISOR_SCAUSE);
  if ((SCause & (1UL << (sizeof (UINTN) * 8- 1))) != 0) {
    //
    // This is interrupt event.
    //
    SCause &= ~(1UL << (sizeof (UINTN) * 8- 1));
    if((SCause == SCAUSE_SUPERVISOR_TIMER_INT) && (mInterruptHandlers[EXCEPT_RISCV_TIMER_INT] != NULL)) {
      mInterruptHandlers[EXCEPT_RISCV_TIMER_INT](EXCEPT_RISCV_TIMER_INT, RiscVSystemContext);
    }
  }
}

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
  IN EFI_VECTOR_HANDOFF_INFO            *VectorInfo OPTIONAL,
  IN CPU_EXCEPTION_INIT_DATA            *InitData OPTIONAL
  )
{
  return InitializeCpuExceptionHandlers (VectorInfo);
}

/**
  The constructor function to initial interrupt handlers in
  RISCV_MACHINE_MODE_CONTEXT.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
CpuExceptionHandlerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Set Superviosr mode trap handler.
  //
  csr_write(CSR_STVEC, SupervisorModeTrap);

  return EFI_SUCCESS;
}
