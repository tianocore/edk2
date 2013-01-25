/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3530I2C_H__
#define __OMAP3530I2C_H__

//I2C register definitions.
#define I2C1BASE        0x48070000

#define I2C_IE          (I2C1BASE + 0x4)
#define XRDY_IE         BIT4
#define RRDY_IE         BIT3
#define ARDY_IE         BIT2
#define NACK_IE         BIT1

#define I2C_STAT        (I2C1BASE + 0x8)
#define BB              BIT12
#define XRDY            BIT4
#define RRDY            BIT3
#define ARDY            BIT2
#define NACK            BIT1

#define I2C_WE          (I2C1BASE + 0xC)
#define I2C_SYSS        (I2C1BASE + 0x10)
#define I2C_BUF         (I2C1BASE + 0x14)
#define I2C_CNT         (I2C1BASE + 0x18)
#define I2C_DATA        (I2C1BASE + 0x1C)
#define I2C_SYSC        (I2C1BASE + 0x20)

#define I2C_CON         (I2C1BASE + 0x24)
#define STT             BIT0
#define STP             BIT1
#define XSA             BIT8
#define TRX             BIT9
#define MST             BIT10
#define I2C_EN          BIT15

#define I2C_OA0         (I2C1BASE + 0x28)
#define I2C_SA          (I2C1BASE + 0x2C)
#define I2C_PSC         (I2C1BASE + 0x30)
#define I2C_SCLL        (I2C1BASE + 0x34)
#define I2C_SCLH        (I2C1BASE + 0x38)
#define I2C_SYSTEST     (I2C1BASE + 0x3C)
#define I2C_BUFSTAT     (I2C1BASE + 0x40)
#define I2C_OA1         (I2C1BASE + 0x44)
#define I2C_OA2         (I2C1BASE + 0x48)
#define I2C_OA3         (I2C1BASE + 0x4C)
#define I2C_ACTOA       (I2C1BASE + 0x50)
#define I2C_SBLOCK      (I2C1BASE + 0x54)

#endif //__OMAP3530I2C_H__
