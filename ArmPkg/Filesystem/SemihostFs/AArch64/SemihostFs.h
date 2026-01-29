/** @file
  Support a Semi Host file system over a debuggers JTAG

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SEMIHOST_FS_H_
#define SEMIHOST_FS_H_

EFI_STATUS
VolumeOpen (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE                         **Root
  );

/**
  Open a file on the host system by means of the semihosting interface.

  @param[in]   This        A pointer to the EFI_FILE_PROTOCOL instance that is
                           the file handle to source location.
  @param[out]  NewHandle   A pointer to the location to return the opened
                           handle for the new file.
  @param[in]   FileName    The Null-terminated string of the name of the file
                           to be opened.
  @param[in]   OpenMode    The mode to open the file : Read or Read/Write or
                           Read/Write/Create
  @param[in]   Attributes  Only valid for EFI_FILE_MODE_CREATE, in which case these
                           are the attribute bits for the newly created file. The
                           mnemonics of the attribute bits are : EFI_FILE_READ_ONLY,
                           EFI_FILE_HIDDEN, EFI_FILE_SYSTEM, EFI_FILE_RESERVED,
                           EFI_FILE_DIRECTORY and EFI_FILE_ARCHIVE.

  @retval  EFI_SUCCESS            The file was open.
  @retval  EFI_NOT_FOUND          The specified file could not be found.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_WRITE_PROTECTED    Attempt to create a directory. This is not possible
                                  with the semi-hosting interface.
  @retval  EFI_OUT_OF_RESOURCES   Not enough resources were available to open the file.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFI_STATUS
FileOpen (
  IN  EFI_FILE  *This,
  OUT EFI_FILE  **NewHandle,
  IN  CHAR16    *FileName,
  IN  UINT64    OpenMode,
  IN  UINT64    Attributes
  );

/**
  Close a specified file handle.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to close.

  @retval  EFI_SUCCESS            The file was closed.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" is NULL.

**/
EFI_STATUS
FileClose (
  IN EFI_FILE  *This
  );

/**
  Close and delete a file.

  @param[in]  This  A pointer to the EFI_FILE_PROTOCOL instance that is the file
                    handle to delete.

  @retval  EFI_SUCCESS              The file was closed and deleted.
  @retval  EFI_WARN_DELETE_FAILURE  The handle was closed, but the file was not deleted.
  @retval  EFI_INVALID_PARAMETER    The parameter "This" is NULL.

**/
EFI_STATUS
FileDelete (
  IN EFI_FILE  *This
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
                                  beyond the end of the file, or the semi-hosting
                                  interface reported an error while performing the
                                  read operation.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" or the parameter "Buffer"
                                  is NULL.
**/
EFI_STATUS
FileRead (
  IN     EFI_FILE  *This,
  IN OUT UINTN     *BufferSize,
  OUT    VOID      *Buffer
  );

/**
  Write data to an open file.

  @param[in]      This        A pointer to the EFI_FILE_PROTOCOL instance that
                              is the file handle to write data to.
  @param[in out]  BufferSize  On input, the size of the Buffer. On output, the
                              size of the data actually written. In both cases,
                              the size is measured in bytes.
  @param[in]      Buffer      The buffer of data to write.

  @retval  EFI_SUCCESS            The data was written.
  @retval  EFI_ACCESS_DENIED      Attempt to write into a read only file or
                                  in a file opened in read only mode.
  @retval  EFI_DEVICE_ERROR       The last issued semi-hosting operation failed.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" or the parameter "Buffer"
                                  is NULL.

**/
EFI_STATUS
FileWrite (
  IN     EFI_FILE  *This,
  IN OUT UINTN     *BufferSize,
  IN     VOID      *Buffer
  );

/**
  Return a file's current position.

  @param[in]   This      A pointer to the EFI_FILE_PROTOCOL instance that is
                         the file handle to get the current position on.
  @param[out]  Position  The address to return the file's current position value.

  @retval  EFI_SUCCESS            The position was returned.
  @retval  EFI_INVALID_PARAMETER  Position is a NULL pointer.

**/
EFI_STATUS
FileGetPosition (
  IN  EFI_FILE  *File,
  OUT UINT64    *Position
  );

/**
  Set a file's current position.

  @param[in]  This      A pointer to the EFI_FILE_PROTOCOL instance that is
                        the file handle to set the requested position on.
  @param[in]  Position  The byte position from the start of the file to set.

  @retval  EFI_SUCCESS       The position was set.
  @retval  EFI_DEVICE_ERROR  The semi-hosting positioning operation failed.
  @retval  EFI_UNSUPPORTED   The seek request for nonzero is not valid on open
                             directories.

**/
EFI_STATUS
FileSetPosition (
  IN EFI_FILE  *File,
  IN UINT64    Position
  );

/**
  Return information about a file or a file system.

  @param[in]      This             A pointer to the EFI_FILE_PROTOCOL instance that
                                   is the file handle the requested information is for.
  @param[in]      InformationType  The type identifier for the information being requested :
                                   EFI_FILE_INFO_ID or EFI_FILE_SYSTEM_INFO_ID or
                                   EFI_FILE_SYSTEM_VOLUME_LABEL_ID
  @param[in out]  BufferSize       The size, in bytes, of Buffer.
  @param[out]     Buffer           A pointer to the data buffer to return. The type of the
                                   data inside the buffer is indicated by InformationType.

  @retval  EFI_SUCCESS           The information was returned.
  @retval  EFI_UNSUPPORTED       The InformationType is not known.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize is too small to return the information.
                                 BufferSize has been updated with the size needed to
                                 complete the request.
  @retval  EFI_INVALID_PARAMETER  The parameter "This" or the parameter "Buffer"
                                  is NULL.

**/
EFI_STATUS
FileGetInfo (
  IN     EFI_FILE  *This,
  IN     EFI_GUID  *InformationType,
  IN OUT UINTN     *BufferSize,
  OUT    VOID      *Buffer
  );

/**
  Set information about a file or a file system.

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
  @retval  EFI_ACCESS_DENIED      An attempt is being made to change the
                                  EFI_FILE_DIRECTORY Attribute.
  @retval  EFI_ACCESS_DENIED      InformationType is EFI_FILE_INFO_ID and
                                  the file is a read-only file or has been
                                  opened in read-only mode and an attempt is
                                  being made to modify a field other than
                                  Attribute.
  @retval  EFI_ACCESS_DENIED      An attempt is made to change the name of a file
                                  to a file that is already present.
  @retval  EFI_WRITE_PROTECTED    An attempt is being made to modify a
                                  read-only attribute.
  @retval  EFI_BAD_BUFFER_SIZE    The size of the buffer is lower than that indicated by
                                  the data inside the buffer.
  @retval  EFI_OUT_OF_RESOURCES   An allocation needed to process the request failed.
  @retval  EFI_INVALID_PARAMETER  At least one of the parameters is invalid.

**/
EFI_STATUS
FileSetInfo (
  IN EFI_FILE  *This,
  IN EFI_GUID  *InformationType,
  IN UINTN     BufferSize,
  IN VOID      *Buffer
  );

EFI_STATUS
FileFlush (
  IN EFI_FILE  *File
  );

#endif // SEMIHOST_FS_H_
