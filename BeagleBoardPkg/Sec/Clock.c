/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#include <Omap3530/Omap3530.h>

VOID
ClockInit (
  VOID
  )
{
  //DPLL1 - DPLL4 are configured part of Configuration header which OMAP3 ROM parses.

  // Enable PLL5 and set to 120 MHz as a reference clock.
  MmioWrite32 (CM_CLKSEL4_PLL, CM_CLKSEL_PLL_MULT(120) | CM_CLKSEL_PLL_DIV(13));
  MmioWrite32 (CM_CLKSEL5_PLL, CM_CLKSEL_DIV_120M(1));
  MmioWrite32 (CM_CLKEN2_PLL, CM_CLKEN_FREQSEL_075_100 | CM_CLKEN_ENABLE);

  // Turn on functional & interface clocks to the USBHOST power domain
  MmioOr32(CM_FCLKEN_USBHOST, CM_FCLKEN_USBHOST_EN_USBHOST2_ENABLE
                              | CM_FCLKEN_USBHOST_EN_USBHOST1_ENABLE);
  MmioOr32(CM_ICLKEN_USBHOST, CM_ICLKEN_USBHOST_EN_USBHOST_ENABLE);

  // Turn on functional & interface clocks to the USBTLL block.
  MmioOr32(CM_FCLKEN3_CORE, CM_FCLKEN3_CORE_EN_USBTLL_ENABLE);
  MmioOr32(CM_ICLKEN3_CORE, CM_ICLKEN3_CORE_EN_USBTLL_ENABLE);

  // Turn on functional & interface clocks to MMC1 and I2C1 modules.
  MmioOr32(CM_FCLKEN1_CORE, CM_FCLKEN1_CORE_EN_MMC1_ENABLE
                            | CM_FCLKEN1_CORE_EN_I2C1_ENABLE);
  MmioOr32(CM_ICLKEN1_CORE, CM_ICLKEN1_CORE_EN_MMC1_ENABLE
                            | CM_ICLKEN1_CORE_EN_I2C1_ENABLE);

  // Turn on functional & interface clocks to various Peripherals.
  MmioOr32(CM_FCLKEN_PER, CM_FCLKEN_PER_EN_UART3_ENABLE
                          | CM_FCLKEN_PER_EN_GPT3_ENABLE
                          | CM_FCLKEN_PER_EN_GPT4_ENABLE
                          | CM_FCLKEN_PER_EN_GPIO2_ENABLE
                          | CM_FCLKEN_PER_EN_GPIO3_ENABLE
                          | CM_FCLKEN_PER_EN_GPIO4_ENABLE
                          | CM_FCLKEN_PER_EN_GPIO5_ENABLE
                          | CM_FCLKEN_PER_EN_GPIO6_ENABLE);
  MmioOr32(CM_ICLKEN_PER, CM_ICLKEN_PER_EN_UART3_ENABLE
                          | CM_ICLKEN_PER_EN_GPT3_ENABLE
                          | CM_ICLKEN_PER_EN_GPT4_ENABLE
                          | CM_ICLKEN_PER_EN_GPIO2_ENABLE
                          | CM_ICLKEN_PER_EN_GPIO3_ENABLE
                          | CM_ICLKEN_PER_EN_GPIO4_ENABLE
                          | CM_ICLKEN_PER_EN_GPIO5_ENABLE
                          | CM_ICLKEN_PER_EN_GPIO6_ENABLE);

  // Turn on functional & inteface clocks to various wakeup modules.
  MmioOr32(CM_FCLKEN_WKUP, CM_FCLKEN_WKUP_EN_GPIO1_ENABLE
                           | CM_FCLKEN_WKUP_EN_WDT2_ENABLE);
  MmioOr32(CM_ICLKEN_WKUP, CM_ICLKEN_WKUP_EN_GPIO1_ENABLE
                           | CM_ICLKEN_WKUP_EN_WDT2_ENABLE);
}
