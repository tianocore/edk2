/** @file

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  CommonLib.h

Abstract:

  Common library assistance routines.

**/

#ifndef _EFI_COMMON_LIB_H
#define _EFI_COMMON_LIB_H

#include <Common/UefiBaseTypes.h>
#include <Common/BuildVersion.h>
#define PRINTED_GUID_BUFFER_SIZE  37  // including null-termination
//
// Function declarations
//
VOID
PeiZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
;

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
;

VOID
ZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
;

VOID
CopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
;

INTN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  )
;

EFI_STATUS
GetFileImage (
  IN CHAR8    *InputFileName,
  OUT CHAR8   **InputFileImage,
  OUT UINT32  *BytesRead
  )
;

EFI_STATUS
PutFileImage (
  IN CHAR8    *OutputFileName,
  IN CHAR8    *OutputFileImage,
  IN UINT32   BytesToWrite
  )
;
/*++

Routine Description:

  This function opens a file and writes OutputFileImage into the file.

Arguments:

  OutputFileName     The name of the file to write.
  OutputFileImage    A pointer to the memory buffer.
  BytesToWrite       The size of the memory buffer.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     No resource to complete operations.

**/

UINT8
CalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
;

UINT8
CalculateSum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
;

UINT16
CalculateChecksum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
;

UINT16
CalculateSum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
;

EFI_STATUS
PrintGuid (
  IN EFI_GUID                     *Guid
  )
;

#define PRINTED_GUID_BUFFER_SIZE  37  // including null-termination
EFI_STATUS
PrintGuidToBuffer (
  IN EFI_GUID     *Guid,
  IN OUT UINT8    *Buffer,
  IN UINT32       BufferLen,
  IN BOOLEAN      Uppercase
  )
;

#define ASSERT(x) assert(x)

#ifdef __GNUC__
#include <stdio.h>
#include <sys/stat.h>
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define strcmpi strcasecmp
size_t _filelength(int fd);
#ifndef __CYGWIN__
char *strlwr(char *s);
#endif
#endif

//
// On windows, mkdir only has one parameter.
// On unix, it has two parameters
//
#if defined(__GNUC__)
#define mkdir(dir, perm) mkdir(dir, perm)
#else
#define mkdir(dir, perm) mkdir(dir)
#endif

#endif
