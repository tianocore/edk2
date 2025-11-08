/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>

#include <Library/ArmCompatSemiHostingLib.h>
#include <Library/BaseLib.h>
#include "SemiHostingPrivate.h"

STATIC BOOLEAN  mSemiHostingChecked   = FALSE;
STATIC BOOLEAN  mSemiHostingSupported = TRUE;

/**
  Check if semihosting connection is supported.

  This function determines whether a semihosting connection is currently
  available and operational.

  @retval TRUE   Semihosting connection is supported and available.
  @retval FALSE  Semihosting is not supported or connection not established.
**/
BOOLEAN
EFIAPI
SemiHostingConnectionSupported (
  VOID
  )
{
  BOOLEAN  SemiHostingSupported;

  if (!mSemiHostingChecked) {
    SemiHostingSupported = (SemiHostingConnectionEnabled () != 0);
    mSemiHostingChecked  = TRUE;
  }

  mSemiHostingSupported = SemiHostingSupported;
  return SemiHostingSupported;
}

/**
  Open a file on the host system using semihosting interface.

  @param[in]  FileName   Null-terminated ASCII string of the file name.
  @param[in]  Mode       Access mode (read, write, append, etc.).
  @param[out] FileHandle Returned file handle identifier if successful.

  @retval  RETURN_SUCCESS            File opened successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter passed.
  @retval  RETURN_NOT_FOUND          File could not be opened.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileOpen (
  IN  CHAR8   *FileName,
  IN  UINT32  Mode,
  OUT UINTN   *FileHandle
  )
{
  SEMIHOSTING_FILE_OPEN_BLOCK  OpenBlock;
  INT32                        Result;

  if (FileHandle == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  // Remove any leading separator (e.g.: '\'). EFI Shell adds one.
  if (*FileName == '\\') {
    FileName++;
  }

  OpenBlock.FileName   = FileName;
  OpenBlock.Mode       = Mode;
  OpenBlock.NameLength = AsciiStrLen (FileName);

  Result = SEMIHOSTING_SYS_OPEN (&OpenBlock);

  if (Result == -1) {
    return RETURN_NOT_FOUND;
  } else {
    *FileHandle = Result;
    return RETURN_SUCCESS;
  }
}

/**
  Move the file position to a specified offset.

  @param[in]  FileHandle  Handle of the open file.
  @param[in]  Offset      Offset (in bytes) to seek to, relative to start of file.

  @retval  RETURN_SUCCESS            File pointer repositioned successfully.
  @retval  RETURN_ABORTED            Seek operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileSeek (
  IN UINTN  FileHandle,
  IN UINTN  Offset
  )
{
  SEMIHOSTING_FILE_SEEK_BLOCK  SeekBlock;
  INT32                        Result;

  SeekBlock.Handle   = FileHandle;
  SeekBlock.Location = Offset;

  Result = SEMIHOSTING_SYS_SEEK (&SeekBlock);

  if (Result < 0) {
    return RETURN_ABORTED;
  } else {
    return RETURN_SUCCESS;
  }
}

/**
  Read data from a file on the host system via semihosting.

  @param[in]      FileHandle  Handle of the open file.
  @param[in, out] Length      On input, size of Buffer in bytes.
                              On output, number of bytes actually read.
  @param[out]     Buffer      Buffer to store the read data.

  @retval  RETURN_SUCCESS            Data read successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid buffer or handle.
  @retval  RETURN_ABORTED            Read failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileRead (
  IN     UINTN  FileHandle,
  IN OUT UINTN  *Length,
  OUT    VOID   *Buffer
  )
{
  SEMIHOSTING_FILE_READ_WRITE_BLOCK  ReadBlock;
  UINT32                             Result;

  if ((Length == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  ReadBlock.Handle = FileHandle;
  ReadBlock.Buffer = Buffer;
  ReadBlock.Length = *Length;

  Result = SEMIHOSTING_SYS_READ (&ReadBlock);

  if ((*Length != 0) && (Result == *Length)) {
    return RETURN_ABORTED;
  } else {
    *Length -= Result;
    return RETURN_SUCCESS;
  }
}

/**
  Write data to a file on the host system via semihosting.

  @param[in]      FileHandle  Handle of the open file.
  @param[in, out] Length      On input, number of bytes to write.
                              On output, number of bytes actually written.
  @param[in]      Buffer      Pointer to data buffer to write.

  @retval  RETURN_SUCCESS            Data written successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid handle or buffer.
  @retval  RETURN_ABORTED            Write operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileWrite (
  IN     UINTN  FileHandle,
  IN OUT UINTN  *Length,
  IN     VOID   *Buffer
  )
{
  SEMIHOSTING_FILE_READ_WRITE_BLOCK  WriteBlock;

  if ((Length == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  WriteBlock.Handle = FileHandle;
  WriteBlock.Buffer = Buffer;
  WriteBlock.Length = *Length;

  *Length = SEMIHOSTING_SYS_WRITE (&WriteBlock);

  if (*Length != 0) {
    return RETURN_ABORTED;
  } else {
    return RETURN_SUCCESS;
  }
}

/**
  Close an open file on the host system via semihosting.

  @param[in]  FileHandle  Handle of the file to close.

  @retval  RETURN_SUCCESS            File closed successfully.
  @retval  RETURN_ABORTED            Close operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileClose (
  IN UINTN  FileHandle
  )
{
  if (SEMIHOSTING_SYS_CLOSE (&FileHandle) == -1) {
    return RETURN_ABORTED;
  } else {
    return RETURN_SUCCESS;
  }
}

/**
  Get the total length of a file.

  @param[in]   FileHandle  Handle of the open file.
  @param[out]  Length      Pointer to store the file length in bytes.

  @retval  RETURN_SUCCESS            File length retrieved successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid length pointer.
  @retval  RETURN_ABORTED            Operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileLength (
  IN  UINTN  FileHandle,
  OUT UINTN  *Length
  )
{
  INT32  Result;

  if (Length == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Result = SEMIHOSTING_SYS_FLEN (&FileHandle);

  if (Result == -1) {
    return RETURN_ABORTED;
  } else {
    *Length = Result;
    return RETURN_SUCCESS;
  }
}

/**
  Get a temporary name for a file from the host running the debug agent.

  @param[out]  Buffer      Pointer to the buffer where the temporary name has to
                           be stored
  @param[in]   Identifier  File name identifier (integer in the range 0 to 255)
  @param[in]   Length      Length of the buffer to store the temporary name

  @retval  RETURN_SUCCESS            Temporary name returned
  @retval  RETURN_INVALID_PARAMETER  Invalid buffer address
  @retval  RETURN_ABORTED            Temporary name not returned

**/
RETURN_STATUS
EFIAPI
SemiHostingFileTmpName (
  OUT  VOID   *Buffer,
  IN   UINT8  Identifier,
  IN   UINTN  Length
  )
{
  SEMIHOSTING_FILE_TMPNAME_BLOCK  TmpNameBlock;
  INT32                           Result;

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  TmpNameBlock.Buffer     = Buffer;
  TmpNameBlock.Identifier = Identifier;
  TmpNameBlock.Length     = Length;

  Result = SEMIHOSTING_SYS_TMPNAME (&TmpNameBlock);

  if (Result != 0) {
    return RETURN_ABORTED;
  } else {
    return RETURN_SUCCESS;
  }
}

/**
  Delete a specified file on the host via semihosting.

  @param[in]  FileName  Null-terminated ASCII string of the file name to remove.

  @retval  RETURN_SUCCESS            File removed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid file name.
  @retval  RETURN_ABORTED            File could not be removed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileRemove (
  IN CHAR8  *FileName
  )
{
  SEMIHOSTING_FILE_REMOVE_BLOCK  RemoveBlock;
  UINT32                         Result;

  if (FileName == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  // Remove any leading separator (e.g.: '\'). EFI Shell adds one.
  if (*FileName == '\\') {
    FileName++;
  }

  RemoveBlock.FileName   = FileName;
  RemoveBlock.NameLength = AsciiStrLen (FileName);

  Result = SEMIHOSTING_SYS_REMOVE (&RemoveBlock);

  if (Result == 0) {
    return RETURN_SUCCESS;
  } else {
    return RETURN_ABORTED;
  }
}

/**
  Rename a specified file.

  @param[in]  FileName     Name of the file to rename.
  @param[in]  NewFileName  The new name of the file.

  @retval  RETURN_SUCCESS            File Renamed
  @retval  RETURN_INVALID_PARAMETER  Either the current or the new name is not specified
  @retval  RETURN_ABORTED            Rename failed

**/
RETURN_STATUS
EFIAPI
SemiHostingFileRename (
  IN  CHAR8  *FileName,
  IN  CHAR8  *NewFileName
  )
{
  SEMIHOSTING_FILE_RENAME_BLOCK  RenameBlock;
  INT32                          Result;

  if ((FileName == NULL) || (NewFileName == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  RenameBlock.FileName          = FileName;
  RenameBlock.FileNameLength    = AsciiStrLen (FileName);
  RenameBlock.NewFileName       = NewFileName;
  RenameBlock.NewFileNameLength = AsciiStrLen (NewFileName);

  Result = SEMIHOSTING_SYS_RENAME (&RenameBlock);

  if (Result != 0) {
    return RETURN_ABORTED;
  } else {
    return RETURN_SUCCESS;
  }
}

/**
  Read a single character from the host input via semihosting.

  @retval  The ASCII value of the character read.
**/
CHAR8
EFIAPI
SemiHostingReadCharacter (
  VOID
  )
{
  return SEMIHOSTING_SYS_READC ();
}

/**
  Write a single character to the host output via semihosting.

  @param[in]  Character  ASCII character to write.

**/
VOID
EFIAPI
SemiHostingWriteCharacter (
  IN CHAR8  Character
  )
{
  SEMIHOSTING_SYS_WRITEC (&Character);
}

/**
  Write a null-terminated string to the host output via semihosting.

  @param[in]  String  Pointer to a null-terminated ASCII string.
**/
VOID
EFIAPI
SemiHostingWriteString (
  IN CHAR8  *String
  )
{
  SEMIHOSTING_SYS_WRITE0 (String);
}

/**
  Execute a command line on the host system.

  @param[in]  CommandLine  Null-terminated ASCII string containing the command to execute.

  @retval  The return code from the host system for the executed command.
**/
UINT32
EFIAPI
SemiHostingSystem (
  IN CHAR8  *CommandLine
  )
{
  return SEMIHOSTING_SYS_SYSTEM (CommandLine);
}
