/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/


#ifndef __PL061_GPIO_H__
#define __PL061_GPIO_H__

#include <Protocol/EmbeddedGpio.h>

// PL061 GPIO Registers
#define PL061_GPIO_DATA_REG_OFFSET      ((UINTN) 0x000)
#define PL061_GPIO_DATA_REG             0x000
#define PL061_GPIO_DIR_REG              0x400
#define PL061_GPIO_IS_REG               0x404
#define PL061_GPIO_IBE_REG              0x408
#define PL061_GPIO_IEV_REG              0x40C
#define PL061_GPIO_IE_REG               0x410
#define PL061_GPIO_RIS_REG              0x414
#define PL061_GPIO_MIS_REG              0x410
#define PL061_GPIO_IC_REG               0x41C
#define PL061_GPIO_AFSEL_REG            0x420

#define PL061_GPIO_PERIPH_ID0           0xFE0
#define PL061_GPIO_PERIPH_ID1           0xFE4
#define PL061_GPIO_PERIPH_ID2           0xFE8
#define PL061_GPIO_PERIPH_ID3           0xFEC

#define PL061_GPIO_PCELL_ID0            0xFF0
#define PL061_GPIO_PCELL_ID1            0xFF4
#define PL061_GPIO_PCELL_ID2            0xFF8
#define PL061_GPIO_PCELL_ID3            0xFFC

#define PL061_GPIO_PINS                 8

// All bits low except one bit high, native bit length
#define GPIO_PIN_MASK(Pin)              (1UL << ((UINTN)(Pin)))

#endif  // __PL061_GPIO_H__
