/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011-2015, ARM Ltd. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  GicV2/ArmGicV2Dxe.c

Abstract:

  Driver implementing the GicV2 interrupt controller protocol

--*/

#include "ArmGicDxe.h"
#include "GicV2/ArmGicV2Lib.h"

#define ARM_GIC_DEFAULT_PRIORITY  0x80

extern EFI_HARDWARE_INTERRUPT_PROTOCOL gHardwareInterruptV2Protocol;

STATIC UINT32 mGicInterruptInterfaceBase;
STATIC UINT32 mGicDistributorBase;

/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported

**/
EFI_STATUS
EFIAPI
GicV2EnableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  if (Source > mGicNumInterrupts) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicEnableInterrupt (mGicDistributorBase, 0, Source);

  return EFI_SUCCESS;
}

/**
  Disable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt disabled.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported

**/
EFI_STATUS
EFIAPI
GicV2DisableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  if (Source > mGicNumInterrupts) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicDisableInterrupt (mGicDistributorBase, 0, Source);

  return EFI_SUCCESS;
}

/**
  Return current state of interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param InterruptState  TRUE: source enabled, FALSE: source disabled.

  @retval EFI_SUCCESS       InterruptState is valid
  @retval EFI_UNSUPPORTED   Source interrupt is not supported

**/
EFI_STATUS
EFIAPI
GicV2GetInterruptSourceState (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN BOOLEAN                            *InterruptState
  )
{
  if (Source > mGicNumInterrupts) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  *InterruptState = ArmGicIsInterruptEnabled (mGicDistributorBase, 0, Source);

  return EFI_SUCCESS;
}

/**
  Signal to the hardware that the End Of Interrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt EOI'ed.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported

**/
EFI_STATUS
EFIAPI
GicV2EndOfInterrupt (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  )
{
  if (Source > mGicNumInterrupts) {
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicV2EndOfInterrupt (mGicInterruptInterfaceBase, Source);
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
GicV2IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE           InterruptType,
  IN EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  UINT32                      GicInterrupt;
  HARDWARE_INTERRUPT_HANDLER  InterruptHandler;

  GicInterrupt = ArmGicV2AcknowledgeInterrupt (mGicInterruptInterfaceBase);

  // Special Interrupts (ID1020-ID1023) have an Interrupt ID greater than the number of interrupt (ie: Spurious interrupt).
  if ((GicInterrupt & ARM_GIC_ICCIAR_ACKINTID) >= mGicNumInterrupts) {
    // The special interrupt do not need to be acknowledge
    return;
  }

  InterruptHandler = gRegisteredInterruptHandlers[GicInterrupt];
  if (InterruptHandler != NULL) {
    // Call the registered interrupt handler.
    InterruptHandler (GicInterrupt, SystemContext);
  } else {
    DEBUG ((EFI_D_ERROR, "Spurious GIC interrupt: 0x%x\n", GicInterrupt));
  }

  GicV2EndOfInterrupt (&gHardwareInterruptV2Protocol, GicInterrupt);
}

//
// The protocol instance produced by this driver
//
EFI_HARDWARE_INTERRUPT_PROTOCOL gHardwareInterruptV2Protocol = {
  RegisterInterruptSource,
  GicV2EnableInterruptSource,
  GicV2DisableInterruptSource,
  GicV2GetInterruptSourceState,
  GicV2EndOfInterrupt
};

/**
  Shutdown our hardware

  DXE Core will disable interrupts and turn off the timer and disable interrupts
  after all the event handlers have run.

  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
VOID
EFIAPI
GicV2ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN    Index;
  UINT32   GicInterrupt;

  // Disable all the interrupts
  for (Index = 0; Index < mGicNumInterrupts; Index++) {
    GicV2DisableInterruptSource (&gHardwareInterruptV2Protocol, Index);
  }

  // Acknowledge all pending interrupts
  do {
    GicInterrupt = ArmGicV2AcknowledgeInterrupt (mGicInterruptInterfaceBase);

    if ((GicInterrupt & ARM_GIC_ICCIAR_ACKINTID) < mGicNumInterrupts) {
      GicV2EndOfInterrupt (&gHardwareInterruptV2Protocol, GicInterrupt);
    }
  } while (!ARM_GIC_IS_SPECIAL_INTERRUPTS (GicInterrupt));

  // Disable Gic Interface
  ArmGicV2DisableInterruptInterface (mGicInterruptInterfaceBase);

  // Disable Gic Distributor
  ArmGicDisableDistributor (mGicDistributorBase);
}

/**
  Initialize the state information for the CPU Architectural Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
GicV2DxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINT32                  RegOffset;
  UINTN                   RegShift;
  UINT32                  CpuTarget;

  // Make sure the Interrupt Controller Protocol is not already installed in the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gHardwareInterruptProtocolGuid);

  mGicInterruptInterfaceBase = PcdGet32 (PcdGicInterruptInterfaceBase);
  mGicDistributorBase = PcdGet32 (PcdGicDistributorBase);
  mGicNumInterrupts = ArmGicGetMaxNumInterrupts (mGicDistributorBase);

  for (Index = 0; Index < mGicNumInterrupts; Index++) {
    GicV2DisableInterruptSource (&gHardwareInterruptV2Protocol, Index);

    // Set Priority
    RegOffset = Index / 4;
    RegShift = (Index % 4) * 8;
    MmioAndThenOr32 (
      mGicDistributorBase + ARM_GIC_ICDIPR + (4 * RegOffset),
      ~(0xff << RegShift),
      ARM_GIC_DEFAULT_PRIORITY << RegShift
      );
  }

  //
  // Targets the interrupts to the Primary Cpu
  //

  // Only Primary CPU will run this code. We can identify our GIC CPU ID by reading
  // the GIC Distributor Target register. The 8 first GICD_ITARGETSRn are banked to each
  // connected CPU. These 8 registers hold the CPU targets fields for interrupts 0-31.
  // More Info in the GIC Specification about "Interrupt Processor Targets Registers"
  //
  // Read the first Interrupt Processor Targets Register (that corresponds to the 4
  // first SGIs)
  CpuTarget = MmioRead32 (mGicDistributorBase + ARM_GIC_ICDIPTR);

  // The CPU target is a bit field mapping each CPU to a GIC CPU Interface. This value
  // is 0 when we run on a uniprocessor platform.
  if (CpuTarget != 0) {
    // The 8 first Interrupt Processor Targets Registers are read-only
    for (Index = 8; Index < (mGicNumInterrupts / 4); Index++) {
      MmioWrite32 (mGicDistributorBase + ARM_GIC_ICDIPTR + (Index * 4), CpuTarget);
    }
  }

  // Set binary point reg to 0x7 (no preemption)
  MmioWrite32 (mGicInterruptInterfaceBase + ARM_GIC_ICCBPR, 0x7);

  // Set priority mask reg to 0xff to allow all priorities through
  MmioWrite32 (mGicInterruptInterfaceBase + ARM_GIC_ICCPMR, 0xff);

  // Enable gic cpu interface
  ArmGicEnableInterruptInterface (mGicInterruptInterfaceBase);

  // Enable gic distributor
  ArmGicEnableDistributor (mGicDistributorBase);

  Status = InstallAndRegisterInterruptService (
          &gHardwareInterruptV2Protocol, GicV2IrqInterruptHandler, GicV2ExitBootServicesEvent);

  return Status;
}
