/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3530PRCM_H__
#define __OMAP3530PRCM_H__

#define CM_FCLKEN1_CORE   (0x48004A00)
#define CM_FCLKEN3_CORE   (0x48004A08)
#define CM_ICLKEN1_CORE   (0x48004A10)
#define CM_ICLKEN3_CORE   (0x48004A18)
#define CM_CLKEN2_PLL     (0x48004D04)
#define CM_CLKSEL4_PLL    (0x48004D4C)
#define CM_CLKSEL5_PLL    (0x48004D50)
#define CM_FCLKEN_USBHOST  (0x48005400)
#define CM_ICLKEN_USBHOST  (0x48005410)
#define CM_CLKSTST_USBHOST (0x4800544c)

//Wakeup clock defintion
#define CM_FCLKEN_WKUP    (0x48004C00)
#define CM_ICLKEN_WKUP    (0x48004C10)

//Peripheral clock definition
#define CM_FCLKEN_PER     (0x48005000)
#define CM_ICLKEN_PER     (0x48005010)
#define CM_CLKSEL_PER     (0x48005040)

//Reset management definition
#define PRM_RSTCTRL       (0x48307250)
#define PRM_RSTST         (0x48307258)

//CORE clock
#define CM_FCLKEN1_CORE_EN_I2C1_MASK    BIT15
#define CM_FCLKEN1_CORE_EN_I2C1_DISABLE (0UL << 15)
#define CM_FCLKEN1_CORE_EN_I2C1_ENABLE  BIT15

#define CM_ICLKEN1_CORE_EN_I2C1_MASK    BIT15
#define CM_ICLKEN1_CORE_EN_I2C1_DISABLE (0UL << 15)
#define CM_ICLKEN1_CORE_EN_I2C1_ENABLE  BIT15

#define CM_FCLKEN1_CORE_EN_MMC1_MASK    BIT24
#define CM_FCLKEN1_CORE_EN_MMC1_DISABLE (0UL << 24)
#define CM_FCLKEN1_CORE_EN_MMC1_ENABLE  BIT24

#define CM_FCLKEN3_CORE_EN_USBTLL_MASK    BIT2
#define CM_FCLKEN3_CORE_EN_USBTLL_DISABLE (0UL << 2)
#define CM_FCLKEN3_CORE_EN_USBTLL_ENABLE  BIT2

#define CM_ICLKEN1_CORE_EN_MMC1_MASK    BIT24
#define CM_ICLKEN1_CORE_EN_MMC1_DISABLE (0UL << 24)
#define CM_ICLKEN1_CORE_EN_MMC1_ENABLE  BIT24

#define CM_ICLKEN3_CORE_EN_USBTLL_MASK    BIT2
#define CM_ICLKEN3_CORE_EN_USBTLL_DISABLE (0UL << 2)
#define CM_ICLKEN3_CORE_EN_USBTLL_ENABLE  BIT2

#define CM_CLKEN_FREQSEL_075_100        (0x03UL << 4)
#define CM_CLKEN_ENABLE                 (7UL << 0)

#define CM_CLKSEL_PLL_MULT(x)           (((x) & 0x07FF) << 8)
#define CM_CLKSEL_PLL_DIV(x)            ((((x) - 1) & 0x7F) << 0)

#define CM_CLKSEL_DIV_120M(x)           (((x) & 0x1F) << 0)

#define CM_FCLKEN_USBHOST_EN_USBHOST2_MASK    BIT1
#define CM_FCLKEN_USBHOST_EN_USBHOST2_DISABLE (0UL << 1)
#define CM_FCLKEN_USBHOST_EN_USBHOST2_ENABLE  BIT1

#define CM_FCLKEN_USBHOST_EN_USBHOST1_MASK    BIT0
#define CM_FCLKEN_USBHOST_EN_USBHOST1_DISABLE (0UL << 0)
#define CM_FCLKEN_USBHOST_EN_USBHOST1_ENABLE  BIT0

#define CM_ICLKEN_USBHOST_EN_USBHOST_MASK     BIT0
#define CM_ICLKEN_USBHOST_EN_USBHOST_DISABLE  (0UL << 0)
#define CM_ICLKEN_USBHOST_EN_USBHOST_ENABLE   BIT0

//Wakeup functional clock
#define CM_FCLKEN_WKUP_EN_GPIO1_DISABLE       (0UL << 3)
#define CM_FCLKEN_WKUP_EN_GPIO1_ENABLE        BIT3

#define CM_FCLKEN_WKUP_EN_WDT2_DISABLE        (0UL << 5)
#define CM_FCLKEN_WKUP_EN_WDT2_ENABLE         BIT5

//Wakeup interface clock
#define CM_ICLKEN_WKUP_EN_GPIO1_DISABLE       (0UL << 3)
#define CM_ICLKEN_WKUP_EN_GPIO1_ENABLE        BIT3

#define CM_ICLKEN_WKUP_EN_WDT2_DISABLE        (0UL << 5)
#define CM_ICLKEN_WKUP_EN_WDT2_ENABLE         BIT5

//Peripheral functional clock
#define CM_FCLKEN_PER_EN_GPT3_DISABLE         (0UL << 4)
#define CM_FCLKEN_PER_EN_GPT3_ENABLE          BIT4

#define CM_FCLKEN_PER_EN_GPT4_DISABLE         (0UL << 5)
#define CM_FCLKEN_PER_EN_GPT4_ENABLE          BIT5

#define CM_FCLKEN_PER_EN_UART3_DISABLE        (0UL << 11)
#define CM_FCLKEN_PER_EN_UART3_ENABLE         BIT11

#define CM_FCLKEN_PER_EN_GPIO2_DISABLE        (0UL << 13)
#define CM_FCLKEN_PER_EN_GPIO2_ENABLE         BIT13

#define CM_FCLKEN_PER_EN_GPIO3_DISABLE        (0UL << 14)
#define CM_FCLKEN_PER_EN_GPIO3_ENABLE         BIT14

#define CM_FCLKEN_PER_EN_GPIO4_DISABLE        (0UL << 15)
#define CM_FCLKEN_PER_EN_GPIO4_ENABLE         BIT15

#define CM_FCLKEN_PER_EN_GPIO5_DISABLE        (0UL << 16)
#define CM_FCLKEN_PER_EN_GPIO5_ENABLE         BIT16

#define CM_FCLKEN_PER_EN_GPIO6_DISABLE        (0UL << 17)
#define CM_FCLKEN_PER_EN_GPIO6_ENABLE         BIT17

//Peripheral interface clock
#define CM_ICLKEN_PER_EN_GPT3_DISABLE         (0UL << 4)
#define CM_ICLKEN_PER_EN_GPT3_ENABLE          BIT4

#define CM_ICLKEN_PER_EN_GPT4_DISABLE         (0UL << 5)
#define CM_ICLKEN_PER_EN_GPT4_ENABLE          BIT5

#define CM_ICLKEN_PER_EN_UART3_DISABLE        (0UL << 11)
#define CM_ICLKEN_PER_EN_UART3_ENABLE         BIT11

#define CM_ICLKEN_PER_EN_GPIO2_DISABLE        (0UL << 13)
#define CM_ICLKEN_PER_EN_GPIO2_ENABLE         BIT13

#define CM_ICLKEN_PER_EN_GPIO3_DISABLE        (0UL << 14)
#define CM_ICLKEN_PER_EN_GPIO3_ENABLE         BIT14

#define CM_ICLKEN_PER_EN_GPIO4_DISABLE        (0UL << 15)
#define CM_ICLKEN_PER_EN_GPIO4_ENABLE         BIT15

#define CM_ICLKEN_PER_EN_GPIO5_DISABLE        (0UL << 16)
#define CM_ICLKEN_PER_EN_GPIO5_ENABLE         BIT16

#define CM_ICLKEN_PER_EN_GPIO6_DISABLE        (0UL << 17)
#define CM_ICLKEN_PER_EN_GPIO6_ENABLE         BIT17

//Timer source clock selection
#define CM_CLKSEL_PER_CLKSEL_GPT3_32K         (0UL << 1)
#define CM_CLKSEL_PER_CLKSEL_GPT3_SYS         BIT1

#define CM_CLKSEL_PER_CLKSEL_GPT4_32K         (0UL << 2)
#define CM_CLKSEL_PER_CLKSEL_GPT4_SYS         BIT2

//Reset management (Global and Cold reset)
#define RST_GS            BIT1
#define RST_DPLL3         BIT2
#define GLOBAL_SW_RST     BIT1
#define GLOBAL_COLD_RST   (0x0UL << 0)

#endif // __OMAP3530PRCM_H__

