/** @file
  Provide constructor and GetTick for BaseRom instance of ACPI Timer Library

  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <OvmfPlatforms.h>
#include "AcpiTimerLib.h"

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
  UINT16  HostBridgeDevId;
  UINTN   Pmba;
  UINT32  PmbaAndVal;
  UINT32  PmbaOrVal;
  UINTN   AcpiCtlReg;
  UINT8   AcpiEnBit;

  //
  // Query Host Bridge DID to determine platform type
  //
  HostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pmba       = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      PmbaAndVal = ~(UINT32)PIIX4_PMBA_MASK;
      PmbaOrVal  = PIIX4_PMBA_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMREGMISC);
      AcpiEnBit  = PIIX4_PMREGMISC_PMIOSE;
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      PmbaAndVal = ~(UINT32)ICH9_PMBASE_MASK;
      PmbaOrVal  = ICH9_PMBASE_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
      AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;
      break;
    case CLOUDHV_DEVICE_ID:
      return RETURN_SUCCESS;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __func__,
        HostBridgeDevId
        ));
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
  }

  //
  // Check to see if the Power Management Base Address is already enabled
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // If the Power Management Base Address is not programmed,
    // then program it now.
    //
    PciAndThenOr32 (Pmba, PmbaAndVal, PmbaOrVal);

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
  UINT16  HostBridgeDevId;
  UINTN   Pmba;

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
    case CLOUDHV_DEVICE_ID:
      return IoRead32 (CLOUDHV_ACPI_TIMER_IO_ADDRESS);
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __func__,
        HostBridgeDevId
        ));
      ASSERT (FALSE);
      return 0;
  }

  //
  //   Read PMBA to read and return the current ACPI timer value.
  //
  return IoRead32 ((PciRead32 (Pmba) & ~PMBA_RTE) + ACPI_TIMER_OFFSET);
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return AcpiGetPerformanceCounter ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64  *StartValue   OPTIONAL,
  OUT      UINT64  *EndValue     OPTIONAL
  )
{
  return AcpiGetPerformanceCounterProperties (StartValue, EndValue);
}
