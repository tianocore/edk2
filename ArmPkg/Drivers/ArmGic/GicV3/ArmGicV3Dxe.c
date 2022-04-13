/** @file
*
*  Copyright (c) 2011-2018, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmGicLib.h>

#include "ArmGicDxe.h"

#define ARM_GIC_DEFAULT_PRIORITY  0x80

extern EFI_HARDWARE_INTERRUPT_PROTOCOL   gHardwareInterruptV3Protocol;
extern EFI_HARDWARE_INTERRUPT2_PROTOCOL  gHardwareInterrupt2V3Protocol;

STATIC UINTN  mGicDistributorBase;
STATIC UINTN  mGicRedistributorsBase;

/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
STATIC
EFI_STATUS
EFIAPI
GicV3EnableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicEnableInterrupt (mGicDistributorBase, mGicRedistributorsBase, Source);

  return EFI_SUCCESS;
}

/**
  Disable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt disabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
STATIC
EFI_STATUS
EFIAPI
GicV3DisableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicDisableInterrupt (mGicDistributorBase, mGicRedistributorsBase, Source);

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
STATIC
EFI_STATUS
EFIAPI
GicV3GetInterruptSourceState (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN BOOLEAN                          *InterruptState
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  *InterruptState = ArmGicIsInterruptEnabled (
                      mGicDistributorBase,
                      mGicRedistributorsBase,
                      Source
                      );

  return EFI_SUCCESS;
}

/**
  Signal to the hardware that the End Of Interrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt ended successfully.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
STATIC
EFI_STATUS
EFIAPI
GicV3EndOfInterrupt (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  if (Source >= mGicNumInterrupts) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicV3EndOfInterrupt (Source);
  return EFI_SUCCESS;
}

/**
  EFI_CPU_INTERRUPT_HANDLER that is called when a processor interrupt occurs.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor. This parameter is
                           processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

  @return None

**/
STATIC
VOID
EFIAPI
GicV3IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINT32                      GicInterrupt;
  HARDWARE_INTERRUPT_HANDLER  InterruptHandler;

  GicInterrupt = ArmGicV3AcknowledgeInterrupt ();

  // Special Interrupts (ID1020-ID1023) have an Interrupt ID greater than the
  // number of interrupt (ie: Spurious interrupt).
  if ((GicInterrupt & ARM_GIC_ICCIAR_ACKINTID) >= mGicNumInterrupts) {
    // The special interrupt do not need to be acknowledge
    return;
  }

  InterruptHandler = gRegisteredInterruptHandlers[GicInterrupt];
  if (InterruptHandler != NULL) {
    // Call the registered interrupt handler.
    InterruptHandler (GicInterrupt, SystemContext);
  } else {
    DEBUG ((DEBUG_ERROR, "Spurious GIC interrupt: 0x%x\n", GicInterrupt));
    GicV3EndOfInterrupt (&gHardwareInterruptV3Protocol, GicInterrupt);
  }
}

// The protocol instance produced by this driver
EFI_HARDWARE_INTERRUPT_PROTOCOL  gHardwareInterruptV3Protocol = {
  RegisterInterruptSource,
  GicV3EnableInterruptSource,
  GicV3DisableInterruptSource,
  GicV3GetInterruptSourceState,
  GicV3EndOfInterrupt
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
GicV3GetTriggerType (
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
GicV3SetTriggerType (
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

  Status = GicV3GetInterruptSourceState (
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
    GicV3DisableInterruptSource (
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
    GicV3EnableInterruptSource (
      (EFI_HARDWARE_INTERRUPT_PROTOCOL *)This,
      Source
      );
  }

  return EFI_SUCCESS;
}

EFI_HARDWARE_INTERRUPT2_PROTOCOL  gHardwareInterrupt2V3Protocol = {
  (HARDWARE_INTERRUPT2_REGISTER)RegisterInterruptSource,
  (HARDWARE_INTERRUPT2_ENABLE)GicV3EnableInterruptSource,
  (HARDWARE_INTERRUPT2_DISABLE)GicV3DisableInterruptSource,
  (HARDWARE_INTERRUPT2_INTERRUPT_STATE)GicV3GetInterruptSourceState,
  (HARDWARE_INTERRUPT2_END_OF_INTERRUPT)GicV3EndOfInterrupt,
  GicV3GetTriggerType,
  GicV3SetTriggerType
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
GicV3ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  // Acknowledge all pending interrupts
  for (Index = 0; Index < mGicNumInterrupts; Index++) {
    GicV3DisableInterruptSource (&gHardwareInterruptV3Protocol, Index);
  }

  // Disable Gic Interface
  ArmGicV3DisableInterruptInterface ();

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
GicV3DxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT64      CpuTarget;
  UINT64      MpId;

  // Make sure the Interrupt Controller Protocol is not already installed in
  // the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gHardwareInterruptProtocolGuid);

  mGicDistributorBase    = PcdGet64 (PcdGicDistributorBase);
  mGicRedistributorsBase = PcdGet64 (PcdGicRedistributorsBase);
  mGicNumInterrupts      = ArmGicGetMaxNumInterrupts (mGicDistributorBase);

  // We will be driving this GIC in native v3 mode, i.e., with Affinity
  // Routing enabled. So ensure that the ARE bit is set.
  if (!FeaturePcdGet (PcdArmGicV3WithV2Legacy)) {
    MmioOr32 (mGicDistributorBase + ARM_GIC_ICDDCR, ARM_GIC_ICDDCR_ARE);
  }

  for (Index = 0; Index < mGicNumInterrupts; Index++) {
    GicV3DisableInterruptSource (&gHardwareInterruptV3Protocol, Index);

    // Set Priority
    ArmGicSetInterruptPriority (
      mGicDistributorBase,
      mGicRedistributorsBase,
      Index,
      ARM_GIC_DEFAULT_PRIORITY
      );
  }

  // Targets the interrupts to the Primary Cpu

  if (FeaturePcdGet (PcdArmGicV3WithV2Legacy)) {
    // Only Primary CPU will run this code. We can identify our GIC CPU ID by
    // reading the GIC Distributor Target register. The 8 first
    // GICD_ITARGETSRn are banked to each connected CPU. These 8 registers
    // hold the CPU targets fields for interrupts 0-31. More Info in the GIC
    // Specification about "Interrupt Processor Targets Registers"

    // Read the first Interrupt Processor Targets Register (that corresponds
    // to the 4 first SGIs)
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
  } else {
    MpId      = ArmReadMpidr ();
    CpuTarget = MpId &
                (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2 | ARM_CORE_AFF3);

    if ((MmioRead32 (
           mGicDistributorBase + ARM_GIC_ICDDCR
           ) & ARM_GIC_ICDDCR_DS) != 0)
    {
      // If the Disable Security (DS) control bit is set, we are dealing with a
      // GIC that has only one security state. In this case, let's assume we are
      // executing in non-secure state (which is appropriate for DXE modules)
      // and that no other firmware has performed any configuration on the GIC.
      // This means we need to reconfigure all interrupts to non-secure Group 1
      // first.

      MmioWrite32 (
        mGicRedistributorsBase + ARM_GICR_CTLR_FRAME_SIZE + ARM_GIC_ICDISR,
        0xffffffff
        );

      for (Index = 32; Index < mGicNumInterrupts; Index += 32) {
        MmioWrite32 (
          mGicDistributorBase + ARM_GIC_ICDISR + Index / 8,
          0xffffffff
          );
      }
    }

    // Route the SPIs to the primary CPU. SPIs start at the INTID 32
    for (Index = 0; Index < (mGicNumInterrupts - 32); Index++) {
      MmioWrite64 (
        mGicDistributorBase + ARM_GICD_IROUTER + (Index * 8),
        CpuTarget
        );
    }
  }

  // Set binary point reg to 0x7 (no preemption)
  ArmGicV3SetBinaryPointer (0x7);

  // Set priority mask reg to 0xff to allow all priorities through
  ArmGicV3SetPriorityMask (0xff);

  // Enable gic cpu interface
  ArmGicV3EnableInterruptInterface ();

  // Enable gic distributor
  ArmGicEnableDistributor (mGicDistributorBase);

  Status = InstallAndRegisterInterruptService (
             &gHardwareInterruptV3Protocol,
             &gHardwareInterrupt2V3Protocol,
             GicV3IrqInterruptHandler,
             GicV3ExitBootServicesEvent
             );

  return Status;
}
