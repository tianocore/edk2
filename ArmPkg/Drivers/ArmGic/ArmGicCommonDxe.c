/*++

Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

--*/

#include "ArmGicDxe.h"

VOID
EFIAPI
IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE           InterruptType,
  IN EFI_SYSTEM_CONTEXT           SystemContext
  );

VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

//
// Making this global saves a few bytes in image size
//
EFI_HANDLE  gHardwareInterruptHandle = NULL;

//
// Notifications
//
EFI_EVENT EfiExitBootServicesEvent      = (EFI_EVENT)NULL;

// Maximum Number of Interrupts
UINTN mGicNumInterrupts                 = 0;

HARDWARE_INTERRUPT_HANDLER  *gRegisteredInterruptHandlers = NULL;

/**
  Register Handler for the specified interrupt source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param Handler  Callback for interrupt. NULL to unregister

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
RegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN HARDWARE_INTERRUPT_HANDLER         Handler
  )
{
  if (Source > mGicNumInterrupts) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((Handler == NULL) && (gRegisteredInterruptHandlers[Source] == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Handler != NULL) && (gRegisteredInterruptHandlers[Source] != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  gRegisteredInterruptHandlers[Source] = Handler;

  // If the interrupt handler is unregistered then disable the interrupt
  if (NULL == Handler){
    return This->DisableInterruptSource (This, Source);
  } else {
    return This->EnableInterruptSource (This, Source);
  }
}

EFI_STATUS
InstallAndRegisterInterruptService (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL   *InterruptProtocol,
  IN EFI_CPU_INTERRUPT_HANDLER          InterruptHandler,
  IN EFI_EVENT_NOTIFY                   ExitBootServicesEvent
  )
{
  EFI_STATUS               Status;
  EFI_CPU_ARCH_PROTOCOL   *Cpu;

  // Initialize the array for the Interrupt Handlers
  gRegisteredInterruptHandlers = (HARDWARE_INTERRUPT_HANDLER*)AllocateZeroPool (sizeof(HARDWARE_INTERRUPT_HANDLER) * mGicNumInterrupts);
  if (gRegisteredInterruptHandlers == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gHardwareInterruptHandle,
                  &gHardwareInterruptProtocolGuid, InterruptProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the CPU protocol that this driver requires.
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Unregister the default exception handler.
  //
  Status = Cpu->RegisterInterruptHandler (Cpu, ARM_ARCH_EXCEPTION_IRQ, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Register to receive interrupts
  //
  Status = Cpu->RegisterInterruptHandler (Cpu, ARM_ARCH_EXCEPTION_IRQ, InterruptHandler);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &EfiExitBootServicesEvent);

  return Status;
}
