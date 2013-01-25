/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __OMAP3530GPMC_H__
#define __OMAP3530GPMC_H__

#define GPMC_BASE             (0x6E000000)

//GPMC NAND definitions.
#define GPMC_SYSCONFIG        (GPMC_BASE + 0x10)
#define SMARTIDLEMODE         (0x2UL << 3)

#define GPMC_SYSSTATUS        (GPMC_BASE + 0x14)
#define GPMC_IRQSTATUS        (GPMC_BASE + 0x18)
#define GPMC_IRQENABLE        (GPMC_BASE + 0x1C)

#define GPMC_TIMEOUT_CONTROL  (GPMC_BASE + 0x40)
#define TIMEOUTENABLE         BIT0
#define TIMEOUTDISABLE        (0x0UL << 0)

#define GPMC_ERR_ADDRESS      (GPMC_BASE + 0x44)
#define GPMC_ERR_TYPE         (GPMC_BASE + 0x48)

#define GPMC_CONFIG           (GPMC_BASE + 0x50)
#define WRITEPROTECT_HIGH     BIT4
#define WRITEPROTECT_LOW      (0x0UL << 4)

#define GPMC_STATUS           (GPMC_BASE + 0x54)

#define GPMC_CONFIG1_0        (GPMC_BASE + 0x60)
#define DEVICETYPE_NOR        (0x0UL << 10)
#define DEVICETYPE_NAND       (0x2UL << 10)
#define DEVICESIZE_X8         (0x0UL << 12)
#define DEVICESIZE_X16        BIT12

#define GPMC_CONFIG2_0        (GPMC_BASE + 0x64)
#define CSONTIME              (0x0UL << 0)
#define CSRDOFFTIME           (0x14UL << 8)
#define CSWROFFTIME           (0x14UL << 16)

#define GPMC_CONFIG3_0        (GPMC_BASE + 0x68)
#define ADVRDOFFTIME          (0x14UL << 8)
#define ADVWROFFTIME          (0x14UL << 16)

#define GPMC_CONFIG4_0        (GPMC_BASE + 0x6C)
#define OEONTIME              BIT0
#define OEOFFTIME             (0xFUL << 8)
#define WEONTIME              BIT16
#define WEOFFTIME             (0xFUL << 24)

#define GPMC_CONFIG5_0        (GPMC_BASE + 0x70)
#define RDCYCLETIME           (0x14UL << 0)
#define WRCYCLETIME           (0x14UL << 8)
#define RDACCESSTIME          (0xCUL << 16)
#define PAGEBURSTACCESSTIME   BIT24

#define GPMC_CONFIG6_0        (GPMC_BASE + 0x74)
#define CYCLE2CYCLESAMECSEN   BIT7
#define CYCLE2CYCLEDELAY      (0xAUL << 8)
#define WRDATAONADMUXBUS      (0xFUL << 16)
#define WRACCESSTIME          BIT24

#define GPMC_CONFIG7_0        (GPMC_BASE + 0x78)
#define BASEADDRESS           (0x30UL << 0)
#define CSVALID               BIT6
#define MASKADDRESS_128MB     (0x8UL << 8)

#define GPMC_NAND_COMMAND_0   (GPMC_BASE + 0x7C)
#define GPMC_NAND_ADDRESS_0   (GPMC_BASE + 0x80)
#define GPMC_NAND_DATA_0      (GPMC_BASE + 0x84)

#define GPMC_ECC_CONFIG       (GPMC_BASE + 0x1F4)
#define ECCENABLE             BIT0
#define ECCDISABLE            (0x0UL << 0)
#define ECCCS_0               (0x0UL << 1)
#define ECC16B                BIT7

#define GPMC_ECC_CONTROL      (GPMC_BASE + 0x1F8)
#define ECCPOINTER_REG1       BIT0
#define ECCCLEAR              BIT8

#define GPMC_ECC_SIZE_CONFIG  (GPMC_BASE + 0x1FC)
#define ECCSIZE0_512BYTES     (0xFFUL << 12)
#define ECCSIZE1_512BYTES     (0xFFUL << 22)

#define GPMC_ECC1_RESULT      (GPMC_BASE + 0x200)
#define GPMC_ECC2_RESULT      (GPMC_BASE + 0x204)
#define GPMC_ECC3_RESULT      (GPMC_BASE + 0x208)
#define GPMC_ECC4_RESULT      (GPMC_BASE + 0x20C)
#define GPMC_ECC5_RESULT      (GPMC_BASE + 0x210)
#define GPMC_ECC6_RESULT      (GPMC_BASE + 0x214)
#define GPMC_ECC7_RESULT      (GPMC_BASE + 0x218)
#define GPMC_ECC8_RESULT      (GPMC_BASE + 0x21C)
#define GPMC_ECC9_RESULT      (GPMC_BASE + 0x220)

#endif //__OMAP3530GPMC_H__
