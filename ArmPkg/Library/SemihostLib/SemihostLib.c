/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

  if (Result == 0) {
    return RETURN_SUCCESS;
  } else {
    return RETURN_ABORTED;
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

  if (Result == *Length) {
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

RETURN_STATUS
SemihostFileRemove (
  IN CHAR8 *FileName
  )
{
  SEMIHOST_FILE_REMOVE_BLOCK  RemoveBlock;
  UINT32                      Result;

  RemoveBlock.FileName    = FileName;
  RemoveBlock.NameLength  = AsciiStrLen(FileName);

  Result = Semihost_SYS_REMOVE(&RemoveBlock);

  if (Result == 0) {
    return RETURN_SUCCESS;
  } else {
    return RETURN_ABORTED;
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
