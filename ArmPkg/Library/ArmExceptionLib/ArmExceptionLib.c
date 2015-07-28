/* @file
*  Main file supporting the SEC Phase for Versatile Express
*
*  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*  Copyright (c) 2015 Hewlett-Packard Company. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/ArmExceptionLib.h>

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DefaultExceptionHandlerLib.h>

RETURN_STATUS InstallExceptionHandlers(VOID);

VOID
ExceptionHandlersStart(
  VOID
  );

// these globals are provided by the architecture specific source (Arm or AArch64)
extern UINTN                    gMaxExceptionNumber;
extern EFI_EXCEPTION_CALLBACK   *gExceptionHandlers;
extern EFI_EXCEPTION_CALLBACK   *gDebuggerExceptionHandlers;

RETURN_STATUS
EFIAPI
ArmInitializeException(
VOID
) {

  ZeroMem(gExceptionHandlers, sizeof(gExceptionHandlers[0]) * gMaxExceptionNumber);

  // if the processor does not implement VBAR then we must copy exception handlers
  // to the fixed vector address instead
  if (FeaturePcdGet(PcdRelocateVectorTable) == TRUE) {

    return InstallExceptionHandlers();

  }
  else { // use VBAR to point to where our exception handlers are

    // The AArch64 Vector table must be 2k-byte aligned - if this assertion fails ensure 'Align=4K'
    // is defined into your FDF for this module.
    ASSERT(((UINTN)ExceptionHandlersStart & ARM_VECTOR_TABLE_ALIGNMENT) == 0);

    // We do not copy the Exception Table at PcdGet32(PcdCpuVectorBaseAddress). We just set Vector
    // Base Address to point into CpuDxe code.
    ArmWriteVBar((UINTN)ExceptionHandlersStart);
  }

  return RETURN_SUCCESS;
}

/**
This function registers and enables the handler specified by ExceptionHandler for a processor
exception type specified by ExceptionType. If ExceptionHandler is NULL, then the
handler for the processor exception type specified by ExceptionType is uninstalled.
The installed handler is called once for each processor exception.

@param  ExceptionType    The exception type for which to register/deregister
@param  ExceptionHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
when a processor exception occurs. If this parameter is NULL, then the handler will be uninstalled.

@retval RETURN_SUCCESS           The handler for the processor exception was successfully installed or uninstalled.
@retval RETURN_ALREADY_STARTED   ExceptionHandler is not NULL, and a handler for ExceptionType was
previously installed.
@retval RETURN_INVALID_PARAMETER ExceptionHandler is NULL, and a handler for ExceptionType was not
previously installed.
@retval RETURN_UNSUPPORTED       The exception specified by ExceptionType is not supported.

**/
RETURN_STATUS
RegisterExceptionHandler(
  IN EFI_EXCEPTION_TYPE             ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER      ExceptionHandler
  ) {
  if (ExceptionType > gMaxExceptionNumber) {
    return RETURN_UNSUPPORTED;
  }

  if ((ExceptionHandler != NULL) && (gExceptionHandlers[ExceptionType] != NULL)) {
    return RETURN_ALREADY_STARTED;
  }

  gExceptionHandlers[ExceptionType] = ExceptionHandler;

  return RETURN_SUCCESS;
}

VOID
EFIAPI
CommonCExceptionHandler(
  IN     EFI_EXCEPTION_TYPE           ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  if (ExceptionType <= gMaxExceptionNumber) {
    if (gExceptionHandlers[ExceptionType]) {
      gExceptionHandlers[ExceptionType](ExceptionType, SystemContext);
      return;
    }
  }
  else {
    DEBUG((EFI_D_ERROR, "Unknown exception type %d\n", ExceptionType));
    ASSERT(FALSE);
  }

  DefaultExceptionHandler(ExceptionType, SystemContext);
}
