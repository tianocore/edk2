/** @file
  Handle OMAP35xx interrupt controller

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/HardwareInterrupt.h>

#include <Omap3530/Omap3530.h>

//
// Notifications
//
EFI_EVENT EfiExitBootServicesEvent      = (EFI_EVENT)NULL;


HARDWARE_INTERRUPT_HANDLER  gRegisteredInterruptHandlers[INT_NROF_VECTORS];

/**
  Shutdown our hardware

  DXE Core will disable interrupts and turn off the timer and disable interrupts
  after all the event handlers have run.

  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  // Disable all interrupts
  MmioWrite32 (INTCPS_MIR(0), 0xFFFFFFFF);
  MmioWrite32 (INTCPS_MIR(1), 0xFFFFFFFF);
  MmioWrite32 (INTCPS_MIR(2), 0xFFFFFFFF);
  MmioWrite32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWIRQAGR);

  // Add code here to disable all FIQs as debugger may have turned one on
}

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
  if (Source > MAX_VECTOR) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((MmioRead32 (INTCPS_ILR(Source)) & INTCPS_ILR_FIQ) == INTCPS_ILR_FIQ) {
    // This vector has been programmed as FIQ so we can't use it for IRQ
    // EFI does not use FIQ, but the debugger can use it to check for
    // ctrl-c. So this ASSERT means you have a conflict with the debug agent
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if ((Handler == NULL) && (gRegisteredInterruptHandlers[Source] == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Handler != NULL) && (gRegisteredInterruptHandlers[Source] != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  gRegisteredInterruptHandlers[Source] = Handler;
  return This->EnableInterruptSource(This, Source);
}


/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
EnableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  UINTN Bank;
  UINTN Bit;

  if (Source > MAX_VECTOR) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  Bank = Source / 32;
  Bit  = 1UL << (Source % 32);

  MmioWrite32 (INTCPS_MIR_CLEAR(Bank), Bit);

  return EFI_SUCCESS;
}


/**
  Disable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt disabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
DisableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  UINTN Bank;
  UINTN Bit;

  if (Source > MAX_VECTOR) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  Bank = Source / 32;
  Bit  = 1UL << (Source % 32);

  MmioWrite32 (INTCPS_MIR_SET(Bank), Bit);

  return EFI_SUCCESS;
}



/**
  Return current state of interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param InterruptState  TRUE: source enabled, FALSE: source disabled.

  @retval EFI_SUCCESS       InterruptState is valid
  @retval EFI_DEVICE_ERROR  InterruptState is not valid

**/
EFI_STATUS
EFIAPI
GetInterruptSourceState (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN BOOLEAN                            *InterruptState
  )
{
  UINTN Bank;
  UINTN Bit;

  if (InterruptState == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Source > MAX_VECTOR) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  Bank = Source / 32;
  Bit  = 1UL << (Source % 32);

  if ((MmioRead32(INTCPS_MIR(Bank)) & Bit) == Bit) {
    *InterruptState = FALSE;
  } else {
    *InterruptState = TRUE;
  }

  return EFI_SUCCESS;
}

/**
  Signal to the hardware that the End Of Intrrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt EOI'ed.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
EndOfInterrupt (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  MmioWrite32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWIRQAGR);
  ArmDataSyncronizationBarrier ();
  return EFI_SUCCESS;
}


/**
  EFI_CPU_INTERRUPT_HANDLER that is called when a processor interrupt occurs.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

  @return None

**/
VOID
EFIAPI
IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE           InterruptType,
  IN EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  UINT32                     Vector;
  HARDWARE_INTERRUPT_HANDLER InterruptHandler;

  Vector = MmioRead32 (INTCPS_SIR_IRQ) & INTCPS_SIR_IRQ_MASK;

  // Needed to prevent infinite nesting when Time Driver lowers TPL
  MmioWrite32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWIRQAGR);
  ArmDataSyncronizationBarrier ();

  InterruptHandler = gRegisteredInterruptHandlers[Vector];
  if (InterruptHandler != NULL) {
    // Call the registered interrupt handler.
    InterruptHandler (Vector, SystemContext);
  }

  // Needed to clear after running the handler
  MmioWrite32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWIRQAGR);
  ArmDataSyncronizationBarrier ();
}

//
// Making this global saves a few bytes in image size
//
EFI_HANDLE  gHardwareInterruptHandle = NULL;

//
// The protocol instance produced by this driver
//
EFI_HARDWARE_INTERRUPT_PROTOCOL gHardwareInterruptProtocol = {
  RegisterInterruptSource,
  EnableInterruptSource,
  DisableInterruptSource,
  GetInterruptSourceState,
  EndOfInterrupt
};

/**
  Initialize the state information for the CPU Architectural Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
InterruptDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_CPU_ARCH_PROTOCOL   *Cpu;

  // Make sure the Interrupt Controller Protocol is not already installed in the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gHardwareInterruptProtocolGuid);

  // Make sure all interrupts are disabled by default.
  MmioWrite32 (INTCPS_MIR(0), 0xFFFFFFFF);
  MmioWrite32 (INTCPS_MIR(1), 0xFFFFFFFF);
  MmioWrite32 (INTCPS_MIR(2), 0xFFFFFFFF);
  MmioOr32 (INTCPS_CONTROL, INTCPS_CONTROL_NEWIRQAGR);

  Status = gBS->InstallMultipleProtocolInterfaces(&gHardwareInterruptHandle,
                                                  &gHardwareInterruptProtocolGuid,   &gHardwareInterruptProtocol,
                                                  NULL);
  ASSERT_EFI_ERROR(Status);

  //
  // Get the CPU protocol that this driver requires.
  //
  Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  ASSERT_EFI_ERROR(Status);

  //
  // Unregister the default exception handler.
  //
  Status = Cpu->RegisterInterruptHandler(Cpu, EXCEPT_ARM_IRQ, NULL);
  ASSERT_EFI_ERROR(Status);

  //
  // Register to receive interrupts
  //
  Status = Cpu->RegisterInterruptHandler(Cpu, EXCEPT_ARM_IRQ, IrqInterruptHandler);
  ASSERT_EFI_ERROR(Status);

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent(EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &EfiExitBootServicesEvent);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

