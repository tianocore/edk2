/** @file
  Provide constructor and GetTick for Dxe instance of ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PlatformInitLib.h>
#include <OvmfPlatforms.h>

//
// Cached ACPI Timer IO Address
//
STATIC UINT32  mAcpiTimerIoAddr;

/**
  The constructor function caches the ACPI tick counter address

  At the time this constructor runs (DXE_CORE or later), ACPI IO space
  has already been enabled by either PlatformPei or by the "Base"
  instance of this library.
  In order to avoid querying the underlying platform type during each
  tick counter read operation, we cache the counter address during
  initialization of this instance of the Timer Library.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
AcpiTimerLibConstructor (
  VOID
  )
{
  UINT16                 HostBridgeDevId;
  UINTN                  Pmba;
  EFI_HOB_GUID_TYPE      *GuidHob;
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob = NULL;

  //
  // Query Host Bridge DID to determine platform type
  // Tdx guest stores the HostBridgePciDevId in a GuidHob.
  // So we first check if this HOB exists
  //
  GuidHob = GetFirstGuidHob (&gUefiOvmfPkgPlatformInfoGuid);
  if (GuidHob != NULL) {
    PlatformInfoHob = (EFI_HOB_PLATFORM_INFO *)GET_GUID_HOB_DATA (GuidHob);
    HostBridgeDevId = PlatformInfoHob->HostBridgeDevId;
  } else {
    DEBUG ((DEBUG_ERROR, "PlatformInfoHob is not found.\n"));
    ASSERT (FALSE);
    return RETURN_UNSUPPORTED;
  }

  switch (HostBridgeDevId) {
    case INTEL_82441_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      break;
    case INTEL_Q35_MCH_DEVICE_ID:
      Pmba = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      break;
    case CLOUDHV_DEVICE_ID:
      mAcpiTimerIoAddr = CLOUDHV_ACPI_TIMER_IO_ADDRESS;
      return RETURN_SUCCESS;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__,
        HostBridgeDevId
        ));
      ASSERT (FALSE);
      return RETURN_UNSUPPORTED;
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
