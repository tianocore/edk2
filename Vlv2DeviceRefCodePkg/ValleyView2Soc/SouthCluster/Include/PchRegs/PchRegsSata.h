/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  PchRegsSata.h

  @brief
  Register names for VLV SATA controllers

  Conventions:

  - Prefixes:
    Definitions beginning with "R_" are registers
    Definitions beginning with "B_" are bits within registers
    Definitions beginning with "V_" are meaningful values of bits within the registers
    Definitions beginning with "S_" are register sizes
    Definitions beginning with "N_" are the bit position
  - In general, PCH registers are denoted by "_PCH_" in register names
  - Registers / bits that are different between PCH generations are denoted by
    "_PCH_<generation_name>_" in register/bit names. e.g., "_PCH_VLV_"
  - Registers / bits that are different between SKUs are denoted by "_<SKU_name>"
    at the end of the register/bit names
  - Registers / bits of new devices introduced in a PCH generation will be just named
    as "_PCH_" without <generation_name> inserted.

**/
#ifndef _PCH_REGS_SATA_H_
#define _PCH_REGS_SATA_H_

///
/// VLV SATA Message Bus
///
#define PCH_SATA_PHY_PORT_ID                0xA3  // SATA PHY Port ID
#define PCH_SATA_PHY_MMIO_READ_OPCODE       0x00  // CUnit to SATA PHY MMIO Read Opcode
#define PCH_SATA_PHY_MMIO_WRITE_OPCODE      0x01  // CUnit to SATA PHY MMIO Write Opcode

///
///  SATA Controller Registers (D19:F0)
///
#define PCI_DEVICE_NUMBER_PCH_SATA          19
#define PCI_FUNCTION_NUMBER_PCH_SATA        0

#define R_PCH_SATA_ID                       0x00  // Identifiers
#define B_PCH_SATA_ID_DID                   0xFFFF0000 // Device ID
#define B_PCH_SATA_ID_VID                   0x0000FFFF // Vendor ID
#define V_PCH_SATA_VENDOR_ID                V_PCH_INTEL_VENDOR_ID
#define V_PCH_SATA_DEVICE_ID_D_IDE          0x0F20  // Desktop IDE Mode (Ports 0 and 1)
#define V_PCH_SATA_DEVICE_ID_D_AHCI         0x0F22  // Desktop AHCI Mode (Ports 0 and 1)
#define V_PCH_SATA_DEVICE_ID_D_RAID         0x2822  // Desktop RAID 0/1/5/10 Mode, based on D19:F0:9Ch[7]

#define V_PCH_SATA_DEVICE_ID_M_IDE          0x0F21  // Mobile IDE Mode (Ports 0 and 1)
#define V_PCH_SATA_DEVICE_ID_M_AHCI         0x0F23  // Mobile AHCI Mode (Ports 0 and 1)
#define V_PCH_SATA_DEVICE_ID_M_RAID         0x282A  // Mobile RAID 0/1/5/10 Mode, based on D19:F0:9Ch[7]

#define R_PCH_SATA_COMMAND                  0x04  // Command
#define B_PCH_SATA_COMMAND_INT_DIS          BIT10 // Interrupt Disable
#define B_PCH_SATA_COMMAND_FBE              BIT9  // Fast Back-to-back Enable
#define B_PCH_SATA_COMMAND_SERR_EN          BIT8  // SERR# Enable
#define B_PCH_SATA_COMMAND_WCC              BIT7  // Wait Cycle Enable
#define B_PCH_SATA_COMMAND_PER              BIT6  // Parity Error Response Enable
#define B_PCH_SATA_COMMAND_VPS              BIT5  // VGA Palette Snooping Enable
#define B_PCH_SATA_COMMAND_PMWE             BIT4  // Memory Write and Invalidate Enable
#define B_PCH_SATA_COMMAND_SCE              BIT3  // Special Cycle Enable
#define B_PCH_SATA_COMMAND_BME              BIT2  // Bus Master Enable
#define B_PCH_SATA_COMMAND_MSE              BIT1  // Memory Space Enable
#define B_PCH_SATA_COMMAND_IOSE             BIT0  // I/O Space Enable

#define R_PCH_SATA_PCISTS                   0x06  // Device Status
#define B_PCH_SATA_PCISTS_DPE               BIT15 // Detected Parity Error
#define B_PCH_SATA_PCISTS_SSE               BIT14 // Signaled System Error
#define B_PCH_SATA_PCISTS_RMA               BIT13 // Received Master-Abort Status
#define B_PCH_SATA_PCISTS_RTA               BIT12 // Received Target-Abort Status
#define B_PCH_SATA_PCISTS_STA               BIT11 // Signaled Target-Abort Status
#define B_PCH_SATA_PCISTS_DEV_STS_MASK      (BIT10 | BIT9) // DEVSEL# Timing Status
#define B_PCH_SATA_PCISTS_DPED              BIT8  // Master Data Parity Error Detected
#define B_PCH_SATA_PCISTS_CAP_LIST          BIT4  // Capabilities List
#define B_PCH_SATA_PCISTS_ITNS              BIT3  // Interrupt Status

#define R_PCH_SATA_RID                      0x08  // Revision ID (8 bits)

#define R_PCH_SATA_PI_REGISTER              0x09  // Programming Interface (8 bits)
#define B_PCH_SATA_PI_REGISTER_SNC          BIT3  // Secondary Mode Native Capable
#define B_PCH_SATA_PI_REGISTER_SNE          BIT2  // Secondary Mode Native Enable
#define B_PCH_SATA_PI_REGISTER_PNC          BIT1  // Primary Mode Native Capable
#define B_PCH_SATA_PI_REGISTER_PNE          BIT0  // Primary Mode Native Enable

#define R_PCH_SATA_CC                       0x0A  // Class Code
#define B_PCH_SATA_CC_BCC                   0xFF00 // Base Class Code
#define B_PCH_SATA_CC_SCC                   0x00FF // Sub Class Code
#define V_PCH_SATA_CC_SCC_IDE               0x01
#define V_PCH_SATA_CC_SCC_AHCI              0x06
#define V_PCH_SATA_CC_SCC_RAID              0x04

#define R_PCH_SATA_CLS                      0x0C  // Cache Line Size (8 bits)
#define B_PCH_SATA_CLS                      0xFF

#define R_PCH_SATA_MLT                      0x0D  // Master Latency Timer (8 bits)
#define B_PCH_SATA_MLT                      0xFF

#define R_PCH_SATA_HTYPE                    0x0E  // Header Type
#define B_PCH_SATA_HTYPE_MFD                BIT7  // Multi-function Device
#define B_PCH_SATA_HTYPE_HL                 0x7F  // Header Layout

#define R_PCH_SATA_PCMD_BAR                 0x10  // Primary Command Block Base Address
#define B_PCH_SATA_PCMD_BAR_BA              0x0000FFF8 // Base Address
#define B_PCH_SATA_PCMD_BAR_RTE             BIT0  // Resource Type Indicator

#define R_PCH_SATA_PCTL_BAR                 0x14  // Primary Control Block Base Address
#define B_PCH_SATA_PCTL_BAR_BA              0x0000FFFC // Base Address
#define B_PCH_SATA_PCTL_BAR_RTE             BIT0  // Resource Type Indicator

#define R_PCH_SATA_SCMD_BAR                 0x18  // Secondary Command Block Base Address
#define B_PCH_SATA_SCMD_BAR_BA              0x0000FFF8 // Base Address
#define B_PCH_SATA_SCMD_BAR_RTE             BIT0  // Resource Type Indicator

#define R_PCH_SATA_SCTL_BAR                 0x1C  // Secondary Control Block Base Address
#define B_PCH_SATA_SCTL_BAR_BA              0x0000FFFC // Base Address
#define B_PCH_SATA_SCTL_BAR_RTE             BIT0  // Resource Type Indicator

#define R_PCH_SATA_LBAR                     0x20  // Legacy IDE Base Address / AHCI Index Data Pair Base Address
#define B_PCH_SATA_LBAR_BA                  0x0000FFE0 // Base Address
#define B_PCH_SATA_LBAR_BA4                 BIT4  // Base Address 4
#define B_PCH_SATA_LBAR_RTE                 BIT0  // Resource Type Indicator

#define R_PCH_SATA_SIDPBA                   0x24  // Serial ATA Index Data Pair Base Address
#define R_PCH_SATA_ABAR                     0x24  // AHCI Base Address
#define B_PCH_SATA_ABAR_BA                  0xFFFFF800 // AHCI Memory Base Address (When CC.SCC not equal 0x01)
#define V_PCH_SATA_ABAR_LENGTH              0x800 // AHCI Memory Length (When CC.SCC not equal 0x01)
#define N_PCH_SATA_ABAR_ALIGNMENT           11    // AHCI Base Address Alignment (When CC.SCC not equal 0x01)
#define B_PCH_SATA_SIDPBA_BA                0x0000FFF0 // Serial ATA Index Data Pair IO Base Address (When CC.SCC equal 0x01)
#define V_PCH_SATA_SIDPBA_LENGTH            0x10  // Serial ATA Index Data Pair IO Length (When CC.SCC equal 0x01)
#define N_PCH_SATA_SIDPBA_ALIGNMENT         4     // Serial ATA Index Data Pair Base Address Alignment (When CC.SCC not equal 0x01)
#define B_PCH_SATA_ABAR_PF                  BIT3  // Prefetchable
#define B_PCH_SATA_ABAR_TP                  (BIT2 | BIT1) // Type
#define B_PCH_SATA_ABAR_RTE                 BIT0  // Resource Type Indicator

#define R_PCH_SATA_SS                       0x2C  // Sub System Identifiers
#define B_PCH_SATA_SS_SSID                  0xFFFF0000 // Subsystem ID
#define B_PCH_SATA_SS_SSVID                 0x0000FFFF // Subsystem Vendor ID

#define R_PCH_SATA_AHCI_CAP_PTR             0x34  // Capabilities Pointer (8 bits)
#define B_PCH_SATA_AHCI_CAP_PTR             0xFF

#define R_PCH_SATA_INTR                     0x3C  // Interrupt Information
#define B_PCH_SATA_INTR_IPIN                0xFFFF0000 // Interrupt Pin
#define B_PCH_SATA_INTR_ILINE               0x0000FFFF // Interrupt Line

#define R_PCH_SATA_PMCS                     0x74  // PCI Power Management Control and Status
#define B_PCH_SATA_PMCS_PMES                BIT15 // PME Status
#define B_PCH_SATA_PMCS_PMEE                BIT8  // PME Enable
#define B_PCH_SATA_PMCS_NSFRST              BIT3  // No Soft Reset
#define V_PCH_SATA_PMCS_NSFRST_1            0x01
#define V_PCH_SATA_PMCS_NSFRST_0            0x00
#define B_PCH_SATA_PMCS_PS                  (BIT1 | BIT0) // Power State
#define V_PCH_SATA_PMCS_PS_3                0x03
#define V_PCH_SATA_PMCS_PS_0                0x00

#define R_PCH_SATA_MAP                      0x90  // Port Mapping Register
#define B_PCH_SATA_MAP_SPD                  (BIT14 | BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8) // SATA Port Disable
#define B_PCH_SATA_PORT6_DISABLED           BIT14
#define B_PCH_SATA_PORT5_DISABLED           BIT13
#define B_PCH_SATA_PORT4_DISABLED           BIT12
#define B_PCH_SATA_PORT3_DISABLED           BIT11
#define B_PCH_SATA_PORT2_DISABLED           BIT10
#define B_PCH_SATA_PORT1_DISABLED           BIT9
#define B_PCH_SATA_PORT0_DISABLED           BIT8
#define B_PCH_SATA_MAP_SMS_MASK             (BIT7 | BIT6) // SATA Mode Select
#define V_PCH_SATA_MAP_SMS_IDE              0x00
#define V_PCH_SATA_MAP_SMS_AHCI             0x40
#define V_PCH_SATA_MAP_SMS_RAID             0x80
#define B_PCH_SATA_PORT_TO_CONTROLLER_CFG   BIT5  // SATA Port-to-Controller Configuration

#define R_PCH_SATA_PCS                      0x92  // Port Control and Status
#define S_PCH_SATA_PCS                      0x2
#define B_PCH_SATA_PCS_OOB_RETRY            BIT15 // OOB Retry Mode
#define B_PCH_SATA_PCS_PORT6_DET            BIT14 // Port 6 Present
#define B_PCH_SATA_PCS_PORT5_DET            BIT13 // Port 5 Present
#define B_PCH_SATA_PCS_PORT4_DET            BIT12 // Port 4 Present
#define B_PCH_SATA_PCS_PORT3_DET            BIT11 // Port 3 Present
#define B_PCH_SATA_PCS_PORT2_DET            BIT10 // Port 2 Present
#define B_PCH_SATA_PCS_PORT1_DET            BIT9  // Port 1 Present
#define B_PCH_SATA_PCS_PORT0_DET            BIT8  // Port 0 Present
#define B_PCH_SATA_PCS_PORT5_EN             BIT5  // Port 5 Enabled
#define B_PCH_SATA_PCS_PORT4_EN             BIT4  // Port 4 Enabled
#define B_PCH_SATA_PCS_PORT3_EN             BIT3  // Port 3 Enabled
#define B_PCH_SATA_PCS_PORT2_EN             BIT2  // Port 2 Enabled
#define B_PCH_SATA_PCS_PORT1_EN             BIT1  // Port 1 Enabled
#define B_PCH_SATA_PCS_PORT0_EN             BIT0  // Port 0 Enabled

#define R_PCH_SATA_AHCI_PI                  0x0C  // Ports Implemented
#define B_PCH_SATA_PORT_MASK                0x3F
#define B_PCH_SATA_PORT5_IMPLEMENTED        BIT5  // Port 5 Implemented
#define B_PCH_SATA_PORT4_IMPLEMENTED        BIT4  // Port 4 Implemented
#define B_PCH_SATA_PORT3_IMPLEMENTED        BIT3  // Port 3 Implemented
#define B_PCH_SATA_PORT2_IMPLEMENTED        BIT2  // Port 2 Implemented
#define B_PCH_SATA_PORT1_IMPLEMENTED        BIT1  // Port 1 Implemented
#define B_PCH_SATA_PORT0_IMPLEMENTED        BIT0  // Port 0 Implemented

#define R_PCH_SATA_AHCI_P0SSTS              0x128 // Port 0 Serial ATA Status
#define R_PCH_SATA_AHCI_P1SSTS              0x1A8 // Port 1 Serial ATA Status
#define B_PCH_SATA_AHCI_PXSSTS_IPM          0x00000F00 // Interface Power Management
#define B_PCH_SATA_AHCI_PXSSTS_IPM_0        0x00000000
#define B_PCH_SATA_AHCI_PXSSTS_IPM_1        0x00000100
#define B_PCH_SATA_AHCI_PXSSTS_IPM_2        0x00000200
#define B_PCH_SATA_AHCI_PXSSTS_IPM_6        0x00000600
#define B_PCH_SATA_AHCI_PXSSTS_SPD          0x000000F0 // Current Interface Speed
#define B_PCH_SATA_AHCI_PXSSTS_SPD_0        0x00000000
#define B_PCH_SATA_AHCI_PXSSTS_SPD_1        0x00000010
#define B_PCH_SATA_AHCI_PXSSTS_SPD_2        0x00000020
#define B_PCH_SATA_AHCI_PXSSTS_SPD_3        0x00000030
#define B_PCH_SATA_AHCI_PXSSTS_DET          0x0000000F // Device Detection
#define B_PCH_SATA_AHCI_PXSSTS_DET_0        0x00000000
#define B_PCH_SATA_AHCI_PXSSTS_DET_1        0x00000001
#define B_PCH_SATA_AHCI_PXSSTS_DET_3        0x00000003
#define B_PCH_SATA_AHCI_PXSSTS_DET_4        0x00000004

//
// Macros of VLV capabilities for SATA controller which are used by SATA controller driver
//
//
//
// Define the individual capabilities of each SATA controller
//
#define PCH_SATA_MAX_CONTROLLERS            1     // Max SATA controllers number supported
#define PCH_SATA_MAX_DEVICES                2     // Max SATA devices number of single SATA channel
#define PCH_IDE_MAX_CHANNELS                2     // Max IDE channels number of single SATA controller
#define PCH_IDE_MAX_DEVICES                 2     // Max IDE devices number of single SATA channel
#define PCH_AHCI_MAX_PORTS                  2     // Max number of SATA ports in VLV
#define PCH_IDE_MAX_PORTS                   2     // Max number of IDE ports in VLV

//
// GPIOS_14 SATA0GP is the SATA port 0 reset pin.
//
#define PCH_GPIO_SATA_PORT0_RESET           14
//
// GPIOS_15 SATA1GP is the SATA port 1 reset pin.
//
#define PCH_GPIO_SATA_PORT1_RESET           15

#endif
