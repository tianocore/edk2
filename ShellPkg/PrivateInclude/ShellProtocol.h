/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_SHELL_INTERNALS_SHELL_PROTOCOL__
#define __UEFI_SHELL_INTERNALS_SHELL_PROTOCOL__

#include <Uefi.h>
#include <Protocol/DevicePath.h>

/**
  Worker function to open a file based on a device path.  this will open the root
  of the volume and then traverse down to the file itself.

  @param DevicePath2                 Device Path of the file
  @param FileHandle               Pointer to the file upon a successful return
  @param OpenMode                 mode to open file in.
  @param Attributes               the File Attributes to use when creating a new file

  @retval EFI_SUCCESS             the file is open and FileHandle is valid
  @retval EFI_UNSUPPORTED         the device path contained non-path elements
  @retval other                   an error occurred.
**/
EFI_STATUS
InternalOpenFileDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  *DevicePath2,
  OUT SHELL_FILE_HANDLE            *FileHandle,
  IN UINT64                        OpenMode,
  IN UINT64                        Attributes OPTIONAL
  );

/**
  Opens a file or a directory by file name.

  This function opens the specified file in the specified OpenMode and returns a file
  handle.
  If the file name begins with >v, then the file handle which is returned refers to the
  shell environment variable with the specified name. If the shell environment variable
  exists, is non-volatile and the OpenMode indicates EFI_FILE_MODE_WRITE, then
  EFI_INVALID_PARAMETER is returned.

  If the file name is >i, then the file handle which is returned refers to the standard
  input. If the OpenMode indicates EFI_FILE_MODE_WRITE, then EFI_INVALID_PARAMETER
  is returned.

  If the file name is >o, then the file handle which is returned refers to the standard
  output. If the OpenMode indicates EFI_FILE_MODE_READ, then EFI_INVALID_PARAMETER
  is returned.

  If the file name is >e, then the file handle which is returned refers to the standard
  error. If the OpenMode indicates EFI_FILE_MODE_READ, then EFI_INVALID_PARAMETER
  is returned.

  If the file name is NUL, then the file handle that is returned refers to the standard NUL
  file. If the OpenMode indicates EFI_FILE_MODE_READ, then EFI_INVALID_PARAMETER is
  returned.

  If return EFI_SUCCESS, the FileHandle is the opened file's handle, else, the
  FileHandle is NULL.

  @param FileName               Points to the NULL-terminated UCS-2 encoded file name.
  @param FileHandle             On return, points to the file handle.
  @param OpenMode               File open mode. Either EFI_FILE_MODE_READ or
                                EFI_FILE_MODE_WRITE from section 12.4 of the UEFI
                                Specification.
  @retval EFI_SUCCESS           The file was opened. FileHandle has the opened file's handle.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value. FileHandle is NULL.
  @retval EFI_UNSUPPORTED       Could not open the file path. FileHandle is NULL.
  @retval EFI_NOT_FOUND         The specified file could not be found on the device or the file
                                system could not be found on the device. FileHandle is NULL.
  @retval EFI_NO_MEDIA          The device has no medium. FileHandle is NULL.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the medium is no
                                longer supported. FileHandle is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error or can't get the file path according
                                the FileName. FileHandle is NULL.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted. FileHandle is NULL.
  @retval EFI_WRITE_PROTECTED   An attempt was made to create a file, or open a file for write
                                when the media is write-protected. FileHandle is NULL.
  @retval EFI_ACCESS_DENIED     The service denied access to the file. FileHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the file. FileHandle
                                is NULL.
  @retval EFI_VOLUME_FULL       The volume is full. FileHandle is NULL.
**/
EFI_STATUS
EFIAPI
EfiShellOpenFileByName (
  IN CONST CHAR16        *FileName,
  OUT SHELL_FILE_HANDLE  *FileHandle,
  IN UINT64              OpenMode
  );

/**
  Disables the page break output mode.
**/
VOID
EFIAPI
EfiShellDisablePageBreak (
  VOID
  );

/**
  Enables the page break output mode.
**/
VOID
EFIAPI
EfiShellEnablePageBreak (
  VOID
  );

/**
  internal worker function to run a command via Device Path

  @param ParentImageHandle      A handle of the image that is executing the specified
                                command line.
  @param DevicePath             device path of the file to execute
  @param CommandLine            Points to the NULL-terminated UCS-2 encoded string
                                containing the command line. If NULL then the command-
                                line will be empty.
  @param Environment            Points to a NULL-terminated array of environment
                                variables with the format 'x=y', where x is the
                                environment variable name and y is the value. If this
                                is NULL, then the current shell environment is used.
  @param[out] StartImageStatus  Returned status from gBS->StartImage.

  @retval EFI_SUCCESS       The command executed successfully. The  status code
                            returned by the command is pointed to by StatusCode.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES Out of resources.
  @retval EFI_UNSUPPORTED   Nested shell invocations are not allowed.
**/
EFI_STATUS
InternalShellExecuteDevicePath (
  IN CONST EFI_HANDLE                *ParentImageHandle,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CONST CHAR16                    *CommandLine OPTIONAL,
  IN CONST CHAR16                    **Environment OPTIONAL,
  OUT EFI_STATUS                     *StartImageStatus OPTIONAL
  );

/**
  Gets the environment variable.

  This function returns the current value of the specified environment variable.

  @param Name                   A pointer to the environment variable name

  @retval !=NULL                The environment variable's value. The returned
                                pointer does not need to be freed by the caller.
  @retval NULL                  The environment variable doesn't exist.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetEnv (
  IN CONST CHAR16  *Name
  );

/**
  Returns the current directory on the specified device.

  If FileSystemMapping is NULL, it returns the current working directory. If the
  FileSystemMapping is not NULL, it returns the current directory associated with the
  FileSystemMapping. In both cases, the returned name includes the file system
  mapping (i.e. fs0:\current-dir).

  @param FileSystemMapping      A pointer to the file system mapping. If NULL,
                                then the current working directory is returned.

  @retval !=NULL                The current directory.
  @retval NULL                  Current directory does not exist.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetCurDir (
  IN CONST CHAR16  *FileSystemMapping OPTIONAL
  );

/**
  Gets the enable status of the page break output mode.

  User can use this function to determine current page break mode.

  @retval TRUE                  The page break output mode is enabled
  @retval FALSE                 The page break output mode is disabled
**/
BOOLEAN
EFIAPI
EfiShellGetPageBreak (
  VOID
  );

/**
  Internal variable setting function.  Allows for setting of the read only variables.

  @param Name                   Points to the NULL-terminated environment variable name.
  @param Value                  Points to the NULL-terminated environment variable value. If the value is an
                                empty string then the environment variable is deleted.
  @param Volatile               Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           The environment variable was successfully updated.
**/
EFI_STATUS
InternalEfiShellSetEnv (
  IN CONST CHAR16  *Name,
  IN CONST CHAR16  *Value,
  IN BOOLEAN       Volatile
  );

#endif //__UEFI_SHELL_INTERNALS_SHELL_PROTOCOL__
