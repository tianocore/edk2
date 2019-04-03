/**

Copyright (c) 2011  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



  @file
  PchRegsSmbus.h

  @brief
  Register names for VLV Smbus Device.

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
#ifndef _PCH_REGS_SMBUS_H_
#define _PCH_REGS_SMBUS_H_

///
/// SMBus Controller Registers (D31:F3)
///
#define PCI_DEVICE_NUMBER_PCH_SMBUS        31
#define PCI_FUNCTION_NUMBER_PCH_SMBUS      3

#define R_PCH_SMBUS_VENDOR_ID              0x00  // Vendor ID
#define V_PCH_SMBUS_VENDOR_ID              V_PCH_INTEL_VENDOR_ID // Intel Vendor ID

#define R_PCH_SMBUS_DEVICE_ID              0x02  // Device ID
#define V_PCH_SMBUS_DEVICE_ID              0x0F12

#define R_PCH_SMBUS_PCICMD                 0x04  // CMD register enables/disables, Memory/IO space access and interrupt
#define B_PCH_SMBUS_PCICMD_INTR_DIS        BIT10 // Interrupt Disable
#define B_PCH_SMBUS_PCICMD_FBE             BIT9  // FBE - reserved as '0'
#define B_PCH_SMBUS_PCICMD_SERR_EN         BIT8  // SERR Enable - reserved as '0'
#define B_PCH_SMBUS_PCICMD_WCC             BIT7  // Wait Cycle Control - reserved as '0'
#define B_PCH_SMBUS_PCICMD_PER             BIT6  // Parity Error - reserved as '0'
#define B_PCH_SMBUS_PCICMD_VPS             BIT5  // VGA Palette Snoop - reserved as '0'
#define B_PCH_SMBUS_PCICMD_PMWE            BIT4  // Postable Memory Write Enable - reserved as '0'
#define B_PCH_SMBUS_PCICMD_SCE             BIT3  // Special Cycle Enable - reserved as '0'
#define B_PCH_SMBUS_PCICMD_BME             BIT2  // Bus Master Enable - reserved as '0'
#define B_PCH_SMBUS_PCICMD_MSE             BIT1  // Memory Space Enable
#define B_PCH_SMBUS_PCICMD_IOSE            BIT0  // I/O Space Enable

#define R_PCH_SMBUS_BASE                   0x20  // The I/O memory bar
#define B_PCH_SMBUS_BASE_BAR               0x0000FFE0 // Base Address
#define B_PCH_SMBUS_BASE_IOSI              BIT0  // IO Space Indicator

#define R_PCH_SMBUS_SVID                   0x2C  // Subsystem Vendor ID
#define B_PCH_SMBUS_SVID                   0xFFFF // Subsystem Vendor ID

//
// SMBus I/O Registers
//
#define R_PCH_SMBUS_HSTS                   0x00  // Host Status Register R/W
#define B_PCH_SMBUS_HSTS_ALL               0xFF
#define B_PCH_SMBUS_BYTE_DONE_STS          BIT7  // Byte Done Status
#define B_PCH_SMBUS_IUS                    BIT6  // In Use Status
#define B_PCH_SMBUS_SMBALERT_STS           BIT5  // SMBUS Alert
#define B_PCH_SMBUS_FAIL                   BIT4  // Failed
#define B_PCH_SMBUS_BERR                   BIT3  // Bus Error
#define B_PCH_SMBUS_DERR                   BIT2  // Device Error
#define B_PCH_SMBUS_ERRORS                 (B_PCH_SMBUS_FAIL | B_PCH_SMBUS_BERR | B_PCH_SMBUS_DERR)
#define B_PCH_SMBUS_INTR                   BIT1  // Interrupt
#define B_PCH_SMBUS_HBSY                   BIT0  // Host Busy

#define R_PCH_SMBUS_HCTL                   0x02  // Host Control Register R/W
#define B_PCH_SMBUS_PEC_EN                 BIT7  // Packet Error Checking Enable
#define B_PCH_SMBUS_START                  BIT6  // Start
#define B_PCH_SMBUS_LAST_BYTE              BIT5  // Last Byte
#define B_PCH_SMBUS_SMB_CMD                0x1C  // SMB Command
#define V_PCH_SMBUS_SMB_CMD_BLOCK_PROCESS  0x1C  // Block Process
#define V_PCH_SMBUS_SMB_CMD_IIC_READ       0x18  // I2C Read
#define V_PCH_SMBUS_SMB_CMD_BLOCK          0x14  // Block
#define V_PCH_SMBUS_SMB_CMD_PROCESS_CALL   0x10  // Process Call
#define V_PCH_SMBUS_SMB_CMD_WORD_DATA      0x0C  // Word Data
#define V_PCH_SMBUS_SMB_CMD_BYTE_DATA      0x08  // Byte Data
#define V_PCH_SMBUS_SMB_CMD_BYTE           0x04  // Byte
#define V_PCH_SMBUS_SMB_CMD_QUICK          0x00  // Quick
#define B_PCH_SMBUS_KILL                   BIT1  // Kill
#define B_PCH_SMBUS_INTREN                 BIT0  // Interrupt Enable

#define R_PCH_SMBUS_HCMD                   0x03  // Host Command Register R/W
#define B_PCH_SMBUS_HCMD                   0xFF  // Command to be transmitted

#define R_PCH_SMBUS_TSA                    0x04  // Transmit Slave Address Register R/W
#define B_PCH_SMBUS_ADDRESS                0xFE  // 7-bit address of the targeted slave
#define B_PCH_SMBUS_RW_SEL                 BIT0  // Direction of the host transfer, 1 = read, 0 = write
#define B_PCH_SMBUS_RW_SEL_READ            0x01  // Read
#define B_PCH_SMBUS_RW_SEL_WRITE           0x00  // Write
//
#define R_PCH_SMBUS_HD0                    0x05  // Data 0 Register R/W
#define R_PCH_SMBUS_HD1                    0x06  // Data 1 Register R/W
#define R_PCH_SMBUS_HBD                    0x07  // Host Block Data Register R/W
#define R_PCH_SMBUS_PEC                    0x08  // Packet Error Check Data Register R/W

#define R_PCH_SMBUS_RSA                    0x09  // Receive Slave Address Register R/W
#define B_PCH_SMBUS_SLAVE_ADDR             0x7F  // TCO slave address (Not used, reserved)

#define R_PCH_SMBUS_SD                     0x0A  // Receive Slave Data Register R/W

#define R_PCH_SMBUS_AUXS                   0x0C  // Auxiliary Status Register R/WC
#define B_PCH_SMBUS_CRCE                   BIT0  // CRC Error
//
#define R_PCH_SMBUS_AUXC                   0x0D  // Auxiliary Control Register R/W
#define B_PCH_SMBUS_E32B                   BIT1  // Enable 32-byte Buffer
#define B_PCH_SMBUS_AAC                    BIT0  // Automatically Append CRC

#define R_PCH_SMBUS_SMLC                   0x0E  // SMLINK Pin Control Register R/W
#define B_PCH_SMBUS_SMLINK_CLK_CTL         BIT2  // Not supported
#define B_PCH_SMBUS_SMLINK1_CUR_STS        BIT1  // Not supported
#define B_PCH_SMBUS_SMLINK0_CUR_STS        BIT0  // Not supported


#define R_PCH_SMBUS_SMBC                   0x0F  // SMBus Pin Control Register R/W
#define B_PCH_SMBUS_SMBCLK_CTL             BIT2  // SMBCLK Control
#define B_PCH_SMBUS_SMBDATA_CUR_STS        BIT1  // SMBDATA Current Status
#define B_PCH_SMBUS_SMBCLK_CUR_STS         BIT0  // SMBCLK Current Status

#define R_PCH_SMBUS_SSTS                   0x10  // Slave Status Register R/WC
#define B_PCH_SMBUS_HOST_NOTIFY_STS        BIT0  // Host Notify Status

#define R_PCH_SMBUS_SCMD                   0x11  // Slave Command Register R/W
#define B_PCH_SMBUS_SMBALERT_DIS           BIT2  // Not supported
#define B_PCH_SMBUS_HOST_NOTIFY_WKEN       BIT1  // Host Notify Wake Enable
#define B_PCH_SMBUS_HOST_NOTIFY_INTREN     BIT0  // Host Notify Interrupt Enable

#define R_PCH_SMBUS_NDA                    0x14  // Notify Device Address Register RO
#define B_PCH_SMBUS_DEVICE_ADDRESS         0xFE  // Device Address

#define R_PCH_SMBUS_NDLB                   0x16  // Notify Data Low Byte Register RO
#define R_PCH_SMBUS_NDHB                   0x17  // Notify Data High Byte Register RO

#endif
