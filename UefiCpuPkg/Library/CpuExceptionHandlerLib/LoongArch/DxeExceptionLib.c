/** @file DxeExceptionLib.c

  LoongArch exception library implemenation for DXE modules.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/CpuLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Protocol/DebugSupport.h>
#include <Register/LoongArch64/Csr.h>

#include "ExceptionCommon.h"

EFI_EXCEPTION_CALLBACK  ExternalInterruptHandler[MAX_LOONGARCH_INTERRUPT + 1] = { 0 };
EFI_EXCEPTION_CALLBACK  ExceptionHandler[MAX_LOONGARCH_EXCEPTION + 1]         = { 0 };

/**
  Registers a function to be called from the processor interrupt or exception handler.

  This function registers and enables the handler specified by InterruptHandler for a processor
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled.
  The installed handler is called once for each processor interrupt or exception.

  @param  InterruptType    A pointer to the processor's current interrupt state. Set to TRUE if interrupts
                           are enabled and FALSE if interrupts are disabled.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
RegisterCpuInterruptHandler (
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  EFI_EXCEPTION_TYPE  ExceptionType;

  ExceptionType = InterruptType & CSR_ESTAT_EXC;

  if (ExceptionType != 0) {
    //
    // Exception >>= CSR_ESTAT_EXC_SHIFT, convert to ECODE
    //
    ExceptionType >>= CSR_ESTAT_EXC_SHIFT;

    if (ExceptionType > EXCEPT_LOONGARCH_FPE) {
      return EFI_UNSUPPORTED;
    }

    if ((InterruptHandler == NULL) && (ExceptionHandler[InterruptType] == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((InterruptHandler != NULL) && (ExceptionHandler[ExceptionType] != NULL)) {
      return EFI_ALREADY_STARTED;
    }

    ExceptionHandler[ExceptionType] = InterruptHandler;
  } else {
    //
    // Interrupt
    //
    if (InterruptType > MAX_LOONGARCH_INTERRUPT) {
      return EFI_UNSUPPORTED;
    }

    if ((InterruptHandler == NULL) && (ExternalInterruptHandler[InterruptType] == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((InterruptHandler != NULL) && (ExternalInterruptHandler[InterruptType] != NULL)) {
      return EFI_ALREADY_STARTED;
    }

    ExternalInterruptHandler[InterruptType] = InterruptHandler;
  }

  return EFI_SUCCESS;
}

/**
  Common exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
CommonExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  EFI_EXCEPTION_TYPE  InterruptType;

  if (ExceptionType == EXCEPT_LOONGARCH_INT) {
    //
    // Interrupt
    //
    InterruptType = GetInterruptType (SystemContext);
    if (InterruptType == 0xFF) {
      ExceptionType = InterruptType;
    } else {
      if ((ExternalInterruptHandler != NULL) && (ExternalInterruptHandler[InterruptType] != NULL)) {
        ExternalInterruptHandler[InterruptType](InterruptType, SystemContext);
        return;
      }
    }
  } else if (ExceptionType == EXCEPT_LOONGARCH_FPD) {
    EnableFloatingPointUnits ();
    InitializeFloatingPointUnits ();
    return;
  } else {
    //
    // Exception
    //
    ExceptionType >>= CSR_ESTAT_EXC_SHIFT;
    if ((ExceptionHandler != NULL) && (ExceptionHandler[ExceptionType] != NULL)) {
      ExceptionHandler[ExceptionType](ExceptionType, SystemContext);
      return;
    }
  }

  //
  // Only the TLB refill exception use the same entry point as normal exceptions.
  //
  if (CsrRead (LOONGARCH_CSR_TLBRERA) & 0x1) {
    ExceptionType = mExceptionKnownNameNum - 1; // Use only to dump the exception context.
  }

  DefaultExceptionHandler (ExceptionType, SystemContext);
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
  return EFI_SUCCESS;
}
