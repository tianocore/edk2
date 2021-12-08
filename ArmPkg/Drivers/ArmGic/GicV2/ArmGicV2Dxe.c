/*++

Copyright (c) 2009, Hewlett-Packard Company. All rights reserved.<BR>
Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011-2017, ARM Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  GicV2/ArmGicV2Dxe.c

Abstract:

  Driver implementing the GicV2 interrupt controller protocol

--*/

#include <Library/ArmGicLib.h>

#include "ArmGicDxe.h"

#define ARM_GIC_DEFAULT_PRIORITY  0x80

extern EFI_HARDWARE_INTERRUPT_PROTOCOL   gHardwareInterruptV2Protocol;
extern EFI_HARDWARE_INTERRUPT2_PROTOCOL  gHardwareInterrupt2V2Protocol;

STATIC UINT32  mGicInterruptInterfaceBase;
STATIC UINT32  mGicDistributorBase;

/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported

**/
STATIC
EFI_STATUS
EFIAPI
GicV2EnableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
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
STATIC
EFI_STATUS
EFIAPI
GicV2DisableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
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
STATIC
EFI_STATUS
EFIAPI
GicV2GetInterruptSourceState (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN BOOLEAN                          *InterruptState
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
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

  @retval EFI_SUCCESS       Source interrupt ended successfully.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported

**/
STATIC
EFI_STATUS
EFIAPI
GicV2EndOfInterrupt (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicV2EndOfInterrupt (mGicInterruptInterfaceBase, Source);
  return EFI_SUCCESS;
}

/**
  EFI_CPU_INTERRUPT_HANDLER that is called when a processor interrupt occurs.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is
                           processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

  @return None

**/
STATIC
VOID
EFIAPI
GicV2IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINT32                      GicInterrupt;
  HARDWARE_INTERRUPT_HANDLER  InterruptHandler;

  GicInterrupt = ArmGicV2AcknowledgeInterrupt (mGicInterruptInterfaceBase);

  // Special Interrupts (ID1020-ID1023) have an Interrupt ID greater than the
  // number of interrupt (ie: Spurious interrupt).
  if ((GicInterrupt & ARM_GIC_ICCIAR_ACKINTID) >= mGicNumInterrupts) {
    // The special interrupts do not need to be acknowledged
    return;
  }

  InterruptHandler = gRegisteredInterruptHandlers[GicInterrupt];
  if (InterruptHandler != NULL) {
    // Call the registered interrupt handler.
    InterruptHandler (GicInterrupt, SystemContext);
  } else {
    DEBUG ((DEBUG_ERROR, "Spurious GIC interrupt: 0x%x\n", GicInterrupt));
    GicV2EndOfInterrupt (&gHardwareInterruptV2Protocol, GicInterrupt);
  }
}

// The protocol instance produced by this driver
EFI_HARDWARE_INTERRUPT_PROTOCOL  gHardwareInterruptV2Protocol = {
  RegisterInterruptSource,
  GicV2EnableInterruptSource,
  GicV2DisableInterruptSource,
  GicV2GetInterruptSourceState,
  GicV2EndOfInterrupt
};

/**
  Get interrupt trigger type of an interrupt

  @param This          Instance pointer for this protocol
  @param Source        Hardware source of the interrupt.
  @param TriggerType   Returns interrupt trigger type.

  @retval EFI_SUCCESS       Source interrupt supported.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported.
**/
STATIC
EFI_STATUS
EFIAPI
GicV2GetTriggerType (
  IN  EFI_HARDWARE_INTERRUPT2_PROTOCOL      *This,
  IN  HARDWARE_INTERRUPT_SOURCE             Source,
  OUT EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE  *TriggerType
  )
{
  UINTN       RegAddress;
  UINTN       Config1Bit;
  EFI_STATUS  Status;

  Status = GicGetDistributorIcfgBaseAndBit (
             Source,
             &RegAddress,
             &Config1Bit
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((MmioRead32 (RegAddress) & (1 << Config1Bit)) == 0) {
    *TriggerType = EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH;
  } else {
    *TriggerType = EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING;
  }

  return EFI_SUCCESS;
}

/**
  Set interrupt trigger type of an interrupt

  @param This          Instance pointer for this protocol
  @param Source        Hardware source of the interrupt.
  @param TriggerType   Interrupt trigger type.

  @retval EFI_SUCCESS       Source interrupt supported.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported.
**/
STATIC
EFI_STATUS
EFIAPI
GicV2SetTriggerType (
  IN  EFI_HARDWARE_INTERRUPT2_PROTOCOL      *This,
  IN  HARDWARE_INTERRUPT_SOURCE             Source,
  IN  EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE  TriggerType
  )
{
  UINTN       RegAddress;
  UINTN       Config1Bit;
  UINT32      Value;
  EFI_STATUS  Status;
  BOOLEAN     SourceEnabled;

  if (  (TriggerType != EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING)
     && (TriggerType != EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH))
  {
    DEBUG ((
      DEBUG_ERROR,
      "Invalid interrupt trigger type: %d\n", \
      TriggerType
      ));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  Status = GicGetDistributorIcfgBaseAndBit (
             Source,
             &RegAddress,
             &Config1Bit
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GicV2GetInterruptSourceState (
             (EFI_HARDWARE_INTERRUPT_PROTOCOL *)This,
             Source,
             &SourceEnabled
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value = (TriggerType == EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING)
          ?  ARM_GIC_ICDICFR_EDGE_TRIGGERED
          :  ARM_GIC_ICDICFR_LEVEL_TRIGGERED;

  // Before changing the value, we must disable the interrupt,
  // otherwise GIC behavior is UNPREDICTABLE.
  if (SourceEnabled) {
    GicV2DisableInterruptSource (
      (EFI_HARDWARE_INTERRUPT_PROTOCOL *)This,
      Source
      );
  }

  MmioAndThenOr32 (
    RegAddress,
    ~(0x1 << Config1Bit),
    Value << Config1Bit
    );

  // Restore interrupt state
  if (SourceEnabled) {
    GicV2EnableInterruptSource (
      (EFI_HARDWARE_INTERRUPT_PROTOCOL *)This,
      Source
      );
  }

  return EFI_SUCCESS;
}

EFI_HARDWARE_INTERRUPT2_PROTOCOL  gHardwareInterrupt2V2Protocol = {
  (HARDWARE_INTERRUPT2_REGISTER)RegisterInterruptSource,
  (HARDWARE_INTERRUPT2_ENABLE)GicV2EnableInterruptSource,
  (HARDWARE_INTERRUPT2_DISABLE)GicV2DisableInterruptSource,
  (HARDWARE_INTERRUPT2_INTERRUPT_STATE)GicV2GetInterruptSourceState,
  (HARDWARE_INTERRUPT2_END_OF_INTERRUPT)GicV2EndOfInterrupt,
  GicV2GetTriggerType,
  GicV2SetTriggerType
};

/**
  Shutdown our hardware

  DXE Core will disable interrupts and turn off the timer and disable
  interrupts after all the event handlers have run.

  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
STATIC
VOID
EFIAPI
GicV2ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN   Index;
  UINT32  GicInterrupt;

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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT32      RegOffset;
  UINTN       RegShift;
  UINT32      CpuTarget;

  // Make sure the Interrupt Controller Protocol is not already installed in
  // the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gHardwareInterruptProtocolGuid);

  mGicInterruptInterfaceBase = PcdGet64 (PcdGicInterruptInterfaceBase);
  mGicDistributorBase        = PcdGet64 (PcdGicDistributorBase);
  mGicNumInterrupts          = ArmGicGetMaxNumInterrupts (mGicDistributorBase);

  for (Index = 0; Index < mGicNumInterrupts; Index++) {
    GicV2DisableInterruptSource (&gHardwareInterruptV2Protocol, Index);

    // Set Priority
    RegOffset = Index / 4;
    RegShift  = (Index % 4) * 8;
    MmioAndThenOr32 (
      mGicDistributorBase + ARM_GIC_ICDIPR + (4 * RegOffset),
      ~(0xff << RegShift),
      ARM_GIC_DEFAULT_PRIORITY << RegShift
      );
  }

  // Targets the interrupts to the Primary Cpu

  // Only Primary CPU will run this code. We can identify our GIC CPU ID by
  // reading the GIC Distributor Target register. The 8 first GICD_ITARGETSRn
  // are banked to each connected CPU. These 8 registers hold the CPU targets
  // fields for interrupts 0-31. More Info in the GIC Specification about
  // "Interrupt Processor Targets Registers"

  // Read the first Interrupt Processor Targets Register (that corresponds to
  // the 4 first SGIs)
  CpuTarget = MmioRead32 (mGicDistributorBase + ARM_GIC_ICDIPTR);

  // The CPU target is a bit field mapping each CPU to a GIC CPU Interface.
  // This value is 0 when we run on a uniprocessor platform.
  if (CpuTarget != 0) {
    // The 8 first Interrupt Processor Targets Registers are read-only
    for (Index = 8; Index < (mGicNumInterrupts / 4); Index++) {
      MmioWrite32 (
        mGicDistributorBase + ARM_GIC_ICDIPTR + (Index * 4),
        CpuTarget
        );
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
             &gHardwareInterruptV2Protocol,
             &gHardwareInterrupt2V2Protocol,
             GicV2IrqInterruptHandler,
             GicV2ExitBootServicesEvent
             );

  return Status;
}
