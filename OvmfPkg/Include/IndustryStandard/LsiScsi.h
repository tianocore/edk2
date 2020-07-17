/** @file

  Macros and type definitions for LSI 53C895A SCSI devices.

  Copyright (C) 2020, SUSE LLC.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LSI_SCSI_H_
#define _LSI_SCSI_H_

//
// Device ID
//
#define LSI_LOGIC_PCI_VENDOR_ID   0x1000
#define LSI_53C895A_PCI_DEVICE_ID 0x0012

//
// LSI 53C895A Registers
//
#define LSI_REG_DSTAT             0x0C
#define LSI_REG_ISTAT0            0x14
#define LSI_REG_DSP               0x2C
#define LSI_REG_SIST0             0x42
#define LSI_REG_SIST1             0x43

//
// The status bits for Interrupt Status Zero (ISTAT0)
//
#define LSI_ISTAT0_DIP            BIT0
#define LSI_ISTAT0_SIP            BIT1
#define LSI_ISTAT0_INTF           BIT2
#define LSI_ISTAT0_CON            BIT3
#define LSI_ISTAT0_SEM            BIT4
#define LSI_ISTAT0_SIGP           BIT5
#define LSI_ISTAT0_SRST           BIT6
#define LSI_ISTAT0_ABRT           BIT7

#endif // _LSI_SCSI_H_
