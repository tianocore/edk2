/** @file
*
*  Copyright (c) 2011-2023, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmGicLib.h>

#include "ArmGicDxe.h"
#include "Base.h"
#include "Protocol/HardwareInterrupt.h"
#include "Uefi/UefiBaseType.h"

#define ARM_GIC_DEFAULT_PRIORITY  0x80

// In GICv3, there are 2 x 64KB frames:
// Redistributor control frame + SGI Control & Generation frame
#define GIC_V3_REDISTRIBUTOR_GRANULARITY  (ARM_GICR_CTLR_FRAME_SIZE           \
                                           + ARM_GICR_SGI_PPI_FRAME_SIZE)

// In GICv4, there are 2 additional 64KB frames:
// VLPI frame + Reserved page frame
#define GIC_V4_REDISTRIBUTOR_GRANULARITY  (GIC_V3_REDISTRIBUTOR_GRANULARITY   \
                                           + ARM_GICR_SGI_VLPI_FRAME_SIZE     \
                                           + ARM_GICR_SGI_RESERVED_FRAME_SIZE)

#define GICD_V3_SIZE  SIZE_64KB

extern EFI_HARDWARE_INTERRUPT_PROTOCOL   gHardwareInterruptV3Protocol;
extern EFI_HARDWARE_INTERRUPT2_PROTOCOL  gHardwareInterrupt2V3Protocol;

STATIC UINTN  mGicDistributorBase;
STATIC UINTN  mGicRedistributorBase;

STATIC HARDWARE_INTERRUPT_HANDLER  *mRegisteredInterruptHandlers;
STATIC UINTN                       mGicMaxSpiIntId;
STATIC UINTN                       mGicMaxExtSpiIntId;

/**
 * Return the base address of the GIC redistributor for the current CPU
 *
 * @retval Base address of the associated GIC Redistributor
 */
STATIC
UINTN
GicGetCpuRedistributorBase (
  IN UINTN  GicRedistributorBase
  )
{
  UINTN       MpId;
  UINTN       CpuAffinity;
  UINTN       Affinity;
  UINTN       GicCpuRedistributorBase;
  UINT64      TypeRegister;
  EFI_STATUS  Status;

  MpId = ArmReadMpidr ();
  // Define CPU affinity as:
  // Affinity0[0:8], Affinity1[9:15], Affinity2[16:23], Affinity3[24:32]
  // whereas Affinity3 is defined at [32:39] in MPIDR
  CpuAffinity = (MpId & (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2)) |
                ((MpId & ARM_CORE_AFF3) >> 8);

  GicCpuRedistributorBase = GicRedistributorBase;

  do {
    Status = gCpuArch->SetMemoryAttributes (
                         gCpuArch,
                         GicCpuRedistributorBase,
                         GIC_V3_REDISTRIBUTOR_GRANULARITY,
                         EFI_MEMORY_UC | EFI_MEMORY_XP
                         );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to map GICv3 redistributor MMIO interface at 0x%lx: %r\n",
        __func__,
        GicCpuRedistributorBase,
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      return 0;
    }

    TypeRegister = MmioRead64 (GicCpuRedistributorBase + ARM_GICR_TYPER);
    Affinity     = ARM_GICR_TYPER_GET_AFFINITY (TypeRegister);
    if (Affinity == CpuAffinity) {
      return GicCpuRedistributorBase;
    }

    // Move to the next GIC Redistributor frame.
    // The GIC specification does not forbid a mixture of redistributors
    // with or without support for virtual LPIs, so we test Virtual LPIs
    // Support (VLPIS) bit for each frame to decide the granularity.
    // Note: The assumption here is that the redistributors are adjacent
    // for all CPUs. However this may not be the case for NUMA systems.
    GicCpuRedistributorBase += (((ARM_GICR_TYPER_VLPIS & TypeRegister) != 0)
                                ? GIC_V4_REDISTRIBUTOR_GRANULARITY
                                : GIC_V3_REDISTRIBUTOR_GRANULARITY);
  } while ((TypeRegister & ARM_GICR_TYPER_LAST) == 0);

  // The Redistributor has not been found for the current CPU
  ASSERT_EFI_ERROR (EFI_NOT_FOUND);
  return 0;
}

typedef enum {
  GicOpSetInterruptPriority,
  GicOpEnableInterrupt,
  GicOpDisableInterrupt,
  GicOpIsInterruptEnabled,
  GicOpNumOps,
} GIC_REG_OPERATIONS;

typedef struct {
  GIC_REG_OPERATIONS    Operation;               // Self-referential operation.
  UINT16                RegisterWidthBits;       // Width of the register. Same for all types.
  UINT16                FieldWidthBits;          // Bits per IntId within the register.
  UINTN                 DistributorSpiOffset;    // Offset from the GIC distributor base for SPIs.
  UINTN                 DistributorExtSpiOffset; // Offset from the GIC distributor base for Extended SPIs.
  UINTN                 RedistributorOffset;     // Offset from the GIC redistributor base for non-shared interrupts.
} GIC_REG_OPERATION_CONFIG;

// Order of the array is important as mGicOperationConfigMap is indexed by
// GIC_REG_OPERATIONS.
STATIC CONST GIC_REG_OPERATION_CONFIG  mGicOperationConfigMap[GicOpNumOps] = {
  [GicOpSetInterruptPriority] = {
    .Operation               = GicOpSetInterruptPriority,
    .RegisterWidthBits       = 32,
    .FieldWidthBits          = 8,
    .DistributorSpiOffset    = ARM_GIC_ICDIPR,
    .DistributorExtSpiOffset = ARM_GIC_ICDIPR_E,
    .RedistributorOffset     = ARM_GIC_ICDIPR,
  },
  [GicOpEnableInterrupt] =      {
    .Operation               = GicOpEnableInterrupt,
    .RegisterWidthBits       = 32,
    .FieldWidthBits          = 1,
    .DistributorSpiOffset    = ARM_GIC_ICDISER,
    .DistributorExtSpiOffset = ARM_GIC_ICDISER_E,
    .RedistributorOffset     = ARM_GICR_ISENABLER,
  },
  [GicOpDisableInterrupt] =     {
    .Operation               = GicOpDisableInterrupt,
    .RegisterWidthBits       = 32,
    .FieldWidthBits          = 1,
    .DistributorSpiOffset    = ARM_GIC_ICDICER,
    .DistributorExtSpiOffset = ARM_GIC_ICDICER_E,
    .RedistributorOffset     = ARM_GICR_ICENABLER,
  },
  [GicOpIsInterruptEnabled] =   {
    .Operation               = GicOpIsInterruptEnabled,
    .RegisterWidthBits       = 32,
    .FieldWidthBits          = 1,
    .DistributorSpiOffset    = ARM_GIC_ICDISER,
    .DistributorExtSpiOffset = ARM_GIC_ICDISER_E,
    .RedistributorOffset     = ARM_GICR_ISENABLER,
  },
};

// Struct describing a bitwise address in the GIC.
typedef struct {
  UINTN     Address; // MMIO address for the register.
  UINT32    Shift;   // Position in the register of the lowest included bit.
  UINT32    Mask;    // Mask of the included bits stating at the position of Shift.
} GIC_BITWISE_ADDRESS;

/**
 * Gets a bitwise address for the given intid Source and Operation.
 *
 * A bitwise address contains all of the information to extract the relevant
 * field from a MMIO register, such as the position and bitmask for the field.
 *
 *  @param [in]  Source         Hardware source of the interrupt
 *  @param [in]  Operation      GIC_REG_OPERATION requested for this interrupt.
 *  @param [out] BitwiseAddress Fully-specified address and mask to populate.
**/
STATIC
VOID
EFIAPI
GicGetBitwiseAddress (
  IN UINTN                 Source,
  GIC_REG_OPERATIONS       Operation,
  OUT GIC_BITWISE_ADDRESS  *BitwiseAddress
  )
{
  UINT32                          RegOffset;
  UINT32                          IntIdsPerRegister;
  CONST GIC_REG_OPERATION_CONFIG  *OpConfig;

  ASSERT (BitwiseAddress != NULL);
  ASSERT (Operation < GicOpNumOps);

  OpConfig          = &mGicOperationConfigMap[Operation];
  IntIdsPerRegister = (UINT32)(OpConfig->RegisterWidthBits / OpConfig->FieldWidthBits);

  BitwiseAddress->Shift = (UINT32)((Source & ~ARM_GIC_ARCH_EXT_SPI_MIN) % IntIdsPerRegister) * OpConfig->FieldWidthBits;
  BitwiseAddress->Mask  = (UINT32)(((UINT64)1 << OpConfig->FieldWidthBits) - 1);

  RegOffset = (UINT32)((Source & ~ARM_GIC_ARCH_EXT_SPI_MIN) / IntIdsPerRegister);
  if (GicCommonSourceIsSpi (Source)) {
    BitwiseAddress->Address = mGicDistributorBase + OpConfig->DistributorSpiOffset + (4 * RegOffset);
  } else if (GicCommonSourceIsExtSpi (Source)) {
    BitwiseAddress->Address = mGicDistributorBase + OpConfig->DistributorExtSpiOffset + (4 * RegOffset);
  } else {
    BitwiseAddress->Address = mGicRedistributorBase + ARM_GICR_CTLR_FRAME_SIZE + OpConfig->RedistributorOffset + (4 * RegOffset);
  }
}

STATIC
VOID
ArmGicSetInterruptPriority (
  IN UINTN   Source,
  IN UINT32  Priority
  )
{
  GIC_BITWISE_ADDRESS  BitwiseAddress;

  GicGetBitwiseAddress (Source, GicOpSetInterruptPriority, &BitwiseAddress);

  Priority &= BitwiseAddress.Mask;
  MmioAndThenOr32 (
    BitwiseAddress.Address,
    ~(BitwiseAddress.Mask << BitwiseAddress.Shift),
    Priority << BitwiseAddress.Shift
    );
}

STATIC
VOID
ArmGicEnableInterrupt (
  IN UINTN  Source
  )
{
  GIC_BITWISE_ADDRESS  BitwiseAddress;

  GicGetBitwiseAddress (Source, GicOpEnableInterrupt, &BitwiseAddress);

  // Write set-enable register.
  MmioWrite32 (
    BitwiseAddress.Address,
    BitwiseAddress.Mask << BitwiseAddress.Shift
    );
}

STATIC
VOID
ArmGicDisableInterrupt (
  IN UINTN  Source
  )
{
  GIC_BITWISE_ADDRESS  BitwiseAddress;

  GicGetBitwiseAddress (Source, GicOpDisableInterrupt, &BitwiseAddress);

  // Write clear-enable register
  MmioWrite32 (
    BitwiseAddress.Address,
    BitwiseAddress.Mask << BitwiseAddress.Shift
    );
}

STATIC
BOOLEAN
ArmGicIsInterruptEnabled (
  IN UINTN  Source
  )
{
  UINT32               Interrupts;
  GIC_BITWISE_ADDRESS  BitwiseAddress;

  GicGetBitwiseAddress (Source, GicOpIsInterruptEnabled, &BitwiseAddress);

  // Read set-enable register
  Interrupts = MmioRead32 (BitwiseAddress.Address);

  return ((Interrupts & (BitwiseAddress.Mask << BitwiseAddress.Shift)) != 0);
}

STATIC
BOOLEAN
EFIAPI
GicIsValidSource (
  IN HARDWARE_INTERRUPT_SOURCE  Source
  )
{
  if (Source <= mGicMaxSpiIntId) {
    return TRUE;
  }

  return (Source >= ARM_GIC_ARCH_EXT_SPI_MIN && Source <= mGicMaxExtSpiIntId);
}

STATIC
EFI_STATUS
EFIAPI
GicV3GetValidIntIdRanges (
  IN UINTN   GicDistributorBase,
  OUT UINTN  *GicMaxSpiIntId,
  OUT UINTN  *GicMaxExtSpiIntId
  )
{
  UINT32  GicTyperReg;

  if ((GicMaxSpiIntId == NULL) || (GicMaxExtSpiIntId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Read GIC_TYPER to determine if extended SPI is enabled. If so, get the
  // maximum extended SPI INTID.
  GicTyperReg     = MmioRead32 (GicDistributorBase + ARM_GIC_ICDICTR);
  *GicMaxSpiIntId = ARM_GIC_ICDICTR_GET_SPI_MAX_INTID (GicTyperReg);

  if ((GicTyperReg & ARM_GIC_ICDICTR_EXT_SPI_ENABLED) != 0) {
    *GicMaxExtSpiIntId = ARM_GIC_ICDICTR_GET_EXT_SPI_MAX_INTID (GicTyperReg);
  } else {
    *GicMaxExtSpiIntId = 0;
  }

  return EFI_SUCCESS;
}

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
  if (!GicIsValidSource (Source)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicEnableInterrupt (Source);

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
  if (!GicIsValidSource (Source)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicDisableInterrupt (Source);

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
  if (!GicIsValidSource (Source)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  *InterruptState = ArmGicIsInterruptEnabled (Source);

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
  if (!GicIsValidSource (Source)) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ArmGicV3EndOfInterrupt (Source);
  return EFI_SUCCESS;
}

/**
  Gets the address of the interrupt callback associated with Source.

  @param  Source    IntId of the interrupt.

  @return Pointer to the associated handler
          NULL if the source is invalid.
**/
STATIC
HARDWARE_INTERRUPT_HANDLER *
EFIAPI
GicV3GetHandlerAddress (
  IN HARDWARE_INTERRUPT_SOURCE  Source
  )
{
  if (!GicIsValidSource (Source)) {
    return NULL;
  }

  if (GicCommonSourceIsExtSpi (Source)) {
    return &mRegisteredInterruptHandlers[
                                         Source - ARM_GIC_ARCH_EXT_SPI_MIN + mGicMaxSpiIntId + 1
    ];
  }

  return &mRegisteredInterruptHandlers[Source];
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
  UINTN                       GicInterrupt;
  HARDWARE_INTERRUPT_HANDLER  *InterruptHandlerPtr;

  GicInterrupt = ArmGicV3AcknowledgeInterrupt ();

  if (GicCommonSourceIsSpecialInterrupt (GicInterrupt)) {
    // The special interrupts do not need to be acknowledged.
    return;
  }

  InterruptHandlerPtr = GicV3GetHandlerAddress (GicInterrupt);
  if (InterruptHandlerPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Interrupt 0x%x out of expected range\n", GicInterrupt));
    return;
  }

  if (*InterruptHandlerPtr != NULL) {
    // Call the registered interrupt handler.
    (*InterruptHandlerPtr)(GicInterrupt, SystemContext);
  } else {
    DEBUG ((DEBUG_ERROR, "Spurious GIC interrupt: 0x%x\n", (UINT32)GicInterrupt));
    GicV3EndOfInterrupt (&gHardwareInterruptV3Protocol, GicInterrupt);
  }
}

EFI_STATUS
EFIAPI
GicV3RegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN HARDWARE_INTERRUPT_HANDLER       Handler
  )
{
  HARDWARE_INTERRUPT_HANDLER  *HandlerDest = GicV3GetHandlerAddress (Source);

  if (HandlerDest == NULL) {
    return EFI_UNSUPPORTED;
  }

  return GicCommonRegisterInterruptSource (
           This,
           Source,
           Handler,
           HandlerDest
           );
}

// The protocol instance produced by this driver
EFI_HARDWARE_INTERRUPT_PROTOCOL  gHardwareInterruptV3Protocol = {
  GicV3RegisterInterruptSource,
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

  if (!GicIsValidSource (Source)) {
    return EFI_UNSUPPORTED;
  }

  Status = GicCommonGetDistributorIcfgBaseAndBit (
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

  if (!GicIsValidSource (Source)) {
    return EFI_UNSUPPORTED;
  }

  Status = GicCommonGetDistributorIcfgBaseAndBit (
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

STATIC
VOID
ArmGicEnableDistributor (
  IN  UINTN  GicDistributorBase
  )
{
  UINT32  GicDistributorCtl;

  GicDistributorCtl = MmioRead32 (GicDistributorBase + ARM_GIC_ICDDCR);
  if ((GicDistributorCtl & ARM_GIC_ICDDCR_ARE) != 0) {
    MmioOr32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x2);
  } else {
    MmioOr32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x1);
  }
}

EFI_HARDWARE_INTERRUPT2_PROTOCOL  gHardwareInterrupt2V3Protocol = {
  (HARDWARE_INTERRUPT2_REGISTER)GicV3RegisterInterruptSource,
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

  // Acknowledge all pending interrupts in SPI range.
  for (Index = 0; Index <= mGicMaxSpiIntId; Index++) {
    GicV3DisableInterruptSource (&gHardwareInterruptV3Protocol, Index);
  }

  // Acknowledge all pending interrupts in extended SPI range.
  for (Index = ARM_GIC_ARCH_EXT_SPI_MIN; Index <= mGicMaxExtSpiIntId; Index++) {
    GicV3DisableInterruptSource (&gHardwareInterruptV3Protocol, Index);
  }

  // Disable Gic Interface
  ArmGicV3DisableInterruptInterface ();

  // Disable Gic Distributor
  ArmGicDisableDistributor (mGicDistributorBase);
}

#ifdef MDE_CPU_ARM

/**
  Dummy GICv5 init funtion to avoid symbol resolution failure on ARM,
  which cannot support GICv5.
 **/
EFI_STATUS
GicV5DxeInitialize (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

#endif

/**
  Invoke the appropriate initializer for the CPU Architectural Protocol

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table
**/
EFI_STATUS
GicDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Make sure the Interrupt Controller Protocol is not already installed in
  // the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gHardwareInterruptProtocolGuid);

  // Locate the CPU arch protocol - cannot fail because of DEPEX
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gCpuArch);
  ASSERT_EFI_ERROR (Status);

  if (ArmHasGicV5SystemRegisters ()) {
    return GicV5DxeInitialize ();
  }

  return GicV3DxeInitialize ();
}

/**
  Initialize the state information for the CPU Architectural Protocol

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
GicV3DxeInitialize (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT64      MpId;
  UINT64      CpuTarget;
  UINT64      RegValue;
  UINT64      TotalIntIds;

  mGicDistributorBase = (UINTN)PcdGet64 (PcdGicDistributorBase);

  Status = gCpuArch->SetMemoryAttributes (gCpuArch, mGicDistributorBase, GICD_V3_SIZE, EFI_MEMORY_UC | EFI_MEMORY_XP);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to map GICv3 distributor MMIO interface: %r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  mGicRedistributorBase = GicGetCpuRedistributorBase (PcdGet64 (PcdGicRedistributorsBase));

  RegValue = ArmGicV3GetControlSystemRegisterEnable ();
  if ((RegValue & ICC_SRE_EL2_SRE) == 0) {
    ArmGicV3SetControlSystemRegisterEnable (RegValue | ICC_SRE_EL2_SRE);
    ASSERT ((ArmGicV3GetControlSystemRegisterEnable () & ICC_SRE_EL2_SRE) != 0);
  }

  Status = GicV3GetValidIntIdRanges (
             mGicDistributorBase,
             &mGicMaxSpiIntId,
             &mGicMaxExtSpiIntId
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // We will be driving this GIC in native v3 mode, i.e., with Affinity
  // Routing enabled. So ensure that the ARE bit is set.
  MmioOr32 (mGicDistributorBase + ARM_GIC_ICDDCR, ARM_GIC_ICDDCR_ARE);

  // Disable all SPI sources.
  for (Index = 0; Index <= mGicMaxSpiIntId; Index++) {
    GicV3DisableInterruptSource (&gHardwareInterruptV3Protocol, Index);

    // Set Priority
    ArmGicSetInterruptPriority (Index, ARM_GIC_DEFAULT_PRIORITY);
  }

  // Disable all extended SPI sources.
  for (Index = ARM_GIC_ARCH_EXT_SPI_MIN; Index <= mGicMaxExtSpiIntId; Index++) {
    GicV3DisableInterruptSource (&gHardwareInterruptV3Protocol, Index);

    // Set Priority
    ArmGicSetInterruptPriority (Index, ARM_GIC_DEFAULT_PRIORITY);
  }

  // Targets the interrupts to the Primary Cpu

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
      mGicRedistributorBase + ARM_GICR_CTLR_FRAME_SIZE + ARM_GIC_ICDISR,
      0xffffffff
      );

    // Configure SPI
    for (Index = 32; Index <= mGicMaxSpiIntId; Index += 32) {
      MmioWrite32 (
        mGicDistributorBase + ARM_GIC_ICDISR + Index / 8,
        0xffffffff
        );
    }

    // Configure extended SPI
    for (Index = ARM_GIC_ARCH_EXT_SPI_MIN; Index <= mGicMaxExtSpiIntId; Index += 32) {
      MmioWrite32 (
        mGicDistributorBase + ARM_GIC_ICDISR_E + (Index - ARM_GIC_ARCH_EXT_SPI_MIN) / 8,
        0xffffffff
        );
    }

    // Route the SPIs to the primary CPU. SPIs start at the INTID 32
    for (Index = 0; Index < (mGicMaxSpiIntId + 1 - 32); Index++) {
      MmioWrite64 (
        mGicDistributorBase + ARM_GICD_IROUTER + (Index * 8),
        CpuTarget
        );
    }

    // Route the extended SPIs to the primary CPU. extended SPIs start at the INTID 4096
    for (Index = ARM_GIC_ARCH_EXT_SPI_MIN; Index <= mGicMaxExtSpiIntId; Index++) {
      MmioWrite64 (
        mGicDistributorBase + ARM_GICD_IROUTER_E + ((Index - ARM_GIC_ARCH_EXT_SPI_MIN) * 8),
        CpuTarget
        );
    }
  }

  // Set binary point reg to 0x7 (no preemption)
  ArmGicV3SetBinaryPointer (0x7);

  // Set priority mask reg to 0xff to allow all priorities through
  ArmGicV3SetPriorityMask (0xff);

  // Use combined priority drop and deactivate (EOImode == 0)
  RegValue  = ArmGicV3GetControlRegister ();
  RegValue &= ~(UINT64)ICC_CTLR_EOImode;
  ArmGicV3SetControlRegister (RegValue);

  // Enable gic cpu interface
  ArmGicV3EnableInterruptInterface ();

  // Enable gic distributor
  ArmGicEnableDistributor (mGicDistributorBase);

  // Allocate contiguous handlers for SPI and extended SPI. extended SPI
  // handlers begin immediately after SPI handlers.
  TotalIntIds = mGicMaxSpiIntId + 1;
  if (mGicMaxExtSpiIntId >= ARM_GIC_ARCH_EXT_SPI_MIN) {
    TotalIntIds += (mGicMaxExtSpiIntId - ARM_GIC_ARCH_EXT_SPI_MIN + 1);
  }

  mRegisteredInterruptHandlers = AllocateZeroPool (
                                   sizeof (HARDWARE_INTERRUPT_HANDLER) *
                                   TotalIntIds
                                   );
  if (mRegisteredInterruptHandlers == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GicCommonInstallAndRegisterInterruptService (
             &gHardwareInterruptV3Protocol,
             &gHardwareInterrupt2V3Protocol,
             GicV3IrqInterruptHandler,
             GicV3ExitBootServicesEvent
             );

  if (EFI_ERROR (Status)) {
    FreePool (mRegisteredInterruptHandlers);
  }

  return Status;
}
