/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SEMIHOST_PRIVATE_H_
#define SEMIHOST_PRIVATE_H_

typedef struct {
  CHAR8    *FileName;
  UINTN    Mode;
  UINTN    NameLength;
} SEMIHOST_FILE_OPEN_BLOCK;

typedef struct {
  UINTN    Handle;
  VOID     *Buffer;
  UINTN    Length;
} SEMIHOST_FILE_READ_WRITE_BLOCK;

typedef struct {
  UINTN    Handle;
  UINTN    Location;
} SEMIHOST_FILE_SEEK_BLOCK;

typedef struct {
  VOID     *Buffer;
  UINTN    Identifier;
  UINTN    Length;
} SEMIHOST_FILE_TMPNAME_BLOCK;

typedef struct {
  CHAR8    *FileName;
  UINTN    NameLength;
} SEMIHOST_FILE_REMOVE_BLOCK;

typedef struct {
  CHAR8    *FileName;
  UINTN    FileNameLength;
  CHAR8    *NewFileName;
  UINTN    NewFileNameLength;
} SEMIHOST_FILE_RENAME_BLOCK;

typedef struct {
  CHAR8    *CommandLine;
  UINTN    CommandLength;
} SEMIHOST_SYSTEM_BLOCK;

#endif // SEMIHOST_PRIVATE_H_
