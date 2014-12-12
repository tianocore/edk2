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

/**
  Flush all modified data associated with a file to a device.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the
                    file handle to flush.

  @retval  EFI_SUCCESS            The data was flushed.
  @retval  EFI_ACCESS_DENIED      The file was opened read-only.
  @retval  EFI_DEVICE_ERROR       The device reported an error.
  @retval  EFI_VOLUME_FULL        The volume is full.
  @retval  EFI_OUT_OF_RESOURCES   Not enough resources were available to flush the data.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFIAPI
EFI_STATUS
BootMonFsFlushFile (
  IN EFI_FILE_PROTOCOL  *This
  );

/**
  Close a specified file handle.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to close.

  @retval  EFI_SUCCESS            The file was closed.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" is NULL or is not an open
                                  file handle.

**/
EFIAPI
EFI_STATUS
BootMonFsCloseFile (
  IN EFI_FILE_PROTOCOL  *This
  );

/**
  Open a file on the boot monitor file system.

  The boot monitor file system does not allow for sub-directories. There is only
  one directory, the root one. On any attempt to create a directory, the function
  returns in error with the EFI_WRITE_PROTECTED error code.

  @param[in]   This        A pointer to the EFI_FILE_PROTOCOL instance that is
                           the file handle to source location.
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
                                  with the Boot Monitor file system.
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

/**
  Write data to an open file.

  The data is not written to the flash yet. It will be written when the file
  will be either read, closed or flushed.

  @param[in]      This        A pointer to the EFI_FILE_PROTOCOL instance that
                              is the file handle to write data to.
  @param[in out]  BufferSize  On input, the size of the Buffer. On output, the
                              size of the data actually written. In both cases,
                              the size is measured in bytes.
  @param[in]      Buffer      The buffer of data to write.

  @retval  EFI_SUCCESS            The data was written.
  @retval  EFI_ACCESS_DENIED      The file was opened read only.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate the buffer to store the
                                  data to write.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
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

/**
  Close and delete a file from the boot monitor file system.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to delete.

  @retval  EFI_SUCCESS              The file was closed and deleted.
  @retval  EFI_INVALID_PARAMETER    The parameter "This" is NULL or is not an open
                                    file handle.
  @retval  EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not deleted.

**/
EFIAPI
EFI_STATUS
BootMonFsDelete (
  IN EFI_FILE_PROTOCOL *This
  );

/**
  Set a file's current position.

  @param[in]  This      A pointer to the EFI_FILE_PROTOCOL instance that is
                        the file handle to set the requested position on.
  @param[in]  Position  The byte position from the start of the file to set.

  @retval  EFI_SUCCESS            The position was set.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFIAPI
EFI_STATUS
BootMonFsSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  );

/**
  Return a file's current position.

  @param[in]   This      A pointer to the EFI_FILE_PROTOCOL instance that is
                         the file handle to get the current position on.
  @param[out]  Position  The address to return the file's current position value.

  @retval  EFI_SUCCESS            The position was returned.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
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

/**
  Set information about a file or a volume.

  @param[in]  This             A pointer to the EFI_FILE_PROTOCOL instance that
                               is the file handle the information is for.
  @param[in]  InformationType  The type identifier for the information being set :
                               EFI_FILE_INFO_ID or EFI_FILE_SYSTEM_INFO_ID or
                               EFI_FILE_SYSTEM_VOLUME_LABEL_ID
  @param[in]  BufferSize       The size, in bytes, of Buffer.
  @param[in]  Buffer           A pointer to the data buffer to write. The type of the
                               data inside the buffer is indicated by InformationType.

  @retval  EFI_SUCCESS            The information was set.
  @retval  EFI_UNSUPPORTED        The InformationType is not known.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_ACCESS_DENIED      An attempt is made to change the name of a file
                                  to a file that is already present.
  @retval  EFI_ACCESS_DENIED      An attempt is being made to change the
                                  EFI_FILE_DIRECTORY Attribute.
  @retval  EFI_ACCESS_DENIED      InformationType is EFI_FILE_INFO_ID and
                                  the file was opened in read-only mode and an
                                  attempt is being made to modify a field other
                                  than Attribute.
  @retval  EFI_WRITE_PROTECTED    An attempt is being made to modify a read-only
                                  attribute.
  @retval  EFI_BAD_BUFFER_SIZE    The size of the buffer is lower than that indicated by
                                  the data inside the buffer.
  @retval  EFI_OUT_OF_RESOURCES   A allocation needed to process the request failed.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
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

/**
  Search for a file given its name coded in Ascii.

  When searching through the files of the volume, if a file is currently not
  open, its name was written on the media and is kept in RAM in the
  "HwDescription.Footer.Filename[]" field of the file's description.

  If a file is currently open, its name might not have been written on the
  media yet, and as the "HwDescription" is a mirror in RAM of what is on the
  media the "HwDescription.Footer.Filename[]" might be outdated. In that case,
  the up to date name of the file is stored in the "Info" field of the file's
  description.

  @param[in]   Instance       Pointer to the description of the volume in which
                              the file has to be search for.
  @param[in]   AsciiFileName  Name of the file.

  @param[out]  File           Pointer to the description of the file if the
                              file was found.

  @retval  EFI_SUCCESS    The file was found.
  @retval  EFI_NOT_FOUND  The file was not found.

**/
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
