/** @file
*
*  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __BOOTMON_FS_API_H
#define __BOOTMON_FS_API_H

#include <Protocol/SimpleFileSystem.h>

EFI_STATUS
BootMonFsInitialize (
  IN BOOTMON_FS_INSTANCE *Instance
  );

UINT32
BootMonFsChecksum (
  IN VOID   *Data,
  IN UINT32 Size
  );

EFI_STATUS
BootMonFsComputeFooterChecksum (
  IN OUT HW_IMAGE_DESCRIPTION *Footer
  );

EFIAPI
EFI_STATUS
OpenBootMonFsOpenVolume (
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE_PROTOCOL              **Root
  );

UINT32
BootMonFsGetImageLength (
  IN BOOTMON_FS_FILE      *File
  );

UINTN
BootMonFsGetPhysicalSize (
  IN BOOTMON_FS_FILE* File
  );

EFI_STATUS
BootMonFsCreateFile (
  IN  BOOTMON_FS_INSTANCE *Instance,
  OUT BOOTMON_FS_FILE     **File
  );

EFIAPI
EFI_STATUS
BootMonFsGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

EFIAPI
EFI_STATUS
BootMonFsReadDirectory (
  IN EFI_FILE_PROTOCOL    *This,
  IN OUT UINTN            *BufferSize,
  OUT VOID                *Buffer
  );

EFIAPI
EFI_STATUS
BootMonFsFlushDirectory (
  IN EFI_FILE_PROTOCOL  *This
  );

EFIAPI
EFI_STATUS
BootMonFsFlushFile (
  IN EFI_FILE_PROTOCOL  *This
  );

EFIAPI
EFI_STATUS
BootMonFsCloseFile (
  IN EFI_FILE_PROTOCOL  *This
  );

/**
  Open a file on the boot monitor file system.

  @param[in]   This        The EFI_FILE_PROTOCOL parent handle.
  @param[out]  NewHandle   A pointer to the location to return the opened
                           handle for the new file.
  @param[in]   FileName    The Null-terminated string of the name of the file
                           to be opened.
  @param[in]   OpenMode    The mode to open the file : Read or Read/Write or
                           Read/Write/Create
  @param[in]   Attributes  Attributes of the file in case of a file creation

  @retval  EFI_SUCCESS            The file was open.
  @retval  EFI_NOT_FOUND          The specified file could not be found or the specified
                                  directory in which to create a file could not be found.
  @retval  EFI_DEVICE_ERROR       The device reported an error.
  @retval  EFI_WRITE_PROTECTED    Attempt to create a directory. This is not possible
                                  with the BootMon file system.
  @retval  EFI_OUT_OF_RESOURCES   Not enough resources were available to open the file.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFIAPI
EFI_STATUS
BootMonFsOpenFile (
  IN EFI_FILE_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN CHAR16             *FileName,
  IN UINT64             OpenMode,
  IN UINT64             Attributes
  );

/**
  Read data from an open file.

  @param[in]      This        A pointer to the EFI_FILE_PROTOCOL instance that
                              is the file handle to read data from.
  @param[in out]  BufferSize  On input, the size of the Buffer. On output, the
                              amount of data returned in Buffer. In both cases,
                              the size is measured in bytes.
  @param[out]     Buffer      The buffer into which the data is read.

  @retval  EFI_SUCCESS            The data was read.
  @retval  EFI_DEVICE_ERROR       On entry, the current file position is
                                  beyond the end of the file, or the device
                                  reported an error while performing the read
                                  operation.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.
**/
EFIAPI
EFI_STATUS
BootMonFsReadFile (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

EFIAPI
EFI_STATUS
BootMonFsSetDirPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  );

EFIAPI
EFI_STATUS
BootMonFsGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  );

EFIAPI
EFI_STATUS
BootMonFsWriteFile (
  IN EFI_FILE_PROTOCOL  *This,
  IN OUT UINTN          *BufferSize,
  IN VOID               *Buffer
  );

EFIAPI
EFI_STATUS
BootMonFsDeleteFail (
  IN EFI_FILE_PROTOCOL *This
  );

EFIAPI
EFI_STATUS
BootMonFsDelete (
  IN EFI_FILE_PROTOCOL *This
  );

EFIAPI
EFI_STATUS
BootMonFsSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  );

EFIAPI
EFI_STATUS
BootMonFsGetPosition(
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  );

//
// UNSUPPORTED OPERATIONS
//

EFIAPI
EFI_STATUS
BootMonFsGetPositionUnsupported (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  );

EFIAPI
EFI_STATUS
BootMonFsSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  );

//
// Directory API
//

EFI_STATUS
BootMonFsOpenDirectory (
  OUT EFI_FILE_PROTOCOL **NewHandle,
  IN CHAR16             *FileName,
  IN BOOTMON_FS_INSTANCE *Volume
  );

//
// Internal API
//
EFI_STATUS
BootMonGetFileFromAsciiFileName (
  IN  BOOTMON_FS_INSTANCE   *Instance,
  IN  CHAR8*                AsciiFileName,
  OUT BOOTMON_FS_FILE       **File
  );

EFI_STATUS
BootMonGetFileFromPosition (
  IN  BOOTMON_FS_INSTANCE   *Instance,
  IN  UINTN                 Position,
  OUT BOOTMON_FS_FILE       **File
  );

#endif
