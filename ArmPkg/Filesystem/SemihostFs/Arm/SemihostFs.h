/** @file
  Support a Semi Host file system over a debuggers JTAG

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SEMIHOST_FS_H__
#define __SEMIHOST_FS_H__

EFI_STATUS
VolumeOpen (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
  OUT EFI_FILE                        **Root
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

EFI_STATUS
FileClose (
  IN EFI_FILE *File
  );

EFI_STATUS
FileDelete(
  IN EFI_FILE *File
  );

EFI_STATUS
FileRead (
  IN     EFI_FILE *File,
  IN OUT UINTN    *BufferSize,
  OUT    VOID     *Buffer
  );

EFI_STATUS
FileWrite (
  IN     EFI_FILE *File,
  IN OUT UINTN    *BufferSize,
  IN     VOID     *Buffer
  );

EFI_STATUS
FileGetPosition (
  IN  EFI_FILE  *File,
  OUT UINT64    *Position
  );

EFI_STATUS
FileSetPosition (
  IN EFI_FILE *File,
  IN UINT64   Position
  );

EFI_STATUS
FileGetInfo (
  IN     EFI_FILE *File,
  IN     EFI_GUID *InformationType,
  IN OUT UINTN    *BufferSize,
  OUT    VOID     *Buffer
  );

EFI_STATUS
FileSetInfo (
  IN EFI_FILE *File,
  IN EFI_GUID *InformationType,
  IN UINTN    BufferSize,
  IN VOID     *Buffer
  );

EFI_STATUS
FileFlush (
  IN EFI_FILE *File
  );

#endif // __SEMIHOST_FS_H__

