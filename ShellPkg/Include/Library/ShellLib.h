/** @file
  Provides interface to shell functionality for shell commands and applications.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright 2018 Dell Technologies.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SHELL_LIB__
#define __SHELL_LIB__

#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>

#define SHELL_FREE_NON_NULL(Pointer)  \
  do {                                \
    if ((Pointer) != NULL) {          \
      FreePool((Pointer));            \
      (Pointer) = NULL;               \
    }                                 \
  } while(FALSE)

extern EFI_SHELL_PARAMETERS_PROTOCOL *gEfiShellParametersProtocol;
extern EFI_SHELL_PROTOCOL            *gEfiShellProtocol;

/**
  Return a clean, fully-qualified version of an input path.  If the return value
  is non-NULL the caller must free the memory when it is no longer needed.

  If asserts are disabled, and if the input parameter is NULL, NULL is returned.

  If there is not enough memory available to create the fully-qualified path or
  a copy of the input path, NULL is returned.

  If there is no working directory, a clean copy of Path is returned.

  Otherwise, the current file system or working directory (as appropriate) is
  prepended to Path and the resulting path is cleaned and returned.

  NOTE: If the input path is an empty string, then the current working directory
  (if it exists) is returned.  In other words, an empty input path is treated
  exactly the same as ".".

  @param[in] Path  A pointer to some file or directory path.

  @retval NULL          The input path is NULL or out of memory.

  @retval non-NULL      A pointer to a clean, fully-qualified version of Path.
                        If there is no working directory, then a pointer to a
                        clean, but not necessarily fully-qualified version of
                        Path.  The caller must free this memory when it is no
                        longer needed.
**/
CHAR16*
EFIAPI
FullyQualifyPath(
  IN     CONST CHAR16     *Path
  );

/**
  This function will retrieve the information about the file for the handle
  specified and store it in allocated pool memory.

  This function allocates a buffer to store the file's information. It is the
  caller's responsibility to free the buffer.

  @param[in] FileHandle         The file handle of the file for which information is
                                being requested.

  @retval NULL                  Information could not be retrieved.

  @return                       The information about the file.
**/
EFI_FILE_INFO*
EFIAPI
ShellGetFileInfo (
  IN SHELL_FILE_HANDLE          FileHandle
  );

/**
  This function sets the information about the file for the opened handle
  specified.

  @param[in]  FileHandle        The file handle of the file for which information
                                is being set.

  @param[in]  FileInfo          The information to set.

  @retval EFI_SUCCESS           The information was set.
  @retval EFI_INVALID_PARAMETER A parameter was out of range or invalid.
  @retval EFI_UNSUPPORTED       The FileHandle does not support FileInfo.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellSetFileInfo (
  IN SHELL_FILE_HANDLE          FileHandle,
  IN EFI_FILE_INFO              *FileInfo
  );

/**
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param[in, out]  FilePath      On input, the device path to the file.  On output,
                                 the remaining device path.
  @param[out]   FileHandle       Pointer to the file handle.
  @param[in]    OpenMode         The mode to open the file with.
  @param[in]    Attributes       The file's file attributes.

  @retval EFI_SUCCESS         The information was set.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Could not open the file path.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device or the file system could not be found on
                                the device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **FilePath,
  OUT SHELL_FILE_HANDLE               *FileHandle,
  IN UINT64                           OpenMode,
  IN UINT64                           Attributes
  );

/**
  This function will open a file or directory referenced by filename.

  If return is EFI_SUCCESS, the Filehandle is the opened file's handle;
  otherwise, the Filehandle is NULL. Attributes is valid only for
  EFI_FILE_MODE_CREATE.

  @param[in] FileName           The pointer to file name.
  @param[out] FileHandle        The pointer to the file handle.
  @param[in] OpenMode           The mode to open the file with.
  @param[in] Attributes         The file's file attributes.

  @retval EFI_SUCCESS         The information was set.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Could not open the file path.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device or the file system could not be found
                                on the device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByName(
  IN CONST CHAR16               *FileName,
  OUT SHELL_FILE_HANDLE         *FileHandle,
  IN UINT64                     OpenMode,
  IN UINT64                     Attributes
  );

/**
  This function creates a directory.

  If return is EFI_SUCCESS, the Filehandle is the opened directory's handle;
  otherwise, the Filehandle is NULL. If the directory already existed, this
  function opens the existing directory.

  @param[in]  DirectoryName     The pointer to Directory name.
  @param[out] FileHandle        The pointer to the file handle.

  @retval EFI_SUCCESS         The information was set.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       Could not open the file path.
  @retval EFI_NOT_FOUND         The specified file could not be found on the
                                device, or the file system could not be found
                                on the device.
  @retval EFI_NO_MEDIA          The device has no medium.
  @retval EFI_MEDIA_CHANGED     The device has a different medium in it or the
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES  Not enough resources were available to open the
                                file.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellCreateDirectory(
  IN CONST CHAR16             *DirectoryName,
  OUT SHELL_FILE_HANDLE       *FileHandle
  );

/**
  This function reads information from an opened file.

  If FileHandle is not a directory, the function reads the requested number of
  bytes from the file at the file's current position and returns them in Buffer.
  If the read goes beyond the end of the file, the read length is truncated to the
  end of the file. The file's current position is increased by the number of bytes
  returned.  If FileHandle is a directory, the function reads the directory entry
  at the file's current position and returns the entry in Buffer. If the Buffer
  is not large enough to hold the current directory entry, then
  EFI_BUFFER_TOO_SMALL is returned and the current file position is not updated.
  BufferSize is set to be the size of the buffer needed to read the entry. On
  success, the current position is updated to the next directory entry. If there
  are no more directory entries, the read returns a zero-length buffer.
  EFI_FILE_INFO is the structure returned as the directory entry.

  @param[in] FileHandle          The opened file handle.
  @param[in, out] ReadSize       On input the size of buffer in bytes.  On return
                                 the number of bytes written.
  @param[out] Buffer             The buffer to put read data into.

  @retval EFI_SUCCESS           Data was read.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL   Buffer is too small. ReadSize contains required
                                size.

**/
EFI_STATUS
EFIAPI
ShellReadFile(
  IN SHELL_FILE_HANDLE          FileHandle,
  IN OUT UINTN                  *ReadSize,
  OUT VOID                      *Buffer
  );

/**
  Write data to a file.

  This function writes the specified number of bytes to the file at the current
  file position. The current file position is advanced the actual number of bytes
  written, which is returned in BufferSize. Partial writes only occur when there
  has been a data error during the write attempt (such as "volume space full").
  The file is automatically grown to hold the data if required. Direct writes to
  opened directories are not supported.

  @param[in] FileHandle          The opened file for writing.

  @param[in, out] BufferSize     On input the number of bytes in Buffer.  On output
                                 the number of bytes written.

  @param[in] Buffer              The buffer containing data to write is stored.

  @retval EFI_SUCCESS           Data was written.
  @retval EFI_UNSUPPORTED       Writes to an open directory are not supported.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The device is write-protected.
  @retval EFI_ACCESS_DENIED     The file was open for read only.
  @retval EFI_VOLUME_FULL       The volume is full.
**/
EFI_STATUS
EFIAPI
ShellWriteFile(
  IN SHELL_FILE_HANDLE          FileHandle,
  IN OUT UINTN                  *BufferSize,
  IN VOID                       *Buffer
  );

/**
  Close an open file handle.

  This function closes a specified file handle. All "dirty" cached file data is
  flushed to the device, and the file is closed. In all cases the handle is
  closed.

  @param[in] FileHandle           The file handle to close.

  @retval EFI_SUCCESS             The file handle was closed sucessfully.
  @retval INVALID_PARAMETER       One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellCloseFile (
  IN SHELL_FILE_HANDLE          *FileHandle
  );

/**
  Delete a file and close the handle

  This function closes and deletes a file. In all cases the file handle is closed.
  If the file cannot be deleted, the warning code EFI_WARN_DELETE_FAILURE is
  returned, but the handle is still closed.

  @param[in] FileHandle             The file handle to delete.

  @retval EFI_SUCCESS               The file was closed sucessfully.
  @retval EFI_WARN_DELETE_FAILURE   The handle was closed, but the file was not
                                    deleted.
  @retval INVALID_PARAMETER         One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellDeleteFile (
  IN SHELL_FILE_HANDLE          *FileHandle
  );

/**
  Set the current position in a file.

  This function sets the current file position for the handle to the position
  supplied. With the exception of seeking to position 0xFFFFFFFFFFFFFFFF, only
  absolute positioning is supported, and moving past the end of the file is
  allowed (a subsequent write would grow the file). Moving to position
  0xFFFFFFFFFFFFFFFF causes the current position to be set to the end of the file.
  If FileHandle is a directory, the only position that may be set is zero. This
  has the effect of starting the read process of the directory entries over.

  @param[in] FileHandle         The file handle on which the position is being set.

  @param[in] Position           The byte position from the begining of the file.

  @retval EFI_SUCCESS           Operation completed sucessfully.
  @retval EFI_UNSUPPORTED       The seek request for non-zero is not valid on
                                directories.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellSetFilePosition (
  IN SHELL_FILE_HANDLE  FileHandle,
  IN UINT64             Position
  );

/**
  Gets a file's current position

  This function retrieves the current file position for the file handle. For
  directories, the current file position has no meaning outside of the file
  system driver and as such the operation is not supported. An error is returned
  if FileHandle is a directory.

  @param[in] FileHandle         The open file handle on which to get the position.
  @param[out] Position          The byte position from the begining of the file.

  @retval EFI_SUCCESS           The operation completed sucessfully.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       The request is not valid on directories.
**/
EFI_STATUS
EFIAPI
ShellGetFilePosition (
  IN SHELL_FILE_HANDLE          FileHandle,
  OUT UINT64                    *Position
  );

/**
  Flushes data on a file

  This function flushes all modified data associated with a file to a device.

  @param[in] FileHandle         The file handle on which to flush data.

  @retval EFI_SUCCESS           The data was flushed.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED     The file was opened for read only.
**/
EFI_STATUS
EFIAPI
ShellFlushFile (
  IN SHELL_FILE_HANDLE          FileHandle
  );

/** Retrieve first entry from a directory.

  This function takes an open directory handle and gets information from the
  first entry in the directory.  A buffer is allocated to contain
  the information and a pointer to the buffer is returned in *Buffer.  The
  caller can use ShellFindNextFile() to get subsequent directory entries.

  The buffer will be freed by ShellFindNextFile() when the last directory
  entry is read.  Otherwise, the caller must free the buffer, using FreePool,
  when finished with it.

  @param[in]  DirHandle         The file handle of the directory to search.
  @param[out] Buffer            The pointer to the buffer for the file's information.

  @retval EFI_SUCCESS           Found the first file.
  @retval EFI_NOT_FOUND         Cannot find the directory.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @return Others                Status of ShellGetFileInfo, ShellSetFilePosition,
                                or ShellReadFile.

  @sa ShellReadFile
**/
EFI_STATUS
EFIAPI
ShellFindFirstFile (
  IN      SHELL_FILE_HANDLE       DirHandle,
     OUT  EFI_FILE_INFO         **Buffer
  );

/** Retrieve next entries from a directory.

  To use this function, the caller must first call the ShellFindFirstFile()
  function to get the first directory entry.  Subsequent directory entries are
  retrieved by using the ShellFindNextFile() function.  This function can
  be called several times to get each entry from the directory.  If the call of
  ShellFindNextFile() retrieved the last directory entry, the next call of
  this function will set *NoFile to TRUE and free the buffer.

  @param[in]  DirHandle         The file handle of the directory.
  @param[in, out] Buffer        The pointer to buffer for file's information.
  @param[in, out] NoFile        The pointer to boolean when last file is found.

  @retval EFI_SUCCESS           Found the next file.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.

  @sa ShellReadFile
**/
EFI_STATUS
EFIAPI
ShellFindNextFile(
  IN      SHELL_FILE_HANDLE       DirHandle,
  IN OUT  EFI_FILE_INFO          *Buffer,
  IN OUT  BOOLEAN                *NoFile
  );

/**
  Retrieve the size of a file.

  This function extracts the file size info from the FileHandle's EFI_FILE_INFO
  data.

  @param[in] FileHandle         The file handle from which size is retrieved.
  @param[out] Size              The pointer to size.

  @retval EFI_SUCCESS           The operation was completed sucessfully.
  @retval EFI_DEVICE_ERROR      Cannot access the file.
**/
EFI_STATUS
EFIAPI
ShellGetFileSize (
  IN SHELL_FILE_HANDLE          FileHandle,
  OUT UINT64                    *Size
  );

/**
  Retrieves the status of the break execution flag

  This function is useful to check whether the application is being asked to halt by the shell.

  @retval TRUE                  The execution break is enabled.
  @retval FALSE                 The execution break is not enabled.
**/
BOOLEAN
EFIAPI
ShellGetExecutionBreakFlag(
  VOID
  );

/**
  Return the value of an environment variable.

  This function gets the value of the environment variable set by the
  ShellSetEnvironmentVariable function.

  @param[in] EnvKey             The key name of the environment variable.

  @retval NULL                  The named environment variable does not exist.
  @return != NULL               The pointer to the value of the environment variable.
**/
CONST CHAR16*
EFIAPI
ShellGetEnvironmentVariable (
  IN CONST CHAR16                *EnvKey
  );

/**
  Set the value of an environment variable.

  This function changes the current value of the specified environment variable. If the
  environment variable exists and the Value is an empty string, then the environment
  variable is deleted. If the environment variable exists and the Value is not an empty
  string, then the value of the environment variable is changed. If the environment
  variable does not exist and the Value is an empty string, there is no action. If the
  environment variable does not exist and the Value is a non-empty string, then the
  environment variable is created and assigned the specified value.

  This is not supported pre-UEFI Shell 2.0.

  @param[in] EnvKey             The key name of the environment variable.
  @param[in] EnvVal             The Value of the environment variable
  @param[in] Volatile           Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           The operation completed sucessfully
  @retval EFI_UNSUPPORTED       This operation is not allowed in pre-UEFI 2.0 Shell environments.
**/
EFI_STATUS
EFIAPI
ShellSetEnvironmentVariable (
  IN CONST CHAR16               *EnvKey,
  IN CONST CHAR16               *EnvVal,
  IN BOOLEAN                    Volatile
  );

/**
  Cause the shell to parse and execute a command line.

  This function creates a nested instance of the shell and executes the specified
  command (CommandLine) with the specified environment (Environment). Upon return,
  the status code returned by the specified command is placed in StatusCode.
  If Environment is NULL, then the current environment is used and all changes made
  by the commands executed will be reflected in the current environment. If the
  Environment is non-NULL, then the changes made will be discarded.
  The CommandLine is executed from the current working directory on the current
  device.

  The EnvironmentVariables and Status parameters are ignored in a pre-UEFI Shell 2.0
  environment.  The values pointed to by the parameters will be unchanged by the
  ShellExecute() function.  The Output parameter has no effect in a
  UEFI Shell 2.0 environment.

  @param[in] ParentHandle         The parent image starting the operation.
  @param[in] CommandLine          The pointer to a NULL terminated command line.
  @param[in] Output               True to display debug output.  False to hide it.
  @param[in] EnvironmentVariables Optional pointer to array of environment variables
                                  in the form "x=y".  If NULL, the current set is used.
  @param[out] Status              The status of the run command line.

  @retval EFI_SUCCESS             The operation completed sucessfully.  Status
                                  contains the status code returned.
  @retval EFI_INVALID_PARAMETER   A parameter contains an invalid value.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
  @retval EFI_UNSUPPORTED         The operation is not allowed.
**/
EFI_STATUS
EFIAPI
ShellExecute (
  IN EFI_HANDLE                 *ParentHandle,
  IN CHAR16                     *CommandLine,
  IN BOOLEAN                    Output,
  IN CHAR16                     **EnvironmentVariables,
  OUT EFI_STATUS                *Status
  );

/**
  Retreives the current directory path.

  If the DeviceName is NULL, it returns the current device's current directory
  name. If the DeviceName is not NULL, it returns the current directory name
  on specified drive.

  Note that the current directory string should exclude the tailing backslash character.

  @param[in] DeviceName         The name of the file system to get directory on.

  @retval NULL                  The directory does not exist.
  @retval != NULL               The directory.
**/
CONST CHAR16*
EFIAPI
ShellGetCurrentDir (
  IN CHAR16                     * CONST DeviceName OPTIONAL
  );

/**
  Sets (enabled or disabled) the page break mode.

  When page break mode is enabled the screen will stop scrolling
  and wait for operator input before scrolling a subsequent screen.

  @param[in] CurrentState       TRUE to enable and FALSE to disable.
**/
VOID
EFIAPI
ShellSetPageBreakMode (
  IN BOOLEAN                    CurrentState
  );

/**
  Opens a group of files based on a path.

  This function uses the Arg to open all the matching files. Each matched
  file has a SHELL_FILE_ARG structure to record the file information. These
  structures are placed on the list ListHead. Users can get the SHELL_FILE_ARG
  structures from ListHead to access each file. This function supports wildcards
  and will process '?' and '*' as such.  The list must be freed with a call to
  ShellCloseFileMetaArg().

  If you are NOT appending to an existing list *ListHead must be NULL.  If
  *ListHead is NULL then it must be callee freed.

  @param[in] Arg                 The pointer to path string.
  @param[in] OpenMode            Mode to open files with.
  @param[in, out] ListHead       Head of linked list of results.

  @retval EFI_SUCCESS           The operation was sucessful and the list head
                                contains the list of opened files.
  @retval != EFI_SUCCESS        The operation failed.

  @sa InternalShellConvertFileListType
**/
EFI_STATUS
EFIAPI
ShellOpenFileMetaArg (
  IN CHAR16                     *Arg,
  IN UINT64                     OpenMode,
  IN OUT EFI_SHELL_FILE_INFO    **ListHead
  );

/**
  Free the linked list returned from ShellOpenFileMetaArg.

  @param[in, out] ListHead       The pointer to free.

  @retval EFI_SUCCESS           The operation was sucessful.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
**/
EFI_STATUS
EFIAPI
ShellCloseFileMetaArg (
  IN OUT EFI_SHELL_FILE_INFO    **ListHead
  );

/**
  Find a file by searching the CWD and then the path.

  If FileName is NULL, then ASSERT.

  If the return value is not NULL then the memory must be caller freed.

  @param[in] FileName           Filename string.

  @retval NULL                  The file was not found.
  @retval !NULL                 The path to the file.
**/
CHAR16 *
EFIAPI
ShellFindFilePath (
  IN CONST CHAR16 *FileName
  );

/**
  Find a file by searching the CWD and then the path with a variable set of file
  extensions.  If the file is not found it will append each extension in the list
  in the order provided and return the first one that is successful.

  If FileName is NULL, then ASSERT.
  If FileExtension is NULL, then the behavior is identical to ShellFindFilePath.

  If the return value is not NULL then the memory must be caller freed.

  @param[in] FileName           The filename string.
  @param[in] FileExtension      Semicolon delimited list of possible extensions.

  @retval NULL                  The file was not found.
  @retval !NULL                 The path to the file.
**/
CHAR16 *
EFIAPI
ShellFindFilePathEx (
  IN CONST CHAR16 *FileName,
  IN CONST CHAR16 *FileExtension
  );

typedef enum {
  TypeFlag  = 0,    ///< A flag that is present or not present only (IE "-a").
  TypeValue,        ///< A flag that has some data following it with a space (IE "-a 1").
  TypePosition,     ///< Some data that did not follow a parameter (IE "filename.txt").
  TypeStart,        ///< A flag that has variable value appended to the end (IE "-ad", "-afd", "-adf", etc...).
  TypeDoubleValue,  ///< A flag that has 2 space seperated value data following it (IE "-a 1 2").
  TypeMaxValue,     ///< A flag followed by all the command line data before the next flag.
  TypeTimeValue,    ///< A flag that has a time value following it (IE "-a -5:00").
  TypeMax,
} SHELL_PARAM_TYPE;

typedef struct {
  CHAR16             *Name;
  SHELL_PARAM_TYPE   Type;
} SHELL_PARAM_ITEM;


/// Helper structure for no parameters (besides -? and -b)
extern SHELL_PARAM_ITEM EmptyParamList[];

/// Helper structure for -sfo only (besides -? and -b)
extern SHELL_PARAM_ITEM SfoParamList[];

/**
  Checks the command line arguments passed against the list of valid ones.
  Optionally removes NULL values first.

  If no initialization is required, then return RETURN_SUCCESS.

  @param[in] CheckList          The pointer to list of parameters to check.
  @param[out] CheckPackage      The package of checked values.
  @param[out] ProblemParam      Optional pointer to pointer to unicode string for
                                the paramater that caused failure.
  @param[in] AutoPageBreak      Will automatically set PageBreakEnabled.
  @param[in] AlwaysAllowNumbers Will never fail for number based flags.

  @retval EFI_SUCCESS           The operation completed sucessfully.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
  @retval EFI_VOLUME_CORRUPTED  The command line was corrupt.
  @retval EFI_DEVICE_ERROR      The commands contained 2 opposing arguments.  One
                                of the command line arguments was returned in
                                ProblemParam if provided.
  @retval EFI_NOT_FOUND         A argument required a value that was missing.
                                The invalid command line argument was returned in
                                ProblemParam if provided.
**/
EFI_STATUS
EFIAPI
ShellCommandLineParseEx (
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT LIST_ENTRY                **CheckPackage,
  OUT CHAR16                    **ProblemParam OPTIONAL,
  IN BOOLEAN                    AutoPageBreak,
  IN BOOLEAN                    AlwaysAllowNumbers
  );

/// Make it easy to upgrade from older versions of the shell library.
#define ShellCommandLineParse(CheckList,CheckPackage,ProblemParam,AutoPageBreak) ShellCommandLineParseEx(CheckList,CheckPackage,ProblemParam,AutoPageBreak,FALSE)

/**
  Frees shell variable list that was returned from ShellCommandLineParse.

  This function will free all the memory that was used for the CheckPackage
  list of postprocessed shell arguments.

  If CheckPackage is NULL, then return.

  @param[in] CheckPackage       The list to de-allocate.
  **/
VOID
EFIAPI
ShellCommandLineFreeVarList (
  IN LIST_ENTRY                 *CheckPackage
  );

/**
  Checks for presence of a flag parameter.

  Flag arguments are in the form of "-<Key>" or "/<Key>", but do not have a value following the key.

  If CheckPackage is NULL then return FALSE.
  If KeyString is NULL then ASSERT().

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[in] KeyString          The Key of the command line argument to check for.

  @retval TRUE                  The flag is on the command line.
  @retval FALSE                 The flag is not on the command line.
**/
BOOLEAN
EFIAPI
ShellCommandLineGetFlag (
  IN CONST LIST_ENTRY         * CONST CheckPackage,
  IN CONST CHAR16             * CONST KeyString
  );

/**
  Returns value from command line argument.

  Value parameters are in the form of "-<Key> value" or "/<Key> value".

  If CheckPackage is NULL, then return NULL.

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[in] KeyString          The Key of the command line argument to check for.

  @retval NULL                  The flag is not on the command line.
  @retval !=NULL                The pointer to unicode string of the value.
**/
CONST CHAR16*
EFIAPI
ShellCommandLineGetValue (
  IN CONST LIST_ENTRY              *CheckPackage,
  IN CHAR16                        *KeyString
  );

/**
  Returns raw value from command line argument.

  Raw value parameters are in the form of "value" in a specific position in the list.

  If CheckPackage is NULL, then return NULL.

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[in] Position           The position of the value.

  @retval NULL                  The flag is not on the command line.
  @retval !=NULL                The pointer to unicode string of the value.
**/
CONST CHAR16*
EFIAPI
ShellCommandLineGetRawValue (
  IN CONST LIST_ENTRY              * CONST CheckPackage,
  IN UINTN                         Position
  );

/**
  Returns the number of command line value parameters that were parsed.

  This will not include flags.

  @param[in] CheckPackage       The package of parsed command line arguments.

  @retval (UINTN)-1             No parsing has occurred.
  @retval other                 The number of value parameters found.
**/
UINTN
EFIAPI
ShellCommandLineGetCount(
  IN CONST LIST_ENTRY              *CheckPackage
  );

/**
  Determines if a parameter is duplicated.

  If Param is not NULL, then it will point to a callee-allocated string buffer
  with the parameter value, if a duplicate is found.

  If CheckPackage is NULL, then ASSERT.

  @param[in] CheckPackage       The package of parsed command line arguments.
  @param[out] Param             Upon finding one, a pointer to the duplicated parameter.

  @retval EFI_SUCCESS           No parameters were duplicated.
  @retval EFI_DEVICE_ERROR      A duplicate was found.
  **/
EFI_STATUS
EFIAPI
ShellCommandLineCheckDuplicate (
  IN CONST LIST_ENTRY              *CheckPackage,
  OUT CHAR16                       **Param
  );

/**
  This function causes the shell library to initialize itself.  If the shell library
  is already initialized it will de-initialize all the current protocol pointers and
  re-populate them again.

  When the library is used with PcdShellLibAutoInitialize set to true this function
  will return EFI_SUCCESS and perform no actions.

  This function is intended for internal access for shell commands only.

  @retval EFI_SUCCESS   The initialization was complete sucessfully.

**/
EFI_STATUS
EFIAPI
ShellInitialize (
  VOID
  );

/**
  Print at a specific location on the screen.

  This function will move the cursor to a given screen location and print the specified string.

  If -1 is specified for either the Row or Col the current screen location for BOTH
  will be used.

  If either Row or Col is out of range for the current console, then ASSERT.
  If Format is NULL, then ASSERT.

  In addition to the standard %-based flags as supported by UefiLib Print() this supports
  the following additional flags:
    %N       -   Set output attribute to normal
    %H       -   Set output attribute to highlight
    %E       -   Set output attribute to error
    %B       -   Set output attribute to blue color
    %V       -   Set output attribute to green color

  Note: The background color is controlled by the shell command cls.

  @param[in] Col        The column to print at.
  @param[in] Row        The row to print at.
  @param[in] Format     The format string.
  @param[in] ...        The variable argument list.

  @return EFI_SUCCESS           The printing was successful.
  @return EFI_DEVICE_ERROR      The console device reported an error.
**/
EFI_STATUS
EFIAPI
ShellPrintEx(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR16         *Format,
  ...
  );

/**
  Print at a specific location on the screen.

  This function will move the cursor to a given screen location and print the specified string.

  If -1 is specified for either the Row or Col the current screen location for BOTH
  will be used.

  If either Row or Col is out of range for the current console, then ASSERT.
  If Format is NULL, then ASSERT.

  In addition to the standard %-based flags as supported by UefiLib Print() this supports
  the following additional flags:
    %N       -   Set output attribute to normal.
    %H       -   Set output attribute to highlight.
    %E       -   Set output attribute to error.
    %B       -   Set output attribute to blue color.
    %V       -   Set output attribute to green color.

  Note: The background color is controlled by the shell command cls.

  @param[in] Col                The column to print at.
  @param[in] Row                The row to print at.
  @param[in] Language           The language of the string to retrieve.  If this parameter
                                is NULL, then the current platform language is used.
  @param[in] HiiFormatStringId  The format string Id for getting from Hii.
  @param[in] HiiFormatHandle    The format string Handle for getting from Hii.
  @param[in] ...                The variable argument list.

  @return EFI_SUCCESS           The printing was successful.
  @return EFI_DEVICE_ERROR      The console device reported an error.
**/
EFI_STATUS
EFIAPI
ShellPrintHiiEx(
  IN INT32                Col OPTIONAL,
  IN INT32                Row OPTIONAL,
  IN CONST CHAR8          *Language OPTIONAL,
  IN CONST EFI_STRING_ID  HiiFormatStringId,
  IN CONST EFI_HII_HANDLE HiiFormatHandle,
  ...
  );

/**
  Function to determine if a given filename represents a directory.

  If DirName is NULL, then ASSERT.

  @param[in] DirName      Path to directory to test.

  @retval EFI_SUCCESS     The Path represents a directory.
  @retval EFI_NOT_FOUND   The Path does not represent a directory.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellIsDirectory(
  IN CONST CHAR16 *DirName
  );

/**
  Function to determine if a given filename represents a file.

  This will search the CWD only.

  If Name is NULL, then ASSERT.

  @param[in] Name         Path to file to test.

  @retval EFI_SUCCESS     The Path represents a file.
  @retval EFI_NOT_FOUND   The Path does not represent a file.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellIsFile(
  IN CONST CHAR16 *Name
  );

/**
  Function to determine if a given filename represents a file.

  This will search the CWD and then the Path.

  If Name is NULL, then ASSERT.

  @param[in] Name         Path to file to test.

  @retval EFI_SUCCESS     The Path represents a file.
  @retval EFI_NOT_FOUND   The Path does not represent a file.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellIsFileInPath(
  IN CONST CHAR16 *Name
  );

/**
  Function to determine whether a string is decimal or hex representation of a number
  and return the number converted from the string.

  Note: this function cannot be used when (UINTN)(-1), (0xFFFFFFFF) may be a valid
  result.  Use ShellConvertStringToUint64 instead.

  @param[in] String   String representation of a number.

  @return             The unsigned integer result of the conversion.
  @retval (UINTN)(-1) An error occurred.
**/
UINTN
EFIAPI
ShellStrToUintn(
  IN CONST CHAR16 *String
  );

/**
  Function return the number converted from a hex representation of a number.

  Note: this function cannot be used when (UINTN)(-1), (0xFFFFFFFF) may be a valid
  result.  Use ShellConvertStringToUint64 instead.

  @param[in] String   String representation of a number.

  @return             The unsigned integer result of the conversion.
  @retval (UINTN)(-1) An error occurred.
**/
UINTN
EFIAPI
ShellHexStrToUintn(
  IN CONST CHAR16 *String
  );

/**
  Safely append with automatic string resizing given length of Destination and
  desired length of copy from Source.

  Append the first D characters of Source to the end of Destination, where D is
  the lesser of Count and the StrLen() of Source. If appending those D characters
  will fit within Destination (whose Size is given as CurrentSize) and
  still leave room for a NULL terminator, then those characters are appended,
  starting at the original terminating NULL of Destination, and a new terminating
  NULL is appended.

  If appending D characters onto Destination will result in a overflow of the size
  given in CurrentSize the string will be grown such that the copy can be performed
  and CurrentSize will be updated to the new size.

  If Source is NULL, there is nothing to append, so return the current buffer in
  Destination.

  If Destination is NULL, then ASSERT().
  If Destination's current length (including NULL terminator) is already more than
  CurrentSize, then ASSERT().

  @param[in, out] Destination    The String to append onto.
  @param[in, out] CurrentSize    On call, the number of bytes in Destination.  On
                                 return, possibly the new size (still in bytes).  If NULL,
                                 then allocate whatever is needed.
  @param[in]      Source         The String to append from.
  @param[in]      Count          The maximum number of characters to append.  If 0, then
                                 all are appended.

  @return                       The Destination after appending the Source.
**/
CHAR16*
EFIAPI
StrnCatGrow (
  IN OUT CHAR16           **Destination,
  IN OUT UINTN            *CurrentSize,
  IN     CONST CHAR16     *Source,
  IN     UINTN            Count
  );

/**
  This is a find and replace function.  Upon successful return the NewString is a copy of
  SourceString with each instance of FindTarget replaced with ReplaceWith.

  If SourceString and NewString overlap the behavior is undefined.

  If the string would grow bigger than NewSize it will halt and return error.

  @param[in] SourceString              The string with source buffer.
  @param[in, out] NewString            The string with resultant buffer.
  @param[in] NewSize                   The size in bytes of NewString.
  @param[in] FindTarget                The string to look for.
  @param[in] ReplaceWith               The string to replace FindTarget with.
  @param[in] SkipPreCarrot             If TRUE will skip a FindTarget that has a '^'
                                       immediately before it.
  @param[in] ParameterReplacing        If TRUE will add "" around items with spaces.

  @retval EFI_INVALID_PARAMETER       SourceString was NULL.
  @retval EFI_INVALID_PARAMETER       NewString was NULL.
  @retval EFI_INVALID_PARAMETER       FindTarget was NULL.
  @retval EFI_INVALID_PARAMETER       ReplaceWith was NULL.
  @retval EFI_INVALID_PARAMETER       FindTarget had length < 1.
  @retval EFI_INVALID_PARAMETER       SourceString had length < 1.
  @retval EFI_BUFFER_TOO_SMALL        NewSize was less than the minimum size to hold
                                      the new string (truncation occurred).
  @retval EFI_SUCCESS                 The string was successfully copied with replacement.
**/
EFI_STATUS
EFIAPI
ShellCopySearchAndReplace(
  IN CHAR16 CONST                     *SourceString,
  IN OUT CHAR16                       *NewString,
  IN UINTN                            NewSize,
  IN CONST CHAR16                     *FindTarget,
  IN CONST CHAR16                     *ReplaceWith,
  IN CONST BOOLEAN                    SkipPreCarrot,
  IN CONST BOOLEAN                    ParameterReplacing
  );

/**
  Check if a Unicode character is a hexadecimal character.

  This internal function checks if a Unicode character is a
  numeric character.  The valid hexadecimal characters are
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.


  @param  Char  The character to check against.

  @retval TRUE  The Char is a hexadecmial character.
  @retval FALSE The Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
ShellIsHexaDecimalDigitCharacter (
  IN      CHAR16                    Char
  );

/**
  Check if a Unicode character is a decimal character.

  This internal function checks if a Unicode character is a
  decimal character.  The valid characters are
  L'0' to L'9'.


  @param  Char  The character to check against.

  @retval TRUE  The Char is a hexadecmial character.
  @retval FALSE The Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
ShellIsDecimalDigitCharacter (
  IN      CHAR16                    Char
  );

///
/// What type of answer is requested.
///
typedef enum {
  ShellPromptResponseTypeYesNo,
  ShellPromptResponseTypeYesNoCancel,
  ShellPromptResponseTypeFreeform,
  ShellPromptResponseTypeQuitContinue,
  ShellPromptResponseTypeYesNoAllCancel,
  ShellPromptResponseTypeEnterContinue,
  ShellPromptResponseTypeAnyKeyContinue,
  ShellPromptResponseTypeMax
} SHELL_PROMPT_REQUEST_TYPE;

///
/// What answer was given.
///
typedef enum {
  ShellPromptResponseYes,
  ShellPromptResponseNo,
  ShellPromptResponseCancel,
  ShellPromptResponseQuit,
  ShellPromptResponseContinue,
  ShellPromptResponseAll,
  ShellPromptResponseMax
} SHELL_PROMPT_RESPONSE;

/**
  Prompt the user and return the resultant answer to the requestor.

  This function will display the requested question on the shell prompt and then
  wait for an appropriate answer to be input from the console.

  If the SHELL_PROMPT_REQUEST_TYPE is SHELL_PROMPT_REQUEST_TYPE_YESNO, ShellPromptResponseTypeQuitContinue
  or SHELL_PROMPT_REQUEST_TYPE_YESNOCANCEL then *Response is of type SHELL_PROMPT_RESPONSE.

  If the SHELL_PROMPT_REQUEST_TYPE is ShellPromptResponseTypeFreeform then *Response is of type
  CHAR16*.

  In either case *Response must be callee freed if Response was not NULL;

  @param Type                     What type of question is asked.  This is used to filter the input
                                  to prevent invalid answers to question.
  @param Prompt                   The pointer to a string prompt used to request input.
  @param Response                 The pointer to Response, which will be populated upon return.

  @retval EFI_SUCCESS             The operation was successful.
  @retval EFI_UNSUPPORTED         The operation is not supported as requested.
  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @return other                   The operation failed.
**/
EFI_STATUS
EFIAPI
ShellPromptForResponse (
  IN SHELL_PROMPT_REQUEST_TYPE   Type,
  IN CHAR16         *Prompt OPTIONAL,
  IN OUT VOID       **Response OPTIONAL
  );

/**
  Prompt the user and return the resultant answer to the requestor.

  This function is the same as ShellPromptForResponse, except that the prompt is
  automatically pulled from HII.

  @param[in] Type What type of question is asked.  This is used to filter the input
                  to prevent invalid answers to question.
  @param[in] HiiFormatStringId   The format string Id for getting from Hii.
  @param[in] HiiFormatHandle     The format string Handle for getting from Hii.
  @param[in, out] Response       The pointer to Response, which will be populated upon return.

  @retval EFI_SUCCESS The operation was sucessful.
  @return other       The operation failed.

  @sa ShellPromptForResponse
**/
EFI_STATUS
EFIAPI
ShellPromptForResponseHii (
  IN SHELL_PROMPT_REQUEST_TYPE         Type,
  IN CONST EFI_STRING_ID  HiiFormatStringId,
  IN CONST EFI_HII_HANDLE HiiFormatHandle,
  IN OUT VOID             **Response
  );

/**
  Function to determin if an entire string is a valid number.

  If Hex it must be preceeded with a 0x, 0X, or has ForceHex set TRUE.

  @param[in] String       The string to evaluate.
  @param[in] ForceHex     TRUE - always assume hex.
  @param[in] StopAtSpace  TRUE to halt upon finding a space, FALSE to keep going.

  @retval TRUE        It is all numeric (dec/hex) characters.
  @retval FALSE       There is a non-numeric character.
**/
BOOLEAN
EFIAPI
ShellIsHexOrDecimalNumber (
  IN CONST CHAR16   *String,
  IN CONST BOOLEAN  ForceHex,
  IN CONST BOOLEAN  StopAtSpace
  );

/**
  Function to verify and convert a string to its numerical 64 bit representation.

  If Hex it must be preceeded with a 0x, 0X, or has ForceHex set TRUE.

  @param[in] String       The string to evaluate.
  @param[out] Value       Upon a successful return the value of the conversion.
  @param[in] ForceHex     TRUE - always assume hex.
  @param[in] StopAtSpace  TRUE to halt upon finding a space, FALSE to
                          process the entire String.

  @retval EFI_SUCCESS             The conversion was successful.
  @retval EFI_INVALID_PARAMETER   String contained an invalid character.
  @retval EFI_NOT_FOUND           String was a number, but Value was NULL.
**/
EFI_STATUS
EFIAPI
ShellConvertStringToUint64(
  IN CONST CHAR16   *String,
     OUT   UINT64   *Value,
  IN CONST BOOLEAN  ForceHex,
  IN CONST BOOLEAN  StopAtSpace
  );

/**
  Function to determine if a given filename exists.

  @param[in] Name         Path to test.

  @retval EFI_SUCCESS     The Path represents a file.
  @retval EFI_NOT_FOUND   The Path does not represent a file.
  @retval other           The path failed to open.
**/
EFI_STATUS
EFIAPI
ShellFileExists(
  IN CONST CHAR16 *Name
  );

/**
  Function to read a single line from a SHELL_FILE_HANDLE. The \n is not included in the returned
  buffer.  The returned buffer must be callee freed.

  If the position upon start is 0, then the Ascii Boolean will be set.  This should be
  maintained and not changed for all operations with the same file.

  @param[in]       Handle        SHELL_FILE_HANDLE to read from.
  @param[in, out]  Ascii         Boolean value for indicating whether the file is
                                 Ascii (TRUE) or UCS2 (FALSE).

  @return                        The line of text from the file.

  @sa ShellFileHandleReadLine
**/
CHAR16*
EFIAPI
ShellFileHandleReturnLine(
  IN SHELL_FILE_HANDLE            Handle,
  IN OUT BOOLEAN                *Ascii
  );

/**
  Function to read a single line (up to but not including the \n) from a SHELL_FILE_HANDLE.

  If the position upon start is 0, then the Ascii Boolean will be set.  This should be
  maintained and not changed for all operations with the same file.

  @param[in]       Handle        SHELL_FILE_HANDLE to read from.
  @param[in, out]  Buffer        The pointer to buffer to read into.
  @param[in, out]  Size          The pointer to number of bytes in Buffer.
  @param[in]       Truncate      If the buffer is large enough, this has no effect.
                                 If the buffer is is too small and Truncate is TRUE,
                                 the line will be truncated.
                                 If the buffer is is too small and Truncate is FALSE,
                                 then no read will occur.

  @param[in, out]  Ascii         Boolean value for indicating whether the file is
                                 Ascii (TRUE) or UCS2 (FALSE).

  @retval EFI_SUCCESS           The operation was successful.  The line is stored in
                                Buffer.
  @retval EFI_END_OF_FILE       There are no more lines in the file.
  @retval EFI_INVALID_PARAMETER Handle was NULL.
  @retval EFI_INVALID_PARAMETER Size was NULL.
  @retval EFI_BUFFER_TOO_SMALL  Size was not large enough to store the line.
                                Size was updated to the minimum space required.
**/
EFI_STATUS
EFIAPI
ShellFileHandleReadLine(
  IN SHELL_FILE_HANDLE          Handle,
  IN OUT CHAR16                 *Buffer,
  IN OUT UINTN                  *Size,
  IN BOOLEAN                    Truncate,
  IN OUT BOOLEAN                *Ascii
  );

/**
  Function to delete a file by name

  @param[in]       FileName       Pointer to file name to delete.

  @retval EFI_SUCCESS             the file was deleted sucessfully
  @retval EFI_WARN_DELETE_FAILURE the handle was closed, but the file was not
                                  deleted
  @retval EFI_INVALID_PARAMETER   One of the parameters has an invalid value.
  @retval EFI_NOT_FOUND           The specified file could not be found on the
                                  device or the file system could not be found
                                  on the device.
  @retval EFI_NO_MEDIA            The device has no medium.
  @retval EFI_MEDIA_CHANGED       The device has a different medium in it or the
                                  medium is no longer supported.
  @retval EFI_DEVICE_ERROR        The device reported an error.
  @retval EFI_VOLUME_CORRUPTED    The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED     The file or medium is write protected.
  @retval EFI_ACCESS_DENIED       The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES    Not enough resources were available to open the
                                  file.
  @retval other                   The file failed to open
**/
EFI_STATUS
EFIAPI
ShellDeleteFileByName(
  IN CONST CHAR16               *FileName
  );

/**
  Function to print help file / man page content in the spec from the UEFI Shell protocol GetHelpText function.

  @param[in] CommandToGetHelpOn  Pointer to a string containing the command name of help file to be printed.
  @param[in] SectionToGetHelpOn  Pointer to the section specifier(s).
  @param[in] PrintCommandText    If TRUE, prints the command followed by the help content, otherwise prints
                                 the help content only.
  @retval EFI_DEVICE_ERROR       The help data format was incorrect.
  @retval EFI_NOT_FOUND          The help data could not be found.
  @retval EFI_SUCCESS            The operation was successful.
**/
EFI_STATUS
EFIAPI
ShellPrintHelp (
  IN CONST CHAR16     *CommandToGetHelpOn,
  IN CONST CHAR16     *SectionToGetHelpOn,
  IN BOOLEAN          PrintCommandText
  );

#endif // __SHELL_LIB__
