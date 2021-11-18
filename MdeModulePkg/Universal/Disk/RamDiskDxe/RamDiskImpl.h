/** @file
  The header file of RamDiskDxe driver.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RAM_DISK_IMPL_H_
#define _RAM_DISK_IMPL_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/FileExplorerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>
#include <Protocol/RamDisk.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/RamDiskHii.h>
#include <Guid/FileInfo.h>
#include <IndustryStandard/Acpi61.h>

#include "RamDiskNVData.h"

///
/// RAM disk general definitions and declarations
///

//
// Default block size for RAM disk
//
#define RAM_DISK_DEFAULT_BLOCK_SIZE  512

//
// RamDiskDxe driver maintains a list of registered RAM disks.
//
extern  LIST_ENTRY  RegisteredRamDisks;

//
// Pointers to the EFI_ACPI_TABLE_PROTOCOL and EFI_ACPI_SDT_PROTOCOL.
//
extern  EFI_ACPI_TABLE_PROTOCOL  *mAcpiTableProtocol;
extern  EFI_ACPI_SDT_PROTOCOL    *mAcpiSdtProtocol;

//
// RAM Disk create method.
//
typedef enum _RAM_DISK_CREATE_METHOD {
  RamDiskCreateOthers = 0,
  RamDiskCreateHii
} RAM_DISK_CREATE_METHOD;

//
// RamDiskDxe driver maintains a list of registered RAM disks.
// The struct contains the list entry and the information of each RAM
// disk
//
typedef struct {
  UINTN                       Signature;

  EFI_HANDLE                  Handle;

  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO2_PROTOCOL      BlockIo2;
  EFI_BLOCK_IO_MEDIA          Media;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  UINT64                      StartingAddr;
  UINT64                      Size;
  EFI_GUID                    TypeGuid;
  UINT16                      InstanceNumber;
  RAM_DISK_CREATE_METHOD      CreateMethod;
  BOOLEAN                     InNfit;
  EFI_QUESTION_ID             CheckBoxId;
  BOOLEAN                     CheckBoxChecked;

  LIST_ENTRY                  ThisInstance;
} RAM_DISK_PRIVATE_DATA;

#define RAM_DISK_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('R', 'D', 'S', 'K')
#define RAM_DISK_PRIVATE_FROM_BLKIO(a)   CR (a, RAM_DISK_PRIVATE_DATA, BlockIo, RAM_DISK_PRIVATE_DATA_SIGNATURE)
#define RAM_DISK_PRIVATE_FROM_BLKIO2(a)  CR (a, RAM_DISK_PRIVATE_DATA, BlockIo2, RAM_DISK_PRIVATE_DATA_SIGNATURE)
#define RAM_DISK_PRIVATE_FROM_THIS(a)    CR (a, RAM_DISK_PRIVATE_DATA, ThisInstance, RAM_DISK_PRIVATE_DATA_SIGNATURE)

///
/// RAM disk HII-related definitions and declarations
///

//
// Tool generated IFR binary data and String package data
//
extern  UINT8  RamDiskHiiBin[];
extern  UINT8  RamDiskDxeStrings[];

typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  UINTN                             Signature;

  RAM_DISK_CONFIGURATION            ConfigStore;

  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HANDLE                        DriverHandle;
  EFI_HII_HANDLE                    HiiHandle;
} RAM_DISK_CONFIG_PRIVATE_DATA;

extern RAM_DISK_CONFIG_PRIVATE_DATA  mRamDiskConfigPrivateDataTemplate;

#define RAM_DISK_CONFIG_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('R', 'C', 'F', 'G')
#define RAM_DISK_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, RAM_DISK_CONFIG_PRIVATE_DATA, ConfigAccess, RAM_DISK_CONFIG_PRIVATE_DATA_SIGNATURE)

/**
  Register a RAM disk with specified address, size and type.

  @param[in]  RamDiskBase    The base address of registered RAM disk.
  @param[in]  RamDiskSize    The size of registered RAM disk.
  @param[in]  RamDiskType    The type of registered RAM disk. The GUID can be
                             any of the values defined in section 9.3.6.9, or a
                             vendor defined GUID.
  @param[in]  ParentDevicePath
                             Pointer to the parent device path. If there is no
                             parent device path then ParentDevicePath is NULL.
  @param[out] DevicePath     On return, points to a pointer to the device path
                             of the RAM disk device.
                             If ParentDevicePath is not NULL, the returned
                             DevicePath is created by appending a RAM disk node
                             to the parent device path. If ParentDevicePath is
                             NULL, the returned DevicePath is a RAM disk device
                             path without appending. This function is
                             responsible for allocating the buffer DevicePath
                             with the boot service AllocatePool().

  @retval EFI_SUCCESS             The RAM disk is registered successfully.
  @retval EFI_INVALID_PARAMETER   DevicePath or RamDiskType is NULL.
                                  RamDiskSize is 0.
  @retval EFI_ALREADY_STARTED     A Device Path Protocol instance to be created
                                  is already present in the handle database.
  @retval EFI_OUT_OF_RESOURCES    The RAM disk register operation fails due to
                                  resource limitation.

**/
EFI_STATUS
EFIAPI
RamDiskRegister (
  IN UINT64                     RamDiskBase,
  IN UINT64                     RamDiskSize,
  IN EFI_GUID                   *RamDiskType,
  IN EFI_DEVICE_PATH            *ParentDevicePath     OPTIONAL,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  );

/**
  Unregister a RAM disk specified by DevicePath.

  @param[in] DevicePath      A pointer to the device path that describes a RAM
                             Disk device.

  @retval EFI_SUCCESS             The RAM disk is unregistered successfully.
  @retval EFI_INVALID_PARAMETER   DevicePath is NULL.
  @retval EFI_UNSUPPORTED         The device specified by DevicePath is not a
                                  valid ramdisk device path and not supported
                                  by the driver.
  @retval EFI_NOT_FOUND           The RAM disk pointed by DevicePath doesn't
                                  exist.

**/
EFI_STATUS
EFIAPI
RamDiskUnregister (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Initialize the BlockIO protocol of a RAM disk device.

  @param[in] PrivateData     Points to RAM disk private data.

**/
VOID
RamDiskInitBlockIo (
  IN     RAM_DISK_PRIVATE_DATA  *PrivateData
  );

/**
  Reset the Block Device.

  @param[in] This            Indicates a pointer to the calling context.
  @param[in] ExtendedVerification
                             Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS             The device was reset.
  @retval EFI_DEVICE_ERROR        The device is not functioning properly and
                                  could not be reset.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  );

/**
  Read BufferSize bytes from Lba into Buffer.

  @param[in]  This           Indicates a pointer to the calling context.
  @param[in]  MediaId        Id of the media, changes every time the media is
                             replaced.
  @param[in]  Lba            The starting Logical Block Address to read from.
  @param[in]  BufferSize     Size of Buffer, must be a multiple of device block
                             size.
  @param[out] Buffer         A pointer to the destination buffer for the data.
                             The caller is responsible for either having
                             implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while performing
                                  the read.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId does not matched the current
                                  device.
  @retval EFI_BAD_BUFFER_SIZE     The Buffer was not a multiple of the block
                                  size of the device.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  OUT VOID                  *Buffer
  );

/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in] This            Indicates a pointer to the calling context.
  @param[in] MediaId         The media ID that the write request is for.
  @param[in] Lba             The starting logical block address to be written.
                             The caller is responsible for writing to only
                             legitimate locations.
  @param[in] BufferSize      Size of Buffer, must be a multiple of device block
                             size.
  @param[in] Buffer          A pointer to the source buffer for the data.

  @retval EFI_SUCCESS             The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED     The device can not be written to.
  @retval EFI_DEVICE_ERROR        The device reported an error while performing
                                  the write.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHNAGED       The MediaId does not matched the current
                                  device.
  @retval EFI_BAD_BUFFER_SIZE     The Buffer was not a multiple of the block
                                  size of the device.
  @retval EFI_INVALID_PARAMETER   The write request contains LBAs that are not
                                  valid, or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  );

/**
  Flush the Block Device.

  @param[in] This            Indicates a pointer to the calling context.

  @retval EFI_SUCCESS             All outstanding data was written to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while writting
                                  back the data
  @retval EFI_NO_MEDIA            There is no media in the device.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  );

/**
  Resets the block device hardware.

  @param[in] This                 The pointer of EFI_BLOCK_IO2_PROTOCOL.
  @param[in] ExtendedVerification The flag about if extend verificate.

  @retval EFI_SUCCESS             The device was reset.
  @retval EFI_DEVICE_ERROR        The block device is not functioning correctly
                                  and could not be reset.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2Reset (
  IN EFI_BLOCK_IO2_PROTOCOL  *This,
  IN BOOLEAN                 ExtendedVerification
  );

/**
  Reads the requested number of blocks from the device.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[in]      MediaId         The media ID that the read request is for.
  @param[in]      Lba             The starting logical block address to read
                                  from on the device.
  @param[in, out] Token           A pointer to the token associated with the
                                  transaction.
  @param[in]      BufferSize      The size of the Buffer in bytes. This must be
                                  a multiple of the intrinsic block size of the
                                  device.
  @param[out]     Buffer          A pointer to the destination buffer for the
                                  data. The caller is responsible for either
                                  having implicit or explicit ownership of the
                                  buffer.

  @retval EFI_SUCCESS             The read request was queued if Token->Event
                                  is not NULL. The data was read correctly from
                                  the device if the Token->Event is NULL.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the read operation.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                  valid, or the buffer is not on proper
                                  alignment.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a
                                  lack of resources.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2ReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  OUT VOID                       *Buffer
  );

/**
  Writes a specified number of blocks to the device.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[in]      MediaId         The media ID that the write request is for.
  @param[in]      Lba             The starting logical block address to be
                                  written. The caller is responsible for
                                  writing to only legitimate locations.
  @param[in, out] Token           A pointer to the token associated with the
                                  transaction.
  @param[in]      BufferSize      The size in bytes of Buffer. This must be a
                                  multiple of the intrinsic block size of the
                                  device.
  @param[in]      Buffer          A pointer to the source buffer for the data.

  @retval EFI_SUCCESS             The write request was queued if Event is not
                                  NULL. The data was written correctly to the
                                  device if the Event is NULL.
  @retval EFI_WRITE_PROTECTED     The device cannot be written to.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to perform the write operation.
  @retval EFI_BAD_BUFFER_SIZE     The BufferSize parameter is not a multiple of
                                  the intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER   The write request contains LBAs that are not
                                  valid, or the buffer is not on proper
                                  alignment.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a
                                  lack of resources.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2WriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  IN     VOID                    *Buffer
  );

/**
  Flushes all modified data to a physical block device.

  @param[in]      This            Indicates a pointer to the calling context.
  @param[in, out] Token           A pointer to the token associated with the
                                  transaction.

  @retval EFI_SUCCESS             The flush request was queued if Event is not
                                  NULL. All outstanding data was written
                                  correctly to the device if the Event is NULL.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting
                                  to write data.
  @retval EFI_WRITE_PROTECTED     The device cannot be written to.
  @retval EFI_NO_MEDIA            There is no media in the device.
  @retval EFI_MEDIA_CHANGED       The MediaId is not for the current media.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a
                                  lack of resources.

**/
EFI_STATUS
EFIAPI
RamDiskBlkIo2FlushBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token
  );

/**
  This function publish the RAM disk configuration Form.

  @param[in, out]  ConfigPrivateData
                             Points to RAM disk configuration private data.

  @retval EFI_SUCCESS             HII Form is installed successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource for HII Form installation.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
InstallRamDiskConfigForm (
  IN OUT RAM_DISK_CONFIG_PRIVATE_DATA  *ConfigPrivateData
  );

/**
  This function removes RAM disk configuration Form.

  @param[in, out]  ConfigPrivateData
                             Points to RAM disk configuration private data.

**/
VOID
UninstallRamDiskConfigForm (
  IN OUT RAM_DISK_CONFIG_PRIVATE_DATA  *ConfigPrivateData
  );

/**
  Unregister all registered RAM disks.

**/
VOID
UnregisterAllRamDisks (
  VOID
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request        A null-terminated Unicode string in
                             <ConfigRequest> format.
  @param[out] Progress       On return, points to a character in the Request
                             string. Points to the string's null terminator if
                             request was successful. Points to the most recent
                             '&' before the first failing name/value pair (or
                             the beginning of the string if the failure is in
                             the first name/value pair) if the request was not
                             successful.
  @param[out] Results        A null-terminated Unicode string in
                             <ConfigAltResp> format which has all values filled
                             in for the names in the Request string. String to
                             be allocated by the called function.

  @retval EFI_SUCCESS             The Results is filled with the requested
                                  values.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER   Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND           Routing data doesn't match any storage in
                                  this driver.

**/
EFI_STATUS
EFIAPI
RamDiskExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT EFI_STRING                           *Progress,
  OUT EFI_STRING                           *Results
  );

/**
  This function processes the results of changes in configuration.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in <ConfigResp>
                             format.
  @param[out] Progress       A pointer to a string filled in with the offset of
                             the most recent '&' before the first failing
                             name/value pair (or the beginning of the string if
                             the failure is in the first name/value pair) or
                             the terminating NULL if all was successful.

  @retval EFI_SUCCESS             The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER   Configuration is NULL.
  @retval EFI_NOT_FOUND           Routing data doesn't match any storage in
                                  this driver.

**/
EFI_STATUS
EFIAPI
RamDiskRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  );

/**
  This function processes the results of changes in configuration.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action         Specifies the type of action taken by the browser.
  @param[in]  QuestionId     A unique value which is sent to the original
                             exporting driver so that it can identify the type
                             of data to expect.
  @param[in]  Type           The type of value for the question.
  @param[in]  Value          A pointer to the data being sent to the original
                             exporting driver.
  @param[out] ActionRequest  On return, points to the action requested by the
                             callback function.

  @retval EFI_SUCCESS             The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES    Not enough storage is available to hold the
                                  variable and its data.
  @retval EFI_DEVICE_ERROR        The variable could not be saved.
  @retval EFI_UNSUPPORTED         The specified Action is not supported by the
                                  callback.

**/
EFI_STATUS
EFIAPI
RamDiskCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                Action,
  IN     EFI_QUESTION_ID                   QuestionId,
  IN     UINT8                             Type,
  IN     EFI_IFR_TYPE_VALUE                *Value,
  OUT EFI_BROWSER_ACTION_REQUEST           *ActionRequest
  );

/**
  This function gets the file information from an open file descriptor,
  and stores it in a buffer allocated from pool.

  @param[in] FHand           File Handle.

  @return    A pointer to a buffer with file information or NULL is returned.

**/
EFI_FILE_INFO *
FileInfo (
  IN EFI_FILE_HANDLE  FHand
  );

/**
  Publish the RAM disk NVDIMM Firmware Interface Table (NFIT) to the ACPI
  table.

  @param[in] PrivateData          Points to RAM disk private data.

  @retval EFI_SUCCESS             The RAM disk NFIT has been published.
  @retval others                  The RAM disk NFIT has not been published.

**/
EFI_STATUS
RamDiskPublishNfit (
  IN RAM_DISK_PRIVATE_DATA  *PrivateData
  );

#endif
