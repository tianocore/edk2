/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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

#include <Protocol/EmbeddedGpio.h>

// PL061 GPIO Registers
#define PL061_GPIO_DATA_REG             ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x000)
#define PL061_GPIO_DIR_REG              ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x400)
#define PL061_GPIO_IS_REG               ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x404)
#define PL061_GPIO_IBE_REG              ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x408)
#define PL061_GPIO_IEV_REG              ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x40C)
#define PL061_GPIO_IE_REG               ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x410)
#define PL061_GPIO_RIS_REG              ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x414)
#define PL061_GPIO_MIS_REG              ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x410)
#define PL061_GPIO_IC_REG               ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x41C)
#define PL061_GPIO_AFSEL_REG            ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0x420)

#define PL061_GPIO_PERIPH_ID0           ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFE0)
#define PL061_GPIO_PERIPH_ID1           ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFE4)
#define PL061_GPIO_PERIPH_ID2           ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFE8)
#define PL061_GPIO_PERIPH_ID3           ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFEC)

#define PL061_GPIO_PCELL_ID0            ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFF0)
#define PL061_GPIO_PCELL_ID1            ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFF4)
#define PL061_GPIO_PCELL_ID2            ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFF8)
#define PL061_GPIO_PCELL_ID3            ((UINT32)PcdGet32 (PcdPL061GpioBase) + 0xFFC)


// GPIO pins are numbered 0..7
#define LAST_GPIO_PIN                   7

// All bits low except one bit high, native bit length
#define GPIO_PIN_MASK(Pin)              (1UL << ((UINTN)(Pin)))
// All bits low except one bit high, restricted to 8 bits (i.e. ensures zeros above 8bits)
#define GPIO_PIN_MASK_HIGH_8BIT(Pin)    (GPIO_PIN_MASK(Pin) && 0xFF)
// All bits high except one bit low, restricted to 8 bits (i.e. ensures zeros above 8bits)
#define GPIO_PIN_MASK_LOW_8BIT(Pin)     ((~GPIO_PIN_MASK(Pin)) && 0xFF)

#endif  // __PL061_GPIO_H__
