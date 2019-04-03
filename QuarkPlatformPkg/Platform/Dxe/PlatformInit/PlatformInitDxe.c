/** @file
Platform init DXE driver for this platform.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Statements that include other files
//
#include "PlatformInitDxe.h"
#include <Library/PciLib.h>
#include <IndustryStandard/Pci.h>

VOID
GetQncName (
  VOID
  )
{
  DEBUG  ((EFI_D_INFO, "QNC Name: "));
  switch (PciRead16 (PCI_LIB_ADDRESS (MC_BUS, MC_DEV, MC_FUN, PCI_DEVICE_ID_OFFSET))) {
  case QUARK_MC_DEVICE_ID:
    DEBUG  ((EFI_D_INFO, "Quark"));
    break;
  case QUARK2_MC_DEVICE_ID:
    DEBUG  ((EFI_D_INFO, "Quark2"));
    break;
  default:
    DEBUG  ((EFI_D_INFO, "Unknown"));
  }

  //
  // Revision
  //
  switch (PciRead8 (PCI_LIB_ADDRESS (MC_BUS, MC_DEV, MC_FUN, PCI_REVISION_ID_OFFSET))) {
  case QNC_MC_REV_ID_A0:
    DEBUG  ((EFI_D_INFO, " - A0 stepping\n"));
    break;
  default:
    DEBUG  ((EFI_D_INFO, " - xx\n"));
  }

  return;
}

EFI_STATUS
EFIAPI
PlatformInit (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
/*++

Routine Description:
  Entry point for the driver.

Arguments:

  ImageHandle  -  Image Handle.
  SystemTable  -  EFI System Table.

Returns:

  EFI_SUCCESS  -  Function has completed successfully.

--*/
{
  EFI_STATUS  Status;

  GetQncName();

  //
  // Create events for configuration callbacks.
  //
  CreateConfigEvents ();

  //
  // Init Platform LEDs.
  //
  Status = PlatformLedInit ((EFI_PLATFORM_TYPE)PcdGet16 (PcdPlatformType));
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

