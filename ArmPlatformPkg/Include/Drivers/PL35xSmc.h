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

#ifndef PL354SMC_H_
#define PL354SMC_H_

#define PL354_SMC_DIRECT_CMD_OFFSET  0x10
#define PL354_SMC_SET_CYCLES_OFFSET  0x14
#define PL354_SMC_SET_OPMODE_OFFSET  0x18

#define PL354_SMC_DIRECT_CMD_ADDR(addr)             ((addr) & 0xFFFFF)
#define PL354_SMC_DIRECT_CMD_ADDR_SET_CRE           (1 << 20)
#define PL354_SMC_DIRECT_CMD_ADDR_CMD_MODE_UPDATE   (3 << 21)
#define PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE        (2 << 21)
#define PL354_SMC_DIRECT_CMD_ADDR_CMD_MODE          (1 << 21)
#define PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE_AXI    (0 << 21)
#define PL354_SMC_DIRECT_CMD_ADDR_CS(interf,chip)   (((interf) << 25) | ((chip) << 23))

#define PL354_SMC_SET_OPMODE_MEM_WIDTH_8                (0 << 0)
#define PL354_SMC_SET_OPMODE_MEM_WIDTH_16               (1 << 0)
#define PL354_SMC_SET_OPMODE_MEM_WIDTH_32               (2 << 0)
#define PL354_SMC_SET_OPMODE_SET_RD_SYNC                (1 << 2)
#define PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_1      (0 << 3)
#define PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_4      (1 << 3)
#define PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_8      (2 << 3)
#define PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_16     (3 << 3)
#define PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_32     (4 << 3)
#define PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_CONT   (5 << 3)
#define PL354_SMC_SET_OPMODE_SET_WR_SYNC                (1 << 6)
#define PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_1      (0 << 7)
#define PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_4      (1 << 7)
#define PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_8      (2 << 7)
#define PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_16     (3 << 7)
#define PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_32     (4 << 7)
#define PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_CONT   (5 << 7)
#define PL354_SMC_SET_OPMODE_SET_BAA                    (1 << 10)
#define PL354_SMC_SET_OPMODE_SET_ADV                    (1 << 11)
#define PL354_SMC_SET_OPMODE_SET_BLS                    (1 << 12)
#define PL354_SMC_SET_OPMODE_SET_BURST_ALIGN_ANY        (0 << 13)
#define PL354_SMC_SET_OPMODE_SET_BURST_ALIGN_32         (1 << 13)
#define PL354_SMC_SET_OPMODE_SET_BURST_ALIGN_64         (2 << 13)
#define PL354_SMC_SET_OPMODE_SET_BURST_ALIGN_128        (3 << 13)
#define PL354_SMC_SET_OPMODE_SET_BURST_ALIGN_256        (4 << 13)


#endif
