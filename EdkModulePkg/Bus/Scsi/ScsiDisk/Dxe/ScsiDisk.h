/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ScsiDisk.h

Abstract:
  
  Header file for SCSI Disk Driver.

--*/

#ifndef _SCSI_DISK_H
#define _SCSI_DISK_H


#include <IndustryStandard/scsi.h>

#define IsDeviceFixed(a)        (a)->FixedDevice ? 1 : 0

#define SCSI_DISK_DEV_SIGNATURE EFI_SIGNATURE_32 ('s', 'c', 'd', 'k')

typedef struct {
  UINT32                    Signature;

  EFI_HANDLE                Handle;

  EFI_BLOCK_IO_PROTOCOL     BlkIo;
  EFI_BLOCK_IO_MEDIA        BlkIoMedia;
  EFI_SCSI_IO_PROTOCOL      *ScsiIo;
  UINT8                     DeviceType;
  BOOLEAN                   FixedDevice;
  UINT16                    Reserved;

  EFI_SCSI_SENSE_DATA       *SenseData;
  UINTN                     SenseDataNumber;
  EFI_SCSI_INQUIRY_DATA     InquiryData;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

} SCSI_DISK_DEV;

#define SCSI_DISK_DEV_FROM_THIS(a)  CR (a, SCSI_DISK_DEV, BlkIo, SCSI_DISK_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gScsiDiskDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gScsiDiskComponentName;
//
// action code used in detect media process
//
#define ACTION_NO_ACTION            0x00
#define ACTION_READ_CAPACITY        0x01
#define ACTION_RETRY_COMMAND_LATER  0x02

EFI_STATUS
EFIAPI
ScsiDiskDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
ScsiDiskDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
ScsiDiskDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
ScsiDiskComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
ScsiDiskComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

EFI_STATUS
EFIAPI
ScsiDiskReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This                  - TODO: add argument description
  ExtendedVerification  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
ScsiDiskReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  LBA         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
ScsiDiskWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This        - TODO: add argument description
  MediaId     - TODO: add argument description
  LBA         - TODO: add argument description
  BufferSize  - TODO: add argument description
  Buffer      - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
EFIAPI
ScsiDiskFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskDetectMedia (
  SCSI_DISK_DEV   *ScsiDiskDevice,
  BOOLEAN         MustReadCap,
  BOOLEAN         *MediaChange
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description
  MustReadCap     - TODO: add argument description
  MediaChange     - TODO: add argument description

Returns:

  TODO: add return values

--*/
;
  
EFI_STATUS
ScsiDiskTestUnitReady (
  SCSI_DISK_DEV        *ScsiDiskDevice,
  BOOLEAN              *NeedRetry,
  EFI_SCSI_SENSE_DATA  **SenseDataArray,
  UINTN                *NumberOfSenseKeys
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice    - TODO: add argument description
  NeedRetry         - TODO: add argument description
  SenseDataArray    - TODO: add argument description
  NumberOfSenseKeys - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
DetectMediaParsingSenseKeys (
  SCSI_DISK_DEV           *ScsiDiskDevice,
  EFI_SCSI_SENSE_DATA     *SenseData,
  UINTN                   NumberOfSenseKeys,
  UINTN                   *Action
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice    - TODO: add argument description
  SenseData         - TODO: add argument description
  NumberOfSenseKeys - TODO: add argument description
  Action            - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskReadCapacity (
  SCSI_DISK_DEV           *ScsiDiskDevice,
  BOOLEAN                 *NeedRetry,
  EFI_SCSI_SENSE_DATA     **SenseDataArray,
  UINTN                   *NumberOfSenseKeys
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice    - TODO: add argument description
  NeedRetry         - TODO: add argument description
  SenseDataArray    - TODO: add argument description
  NumberOfSenseKeys - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
CheckHostAdapterStatus (
  UINT8   HostAdapterStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  HostAdapterStatus - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
CheckTargetStatus (
  UINT8   TargetStatus
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  TargetStatus  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskRequestSenseKeys (
  SCSI_DISK_DEV           *ScsiDiskDevice,
  BOOLEAN                 *NeedRetry,
  EFI_SCSI_SENSE_DATA     **SenseDataArray,
  UINTN                   *NumberOfSenseKeys,
  BOOLEAN                 AskResetIfError
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice    - TODO: add argument description
  NeedRetry         - TODO: add argument description
  SenseDataArray    - TODO: add argument description
  NumberOfSenseKeys - TODO: add argument description
  AskResetIfError   - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskInquiryDevice (
  SCSI_DISK_DEV   *ScsiDiskDevice,
  BOOLEAN         *NeedRetry
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description
  NeedRetry       - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
ParseInquiryData (
  SCSI_DISK_DEV   *ScsiDiskDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskReadSectors (
  SCSI_DISK_DEV     *ScsiDiskDevice,
  VOID              *Buffer,
  EFI_LBA           Lba,
  UINTN             NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description
  Buffer          - TODO: add argument description
  Lba             - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskWriteSectors (
  SCSI_DISK_DEV     *ScsiDiskDevice,
  VOID              *Buffer,
  EFI_LBA           Lba,
  UINTN             NumberOfBlocks
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description
  Buffer          - TODO: add argument description
  Lba             - TODO: add argument description
  NumberOfBlocks  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskRead10 (
  SCSI_DISK_DEV         *ScsiDiskDevice,
  BOOLEAN               *NeedRetry,
  EFI_SCSI_SENSE_DATA   **SenseDataArray,
  UINTN                 *NumberOfSenseKeys,
  UINT64                Timeout,
  UINT8                 *DataBuffer,
  UINT32                *DataLength,
  UINT32                StartLba,
  UINT32                SectorSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice    - TODO: add argument description
  NeedRetry         - TODO: add argument description
  SenseDataArray    - TODO: add argument description
  NumberOfSenseKeys - TODO: add argument description
  Timeout           - TODO: add argument description
  DataBuffer        - TODO: add argument description
  DataLength        - TODO: add argument description
  StartLba          - TODO: add argument description
  SectorSize        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

EFI_STATUS
ScsiDiskWrite10 (
  SCSI_DISK_DEV         *ScsiDiskDevice,
  BOOLEAN               *NeedRetry,
  EFI_SCSI_SENSE_DATA   **SenseDataArray,
  UINTN                 *NumberOfSenseKeys,
  UINT64                Timeout,
  UINT8                 *DataBuffer,
  UINT32                *DataLength,
  UINT32                StartLba,
  UINT32                SectorSize
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice    - TODO: add argument description
  NeedRetry         - TODO: add argument description
  SenseDataArray    - TODO: add argument description
  NumberOfSenseKeys - TODO: add argument description
  Timeout           - TODO: add argument description
  DataBuffer        - TODO: add argument description
  DataLength        - TODO: add argument description
  StartLba          - TODO: add argument description
  SectorSize        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

VOID
GetMediaInfo (
  SCSI_DISK_DEV                 *ScsiDiskDevice,
  EFI_SCSI_DISK_CAPACITY_DATA   *Capacity
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description
  Capacity        - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

BOOLEAN
ScsiDiskIsNoMedia (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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
ScsiDiskIsMediaError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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
ScsiDiskIsHardwareError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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
ScsiDiskIsMediaChange (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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
ScsiDiskIsResetBefore (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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
ScsiDiskIsDriveReady (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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
ScsiDiskHaveSenseKey (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
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

VOID
ReleaseScsiDiskDeviceResources (
  IN  SCSI_DISK_DEV   *ScsiDiskDevice
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  ScsiDiskDevice  - TODO: add argument description

Returns:

  TODO: add return values

--*/
;

#endif
