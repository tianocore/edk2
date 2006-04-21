/*++
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    ide.h

Abstract:

    Header file for IDE Bus Driver, containing the helper functions' 
    entire prototype.

Revision History

    2002-6: Add Atapi6 enhancement, support >120GB hard disk, including
            Add - IDEBlkIoReadBlocksExt() func definition
            Add - IDEBlkIoWriteBlocksExt() func definition
            
++*/

// TODO: fix comment to end with --*/
#ifndef _IDE_H
#define _IDE_H

//
// Helper functions Prototype
//
EFI_STATUS
DeRegisterIdeDevice (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  EFI_HANDLE                     Handle
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  Controller  - TODO: add argument description
  Handle      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EnableIdeDevice (
  IN EFI_HANDLE                          Controller,
  IN EFI_PCI_IO_PROTOCOL                 *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL            *RemainingDevicePath
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Controller          - TODO: add argument description
  PciIo               - TODO: add argument description
  ParentDevicePath    - TODO: add argument description
  RemainingDevicePath - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

UINT8
IDEReadPortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo - TODO: add argument description
  Port  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
IDEReadPortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  OUT  VOID                 *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo   - TODO: add argument description
  Port    - TODO: add argument description
  Count   - TODO: add argument description
  Buffer  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
IDEWritePortB (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT8                 Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo - TODO: add argument description
  Port  - TODO: add argument description
  Data  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
IDEWritePortW (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINT16                Data
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo - TODO: add argument description
  Port  - TODO: add argument description
  Data  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
IDEWritePortWMultiple (
  IN  EFI_PCI_IO_PROTOCOL   *PciIo,
  IN  UINT16                Port,
  IN  UINTN                 Count,
  IN  VOID                  *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo   - TODO: add argument description
  Port    - TODO: add argument description
  Count   - TODO: add argument description
  Buffer  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
GetIdeRegistersBaseAddr (
  IN  EFI_PCI_IO_PROTOCOL         *PciIo,
  OUT IDE_REGISTERS_BASE_ADDR     *IdeRegsBaseAddr
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  PciIo           - TODO: add argument description
  IdeRegsBaseAddr - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ReassignIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DiscoverIdeDevice (
  IN IDE_BLK_IO_DEV *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DetectIDEController (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DRQClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev                - TODO: add argument description
  TimeoutInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DRQClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev                - TODO: add argument description
  TimeoutInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DRQReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev                - TODO: add argument description
  TimeoutInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DRQReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev                - TODO: add argument description
  TimeoutInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
WaitForBSYClear (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev                - TODO: add argument description
  TimeoutInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
WaitForBSYClear2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           TimeoutInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev                - TODO: add argument description
  TimeoutInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DRDYReady (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev              - TODO: add argument description
  DelayInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DRDYReady2 (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINTN           DelayInMilliSeconds
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev              - TODO: add argument description
  DelayInMilliSeconds - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
SwapStringChars (
  IN CHAR8  *Destination,
  IN CHAR8  *Source,
  IN UINT32 Size
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  Destination - TODO: add argument description
  Source      - TODO: add argument description
  Size        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
//  ATA device functions' prototype
//
EFI_STATUS
ATAIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
PrintAtaModuleName (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev        - TODO: add argument description
  Buffer        - TODO: add argument description
  ByteCount     - TODO: add argument description
  AtaCommand    - TODO: add argument description
  Head          - TODO: add argument description
  SectorCount   - TODO: add argument description
  SectorNumber  - TODO: add argument description
  CylinderLsb   - TODO: add argument description
  CylinderMsb   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev        - TODO: add argument description
  Buffer        - TODO: add argument description
  ByteCount     - TODO: add argument description
  AtaCommand    - TODO: add argument description
  Head          - TODO: add argument description
  SectorCount   - TODO: add argument description
  SectorNumber  - TODO: add argument description
  CylinderLsb   - TODO: add argument description
  CylinderMsb   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
CheckErrorStatus (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  Lba             - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *BufferData,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  BufferData      - TODO: add argument description
  Lba             - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeBlkIoDevice  - TODO: add argument description
  MediaId         - TODO: add argument description
  LBA             - TODO: add argument description
  BufferSize      - TODO: add argument description
  Buffer          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeBlkIoDevice  - TODO: add argument description
  MediaId         - TODO: add argument description
  LBA             - TODO: add argument description
  BufferSize      - TODO: add argument description
  Buffer          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

//
// ATAPI device functions' prototype
//
EFI_STATUS
ATAPIIdentify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiInquiry (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiPacketCommandIn (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev    - TODO: add argument description
  Packet    - TODO: add argument description
  Buffer    - TODO: add argument description
  ByteCount - TODO: add argument description
  TimeOut   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiPacketCommandOut (
  IN  IDE_BLK_IO_DEV        *IdeDev,
  IN  ATAPI_PACKET_COMMAND  *Packet,
  IN  UINT16                *Buffer,
  IN  UINT32                ByteCount,
  IN  UINTN                 TimeOut
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev    - TODO: add argument description
  Packet    - TODO: add argument description
  Buffer    - TODO: add argument description
  ByteCount - TODO: add argument description
  TimeOut   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
PioReadWriteData (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT16          *Buffer,
  IN  UINT32          ByteCount,
  IN  BOOLEAN         Read,
  IN  UINTN           TimeOut
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev    - TODO: add argument description
  Buffer    - TODO: add argument description
  ByteCount - TODO: add argument description
  Read      - TODO: add argument description
  TimeOut   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiTestUnitReady (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiRequestSense (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT UINTN           *SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiReadCapacity (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiDetectMedia (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  OUT BOOLEAN         *MediaChange
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  MediaChange - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiReadSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  Buffer          - TODO: add argument description
  Lba             - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiWriteSectors (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  EFI_LBA         Lba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  Buffer          - TODO: add argument description
  Lba             - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiSoftReset (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiBlkIoReadBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeBlkIoDevice  - TODO: add argument description
  MediaId         - TODO: add argument description
  LBA             - TODO: add argument description
  BufferSize      - TODO: add argument description
  Buffer          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtapiBlkIoWriteBlocks (
  IN IDE_BLK_IO_DEV   *IdeBlkIoDevice,
  IN UINT32           MediaId,
  IN EFI_LBA          LBA,
  IN UINTN            BufferSize,
  OUT VOID            *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeBlkIoDevice  - TODO: add argument description
  MediaId         - TODO: add argument description
  LBA             - TODO: add argument description
  BufferSize      - TODO: add argument description
  Buffer          - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsNoMedia (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsMediaError (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsMediaChange (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
IsDriveReady (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts,
  OUT BOOLEAN               *NeedRetry
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description
  NeedRetry   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
HaveSenseKey (
  IN  REQUEST_SENSE_DATA    *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  SenseData   - TODO: add argument description
  SenseCounts - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
IsLS120orZipWriteProtected (
  IN  IDE_BLK_IO_DEV    *IdeDev,
  OUT BOOLEAN           *WriteProtected
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  WriteProtected  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
ReleaseIdeResources (
  IN  IDE_BLK_IO_DEV  *IdeBlkIoDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeBlkIoDevice  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SetDeviceTransferMode (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_TRANSFER_MODE    *TransferMode
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev        - TODO: add argument description
  TransferMode  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ReadNativeMaxAddress (
  IN  IDE_BLK_IO_DEV                *IdeDev,
  OUT EFI_LBA                       *NativeMaxAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev            - TODO: add argument description
  NativeMaxAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SetMaxAddress (
  IN  IDE_BLK_IO_DEV                *IdeDev,
  IN  EFI_LBA                       MaxAddress,
  IN  BOOLEAN                       bVolatile
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  MaxAddress  - TODO: add argument description
  bVolatile   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

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
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  AtaCommand  - TODO: add argument description
  Device      - TODO: add argument description
  Feature     - TODO: add argument description
  SectorCount - TODO: add argument description
  LbaLow      - TODO: add argument description
  LbaMiddle   - TODO: add argument description
  LbaHigh     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaNonDataCommandInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  AtaCommand  - TODO: add argument description
  Device      - TODO: add argument description
  Feature     - TODO: add argument description
  SectorCount - TODO: add argument description
  LbaAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaReadSectorsExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  StartLba        - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaWriteSectorsExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  StartLba        - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaUdmaReadExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  StartLba        - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaUdmaRead (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  StartLba        - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaUdmaWriteExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  StartLba        - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaUdmaWrite (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *DataBuffer,
  IN  EFI_LBA         StartLba,
  IN  UINTN           NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DataBuffer      - TODO: add argument description
  StartLba        - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaCommandIssueExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  AtaCommand  - TODO: add argument description
  Device      - TODO: add argument description
  Feature     - TODO: add argument description
  SectorCount - TODO: add argument description
  LbaAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaCommandIssue (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  UINT8           AtaCommand,
  IN  UINT8           Device,
  IN  UINT16          Feature,
  IN  UINT16          SectorCount,
  IN  EFI_LBA         LbaAddress
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  AtaCommand  - TODO: add argument description
  Device      - TODO: add argument description
  Feature     - TODO: add argument description
  SectorCount - TODO: add argument description
  LbaAddress  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaAtapi6Identify (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;


VOID
AtaSMARTSupport (
  IN  IDE_BLK_IO_DEV  *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaPioDataInExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  OUT VOID        *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  EFI_LBA         StartLba,
  IN  UINT16          SectorCount
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  Buffer      - TODO: add argument description
  ByteCount   - TODO: add argument description
  AtaCommand  - TODO: add argument description
  StartLba    - TODO: add argument description
  SectorCount - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
AtaPioDataOutExt (
  IN  IDE_BLK_IO_DEV  *IdeDev,
  IN  VOID            *Buffer,
  IN  UINT32          ByteCount,
  IN  UINT8           AtaCommand,
  IN  EFI_LBA         StartLba,
  IN  UINT16          SectorCount
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev      - TODO: add argument description
  Buffer      - TODO: add argument description
  ByteCount   - TODO: add argument description
  AtaCommand  - TODO: add argument description
  StartLba    - TODO: add argument description
  SectorCount - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
SetDriveParameters (
  IN IDE_BLK_IO_DEV       *IdeDev,
  IN ATA_DRIVE_PARMS      *DriveParameters
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev          - TODO: add argument description
  DriveParameters - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EnableInterrupt (
  IN IDE_BLK_IO_DEV       *IdeDev
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  IdeDev  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
