/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __OMAP3530_H__
#define __OMAP3530_H__

#include "Omap3530Gpio.h"
#include "Omap3530Interrupt.h"
#include "Omap3530Prcm.h"
#include "Omap3530Timer.h"
#include "Omap3530Uart.h"
#include "Omap3530Usb.h"
#include "Omap3530MMCHS.h"
#include "Omap3530I2c.h"
#include "Omap3530PadConfiguration.h"
#include "Omap3530Gpmc.h"
#include "Omap3530Dma.h"


//CONTROL_PBIAS_LITE
#define CONTROL_PBIAS_LITE    0x48002520
#define PBIASLITEVMODE0       BIT0
#define PBIASLITEPWRDNZ0      BIT1
#define PBIASSPEEDCTRL0       BIT2
#define PBIASLITEVMODE1       BIT8
#define PBIASLITEWRDNZ1       BIT9

#endif // __OMAP3530_H__

