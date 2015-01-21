/**@file
  Clock generator setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
**/

#ifndef _BOARD_CLK_GEN_H_
#define _BOARD_CLK_GEN_H_

#include <PiPei.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/SmbusLib.h>
#include <Ppi/Smbus.h>
#include <IndustryStandard/SmBus.h>
#include <Guid/PlatformInfo.h>


#define CLOCK_GENERATOR_ADDRESS  0xd2

#define CLOCK_GENERATOR_SEETINGS_TABLET {0xB1, 0x82, 0xFF, 0xBF, 0xFF, 0x80}
#define CLOCK_GENERATOR_SETTINGS_MOBILE {0xB1, 0x82, 0xFF, 0xBF, 0xFF, 0x80}
#define CLOCK_GENERATOR_SETTINGS_DESKTOP {0xB1, 0x82, 0xFF, 0xBF, 0xFF, 0x80}

typedef enum {
  ClockGeneratorCk410,
  ClockGeneratorCk505,
  ClockGeneratorMax
} CLOCK_GENERATOR_TYPE;

typedef struct {
  CLOCK_GENERATOR_TYPE      ClockType;
  UINT8                     ClockId;
  UINT8                     SpreadSpectrumByteOffset;
  UINT8                     SpreadSpectrumBitOffset;
} CLOCK_GENERATOR_DETAILS;

#define MAX_CLOCK_GENERATOR_BUFFER_LENGTH           0x20

//
// CK410 Definitions
//
#define CK410_GENERATOR_ID                          0x65
#define CK410_GENERATOR_SPREAD_SPECTRUM_BYTE        1
#define CK410_GENERATOR_SPREAD_SPECTRUM_BIT         BIT0
#define CK410_GENERATOR_CLOCK_FREERUN_BYTE          4
#define CK410_GENERATOR_CLOCK_FREERUN_BIT           (BIT0 | BIT1 | BIT2)

//
// CK505 Definitions
//
#define VF_CK505_GENERATOR_ID                       0x5
#define CK505_GENERATOR_ID                          0x5  // Confirmed readout is 5
#define CK505_GENERATOR_SPREAD_SPECTRUM_BYTE        4
#define CK505_GENERATOR_SPREAD_SPECTRUM_BIT         (BIT0 | BIT1)
#define CK505_GENERATOR_PERCENT_SPREAD_BYTE      1
#define CK505_GENERATOR_PERCENT_MASK        ~(0xE)
#define CK505_GENERATOR_PERCENT_250_VALUE      0xC
#define CK505_GENERATOR_PERCENT_050_VALUE      0x4
#define CK505_GENERATOR_PERCENT_000_VALUE      0x2

//
// IDT Definitions
//
#define IDT_GENERATOR_ID_REVA                       0x1    //IDT Rev A
#define IDTRevA_GENERATOR_SPREAD_SPECTRUM_BYTE        0
#define IDTRevA_GENERATOR_SPREAD_SPECTRUM_BIT       BIT0
#define IDTRevA_GENERATOR_PERCENT_SPREAD_BYTE      5
#define IDTRevA_GENERATOR_PERCENT_250_VALUE      0xF
#define IDTRevA_GENERATOR_PERCENT_050_VALUE      0x3
#define IDTRevA_GENERATOR_PERCENT_000_VALUE      0xE
#define IDTRevA_GENERATOR_PERCENT_MASK        ~(0xF)

#define IDT_GENERATOR_ID_REVB                       0x11  //IDT RevB
#define IDT_GENERATOR_ID_REVD                       0x21  //IDT RevD

//
// CLOCK CONTROLLER
// SmBus address to read DIMM SPD
//
#define SMBUS_BASE_ADDRESS                  0xEFA0
#define SMBUS_BUS_DEV_FUNC                  0x1F0300
#define PLATFORM_NUM_SMBUS_RSVD_ADDRESSES   4
#define SMBUS_ADDR_CH_A_1                   0xA0
#define SMBUS_ADDR_CH_A_2                   0xA2
#define SMBUS_ADDR_CH_B_1                   0xA4
#define SMBUS_ADDR_CH_B_2                   0xA6

//
// Bits for FWH_DEC_EN1—Firmware Hub Decode Enable Register (LPC I/F—D31:F0)
//
#define   B_ICH_LPC_FWH_BIOS_DEC_F0             0x4000
#define   B_ICH_LPC_FWH_BIOS_DEC_E0             0x1000
#define   B_ICH_LPC_FWH_BIOS_DEC_E8             0x2000
#define   B_ICH_LPC_FWH_BIOS_LEG_F              0x0080
#define   B_ICH_LPC_FWH_BIOS_LEG_E              0x0040


//
// An arbitrary maximum length for clock generator buffers
//
#define MAX_CLOCK_GENERATOR_BUFFER_LENGTH           0x20

//
// SmBus Bus Device Function and Register Definitions
//
#define SMBUS_BUS_NUM          0
#define SMBUS_DEV_NUM          31
#define SMBUS_FUNC_NUM          3
#define SMBUS_BUS_DEV_FUNC_NUM    \
      SB_PCI_CFG_ADDRESS(SMBUS_BUS_NUM, SMBUS_DEV_NUM, SMBUS_FUNC_NUM, 0)

//
//ICH7: SMBus I/O Space Equates;
//
#define  BIT_SLAVE_ADDR    BIT00
#define  BIT_COMMAND      BIT01
#define  BIT_DATA      BIT02
#define  BIT_COUNT      BIT03
#define  BIT_WORD      BIT04
#define  BIT_CONTROL      BIT05
#define  BIT_PEC        BIT06
#define  BIT_READ      BIT07
#define  SMBUS_IO_READ_BIT  BIT00


#define  SMB_CMD_QUICK      0x00
#define  SMB_CMD_BYTE      0x04
#define  SMB_CMD_BYTE_DATA    0x08
#define  SMB_CMD_WORD_DATA    0x0C
#define  SMB_CMD_PROCESS_CALL  0x10
#define  SMB_CMD_BLOCK      0x14
#define  SMB_CMD_I2C_READ    0x18
#define  SMB_CMD_RESERVED    0x1c

#define  HST_STS_BYTE_DONE 0x80
#define SMB_HST_STS    0x000
#define SMB_HST_CNT    0x002
#define SMB_HST_CMD    0x003
#define SMB_HST_ADD    0x004
#define SMB_HST_DAT_0    0x005
#define SMB_HST_DAT_1    0x006
#define SMB_HST_BLK_DAT   0x007
#define SMB_PEC     0x008
#define SMB_RCV_SLVA    0x009
#define SMB_SLV_DAT    0x00A
#define SMB_AUX_STS    0x00C
#define SMB_AUX_CTL    0x00D
#define SMB_SMLINK_PIN_CTL  0x00E
#define SMB_SMBUS_PIN_CTL  0x00F
#define SMB_SLV_STS    0x010
#define SMB_SLV_CMD    0x011
#define SMB_NTFY_DADDR    0x014
#define SMB_NTFY_DLOW    0x016
#define SMB_NTFY_DHIGH    0x017

//
// PCI Register Definitions - use SmbusPolicyPpi->PciAddress + offset listed below
//
#define R_COMMAND                     0x04      // PCI Command Register, 16bit
#define   B_IOSE                        0x01    // RW
#define R_BASE_ADDRESS                0x20      // PCI BAR for SMBus I/O
#define   B_BASE_ADDRESS                0xFFE0  // RW
#define R_HOST_CONFIGURATION          0x40      // SMBus Host Configuration Register
#define   B_HST_EN                      0x01    // RW
#define   B_SMB_SMI_EN                  0x02    // RW
#define   B_I2C_EN                      0x04    // RW
//
// I/O Register Definitions - use SmbusPolicyPpi->BaseAddress + offset listed below
//
#define HOST_STATUS_REGISTER      0x00  // Host Status Register R/W
#define    HST_STS_HOST_BUSY                  0x01  // RO
#define   HST_STS_INTR                  0x02  // R/WC
#define   HST_STS_DEV_ERR                  0x04  // R/WC
#define   HST_STS_BUS_ERR                  0x08  // R/WC
#define   HST_STS_FAILED                  0x10  // R/WC
#define   SMBUS_B_SMBALERT_STS          0x20  // R/WC
#define   HST_STS_INUSE                   0x40  // R/WC
#define   SMBUS_B_BYTE_DONE_STS         0x80  // R/WC
#define   SMBUS_B_HSTS_ALL              0xFF  // R/WC
#define  HOST_CONTROL_REGISTER                  0x02  // Host Control Register R/W
#define    HST_CNT_INTREN                0x01  // RW
#define   HST_CNT_KILL                  0x02  // RW
#define   SMBUS_B_SMB_CMD               0x1C  // RW
#define     SMBUS_V_SMB_CMD_QUICK         0x00
#define     SMBUS_V_SMB_CMD_BYTE          0x04
#define     SMBUS_V_SMB_CMD_BYTE_DATA     0x08
#define     SMBUS_V_SMB_CMD_WORD_DATA     0x0C
#define     SMBUS_V_SMB_CMD_PROCESS_CALL  0x10
#define     SMBUS_V_SMB_CMD_BLOCK         0x14
#define     SMBUS_V_SMB_CMD_IIC_READ      0x18
#define   SMBUS_B_LAST_BYTE             0x20  // WO
#define   HST_CNT_START                 0x40  // WO
#define   HST_CNT_PEC_EN                0x80  // RW
#define  HOST_COMMAND_REGISTER                  0x03  // Host Command Register R/W
#define  XMIT_SLAVE_ADDRESS_REGISTER                   0x04  // Transmit Slave Address Register R/W
#define   SMBUS_B_RW_SEL                0x01  // RW
#define   SMBUS_B_ADDRESS               0xFE  // RW
#define  HOST_DATA_0_REGISTER                   0x05  // Data 0 Register R/W
#define  HOST_DATA_1_REGISTER                   0x06  // Data 1 Register R/W
#define  HOST_BLOCK_DATA_BYTE_REGISTER          0x07  // Host Block Data Register R/W
#define SMBUS_R_PEC                   0x08  // Packet Error Check Data Register R/W
#define SMBUS_R_RSA                   0x09  // Receive Slave Address Register R/W
#define   SMBUS_B_SLAVE_ADDR            0x7F  // RW
#define SMBUS_R_SD                    0x0A  // Receive Slave Data Register R/W
#define SMBUS_R_AUXS                  0x0C  // Auxiliary Status Register R/WC
#define   SMBUS_B_CRCE                  0x01  //R/WC
#define AUXILIARY_CONTROL_REGISTER                  0x0D  // Auxiliary Control Register R/W
#define   SMBUS_B_AAC                  0x01  //R/W
#define   SMBUS_B_E32B                 0x02  //R/W
#define  SMBUS_R_SMLC                  0x0E  // SMLINK Pin Control Register R/W
#define   SMBUS_B_SMLINK0_CUR_STS       0x01  // RO
#define   SMBUS_B_SMLINK1_CUR_STS       0x02  // RO
#define   SMBUS_B_SMLINK_CLK_CTL        0x04  // RW
#define SMBUS_R_SMBC                  0x0F  // SMBus Pin Control Register R/W
#define   SMBUS_B_SMBCLK_CUR_STS        0x01  // RO
#define   SMBUS_B_SMBDATA_CUR_STS       0x02  // RO
#define   SMBUS_B_SMBCLK_CTL            0x04  // RW
#define SMBUS_R_SSTS                  0x10  // Slave Status Register R/WC
#define   SMBUS_B_HOST_NOTIFY_STS       0x01  // R/WC
#define SMBUS_R_SCMD                  0x11  // Slave Command Register R/W
#define   SMBUS_B_HOST_NOTIFY_INTREN    0x01  // R/W
#define   SMBUS_B_HOST_NOTIFY_WKEN      0x02  // R/W
#define   SMBUS_B_SMBALERT_DIS          0x04  // R/W
#define SMBUS_R_NDA                   0x14  // Notify Device Address Register RO
#define   SMBUS_B_DEVICE_ADDRESS        0xFE  // RO
#define SMBUS_R_NDLB                  0x16  // Notify Data Low Byte Register RO
#define SMBUS_R_NDHB                  0x17  // Notify Data High Byte Register RO
#define BUS_TRIES           3       // How many times to retry on Bus Errors
#define SMBUS_NUM_RESERVED  21      // Number of device addresses that are
                                    //   reserved by the SMBus spec.
#define SMBUS_ADDRESS_ARP   0xC2 >> 1
#define   SMBUS_DATA_PREPARE_TO_ARP   0x01
#define   SMBUS_DATA_RESET_DEVICE     0x02
#define   SMBUS_DATA_GET_UDID_GENERAL 0x03
#define   SMBUS_DATA_ASSIGN_ADDRESS   0x04
#define SMBUS_GET_UDID_LENGTH 17    // 16 byte UDID + 1 byte address


EFI_STATUS
ConfigurePlatformClocks (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *SmbusPpi
  );


#endif
