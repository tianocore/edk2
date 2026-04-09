/*++

Copyright (c) 2013-2023, Arm Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

--*/

#include "ArmGicDxe.h"
#include "Uefi/UefiBaseType.h"

// Making this global saves a few bytes in image size
EFI_HANDLE  gHardwareInterruptHandle = NULL;

// Notifications
EFI_EVENT  EfiExitBootServicesEvent = (EFI_EVENT)NULL;

EFI_CPU_ARCH_PROTOCOL  *gCpuArch;

/**
 *
 * Return whether the Source interrupt index refers to an extended shared
 * interrupt.
 *
 * @param Source  The source intid to test
 *
 * @return True if Source is an extended SPI intid, false otherwise.
 */
BOOLEAN
GicCommonSourceIsExtSpi (
  IN UINTN  Source
  )
{
  return Source >= ARM_GIC_ARCH_EXT_SPI_MIN && Source <= ARM_GIC_ARCH_EXT_SPI_MAX;
}

/**
 * Return whether this is a special interrupt.
 *
 * @param Source  The source intid to test
 *
 * @return True if Source is a special interrupt intid, false otherwise.
 */
BOOLEAN
GicCommonSourceIsSpecialInterrupt (
  IN UINTN  Source
  )
{
  return (Source >= 1020) && (Source <= 1023);
}

/**
 *
 * Return whether the Source interrupt index refers to a shared interrupt (SPI)
 *
 * @param Source  The source intid to test
 *
 * @return True if Source is a SPI intid, false otherwise.
 */
BOOLEAN
GicCommonSourceIsSpi (
  IN UINTN  Source
  )
{
  return (Source >= ARM_GIC_ARCH_SPI_MIN) && (Source <= ARM_GIC_ARCH_SPI_MAX);
}

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
GicCommonGetDistributorIcfgBaseAndBit (
  IN HARDWARE_INTERRUPT_SOURCE  Source,
  OUT UINTN                     *RegAddress,
  OUT UINTN                     *Config1Bit
  )
{
  UINTN                      RegIndex;
  UINTN                      Field;
  UINTN                      RegOffset;
  HARDWARE_INTERRUPT_SOURCE  AdjustedSource;

  // Translate ESPI sources into the SPI range for indexing purposes.
  AdjustedSource = Source & ~(ARM_GIC_ARCH_EXT_SPI_MIN);

  RegOffset = (GicCommonSourceIsExtSpi (Source)) ? ARM_GIC_ICDICFR_E : ARM_GIC_ICDICFR;

  RegIndex    = AdjustedSource / ARM_GIC_ICDICFR_F_STRIDE; // NOTE: truncation is significant
  Field       = AdjustedSource % ARM_GIC_ICDICFR_F_STRIDE;
  *RegAddress = (UINTN)PcdGet64 (PcdGicDistributorBase)
                + RegOffset
                + (ARM_GIC_ICDICFR_BYTES * RegIndex);
  *Config1Bit = ((Field * ARM_GIC_ICDICFR_F_WIDTH)
                 + ARM_GIC_ICDICFR_F_CONFIG1_BIT);

  return EFI_SUCCESS;
}

/**
  Register Handler for the specified interrupt source.

  @param This        Instance pointer for this protocol
  @param Source      Hardware source of the interrupt
  @param Handler     Callback for interrupt. NULL to unregister
  @param HandlerDest Address of where to store Handler.

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
EFI_STATUS
EFIAPI
GicCommonRegisterInterruptSource (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL  *This,
  IN HARDWARE_INTERRUPT_SOURCE        Source,
  IN HARDWARE_INTERRUPT_HANDLER       Handler,
  IN HARDWARE_INTERRUPT_HANDLER       *HandlerDest
  )
{
  EFI_STATUS  Status;

  if (HandlerDest == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Handler == NULL) && (*HandlerDest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Handler != NULL) && (*HandlerDest != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  if (NULL == Handler) {
    // Removing the handler - disable the interrupt first.
    Status       = This->DisableInterruptSource (This, Source);
    *HandlerDest = Handler;
    return Status;
  } else {
    // Registering a handler - set up the handler before enabling.
    *HandlerDest = Handler;
    return This->EnableInterruptSource (This, Source);
  }
}

EFI_STATUS
GicCommonInstallAndRegisterInterruptService (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL   *InterruptProtocol,
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL  *Interrupt2Protocol,
  IN EFI_CPU_INTERRUPT_HANDLER         InterruptHandler,
  IN EFI_EVENT_NOTIFY                  ExitBootServicesEvent
  )
{
  EFI_STATUS  Status;

  // Register to receive interrupts
  Status = gCpuArch->RegisterInterruptHandler (gCpuArch, ARM_ARCH_EXCEPTION_IRQ, InterruptHandler);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cpu->RegisterInterruptHandler() - %r\n", __func__, Status));
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
