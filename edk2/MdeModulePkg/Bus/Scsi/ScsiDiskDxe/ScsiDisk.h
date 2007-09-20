/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
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

  Reset SCSI Disk  

Arguments:

  This                  - The pointer of EFI_BLOCK_IO_PROTOCOL
  ExtendedVerification  - The flag about if extend verificate

Returns:

  EFI_STATUS

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

  The function is to Read Block from SCSI Disk

Arguments:

  This        - The pointer of EFI_BLOCK_IO_PROTOCOL
  MediaId     - The Id of Media detected
  LBA         - The logic block address
  BufferSize  - The size of Buffer
  Buffer      - The buffer to fill the read out data

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in.
  EFI_SUCCESS           - Successfully to read out block.
  EFI_DEVICE_ERROR      - Fail to detect media.
  EFI_NO_MEDIA          - Media is not present.
  EFI_MEDIA_CHANGED     - Media has changed.
  EFI_BAD_BUFFER_SIZE   - The buffer size is not multiple of BlockSize.

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

  The function is to Write Block to SCSI Disk

Arguments:

  This        - The pointer of EFI_BLOCK_IO_PROTOCOL
  MediaId     - The Id of Media detected
  LBA         - The logic block address
  BufferSize  - The size of Buffer
  Buffer      - The buffer to fill the read out data

Returns:

  EFI_INVALID_PARAMETER - Invalid parameter passed in.
  EFI_SUCCESS           - Successfully to read out block.
  EFI_DEVICE_ERROR      - Fail to detect media.
  EFI_NO_MEDIA          - Media is not present.
  EFI_MEDIA_CHANGED     - Media has changed.
  EFI_BAD_BUFFER_SIZE   - The buffer size is not multiple of BlockSize.

--*/
;

EFI_STATUS
EFIAPI
ScsiDiskFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )
/*++

Routine Description:

  Flush Block to Disk

Arguments:

  This  - The pointer of EFI_BLOCK_IO_PROTOCOL

Returns:

  EFI_SUCCESS 

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

  Dectect Device and read out capacity ,if error occurs, parse the sense key.

Arguments:

  ScsiDiskDevice     - The pointer of SCSI_DISK_DEV
  MustReadCapacity   - The flag about reading device capacity
  MediaChange        - The pointer of flag indicates if media has changed 

Returns:

  EFI_DEVICE_ERROR   - Indicates that error occurs
  EFI_SUCCESS        - Successfully to detect media

--*/
;

EFI_STATUS
ScsiDiskTestUnitReady (
  SCSI_DISK_DEV       *ScsiDiskDevice,
  BOOLEAN             *NeedRetry,
  EFI_SCSI_SENSE_DATA **SenseDataArray,
  UINTN               *NumberOfSenseKeys
  )
/*++

Routine Description:

  When Test Unit Ready command succeeds, retrieve Sense Keys via Request Sense;
  When Test Unit Ready command encounters any error caused by host adapter or
  target, return error without retrieving Sense Keys.
  
Arguments:

  ScsiDiskDevice  - The pointer of SCSI_DISK_DEV
  NeedRetry       - The pointer of flag indicates try again
  SenseDataArray  - The pointer of an array of sense data
  NumberOfSenseKeys - The pointer of the number of sense data array
  
Returns:

  EFI_DEVICE_ERROR   - Indicates that error occurs
  EFI_SUCCESS        - Successfully to test unit

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

  Parsing Sense Keys which got from request sense command.
  
Arguments:

  ScsiDiskDevice    - The pointer of SCSI_DISK_DEV
  SenseData         - The pointer of EFI_SCSI_SENSE_DATA
  NumberOfSenseKeys - The number of sense key  
  Action            - The pointer of action which indicates what is need to do next

Returns:

  EFI_DEVICE_ERROR   - Indicates that error occurs
  EFI_SUCCESS        - Successfully to complete the parsing

--*/
;

EFI_STATUS
ScsiDiskReadCapacity (
  SCSI_DISK_DEV       *ScsiDiskDevice,
  BOOLEAN             *NeedRetry,
  EFI_SCSI_SENSE_DATA **SenseDataArray,
  UINTN               *NumberOfSenseKeys
  )
/*++

Routine Description:

  Send read capacity command to device and get the device parameter

Arguments:

  ScsiDiskDevice     -  The pointer of SCSI_DISK_DEV
  NeedRetry          -  The pointer of flag indicates if need a retry
  SenseDataArray     -  The pointer of an array of sense data
  NumberOfSenseKeys  -  The number of sense key

Returns:

  EFI_DEVICE_ERROR   - Indicates that error occurs
  EFI_SUCCESS        - Successfully to read capacity

--*/
;

EFI_STATUS
CheckHostAdapterStatus (
  UINT8   HostAdapterStatus
  )
/*++

Routine Description:

  Check the HostAdapter status
  
Arguments:

  HostAdapterStatus - Host Adapter status

Returns:

  EFI_SUCCESS       
  EFI_TIMEOUT       
  EFI_NOT_READY     
  EFI_DEVICE_ERROR  

--*/
;

EFI_STATUS
CheckTargetStatus (
  UINT8   TargetStatus
  )
/*++

Routine Description:

  Check the target status
  
Arguments:

  TargetStatus  - Target status

Returns:

  EFI_NOT_READY  
  EFI_DEVICE_ERROR 
  EFI_SUCCESS

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

  Retrieve all sense keys from the device.
  When encountering error during the process,
  if retrieve sense keys before error encounterred,
  return the sense keys with return status set to EFI_SUCCESS,
  and NeedRetry set to FALSE; otherwize, return the proper return
  status.

Arguments:

  ScsiDiskDevice     -  The pointer of SCSI_DISK_DEV
  NeedRetry          -  The pointer of flag indicates if need a retry
  SenseDataArray     -  The pointer of an array of sense data
  NumberOfSenseKeys  -  The number of sense key
  AskResetIfError    -  The flag indicates if need reset when error occurs
  
Returns:

  EFI_DEVICE_ERROR   - Indicates that error occurs
  EFI_SUCCESS        - Successfully to request sense key

--*/
;

EFI_STATUS
ScsiDiskInquiryDevice (
  SCSI_DISK_DEV   *ScsiDiskDevice,
  BOOLEAN         *NeedRetry
  )
/*++

Routine Description:

  Send out Inquiry command to Device

Arguments:

  ScsiDiskDevice  - The pointer of SCSI_DISK_DEV
  NeedRetry       - Indicates if needs try again when error happens

Returns:

  EFI_DEVICE_ERROR   - Indicates that error occurs
  EFI_SUCCESS        - Successfully to detect media

--*/
;

VOID
ParseInquiryData (
  SCSI_DISK_DEV   *ScsiDiskDevice
  )
/*++

Routine Description:

  Parse Inquiry data

Arguments:

  ScsiDiskDevice  - The pointer of SCSI_DISK_DEV

Returns:

  NONE

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

  Read sector from SCSI Disk

Arguments:

  ScsiDiskDevice  - The poiniter of SCSI_DISK_DEV
  Buffer          - The buffer to fill in the read out data
  Lba             - Logic block address
  NumberOfBlocks  - The number of blocks to read

Returns:

  EFI_DEVICE_ERROR
  EFI_SUCCESS

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

  Write SCSI Disk sectors

Arguments:

  ScsiDiskDevice  - The pointer of SCSI_DISK_DEV
  Buffer          - The data buffer to write sector
  Lba             - Logic block address
  NumberOfBlocks  - The number of blocks to write

Returns:

  EFI_DEVICE_ERROR 
  EFI_SUCCESS

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

  Sumbmit Read command 

Arguments:

  ScsiDiskDevice    - The pointer of ScsiDiskDevice
  NeedRetry         - The pointer of flag indicates if needs retry if error happens
  SenseDataArray    - The pointer of an array of sense data
  NumberOfSenseKeys - The number of sense key
  Timeout           - The time to complete the command
  DataBuffer        - The buffer to fill with the read out data
  DataLength        - The length of buffer
  StartLba          - The start logic block address
  SectorSize        - The size of sector

Returns:

  EFI_STATUS

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

  Submit Write Command

Arguments:

  ScsiDiskDevice    - The pointer of ScsiDiskDevice
  NeedRetry         - The pointer of flag indicates if needs retry if error happens
  SenseDataArray    - The pointer of an array of sense data
  NumberOfSenseKeys - The number of sense key
  Timeout           - The time to complete the command
  DataBuffer        - The buffer to fill with the read out data
  DataLength        - The length of buffer
  StartLba          - The start logic block address
  SectorSize        - The size of sector

Returns:

  EFI_STATUS

--*/
;

VOID
GetMediaInfo (
  SCSI_DISK_DEV                 *ScsiDiskDevice,
  EFI_SCSI_DISK_CAPACITY_DATA   *Capacity
  )
/*++

Routine Description:

  Get information from media read capacity command

Arguments:

  ScsiDiskDevice  - The pointer of SCSI_DISK_DEV
  Capacity        - The pointer of EFI_SCSI_DISK_CAPACITY_DATA

Returns:

  NONE

--*/
;

BOOLEAN
ScsiDiskIsNoMedia (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  Check sense key to find if media presents

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key

Returns:

  BOOLEAN

--*/
;

BOOLEAN
ScsiDiskIsMediaError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  Parse sense key

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key

Returns:

  BOOLEAN

--*/
;

BOOLEAN
ScsiDiskIsHardwareError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  Check sense key to find if hardware error happens

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key

Returns:

  BOOLEAN

--*/
;

BOOLEAN
ScsiDiskIsMediaChange (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

Routine Description:

 Check sense key to find if media has changed

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key

Returns:

  BOOLEAN

--*/
;

BOOLEAN
ScsiDiskIsResetBefore (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  Check sense key to find if reset happens

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key

Returns:

  BOOLEAN

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

  Check sense key to find if the drive is ready

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key
  RetryLater  - The flag means if need a retry 

Returns:

  BOOLEAN

--*/
;

BOOLEAN
ScsiDiskHaveSenseKey (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  )
/*++

Routine Description:

  Check sense key to find if it has sense key

Arguments:

  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  SenseCounts - The number of sense key

Returns:

  BOOLEAN

--*/
;

VOID
ReleaseScsiDiskDeviceResources (
  IN  SCSI_DISK_DEV   *ScsiDiskDevice
  )
/*++

Routine Description:

  Release resource about disk device

Arguments:

  ScsiDiskDevice  - The pointer of SCSI_DISK_DEV

Returns:

  NONE

--*/
;

#endif
