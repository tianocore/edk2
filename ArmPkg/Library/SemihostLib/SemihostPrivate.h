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

#if defined (__GNUC__)

#define SEMIHOST_SUPPORTED  TRUE

UINT32
GccSemihostCall (
  IN UINT32  Operation,
  IN UINTN   SystemBlockAddress
  ); // __attribute__ ((interrupt ("SVC")));

#define SEMIHOST_SYS_OPEN(OpenBlock)        GccSemihostCall(0x01, (UINTN)(OpenBlock))
#define SEMIHOST_SYS_CLOSE(Handle)          GccSemihostCall(0x02, (UINTN)(Handle))
#define SEMIHOST_SYS_WRITE0(String)         GccSemihostCall(0x04, (UINTN)(String))
#define SEMIHOST_SYS_WRITEC(Character)      GccSemihostCall(0x03, (UINTN)(Character))
#define SEMIHOST_SYS_WRITE(WriteBlock)      GccSemihostCall(0x05, (UINTN)(WriteBlock))
#define SEMIHOST_SYS_READ(ReadBlock)        GccSemihostCall(0x06, (UINTN)(ReadBlock))
#define SEMIHOST_SYS_READC()                GccSemihostCall(0x07, (UINTN)(0))
#define SEMIHOST_SYS_SEEK(SeekBlock)        GccSemihostCall(0x0A, (UINTN)(SeekBlock))
#define SEMIHOST_SYS_FLEN(Handle)           GccSemihostCall(0x0C, (UINTN)(Handle))
#define SEMIHOST_SYS_TMPNAME(TmpNameBlock)  GccSemihostCall(0x0D, (UINTN)(TmpNameBlock))
#define SEMIHOST_SYS_REMOVE(RemoveBlock)    GccSemihostCall(0x0E, (UINTN)(RemoveBlock))
#define SEMIHOST_SYS_RENAME(RenameBlock)    GccSemihostCall(0x0F, (UINTN)(RenameBlock))
#define SEMIHOST_SYS_SYSTEM(SystemBlock)    GccSemihostCall(0x12, (UINTN)(SystemBlock))

#else // __GNUC__

#define SEMIHOST_SUPPORTED  FALSE

#define SEMIHOST_SYS_OPEN(OpenBlock)  (-1)
#define SEMIHOST_SYS_CLOSE(Handle)    (-1)
#define SEMIHOST_SYS_WRITE0(String)
#define SEMIHOST_SYS_WRITEC(Character)
#define SEMIHOST_SYS_WRITE(WriteBlock)      (0)
#define SEMIHOST_SYS_READ(ReadBlock)        ((ReadBlock)->Length)
#define SEMIHOST_SYS_READC()                ('x')
#define SEMIHOST_SYS_SEEK(SeekBlock)        (-1)
#define SEMIHOST_SYS_FLEN(Handle)           (-1)
#define SEMIHOST_SYS_TMPNAME(TmpNameBlock)  (-1)
#define SEMIHOST_SYS_REMOVE(RemoveBlock)    (-1)
#define SEMIHOST_SYS_RENAME(RenameBlock)    (-1)
#define SEMIHOST_SYS_SYSTEM(SystemBlock)    (-1)

#endif // __GNUC__

#endif // SEMIHOST_PRIVATE_H_
