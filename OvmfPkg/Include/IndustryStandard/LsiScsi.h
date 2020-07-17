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
#define LSI_REG_CSBC              0xDC

//
// The status bits for DMA Status (DSTAT)
//
#define LSI_DSTAT_IID             BIT0
#define LSI_DSTAT_R               BIT1
#define LSI_DSTAT_SIR             BIT2
#define LSI_DSTAT_SSI             BIT3
#define LSI_DSTAT_ABRT            BIT4
#define LSI_DSTAT_BF              BIT5
#define LSI_DSTAT_MDPE            BIT6
#define LSI_DSTAT_DFE             BIT7

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

//
// The status bits for SCSI Interrupt Status Zero (SIST0)
//
#define LSI_SIST0_PAR             BIT0
#define LSI_SIST0_RST             BIT1
#define LSI_SIST0_UDC             BIT2
#define LSI_SIST0_SGE             BIT3
#define LSI_SIST0_RSL             BIT4
#define LSI_SIST0_SEL             BIT5
#define LSI_SIST0_CMP             BIT6
#define LSI_SIST0_MA              BIT7

//
// The status bits for SCSI Interrupt Status One (SIST1)
//
#define LSI_SIST1_HTH             BIT0
#define LSI_SIST1_GEN             BIT1
#define LSI_SIST1_STO             BIT2
#define LSI_SIST1_R3              BIT3
#define LSI_SIST1_SBMC            BIT4
#define LSI_SIST1_R5              BIT5
#define LSI_SIST1_R6              BIT6
#define LSI_SIST1_R7              BIT7

//
// LSI 53C895A Script Instructions
//
#define LSI_INS_TYPE_BLK          0x00000000
#define LSI_INS_TYPE_IO           BIT30
#define LSI_INS_TYPE_TC           BIT31

#define LSI_INS_BLK_SCSIP_DAT_OUT 0x00000000
#define LSI_INS_BLK_SCSIP_DAT_IN  BIT24
#define LSI_INS_BLK_SCSIP_CMD     BIT25
#define LSI_INS_BLK_SCSIP_STAT    (BIT24 | BIT25)
#define LSI_INS_BLK_SCSIP_MSG_OUT (BIT25 | BIT26)
#define LSI_INS_BLK_SCSIP_MSG_IN  (BIT24 | BIT25 | BIT26)

#define LSI_INS_IO_OPC_SEL        0x00000000
#define LSI_INS_IO_OPC_WAIT_RESEL BIT28

#define LSI_INS_TC_CP             BIT17
#define LSI_INS_TC_JMP            BIT19
#define LSI_INS_TC_RA             BIT23

#define LSI_INS_TC_OPC_JMP        0x00000000
#define LSI_INS_TC_OPC_INT        (BIT27 | BIT28)

#define LSI_INS_TC_SCSIP_DAT_OUT  0x00000000
#define LSI_INS_TC_SCSIP_MSG_IN   (BIT24 | BIT25 | BIT26)

#endif // _LSI_SCSI_H_
