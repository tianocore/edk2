/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SEMIHOSTING_H__
#define __SEMIHOSTING_H__

/*
 *
 *  Please refer to ARM RVDS 3.0 Compiler and Libraries Guide for more information
 *  about the semihosting interface.
 *
 */
 
#define SEMIHOST_FILE_MODE_READ     (0 << 2)
#define SEMIHOST_FILE_MODE_WRITE    (1 << 2)
#define SEMIHOST_FILE_MODE_APPEND   (2 << 2)
#define SEMIHOST_FILE_MODE_CREATE   (1 << 1)
#define SEMIHOST_FILE_MODE_BINARY   (1 << 0)
#define SEMIHOST_FILE_MODE_ASCII    (0 << 0)

BOOLEAN
SemihostConnectionSupported (
  VOID
  );

RETURN_STATUS
SemihostFileOpen (
  IN  CHAR8  *FileName,
  IN  UINT32 Mode,
  OUT UINT32 *FileHandle
  );

RETURN_STATUS
SemihostFileSeek (
  IN UINT32 FileHandle,
  IN UINT32 Offset
  );

RETURN_STATUS
SemihostFileRead (
  IN     UINT32 FileHandle,
  IN OUT UINT32 *Length,
  OUT    VOID   *Buffer
  );

RETURN_STATUS
SemihostFileWrite (
  IN     UINT32 FileHandle,
  IN OUT UINT32 *Length,
  IN     VOID   *Buffer
  );

RETURN_STATUS
SemihostFileClose (
  IN UINT32 FileHandle
  );

RETURN_STATUS
SemihostFileLength (
  IN  UINT32 FileHandle,
  OUT UINT32 *Length
  );

RETURN_STATUS
SemihostFileRemove (
  IN CHAR8 *FileName
  );

CHAR8
SemihostReadCharacter (
  VOID
  );

VOID
SemihostWriteCharacter (
  IN CHAR8 Character
  );

VOID
SemihostWriteString (
  IN CHAR8 *String
  );
  
UINT32
SemihostSystem (
  IN CHAR8 *CommandLine
  );
  
#endif // __SEMIHOSTING_H__
