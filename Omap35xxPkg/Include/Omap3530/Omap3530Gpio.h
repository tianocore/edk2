/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OMAP3530GPIO_H__
#define __OMAP3530GPIO_H__

#define GPIO1_BASE (0x48310000)
#define GPIO2_BASE (0x49050000)
#define GPIO3_BASE (0x49052000)
#define GPIO4_BASE (0x49054000)
#define GPIO5_BASE (0x49056000)
#define GPIO6_BASE (0x49058000)

#define GPIO_SYSCONFIG        (0x0010)
#define GPIO_SYSSTATUS        (0x0014)
#define GPIO_IRQSTATUS1       (0x0018)
#define GPIO_IRQENABLE1       (0x001C)
#define GPIO_WAKEUPENABLE     (0x0020)
#define GPIO_IRQSTATUS2       (0x0028)
#define GPIO_IRQENABLE2       (0x002C)
#define GPIO_CTRL             (0x0030)
#define GPIO_OE               (0x0034)
#define GPIO_DATAIN           (0x0038)
#define GPIO_DATAOUT          (0x003C)
#define GPIO_LEVELDETECT0     (0x0040)
#define GPIO_LEVELDETECT1     (0x0044)
#define GPIO_RISINGDETECT     (0x0048)
#define GPIO_FALLINGDETECT    (0x004C)
#define GPIO_DEBOUNCENABLE    (0x0050)
#define GPIO_DEBOUNCINGTIME   (0x0054)
#define GPIO_CLEARIRQENABLE1  (0x0060)
#define GPIO_SETIRQENABLE1    (0x0064)
#define GPIO_CLEARIRQENABLE2  (0x0070)
#define GPIO_SETIRQENABLE2    (0x0074)
#define GPIO_CLEARWKUENA      (0x0080)
#define GPIO_SETWKUENA        (0x0084)
#define GPIO_CLEARDATAOUT     (0x0090)
#define GPIO_SETDATAOUT       (0x0094)

#define GPIO_SYSCONFIG_IDLEMODE_MASK      (3UL << 3)
#define GPIO_SYSCONFIG_IDLEMODE_FORCE     (0UL << 3)
#define GPIO_SYSCONFIG_IDLEMODE_NONE      BIT3
#define GPIO_SYSCONFIG_IDLEMODE_SMART     (2UL << 3)
#define GPIO_SYSCONFIG_ENAWAKEUP_MASK     BIT2
#define GPIO_SYSCONFIG_ENAWAKEUP_DISABLE  (0UL << 2)
#define GPIO_SYSCONFIG_ENAWAKEUP_ENABLE   BIT2
#define GPIO_SYSCONFIG_SOFTRESET_MASK     BIT1
#define GPIO_SYSCONFIG_SOFTRESET_NORMAL   (0UL << 1)
#define GPIO_SYSCONFIG_SOFTRESET_RESET    BIT1
#define GPIO_SYSCONFIG_AUTOIDLE_MASK      BIT0
#define GPIO_SYSCONFIG_AUTOIDLE_FREE_RUN  (0UL << 0)
#define GPIO_SYSCONFIG_AUTOIDLE_ON        BIT0

#define GPIO_SYSSTATUS_RESETDONE_MASK     BIT0
#define GPIO_SYSSTATUS_RESETDONE_ONGOING  (0UL << 0)
#define GPIO_SYSSTATUS_RESETDONE_COMPLETE BIT0

#define GPIO_IRQSTATUS_MASK(x)            (1UL << (x))
#define GPIO_IRQSTATUS_NOT_TRIGGERED(x)   (0UL << (x))
#define GPIO_IRQSTATUS_TRIGGERED(x)       (1UL << (x))
#define GPIO_IRQSTATUS_CLEAR(x)           (1UL << (x))

#define GPIO_IRQENABLE_MASK(x)            (1UL << (x))
#define GPIO_IRQENABLE_DISABLE(x)         (0UL << (x))
#define GPIO_IRQENABLE_ENABLE(x)          (1UL << (x))

#define GPIO_WAKEUPENABLE_MASK(x)         (1UL << (x))
#define GPIO_WAKEUPENABLE_DISABLE(x)      (0UL << (x))
#define GPIO_WAKEUPENABLE_ENABLE(x)       (1UL << (x))

#define GPIO_CTRL_GATINGRATIO_MASK        (3UL << 1)
#define GPIO_CTRL_GATINGRATIO_DIV_1       (0UL << 1)
#define GPIO_CTRL_GATINGRATIO_DIV_2       BIT1
#define GPIO_CTRL_GATINGRATIO_DIV_4       (2UL << 1)
#define GPIO_CTRL_GATINGRATIO_DIV_8       (3UL << 1)
#define GPIO_CTRL_DISABLEMODULE_MASK      BIT0
#define GPIO_CTRL_DISABLEMODULE_ENABLE    (0UL << 0)
#define GPIO_CTRL_DISABLEMODULE_DISABLE   BIT0

#define GPIO_OE_MASK(x)                   (1UL << (x))
#define GPIO_OE_OUTPUT(x)                 (0UL << (x))
#define GPIO_OE_INPUT(x)                  (1UL << (x))

#define GPIO_DATAIN_MASK(x)               (1UL << (x))

#define GPIO_DATAOUT_MASK(x)              (1UL << (x))

#define GPIO_LEVELDETECT_MASK(x)          (1UL << (x))
#define GPIO_LEVELDETECT_DISABLE(x)       (0UL << (x))
#define GPIO_LEVELDETECT_ENABLE(x)        (1UL << (x))

#define GPIO_RISINGDETECT_MASK(x)         (1UL << (x))
#define GPIO_RISINGDETECT_DISABLE(x)      (0UL << (x))
#define GPIO_RISINGDETECT_ENABLE(x)       (1UL << (x))

#define GPIO_FALLINGDETECT_MASK(x)        (1UL << (x))
#define GPIO_FALLINGDETECT_DISABLE(x)     (0UL << (x))
#define GPIO_FALLINGDETECT_ENABLE(x)      (1UL << (x))

#define GPIO_DEBOUNCENABLE_MASK(x)        (1UL << (x))
#define GPIO_DEBOUNCENABLE_DISABLE(x)     (0UL << (x))
#define GPIO_DEBOUNCENABLE_ENABLE(x)      (1UL << (x))

#define GPIO_DEBOUNCINGTIME_MASK          (0xFF)
#define GPIO_DEBOUNCINGTIME_US(x)         ((((x) / 31) - 1) & GPIO_DEBOUNCINGTIME_MASK)

#define GPIO_CLEARIRQENABLE_BIT(x)        (1UL << (x))

#define GPIO_SETIRQENABLE_BIT(x)          (1UL << (x))

#define GPIO_CLEARWKUENA_BIT(x)           (1UL << (x))

#define GPIO_SETWKUENA_BIT(x)             (1UL << (x))

#define GPIO_CLEARDATAOUT_BIT(x)          (1UL << (x))

#define GPIO_SETDATAOUT_BIT(x)            (1UL << (x))

#endif // __OMAP3530GPIO_H__

