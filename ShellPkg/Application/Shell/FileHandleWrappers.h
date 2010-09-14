/** @file
  EFI_FILE_PROTOCOL wrappers for other items (Like Environment Variables, StdIn, StdOut, StdErr, etc...)

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SHELL_FILE_HANDLE_WRAPPERS_HEADER_
#define _SHELL_FILE_HANDLE_WRAPPERS_HEADER_

typedef struct {
  LIST_ENTRY        Link;
  CHAR16*           Buffer;
} SHELL_LINE_LIST;

typedef struct {
  UINTN             LogCount;
  SHELL_LINE_LIST   *Log;
} SHELL_LINE_LOG;

///
/// FILE sytle interfaces for StdIn.
///
extern EFI_FILE_PROTOCOL FileInterfaceStdIn;

///
/// FILE sytle interfaces for StdOut.
///
extern EFI_FILE_PROTOCOL FileInterfaceStdOut;

///
/// FILE sytle interfaces for StdErr.
///
extern EFI_FILE_PROTOCOL FileInterfaceStdErr;

///
/// FILE style interface for NUL file.
///
extern EFI_FILE_PROTOCOL FileInterfaceNulFile;

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  environment variables through file operations.

  @param EnvName    The name of the Environment Variable to be operated on.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL*
EFIAPI
CreateFileInterfaceEnv(
  CONST CHAR16 *EnvName
  );

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  a file entirely in memory through file operations.

  @param[in] Unicode  TRUE if the data is UNICODE, FALSE otherwise.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL*
EFIAPI
CreateFileInterfaceMem(
  IN CONST BOOLEAN Unicode
  );

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  a file entirely with unicode awareness through file operations.

  @param[in] Template The pointer to the handle to start with.
  @param[in] Unicode  TRUE if the data is UNICODE, FALSE otherwise.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL*
CreateFileInterfaceFile(
  IN CONST EFI_FILE_PROTOCOL  *Template,
  IN CONST BOOLEAN            Unicode
  );

#endif //_SHELL_FILE_HANDLE_WRAPPERS_HEADER_

