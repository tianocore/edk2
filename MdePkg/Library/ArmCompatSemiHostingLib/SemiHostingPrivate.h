/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SEMIHOSTING_PRIVATE_H_
#define SEMIHOSTING_PRIVATE_H_

typedef struct {
  CHAR8    *FileName;
  UINTN    Mode;
  UINTN    NameLength;
} SEMIHOSTING_FILE_OPEN_BLOCK;

typedef struct {
  UINTN    Handle;
  VOID     *Buffer;
  UINTN    Length;
} SEMIHOSTING_FILE_READ_WRITE_BLOCK;

typedef struct {
  UINTN    Handle;
  UINTN    Location;
} SEMIHOSTING_FILE_SEEK_BLOCK;

typedef struct {
  VOID     *Buffer;
  UINTN    Identifier;
  UINTN    Length;
} SEMIHOSTING_FILE_TMPNAME_BLOCK;

typedef struct {
  CHAR8    *FileName;
  UINTN    NameLength;
} SEMIHOSTING_FILE_REMOVE_BLOCK;

typedef struct {
  CHAR8    *FileName;
  UINTN    FileNameLength;
  CHAR8    *NewFileName;
  UINTN    NewFileNameLength;
} SEMIHOSTING_FILE_RENAME_BLOCK;

UINT32
EFIAPI
SemiHostingCall (
  IN UINT32  Operation,
  IN UINTN   SystemBlockAddress
  );

UINT32
EFIAPI
SemiHostingConnectionEnabled (
  VOID
  );

#define SEMIHOSTING_SYS_OPEN(OpenBlock)        SemiHostingCall(0x01, (UINTN)(OpenBlock))
#define SEMIHOSTING_SYS_CLOSE(Handle)          SemiHostingCall(0x02, (UINTN)(Handle))
#define SEMIHOSTING_SYS_WRITE0(String)         SemiHostingCall(0x04, (UINTN)(String))
#define SEMIHOSTING_SYS_WRITEC(Character)      SemiHostingCall(0x03, (UINTN)(Character))
#define SEMIHOSTING_SYS_WRITE(WriteBlock)      SemiHostingCall(0x05, (UINTN)(WriteBlock))
#define SEMIHOSTING_SYS_READ(ReadBlock)        SemiHostingCall(0x06, (UINTN)(ReadBlock))
#define SEMIHOSTING_SYS_READC()                SemiHostingCall(0x07, (UINTN)(0))
#define SEMIHOSTING_SYS_SEEK(SeekBlock)        SemiHostingCall(0x0A, (UINTN)(SeekBlock))
#define SEMIHOSTING_SYS_FLEN(Handle)           SemiHostingCall(0x0C, (UINTN)(Handle))
#define SEMIHOSTING_SYS_TMPNAME(TmpNameBlock)  SemiHostingCall(0x0D, (UINTN)(TmpNameBlock))
#define SEMIHOSTING_SYS_REMOVE(RemoveBlock)    SemiHostingCall(0x0E, (UINTN)(RemoveBlock))
#define SEMIHOSTING_SYS_RENAME(RenameBlock)    SemiHostingCall(0x0F, (UINTN)(RenameBlock))
#define SEMIHOSTING_SYS_SYSTEM(SystemBlock)    SemiHostingCall(0x12, (UINTN)(SystemBlock))

#endif // SEMIHOSTING_PRIVATE_H_
