/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"
#include <Library/CpuExceptionHandlerLib.h>
#include <Guid/VectorHandoffTable.h>

EFI_STATUS
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL    *Cpu
  ) {
  EFI_STATUS                      Status;
  EFI_VECTOR_HANDOFF_INFO         *VectorInfoList;
  EFI_VECTOR_HANDOFF_INFO         *VectorInfo;
  BOOLEAN                         IrqEnabled;
  BOOLEAN                         FiqEnabled;

  VectorInfo = (EFI_VECTOR_HANDOFF_INFO *)NULL;
  Status = EfiGetSystemConfigurationTable(&gEfiVectorHandoffTableGuid, (VOID **)&VectorInfoList);
  if (Status == EFI_SUCCESS && VectorInfoList != NULL) {
    VectorInfo = VectorInfoList;
  }

  // intialize the CpuExceptionHandlerLib so we take over the exception vector table from the DXE Core
  InitializeCpuExceptionHandlers(VectorInfo);

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
RegisterInterruptHandler(
  IN EFI_EXCEPTION_TYPE             InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  ) {
  // pass down to CpuExceptionHandlerLib
  return (EFI_STATUS)RegisterCpuInterruptHandler(InterruptType, InterruptHandler);
}
