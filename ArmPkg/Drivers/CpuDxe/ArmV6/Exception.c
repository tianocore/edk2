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
#include <Library/ArmExceptionLib.h>

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
  // pass down to ExceptionLib
  return (EFI_STATUS)RegisterExceptionHandler(InterruptType, InterruptHandler);
}

EFI_STATUS
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL    *Cpu
  )
{
  EFI_STATUS           Status;
  BOOLEAN              IrqEnabled;
  BOOLEAN              FiqEnabled;
  EFI_PHYSICAL_ADDRESS Base;

  Status = EFI_SUCCESS;

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
    // Reserve space for the exception handlers (they already were copied by ArmExceptionLib)
    //
    Base = (EFI_PHYSICAL_ADDRESS)PcdGet32 (PcdCpuVectorBaseAddress);
    Status = gBS->AllocatePages (AllocateAddress, EfiBootServicesCode, 1, &Base);
    // If the request was for memory that's not in the memory map (which is often the case for 0x00000000
    // on embedded systems, for example, we don't want to hang up.  So we'll check here for a status of
    // EFI_NOT_FOUND, and continue in that case.
    if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND)) {
      ASSERT_EFI_ERROR (Status);
    }

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
