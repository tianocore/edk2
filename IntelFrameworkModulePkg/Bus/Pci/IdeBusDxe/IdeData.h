/** @file
  Header file for IDE Bus Driver's Data Structures

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IDE_DATA_H_
#define _IDE_DATA_H_

#include <IndustryStandard/Atapi.h>

//
// common constants
//
#define STALL_1_MILLI_SECOND  1000    // stall 1 ms
#define STALL_1_SECOND        1000000 // stall 1 second
typedef enum {
  IdePrimary    = 0,
  IdeSecondary  = 1,
  IdeMaxChannel = 2
} EFI_IDE_CHANNEL;

typedef enum {
  IdeMaster     = 0,
  IdeSlave      = 1,
  IdeMaxDevice  = 2
} EFI_IDE_DEVICE;

typedef enum {
  IdeMagnetic,                        /* ZIP Drive or LS120 Floppy Drive */
  IdeCdRom,                           /* ATAPI CDROM */
  IdeHardDisk,                        /* Hard Disk */
  Ide48bitAddressingHardDisk,         /* Hard Disk larger than 120GB */
  IdeUnknown
} IDE_DEVICE_TYPE;

typedef enum {
  SenseNoSenseKey,
  SenseDeviceNotReadyNoRetry,
  SenseDeviceNotReadyNeedRetry,
  SenseNoMedia,
  SenseMediaChange,
  SenseMediaError,
  SenseOtherSense
} SENSE_RESULT;

typedef enum {
  AtaUdmaReadOp,
  AtaUdmaReadExtOp,
  AtaUdmaWriteOp,
  AtaUdmaWriteExtOp
} ATA_UDMA_OPERATION;

//
// IDE Registers
//
typedef union {
  UINT16  Command;        /* when write */
  UINT16  Status;         /* when read */
} IDE_CMD_OR_STATUS;

typedef union {
  UINT16  Error;          /* when read */
  UINT16  Feature;        /* when write */
} IDE_ERROR_OR_FEATURE;

typedef union {
  UINT16  AltStatus;      /* when read */
  UINT16  DeviceControl;  /* when write */
} IDE_ALTSTATUS_OR_DEVICECONTROL;

//
// IDE registers set
//
typedef struct {
  UINT16                          Data;
  IDE_ERROR_OR_FEATURE            Reg1;
  UINT16                          SectorCount;
  UINT16                          SectorNumber;
  UINT16                          CylinderLsb;
  UINT16                          CylinderMsb;
  UINT16                          Head;
  IDE_CMD_OR_STATUS               Reg;

  IDE_ALTSTATUS_OR_DEVICECONTROL  Alt;
  UINT16                          DriveAddress;

  UINT16                          MasterSlave;
  UINT16                          BusMasterBaseAddr;
} IDE_BASE_REGISTERS;

//
// IDE registers' base addresses
//
typedef struct {
  UINT16  CommandBlockBaseAddr;
  UINT16  ControlBlockBaseAddr;
  UINT16  BusMasterBaseAddr;
} IDE_REGISTERS_BASE_ADDR;

//
// Bit definitions in Programming Interface byte of the Class Code field
// in PCI IDE controller's Configuration Space
//
#define IDE_PRIMARY_OPERATING_MODE            BIT0
#define IDE_PRIMARY_PROGRAMMABLE_INDICATOR    BIT1
#define IDE_SECONDARY_OPERATING_MODE          BIT2
#define IDE_SECONDARY_PROGRAMMABLE_INDICATOR  BIT3


//
// Bus Master Reg
//
#define BMIC_NREAD      BIT3
#define BMIC_START      BIT0
#define BMIS_INTERRUPT  BIT2
#define BMIS_ERROR      BIT1

#define BMICP_OFFSET    0x00
#define BMISP_OFFSET    0x02
#define BMIDP_OFFSET    0x04
#define BMICS_OFFSET    0x08
#define BMISS_OFFSET    0x0A
#define BMIDS_OFFSET    0x0C

//
// Time Out Value For IDE Device Polling
//

//
// ATATIMEOUT is used for waiting time out for ATA device
//

//
// 1 second
//
#define ATATIMEOUT  1000

//
// ATAPITIMEOUT is used for waiting operation
// except read and write time out for ATAPI device
//

//
// 1 second
//
#define ATAPITIMEOUT  1000

//
// ATAPILONGTIMEOUT is used for waiting read and
// write operation timeout for ATAPI device
//

//
// 2 seconds
//
#define CDROMLONGTIMEOUT  2000

//
// 5 seconds
//
#define ATAPILONGTIMEOUT  5000

//
// 10 seconds
//
#define ATASMARTTIMEOUT   10000


//
// ATAPI6 related data structure definition
//

//
// The maximum sectors count in 28 bit addressing mode
//
#define MAX_28BIT_ADDRESSING_CAPACITY 0xfffffff

#pragma pack(1)

typedef struct {
  UINT32  RegionBaseAddr;
  UINT16  ByteCount;
  UINT16  EndOfTable;
} IDE_DMA_PRD;

#pragma pack()

#define SETFEATURE        TRUE
#define CLEARFEATURE      FALSE

///
/// PIO mode definition
///
typedef enum _ATA_PIO_MODE_ {
  AtaPioModeBelow2,
  AtaPioMode2,
  AtaPioMode3,
  AtaPioMode4
} ATA_PIO_MODE;

//
// Multi word DMA definition
//
typedef enum _ATA_MDMA_MODE_ {
  AtaMdmaMode0,
  AtaMdmaMode1,
  AtaMdmaMode2
} ATA_MDMA_MODE;

//
// UDMA mode definition
//
typedef enum _ATA_UDMA_MODE_ {
  AtaUdmaMode0,
  AtaUdmaMode1,
  AtaUdmaMode2,
  AtaUdmaMode3,
  AtaUdmaMode4,
  AtaUdmaMode5
} ATA_UDMA_MODE;

#define ATA_MODE_CATEGORY_DEFAULT_PIO 0x00
#define ATA_MODE_CATEGORY_FLOW_PIO    0x01
#define ATA_MODE_CATEGORY_MDMA        0x04
#define ATA_MODE_CATEGORY_UDMA        0x08

#pragma pack(1)

typedef struct {
  UINT8 ModeNumber : 3;
  UINT8 ModeCategory : 5;
} ATA_TRANSFER_MODE;

typedef struct {
  UINT8 Sector;
  UINT8 Heads;
  UINT8 MultipleSector;
} ATA_DRIVE_PARMS;

#pragma pack()
//
// IORDY Sample Point field value
//
#define ISP_5_CLK 0
#define ISP_4_CLK 1
#define ISP_3_CLK 2
#define ISP_2_CLK 3

//
// Recovery Time field value
//
#define RECVY_4_CLK 0
#define RECVY_3_CLK 1
#define RECVY_2_CLK 2
#define RECVY_1_CLK 3

//
// Slave IDE Timing Register Enable
//
#define SITRE BIT14

//
// DMA Timing Enable Only Select 1
//
#define DTE1  BIT7

//
// Pre-fetch and Posting Enable Select 1
//
#define PPE1  BIT6

//
// IORDY Sample Point Enable Select 1
//
#define IE1 BIT5

//
// Fast Timing Bank Drive Select 1
//
#define TIME1 BIT4

//
// DMA Timing Enable Only Select 0
//
#define DTE0  BIT3

//
// Pre-fetch and Posting Enable Select 0
//
#define PPE0  BIT2

//
// IOREY Sample Point Enable Select 0
//
#define IE0 BIT1

//
// Fast Timing Bank Drive Select 0
//
#define TIME0 BIT0

#endif
