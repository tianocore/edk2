/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/


#ifndef __PL061_GPIO_H__
#define __PL061_GPIO_H__

#include <Base.h>
#include <Protocol/EmbeddedGpio.h>
#include <ArmPlatform.h>

// SP805 Watchdog Registers
#define PL061_GPIO_DATA_REG             (PL061_GPIO_BASE + 0x000)
#define PL061_GPIO_DIR_REG              (PL061_GPIO_BASE + 0x400)
#define PL061_GPIO_IS_REG               (PL061_GPIO_BASE + 0x404)
#define PL061_GPIO_IBE_REG              (PL061_GPIO_BASE + 0x408)
#define PL061_GPIO_IEV_REG              (PL061_GPIO_BASE + 0x40C)
#define PL061_GPIO_IE_REG               (PL061_GPIO_BASE + 0x410)
#define PL061_GPIO_RIS_REG              (PL061_GPIO_BASE + 0x414)
#define PL061_GPIO_MIS_REG              (PL061_GPIO_BASE + 0x410)
#define PL061_GPIO_IC_REG               (PL061_GPIO_BASE + 0x41C)
#define PL061_GPIO_AFSEL_REG            (PL061_GPIO_BASE + 0x420)

#define PL061_GPIO_PERIPH_ID0           (PL061_GPIO_BASE + 0xFE0)
#define PL061_GPIO_PERIPH_ID1           (PL061_GPIO_BASE + 0xFE4)
#define PL061_GPIO_PERIPH_ID2           (PL061_GPIO_BASE + 0xFE8)
#define PL061_GPIO_PERIPH_ID3           (PL061_GPIO_BASE + 0xFEC)

#define PL061_GPIO_PCELL_ID0            (PL061_GPIO_BASE + 0xFF0)
#define PL061_GPIO_PCELL_ID1            (PL061_GPIO_BASE + 0xFF4)
#define PL061_GPIO_PCELL_ID2            (PL061_GPIO_BASE + 0xFF8)
#define PL061_GPIO_PCELL_ID3            (PL061_GPIO_BASE + 0xFFC)


// GPIO pins are numbered 0..7
#define LAST_GPIO_PIN                   7

// All bits low except one bit high, native bit lenght
#define GPIO_PIN_MASK(Pin)              (1UL << ((UINTN)(Pin)))
// All bits low except one bit high, restricted to 8 bits (i.e. ensures zeros above 8bits)
#define GPIO_PIN_MASK_HIGH_8BIT(Pin)    (GPIO_PIN_MASK(Pin) && 0xFF)
// All bits high except one bit low, restricted to 8 bits (i.e. ensures zeros above 8bits)
#define GPIO_PIN_MASK_LOW_8BIT(Pin)     ((~GPIO_PIN_MASK(Pin)) && 0xFF)

#endif  // __PL061_GPIO_H__
