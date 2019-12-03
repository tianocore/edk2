/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_PROTOCOL_HEADER_
#define _SHELL_PROTOCOL_HEADER_

#include "Shell.h"

typedef struct {
  LIST_ENTRY                Link;
  EFI_SHELL_PROTOCOL        *Interface;
  EFI_HANDLE                Handle;
} SHELL_PROTOCOL_HANDLE_LIST;

// flags values...
#define SHELL_MAP_FLAGS_CONSIST BIT1

/**
  Function to create and install on the current handle.

  Will overwrite any existing ShellProtocols in the system to be sure that
  the current shell is in control.

  This must be removed via calling CleanUpShellProtocol().

  @param[in, out] NewShell   The pointer to the pointer to the structure
  to install.

  @retval EFI_SUCCESS     The operation was successful.
  @return                 An error from LocateHandle, CreateEvent, or other core function.
**/
EFI_STATUS
CreatePopulateInstallShellProtocol (
  IN OUT EFI_SHELL_PROTOCOL  **NewShell
  );

/**
  Opposite of CreatePopulateInstallShellProtocol.

  Free all memory and restore the system to the state it was in before calling
  CreatePopulateInstallShellProtocol.

  @param[in, out] NewShell   The pointer to the new shell protocol structure.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpShellProtocol (
  IN OUT EFI_SHELL_PROTOCOL  *NewShell
  );

/**
  Cleanup the shell environment.

  @param[in, out] NewShell   The pointer to the new shell protocol structure.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpShellEnvironment (
  IN OUT EFI_SHELL_PROTOCOL  *NewShell
  );

/**
  This function creates a mapping for a device path.

  @param DevicePath             Points to the device path. If this is NULL and Mapping points to a valid mapping,
                                then the mapping will be deleted.
  @param Mapping                Points to the NULL-terminated mapping for the device path.  Must end with a ':'

  @retval EFI_SUCCESS           Mapping created or deleted successfully.
  @retval EFI_NO_MAPPING        There is no handle that corresponds exactly to DevicePath. See the
                                boot service function LocateDevicePath().
  @retval EFI_ACCESS_DENIED     The mapping is a built-in alias.
  @retval EFI_INVALID_PARAMETER Mapping was NULL
  @retval EFI_INVALID_PARAMETER Mapping did not end with a ':'
  @retval EFI_INVALID_PARAMETER DevicePath was not pointing at a device that had a SIMPLE_FILE_SYSTEM_PROTOCOL installed.
  @retval EFI_NOT_FOUND         There was no mapping found to delete
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed
**/
EFI_STATUS
EFIAPI
EfiShellSetMap(
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath OPTIONAL,
  IN CONST CHAR16 *Mapping
  );

/**
  Gets the device path from the mapping.

  This function gets the device path associated with a mapping.

  @param Mapping                A pointer to the mapping

  @retval !=NULL                Pointer to the device path that corresponds to the
                                device mapping. The returned pointer does not need
                                to be freed.
  @retval NULL                  There is no device path associated with the
                                specified mapping.
**/
CONST EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiShellGetDevicePathFromMap(
  IN CONST CHAR16 *Mapping
  );

/**
  Gets the mapping that most closely matches the device path.

  This function gets the mapping which corresponds to the device path *DevicePath. If
  there is no exact match, then the mapping which most closely matches *DevicePath
  is returned, and *DevicePath is updated to point to the remaining portion of the
  device path. If there is an exact match, the mapping is returned and *DevicePath
  points to the end-of-device-path node.

  @param DevicePath             On entry, points to a device path pointer. On
                                exit, updates the pointer to point to the
                                portion of the device path after the mapping.

  @retval NULL                  No mapping was found.
  @return !=NULL                Pointer to NULL-terminated mapping. The buffer
                                is callee allocated and should be freed by the caller.
**/
CONST CHAR16 *
EFIAPI
EfiShellGetMapFromDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL **DevicePath
  );

/**
  Converts a device path to a file system-style path.

  This function converts a device path to a file system path by replacing part, or all, of
  the device path with the file-system mapping. If there are more than one application
  file system mappings, the one that most closely matches Path will be used.

  @param Path                   The pointer to the device path

  @retval NULL                  the device path could not be found.
  @return all                   The pointer of the NULL-terminated file path. The path
                                is callee-allocated and should be freed by the caller.
**/
CHAR16 *
EFIAPI
EfiShellGetFilePathFromDevicePath(
  IN CONST EFI_DEVICE_PATH_PROTOCOL *Path
  );

/**
  Converts a file system style name to a device path.

  This function converts a file system style name to a device path, by replacing any
  mapping references to the associated device path.

  @param Path                   the pointer to the path

  @return all                   The pointer of the file path. The file path is callee
                                allocated and should be freed by the caller.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiShellGetDevicePathFromFilePath(
  IN CONST CHAR16 *Path
  );

/**
  Gets the name of the device specified by the device handle.

  This function gets the user-readable name of the device specified by the device
  handle. If no user-readable name could be generated, then *BestDeviceName will be
  NULL and EFI_NOT_FOUND will be returned.

  If EFI_DEVICE_NAME_USE_COMPONENT_NAME is set, then the function will return the
  device's name using the EFI_COMPONENT_NAME2_PROTOCOL, if present on
  DeviceHandle.

  If EFI_DEVICE_NAME_USE_DEVICE_PATH is set, then the function will return the
  device's name using the EFI_DEVICE_PATH_PROTOCOL, if present on DeviceHandle.
  If both EFI_DEVICE_NAME_USE_COMPONENT_NAME and
  EFI_DEVICE_NAME_USE_DEVICE_PATH are set, then
  EFI_DEVICE_NAME_USE_COMPONENT_NAME will have higher priority.

  @param DeviceHandle           The handle of the device.
  @param Flags                  Determines the possible sources of component names.
                                Valid bits are:
                                  EFI_DEVICE_NAME_USE_COMPONENT_NAME
                                  EFI_DEVICE_NAME_USE_DEVICE_PATH
  @param Language               A pointer to the language specified for the device
                                name, in the same format as described in the UEFI
                                specification, Appendix M
  @param BestDeviceName         On return, points to the callee-allocated NULL-
                                terminated name of the device. If no device name
                                could be found, points to NULL. The name must be
                                freed by the caller...

  @retval EFI_SUCCESS           Get the name successfully.
  @retval EFI_NOT_FOUND         Fail to get the device name.
  @retval EFI_INVALID_PARAMETER Flags did not have a valid bit set.
  @retval EFI_INVALID_PARAMETER BestDeviceName was NULL
  @retval EFI_INVALID_PARAMETER DeviceHandle was NULL
**/
EFI_STATUS
EFIAPI
EfiShellGetDeviceName(
  IN EFI_HANDLE DeviceHandle,
  IN EFI_SHELL_DEVICE_NAME_FLAGS Flags,
  IN CHAR8 *Language,
  OUT CHAR16 **BestDeviceName
  );

/**
  Opens the root directory of a device on a handle

  This function opens the root directory of a device and returns a file handle to it.

  @param DeviceHandle           The handle of the device that contains the volume.
  @param FileHandle             On exit, points to the file handle corresponding to the root directory on the
                                device.

  @retval EFI_SUCCESS           Root opened successfully.
  @retval EFI_NOT_FOUND         EFI_SIMPLE_FILE_SYSTEM could not be found or the root directory
                                could not be opened.
  @retval EFI_VOLUME_CORRUPTED  The data structures in the volume were corrupted.
  @retval EFI_DEVICE_ERROR      The device had an error
**/
EFI_STATUS
EFIAPI
EfiShellOpenRootByHandle(
  IN EFI_HANDLE DeviceHandle,
  OUT SHELL_FILE_HANDLE *FileHandle
  );

/**
  Opens the root directory of a device.

  This function opens the root directory of a device and returns a file handle to it.

  @param DevicePath             Points to the device path corresponding to the device where the
                                EFI_SIMPLE_FILE_SYSTEM_PROTOCOL is installed.
  @param FileHandle             On exit, points to the file handle corresponding to the root directory on the
                                device.

  @retval EFI_SUCCESS           Root opened successfully.
  @retval EFI_NOT_FOUND         EFI_SIMPLE_FILE_SYSTEM could not be found or the root directory
                                could not be opened.
  @retval EFI_VOLUME_CORRUPTED  The data structures in the volume were corrupted.
  @retval EFI_DEVICE_ERROR      The device had an error
**/
EFI_STATUS
EFIAPI
EfiShellOpenRoot(
  IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  OUT SHELL_FILE_HANDLE *FileHandle
  );

/**
  Returns whether any script files are currently being processed.

  @retval TRUE                 There is at least one script file active.
  @retval FALSE                No script files are active now.

**/
BOOLEAN
EFIAPI
EfiShellBatchIsActive (
  VOID
  );

/**
  Worker function to open a file based on a device path.  this will open the root
  of the volume and then traverse down to the file itself.

  @param DevicePath2                 Device Path of the file
  @param FileHandle               Pointer to the file upon a successful return
  @param OpenMode                 mode to open file in.
  @param Attributes               the File Attributes to use when creating a new file

  @retval EFI_SUCCESS             the file is open and FileHandle is valid
  @retval EFI_UNSUPPORTED         the device path cotained non-path elements
  @retval other                   an error ocurred.
**/
EFI_STATUS
InternalOpenFileDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL *DevicePath2,
  OUT SHELL_FILE_HANDLE             *FileHandle,
  IN UINT64                       OpenMode,
  IN UINT64                       Attributes OPTIONAL
  );

/**
  Creates a file or directory by name.

  This function creates an empty new file or directory with the specified attributes and
  returns the new file's handle. If the file already exists and is read-only, then
  EFI_INVALID_PARAMETER will be returned.

  If the file already existed, it is truncated and its attributes updated. If the file is
  created successfully, the FileHandle is the file's handle, else, the FileHandle is NULL.

  If the file name begins with >v, then the file handle which is returned refers to the
  shell environment variable with the specified name. If the shell environment variable
  already exists and is non-volatile then EFI_INVALID_PARAMETER is returned.

  @param FileName           Pointer to NULL-terminated file path
  @param FileAttribs        The new file's attrbiutes.  the different attributes are
                            described in EFI_FILE_PROTOCOL.Open().
  @param FileHandle         On return, points to the created file handle or directory's handle

  @retval EFI_SUCCESS       The file was opened.  FileHandle points to the new file's handle.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED   could not open the file path
  @retval EFI_NOT_FOUND     the specified file could not be found on the devide, or could not
                            file the file system on the device.
  @retval EFI_NO_MEDIA      the device has no medium.
  @retval EFI_MEDIA_CHANGED The device has a different medium in it or the medium is no
                            longer supported.
  @retval EFI_DEVICE_ERROR The device reported an error or can't get the file path according
                            the DirName.
  @retval EFI_VOLUME_CORRUPTED The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED An attempt was made to create a file, or open a file for write
                            when the media is write-protected.
  @retval EFI_ACCESS_DENIED The service denied access to the file.
  @retval EFI_OUT_OF_RESOURCES Not enough resources were available to open the file.
  @retval EFI_VOLUME_FULL   The volume is full.
**/
EFI_STATUS
EFIAPI
EfiShellCreateFile(
  IN CONST CHAR16     *FileName,
  IN UINT64           FileAttribs,
  OUT SHELL_FILE_HANDLE *FileHandle
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
EfiShellOpenFileByName(
  IN CONST CHAR16 *FileName,
  OUT SHELL_FILE_HANDLE *FileHandle,
  IN UINT64 OpenMode
  );

/**
  Deletes the file specified by the file name.

  This function deletes a file.

  @param FileName                 Points to the NULL-terminated file name.

  @retval EFI_SUCCESS             The file was closed and deleted, and the handle was closed.
  @retval EFI_WARN_DELETE_FAILURE The handle was closed but the file was not deleted.
  @sa EfiShellCreateFile
  @sa FileHandleDelete
**/
EFI_STATUS
EFIAPI
EfiShellDeleteFileByName(
  IN CONST CHAR16 *FileName
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
InternalShellExecuteDevicePath(
  IN CONST EFI_HANDLE               *ParentImageHandle,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN CONST CHAR16                   *CommandLine OPTIONAL,
  IN CONST CHAR16                   **Environment OPTIONAL,
  OUT EFI_STATUS                    *StartImageStatus OPTIONAL
  );

/**
  Execute the command line.

  This function creates a nested instance of the shell and executes the specified
  command (CommandLine) with the specified environment (Environment). Upon return,
  the status code returned by the specified command is placed in StatusCode.

  If Environment is NULL, then the current environment is used and all changes made
  by the commands executed will be reflected in the current environment. If the
  Environment is non-NULL, then the changes made will be discarded.

  The CommandLine is executed from the current working directory on the current
  device.

  @param ParentImageHandle      A handle of the image that is executing the specified
                                command line.
  @param CommandLine            Points to the NULL-terminated UCS-2 encoded string
                                containing the command line. If NULL then the command-
                                line will be empty.
  @param Environment            Points to a NULL-terminated array of environment
                                variables with the format 'x=y', where x is the
                                environment variable name and y is the value. If this
                                is NULL, then the current shell environment is used.
  @param StatusCode             Points to the status code returned by the command.

  @retval EFI_SUCCESS           The command executed successfully. The  status code
                                returned by the command is pointed to by StatusCode.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Out of resources.
  @retval EFI_UNSUPPORTED       Nested shell invocations are not allowed.
**/
EFI_STATUS
EFIAPI
EfiShellExecute(
  IN EFI_HANDLE *ParentImageHandle,
  IN CHAR16 *CommandLine OPTIONAL,
  IN CHAR16 **Environment OPTIONAL,
  OUT EFI_STATUS *StatusCode OPTIONAL
  );

/**
  Utility cleanup function for EFI_SHELL_FILE_INFO objects.

  1) frees all pointers (non-NULL)
  2) Closes the SHELL_FILE_HANDLE

  @param FileListNode     pointer to the list node to free
**/
VOID
FreeShellFileInfoNode(
  IN EFI_SHELL_FILE_INFO *FileListNode
  );

/**
  Frees the file list.

  This function cleans up the file list and any related data structures. It has no
  impact on the files themselves.

  @param FileList               The file list to free. Type EFI_SHELL_FILE_INFO is
                                defined in OpenFileList()

  @retval EFI_SUCCESS           Free the file list successfully.
  @retval EFI_INVALID_PARAMETER FileList was NULL or *FileList was NULL;
**/
EFI_STATUS
EFIAPI
EfiShellFreeFileList(
  IN EFI_SHELL_FILE_INFO **FileList
  );

/**
  Deletes the duplicate file names files in the given file list.

  This function deletes the reduplicate files in the given file list.

  @param FileList               A pointer to the first entry in the file list.

  @retval EFI_SUCCESS           Always success.
  @retval EFI_INVALID_PARAMETER FileList was NULL or *FileList was NULL;
**/
EFI_STATUS
EFIAPI
EfiShellRemoveDupInFileList(
  IN EFI_SHELL_FILE_INFO **FileList
  );

/**
  Allocates and populates a EFI_SHELL_FILE_INFO structure.  if any memory operation
  failed it will return NULL.

  @param[in] BasePath         the Path to prepend onto filename for FullPath
  @param[in] Status           Status member initial value.
  @param[in] FileName         FileName member initial value.
  @param[in] Handle           Handle member initial value.
  @param[in] Info             Info struct to copy.

**/
EFI_SHELL_FILE_INFO *
CreateAndPopulateShellFileInfo(
  IN CONST CHAR16 *BasePath,
  IN CONST EFI_STATUS Status,
  IN CONST CHAR16 *FileName,
  IN CONST SHELL_FILE_HANDLE Handle,
  IN CONST EFI_FILE_INFO *Info
  );

/**
  Find all files in a specified directory.

  @param FileDirHandle     Handle of the directory to search.
  @param FileList          On return, points to the list of files in the directory
                           or NULL if there are no files in the directory.

  @retval EFI_SUCCESS           File information was returned successfully.
  @retval EFI_VOLUME_CORRUPTED  The file system structures have been corrupted.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_NO_MEDIA          The device media is not present.
  @retval EFI_INVALID_PARAMETER The FileDirHandle was not a directory.
**/
EFI_STATUS
EFIAPI
EfiShellFindFilesInDir(
  IN SHELL_FILE_HANDLE FileDirHandle,
  OUT EFI_SHELL_FILE_INFO **FileList
  );

/**
  Find files that match a specified pattern.

  This function searches for all files and directories that match the specified
  FilePattern. The FilePattern can contain wild-card characters. The resulting file
  information is placed in the file list FileList.

  Wildcards are processed
  according to the rules specified in UEFI Shell 2.0 spec section 3.7.1.

  The files in the file list are not opened. The OpenMode field is set to 0 and the FileInfo
  field is set to NULL.

  if *FileList is not NULL then it must be a pre-existing and properly initialized list.

  @param FilePattern      Points to a NULL-terminated shell file path, including wildcards.
  @param FileList         On return, points to the start of a file list containing the names
                          of all matching files or else points to NULL if no matching files
                          were found.  only on a EFI_SUCCESS return will; this be non-NULL.

  @retval EFI_SUCCESS           Files found.  FileList is a valid list.
  @retval EFI_NOT_FOUND         No files found.
  @retval EFI_NO_MEDIA          The device has no media
  @retval EFI_DEVICE_ERROR      The device reported an error
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted
**/
EFI_STATUS
EFIAPI
EfiShellFindFiles(
  IN CONST CHAR16 *FilePattern,
  OUT EFI_SHELL_FILE_INFO **FileList
  );

/**
  Opens the files that match the path specified.

  This function opens all of the files specified by Path. Wildcards are processed
  according to the rules specified in UEFI Shell 2.0 spec section 3.7.1. Each
  matching file has an EFI_SHELL_FILE_INFO structure created in a linked list.

  @param Path                   A pointer to the path string.
  @param OpenMode               Specifies the mode used to open each file, EFI_FILE_MODE_READ or
                                EFI_FILE_MODE_WRITE.
  @param FileList               Points to the start of a list of files opened.

  @retval EFI_SUCCESS           Create the file list successfully.
  @return Others                Can't create the file list.
**/
EFI_STATUS
EFIAPI
EfiShellOpenFileList(
  IN CHAR16 *Path,
  IN UINT64 OpenMode,
  IN OUT EFI_SHELL_FILE_INFO **FileList
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
EfiShellGetEnv(
  IN CONST CHAR16 *Name
  );

/**
  Sets the environment variable.

  This function changes the current value of the specified environment variable. If the
  environment variable exists and the Value is an empty string, then the environment
  variable is deleted. If the environment variable exists and the Value is not an empty
  string, then the value of the environment variable is changed. If the environment
  variable does not exist and the Value is an empty string, there is no action. If the
  environment variable does not exist and the Value is a non-empty string, then the
  environment variable is created and assigned the specified value.

  For a description of volatile and non-volatile environment variables, see UEFI Shell
  2.0 specification section 3.6.1.

  @param Name                   Points to the NULL-terminated environment variable name.
  @param Value                  Points to the NULL-terminated environment variable value. If the value is an
                                empty string then the environment variable is deleted.
  @param Volatile               Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           The environment variable was successfully updated.
**/
EFI_STATUS
EFIAPI
EfiShellSetEnv(
  IN CONST CHAR16 *Name,
  IN CONST CHAR16 *Value,
  IN BOOLEAN Volatile
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
EfiShellGetCurDir(
  IN CONST CHAR16 *FileSystemMapping OPTIONAL
  );

/**
  Changes the current directory on the specified device.

  If the FileSystem is NULL, and the directory Dir does not contain a file system's
  mapped name, this function changes the current working directory. If FileSystem is
  NULL and the directory Dir contains a mapped name, then the current file system and
  the current directory on that file system are changed.

  If FileSystem is not NULL, and Dir is NULL, then this changes the current working file
  system.

  If FileSystem is not NULL and Dir is not NULL, then this function changes the current
  directory on the specified file system.

  If the current working directory or the current working file system is changed then the
  %cwd% environment variable will be updated

  @param FileSystem             A pointer to the file system's mapped name. If NULL, then the current working
                                directory is changed.
  @param Dir                    Points to the NULL-terminated directory on the device specified by FileSystem.

  @retval EFI_SUCCESS           The operation was sucessful
**/
EFI_STATUS
EFIAPI
EfiShellSetCurDir(
  IN CONST CHAR16 *FileSystem OPTIONAL,
  IN CONST CHAR16 *Dir
  );

/**
  Return help information about a specific command.

  This function returns the help information for the specified command. The help text
  can be internal to the shell or can be from a UEFI Shell manual page.

  If Sections is specified, then each section name listed will be compared in a casesensitive
  manner, to the section names described in Appendix B. If the section exists,
  it will be appended to the returned help text. If the section does not exist, no
  information will be returned. If Sections is NULL, then all help text information
  available will be returned.

  @param Command                Points to the NULL-terminated UEFI Shell command name.
  @param Sections               Points to the NULL-terminated comma-delimited
                                section names to return. If NULL, then all
                                sections will be returned.
  @param HelpText               On return, points to a callee-allocated buffer
                                containing all specified help text.

  @retval EFI_SUCCESS           The help text was returned.
  @retval EFI_OUT_OF_RESOURCES  The necessary buffer could not be allocated to hold the
                                returned help text.
  @retval EFI_INVALID_PARAMETER HelpText is NULL
  @retval EFI_NOT_FOUND         There is no help text available for Command.
**/
EFI_STATUS
EFIAPI
EfiShellGetHelpText(
  IN CONST CHAR16 *Command,
  IN CONST CHAR16 *Sections OPTIONAL,
  OUT CHAR16 **HelpText
  );

/**
  Gets the enable status of the page break output mode.

  User can use this function to determine current page break mode.

  @retval TRUE                  The page break output mode is enabled
  @retval FALSE                 The page break output mode is disabled
**/
BOOLEAN
EFIAPI
EfiShellGetPageBreak(
  VOID
  );

/**
  Judges whether the active shell is the root shell.

  This function makes the user to know that whether the active Shell is the root shell.

  @retval TRUE                  The active Shell is the root Shell.
  @retval FALSE                 The active Shell is NOT the root Shell.
**/
BOOLEAN
EFIAPI
EfiShellIsRootShell(
  VOID
  );

/**
  This function returns the command associated with a alias or a list of all
  alias'.

  @param[in] Command            Points to the NULL-terminated shell alias.
                                If this parameter is NULL, then all
                                aliases will be returned in ReturnedData.
  @param[out] Volatile          upon return of a single command if TRUE indicates
                                this is stored in a volatile fashion.  FALSE otherwise.
  @return                        If Alias is not NULL, it will return a pointer to
                                the NULL-terminated command for that alias.
                                If Alias is NULL, ReturnedData points to a ';'
                                delimited list of alias (e.g.
                                ReturnedData = "dir;del;copy;mfp") that is NULL-terminated.
  @retval NULL                  an error ocurred
  @retval NULL                  Alias was not a valid Alias
**/
CONST CHAR16 *
EFIAPI
EfiShellGetAlias(
  IN  CONST CHAR16 *Command,
  OUT BOOLEAN      *Volatile OPTIONAL
  );

/**
  Changes a shell command alias.

  This function creates an alias for a shell command or if Alias is NULL it will delete an existing alias.

  this function does not check for built in alias'.

  @param[in] Command            Points to the NULL-terminated shell command or existing alias.
  @param[in] Alias              Points to the NULL-terminated alias for the shell command. If this is NULL, and
                                Command refers to an alias, that alias will be deleted.
  @param[in] Volatile           if TRUE the Alias being set will be stored in a volatile fashion.  if FALSE the
                                Alias being set will be stored in a non-volatile fashion.

  @retval EFI_SUCCESS           Alias created or deleted successfully.
  @retval EFI_NOT_FOUND         the Alias intended to be deleted was not found
**/
EFI_STATUS
InternalSetAlias(
  IN CONST CHAR16 *Command,
  IN CONST CHAR16 *Alias OPTIONAL,
  IN BOOLEAN Volatile
  );

/**
  Changes a shell command alias.

  This function creates an alias for a shell command or if Alias is NULL it will delete an existing alias.


  @param[in] Command            Points to the NULL-terminated shell command or existing alias.
  @param[in] Alias              Points to the NULL-terminated alias for the shell command. If this is NULL, and
                                Command refers to an alias, that alias will be deleted.
  @param[in] Replace            If TRUE and the alias already exists, then the existing alias will be replaced. If
                                FALSE and the alias already exists, then the existing alias is unchanged and
                                EFI_ACCESS_DENIED is returned.
  @param[in] Volatile           if TRUE the Alias being set will be stored in a volatile fashion.  if FALSE the
                                Alias being set will be stored in a non-volatile fashion.

  @retval EFI_SUCCESS           Alias created or deleted successfully.
  @retval EFI_NOT_FOUND         the Alias intended to be deleted was not found
  @retval EFI_ACCESS_DENIED     The alias is a built-in alias or already existed and Replace was set to
                                FALSE.
**/
EFI_STATUS
EFIAPI
EfiShellSetAlias(
  IN CONST CHAR16 *Command,
  IN CONST CHAR16 *Alias OPTIONAL,
  IN BOOLEAN Replace,
  IN BOOLEAN Volatile
  );

/**
  Utility cleanup function for EFI_SHELL_FILE_INFO objects.

  1) frees all pointers (non-NULL)
  2) Closes the SHELL_FILE_HANDLE

  @param FileListNode     pointer to the list node to free
**/
VOID
InternalFreeShellFileInfoNode(
  IN EFI_SHELL_FILE_INFO *FileListNode
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
InternalEfiShellSetEnv(
  IN CONST CHAR16 *Name,
  IN CONST CHAR16 *Value,
  IN BOOLEAN Volatile
  );

/**
  Function to start monitoring for CTRL-C using SimpleTextInputEx.  This
  feature's enabled state was not known when the shell initially launched.

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough mnemory available.
**/
EFI_STATUS
InernalEfiShellStartMonitor(
  VOID
  );

/**
  Notification function for keystrokes.

  @param[in] KeyData    The key that was pressed.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
NotificationFunction(
  IN EFI_KEY_DATA *KeyData
  );
#endif //_SHELL_PROTOCOL_HEADER_

