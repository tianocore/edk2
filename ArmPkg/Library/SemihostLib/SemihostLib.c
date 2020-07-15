/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/SemihostLib.h>

#include "SemihostPrivate.h"

BOOLEAN
SemihostConnectionSupported (
  VOID
  )
{
  return SEMIHOST_SUPPORTED;
}

RETURN_STATUS
SemihostFileOpen (
  IN  CHAR8  *FileName,
  IN  UINT32 Mode,
  OUT UINTN  *FileHandle
  )
{
  SEMIHOST_FILE_OPEN_BLOCK  OpenBlock;
  INT32                     Result;

  if (FileHandle == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  // Remove any leading separator (e.g.: '\'). EFI Shell adds one.
  if (*FileName == '\\') {
    FileName++;
  }

  OpenBlock.FileName    = FileName;
  OpenBlock.Mode        = Mode;
  OpenBlock.NameLength  = AsciiStrLen(FileName);

  Result = Semihost_SYS_OPEN(&OpenBlock);

  if (Result == -1) {
    return RETURN_NOT_FOUND;
  } else {
    *FileHandle = Result;
    return RETURN_SUCCESS;
  }
}

RETURN_STATUS
SemihostFileSeek (
  IN UINTN  FileHandle,
  IN UINTN  Offset
  )
{
  SEMIHOST_FILE_SEEK_BLOCK  SeekBlock;
  INT32                     Result;

  SeekBlock.Handle   = FileHandle;
  SeekBlock.Location = Offset;

  Result = Semihost_SYS_SEEK(&SeekBlock);

  // Semihosting does not behave as documented. It returns the offset on
  // success.
  if (Result < 0) {
    return RETURN_ABORTED;
  } else {
    return RETURN_SUCCESS;
  }
}

RETURN_STATUS
SemihostFileRead (
  IN     UINTN  FileHandle,
  IN OUT UINTN  *Length,
  OUT    VOID   *Buffer
  )
{
  SEMIHOST_FILE_READ_WRITE_BLOCK  ReadBlock;
  UINT32                          Result;

  if ((Length == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  ReadBlock.Handle = FileHandle;
  ReadBlock.Buffer = Buffer;
  ReadBlock.Length = *Length;

  Result = Semihost_SYS_READ(&ReadBlock);

  if ((*Length != 0) && (Result == *Length)) {
    return RETURN_ABORTED;
  } else {
    *Length -= Result;
    return RETURN_SUCCESS;
  }
}

RETURN_STATUS
SemihostFileWrite (
  IN     UINTN  FileHandle,
  IN OUT UINTN  *Length,
  IN     VOID   *Buffer
  )
{
  SEMIHOST_FILE_READ_WRITE_BLOCK  WriteBlock;

  if ((Length == NULL) || (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  WriteBlock.Handle = FileHandle;
  WriteBlock.Buffer = Buffer;
  WriteBlock.Length = *Length;

  *Length = Semihost_SYS_WRITE(&WriteBlock);

  if (*Length != 0)
    return RETURN_ABORTED;
  else
    return RETURN_SUCCESS;
}

RETURN_STATUS
SemihostFileClose (
  IN UINTN  FileHandle
  )
{
  INT32 Result = Semihost_SYS_CLOSE(&FileHandle);

  if (Result == -1) {
    return RETURN_INVALID_PARAMETER;
  } else {
    return RETURN_SUCCESS;
  }
}

RETURN_STATUS
SemihostFileLength (
  IN  UINTN  FileHandle,
  OUT UINTN  *Length
  )
{
  INT32       Result;

  if (Length == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  Result = Semihost_SYS_FLEN(&FileHandle);

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
SemihostFileTmpName(
  OUT  VOID   *Buffer,
  IN   UINT8  Identifier,
  IN   UINTN  Length
  )
{
  SEMIHOST_FILE_TMPNAME_BLOCK  TmpNameBlock;
  INT32                        Result;

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  TmpNameBlock.Buffer     = Buffer;
  TmpNameBlock.Identifier = Identifier;
  TmpNameBlock.Length     = Length;

  Result = Semihost_SYS_TMPNAME (&TmpNameBlock);

  if (Result != 0) {
    return  RETURN_ABORTED;
  } else {
    return  RETURN_SUCCESS;
  }
}

RETURN_STATUS
SemihostFileRemove (
  IN CHAR8 *FileName
  )
{
  SEMIHOST_FILE_REMOVE_BLOCK  RemoveBlock;
  UINT32                      Result;

  // Remove any leading separator (e.g.: '\'). EFI Shell adds one.
  if (*FileName == '\\') {
    FileName++;
  }

  RemoveBlock.FileName    = FileName;
  RemoveBlock.NameLength  = AsciiStrLen(FileName);

  Result = Semihost_SYS_REMOVE(&RemoveBlock);

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
SemihostFileRename(
  IN  CHAR8  *FileName,
  IN  CHAR8  *NewFileName
  )
{
  SEMIHOST_FILE_RENAME_BLOCK  RenameBlock;
  INT32                       Result;

  if ((FileName == NULL) || (NewFileName == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  RenameBlock.FileName          = FileName;
  RenameBlock.FileNameLength    = AsciiStrLen (FileName);
  RenameBlock.NewFileName       = NewFileName;
  RenameBlock.NewFileNameLength = AsciiStrLen (NewFileName);

  Result = Semihost_SYS_RENAME (&RenameBlock);

  if (Result != 0) {
    return  RETURN_ABORTED;
  } else {
    return  RETURN_SUCCESS;
  }
}

CHAR8
SemihostReadCharacter (
  VOID
  )
{
  return Semihost_SYS_READC();
}

VOID
SemihostWriteCharacter (
  IN CHAR8 Character
  )
{
  Semihost_SYS_WRITEC(&Character);
}

VOID
SemihostWriteString (
  IN CHAR8 *String
  )
{
  Semihost_SYS_WRITE0(String);
}

UINT32
SemihostSystem (
  IN CHAR8 *CommandLine
  )
{
  SEMIHOST_SYSTEM_BLOCK SystemBlock;

  SystemBlock.CommandLine   = CommandLine;
  SystemBlock.CommandLength = AsciiStrLen(CommandLine);

  return Semihost_SYS_SYSTEM(&SystemBlock);
}
