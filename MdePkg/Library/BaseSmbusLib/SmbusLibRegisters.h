/** @file
  Base SMBUS library implementation built upon I/O library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SmbusLib.h

**/

#ifndef __SMBUS_LIB_REGISTER_H
#define __SMBUS_LIB_REGISTER_H

#define SMBUS_R_HST_STS               0x00  // Host Status Register
#define SMBUS_B_HOST_BUSY             0x01  // RO
#define SMBUS_B_INTR                  0x02  // R/WC
#define SMBUS_B_DEV_ERR               0x04  // R/WC
#define SMBUS_B_BUS_ERR               0x08  // R/WC
#define SMBUS_B_FAILED                0x10  // R/WC
#define SMBUS_B_SMBALERT_STS          0x20  // R/WC
#define SMBUS_B_INUSE_STS             0x40  // R/WC
#define SMBUS_B_BYTE_DONE_STS         0x80  // R/WC
#define SMBUS_B_ERROR                 (SMBUS_B_DEV_ERR | SMBUS_B_BUS_ERR | SMBUS_B_FAILED)
#define SMBUS_B_HSTS_ALL              0xFF  // R/WC


#define SMBUS_R_HST_CTL               0x02  // Host Control Register R/W
#define SMBUS_B_INTREN                0x01  // RW
#define SMBUS_B_KILL                  0x02  // RW
#define SMBUS_B_CMD                   (7 << 2) // RW
#define SMBUS_V_SMB_CMD_QUICK         (0 << 2)
#define SMBUS_V_SMB_CMD_BYTE          (1 << 2)
#define SMBUS_V_SMB_CMD_BYTE_DATA     (2 << 2)
#define SMBUS_V_SMB_CMD_WORD_DATA     (3 << 2)
#define SMBUS_V_SMB_CMD_PROCESS_CALL  (4 << 2)
#define SMBUS_V_SMB_CMD_BLOCK         (5 << 2)
#define SMBUS_V_SMB_CMD_IIC_READ      (6 << 2)
#define SMBUS_V_SMB_CMD_BLOCK_PROCESS (7 << 2)
#define SMBUS_B_LAST_BYTE             0x20  // WO
#define SMBUS_B_START                 0x40  // WO
#define SMBUS_B_PEC_EN                0x80  // RW


#define SMBUS_R_HST_CMD               0x03  // Host Command Register R/W


#define SMBUS_R_XMIT_SLVA             0x04  // Transmit Slave Address Register R/W
#define SMBUS_B_RW                    0x01  // RW
#define SMBUS_B_READ                  0x01  // RW
#define SMBUS_B_WRITE                 0x00  // RW
#define SMBUS_B_ADDRESS               0xFE  // RW


#define SMBUS_R_HST_D0                0x05  // Data 0 Register R/W


#define SMBUS_R_HST_D1                0x06  // Data 1 Register R/W


#define SMBUS_R_HOST_BLOCK_DB         0x07  // Host Block Data Register R/W


#define SMBUS_R_PEC                   0x08  // Packet Error Check Data Register R/W


#define SMBUS_R_RCV_SLVA              0x09  // Receive Slave Address Register R/W
#define SMBUS_B_SLAVE_ADDR            0x7F  // RW


#define SMBUS_R_SLV_DATA              0x0A  // Receive Slave Data Register R/W


#define SMBUS_R_AUX_STS               0x0C  // Auxiliary Status Register R/WC
#define SMBUS_B_CRCE                  0x01  // R/WC


#define SMBUS_R_AUX_CTL               0x0D  // Auxiliary Control Register R/W
#define SMBUS_B_AAC                   0x01  // R/W
#define SMBUS_B_E32B                  0x02  // R/W


#define SMBUS_R_SMLINK_PIN_CTL        0x0E  // SMLINK Pin Control Register R/W
#define SMBUS_B_SMLINK0_CUR_STS       0x01  // RO
#define SMBUS_B_SMLINK1_CUR_STS       0x02  // RO
#define SMBUS_B_SMLINK_CLK_CTL        0x04  // RW


#define SMBUS_R_SMBUS_PIN_CTL         0x0F  // SMBus Pin Control Register R/W
#define SMBUS_B_SMBCLK_CUR_STS        0x01  // RO
#define SMBUS_B_SMBDATA_CUR_STS       0x02  // RO
#define SMBUS_B_SMBCLK_CTL            0x04  // RW


#define SMBUS_R_SLV_STS               0x10  // Slave Status Register R/WC
#define SMBUS_B_HOST_NOTIFY_STS       0x01  // R/WC


#define SMBUS_R_SLV_CMD               0x11  // Slave Command Register R/W
#define SMBUS_B_HOST_NOTIFY_INTREN    0x01  // R/W
#define SMBUS_B_HOST_NOTIFY_WKEN      0x02  // R/W
#define SMBUS_B_SMBALERT_DIS          0x04  // R/W


#define SMBUS_R_NOTIFY_DADDR          0x14  // Notify Device Address Register RO
#define SMBUS_B_DEVICE_ADDRESS        0xFE  // RO


#define SMBUS_R_NOTIFY_DLOW           0x16  // Notify Data Low Byte Register RO


#define SMBUS_R_NOTIFY_DHIGH          0x17  // Notify Data High Byte Register RO   


#endif
