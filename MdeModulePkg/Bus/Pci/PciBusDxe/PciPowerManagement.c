/** @file
  Power management support functions implementation for PCI Bus module.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PciBus.h"

/**
  This function is intended to turn off PWE assertion and
  put the device to D0 state if the device supports
  PCI Power Management.

  @param PciIoDevice      PCI device instance.

  @retval EFI_UNSUPPORTED PCI Device does not support power management.
  @retval EFI_SUCCESS     Turned off PWE successfully.

**/
EFI_STATUS
ResetPowerManagementFeature (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_STATUS  Status;
  UINT8       PowerManagementRegBlock;
  UINT16      PowerManagementCSR;

  PowerManagementRegBlock = 0;

  Status = LocateCapabilityRegBlock (
             PciIoDevice,
             EFI_PCI_CAPABILITY_ID_PMI,
             &PowerManagementRegBlock,
             NULL
             );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Turn off the PWE assertion and put the device into D0 State
  //

  //
  // Read PMCSR
  //
  Status = PciIoDevice->PciIo.Pci.Read (
                                    &PciIoDevice->PciIo,
                                    EfiPciIoWidthUint16,
                                    PowerManagementRegBlock + 4,
                                    1,
                                    &PowerManagementCSR
                                    );

  if (!EFI_ERROR (Status)) {
    //
    // Clear PME_Status bit
    //
    PowerManagementCSR |= BIT15;
    //
    // Clear PME_En bit. PowerState = D0.
    //
    PowerManagementCSR &= ~(BIT8 | BIT1 | BIT0);

    //
    // Write PMCSR
    //
    Status = PciIoDevice->PciIo.Pci.Write (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint16,
                                      PowerManagementRegBlock + 4,
                                      1,
                                      &PowerManagementCSR
                                      );
  }

  return Status;
}
