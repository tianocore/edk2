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

#ifndef _PL341DMC_H_
#define _PL341DMC_H_


typedef struct  {
    UINTN   HasQos;        // has QoS registers
    UINTN   MaxChip;       // number of memory chips accessible
    BOOLEAN IsUserCfg;
    UINT32  User0Cfg;
    UINT32  User2Cfg;
    UINT32  RefreshPeriod;
    UINT32  CasLatency;
    UINT32  WriteLatency;
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
    UINT32  MemoryCfg;
    UINT32  MemoryCfg2;
    UINT32  MemoryCfg3;
    UINT32  ChipCfg0;
    UINT32  ChipCfg1;
    UINT32  ChipCfg2;
    UINT32  ChipCfg3;
    UINT32  t_faw;
    UINT32  t_data_en;
    UINT32  t_wdata_en;
    UINT32  ModeReg;
    UINT32  ExtModeReg;
} PL341_DMC_CONFIG;

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

//
// DMC Configuration Register Map
//
#define DMC_STATUS_REG              0x00
#define DMC_COMMAND_REG             0x04
#define DMC_DIRECT_CMD_REG          0x08
#define DMC_MEMORY_CONFIG_REG       0x0C
#define DMC_REFRESH_PRD_REG         0x10
#define DMC_CAS_LATENCY_REG         0x14
#define DMC_WRITE_LATENCY_REG       0x18
#define DMC_T_MRD_REG               0x1C
#define DMC_T_RAS_REG               0x20
#define DMC_T_RC_REG                0x24
#define DMC_T_RCD_REG               0x28
#define DMC_T_RFC_REG               0x2C
#define DMC_T_RP_REG                0x30
#define DMC_T_RRD_REG               0x34
#define DMC_T_WR_REG                0x38
#define DMC_T_WTR_REG               0x3C
#define DMC_T_XP_REG                0x40
#define DMC_T_XSR_REG               0x44
#define DMC_T_ESR_REG               0x48
#define DMC_MEMORY_CFG2_REG         0x4C
#define DMC_MEMORY_CFG3_REG         0x50
#define DMC_T_FAW_REG               0x54
#define DMC_T_RDATA_EN              0x5C        /* DFI read data enable register */
#define DMC_T_WRLAT_DIFF            0x60        /* DFI write data enable register */

// Returns the state of the memory controller:
#define DMC_STATUS_CONFIG       0x0
#define DMC_STATUS_READY        0x1
#define DMC_STATUS_PAUSED       0x2
#define DMC_STATUS_LOWPOWER     0x3

// Changes the state of the memory controller:
#define DMC_COMMAND_GO              0x0
#define DMC_COMMAND_SLEEP           0x1
#define DMC_COMMAND_WAKEUP          0x2
#define DMC_COMMAND_PAUSE           0x3
#define DMC_COMMAND_CONFIGURE       0x4
#define DMC_COMMAND_ACTIVEPAUSE     0x7

// Determines the command required
#define DMC_DIRECT_CMD_MEMCMD_PRECHARGEALL      0x0
#define DMC_DIRECT_CMD_MEMCMD_AUTOREFRESH       (0x1 << 18)
#define DMC_DIRECT_CMD_MEMCMD_MODEREG           (0x2 << 18)
#define DMC_DIRECT_CMD_MEMCMD_EXTMODEREG        (0x2 << 18)
#define DMC_DIRECT_CMD_MEMCMD_NOP               (0x3 << 18)
#define DMC_DIRECT_CMD_MEMCMD_DPD               (0x1 << 22)
#define DMC_DIRECT_CMD_BANKADDR(n)              ((n & 0x3) << 16)
#define DMC_DIRECT_CMD_CHIP_ADDR(n)             ((n & 0x3) << 20)


//
// AXI ID configuration register map
//
#define DMC_ID_0_CFG_REG            0x100
#define DMC_ID_1_CFG_REG            0x104
#define DMC_ID_2_CFG_REG            0x108
#define DMC_ID_3_CFG_REG            0x10C
#define DMC_ID_4_CFG_REG            0x110
#define DMC_ID_5_CFG_REG            0x114
#define DMC_ID_6_CFG_REG            0x118
#define DMC_ID_7_CFG_REG            0x11C
#define DMC_ID_8_CFG_REG            0x120
#define DMC_ID_9_CFG_REG            0x124
#define DMC_ID_10_CFG_REG           0x128
#define DMC_ID_11_CFG_REG           0x12C
#define DMC_ID_12_CFG_REG           0x130
#define DMC_ID_13_CFG_REG           0x134
#define DMC_ID_14_CFG_REG           0x138
#define DMC_ID_15_CFG_REG           0x13C

// Set the QoS
#define DMC_ID_CFG_QOS_DISABLE      0
#define DMC_ID_CFG_QOS_ENABLE       1
#define DMC_ID_CFG_QOS_MIN          2


//
// Chip configuration register map
//
#define DMC_CHIP_0_CFG_REG          0x200
#define DMC_CHIP_1_CFG_REG          0x204
#define DMC_CHIP_2_CFG_REG          0x208
#define DMC_CHIP_3_CFG_REG          0x20C

//
// User Defined Pins
//
#define DMC_USER_STATUS_REG         0x300
#define DMC_USER_0_CFG_REG          0x304
#define DMC_USER_1_CFG_REG          0x308
#define DMC_FEATURE_CRTL_REG        0x30C
#define DMC_USER_2_CFG_REG          0x310


//
// PHY Register Settings
//
#define PHY_PTM_DFI_CLK_RANGE       0xE00  // DDR2 PHY PTM register offset
#define PHY_PTM_IOTERM              0xE04
#define PHY_PTM_PLL_EN              0xe0c
#define PHY_PTM_PLL_RANGE           0xe18
#define PHY_PTM_FEEBACK_DIV         0xe1c
#define PHY_PTM_RCLK_DIV            0xe20
#define PHY_PTM_LOCK_STATUS         0xe28
#define PHY_PTM_INIT_DONE           0xe34
#define PHY_PTM_ADDCOM_IOSTR_OFF    0xec8
#define PHY_PTM_SQU_TRAINING        0xee8
#define PHY_PTM_SQU_STAT            0xeec

// ==============================================================================
// PIPD 40G DDR2/DDR3 PHY Register definitions
//
// Offsets from APB Base Address
// ==============================================================================
#define PHY_BYTE0_OFFSET            0x000
#define PHY_BYTE1_OFFSET            0x200
#define PHY_BYTE2_OFFSET            0x400
#define PHY_BYTE3_OFFSET            0x600

#define PHY_BYTE0_COARSE_SQADJ_INIT 0x064  ;// Coarse squelch adjust
#define PHY_BYTE1_COARSE_SQADJ_INIT 0x264  ;// Coarse squelch adjust
#define PHY_BYTE2_COARSE_SQADJ_INIT 0x464  ;// Coarse squelch adjust
#define PHY_BYTE3_COARSE_SQADJ_INIT 0x664  ;// Coarse squelch adjust

#define PHY_BYTE0_IOSTR_OFFSET      0x004
#define PHY_BYTE1_IOSTR_OFFSET      0x204
#define PHY_BYTE2_IOSTR_OFFSET      0x404
#define PHY_BYTE3_IOSTR_OFFSET      0x604


;//--------------------------------------------------------------------------

// DFI Clock ranges:

#define PHY_PTM_DFI_CLK_RANGE_200MHz            0x0
#define PHY_PTM_DFI_CLK_RANGE_201_267MHz        0x1
#define PHY_PTM_DFI_CLK_RANGE_268_333MHz        0x2
#define PHY_PTM_DFI_CLK_RANGE_334_400MHz        0x3
#define PHY_PTM_DFI_CLK_RANGE_401_533MHz        0x4
#define PHY_PTM_DFI_CLK_RANGE_534_667MHz        0x5
#define PHY_PTM_DFI_CLK_RANGE_668_800MHz        0x6



#define  PHY_PTM_DFI_CLK_RANGE_VAL              PHY_PTM_DFI_CLK_RANGE_334_400MHz

//--------------------------------------------------------------------------


// PLL Range

#define PHY_PTM_PLL_RANGE_200_400MHz            0x0     // b0 = frequency >= 200 MHz and < 400 MHz
#define PHY_PTM_PLL_RANGE_400_800MHz            0x1     // b1 = frequency >= 400 MHz.
#define PHY_PTM_FEEBACK_DIV_200_400MHz          0x0     // b0 = frequency >= 200 MHz and < 400 MHz
#define PHY_PTM_FEEBACK_DIV_400_800MHz          0x1     // b1 = frequency >= 400 MHz.
#define PHY_PTM_REFCLK_DIV_200_400MHz           0x0
#define PHY_PTM_REFCLK_DIV_400_800MHz           0x1

#define TC_UIOLHNC_MASK                         0x000003C0
#define TC_UIOLHNC_SHIFT                        0x6
#define TC_UIOLHPC_MASK                         0x0000003F
#define TC_UIOLHPC_SHIFT                        0x2
#define TC_UIOHOCT_MASK                         0x2
#define TC_UIOHOCT_SHIFT                        0x1
#define TC_UIOHSTOP_SHIFT                       0x0
#define TC_UIOLHXC_VALUE                        0x4

#define PHY_PTM_SQU_TRAINING_ENABLE             0x1
#define PHY_PTM_SQU_TRAINING_DISABLE            0x0


//--------------------------------------
// JEDEC DDR2 Device Register definitions and settings
//--------------------------------------
#define DDR_MODESET_SHFT                        14
#define DDR_MODESET_MR                          0x0  ;// Mode register
#define DDR_MODESET_EMR                         0x1  ;// Extended Mode register
#define DDR_MODESET_EMR2                        0x2
#define DDR_MODESET_EMR3                        0x3

//
// Extended Mode Register settings
//
#define DDR_EMR_OCD_MASK                        0x0000380
#define DDR_EMR_OCD_SHIFT                       0x7
#define DDR_EMR_RTT_MASK                        0x00000044                  // DDR2 Device RTT (ODT) settings
#define DDR_EMR_RTT_SHIFT                       0x2
#define DDR_EMR_ODS_MASK                        0x00000002                  // DDR2 Output Drive Strength
#define DDR_EMR_ODS_SHIFT                       0x0001

// Termination Values:
#define DDR_EMR_RTT_50R                         0x00000044                  // DDR2 50 Ohm termination
#define DDR_EMR_RTT_75R                         0x00000004                  // DDR2 75 Ohm termination
#define DDR_EMR_RTT_150                         0x00000040                  // DDR2 150 Ohm termination

// Output Drive Strength Values:
#define DDR_EMR_ODS_FULL                        0x0                         // DDR2 Full Drive Strength
#define DDR_EMR_ODS_HALF                        0x1                         // DDR2 Half Drive Strength

// OCD values
#define DDR_EMR_OCD_DEFAULT                     0x7
#define DDR_EMR_OCD_NS                          0x0

#define DDR_EMR_ODS_VAL                         DDR_EMR_ODS_FULL

#define DDR_SDRAM_START_ADDR                    0x10000000


// ----------------------------------------
// PHY IOTERM values
// ----------------------------------------
#define PHY_PTM_IOTERM_OFF                      0x0
#define PHY_PTM_IOTERM_150R                     0x1
#define PHY_PTM_IOTERM_75R                      0x2
#define PHY_PTM_IOTERM_50R                      0x3

#define PHY_BYTE_IOSTR_60OHM                    0x0
#define PHY_BYTE_IOSTR_40OHM                    0x1
#define PHY_BYTE_IOSTR_30OHM                    0x2
#define PHY_BYTE_IOSTR_30AOHM                   0x3

#define DDR2_MR_BURST_LENGTH_4     (2)
#define DDR2_MR_BURST_LENGTH_8     (3)
#define DDR2_MR_DLL_RESET          (1 << 8)
#define DDR2_MR_CAS_LATENCY_4      (4 << 4)
#define DDR2_MR_CAS_LATENCY_5      (5 << 4)
#define DDR2_MR_CAS_LATENCY_6      (6 << 4)
#define DDR2_MR_WR_CYCLES_2        (1 << 9)
#define DDR2_MR_WR_CYCLES_3        (2 << 9)
#define DDR2_MR_WR_CYCLES_4        (3 << 9)
#define DDR2_MR_WR_CYCLES_5        (4 << 9)
#define DDR2_MR_WR_CYCLES_6        (5 << 9)


VOID
PL341DmcInit (
  IN  UINTN             DmcBase,
  IN  PL341_DMC_CONFIG* DmcConfig
  );

VOID PL341DmcPhyInit (
  IN UINTN   DmcPhyBase
  );

VOID PL341DmcTrainPHY (
  IN UINTN   DmcPhyBase
  );

#endif /* _PL341DMC_H_ */
