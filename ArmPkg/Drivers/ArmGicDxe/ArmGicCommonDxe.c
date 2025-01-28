/*++

Copyright (c) 2013-2023, Arm Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

--*/

#include "ArmGicDxe.h"

// Making this global saves a few bytes in image size
EFI_HANDLE  gHardwareInterruptHandle = NULL;

// Notifications
EFI_EVENT  EfiExitBootServicesEvent = (EFI_EVENT)NULL;

// Maximum Number of Interrupts
UINTN  mGicNumInterrupts = 0;

HARDWARE_INTERRUPT_HANDLER  *gRegisteredInterruptHandlers = NULL;
EFI_CPU_ARCH_PROTOCOL       *gCpuArch;

/**
  Calculate GICD_ICFGRn base address and corresponding bit
  field Int_config[1] of the GIC distributor register.

  @param Source       Hardware source of the interrupt.
  @param RegAddress   Corresponding GICD_ICFGRn base address.
  @param Config1Bit   Bit number of F Int_config[1] bit in the register.

  @retval EFI_SUCCESS       Source interrupt supported.
  @retval EFI_UNSUPPORTED   Source interrupt is not supported.
**/
EFI_STATUS
GicGetDistributorIcfgBaseAndBit (
  IN HARDWARE_INTERRUPT_SOURCE  Source,
  OUT UINTN                     *RegAddress,
  OUT UINTN                     *Config1Bit
  )
{
  UINTN  RegIndex;
  UINTN  Field;

  if (Source >= mGicNumInterrupts) {
    ASSERT (Source < mGicNumInterrupts);
    return EFI_UNSUPPORTED;
  }

  RegIndex    = Source / ARM_GIC_ICDICFR_F_STRIDE; // NOTE: truncation is significant
  Field       = Source % ARM_GIC_ICDICFR_F_STRIDE;
  *RegAddress = (UINTN)PcdGet64 (PcdGicDistributorBase)
                + ARM_GIC_ICDICFR
                + (ARM_GIC_ICDICFR_BYTES * RegIndex);
  *Config1Bit = ((Field * ARM_GIC_ICDICFR_F_WIDTH)
                 + ARM_GIC_ICDICFR_F_CONFIG1_BIT);

  return EFI_SUCCESS;
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
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN HARDWARE_INTERRUPT_HANDLER       Handler
  )
{
  if (Source >= mGicNumInterrupts) {
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

  // If the interrupt handler is unregistered then disable the interrupt
  if (NULL == Handler) {
    return This->DisableInterruptSource (This, Source);
  } else {
    return This->EnableInterruptSource (This, Source);
  }
}

EFI_STATUS
InstallAndRegisterInterruptService (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL   *InterruptProtocol,
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL  *Interrupt2Protocol,
  IN EFI_CPU_INTERRUPT_HANDLER         InterruptHandler,
  IN EFI_EVENT_NOTIFY                  ExitBootServicesEvent
  )
{
  EFI_STATUS   Status;
  CONST UINTN  RihArraySize =
    (sizeof (HARDWARE_INTERRUPT_HANDLER) * mGicNumInterrupts);

  // Initialize the array for the Interrupt Handlers
  gRegisteredInterruptHandlers = AllocateZeroPool (RihArraySize);
  if (gRegisteredInterruptHandlers == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Register to receive interrupts
  Status = gCpuArch->RegisterInterruptHandler (gCpuArch, ARM_ARCH_EXCEPTION_IRQ, InterruptHandler);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cpu->RegisterInterruptHandler() - %r\n", __func__, Status));
    FreePool (gRegisteredInterruptHandlers);
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gHardwareInterruptHandle,
                  &gHardwareInterruptProtocolGuid,
                  InterruptProtocol,
                  &gHardwareInterrupt2ProtocolGuid,
                  Interrupt2Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  ExitBootServicesEvent,
                  NULL,
                  &EfiExitBootServicesEvent
                  );

  return Status;
}

/**
  Return the GIC CPU Interrupt Interface ID.

  @param GicInterruptInterfaceBase  Base address of the GIC Interrupt Interface.

  @retval CPU Interface Identification information.
**/
UINT32
EFIAPI
ArmGicGetInterfaceIdentification (
  IN  UINTN  GicInterruptInterfaceBase
  )
{
  // Read the GIC Identification Register
  return MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIIDR);
}

UINTN
EFIAPI
ArmGicGetMaxNumInterrupts (
  IN  UINTN  GicDistributorBase
  )
{
  UINTN  ItLines;

  ItLines = MmioRead32 (GicDistributorBase + ARM_GIC_ICDICTR) & 0x1F;

  //
  // Interrupt ID 1020-1023 are reserved.
  //
  return (ItLines == 0x1f) ? 1020 : 32 * (ItLines + 1);
}

VOID
EFIAPI
ArmGicDisableDistributor (
  IN  UINTN  GicDistributorBase
  )
{
  // Disable Gic Distributor
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x0);
}
