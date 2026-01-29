/*++

Copyright (c) 2025, ARM Limited. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  ArmGicV5Dxe.c

Abstract:

  Driver implementing the GICv5 interrupt controller protocol

--*/

#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Protocol/Cpu.h>
#include <Protocol/HardwareInterrupt.h>
#include <Protocol/HardwareInterrupt2.h>

#include "ArmGicDxe.h"
#include "ArmGicV5.h"

STATIC UINTN                       mGicIrsConfigFrameBase;
STATIC UINT32                      SpiRange;
STATIC UINTN                       mGicNumInterrupts;
STATIC HARDWARE_INTERRUPT_HANDLER  *mRegisteredInterruptHandlers;

STATIC EFI_HARDWARE_INTERRUPT_PROTOCOL  mHardwareInterruptV5Protocol;

/**
  Wait for IRS SPI writes to complete.

  @retval TRUE   Writes have completed successfully.
  @retval FALSE  Writes have timed out.
**/
STATIC
BOOLEAN
WaitForIrsSpiIdle (
  )
{
  UINT32  Timeout = GICV5_IRS_IDLE_TIMEOUT_MS;

  while (!(MmioRead32 (mGicIrsConfigFrameBase + GICV5_IRS_SPI_STATUSR) & GICV5_IRS_SPI_STATUSR_IDLE) && (Timeout > 0)) {
    MicroSecondDelay (1);
    Timeout--;
  }

  if (!(MmioRead32 (mGicIrsConfigFrameBase + GICV5_IRS_SPI_STATUSR) & GICV5_IRS_SPI_STATUSR_IDLE)) {
    DEBUG ((DEBUG_ERROR, "WaitForIrsSpiIdle: timed out\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  Wait for IRS SPI writes to complete, and return valid status.

  @retval TRUE   Writes have completed successfully, register status is valid
  @retval FALSE  Writes have timed out, or register status is invalid.
**/
STATIC
BOOLEAN
WaitForIrsSpiIdleValid (
  )
{
  if (!WaitForIrsSpiIdle ()) {
    return FALSE;
  }

  if (!(MmioRead32 (mGicIrsConfigFrameBase + GICV5_IRS_SPI_STATUSR) & GICV5_IRS_SPI_STATUSR_V)) {
    DEBUG ((DEBUG_ERROR, "WaitForIrsSpiIdle: status invalid\n"));
    return FALSE;
  }

  return TRUE;
}

/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_UNSUPPORTED   Invalid interrupt source number.
**/
STATIC
EFI_STATUS
EFIAPI
GicV5EnableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  UINT32  InterruptType = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINT32  InterruptId   = Source & GICV5_INTERRUPT_ID_MASK;

  if (InterruptType == GICV5_INTERRUPT_TYPE_SPI) {
    ArmGicV5SpiEnable (Source);
  } else if (InterruptType == GICV5_INTERRUPT_TYPE_PPI) {
    if (InterruptId >= GICV5_NUM_PPI_INTERRUPTS) {
      DEBUG ((DEBUG_ERROR, "GicV5EnableInterruptSource: invalid PPI number\n"));
      ASSERT (InterruptId < GICV5_NUM_PPI_INTERRUPTS);
      return EFI_UNSUPPORTED;
    } else if (InterruptId >= 64) {
      UINT64  mask = ArmGicV5GetPpiEnabler1 ();
      mask |= (1ull << (InterruptId - 64));
      ArmGicV5SetPpiEnabler1 (mask);
    } else {
      UINT64  mask = ArmGicV5GetPpiEnabler0 ();
      mask |= (1ull << InterruptId);
      ArmGicV5SetPpiEnabler0 (mask);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "GicV5EnableInterruptSource: invalid interrupt type\n"));
    ASSERT (InterruptType == GICV5_INTERRUPT_TYPE_SPI || InterruptType == GICV5_INTERRUPT_TYPE_PPI);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Disable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_UNSUPPORTED   Invalid interrupt source number.
**/
STATIC
EFI_STATUS
EFIAPI
GicV5DisableInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  UINT32  InterruptType = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINT32  InterruptId   = Source & GICV5_INTERRUPT_ID_MASK;

  if (InterruptType == GICV5_INTERRUPT_TYPE_SPI) {
    ArmGicV5SpiDisable (Source);
  } else if (InterruptType == GICV5_INTERRUPT_TYPE_PPI) {
    if (InterruptId >= GICV5_NUM_PPI_INTERRUPTS) {
      DEBUG ((DEBUG_ERROR, "GicV5DisableInterruptSource: invalid PPI number\n"));
      ASSERT (InterruptId < GICV5_NUM_PPI_INTERRUPTS);
      return EFI_UNSUPPORTED;
    } else if (InterruptId >= 64) {
      UINT64  mask = ArmGicV5GetPpiEnabler1 ();
      mask &= ~(1ull << (InterruptId - 64));
      ArmGicV5SetPpiEnabler1 (mask);
    } else {
      UINT64  mask = ArmGicV5GetPpiEnabler0 ();
      mask &= ~(1ull << InterruptId);
      ArmGicV5SetPpiEnabler0 (mask);
    }
  } else {
    DEBUG ((DEBUG_ERROR, "GicV5DisableInterruptSource: invalid interrupt type\n"));
    ASSERT (InterruptType == GICV5_INTERRUPT_TYPE_SPI || InterruptType == GICV5_INTERRUPT_TYPE_PPI);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Return current state of interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param InterruptState  TRUE: source enabled, FALSE: source disabled.

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_UNSUPPORTED   Invalid interrupt source number.
**/
STATIC
EFI_STATUS
EFIAPI
GicV5GetInterruptSourceState (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN BOOLEAN                          *InterruptState
  )
{
  UINT32  InterruptType = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINT32  InterruptId   = Source & GICV5_INTERRUPT_ID_MASK;

  UINT64  Mask;
  UINT64  Status;

  if (InterruptType == GICV5_INTERRUPT_TYPE_SPI) {
    Status = ArmGicV5ReadInterruptConfig (Source);

    if (Status & 1) {
      ASSERT (!(Status & 1));
      return EFI_UNSUPPORTED;
    }

    *InterruptState = (Status & 4) ? 1 : 0;
  } else if (InterruptType == GICV5_INTERRUPT_TYPE_PPI) {
    if (InterruptId >= GICV5_NUM_PPI_INTERRUPTS) {
      DEBUG ((DEBUG_ERROR, "GicV5GetInterruptSourceState: invalid PPI number\n"));
      ASSERT (InterruptId < GICV5_NUM_PPI_INTERRUPTS);
      return EFI_UNSUPPORTED;
    } else if (InterruptId >= 64) {
      Mask = ArmGicV5GetPpiEnabler1 ();
    } else {
      Mask = ArmGicV5GetPpiEnabler0 ();
    }

    *InterruptState = (Mask >> (InterruptId & 63)) & 1;
  } else {
    DEBUG ((DEBUG_ERROR, "GicV5GetInterruptSourceState: invalid interrupt type\n"));
    ASSERT (InterruptType == GICV5_INTERRUPT_TYPE_SPI || InterruptType == GICV5_INTERRUPT_TYPE_PPI);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Signal to the hardware that the End Of Interrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_UNSUPPORTED   Invalid interrupt source number.
**/
STATIC
EFI_STATUS
EFIAPI
GicV5EndOfInterrupt (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source
  )
{
  UINT32  InterruptType = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINT32  InterruptId   = Source & GICV5_INTERRUPT_ID_MASK;

  if ((InterruptType == GICV5_INTERRUPT_TYPE_PPI) && (InterruptId >= GICV5_NUM_PPI_INTERRUPTS)) {
    DEBUG ((DEBUG_ERROR, "GicV5EndOfInterrupt: invalid PPI number\n"));
    ASSERT (!((InterruptType == GICV5_INTERRUPT_TYPE_PPI) && (InterruptId >= GICV5_NUM_PPI_INTERRUPTS)));
    return EFI_UNSUPPORTED;
  }

  ArmGicV5DeactivateInterrupt (Source);
  ArmGicV5EndOfInterrupt ();

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
GicV5IrqInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINT64                      GicInterrupt;
  HARDWARE_INTERRUPT_HANDLER  InterruptHandler;
  UINT32                      IntId;
  UINT32                      IntType;

  GicInterrupt = ArmGicV5AcknowledgeInterrupt ();
  IntId        = GicInterrupt & GICV5_INTERRUPT_ID_MASK;
  IntType      = GicInterrupt & GICV5_INTERRUPT_TYPE_MASK;

  if (!(GicInterrupt & GICV5_INTERRUPT_ID_VALID)) {
    DEBUG ((DEBUG_ERROR, "Spurious invalid GIC interrupt\n"));
    return;
  }

  if (IntType == GICV5_INTERRUPT_TYPE_SPI) {
    IntId += GICV5_NUM_PPI_INTERRUPTS;
  } else if (IntType != GICV5_INTERRUPT_TYPE_PPI) {
    // Only PPI and SPI are currently supported
    DEBUG ((DEBUG_ERROR, "Unsupported GIC interrupt type\n"));
    GicV5EndOfInterrupt (&mHardwareInterruptV5Protocol, GicInterrupt);
    return;
  }

  InterruptHandler = mRegisteredInterruptHandlers[IntId];
  if (InterruptHandler != NULL) {
    // Call the registered interrupt handler.
    InterruptHandler (GicInterrupt, SystemContext);
  } else {
    DEBUG ((DEBUG_ERROR, "Spurious GIC interrupt: 0x%x\n", (UINT32)IntId));
    GicV5EndOfInterrupt (&mHardwareInterruptV5Protocol, GicInterrupt);
  }
}

STATIC
EFI_STATUS
EFIAPI
GicV5RegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN HARDWARE_INTERRUPT_HANDLER       Handler
  )
{
  // The GICv5 driver supports both PPIs and SPIs. To support both in a single
  // interrupt handler array, map IDs 0 to NUM_PPI_INTERRUPTS-1 as PPIs, and
  // NUM_PPI_INTERRUPTS and above as SPIs.
  UINTN  Type = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINTN  IntId;

  if (Type == GICV5_INTERRUPT_TYPE_PPI) {
    // PPI
    IntId = Source & GICV5_INTERRUPT_ID_MASK;
    if (IntId >= GICV5_NUM_PPI_INTERRUPTS) {
      // GICv5 defines valid PPI IDs as 0-127
      ASSERT (IntId < GICV5_NUM_PPI_INTERRUPTS);
      return EFI_UNSUPPORTED;
    }
  } else if (Type == GICV5_INTERRUPT_TYPE_SPI) {
    // SPI
    IntId = (Source & GICV5_INTERRUPT_ID_MASK) + GICV5_NUM_PPI_INTERRUPTS;
  } else {
    ASSERT (Type == GICV5_INTERRUPT_TYPE_PPI || Type == GICV5_INTERRUPT_TYPE_SPI);
    return EFI_UNSUPPORTED;
  }

  if (IntId >= mGicNumInterrupts) {
    ASSERT (IntId < mGicNumInterrupts);
    return EFI_UNSUPPORTED;
  }

  return GicCommonRegisterInterruptSource (
           This,
           Source,
           Handler,
           &mRegisteredInterruptHandlers[IntId]
           );
}

// The protocol instance produced by this driver
STATIC EFI_HARDWARE_INTERRUPT_PROTOCOL  mHardwareInterruptV5Protocol = {
  GicV5RegisterInterruptSource,
  GicV5EnableInterruptSource,
  GicV5DisableInterruptSource,
  GicV5GetInterruptSourceState,
  GicV5EndOfInterrupt
};

/**
  Select SPI ID to be read or written

  @param InterruptId   Returns interrupt trigger type.

  @retval EFI_SUCCESS       Source interrupt supported.
  @retval EFI_TIMEOUT       Timed out waiting for IRS to go idle.
**/
STATIC
EFI_STATUS
SelectSPIId (
  UINT32  InterruptId
  )
{
  if (!WaitForIrsSpiIdle ()) {
    ASSERT (FALSE);
    return EFI_TIMEOUT;
  }

  MmioWrite32 (mGicIrsConfigFrameBase + GICV5_IRS_SPI_SELR, InterruptId);

  if (!WaitForIrsSpiIdleValid ()) {
    ASSERT (FALSE);
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

/**
  Get interrupt trigger type of an interrupt

  Following completion of this function, the configuration for the
  selected interrupt can be read or written via GICV5_IRS_SPI_CFGR.

  @param This          Instance pointer for this protocol
  @param Source        Hardware source of the interrupt.
  @param TriggerType   Returns interrupt trigger type.

  @retval EFI_SUCCESS       Source interrupt supported.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported.
  @retval EFI_TIMEOUT       Timed out waiting for IRS to go idle.
**/
STATIC
EFI_STATUS
EFIAPI
GicV5GetTriggerType (
  IN  EFI_HARDWARE_INTERRUPT2_PROTOCOL      *This,
  IN  HARDWARE_INTERRUPT_SOURCE             Source,
  OUT EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE  *TriggerType
  )
{
  UINT32      InterruptType = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINT32      InterruptId   = Source & GICV5_INTERRUPT_ID_MASK;
  UINT32      GicTriggerType;
  EFI_STATUS  Status;

  if (InterruptType == GICV5_INTERRUPT_TYPE_SPI) {
    Status = SelectSPIId (InterruptId);

    if (EFI_ERROR (Status)) {
      return Status;
    }

    GicTriggerType = MmioRead32 (mGicIrsConfigFrameBase + GICV5_IRS_SPI_CFGR) & 1;
  } else if (InterruptType == GICV5_INTERRUPT_TYPE_PPI) {
    if (InterruptId >= GICV5_NUM_PPI_INTERRUPTS) {
      DEBUG ((DEBUG_ERROR, "GicV5GetTriggerType: invalid PPI number\n"));
      ASSERT (InterruptId < GICV5_NUM_PPI_INTERRUPTS);
      return EFI_UNSUPPORTED;
    } else if (InterruptId < 64) {
      GicTriggerType = (ArmGicV5GetPPIHMR0 () >> InterruptId) & 1;
    } else {
      GicTriggerType = (ArmGicV5GetPPIHMR1 () >> (InterruptId - 64)) & 1;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "GicV5GetTriggerType: invalid interrupt type\n"));
    ASSERT (InterruptType == GICV5_INTERRUPT_TYPE_SPI || InterruptType == GICV5_INTERRUPT_TYPE_PPI);
    return EFI_UNSUPPORTED;
  }

  if (GicTriggerType == GICV5_PPI_EDGE_TRIGGER) {
    *TriggerType = EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING;
  } else if (GicTriggerType == GICV5_PPI_LEVEL_TRIGGER) {
    *TriggerType = EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH;
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
  @retval EFI_TIMEOUT       Timed out waiting for IRS to go idle.
**/
STATIC
EFI_STATUS
EFIAPI
GicV5SetTriggerType (
  IN  EFI_HARDWARE_INTERRUPT2_PROTOCOL      *This,
  IN  HARDWARE_INTERRUPT_SOURCE             Source,
  IN  EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE  TriggerType
  )
{
  UINT32      InterruptType = Source & GICV5_INTERRUPT_TYPE_MASK;
  UINT32      InterruptId   = Source & GICV5_INTERRUPT_ID_MASK;
  UINT32      GicTriggerType;
  EFI_STATUS  Status;

  if (InterruptType == GICV5_INTERRUPT_TYPE_SPI) {
    if (TriggerType == EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING) {
      GicTriggerType = GICV5_PPI_EDGE_TRIGGER;
    } else if (TriggerType == EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH) {
      GicTriggerType = GICV5_PPI_LEVEL_TRIGGER;
    } else {
      DEBUG ((DEBUG_ERROR, "GicV5SetTriggerType: invalid interrupt type\n"));
      ASSERT (TriggerType == EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING || TriggerType == EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH);
      return EFI_UNSUPPORTED;
    }

    Status = SelectSPIId (InterruptId);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    MmioWrite32 (mGicIrsConfigFrameBase + GICV5_IRS_SPI_CFGR, GicTriggerType);
    if (!WaitForIrsSpiIdle ()) {
      ASSERT (FALSE);
      return EFI_TIMEOUT;
    }
  } else if (InterruptType == GICV5_INTERRUPT_TYPE_PPI) {
    if (InterruptId >= GICV5_NUM_PPI_INTERRUPTS) {
      DEBUG ((DEBUG_ERROR, "GicV5SetTriggerType: invalid PPI number\n"));
      ASSERT (InterruptId < GICV5_NUM_PPI_INTERRUPTS);
      return EFI_UNSUPPORTED;
    } else if (InterruptId < 64) {
      GicTriggerType = (ArmGicV5GetPPIHMR0 () >> InterruptId) & 1;
    } else {
      GicTriggerType = (ArmGicV5GetPPIHMR1 () >> (InterruptId - 64)) & 1;
    }

    if (((GicTriggerType == GICV5_PPI_EDGE_TRIGGER) && (TriggerType != EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING)) ||
        ((GicTriggerType == GICV5_PPI_LEVEL_TRIGGER) && (TriggerType != EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH)))
    {
      // PPI trigger types are fixed on GICv5. Fail on any attempt to change.
      return EFI_UNSUPPORTED;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "GicV5SetTriggerType: invalid interrupt type\n"));
    ASSERT (InterruptType == GICV5_INTERRUPT_TYPE_SPI || InterruptType == GICV5_INTERRUPT_TYPE_PPI);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC EFI_HARDWARE_INTERRUPT2_PROTOCOL  mHardwareInterrupt2V5Protocol = {
  (HARDWARE_INTERRUPT2_REGISTER)GicV5RegisterInterruptSource,
  (HARDWARE_INTERRUPT2_ENABLE)GicV5EnableInterruptSource,
  (HARDWARE_INTERRUPT2_DISABLE)GicV5DisableInterruptSource,
  (HARDWARE_INTERRUPT2_INTERRUPT_STATE)GicV5GetInterruptSourceState,
  (HARDWARE_INTERRUPT2_END_OF_INTERRUPT)GicV5EndOfInterrupt,
  GicV5GetTriggerType,
  GicV5SetTriggerType
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
GicV5ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Index;

  // Acknowledge all pending interrupts
  for (Index = 0; Index < GICV5_NUM_PPI_INTERRUPTS; Index++) {
    GicV5DisableInterruptSource (&mHardwareInterruptV5Protocol, Index | GICV5_INTERRUPT_TYPE_PPI);
  }

  for (Index = 0; Index < SpiRange; Index++) {
    GicV5DisableInterruptSource (&mHardwareInterruptV5Protocol, Index | GICV5_INTERRUPT_TYPE_SPI);
  }

  // Disable Gic Interface
  ArmGicV5DisableInterruptInterface (mGicIrsConfigFrameBase);

  return;
}

/**
  Initialize the state information for the CPU Architectural Protocol

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems
  @retval EFI_UNSUPPORTED       GIC version not supported
**/
EFI_STATUS
GicV5DxeInitialize (
  VOID
  )
{
  EFI_STATUS  Status;

  mGicIrsConfigFrameBase = (UINTN)PcdGet64 (PcdGicIrsConfigFrameBase);

  Status = gCpuArch->SetMemoryAttributes (gCpuArch, mGicIrsConfigFrameBase, GICV5_IRS_CONFIG_FRAME_SIZE, EFI_MEMORY_UC | EFI_MEMORY_XP);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to map GICv5 configuration frame: %r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  SpiRange = MmioRead32 (mGicIrsConfigFrameBase + GICV5_IRS_IDR5) & 0x00ffffff;

  mGicNumInterrupts = GICV5_NUM_PPI_INTERRUPTS + SpiRange;

  mRegisteredInterruptHandlers = AllocateZeroPool (mGicNumInterrupts * sizeof (HARDWARE_INTERRUPT_HANDLER));
  if (mRegisteredInterruptHandlers == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GicCommonInstallAndRegisterInterruptService (
             &mHardwareInterruptV5Protocol,
             &mHardwareInterrupt2V5Protocol,
             GicV5IrqInterruptHandler,
             GicV5ExitBootServicesEvent
             );

  if (Status != EFI_SUCCESS) {
    FreePool (mRegisteredInterruptHandlers);
    return Status;
  }

  ArmGicV5EnableInterruptInterface (mGicIrsConfigFrameBase);

  return EFI_SUCCESS;
}
