/** @file
Platform Pcie Helper Lib.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

//
// Routines local to this source module.
//
VOID
LegacyGpioSetLevel (
  IN CONST UINT32                         LevelRegOffset,
  IN CONST UINT32                         GpioNum,
  IN CONST BOOLEAN                        HighLevel
  )
{
  UINT32                            RegValue;
  UINT32                            GpioBaseAddress;
  UINT32                            GpioNumMask;

  GpioBaseAddress =  LpcPciCfg32 (R_QNC_LPC_GBA_BASE) & B_QNC_LPC_GPA_BASE_MASK;
  ASSERT (GpioBaseAddress > 0);

  RegValue = IoRead32 (GpioBaseAddress + LevelRegOffset);
  GpioNumMask = (1 << GpioNum);
  if (HighLevel) {
    RegValue |= (GpioNumMask);
  } else {
    RegValue &= ~(GpioNumMask);
  }
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGLVL_RESUME_WELL, RegValue);
}

//
// Routines exported by this component.
//

/**
  Platform assert PCI express PERST# signal.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

**/
VOID
EFIAPI
PlatformPERSTAssert (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  if (PlatformType == GalileoGen2) {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_GEN2_PCIEXP_PERST_RESUMEWELL_GPIO, FALSE);
  } else {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, PCIEXP_PERST_RESUMEWELL_GPIO, FALSE);
  }
}

/**
  Platform de assert PCI express PERST# signal.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

**/
VOID
EFIAPI
PlatformPERSTDeAssert (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  if (PlatformType == GalileoGen2) {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_GEN2_PCIEXP_PERST_RESUMEWELL_GPIO, TRUE);
  } else {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, PCIEXP_PERST_RESUMEWELL_GPIO, TRUE);
  }
}

/** Early initialisation of the PCIe controller.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
PlatformPciExpressEarlyInit (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{

  //
  // Release and wait for PCI controller to come out of reset.
  //
  SocUnitReleasePcieControllerPreWaitPllLock (PlatformType);
  MicroSecondDelay (PCIEXP_DELAY_US_WAIT_PLL_LOCK);
  SocUnitReleasePcieControllerPostPllLock (PlatformType);

  //
  // Early PCIe initialisation
  //
  SocUnitEarlyInitialisation ();

  //
  // Do North cluster early PCIe init.
  //
  PciExpressEarlyInit ();

  return EFI_SUCCESS;
}

