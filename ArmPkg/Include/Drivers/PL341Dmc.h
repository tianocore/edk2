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

#ifndef PL341DMC_H_
#define PL341DMC_H_


struct pl341_dmc_config {
    UINTN  base;           // base address for the controller
    UINTN  has_qos;        // has QoS registers
    UINTN  max_chip;       // number of memory chips accessible
    UINT32  refresh_prd;
    UINT32  cas_latency;
    UINT32  write_latency;
    UINT32  t_mrd;
    UINT32  t_ras;
    UINT32  t_rc;
    UINT32  t_rcd;
    UINT32  t_rfc;
    UINT32  t_rp;
    UINT32  t_rrd;
    UINT32  t_wr;
    UINT32  t_wtr;
    UINT32  t_xp;
    UINT32  t_xsr;
    UINT32  t_esr;
    UINT32  memory_cfg;
    UINT32  memory_cfg2;
    UINT32  memory_cfg3;
    UINT32  chip_cfg0;
    UINT32  chip_cfg1;
    UINT32  chip_cfg2;
    UINT32  chip_cfg3;
    UINT32  t_faw;
};

/* Memory config bit fields */
#define DMC_MEMORY_CONFIG_COLUMN_ADDRESS_9      0x1
#define DMC_MEMORY_CONFIG_COLUMN_ADDRESS_10     0x2
#define DMC_MEMORY_CONFIG_COLUMN_ADDRESS_11     0x3
#define DMC_MEMORY_CONFIG_COLUMN_ADDRESS_12     0x4
#define DMC_MEMORY_CONFIG_ROW_ADDRESS_11        (0x0 << 3)
#define DMC_MEMORY_CONFIG_ROW_ADDRESS_12        (0x1 << 3)
#define DMC_MEMORY_CONFIG_ROW_ADDRESS_13        (0x2 << 3)
#define DMC_MEMORY_CONFIG_ROW_ADDRESS_14        (0x3 << 3)
#define DMC_MEMORY_CONFIG_ROW_ADDRESS_15        (0x4 << 3)
#define DMC_MEMORY_CONFIG_ROW_ADDRESS_16        (0x5 << 3)
#define DMC_MEMORY_CONFIG_BURST_2               (0x1 << 15)
#define DMC_MEMORY_CONFIG_BURST_4               (0x2 << 15)
#define DMC_MEMORY_CONFIG_BURST_8               (0x3 << 15)
#define DMC_MEMORY_CONFIG_BURST_16              (0x4 << 15)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_1    (0x0 << 21)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_2    (0x1 << 21)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_3    (0x2 << 21)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_4    (0x3 << 21)

#define DMC_MEMORY_CFG2_CLK_ASYNC    (0x0 << 0)
#define DMC_MEMORY_CFG2_CLK_SYNC    (0x1 << 0)
#define DMC_MEMORY_CFG2_DQM_INIT    (0x1 << 2)
#define DMC_MEMORY_CFG2_CKE_INIT    (0x1 << 3)
#define DMC_MEMORY_CFG2_BANK_BITS_2    (0x0 << 4)
#define DMC_MEMORY_CFG2_BANK_BITS_3    (0x3 << 4)
#define DMC_MEMORY_CFG2_MEM_WIDTH_16    (0x0 << 6)
#define DMC_MEMORY_CFG2_MEM_WIDTH_32    (0x1 << 6)
#define DMC_MEMORY_CFG2_MEM_WIDTH_64    (0x2 << 6)
#define DMC_MEMORY_CFG2_MEM_WIDTH_RESERVED  (0x3 << 6)



VOID PL341DmcInit(struct pl341_dmc_config *config);


#endif /* PL341DMC_H_ */
