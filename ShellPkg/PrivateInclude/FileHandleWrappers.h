/** @file
  EFI_FILE_PROTOCOL wrappers for other items (Like Environment Variables, StdIn, StdOut, StdErr, etc...)

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_SHELL_INTERNALS_FILE_HANDLE_WRAPPERS__
#define __UEFI_SHELL_INTERNALS_FILE_HANDLE_WRAPPERS__

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

///
/// FILE styte interfaces for StdIn.
///
extern EFI_FILE_PROTOCOL  FileInterfaceStdIn;

///
/// FILE styte interfaces for StdOut.
///
extern EFI_FILE_PROTOCOL  FileInterfaceStdOut;

///
/// FILE styte interfaces for StdErr.
///
extern EFI_FILE_PROTOCOL  FileInterfaceStdErr;

///
/// FILE style interface for NUL file.
///
extern EFI_FILE_PROTOCOL  FileInterfaceNulFile;

/**
  File style interface for console (Open).

  @param[in] This       Ignored.
  @param[out] NewHandle Ignored.
  @param[in] FileName   Ignored.
  @param[in] OpenMode   Ignored.
  @param[in] Attributes Ignored.

  @retval EFI_NOT_FOUND
**/
EFI_STATUS
EFIAPI
FileInterfaceOpenNotFound (
  IN EFI_FILE_PROTOCOL   *This,
  OUT EFI_FILE_PROTOCOL  **NewHandle,
  IN CHAR16              *FileName,
  IN UINT64              OpenMode,
  IN UINT64              Attributes
  );

/**
  File style interface for console (Close, Delete, & Flush)

  @param[in] This       Ignored.

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
FileInterfaceNopGeneric (
  IN EFI_FILE_PROTOCOL  *This
  );

/**
  File style interface for console (GetPosition).

  @param[in] This       Ignored.
  @param[out] Position  Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopGetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  OUT UINT64            *Position
  );

/**
  File style interface for console (SetPosition).

  @param[in] This       Ignored.
  @param[in] Position   Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopSetPosition (
  IN EFI_FILE_PROTOCOL  *This,
  IN UINT64             Position
  );

/**
  File style interface for console (GetInfo).

  @param[in] This              Ignored.
  @param[in] InformationType   Ignored.
  @param[in, out] BufferSize   Ignored.
  @param[out] Buffer           Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopGetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN OUT UINTN          *BufferSize,
  OUT VOID              *Buffer
  );

/**
  File style interface for console (SetInfo).

  @param[in] This       Ignored.
  @param[in] InformationType   Ignored.
  @param[in] BufferSize Ignored.
  @param[in] Buffer     Ignored.

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
FileInterfaceNopSetInfo (
  IN EFI_FILE_PROTOCOL  *This,
  IN EFI_GUID           *InformationType,
  IN UINTN              BufferSize,
  IN VOID               *Buffer
  );

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  environment variables through file operations.

  @param EnvName    The name of the Environment Variable to be operated on.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceEnv (
  CONST CHAR16  *EnvName
  );

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  a file entirely in memory through file operations.

  @param[in] Unicode  TRUE if the data is UNICODE, FALSE otherwise.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceMem (
  IN CONST BOOLEAN  Unicode
  );

/**
  Creates a EFI_FILE_PROTOCOL (almost) object for using to access
  a file entirely with unicode awareness through file operations.

  @param[in] Template The pointer to the handle to start with.
  @param[in] Unicode  TRUE if the data is UNICODE, FALSE otherwise.

  @retval NULL      Memory could not be allocated.
  @return other     a pointer to an EFI_FILE_PROTOCOL structure
**/
EFI_FILE_PROTOCOL *
CreateFileInterfaceFile (
  IN CONST EFI_FILE_PROTOCOL  *Template,
  IN CONST BOOLEAN            Unicode
  );

#endif //__UEFI_SHELL_INTERNALS_FILE_HANDLE_WRAPPERS__
