/** @file
  Header file for IDE Bus Driver, containing the helper functions'
  entire prototype.

  Copyright (c) 2006 - 2007 Intel Corporation. <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  2002-6: Add Atapi6 enhancement, support >120GB hard disk, including
  Add - IDEBlkIoReadBlocksExt() func definition
  Add - IDEBlkIoWriteBlocksExt() func definition

**/

#ifndef _IDE_H
#define _IDE_H

//
// Helper functions Prototype
//
/**
  TODO: Add function description

  @param  This TODO: add argument description
  @param  Controller TODO: add argument description
  @param  Handle TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DeRegisterIdeDevice (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
;

/**
  TODO: Add function description

  @param  Controller TODO: add argument description
  @param  PciIo TODO: add argument description
  @param  ParentDevicePath TODO: add argument description
  @param  RemainingDevicePath TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EnableIdeDevice (
  IN EFI_HANDLE                          Controller,
  IN EFI_PCI_IO_PROTOCOL                 *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL            *RemainingDevicePath
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  Port TODO: add argument description

  TODO: add return values

**/
UINT8
IDEReadPortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  Port TODO: add argument description
  @param  Count TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
VOID
IDEReadPortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  OUT  VOID                 *Buffer
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  Port TODO: add argument description
  @param  Data TODO: add argument description

  TODO: add return values

**/
VOID
IDEWritePortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT8                 Data
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  Port TODO: add argument description
  @param  Data TODO: add argument description

  TODO: add return values

**/
VOID
IDEWritePortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT16                Data
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  Port TODO: add argument description
  @param  Count TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
VOID
IDEWritePortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  )
;

/**
  TODO: Add function description

  @param  PciIo TODO: add argument description
  @param  IdeRegsBaseAddr TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
GetIdeRegistersBaseAddr (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  OUT IDE_REGISTERS_BASE_ADDR     *IdeRegsBaseAddr
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
ReassignIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DiscoverIdeDevice (
  IN IDE_BLK_IO_DEV *IdeDev
  )
;

/**
  This interface is used to initialize all state data related to the
  detection of one channel.

  @retval EFI_SUCCESS Completed successfully.

**/
EFI_STATUS
InitializeIDEChannelData (
  VOID
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DetectIDEController (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TimeoutInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DRQClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TimeoutInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DRQClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TimeoutInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DRQReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TimeoutInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DRQReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TimeoutInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
WaitForBSYClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TimeoutInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
WaitForBSYClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DelayInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DRDYReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DelayInMilliSeconds TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
DRDYReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
;

/**
  TODO: Add function description

  @param  Destination TODO: add argument description
  @param  Source TODO: add argument description
  @param  Size TODO: add argument description

  TODO: add return values

**/
VOID
SwapStringChars (
  IN CHAR8  *Destination,
  IN CHAR8  *Source,
  IN UINT32 Size
  )
;

//
//  ATA device functions' prototype
//
/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
ATAIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
VOID
PrintAtaModuleName (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  Head TODO: add argument description
  @param  SectorCount TODO: add argument description
  @param  SectorNumber TODO: add argument description
  @param  CylinderLsb TODO: add argument description
  @param  CylinderMsb TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaPioDataIn (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  UINT8           Head,
  IN  UINT8           SectorCount,
  IN  UINT8           SectorNumber,
  IN  UINT8           CylinderLsb,
  IN  UINT8           CylinderMsb
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  Head TODO: add argument description
  @param  SectorCount TODO: add argument description
  @param  SectorNumber TODO: add argument description
  @param  CylinderLsb TODO: add argument description
  @param  CylinderMsb TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaPioDataOut (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  UINT8           Head,
  IN  UINT8           SectorCount,
  IN  UINT8           SectorNumber,
  IN  UINT8           CylinderLsb,
  IN  UINT8           CylinderMsb
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
CheckErrorStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  Lba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  BufferData TODO: add argument description
  @param  Lba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *BufferData,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeBlkIoDevice TODO: add argument description
  @param  MediaId TODO: add argument description
  @param  LBA TODO: add argument description
  @param  BufferSize TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
;

/**
  TODO: Add function description

  @param  IdeBlkIoDevice TODO: add argument description
  @param  MediaId TODO: add argument description
  @param  LBA TODO: add argument description
  @param  BufferSize TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
;

//
// ATAPI device functions' prototype
//
/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
ATAPIIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiInquiry (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Packet TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  TimeOut TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiPacketCommandIn (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Packet TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  TimeOut TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiPacketCommandOut (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  Read TODO: add argument description
  @param  TimeOut TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
PioReadWriteData (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT16          *Buffer,
  IN  UINT32          ByteCount,
  IN  BOOLEAN         Read,
  IN  UINTN           TimeOut
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiTestUnitReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT SENSE_RESULT	  *SResult  
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  SenseCounts TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiRequestSense (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT UINTN           *SenseCounts
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiReadCapacity (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT SENSE_RESULT	  *SResult  
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  MediaChange TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiDetectMedia (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT BOOLEAN         *MediaChange
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  Lba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  Lba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeBlkIoDevice TODO: add argument description
  @param  MediaId TODO: add argument description
  @param  LBA TODO: add argument description
  @param  BufferSize TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
;

/**
  TODO: Add function description

  @param  IdeBlkIoDevice TODO: add argument description
  @param  MediaId TODO: add argument description
  @param  LBA TODO: add argument description
  @param  BufferSize TODO: add argument description
  @param  Buffer TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  SenseCount TODO: add argument description
  @param  Result TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
ParseSenseData (
  IN IDE_BLK_IO_DEV     *IdeDev,
  IN UINTN              SenseCount,
  OUT SENSE_RESULT      *Result
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtapiReadPendingData (
  IN IDE_BLK_IO_DEV     *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  WriteProtected TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
IsLS120orZipWriteProtected (
  IN  IDE_BLK_IO_DEV    *IdeDev,
  OUT BOOLEAN           *WriteProtected
  )
;

/**
  TODO: Add function description

  @param  IdeBlkIoDevice TODO: add argument description

  TODO: add return values

**/
VOID
ReleaseIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeBlkIoDevice
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  TransferMode TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
SetDeviceTransferMode (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_TRANSFER_MODE    *TransferMode
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  NativeMaxAddress TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
ReadNativeMaxAddress (
  IN  IDE_BLK_IO_DEV                *IdeDev,
  OUT EFI_LBA                       *NativeMaxAddress
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  MaxAddress TODO: add argument description
  @param  bVolatile TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
SetMaxAddress (
  IN  IDE_BLK_IO_DEV                *IdeDev,
  IN  EFI_LBA                       MaxAddress,
  IN  BOOLEAN                       bVolatile
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  Device TODO: add argument description
  @param  Feature TODO: add argument description
  @param  SectorCount TODO: add argument description
  @param  LbaLow TODO: add argument description
  @param  LbaMiddle TODO: add argument description
  @param  LbaHigh TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaNonDataCommandIn (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT8           Feature,
  IN  UINT8           SectorCount,
  IN  UINT8           LbaLow,
  IN  UINT8           LbaMiddle,
  IN  UINT8           LbaHigh
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  Device TODO: add argument description
  @param  Feature TODO: add argument description
  @param  SectorCount TODO: add argument description
  @param  LbaAddress TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaNonDataCommandInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaReadSectorsExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaWriteSectorsExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaUdmaReadExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaUdmaRead (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaUdmaWriteExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  Perform an ATA Udma operation (Read, ReadExt, Write, WriteExt).
  
  @param[in] *IdeDev
  pointer pointing to IDE_BLK_IO_DEV data structure, used
  to record all the information of the IDE device.

  @param[in] *DataBuffer
  A pointer to the source buffer for the data.

  @param[in] StartLba
  The starting logical block address to write to
  on the device media.

  @param[in] NumberOfBlocks
  The number of transfer data blocks.
  
  @param[in] UdmaOp
  The perform operations could be AtaUdmaReadOp, AtaUdmaReadExOp,
  AtaUdmaWriteOp, AtaUdmaWriteExOp

  @return The device status of UDMA operation. If the operation is
  successful, return EFI_SUCCESS.

**/
EFI_STATUS
DoAtaUdma (
  IN  IDE_BLK_IO_DEV      *IdeDev,
  IN  VOID                *DataBuffer,
  IN  EFI_LBA             StartLba,
  IN  UINTN               NumberOfBlocks,
  IN  ATA_UDMA_OPERATION  UdmaOp
  )
;


/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DataBuffer TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  NumberOfBlocks TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaUdmaWrite (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  Device TODO: add argument description
  @param  Feature TODO: add argument description
  @param  SectorCount TODO: add argument description
  @param  LbaAddress TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaCommandIssueExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  Device TODO: add argument description
  @param  Feature TODO: add argument description
  @param  SectorCount TODO: add argument description
  @param  LbaAddress TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaCommandIssue (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaAtapi6Identify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;


/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
VOID
AtaSMARTSupport (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  SectorCount TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaPioDataInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  OUT VOID        *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  EFI_LBA         StartLba,
  IN  UINT16          SectorCount
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  Buffer TODO: add argument description
  @param  ByteCount TODO: add argument description
  @param  AtaCommand TODO: add argument description
  @param  StartLba TODO: add argument description
  @param  SectorCount TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
AtaPioDataOutExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  EFI_LBA         StartLba,
  IN  UINT16          SectorCount
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description
  @param  DriveParameters TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
SetDriveParameters (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_DRIVE_PARMS      *DriveParameters
  )
;

/**
  TODO: Add function description

  @param  IdeDev TODO: add argument description

  TODO: add return values

**/
EFI_STATUS
EnableInterrupt (
  IN IDE_BLK_IO_DEV       *IdeDev
  )
;

/**
  Clear pending IDE interrupt before OS loader/kernel take control of the IDE device.

  @param[in]  Event   Pointer to this event
  @param[in]  Context Event hanlder private data

  @retval  EFI_SUCCESS - Interrupt cleared

**/
VOID
EFIAPI
ClearInterrupt (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
;

#endif
