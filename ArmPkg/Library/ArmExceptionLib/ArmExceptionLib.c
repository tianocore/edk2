/* @file
*  Main file supporting the SEC Phase for Versatile Express
*
*  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
*  Copyright (c) 2011-2021, Arm Limited. All rights reserved.<BR>
*  Copyright (c) 2016 HP Development Company, L.P.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/CpuExceptionHandlerLib.h>

#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DefaultExceptionHandlerLib.h>

STATIC
RETURN_STATUS
CopyExceptionHandlers(
  IN  PHYSICAL_ADDRESS        BaseAddress
  );

EFI_STATUS
EFIAPI
RegisterExceptionHandler(
  IN EFI_EXCEPTION_TYPE            ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  );

VOID
ExceptionHandlersStart(
  VOID
  );

VOID
ExceptionHandlersEnd(
  VOID
  );

RETURN_STATUS ArchVectorConfig(
  IN  UINTN       VectorBaseAddress
  );

// these globals are provided by the architecture specific source (Arm or AArch64)
extern UINTN                    gMaxExceptionNumber;
extern EFI_EXCEPTION_CALLBACK   gExceptionHandlers[];
extern EFI_EXCEPTION_CALLBACK   gDebuggerExceptionHandlers[];
extern PHYSICAL_ADDRESS         gExceptionVectorAlignmentMask;
extern UINTN                    gDebuggerNoHandlerValue;

// A compiler flag adjusts the compilation of this library to a variant where
// the vectors are relocated (copied) to another location versus using the
// vectors in-place.  Since this effects an assembly .align directive we must
// address this at library build time.  Since this affects the build of the
// library we cannot represent this in a PCD since PCDs are evaluated on
// a per-module basis.
#if defined(ARM_RELOCATE_VECTORS)
STATIC CONST BOOLEAN gArmRelocateVectorTable = TRUE;
#else
STATIC CONST BOOLEAN gArmRelocateVectorTable = FALSE;
#endif


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
InitializeCpuExceptionHandlers(
  IN EFI_VECTOR_HANDOFF_INFO       *VectorInfo OPTIONAL
  )
{
  RETURN_STATUS     Status;
  UINTN             VectorBase;

  Status = EFI_SUCCESS;

  // if we are requested to copy exception handlers to another location
  if (gArmRelocateVectorTable) {

    VectorBase = PcdGet64(PcdCpuVectorBaseAddress);
    Status = CopyExceptionHandlers(VectorBase);

  }
  else { // use VBAR to point to where our exception handlers are

    // The vector table must be aligned for the architecture.  If this
    // assertion fails ensure the appropriate FFS alignment is in effect,
    // which can be accomplished by ensuring the proper Align=X statement
    // in the platform packaging rules.  For ARM Align=32 is required and
    // for AArch64 Align=4K is required.  Align=Auto can be used but this
    // is known to cause an issue with populating the reset vector area
    // for encapsulated FVs.
    ASSERT(((UINTN)ExceptionHandlersStart & gExceptionVectorAlignmentMask) == 0);

    // We do not copy the Exception Table at PcdGet64(PcdCpuVectorBaseAddress). We just set Vector
    // Base Address to point into CpuDxe code.
    VectorBase = (UINTN)ExceptionHandlersStart;

    Status = RETURN_SUCCESS;
  }

  if (!RETURN_ERROR(Status)) {
    // call the architecture-specific routine to prepare for the new vector
    // configuration to take effect
    ArchVectorConfig(VectorBase);

    ArmWriteVBar(VectorBase);
  }

  return RETURN_SUCCESS;
}

/**
Copies exception handlers to the specified address.

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
STATIC
RETURN_STATUS
CopyExceptionHandlers(
  IN  PHYSICAL_ADDRESS        BaseAddress
  )
{
  RETURN_STATUS        Status;
  UINTN                Length;
  UINTN                Index;
  UINT32               *VectorBase;

  // ensure that the destination value specifies an address meeting the vector alignment requirements
  ASSERT ((BaseAddress & gExceptionVectorAlignmentMask) == 0);

  //
  // Copy an implementation of the exception vectors to PcdCpuVectorBaseAddress.
  //
  Length = (UINTN)ExceptionHandlersEnd - (UINTN)ExceptionHandlersStart;

  VectorBase = (UINT32 *)(UINTN)BaseAddress;

  if (FeaturePcdGet(PcdDebuggerExceptionSupport) == TRUE) {
    // Save existing vector table, in case debugger is already hooked in
    CopyMem((VOID *)gDebuggerExceptionHandlers, (VOID *)VectorBase, sizeof (EFI_EXCEPTION_CALLBACK)* (gMaxExceptionNumber+1));
  }

  // Copy our assembly code into the page that contains the exception vectors.
  CopyMem((VOID *)VectorBase, (VOID *)ExceptionHandlersStart, Length);

  //
  // Initialize the C entry points for interrupts
  //
  for (Index = 0; Index <= gMaxExceptionNumber; Index++) {
    if (!FeaturePcdGet(PcdDebuggerExceptionSupport) ||
      (gDebuggerExceptionHandlers[Index] == 0) || (gDebuggerExceptionHandlers[Index] == (VOID *)gDebuggerNoHandlerValue)) {

      Status = RegisterExceptionHandler(Index, NULL);
      ASSERT_EFI_ERROR(Status);
    }
    else {
      // If the debugger has already hooked put its vector back
      VectorBase[Index] = (UINT32)(UINTN)gDebuggerExceptionHandlers[Index];
    }
  }

  // Flush Caches since we updated executable stuff
  InvalidateInstructionCacheRange((VOID *)(UINTN)BaseAddress, Length);

  return RETURN_SUCCESS;
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
InitializeCpuInterruptHandlers(
IN EFI_VECTOR_HANDOFF_INFO       *VectorInfo OPTIONAL
)
{
  // not needed, this is what the CPU driver is for
  return EFI_UNSUPPORTED;
}

/**
Registers a function to be called from the processor exception handler. (On ARM/AArch64 this only
provides exception handlers, not interrupt handling which is provided through the Hardware Interrupt
Protocol.)

This function registers and enables the handler specified by ExceptionHandler for a processor
interrupt or exception type specified by ExceptionType. If ExceptionHandler is NULL, then the
handler for the processor interrupt or exception type specified by ExceptionType is uninstalled.
The installed handler is called once for each processor interrupt or exception.
NOTE: This function should be invoked after InitializeCpuExceptionHandlers() or
InitializeCpuInterruptHandlers() invoked, otherwise EFI_UNSUPPORTED returned.

@param[in]  ExceptionType     Defines which interrupt or exception to hook.
@param[in]  ExceptionHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
when a processor interrupt occurs. If this parameter is NULL, then the handler
will be uninstalled.

@retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
@retval EFI_ALREADY_STARTED   ExceptionHandler is not NULL, and a handler for ExceptionType was
previously installed.
@retval EFI_INVALID_PARAMETER ExceptionHandler is NULL, and a handler for ExceptionType was not
previously installed.
@retval EFI_UNSUPPORTED       The interrupt specified by ExceptionType is not supported,
or this function is not supported.
**/
RETURN_STATUS
RegisterCpuInterruptHandler(
  IN EFI_EXCEPTION_TYPE             ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER      ExceptionHandler
  )
{
  if (ExceptionType > gMaxExceptionNumber) {
    return RETURN_UNSUPPORTED;
  }

  if ((ExceptionHandler != NULL) && (gExceptionHandlers[ExceptionType] != NULL)) {
    return RETURN_ALREADY_STARTED;
  }

  gExceptionHandlers[ExceptionType] = ExceptionHandler;

  return RETURN_SUCCESS;
}

/**
Register exception handler.

@param  This                  A pointer to the SMM_CPU_SERVICE_PROTOCOL instance.
@param  ExceptionType         Defines which interrupt or exception to hook. Type EFI_EXCEPTION_TYPE and
the valid values for this parameter are defined in EFI_DEBUG_SUPPORT_PROTOCOL
of the UEFI 2.0 specification.
@param  InterruptHandler      A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER
that is called when a processor interrupt occurs.
If this parameter is NULL, then the handler will be uninstalled.

@retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
@retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was previously installed.
@retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not previously installed.
@retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
EFIAPI
RegisterExceptionHandler(
  IN EFI_EXCEPTION_TYPE            ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  )
{
  return RegisterCpuInterruptHandler(ExceptionType, InterruptHandler);
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

