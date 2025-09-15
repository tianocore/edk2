/** @file
  Provide constructor and GetTick for Base instance of ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <OvmfPlatforms.h>
#include "AcpiTimerLib.h"
#include "TscTimerLib.h"

//
// Cached ACPI Timer IO Address
//
STATIC UINT32  mAcpiTimerIoAddr;

/**
  The constructor function caches the ACPI tick counter address, and,
  if necessary, enables ACPI IO space.

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
      mAcpiTimerIoAddr =  CLOUDHV_ACPI_TIMER_IO_ADDRESS;
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

  mAcpiTimerIoAddr = (PciRead32 (Pmba) & ~PMBA_RTE) + ACPI_TIMER_OFFSET;
  return RETURN_SUCCESS;
}

/**
  Internal function to read the current tick counter of ACPI.

  Read the current ACPI tick counter using the counter address cached
  by this instance's constructor.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  )
{
  //
  //   Return the current ACPI timer value.
  //
  return IoRead32 (mAcpiTimerIoAddr);
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
  return IsTscTimerFrequencyAvailable () ? TscGetPerformanceCounter () : AcpiGetPerformanceCounter ();
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
  return IsTscTimerFrequencyAvailable () ? TscGetPerformanceCounterProperties (StartValue, EndValue) : AcpiGetPerformanceCounterProperties (StartValue, EndValue);
}
