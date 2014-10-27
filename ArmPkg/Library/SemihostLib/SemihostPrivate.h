/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SEMIHOST_PRIVATE_H__
#define __SEMIHOST_PRIVATE_H__

typedef struct {
  CHAR8   *FileName;
  UINTN    Mode;
  UINTN    NameLength;
} SEMIHOST_FILE_OPEN_BLOCK;

typedef struct {
  UINTN    Handle;
  VOID    *Buffer;
  UINTN    Length;
} SEMIHOST_FILE_READ_WRITE_BLOCK;

typedef struct {
  UINTN    Handle;
  UINTN    Location;
} SEMIHOST_FILE_SEEK_BLOCK;

typedef struct {
  VOID    *Buffer;
  UINTN    Identifier;
  UINTN    Length;
} SEMIHOST_FILE_TMPNAME_BLOCK;

typedef struct {
  CHAR8   *FileName;
  UINTN    NameLength;
} SEMIHOST_FILE_REMOVE_BLOCK;

typedef struct {
  CHAR8   *FileName;
  UINTN    FileNameLength;
  CHAR8   *NewFileName;
  UINTN    NewFileNameLength;
} SEMIHOST_FILE_RENAME_BLOCK;

typedef struct {
  CHAR8   *CommandLine;
  UINTN    CommandLength;
} SEMIHOST_SYSTEM_BLOCK;

#if defined(__CC_ARM)

#if defined(__thumb__)
#define SWI 0xAB
#else
#define SWI 0x123456
#endif

#define SEMIHOST_SUPPORTED  TRUE

__swi(SWI)
INT32
_Semihost_SYS_OPEN(
  IN UINTN                    SWI_0x01,
  IN SEMIHOST_FILE_OPEN_BLOCK *OpenBlock
  );

__swi(SWI)
INT32
_Semihost_SYS_CLOSE(
  IN UINTN  SWI_0x02,
  IN UINT32 *Handle
  );

__swi(SWI)
VOID
_Semihost_SYS_WRITEC(
  IN UINTN    SWI_0x03,
  IN CHAR8    *Character
  );

__swi(SWI)
VOID
_Semihost_SYS_WRITE0(
  IN UINTN SWI_0x04,
  IN CHAR8 *String
  );

__swi(SWI)
UINT32
_Semihost_SYS_WRITE(
  IN     UINTN                          SWI_0x05,
  IN OUT SEMIHOST_FILE_READ_WRITE_BLOCK *WriteBlock
  );

__swi(SWI)
UINT32
_Semihost_SYS_READ(
  IN     UINTN                          SWI_0x06,
  IN OUT SEMIHOST_FILE_READ_WRITE_BLOCK *ReadBlock
  );

__swi(SWI)
CHAR8
_Semihost_SYS_READC(
  IN     UINTN SWI_0x07,
  IN     UINTN Zero
  );

__swi(SWI)
INT32
_Semihost_SYS_SEEK(
  IN UINTN                    SWI_0x0A,
  IN SEMIHOST_FILE_SEEK_BLOCK *SeekBlock
  );

__swi(SWI)
INT32
_Semihost_SYS_FLEN(
  IN UINTN  SWI_0x0C,
  IN UINT32 *Handle
  );

__swi(SWI)
UINT32
_Semihost_SYS_TMPNAME(
  IN UINTN                       SWI_0x0D,
  IN SEMIHOST_FILE_TMPNAME_BLOCK *TmpNameBlock
  );

__swi(SWI)
UINT32
_Semihost_SYS_REMOVE(
  IN UINTN                      SWI_0x0E,
  IN SEMIHOST_FILE_REMOVE_BLOCK *RemoveBlock
  );

__swi(SWI)
UINT32
_Semihost_SYS_RENAME(
  IN UINTN                      SWI_0x0F,
  IN SEMIHOST_FILE_RENAME_BLOCK *RenameBlock
  );

__swi(SWI)
UINT32
_Semihost_SYS_SYSTEM(
  IN UINTN                 SWI_0x12,
  IN SEMIHOST_SYSTEM_BLOCK *SystemBlock
  );

#define Semihost_SYS_OPEN(OpenBlock)        _Semihost_SYS_OPEN(0x01, OpenBlock)
#define Semihost_SYS_CLOSE(Handle)          _Semihost_SYS_CLOSE(0x02, Handle)
#define Semihost_SYS_WRITE0(String)         _Semihost_SYS_WRITE0(0x04, String)
#define Semihost_SYS_WRITEC(Character)      _Semihost_SYS_WRITEC(0x03, Character)
#define Semihost_SYS_WRITE(WriteBlock)      _Semihost_SYS_WRITE(0x05, WriteBlock)
#define Semihost_SYS_READ(ReadBlock)        _Semihost_SYS_READ(0x06, ReadBlock)
#define Semihost_SYS_READC()                _Semihost_SYS_READC(0x07, 0)
#define Semihost_SYS_SEEK(SeekBlock)        _Semihost_SYS_SEEK(0x0A, SeekBlock)
#define Semihost_SYS_FLEN(Handle)           _Semihost_SYS_FLEN(0x0C, Handle)
#define Semihost_SYS_TMPNAME(TmpNameBlock)  _Semihost_SYS_TMPNAME(0x0D, TmpNameBlock)
#define Semihost_SYS_REMOVE(RemoveBlock)    _Semihost_SYS_REMOVE(0x0E, RemoveBlock)
#define Semihost_SYS_RENAME(RenameBlock)    _Semihost_SYS_RENAME(0x0F, RenameBlock)
#define Semihost_SYS_SYSTEM(SystemBlock)    _Semihost_SYS_SYSTEM(0x12, SystemBlock)

#elif defined(__GNUC__) // __CC_ARM

#define SEMIHOST_SUPPORTED  TRUE

UINT32
GccSemihostCall (
  IN UINT32   Operation,
  IN UINTN    SystemBlockAddress
  ); // __attribute__ ((interrupt ("SVC")));

#define Semihost_SYS_OPEN(OpenBlock)        GccSemihostCall(0x01, (UINTN)(OpenBlock))
#define Semihost_SYS_CLOSE(Handle)          GccSemihostCall(0x02, (UINTN)(Handle))
#define Semihost_SYS_WRITE0(String)         GccSemihostCall(0x04, (UINTN)(String))
#define Semihost_SYS_WRITEC(Character)      GccSemihostCall(0x03, (UINTN)(Character))
#define Semihost_SYS_WRITE(WriteBlock)      GccSemihostCall(0x05, (UINTN)(WriteBlock))
#define Semihost_SYS_READ(ReadBlock)        GccSemihostCall(0x06, (UINTN)(ReadBlock))
#define Semihost_SYS_READC()                GccSemihostCall(0x07, (UINTN)(0))
#define Semihost_SYS_SEEK(SeekBlock)        GccSemihostCall(0x0A, (UINTN)(SeekBlock))
#define Semihost_SYS_FLEN(Handle)           GccSemihostCall(0x0C, (UINTN)(Handle))
#define Semihost_SYS_TMPNAME(TmpNameBlock)  GccSemihostCall(0x0D, (UINTN)(TmpNameBlock))
#define Semihost_SYS_REMOVE(RemoveBlock)    GccSemihostCall(0x0E, (UINTN)(RemoveBlock))
#define Semihost_SYS_RENAME(RenameBlock)    GccSemihostCall(0x0F, (UINTN)(RenameBlock))
#define Semihost_SYS_SYSTEM(SystemBlock)    GccSemihostCall(0x12, (UINTN)(SystemBlock))

#else // __CC_ARM

#define SEMIHOST_SUPPORTED  FALSE

#define Semihost_SYS_OPEN(OpenBlock)        (-1)
#define Semihost_SYS_CLOSE(Handle)          (-1)
#define Semihost_SYS_WRITE0(String)
#define Semihost_SYS_WRITEC(Character)
#define Semihost_SYS_WRITE(WriteBlock)      (0)
#define Semihost_SYS_READ(ReadBlock)        ((ReadBlock)->Length)
#define Semihost_SYS_READC()                ('x')
#define Semihost_SYS_SEEK(SeekBlock)        (-1)
#define Semihost_SYS_FLEN(Handle)           (-1)
#define Semihost_SYS_TMPNAME(TmpNameBlock)  (-1)
#define Semihost_SYS_REMOVE(RemoveBlock)    (-1)
#define Semihost_SYS_RENAME(RenameBlock)    (-1)
#define Semihost_SYS_SYSTEM(SystemBlock)    (-1)

#endif // __CC_ARM

#endif //__SEMIHOST_PRIVATE_H__
