/** @file 
Definition of FDC registers and structures.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_RECOVERY_FDC_H_
#define _PEI_RECOVERY_FDC_H_

//
// FDC Registers
//
#define FDC_REGISTER_DOR  2 //Digital Output Register
#define FDC_REGISTER_MSR  4 //Main Status Register
#define FDC_REGISTER_DTR  5 //Data Register
#define FDC_REGISTER_CCR  7 //Configuration Control Register(data rate select)
#define FDC_REGISTER_DIR  7 //Digital Input Register(diskchange)
//
// FDC Register Bit Definitions
//
//
// Digital Out Register(WO)
//
#define SELECT_DRV      BIT0  // Select Drive: 0=A 1=B
#define RESET_FDC       BIT2  // Reset FDC
#define INT_DMA_ENABLE  BIT3  // Enable Int & DMA
#define DRVA_MOTOR_ON   BIT4  // Turn On Drive A Motor
#define DRVB_MOTOR_ON   BIT5  // Turn On Drive B Motor
//
// Main Status Register(RO)
//
#define MSR_DAB BIT0  // Drive A Busy
#define MSR_DBB BIT1  // Drive B Busy
#define MSR_CB  BIT4  // FDC Busy
#define MSR_NDM BIT5  // Non-DMA Mode
#define MSR_DIO BIT6  // Data Input/Output
#define MSR_RQM BIT7  // Request For Master
//
// Configuration Control Register(WO)
//
#define CCR_DRC (BIT0 | BIT1) // Data Rate select
//
// Digital Input Register(RO)
//
#define DIR_DCL     BIT7  // Disk change line
#define DRC_500KBS  0x0   // 500K
#define DRC_300KBS  0x01  // 300K
#define DRC_250KBS  0x02  // 250K
//
// FDC Command Code
//
#define READ_DATA_CMD         0x06
#define SEEK_CMD              0x0F
#define RECALIBRATE_CMD       0x07
#define SENSE_INT_STATUS_CMD  0x08
#define SPECIFY_CMD           0x03
#define SENSE_DRV_STATUS_CMD  0x04

///
/// CMD_MT: Multi_Track Selector
/// when set , this flag selects the multi-track operating mode.
/// In this mode, the FDC treats a complete cylinder under head0 and 1 as a single track
///
#define CMD_MT  BIT7

///
/// CMD_MFM: MFM/FM Mode Selector
/// A one selects the double density(MFM) mode
/// A zero selects single density (FM) mode
///
#define CMD_MFM BIT6

///
/// CMD_SK: Skip Flag
/// When set to 1, sectors containing a deleted data address mark will automatically be skipped
/// during the execution of Read Data.
/// When set to 0, the sector is read or written the same as the read and write commands.
///
#define CMD_SK  BIT5

//
// FDC Status Register Bit Definitions
//
//
// Status Register 0
//
#define STS0_IC (BIT7 | BIT6) // Interrupt Code
#define STS0_SE BIT5          // Seek End: the FDC completed a seek or recalibrate command
#define STS0_EC BIT4          // Equipment Check
#define STS0_NR BIT3          // Not Ready(unused), this bit is always 0
#define STS0_HA BIT2          // Head Address: the current head address
//
// STS0_US1 & STS0_US0: Drive Select(the current selected drive)
//
#define STS0_US1  BIT1  // Unit Select1
#define STS0_US0  BIT0  // Unit Select0
//
// Status Register 1
//
#define STS1_EN BIT7  // End of Cylinder
//
// BIT6 is unused
//
#define STS1_DE BIT5  // Data Error: The FDC detected a CRC error in either the ID field or data field of a sector
#define STS1_OR BIT4  // Overrun/Underrun: Becomes set if FDC does not receive CPU or DMA service within the required time interval
//
// BIT3 is unused
//
#define STS1_ND BIT2  // No data
#define STS1_NW BIT1  // Not Writable
#define STS1_MA BIT0  // Missing Address Mark

//
// Status Register 2
//
// BIT7 is unused
//
#define STS2_CM BIT6  // Control Mark
#define STS2_DD BIT5  // Data Error in Data Field: The FDC detected a CRC error in the data field
#define STS2_WC BIT4  // Wrong Cylinder: The track address from sector ID field is different from the track address maintained inside FDC
//
// BIT3 is unused
// BIT2 is unused
//
#define STS2_BC BIT1  // Bad Cylinder
#define STS2_MD BIT0  // Missing Address Mark in DataField

//
// Status Register 3
//
// BIT7 is unused
//
#define STS3_WP BIT6  // Write Protected
//
// BIT5 is unused
//
#define STS3_T0 BIT4  // Track 0
//
// BIT3 is unused
//
#define STS3_HD BIT2  // Head Address
//
// STS3_US1 & STS3_US0 : Drive Select
//
#define STS3_US1  BIT1  // Unit Select1
#define STS3_US0  BIT0  // Unit Select0

//
// Status Register 0 Interrupt Code Description
//
#define IC_NT   0x0   // Normal Termination of Command
#define IC_AT   0x40  // Abnormal Termination of Command
#define IC_IC   0x80  // Invalid Command
#define IC_ATRC 0xC0  // Abnormal Termination caused by Polling

///
/// Table of parameters for diskette
///
typedef struct {
  UINT8 EndOfTrack;          ///< End of track
  UINT8 GapLength;           ///< Gap length
  UINT8 DataLength;          ///< Data length
  UINT8 Number;              ///< Number of bytes per sector
  UINT8 MaxTrackNum;
  UINT8 MotorStartTime;
  UINT8 MotorOffTime;
  UINT8 HeadSettlingTime;
  UINT8 DataTransferRate;
} DISKET_PARA_TABLE;

///
/// Structure for FDC Command Packet 1
///
typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
  UINT8 Cylinder;
  UINT8 Head;
  UINT8 Sector;
  UINT8 Number;
  UINT8 EndOfTrack;
  UINT8 GapLength;
  UINT8 DataLength;
} FDC_COMMAND_PACKET1;

///
/// Structure for FDC Command Packet 2
///
typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
} FDC_COMMAND_PACKET2;

///
/// Structure for FDC Specify Command
///
typedef struct {
  UINT8 CommandCode;
  UINT8 SrtHut;
  UINT8 HltNd;
} FDC_SPECIFY_CMD;

///
/// Structure for FDC Seek Command
///
typedef struct {
  UINT8 CommandCode;
  UINT8 DiskHeadSel;
  UINT8 NewCylinder;
} FDC_SEEK_CMD;

///
/// Structure for FDC Result Packet
///
typedef struct {
  UINT8 Status0;
  UINT8 Status1;
  UINT8 Status2;
  UINT8 CylinderNumber;
  UINT8 HeaderAddress;
  UINT8 Record;
  UINT8 Number;
} FDC_RESULT_PACKET;

#endif
