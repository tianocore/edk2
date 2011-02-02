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

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Drivers/PL341Dmc.h>

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
#define DMC_DIRECT_CMD_CHIP_ADDR(n)\s\s\s\s((n & 0x3) << 20)


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
#define TC_UIOLHNC_MASK                         0x000003C0
#define TC_UIOLHNC_SHIFT                        0x6
#define TC_UIOLHPC_MASK                         0x0000003F
#define TC_UIOLHPC_SHIFT                        0x2
#define TC_UIOHOCT_MASK                         0x2
#define TC_UIOHOCT_SHIFT                        0x1
#define TC_UIOHSTOP_SHIFT                       0x0
#define TC_UIOLHXC_VALUE                        0x4                     

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
#define DDR_EMR_RTT_50                          0x00000044                  // DDR2 50 Ohm termination
#define DDR_EMR_RTT_75R                         0x00000004                  // DDR2 75 Ohm termination
#define DDR_EMR_RTT_150                         0x00000040                  // DDR2 150 Ohm termination
// Output Drive Strength Values:
#define DDR_EMR_ODS_FULL                        0x0                         // DDR2 Full Drive Strength
#define DDR_EMR_ODS_HALF                        0x1                         // DDR2 Half Drive Strength
// OCD values
#define DDR_EMR_OCD_DEFAULT                     0x7
#define DDR_EMR_OCD_NS                          0x0

#define DDR_EMR_ODS_VAL                         DDR_EMR_ODS_FULL



#define DmcWriteReg(reg,val)                    MmioWrite32(DmcBase + reg, val)
#define DmcReadReg(reg)                         MmioRead32(DmcBase + reg)

// Initialize PL341 Dynamic Memory Controller
VOID PL341DmcInit(struct pl341_dmc_config *config) {
    UINTN   DmcBase = config->base;
    UINT32  i, chip, val32;

    // Set config mode
    DmcWriteReg(DMC_COMMAND_REG, DMC_COMMAND_CONFIGURE);

    //
    // Setup the QoS AXI ID bits    
    //

    if (config->has_qos) {
\s\s// CLCD AXIID = 000
\s\sDmcWriteReg(DMC_ID_0_CFG_REG, DMC_ID_CFG_QOS_ENABLE | DMC_ID_CFG_QOS_MIN);

\s\s// Default disable QoS
\s\sDmcWriteReg(DMC_ID_1_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_2_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_3_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_4_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_5_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_6_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_7_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_8_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_9_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_10_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_11_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_12_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_13_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_14_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
\s\sDmcWriteReg(DMC_ID_15_CFG_REG, DMC_ID_CFG_QOS_DISABLE);
    }

    //
    // Initialise memory controlller
    //
    DmcWriteReg(DMC_REFRESH_PRD_REG, config->refresh_prd);
    DmcWriteReg(DMC_CAS_LATENCY_REG, config->cas_latency);
    DmcWriteReg(DMC_WRITE_LATENCY_REG, config->write_latency);
    DmcWriteReg(DMC_T_MRD_REG, config->t_mrd);
    DmcWriteReg(DMC_T_RAS_REG, config->t_ras);
    DmcWriteReg(DMC_T_RC_REG, config->t_rc);
    DmcWriteReg(DMC_T_RCD_REG, config->t_rcd);
    DmcWriteReg(DMC_T_RFC_REG, config->t_rfc);
    DmcWriteReg(DMC_T_RP_REG, config->t_rp);
    DmcWriteReg(DMC_T_RRD_REG, config->t_rrd);
    DmcWriteReg(DMC_T_WR_REG, config->t_wr);
    DmcWriteReg(DMC_T_WTR_REG, config->t_wtr);
    DmcWriteReg(DMC_T_XP_REG, config->t_xp);
    DmcWriteReg(DMC_T_XSR_REG, config->t_xsr);
    DmcWriteReg(DMC_T_ESR_REG, config->t_esr);
    DmcWriteReg(DMC_T_FAW_REG, config->t_faw);

    // =======================================================================
    // Initialise PL341 Mem Config Registers
    // =======================================================================

    // |======================================
    // | Set PL341 Memory Config
    // |======================================
    DmcWriteReg(DMC_MEMORY_CONFIG_REG, config->memory_cfg);

    // |======================================
    // | Set PL341 Memory Config 2
    // |======================================
    DmcWriteReg(DMC_MEMORY_CFG2_REG, config->memory_cfg2);

    // |======================================
    // | Set PL341 Chip Select <n>
    // |======================================
    DmcWriteReg(DMC_CHIP_0_CFG_REG, config->chip_cfg0);
    DmcWriteReg(DMC_CHIP_1_CFG_REG, config->chip_cfg1);
    DmcWriteReg(DMC_CHIP_2_CFG_REG, config->chip_cfg2);
    DmcWriteReg(DMC_CHIP_3_CFG_REG, config->chip_cfg3);

    // |======================================
    // | Set PL341 Memory Config 3 
    // |======================================
    DmcWriteReg(DMC_MEMORY_CFG3_REG, config->memory_cfg3);

\s\s// |========================================================
\s\s// |Set Test Chip PHY Registers via PL341 User Config Reg
\s\s// |Note that user_cfgX registers are Write Only
\s\s// |
\s\s// |DLL Freq set = 250MHz - 266MHz
\s\s// |======================================================== 
\s\sDmcWriteReg(DMC_USER_0_CFG_REG, 0x7C924924);
 
\s\s// user_config2
\s\s// ------------
\s\s// Set defaults before calibrating the DDR2 buffer impendence
\s\s// -Disable ODT
\s\s// -Default drive strengths
\s\sDmcWriteReg(DMC_USER_2_CFG_REG, 0x40000198);
 
\s\s// |=======================================================
\s\s// |Auto calibrate the DDR2 buffers impendence 
\s\s// |=======================================================
\s\sval32 = DmcReadReg(DMC_USER_STATUS_REG);
\s\swhile (!(val32 & 0x100)) {
\s\s    val32 = DmcReadReg(DMC_USER_STATUS_REG);
\s\s}

\s\s// Set the output driven strength
\s\sDmcWriteReg(DMC_USER_2_CFG_REG, 0x40800000 | 
\s\s\s\s    (TC_UIOLHXC_VALUE << TC_UIOLHNC_SHIFT) | 
\s\s\s\s    (TC_UIOLHXC_VALUE << TC_UIOLHPC_SHIFT) |
\s\s\s\s    (0x1 << TC_UIOHOCT_SHIFT) | 
\s\s\s\s    (0x1 << TC_UIOHSTOP_SHIFT));

\s\s// |======================================
\s\s// | Set PL341 Feature Control Register 
\s\s// |======================================
\s\s// | Disable early BRESP - use to optimise CLCD performance
\s\sDmcWriteReg(DMC_FEATURE_CRTL_REG, 0x00000001);
 
    //=================
    // Config memories
    //=================

    for (chip = 0; chip <= config-> max_chip; chip++) {
\s\s// send nop
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | DMC_DIRECT_CMD_MEMCMD_NOP);
\s\s// pre-charge all
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | DMC_DIRECT_CMD_MEMCMD_PRECHARGEALL);

\s\s// delay
\s\sfor (i = 0; i < 10; i++) {
\s\s    val32 = DmcReadReg(DMC_STATUS_REG);
\s\s}

\s\s// set (EMR2) extended mode register 2
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, 
\s\s\s\s    DMC_DIRECT_CMD_CHIP_ADDR(chip) | 
\s\s\s\s    DMC_DIRECT_CMD_BANKADDR(2) | 
\s\s\s\s    DMC_DIRECT_CMD_MEMCMD_EXTMODEREG);
\s\s// set (EMR3) extended mode register 3
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, 
\s\s\s\s    DMC_DIRECT_CMD_CHIP_ADDR(chip) | 
\s\s\s\s    DMC_DIRECT_CMD_BANKADDR(3) | 
\s\s\s\s    DMC_DIRECT_CMD_MEMCMD_EXTMODEREG);

\s\s// =================================
\s\s//  set (EMR) Extended Mode Register
\s\s// ==================================
\s\s// Put into OCD default state
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, 
\s\s\s\s    DMC_DIRECT_CMD_CHIP_ADDR(chip) | 
\s\s\s\s    DMC_DIRECT_CMD_BANKADDR(1) | 
\s\s\s\s    DMC_DIRECT_CMD_MEMCMD_EXTMODEREG);

\s\s// ===========================================================        
\s\s// set (MR) mode register - With DLL reset
\s\s// ===========================================================
\s\s// Burst Length = 4 (010)
\s\s// Burst Type   = Seq (0)
\s\s// Latency      = 4 (100)
\s\s// Test mode    = Off (0)
\s\s// DLL reset    = Yes (1)
\s\s// Wr Recovery  = 4  (011)      
\s\s// PD           = Normal (0)
  DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | 0x00080742);
        
\s\s// pre-charge all 
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | DMC_DIRECT_CMD_MEMCMD_PRECHARGEALL);
\s\s// auto-refresh 
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | DMC_DIRECT_CMD_MEMCMD_AUTOREFRESH);
\s\s// auto-refresh 
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | DMC_DIRECT_CMD_MEMCMD_AUTOREFRESH);

\s\s// delay
\s\sfor (i = 0; i < 10; i++) {
\s\s    val32 = DmcReadReg(DMC_STATUS_REG);
\s\s}

\s\s// ===========================================================        
\s\s// set (MR) mode register - Without DLL reset
\s\s// ===========================================================
  // auto-refresh
  DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | DMC_DIRECT_CMD_MEMCMD_AUTOREFRESH);
  DmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | 0x00080642);

  // delay
  for (i = 0; i < 10; i++) {
    val32 = DmcReadReg(DMC_STATUS_REG);
  }

\s\s// ======================================================        
\s\s// set (EMR) extended mode register - Enable OCD defaults
\s\s// ====================================================== 
\s\sval32 = 0; //NOP
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | 0x00090000 |
\s\s\s\s    (DDR_EMR_OCD_DEFAULT << DDR_EMR_OCD_SHIFT) | 
\s\s\s\s    DDR_EMR_RTT_75R | 
\s\s\s\s    (DDR_EMR_ODS_VAL << DDR_EMR_ODS_MASK));

\s\s// delay
\s\sfor (i = 0; i < 10; i++) {
\s\s    val32 = DmcReadReg(DMC_STATUS_REG);
\s\s}

\s\s// Set (EMR) extended mode register - OCD Exit
\s\sval32 = 0; //NOP
\s\sDmcWriteReg(DMC_DIRECT_CMD_REG, DMC_DIRECT_CMD_CHIP_ADDR(chip) | 0x00090000 | 
\s\s\s\s    (DDR_EMR_OCD_NS << DDR_EMR_OCD_SHIFT) | 
\s\s\s\s    DDR_EMR_RTT_75R |
\s\s\s\s    (DDR_EMR_ODS_VAL << DDR_EMR_ODS_MASK));

    }

    //----------------------------------------    
    // go command
    DmcWriteReg(DMC_COMMAND_REG, DMC_COMMAND_GO);

    // wait for ready
    val32 = DmcReadReg(DMC_STATUS_REG);
    while (!(val32 & DMC_STATUS_READY)) {
        val32 = DmcReadReg(DMC_STATUS_REG);
    }
}
