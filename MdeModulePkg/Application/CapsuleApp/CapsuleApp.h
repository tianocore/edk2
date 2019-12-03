/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _CAPSULE_APP_H_
#define _CAPSULE_APP_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/FileHandleLib.h>
#include <Library/SortLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>
#include <Protocol/FirmwareManagement.h>
#include <Guid/GlobalVariable.h>
#include <Guid/CapsuleReport.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/FmpCapsule.h>
#include <Guid/FileInfo.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/Gpt.h>
#include <IndustryStandard/WindowsUxCapsule.h>

#define CAPSULE_HEADER_SIZE  0x20

#define NESTED_CAPSULE_HEADER_SIZE  SIZE_4KB
#define SYSTEM_FIRMWARE_FLAG 0x50000
#define DEVICE_FIRMWARE_FLAG 0x78010

#define MAJOR_VERSION   1
#define MINOR_VERSION   0

#define MAX_CAPSULE_NUM 10

//
// (20 * (6+5+2))+1) unicode characters from EFI FAT spec (doubled for bytes)
//
#define MAX_FILE_NAME_SIZE   522
#define MAX_FILE_NAME_LEN    (MAX_FILE_NAME_SIZE / sizeof(CHAR16))

extern UINTN  Argc;
extern CHAR16 **Argv;

/**

  This function parse application ARG.

  @return Status
**/
EFI_STATUS
GetArg (
  VOID
  );

/**
  Get shell protocol.

  @return Pointer to shell protocol.

**/
EFI_SHELL_PROTOCOL *
GetShellProtocol (
  VOID
  );


/**
  Read a file.

  @param[in]  FileName        The file to be read.
  @param[out] BufferSize      The file buffer size
  @param[out] Buffer          The file buffer

  @retval EFI_SUCCESS    Read file successfully
  @retval EFI_NOT_FOUND  Shell protocol or file not found
  @retval others         Read file failed
**/
EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16                               *FileName,
  OUT UINTN                                *BufferSize,
  OUT VOID                                 **Buffer
  );

/**
  Write a file.

  @param[in] FileName        The file to be written.
  @param[in] BufferSize      The file buffer size
  @param[in] Buffer          The file buffer

  @retval EFI_SUCCESS    Write file successfully
  @retval EFI_NOT_FOUND  Shell protocol not found
  @retval others         Write file failed
**/
EFI_STATUS
WriteFileFromBuffer (
  IN  CHAR16                               *FileName,
  IN  UINTN                                BufferSize,
  IN  VOID                                 *Buffer
  );


/**
  Dump capsule information

  @param[in] CapsuleName  The name of the capsule image.

  @retval EFI_SUCCESS            The capsule information is dumped.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
**/
EFI_STATUS
DumpCapsule (
  IN CHAR16                                        *CapsuleName
  );

/**
  Dump capsule status variable.

  @retval EFI_SUCCESS            The capsule status variable is dumped.
  @retval EFI_UNSUPPORTED        Input parameter is not valid.
**/
EFI_STATUS
DumpCapsuleStatusVariable (
  VOID
  );

/**
  Dump FMP protocol info.
**/
VOID
DumpFmpData (
  VOID
  );

/**
  Dump FMP image data.

  @param[in]  ImageTypeId   The ImageTypeId of the FMP image.
                            It is used to identify the FMP protocol.
  @param[in]  ImageIndex    The ImageIndex of the FMP image.
                            It is the input parameter for FMP->GetImage().
  @param[in]  ImageName     The file name to hold the output FMP image.
**/
VOID
DumpFmpImage (
  IN EFI_GUID  *ImageTypeId,
  IN UINTN     ImageIndex,
  IN CHAR16    *ImageName
  );

/**
  Dump ESRT info.
**/
VOID
DumpEsrtData (
  VOID
  );

/**
  Dump Provisioned Capsule.

  @param[in]  DumpCapsuleInfo  The flag to indicate whether to dump the capsule inforomation.
**/
VOID
DumpProvisionedCapsule (
  IN BOOLEAN                      DumpCapsuleInfo
  );

/**
  Dump all EFI System Partition.
**/
VOID
DumpAllEfiSysPartition (
  VOID
  );


/**
  Get SimpleFileSystem from boot option file path.

  @param[in]  DevicePath     The file path of boot option
  @param[out] FullPath       The full device path of boot device
  @param[out] Fs             The file system within EfiSysPartition

  @retval EFI_SUCCESS    Get file system successfully
  @retval EFI_NOT_FOUND  No valid file system found
  @retval others         Get file system failed

**/
EFI_STATUS
GetEfiSysPartitionFromBootOptionFilePath (
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL         **FullPath,
  OUT EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  **Fs
  );


/**
  Process Capsule On Disk.

  @param[in]  CapsuleBuffer       An array of pointer to capsule images
  @param[in]  CapsuleBufferSize   An array of UINTN to capsule images size
  @param[in]  FilePath            An array of capsule images file path
  @param[in]  Map                 File system mapping string
  @param[in]  CapsuleNum          The count of capsule images

  @retval EFI_SUCCESS       Capsule on disk success.
  @retval others            Capsule on disk fail.

**/
EFI_STATUS
ProcessCapsuleOnDisk (
  IN VOID                          **CapsuleBuffer,
  IN UINTN                         *CapsuleBufferSize,
  IN CHAR16                        **FilePath,
  IN CHAR16                        *Map,
  IN UINTN                         CapsuleNum
  );

#endif

