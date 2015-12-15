/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2014, ARM Limited. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"

//FIXME: Will not compile on non-ARMv7 builds
#include <Chipset/ArmV7.h>

VOID
ExceptionHandlersStart (
  VOID
  );

VOID
ExceptionHandlersEnd (
  VOID
  );

VOID
CommonExceptionEntry (
  VOID
  );

VOID
AsmCommonExceptionEntry (
  VOID
  );


EFI_EXCEPTION_CALLBACK  gExceptionHandlers[MAX_ARM_EXCEPTION + 1];
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_ARM_EXCEPTION + 1];



/**
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
RegisterInterruptHandler (
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
{
  if (InterruptType > MAX_ARM_EXCEPTION) {
    return EFI_UNSUPPORTED;
  }

  if ((InterruptHandler != NULL) && (gExceptionHandlers[InterruptType] != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  gExceptionHandlers[InterruptType] = InterruptHandler;

  return EFI_SUCCESS;
}




VOID
EFIAPI
CommonCExceptionHandler (
  IN     EFI_EXCEPTION_TYPE           ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  if (ExceptionType <= MAX_ARM_EXCEPTION) {
    if (gExceptionHandlers[ExceptionType]) {
      gExceptionHandlers[ExceptionType] (ExceptionType, SystemContext);
      return;
    }
  } else {
    DEBUG ((EFI_D_ERROR, "Unknown exception type %d from %08x\n", ExceptionType, SystemContext.SystemContextArm->PC));
    ASSERT (FALSE);
  }

  if (ExceptionType == EXCEPT_ARM_SOFTWARE_INTERRUPT) {
    //
    // ARM JTAG debuggers some times use this vector, so it is not an error to get one
    //
    return;
  }

  DefaultExceptionHandler (ExceptionType, SystemContext);
}



EFI_STATUS
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL    *Cpu
  )
{
  EFI_STATUS           Status;
  UINTN                Offset;
  UINTN                Length;
  UINTN                Index;
  BOOLEAN              IrqEnabled;
  BOOLEAN              FiqEnabled;
  EFI_PHYSICAL_ADDRESS Base;
  UINT32               *VectorBase;

  Status = EFI_SUCCESS;
  ZeroMem (gExceptionHandlers,sizeof(*gExceptionHandlers));

  //
  // Disable interrupts
  //
  Cpu->GetInterruptState (Cpu, &IrqEnabled);
  Cpu->DisableInterrupt (Cpu);

  //
  // EFI does not use the FIQ, but a debugger might so we must disable
  // as we take over the exception vectors.
  //
  FiqEnabled = ArmGetFiqState ();
  ArmDisableFiq ();

  if (FeaturePcdGet(PcdRelocateVectorTable) == TRUE) {
    //
    // Copy an implementation of the ARM exception vectors to PcdCpuVectorBaseAddress.
    //
    Length = (UINTN)ExceptionHandlersEnd - (UINTN)ExceptionHandlersStart;

    // Check if the exception vector is in the low address
    if (PcdGet32 (PcdCpuVectorBaseAddress) == 0x0) {
      // Set SCTLR.V to 0 to enable VBAR to be used
      ArmSetLowVectors ();
    } else {
      ArmSetHighVectors ();
    }

    //
    // Reserve space for the exception handlers
    //
    Base = (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdCpuVectorBaseAddress);
    VectorBase = (UINT32 *)(UINTN)Base;
    Status = gBS->AllocatePages (AllocateAddress, EfiBootServicesCode, EFI_SIZE_TO_PAGES (Length), &Base);
    // If the request was for memory that's not in the memory map (which is often the case for 0x00000000
    // on embedded systems, for example, we don't want to hang up.  So we'll check here for a status of
    // EFI_NOT_FOUND, and continue in that case.
    if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND)) {
      ASSERT_EFI_ERROR (Status);
    }

    if (FeaturePcdGet(PcdDebuggerExceptionSupport) == TRUE) {
      // Save existing vector table, in case debugger is already hooked in
      CopyMem ((VOID *)gDebuggerExceptionHandlers, (VOID *)VectorBase, sizeof (gDebuggerExceptionHandlers));
    }

    // Copy our assembly code into the page that contains the exception vectors.
    CopyMem ((VOID *)VectorBase, (VOID *)ExceptionHandlersStart, Length);

    //
    // Patch in the common Assembly exception handler
    //
    Offset = (UINTN)CommonExceptionEntry - (UINTN)ExceptionHandlersStart;
    *(UINTN *) ((UINT8 *)(UINTN)PcdGet32 (PcdCpuVectorBaseAddress) + Offset) = (UINTN)AsmCommonExceptionEntry;

    //
    // Initialize the C entry points for interrupts
    //
    for (Index = 0; Index <= MAX_ARM_EXCEPTION; Index++) {
      if (!FeaturePcdGet(PcdDebuggerExceptionSupport) ||
          (gDebuggerExceptionHandlers[Index] == 0) || (gDebuggerExceptionHandlers[Index] == (VOID *)(UINTN)0xEAFFFFFE)) {
        // Exception handler contains branch to vector location (jmp $) so no handler
        // NOTE: This code assumes vectors are ARM and not Thumb code
        Status = RegisterInterruptHandler (Index, NULL);
        ASSERT_EFI_ERROR (Status);
      } else {
        // If the debugger has already hooked put its vector back
        VectorBase[Index] = (UINT32)(UINTN)gDebuggerExceptionHandlers[Index];
      }
    }

    // Flush Caches since we updated executable stuff
    InvalidateInstructionCacheRange ((VOID *)PcdGet32(PcdCpuVectorBaseAddress), Length);

    //Note: On ARM processor with the Security Extension, the Vector Table can be located anywhere in the memory.
    //      The Vector Base Address Register defines the location
    ArmWriteVBar (PcdGet32(PcdCpuVectorBaseAddress));
  } else {
    // The Vector table must be 32-byte aligned
    if (((UINT32)ExceptionHandlersStart & ARM_VECTOR_TABLE_ALIGNMENT) != 0) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    // We do not copy the Exception Table at PcdGet32(PcdCpuVectorBaseAddress). We just set Vector Base Address to point into CpuDxe code.
    ArmWriteVBar ((UINT32)ExceptionHandlersStart);
  }

  if (FiqEnabled) {
    ArmEnableFiq ();
  }

  if (IrqEnabled) {
    //
    // Restore interrupt state
    //
    Status = Cpu->EnableInterrupt (Cpu);
  }

  return Status;
}
