/** @file
System On Chip Unit (SOCUnit) routines.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

/** Early initialisation of the SOC Unit

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
SocUnitEarlyInitialisation (
  VOID
  )
{
  UINT32      NewValue;

  //
  // Set the mixer load resistance
  //
  NewValue = QNCPortIORead (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L0);
  NewValue &= OCFGPIMIXLOAD_1_0_MASK;
  QNCPortIOWrite (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L0, NewValue);

  NewValue = QNCPortIORead (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L1);
  NewValue &= OCFGPIMIXLOAD_1_0_MASK;
  QNCPortIOWrite (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L1, NewValue);

  return EFI_SUCCESS;
}

/** Tasks to release PCI controller from reset pre wait for PLL Lock.

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPreWaitPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  UINT32      NewValue;

  //
  // Assert PERST# and validate time assertion time.
  //
  PlatformPERSTAssert (PlatformType);
  ASSERT (PCIEXP_PERST_MIN_ASSERT_US <= (PCIEXP_DELAY_US_POST_CMNRESET_RESET + PCIEXP_DELAY_US_WAIT_PLL_LOCK + PCIEXP_DELAY_US_POST_SBI_RESET));

  //
  // PHY Common lane reset.
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= SOCCLKEN_CONFIG_PHY_I_CMNRESET_L;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  //
  // Wait post common lane reset.
  //
  MicroSecondDelay (PCIEXP_DELAY_US_POST_CMNRESET_RESET);

  //
  // PHY Sideband interface reset.
  // Controller main reset
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= (SOCCLKEN_CONFIG_SBI_RST_100_CORE_B | SOCCLKEN_CONFIG_PHY_I_SIDE_RST_L);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  return EFI_SUCCESS;
}

/** Tasks to release PCI controller from reset after PLL has locked

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPostPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  UINT32 NewValue;

  //
  // Controller sideband interface reset.
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= SOCCLKEN_CONFIG_SBI_BB_RST_B;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  //
  // Wait post sideband interface reset.
  //
  MicroSecondDelay (PCIEXP_DELAY_US_POST_SBI_RESET);

  //
  // Deassert PERST#.
  //
  PlatformPERSTDeAssert (PlatformType);

  //
  // Wait post de assert PERST#.
  //
  MicroSecondDelay (PCIEXP_DELAY_US_POST_PERST_DEASSERT);

  //
  // Controller primary interface reset.
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= SOCCLKEN_CONFIG_BB_RST_B;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  return EFI_SUCCESS;
}

