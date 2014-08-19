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

#include <Uefi.h>

#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#include <Drivers/PL341Dmc.h>

// Macros for writing to DDR2 controller.
#define DmcWriteReg(reg,val)                    MmioWrite32(DmcBase + reg, val)
#define DmcReadReg(reg)                         MmioRead32(DmcBase + reg)

// Macros for writing/reading to DDR2 PHY controller
#define DmcPhyWriteReg(reg,val)                    MmioWrite32(DmcPhyBase + reg, val)
#define DmcPhyReadReg(reg)                         MmioRead32(DmcPhyBase + reg)

// Initialise PL341 Dynamic Memory Controller
VOID
PL341DmcInit (
  IN  UINTN             DmcBase,
  IN  PL341_DMC_CONFIG* DmcConfig
  )
{
  UINTN Index;
  UINT32 Chip;

  // Set config mode
  DmcWriteReg(DMC_COMMAND_REG, DMC_COMMAND_CONFIGURE);

  //
  // Setup the QoS AXI ID bits
  //
  if (DmcConfig->HasQos) {
    // CLCD AXIID = 000
    DmcWriteReg(DMC_ID_0_CFG_REG, DMC_ID_CFG_QOS_ENABLE | DMC_ID_CFG_QOS_MIN);

    // Default disable QoS
    DmcWriteReg(DMC_ID_1_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_2_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_3_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_4_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_5_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_6_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_7_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_8_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_9_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_10_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_11_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_12_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_13_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_14_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    DmcWriteReg(DMC_ID_15_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
  }

  //
  // Initialise memory controlller
  //
  DmcWriteReg(DMC_REFRESH_PRD_REG, DmcConfig->RefreshPeriod);
  DmcWriteReg(DMC_CAS_LATENCY_REG, DmcConfig->CasLatency);
  DmcWriteReg(DMC_WRITE_LATENCY_REG, DmcConfig->WriteLatency);
  DmcWriteReg(DMC_T_MRD_REG, DmcConfig->t_mrd);
  DmcWriteReg(DMC_T_RAS_REG, DmcConfig->t_ras);
  DmcWriteReg(DMC_T_RC_REG, DmcConfig->t_rc);
  DmcWriteReg(DMC_T_RCD_REG, DmcConfig->t_rcd);
  DmcWriteReg(DMC_T_RFC_REG, DmcConfig->t_rfc);
  DmcWriteReg(DMC_T_RP_REG, DmcConfig->t_rp);
  DmcWriteReg(DMC_T_RRD_REG, DmcConfig->t_rrd);
  DmcWriteReg(DMC_T_WR_REG, DmcConfig->t_wr);
  DmcWriteReg(DMC_T_WTR_REG, DmcConfig->t_wtr);
  DmcWriteReg(DMC_T_XP_REG, DmcConfig->t_xp);
  DmcWriteReg(DMC_T_XSR_REG, DmcConfig->t_xsr);
  DmcWriteReg(DMC_T_ESR_REG, DmcConfig->t_esr);
  DmcWriteReg(DMC_T_FAW_REG, DmcConfig->t_faw);
  DmcWriteReg(DMC_T_WRLAT_DIFF, DmcConfig->t_wdata_en);
  DmcWriteReg(DMC_T_RDATA_EN, DmcConfig->t_data_en);

  //
  // Initialise PL341 Mem Config Registers
  //

  // Set PL341 Memory Config
  DmcWriteReg(DMC_MEMORY_CONFIG_REG, DmcConfig->MemoryCfg);

  // Set PL341 Memory Config 2
  DmcWriteReg(DMC_MEMORY_CFG2_REG, DmcConfig->MemoryCfg2);

  // Set PL341 Memory Config 3
  DmcWriteReg(DMC_MEMORY_CFG3_REG, DmcConfig->MemoryCfg3);

  // Set PL341 Chip Select <n>
  DmcWriteReg(DMC_CHIP_0_CFG_REG, DmcConfig->ChipCfg0);
  DmcWriteReg(DMC_CHIP_1_CFG_REG, DmcConfig->ChipCfg1);
  DmcWriteReg(DMC_CHIP_2_CFG_REG, DmcConfig->ChipCfg2);
  DmcWriteReg(DMC_CHIP_3_CFG_REG, DmcConfig->ChipCfg3);

  // Delay
  for (Index = 0; Index < 10; Index++) {
    DmcReadReg(DMC_STATUS_REG);
  }

  if (DmcConfig->IsUserCfg) {
    //
    // Set Test Chip PHY Registers via PL341 User Config Reg
    // Note that user_cfgX registers are Write Only
    //
    // DLL Freq set = 250MHz - 266MHz
    //
    DmcWriteReg(DMC_USER_0_CFG_REG, DmcConfig->User0Cfg);

    // user_config2
    // ------------
    // Set defaults before calibrating the DDR2 buffer impendence
    // - Disable ODT
    // - Default drive strengths
    DmcWriteReg(DMC_USER_2_CFG_REG, 0x40000198);

    //
    // Auto calibrate the DDR2 buffers impendence
    //
    while (!(DmcReadReg(DMC_USER_STATUS_REG) & 0x100));

    // Set the output driven strength
    DmcWriteReg(DMC_USER_2_CFG_REG, 0x40800000 | DmcConfig->User2Cfg);

    //
    // Set PL341 Feature Control Register
    //
    // Disable early BRESP - use to optimise CLCD performance
    DmcWriteReg(DMC_FEATURE_CRTL_REG, 0x00000001);
  }

  //
  // Config memories
  //
  for (Chip = 0; Chip < DmcConfig->MaxChip; Chip++) {
    // Send nop
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_NOP);

    // Pre-charge all
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_PRECHARGEALL);

    // Delay
    for (Index = 0; Index < 10; Index++) {
      DmcReadReg(DMC_STATUS_REG);
    }

    // Set (EMR2) extended mode register 2
    DmcWriteReg(DMC_DIRECT_CMD_REG,
      DMC_DIRECT_CMD_CHIP_ADDR(Chip) |
      DMC_DIRECT_CMD_BANKADDR(2) |
      DMC_DIRECT_CMD_MEMCMD_EXTMODEREG);

    // Set (EMR3) extended mode register 3
    DmcWriteReg(DMC_DIRECT_CMD_REG,
      DMC_DIRECT_CMD_CHIP_ADDR(Chip) |
      DMC_DIRECT_CMD_BANKADDR(3) |
      DMC_DIRECT_CMD_MEMCMD_EXTMODEREG);

    //
    // Set (EMR) Extended Mode Register
    //
    // Put into OCD default state
    DmcWriteReg(DMC_DIRECT_CMD_REG,DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_BANKADDR(1) | DMC_DIRECT_CMD_MEMCMD_EXTMODEREG);

    //
    // Set (MR) mode register - With DLL reset
    //
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_EXTMODEREG | DmcConfig->ModeReg | DDR2_MR_DLL_RESET);

    // Pre-charge all
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_PRECHARGEALL);
    // Auto-refresh
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_AUTOREFRESH);
    // Auto-refresh
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_AUTOREFRESH);

    //
    // Set (MR) mode register - Without DLL reset
    //
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | DMC_DIRECT_CMD_MEMCMD_EXTMODEREG | DmcConfig->ModeReg);

    // Delay
    for (Index = 0; Index < 10; Index++) {
      DmcReadReg(DMC_STATUS_REG);
    }

    //
    // Set (EMR) extended mode register - Enable OCD defaults
    //
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | (0x00090000) |
        (1 << DDR_MODESET_SHFT) | (DDR_EMR_OCD_DEFAULT << DDR_EMR_OCD_SHIFT) | DmcConfig->ExtModeReg);

    // Delay
    for (Index = 0; Index < 10; Index++) {
      DmcReadReg(DMC_STATUS_REG);
    }

    // Set (EMR) extended mode register - OCD Exit
    DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(Chip) | (0x00090000) |
        (1 << DDR_MODESET_SHFT) | (DDR_EMR_OCD_NS << DDR_EMR_OCD_SHIFT) | DmcConfig->ExtModeReg);

    // Delay
    for (Index = 0; Index < 10; Index++) {
      DmcReadReg(DMC_STATUS_REG);
    }
  }

  // Move DDR2 Controller to Ready state by issueing GO command
  DmcWriteReg(DMC_COMMAND_REG, DMC_COMMAND_GO);

  // wait for ready
  while (!(DmcReadReg(DMC_STATUS_REG) & DMC_STATUS_READY));

}
