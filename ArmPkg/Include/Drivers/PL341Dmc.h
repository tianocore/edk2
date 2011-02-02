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
    UINTN\s\sbase;           // base address for the controller
    UINTN\s\shas_qos;        // has QoS registers
    UINTN\s\smax_chip;       // number of memory chips accessible
    UINT32\s\srefresh_prd;
    UINT32\s\scas_latency;
    UINT32\s\swrite_latency;
    UINT32\s\st_mrd;
    UINT32\s\st_ras;
    UINT32\s\st_rc;
    UINT32\s\st_rcd;
    UINT32\s\st_rfc;
    UINT32\s\st_rp;
    UINT32\s\st_rrd;
    UINT32\s\st_wr;
    UINT32\s\st_wtr;
    UINT32\s\st_xp;
    UINT32\s\st_xsr;
    UINT32\s\st_esr;
    UINT32\s\smemory_cfg;
    UINT32\s\smemory_cfg2;
    UINT32\s\smemory_cfg3;
    UINT32\s\schip_cfg0;
    UINT32\s\schip_cfg1;
    UINT32\s\schip_cfg2;
    UINT32\s\schip_cfg3;
    UINT32\s\st_faw;
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
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_1\s\s\s\s(0x0 << 21)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_2\s\s\s\s(0x1 << 21)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_3\s\s\s\s(0x2 << 21)
#define DMC_MEMORY_CONFIG_ACTIVE_CHIP_4\s\s\s\s(0x3 << 21)

#define DMC_MEMORY_CFG2_CLK_ASYNC\s\s\s\s(0x0 << 0)
#define DMC_MEMORY_CFG2_CLK_SYNC\s\s\s\s(0x1 << 0)
#define DMC_MEMORY_CFG2_DQM_INIT\s\s\s\s(0x1 << 2)
#define DMC_MEMORY_CFG2_CKE_INIT\s\s\s\s(0x1 << 3)
#define DMC_MEMORY_CFG2_BANK_BITS_2\s\s\s\s(0x0 << 4)
#define DMC_MEMORY_CFG2_BANK_BITS_3\s\s\s\s(0x3 << 4)
#define DMC_MEMORY_CFG2_MEM_WIDTH_16\s\s\s\s(0x0 << 6)
#define DMC_MEMORY_CFG2_MEM_WIDTH_32\s\s\s\s(0x1 << 6)
#define DMC_MEMORY_CFG2_MEM_WIDTH_64\s\s\s\s(0x2 << 6)
#define DMC_MEMORY_CFG2_MEM_WIDTH_RESERVED\s\s(0x3 << 6)



VOID PL341DmcInit(struct pl341_dmc_config *config);


#endif /* PL341DMC_H_ */
