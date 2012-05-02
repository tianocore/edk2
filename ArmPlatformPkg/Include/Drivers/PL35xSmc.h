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

#ifndef PL35xSMC_H_
#define PL35xSMC_H_

#define PL350_SMC_DIRECT_CMD_OFFSET   0x10
#define PL350_SMC_SET_CYCLES_OFFSET   0x14
#define PL350_SMC_SET_OPMODE_OFFSET   0x18
#define PL350_SMC_REFRESH_0_OFFSET    0x20
#define PL350_SMC_REFRESH_1_OFFSET    0x24

#define PL350_SMC_DIRECT_CMD_ADDR(addr)                 ((addr) & 0xFFFFF)
#define PL350_SMC_DIRECT_CMD_ADDR_SET_CRE               (1 << 20)
#define PL350_SMC_DIRECT_CMD_ADDR_CMD_MODE_UPDATE       (3 << 21)
#define PL350_SMC_DIRECT_CMD_ADDR_CMD_UPDATE            (2 << 21)
#define PL350_SMC_DIRECT_CMD_ADDR_CMD_MODE              (1 << 21)
#define PL350_SMC_DIRECT_CMD_ADDR_CMD_UPDATE_AXI        (0 << 21)
#define PL350_SMC_DIRECT_CMD_ADDR_CS_INTERF(interf,chip) (((interf) << 25) | ((chip) << 23))
#define PL350_SMC_DIRECT_CMD_ADDR_CS(ChipSelect)        (((ChipSelect) & 0x7) << 23)

#define PL350_SMC_SET_OPMODE_MEM_WIDTH_8                (0 << 0)
#define PL350_SMC_SET_OPMODE_MEM_WIDTH_16               (1 << 0)
#define PL350_SMC_SET_OPMODE_MEM_WIDTH_32               (2 << 0)
#define PL350_SMC_SET_OPMODE_SET_RD_SYNC                (1 << 2)
#define PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_1      (0 << 3)
#define PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_4      (1 << 3)
#define PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_8      (2 << 3)
#define PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_16     (3 << 3)
#define PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_32     (4 << 3)
#define PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_CONT   (5 << 3)
#define PL350_SMC_SET_OPMODE_SET_WR_SYNC                (1 << 6)
#define PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_1      (0 << 7)
#define PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_4      (1 << 7)
#define PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_8      (2 << 7)
#define PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_16     (3 << 7)
#define PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_32     (4 << 7)
#define PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_CONT   (5 << 7)
#define PL350_SMC_SET_OPMODE_SET_BAA                    (1 << 10)
#define PL350_SMC_SET_OPMODE_SET_ADV                    (1 << 11)
#define PL350_SMC_SET_OPMODE_SET_BLS                    (1 << 12)
#define PL350_SMC_SET_OPMODE_SET_BURST_ALIGN_ANY        (0 << 13)
#define PL350_SMC_SET_OPMODE_SET_BURST_ALIGN_32         (1 << 13)
#define PL350_SMC_SET_OPMODE_SET_BURST_ALIGN_64         (2 << 13)
#define PL350_SMC_SET_OPMODE_SET_BURST_ALIGN_128        (3 << 13)
#define PL350_SMC_SET_OPMODE_SET_BURST_ALIGN_256        (4 << 13)

#define PL350_SMC_SET_CYCLE_NAND_T_RC(t)                (((t) & 0xF) << 0)
#define PL350_SMC_SET_CYCLE_NAND_T_WC(t)                (((t) & 0xF) << 4)
#define PL350_SMC_SET_CYCLE_NAND_T_REA(t)              (((t) & 0x7) << 8)
#define PL350_SMC_SET_CYCLE_NAND_T_WP(t)                (((t) & 0x7) << 11)
#define PL350_SMC_SET_CYCLE_NAND_T_CLR(t)                (((t) & 0x7) << 14)
#define PL350_SMC_SET_CYCLE_NAND_T_AR(t)                (((t) & 0x7) << 17)
#define PL350_SMC_SET_CYCLE_NAND_T_RR(t)                (((t) & 0x7) << 20)

#define PL350_SMC_SET_CYCLE_SRAM_T_RC(t)                (((t) & 0xF) << 0)
#define PL350_SMC_SET_CYCLE_SRAM_T_WC(t)                (((t) & 0xF) << 4)
#define PL350_SMC_SET_CYCLE_SRAM_T_CEOE(t)              (((t) & 0x7) << 8)
#define PL350_SMC_SET_CYCLE_SRAM_T_WP(t)                (((t) & 0x7) << 11)
#define PL350_SMC_SET_CYCLE_SRAM_T_PC(t)                (((t) & 0x7) << 14)
#define PL350_SMC_SET_CYCLE_SRAM_T_TR(t)                (((t) & 0x7) << 17)
#define PL350_SMC_SET_CYCLE_SRAM_WE_TIME                (1 << 20)

#endif
