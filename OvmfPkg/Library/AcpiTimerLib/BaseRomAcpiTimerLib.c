/** @file
  Provide constructor and GetTick for BaseRom instance of ACPI Timer Library

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>
#include <OvmfPlatforms.h>

/**
  The constructor function enables ACPI IO space.

  If ACPI I/O space not enabled, this function will enable it.
  It will always return RETURN_SUCCESS.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
AcpiTimerLibConstructor (
  VOID
  )
{
  UINT16 HostBridgeDevId;
  UINTN Pmba;
  UINTN AcpiCtlReg;
  UINT8 AcpiEnBit;

  //
  // Query Host Bridge DID to determine platform type
  //
  HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pmba       = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      AcpiCtlReg = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMREGMISC);
      AcpiEnBit  = PIIX4_PMREGMISC_PMIOSE;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
      AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;
      break;
    default:
      DEBUG ((EFI_D_ERROR, "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__, HostBridgeDevId));
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
  }

  //
  // Check to see if the Power Management Base Address is already enabled
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // If the Power Management Base Address is not programmed,
    // then program the Power Management Base Address from a PCD.
    //
    PciAndThenOr32 (Pmba, (UINT32) ~0xFFC0, PcdGet16 (PcdAcpiPmBaseAddress));

    //
    // Enable PMBA I/O port decodes
    //
    PciOr8 (AcpiCtlReg, AcpiEnBit);
  }

  return RETURN_SUCCESS;
}

/**
  Internal function to read the current tick counter of ACPI.

  Dynamically compute the address of the ACPI tick counter based on the
  properties of the underlying platform, to avoid relying on global variables.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  )
{
  UINT16 HostBridgeDevId;
  UINTN Pmba;

  //
  // Query Host Bridge DID to determine platform type
  //
  HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      break;
    default:
      DEBUG ((EFI_D_ERROR, "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__, HostBridgeDevId));
      ASSERT (FALSE);
      return 0;
  }

  //
  //   Read PMBA to read and return the current ACPI timer value.
  //
  return IoRead32 ((PciRead32 (Pmba) & ~PMBA_RTE) + ACPI_TIMER_OFFSET);
}
