/** @file
  Common defines and definitions for a component update driver.

  Copyright (c) 2002 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_UPDATE_DRIVER_H_
#define _EFI_UPDATE_DRIVER_H_

#include <PiDxe.h>

#include <Protocol/LoadedImage.h>
#include <Guid/Capsule.h>
#include <Guid/CapsuleDataFile.h>
#include <Protocol/FaultTolerantWrite.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/FirmwareVolume2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>

extern EFI_HII_HANDLE gHiiHandle;

typedef enum {
  UpdateWholeFV = 0,              // 0, update whole FV
  UpdateFvFile,                   // 1, update a set of FV files asynchronously
  UpdateFvRange,                  // 2, update part of FV or flash
  UpdateOperationMaximum          // 3
} UPDATE_OPERATION_TYPE;

typedef struct {
  UINTN                           Index;
  UPDATE_OPERATION_TYPE           UpdateType;
  EFI_DEVICE_PATH_PROTOCOL        DevicePath;
  EFI_PHYSICAL_ADDRESS            BaseAddress;
  EFI_GUID                        FileGuid;
  UINTN                           Length;
  BOOLEAN                         FaultTolerant;
} UPDATE_CONFIG_DATA;

typedef struct _SECTION_ITEM SECTION_ITEM;
struct _SECTION_ITEM {
  CHAR8                           *ptrSection;
  UINTN                           SecNameLen;
  CHAR8                           *ptrEntry;
  CHAR8                           *ptrValue;
  SECTION_ITEM                    *ptrNext;
};

typedef struct _COMMENT_LINE COMMENT_LINE;
struct _COMMENT_LINE {
  CHAR8                           *ptrComment;
  COMMENT_LINE                    *ptrNext;
};

typedef struct {
  EFI_GUID                        FileGuid;
} UPDATE_PRIVATE_DATA;

#define MAX_LINE_LENGTH           512
#define EFI_D_UPDATE              EFI_D_ERROR

#define MIN_ALIGNMENT_SIZE        4
#define ALIGN_SIZE(a)   ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)

/**
  Parse Config data file to get the updated data array.

  @param DataBuffer      Config raw file buffer.
  @param BufferSize      Size of raw buffer.
  @param NumOfUpdates    Pointer to the number of update data.
  @param UpdateArray     Pointer to the config of update data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseUpdateDataFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize,
  IN OUT  UINTN                         *NumOfUpdates,
  IN OUT  UPDATE_CONFIG_DATA            **UpdateArray
  );

/**
  Update the whole FV image, and reinsall FVB protocol for the updated FV image.

  @param FvbHandle       Handle of FVB protocol for the updated flash range.
  @param FvbProtocol     FVB protocol.
  @param ConfigData      Config data on updating driver.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.

  @retval EFI_INVALID_PARAMETER  Update type is not UpdateWholeFV.
                                 Or Image size is not same to the size of whole FV.
  @retval EFI_OUT_OF_RESOURCES   No enoug memory is allocated.
  @retval EFI_SUCCESS            FV image is updated, and its FVB protocol is reinstalled.

**/
EFI_STATUS
PerformUpdateOnWholeFv (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize
  );

/**
  Update certain file in the FV.

  @param FvbHandle       Handle of FVB protocol for the updated flash range.
  @param FvbProtocol     FVB protocol.
  @param ConfigData      Config data on updating driver.
  @param ImageBuffer     Image buffer to be updated.
  @param ImageSize       Image size.
  @param FileType        FFS file type.
  @param FileAttributes  FFS file attribute

  @retval EFI_INVALID_PARAMETER  Update type is not UpdateFvFile.
                                 Or Image size is not same to the size of whole FV.
  @retval EFI_UNSUPPORTED        PEIM FFS is unsupported to be updated.
  @retval EFI_SUCCESS            The FFS file is added into FV.

**/
EFI_STATUS
PerformUpdateOnFvFile (
  IN EFI_HANDLE                         FvbHandle,
  IN EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN UPDATE_CONFIG_DATA                 *ConfigData,
  IN UINT8                              *ImageBuffer,
  IN UINTN                              ImageSize,
  IN EFI_FV_FILETYPE                    FileType,
  IN EFI_FV_FILE_ATTRIBUTES             FileAttributes
  );

/**
  Update the buffer into flash area in fault tolerant write method.

  @param ImageBuffer     Image buffer to be updated.
  @param SizeLeft        Size of the image buffer.
  @param UpdatedSize     Size of the updated buffer.
  @param ConfigData      Config data on updating driver.
  @param FlashAddress    Flash address to be updated as start address.
  @param FvbProtocol     FVB protocol.
  @param FvbHandle       Handle of FVB protocol for the updated flash range.

  @retval EFI_SUCCESS            Buffer data is updated into flash.
  @retval EFI_INVALID_PARAMETER  Base flash address is not in FVB flash area.
  @retval EFI_NOT_FOUND          FTW protocol doesn't exist.
  @retval EFI_OUT_OF_RESOURCES   No enough backup space.
  @retval EFI_ABORTED            Error happen when update flash area.

**/
EFI_STATUS
FaultTolerantUpdateOnPartFv (
  IN       UINT8                         *ImageBuffer,
  IN       UINTN                         SizeLeft,
  IN OUT   UINTN                         *UpdatedSize,
  IN       UPDATE_CONFIG_DATA            *ConfigData,
  IN       EFI_PHYSICAL_ADDRESS          FlashAddress,
  IN       EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN       EFI_HANDLE                    FvbHandle
  );

/**
  Directly update the buffer into flash area without fault tolerant write method.

  @param ImageBuffer     Image buffer to be updated.
  @param SizeLeft        Size of the image buffer.
  @param UpdatedSize     Size of the updated buffer.
  @param FlashAddress    Flash address to be updated as start address.
  @param FvbProtocol     FVB protocol.
  @param FvbHandle       Handle of FVB protocol for the updated flash range.

  @retval EFI_SUCCESS            Buffer data is updated into flash.
  @retval EFI_INVALID_PARAMETER  Base flash address is not in FVB flash area.
  @retval EFI_OUT_OF_RESOURCES   No enough backup space.

**/
EFI_STATUS
NonFaultTolerantUpdateOnPartFv (
  IN      UINT8                         *ImageBuffer,
  IN      UINTN                         SizeLeft,
  IN OUT  UINTN                         *UpdatedSize,
  IN      EFI_PHYSICAL_ADDRESS          FlashAddress,
  IN      EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *FvbProtocol,
  IN      EFI_HANDLE                    FvbHandle
  );

#endif
