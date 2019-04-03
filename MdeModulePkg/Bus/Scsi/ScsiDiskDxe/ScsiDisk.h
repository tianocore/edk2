/** @file
  Header file for SCSI Disk Driver.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SCSI_DISK_H_
#define _SCSI_DISK_H_


#include <Uefi.h>


#include <Protocol/ScsiIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/EraseBlock.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/ScsiPassThru.h>
#include <Protocol/DiskInfo.h>


#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiScsiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <IndustryStandard/Scsi.h>
#include <IndustryStandard/Atapi.h>

#define IS_DEVICE_FIXED(a)        (a)->FixedDevice ? 1 : 0

typedef struct {
  UINT32                    MaxLbaCnt;
  UINT32                    MaxBlkDespCnt;
  UINT32                    GranularityAlignment;
} SCSI_UNMAP_PARAM_INFO;

#define SCSI_DISK_DEV_SIGNATURE SIGNATURE_32 ('s', 'c', 'd', 'k')

typedef struct {
  UINT32                    Signature;

  EFI_HANDLE                Handle;

  EFI_BLOCK_IO_PROTOCOL     BlkIo;
  EFI_BLOCK_IO2_PROTOCOL    BlkIo2;
  EFI_BLOCK_IO_MEDIA        BlkIoMedia;
  EFI_ERASE_BLOCK_PROTOCOL  EraseBlock;
  EFI_SCSI_IO_PROTOCOL      *ScsiIo;
  UINT8                     DeviceType;
  BOOLEAN                   FixedDevice;
  UINT16                    Reserved;

  EFI_SCSI_SENSE_DATA       *SenseData;
  UINTN                     SenseDataNumber;
  EFI_SCSI_INQUIRY_DATA     InquiryData;

  EFI_UNICODE_STRING_TABLE  *ControllerNameTable;

  EFI_DISK_INFO_PROTOCOL    DiskInfo;

  //
  // The following fields are only valid for ATAPI/SATA device
  //
  UINT32                    Channel;
  UINT32                    Device;
  ATAPI_IDENTIFY_DATA       IdentifyData;

  //
  // Scsi UNMAP command parameters information
  //
  SCSI_UNMAP_PARAM_INFO     UnmapInfo;
  BOOLEAN                   BlockLimitsVpdSupported;

  //
  // The flag indicates if 16-byte command can be used
  //
  BOOLEAN                   Cdb16Byte;

  //
  // The queue for asynchronous task requests
  //
  LIST_ENTRY                AsyncTaskQueue;
} SCSI_DISK_DEV;

#define SCSI_DISK_DEV_FROM_BLKIO(a)  CR (a, SCSI_DISK_DEV, BlkIo, SCSI_DISK_DEV_SIGNATURE)
#define SCSI_DISK_DEV_FROM_BLKIO2(a)  CR (a, SCSI_DISK_DEV, BlkIo2, SCSI_DISK_DEV_SIGNATURE)
#define SCSI_DISK_DEV_FROM_ERASEBLK(a)  CR (a, SCSI_DISK_DEV, EraseBlock, SCSI_DISK_DEV_SIGNATURE)

#define SCSI_DISK_DEV_FROM_DISKINFO(a) CR (a, SCSI_DISK_DEV, DiskInfo, SCSI_DISK_DEV_SIGNATURE)

//
// Asynchronous I/O request
//
//
// Private data structure for a BlockIo2 request
//
typedef struct {
  EFI_BLOCK_IO2_TOKEN                  *Token;
  //
  // The flag indicates if the last Scsi Read/Write sub-task for a BlockIo2
  // request is sent to device
  //
  BOOLEAN                              LastScsiRW;

  //
  // The queue for Scsi Read/Write sub-tasks of a BlockIo2 request
  //
  LIST_ENTRY                           ScsiRWQueue;

  LIST_ENTRY                           Link;
} SCSI_BLKIO2_REQUEST;

//
// Private data structure for a SCSI Read/Write request
//
typedef struct {
  SCSI_DISK_DEV                        *ScsiDiskDevice;
  UINT64                               Timeout;
  EFI_SCSI_SENSE_DATA                  *SenseData;
  UINT8                                SenseDataLength;
  UINT8                                HostAdapterStatus;
  UINT8                                TargetStatus;
  UINT8                                *InBuffer;
  UINT8                                *OutBuffer;
  UINT32                               DataLength;
  UINT64                               StartLba;
  UINT32                               SectorCount;
  UINT8                                TimesRetry;

  //
  // The BlockIo2 request this SCSI command belongs to
  //
  SCSI_BLKIO2_REQUEST                  *BlkIo2Req;

  LIST_ENTRY                           Link;
} SCSI_ASYNC_RW_REQUEST;

//
// Private data structure for an EraseBlock request
//
typedef struct {
  EFI_ERASE_BLOCK_TOKEN                *Token;

  EFI_SCSI_IO_SCSI_REQUEST_PACKET      CommandPacket;

  LIST_ENTRY                           Link;
} SCSI_ERASEBLK_REQUEST;

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gScsiDiskDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gScsiDiskComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gScsiDiskComponentName2;
//
// action code used in detect media process
//
#define ACTION_NO_ACTION               0x00
#define ACTION_READ_CAPACITY           0x01
#define ACTION_RETRY_COMMAND_LATER     0x02
#define ACTION_RETRY_WITH_BACKOFF_ALGO 0x03

#define SCSI_COMMAND_VERSION_1      0x01
#define SCSI_COMMAND_VERSION_2      0x02
#define SCSI_COMMAND_VERSION_3      0x03

//
// SCSI Disk Timeout Experience Value
//
// As ScsiDisk and ScsiBus driver are used to manage SCSI or ATAPI devices, the timout
// value is updated to 30s to follow ATA/ATAPI spec in which the device may take up to 30s
// to respond command.
//
#define SCSI_DISK_TIMEOUT           EFI_TIMER_PERIOD_SECONDS (30)

/**
  Test to see if this driver supports ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions.
  If any other agent wishes to call Supported() it must also follow these
  calling restrictions.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
ScsiDiskDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath   OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions. If
  any other agent wishes to call Start() it must also follow these calling
  restrictions.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
ScsiDiskDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath   OPTIONAL
  );

/**
  Stop this driver on ControllerHandle.

  This service is called by the EFI boot service DisconnectController().
  In order to make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController() must follow these
  calling restrictions. If any other agent wishes to call Stop() it must
  also follow these calling restrictions.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ScsiDiskDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer   OPTIONAL
  );

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ScsiDiskComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );


/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
ScsiDiskComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Reset SCSI Disk.


  @param  This                 The pointer of EFI_BLOCK_IO_PROTOCOL
  @param  ExtendedVerification The flag about if extend verificate

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.
  @return EFI_STATUS is retured from EFI_SCSI_IO_PROTOCOL.ResetDevice().

**/
EFI_STATUS
EFIAPI
ScsiDiskReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  );


/**
  The function is to Read Block from SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL.
  @param  MediaId    The Id of Media detected
  @param  Lba        The logic block address
  @param  BufferSize The size of Buffer
  @param  Buffer     The buffer to fill the read out data

  @retval EFI_SUCCESS           Successfully to read out block.
  @retval EFI_DEVICE_ERROR      Fail to detect media.
  @retval EFI_NO_MEDIA          Media is not present.
  @retval EFI_MEDIA_CHANGED     Media has changed.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER Invalid parameter passed in.

**/
EFI_STATUS
EFIAPI
ScsiDiskReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  );


/**
  The function is to Write Block to SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL
  @param  MediaId    The Id of Media detected
  @param  Lba        The logic block address
  @param  BufferSize The size of Buffer
  @param  Buffer     The buffer to fill the read out data

  @retval EFI_SUCCESS           Successfully to read out block.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      Fail to detect media.
  @retval EFI_NO_MEDIA          Media is not present.
  @retval EFI_MEDIA_CHNAGED     Media has changed.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER Invalid parameter passed in.

**/
EFI_STATUS
EFIAPI
ScsiDiskWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  );


/**
  Flush Block to Disk.

  EFI_SUCCESS is returned directly.

  @param  This              The pointer of EFI_BLOCK_IO_PROTOCOL

  @retval EFI_SUCCESS       All outstanding data was written to the device

**/
EFI_STATUS
EFIAPI
ScsiDiskFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  );


/**
  Reset SCSI Disk.

  @param  This                 The pointer of EFI_BLOCK_IO2_PROTOCOL.
  @param  ExtendedVerification The flag about if extend verificate.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.
  @return EFI_STATUS is returned from EFI_SCSI_IO_PROTOCOL.ResetDevice().

**/
EFI_STATUS
EFIAPI
ScsiDiskResetEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  BOOLEAN                 ExtendedVerification
  );

/**
  The function is to Read Block from SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL.
  @param  MediaId    The Id of Media detected.
  @param  Lba        The logic block address.
  @param  Token      A pointer to the token associated with the transaction.
  @param  BufferSize The size of Buffer.
  @param  Buffer     The buffer to fill the read out data.

  @retval EFI_SUCCESS           The read request was queued if Token-> Event is
                                not NULL. The data was read correctly from the
                                device if theToken-> Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while attempting
                                to perform the read operation.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of
                                the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not
                                valid, or the buffer is not on proper
                                alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.

**/
EFI_STATUS
EFIAPI
ScsiDiskReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL   *This,
  IN     UINT32                   MediaId,
  IN     EFI_LBA                  Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN      *Token,
  IN     UINTN                    BufferSize,
  OUT    VOID                     *Buffer
  );

/**
  The function is to Write Block to SCSI Disk.

  @param  This       The pointer of EFI_BLOCK_IO_PROTOCOL.
  @param  MediaId    The Id of Media detected.
  @param  Lba        The logic block address.
  @param  Token      A pointer to the token associated with the transaction.
  @param  BufferSize The size of Buffer.
  @param  Buffer     The buffer to fill the read out data.

  @retval EFI_SUCCESS           The data were written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR      The device reported an error while attempting
                                to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of
                                the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not
                                valid, or the buffer is not on proper
                                alignment.

**/
EFI_STATUS
EFIAPI
ScsiDiskWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL *This,
  IN     UINT32                 MediaId,
  IN     EFI_LBA                Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN    *Token,
  IN     UINTN                  BufferSize,
  IN     VOID                   *Buffer
  );

/**
  Flush the Block Device.

  @param  This       Indicates a pointer to the calling context.
  @param  Token      A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS         All outstanding data was written to the device.
  @retval EFI_DEVICE_ERROR    The device reported an error while attempting to
                              write data.
  @retval EFI_WRITE_PROTECTED The device cannot be written to.
  @retval EFI_NO_MEDIA        There is no media in the device.
  @retval EFI_MEDIA_CHANGED   The MediaId is not for the current media.

**/
EFI_STATUS
EFIAPI
ScsiDiskFlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token
  );

/**
  Erase a specified number of device blocks.

  @param[in]       This           Indicates a pointer to the calling context.
  @param[in]       MediaId        The media ID that the erase request is for.
  @param[in]       Lba            The starting logical block address to be
                                  erased. The caller is responsible for erasing
                                  only legitimate locations.
  @param[in, out]  Token          A pointer to the token associated with the
                                  transaction.
  @param[in]       Size           The size in bytes to be erased. This must be
                                  a multiple of the physical block size of the
                                  device.

  @retval EFI_SUCCESS             The erase request was queued if Event is not
                                  NULL. The data was erased correctly to the
                                  device if the Event is NULL.to the device.
  @retval EFI_WRITE_PROTECTED     The device cannot be erased due to write
                                  protection.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the erase operation.
  @retval EFI_INVALID_PARAMETER   The erase request contains LBAs that are not
                                  valid.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.

**/
EFI_STATUS
EFIAPI
ScsiDiskEraseBlocks (
  IN     EFI_ERASE_BLOCK_PROTOCOL      *This,
  IN     UINT32                        MediaId,
  IN     EFI_LBA                       Lba,
  IN OUT EFI_ERASE_BLOCK_TOKEN         *Token,
  IN     UINTN                         Size
  );


/**
  Provides inquiry information for the controller type.

  This function is used by the IDE bus driver to get inquiry data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData       Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize   Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *InquiryDataSize
  );


/**
  Provides identify information for the controller type.

  This function is used by the IDE bus driver to get identify data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]     This               Pointer to the EFI_DISK_INFO_PROTOCOL
                                    instance.
  @param[in, out] IdentifyData      Pointer to a buffer for the identify data.
  @param[in, out] IdentifyDataSize  Pointer to the value for the identify data
                                    size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading IdentifyData from device
  @retval EFI_BUFFER_TOO_SMALL   IdentifyDataSize not big enough

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  );


/**
  Provides sense data information for the controller type.

  This function is used by the IDE bus driver to get sense data.
  Data format of Sense data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] SenseData         Pointer to the SenseData.
  @param[in, out] SenseDataSize     Size of SenseData in bytes.
  @param[out]     SenseDataNumber   Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT    UINT8                    *SenseDataNumber
  );

/**
  This function is used by the IDE bus driver to get controller information.

  @param[in]  This         Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[out] IdeChannel   Pointer to the Ide Channel number.  Primary or secondary.
  @param[out] IdeDevice    Pointer to the Ide Device number.  Master or slave.

  @retval EFI_SUCCESS       IdeChannel and IdeDevice are valid.
  @retval EFI_UNSUPPORTED   This is not an IDE device.

**/
EFI_STATUS
EFIAPI
ScsiDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL   *This,
  OUT UINT32                   *IdeChannel,
  OUT UINT32                   *IdeDevice
  );


/**
  Detect Device and read out capacity ,if error occurs, parse the sense key.

  @param  ScsiDiskDevice    The pointer of SCSI_DISK_DEV
  @param  MustReadCapacity  The flag about reading device capacity
  @param  MediaChange       The pointer of flag indicates if media has changed

  @retval EFI_DEVICE_ERROR  Indicates that error occurs
  @retval EFI_SUCCESS       Successfully to detect media

**/
EFI_STATUS
ScsiDiskDetectMedia (
  IN   SCSI_DISK_DEV   *ScsiDiskDevice,
  IN   BOOLEAN         MustReadCapacity,
  OUT  BOOLEAN         *MediaChange
  );

/**
  To test device.

  When Test Unit Ready command succeeds, retrieve Sense Keys via Request Sense;
  When Test Unit Ready command encounters any error caused by host adapter or
  target, return error without retrieving Sense Keys.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates try again
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The pointer of the number of sense data array

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to test unit

**/
EFI_STATUS
ScsiDiskTestUnitReady (
  IN  SCSI_DISK_DEV         *ScsiDiskDevice,
  OUT BOOLEAN               *NeedRetry,
  OUT EFI_SCSI_SENSE_DATA   **SenseDataArray,
  OUT UINTN                 *NumberOfSenseKeys
  );


/**
  Parsing Sense Keys which got from request sense command.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  SenseData          The pointer of EFI_SCSI_SENSE_DATA
  @param  NumberOfSenseKeys  The number of sense key
  @param  Action             The pointer of action which indicates what is need to do next

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to complete the parsing

**/
EFI_STATUS
DetectMediaParsingSenseKeys (
  OUT  SCSI_DISK_DEV           *ScsiDiskDevice,
  IN   EFI_SCSI_SENSE_DATA     *SenseData,
  IN   UINTN                   NumberOfSenseKeys,
  OUT  UINTN                   *Action
  );


/**
  Send read capacity command to device and get the device parameter.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates if need a retry
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The number of sense key

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to read capacity

**/
EFI_STATUS
ScsiDiskReadCapacity (
  IN  OUT  SCSI_DISK_DEV           *ScsiDiskDevice,
      OUT  BOOLEAN                 *NeedRetry,
      OUT  EFI_SCSI_SENSE_DATA     **SenseDataArray,
      OUT  UINTN                   *NumberOfSenseKeys
  );

/**
  Check the HostAdapter status and re-interpret it in EFI_STATUS.

  @param  HostAdapterStatus  Host Adapter status

  @retval  EFI_SUCCESS       Host adapter is OK.
  @retval  EFI_TIMEOUT       Timeout.
  @retval  EFI_NOT_READY     Adapter NOT ready.
  @retval  EFI_DEVICE_ERROR  Adapter device error.

**/
EFI_STATUS
CheckHostAdapterStatus (
  IN UINT8   HostAdapterStatus
  );


/**
  Check the target status and re-interpret it in EFI_STATUS.

  @param  TargetStatus  Target status

  @retval EFI_NOT_READY       Device is NOT ready.
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
CheckTargetStatus (
  IN  UINT8   TargetStatus
  );

/**
  Retrieve all sense keys from the device.

  When encountering error during the process, if retrieve sense keys before
  error encountered, it returns the sense keys with return status set to EFI_SUCCESS,
  and NeedRetry set to FALSE; otherwize, return the proper return status.

  @param  ScsiDiskDevice     The pointer of SCSI_DISK_DEV
  @param  NeedRetry          The pointer of flag indicates if need a retry
  @param  SenseDataArray     The pointer of an array of sense data
  @param  NumberOfSenseKeys  The number of sense key
  @param  AskResetIfError    The flag indicates if need reset when error occurs

  @retval EFI_DEVICE_ERROR   Indicates that error occurs
  @retval EFI_SUCCESS        Successfully to request sense key

**/
EFI_STATUS
ScsiDiskRequestSenseKeys (
  IN  OUT  SCSI_DISK_DEV           *ScsiDiskDevice,
      OUT  BOOLEAN                 *NeedRetry,
      OUT  EFI_SCSI_SENSE_DATA     **SenseDataArray,
      OUT  UINTN                   *NumberOfSenseKeys,
  IN       BOOLEAN                 AskResetIfError
  );

/**
  Send out Inquiry command to Device.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  NeedRetry       Indicates if needs try again when error happens

  @retval  EFI_DEVICE_ERROR  Indicates that error occurs
  @retval  EFI_SUCCESS       Successfully to detect media

**/
EFI_STATUS
ScsiDiskInquiryDevice (
  IN OUT  SCSI_DISK_DEV   *ScsiDiskDevice,
     OUT  BOOLEAN         *NeedRetry
  );

/**
  Parse Inquiry data.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV

**/
VOID
ParseInquiryData (
  IN OUT SCSI_DISK_DEV   *ScsiDiskDevice
  );

/**
  Read sector from SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Buffer          The buffer to fill in the read out data
  @param  Lba             Logic block address
  @param  NumberOfBlocks  The number of blocks to read

  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskReadSectors (
  IN   SCSI_DISK_DEV     *ScsiDiskDevice,
  OUT  VOID              *Buffer,
  IN   EFI_LBA           Lba,
  IN   UINTN             NumberOfBlocks
  );

/**
  Write sector to SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Buffer          The buffer of data to be written into SCSI Disk
  @param  Lba             Logic block address
  @param  NumberOfBlocks  The number of blocks to read

  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskWriteSectors (
  IN  SCSI_DISK_DEV     *ScsiDiskDevice,
  IN  VOID              *Buffer,
  IN  EFI_LBA           Lba,
  IN  UINTN             NumberOfBlocks
  );

/**
  Asynchronously read sector from SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV.
  @param  Buffer          The buffer to fill in the read out data.
  @param  Lba             Logic block address.
  @param  NumberOfBlocks  The number of blocks to read.
  @param  Token           A pointer to the token associated with the
                          non-blocking read request.

  @retval EFI_INVALID_PARAMETER  Token is NULL or Token->Event is NULL.
  @retval EFI_DEVICE_ERROR       Indicates a device error.
  @retval EFI_SUCCESS            Operation is successful.

**/
EFI_STATUS
ScsiDiskAsyncReadSectors (
  IN   SCSI_DISK_DEV         *ScsiDiskDevice,
  OUT  VOID                  *Buffer,
  IN   EFI_LBA               Lba,
  IN   UINTN                 NumberOfBlocks,
  IN   EFI_BLOCK_IO2_TOKEN   *Token
  );

/**
  Asynchronously write sector to SCSI Disk.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV.
  @param  Buffer          The buffer of data to be written into SCSI Disk.
  @param  Lba             Logic block address.
  @param  NumberOfBlocks  The number of blocks to read.
  @param  Token           A pointer to the token associated with the
                          non-blocking read request.

  @retval EFI_INVALID_PARAMETER  Token is NULL or Token->Event is NULL
  @retval EFI_DEVICE_ERROR  Indicates a device error.
  @retval EFI_SUCCESS       Operation is successful.

**/
EFI_STATUS
ScsiDiskAsyncWriteSectors (
  IN  SCSI_DISK_DEV          *ScsiDiskDevice,
  IN  VOID                   *Buffer,
  IN  EFI_LBA                Lba,
  IN  UINTN                  NumberOfBlocks,
  IN  EFI_BLOCK_IO2_TOKEN    *Token
  );

/**
  Submit Read(10) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to read

  @return  EFI_STATUS is returned by calling ScsiRead10Command().
**/
EFI_STATUS
ScsiDiskRead10 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
     OUT BOOLEAN               *NeedRetry,
  IN     UINT64                Timeout,
     OUT UINT8                 *DataBuffer,
  IN OUT UINT32                *DataLength,
  IN     UINT32                StartLba,
  IN     UINT32                SectorCount
  );

/**
  Submit Write(10) Command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to write

  @return  EFI_STATUS is returned by calling ScsiWrite10Command().

**/
EFI_STATUS
ScsiDiskWrite10 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
     OUT BOOLEAN               *NeedRetry,
  IN     UINT64                Timeout,
  IN     UINT8                 *DataBuffer,
  IN OUT UINT32                *DataLength,
  IN     UINT32                StartLba,
  IN     UINT32                SectorCount
  );

/**
  Submit Read(16) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to read

  @return  EFI_STATUS is returned by calling ScsiRead16Command().
**/
EFI_STATUS
ScsiDiskRead16 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
     OUT BOOLEAN               *NeedRetry,
  IN     UINT64                Timeout,
     OUT UINT8                 *DataBuffer,
  IN OUT UINT32                *DataLength,
  IN     UINT64                StartLba,
  IN     UINT32                SectorCount
  );

/**
  Submit Write(16) Command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice
  @param  NeedRetry          The pointer of flag indicates if needs retry if error happens
  @param  Timeout            The time to complete the command
  @param  DataBuffer         The buffer to fill with the read out data
  @param  DataLength         The length of buffer
  @param  StartLba           The start logic block address
  @param  SectorCount        The number of blocks to write

  @return  EFI_STATUS is returned by calling ScsiWrite16Command().

**/
EFI_STATUS
ScsiDiskWrite16 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
     OUT BOOLEAN               *NeedRetry,
  IN     UINT64                Timeout,
  IN     UINT8                 *DataBuffer,
  IN OUT UINT32                *DataLength,
  IN     UINT64                StartLba,
  IN     UINT32                SectorCount
  );

/**
  Submit Async Read(10) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer to fill with the read out data.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to read.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiRead10CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncRead10 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
  IN     UINT64                Timeout,
  IN     UINT8                 TimesRetry,
     OUT UINT8                 *DataBuffer,
  IN     UINT32                DataLength,
  IN     UINT32                StartLba,
  IN     UINT32                SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST   *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN   *Token
  );

/**
  Submit Async Write(10) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer contains the data to write.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to write.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiWrite10CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncWrite10 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
  IN     UINT64                Timeout,
  IN     UINT8                 TimesRetry,
  IN     UINT8                 *DataBuffer,
  IN     UINT32                DataLength,
  IN     UINT32                StartLba,
  IN     UINT32                SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST   *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN   *Token
  );

/**
  Submit Async Read(16) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer to fill with the read out data.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to read.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiRead16CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncRead16 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
  IN     UINT64                Timeout,
  IN     UINT8                 TimesRetry,
     OUT UINT8                 *DataBuffer,
  IN     UINT32                DataLength,
  IN     UINT64                StartLba,
  IN     UINT32                SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST   *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN   *Token
  );

/**
  Submit Async Write(16) command.

  @param  ScsiDiskDevice     The pointer of ScsiDiskDevice.
  @param  Timeout            The time to complete the command.
  @param  TimesRetry         The number of times the command has been retried.
  @param  DataBuffer         The buffer contains the data to write.
  @param  DataLength         The length of buffer.
  @param  StartLba           The start logic block address.
  @param  SectorCount        The number of blocks to write.
  @param  BlkIo2Req          The upstream BlockIo2 request.
  @param  Token              The pointer to the token associated with the
                             non-blocking read request.

  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a
                                lack of resources.
  @return others                Status returned by calling
                                ScsiWrite16CommandEx().

**/
EFI_STATUS
ScsiDiskAsyncWrite16 (
  IN     SCSI_DISK_DEV         *ScsiDiskDevice,
  IN     UINT64                Timeout,
  IN     UINT8                 TimesRetry,
  IN     UINT8                 *DataBuffer,
  IN     UINT32                DataLength,
  IN     UINT64                StartLba,
  IN     UINT32                SectorCount,
  IN OUT SCSI_BLKIO2_REQUEST   *BlkIo2Req,
  IN     EFI_BLOCK_IO2_TOKEN   *Token
  );

/**
  Get information from media read capacity command.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV
  @param  Capacity10      The pointer of EFI_SCSI_DISK_CAPACITY_DATA
  @param  Capacity16      The pointer of EFI_SCSI_DISK_CAPACITY_DATA16
**/
VOID
GetMediaInfo (
  IN OUT SCSI_DISK_DEV                  *ScsiDiskDevice,
  IN     EFI_SCSI_DISK_CAPACITY_DATA    *Capacity10,
  IN     EFI_SCSI_DISK_CAPACITY_DATA16  *Capacity16
  );

/**
  Check sense key to find if media presents.

  @param  SenseData   The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts The number of sense key

  @retval TRUE    NOT any media
  @retval FALSE   Media presents
**/
BOOLEAN
ScsiDiskIsNoMedia (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  );

/**
  Parse sense key.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key

  @retval TRUE   Error
  @retval FALSE  NOT error

**/
BOOLEAN
ScsiDiskIsMediaError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  );

/**
  Check sense key to find if hardware error happens.

  @param  SenseData     The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts   The number of sense key

  @retval TRUE  Hardware error exits.
  @retval FALSE NO error.

**/
BOOLEAN
ScsiDiskIsHardwareError (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  );

/**
  Check sense key to find if media has changed.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key

  @retval TRUE   Media is changed.
  @retval FALSE  Medit is NOT changed.
**/
BOOLEAN
ScsiDiskIsMediaChange (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  );

/**
  Check sense key to find if reset happens.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key

  @retval TRUE  It is reset before.
  @retval FALSE It is NOT reset before.

**/
BOOLEAN
ScsiDiskIsResetBefore (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  );

/**
  Check sense key to find if the drive is ready.

  @param  SenseData    The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts  The number of sense key
  @param  RetryLater   The flag means if need a retry

  @retval TRUE  Drive is ready.
  @retval FALSE Drive is NOT ready.

**/
BOOLEAN
ScsiDiskIsDriveReady (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts,
  OUT BOOLEAN               *RetryLater
  );

/**
  Check sense key to find if it has sense key.

  @param  SenseData   - The pointer of EFI_SCSI_SENSE_DATA
  @param  SenseCounts - The number of sense key

  @retval TRUE  It has sense key.
  @retval FALSE It has NOT any sense key.

**/
BOOLEAN
ScsiDiskHaveSenseKey (
  IN  EFI_SCSI_SENSE_DATA   *SenseData,
  IN  UINTN                 SenseCounts
  );

/**
  Release resource about disk device.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV

**/
VOID
ReleaseScsiDiskDeviceResources (
  IN  SCSI_DISK_DEV   *ScsiDiskDevice
  );

/**
  Determine if Block Io should be produced.


  @param  ChildHandle  Child Handle to retrieve Parent information.

  @retval  TRUE    Should produce Block Io.
  @retval  FALSE   Should not produce Block Io.

**/
BOOLEAN
DetermineInstallBlockIo (
  IN  EFI_HANDLE      ChildHandle
  );

/**
  Initialize the installation of DiskInfo protocol.

  This function prepares for the installation of DiskInfo protocol on the child handle.
  By default, it installs DiskInfo protocol with SCSI interface GUID. If it further
  detects that the physical device is an ATAPI/AHCI device, it then updates interface GUID
  to be IDE/AHCI interface GUID.

  @param  ScsiDiskDevice  The pointer of SCSI_DISK_DEV.
  @param  ChildHandle     Child handle to install DiskInfo protocol.

**/
VOID
InitializeInstallDiskInfo (
  IN  SCSI_DISK_DEV   *ScsiDiskDevice,
  IN  EFI_HANDLE      ChildHandle
  );

/**
  Search protocol database and check to see if the protocol
  specified by ProtocolGuid is present on a ControllerHandle and opened by
  ChildHandle with an attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  If the ControllerHandle is found, then the protocol specified by ProtocolGuid
  will be opened on it.


  @param  ProtocolGuid   ProtocolGuid pointer.
  @param  ChildHandle    Child Handle to retrieve Parent information.

**/
VOID *
EFIAPI
GetParentProtocol (
  IN  EFI_GUID                          *ProtocolGuid,
  IN  EFI_HANDLE                        ChildHandle
  );

/**
  Determine if EFI Erase Block Protocol should be produced.

  @param   ScsiDiskDevice    The pointer of SCSI_DISK_DEV.
  @param   ChildHandle       Handle of device.

  @retval  TRUE    Should produce EFI Erase Block Protocol.
  @retval  FALSE   Should not produce EFI Erase Block Protocol.

**/
BOOLEAN
DetermineInstallEraseBlock (
  IN  SCSI_DISK_DEV          *ScsiDiskDevice,
  IN  EFI_HANDLE             ChildHandle
  );

#endif
