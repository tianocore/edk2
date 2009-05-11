/** @file
  Provides interface to shell functionality for shell commands and applications.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if !defined(__SHELL_LIB__)
#define __SHELL_LIB__

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/EfiShell.h>

/**
  This function will retrieve the information about the file for the handle 
  specified and store it in allocated pool memory.

  This function allocates a buffer to store the file’s information. It is the 
  caller’s responsibility to free the buffer

  @param  FileHandle  The file handle of the file for which information is 
                      being requested.

  @retval NULL information could not be retrieved.

  @return the information about the file
**/
EFI_FILE_INFO*
EFIAPI
ShellGetFileInfo (
  IN EFI_FILE_HANDLE            FileHandle
  );

/**
  This function will set the information about the file for the opened handle 
  specified.

  @param  FileHandle            The file handle of the file for which information 
                                is being set

  @param  FileInfo              The infotmation to set.

  @retval EFI_SUCCESS		        The information was set.
  @retval EFI_UNSUPPORTED       The InformationType is not known.
  @retval EFI_NO_MEDIA		      The device has no medium.
  @retval EFI_DEVICE_ERROR	    The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED   The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	    The file was opened read only.
  @retval EFI_VOLUME_FULL	      The volume is full.
**/
EFI_STATUS
EFIAPI
ShellSetFileInfo (
  IN EFI_FILE_HANDLE  	        FileHandle,
  IN EFI_FILE_INFO              *FileInfo
  );

/**
  This function will open a file or directory referenced by DevicePath.

  This function opens a file with the open mode according to the file path. The 
  Attributes is valid only for EFI_FILE_MODE_CREATE.

  @param  FilePath 		    on input the device path to the file.  On output 
                          the remaining device path.
  @param  DeviceHandle  	pointer to the system device handle.
  @param  FileHandle		  pointer to the file handle.
  @param  OpenMode	    	the mode to open the file with.
  @param  Attributes	  	the file's file attributes.

  @retval EFI_SUCCESS		        The information was set.
  @retval EFI_INVALID_PARAMETER	One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED	      Could not open the file path.	
  @retval EFI_NOT_FOUND	        The specified file could not be found on the 
                                device or the file system could not be found on 
                                the device.
  @retval EFI_NO_MEDIA		      The device has no medium.
  @retval EFI_MEDIA_CHANGED	    The device has a different medium in it or the 
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR	    The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	  The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	    The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES	Not enough resources were available to open the 
                                file.
  @retval EFI_VOLUME_FULL	      The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByDevicePath(
  IN OUT EFI_DEVICE_PATH_PROTOCOL  	  **FilePath,
  OUT EFI_HANDLE                    	*DeviceHandle,
  OUT EFI_FILE_HANDLE               	*FileHandle,
  IN UINT64                          	OpenMode,
  IN UINT64                          	Attributes
  );

/**
  This function will open a file or directory referenced by filename.

  If return is EFI_SUCCESS, the Filehandle is the opened file’s handle; 
  otherwise, the Filehandle is NULL. The Attributes is valid only for 
  EFI_FILE_MODE_CREATE.

  @param  FileName 		  pointer to file name
  @param  FileHandle		pointer to the file handle.
  @param  OpenMode		  the mode to open the file with.
  @param  Attributes		the file's file attributes.

  @retval EFI_SUCCESS		The information was set.
  @retval EFI_INVALID_PARAMETER	One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED	Could not open the file path.	
  @retval EFI_NOT_FOUND	        The specified file could not be found on the 
                                device or the file system could not be found 
                                on the device.
  @retval EFI_NO_MEDIA		The device has no medium.
  @retval EFI_MEDIA_CHANGED	The device has a different medium in it or the 
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES	Not enough resources were available to open the 
                                file.
  @retval EFI_VOLUME_FULL	The volume is full.
**/
EFI_STATUS
EFIAPI
ShellOpenFileByName(
  IN CHAR16		                  *FilePath,
  OUT EFI_FILE_HANDLE           *FileHandle,
  IN UINT64                     OpenMode,
  IN UINT64                    	Attributes
  );

/**
  This function create a directory

  If return is EFI_SUCCESS, the Filehandle is the opened directory's handle; 
  otherwise, the Filehandle is NULL. If the directory already existed, this 
  function opens the existing directory.

  @param  DirectoryName         pointer to Directory name
  @param  FileHandle		        pointer to the file handle.

  @retval EFI_SUCCESS		        The information was set.
  @retval EFI_INVALID_PARAMETER	One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED	Could not open the file path.	
  @retval EFI_NOT_FOUND	        The specified file could not be found on the 
                                device or the file system could not be found 
                                on the device.
  @retval EFI_NO_MEDIA		The device has no medium.
  @retval EFI_MEDIA_CHANGED	The device has a different medium in it or the 
                                medium is no longer supported.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	The file was opened read only.
  @retval EFI_OUT_OF_RESOURCES	Not enough resources were available to open the 
                                file.
  @retval EFI_VOLUME_FULL	The volume is full.
**/
EFI_STATUS
EFIAPI
ShellCreateDirectory(
  IN CHAR16                   *DirectoryName,
  OUT EFI_FILE_HANDLE         *FileHandle
  );

/**
  This function reads information from an opened file.

  If FileHandle is not a directory, the function reads the requested number of 
  bytes from the file at the file’s current position and returns them in Buffer. 
  If the read goes beyond the end of the file, the read length is truncated to the
  end of the file. The file’s current position is increased by the number of bytes 
  returned.  If FileHandle is a directory, the function reads the directory entry 
  at the file’s current position and returns the entry in Buffer. If the Buffer 
  is not large enough to hold the current directory entry, then 
  EFI_BUFFER_TOO_SMALL is returned and the current file position is not updated. 
  BufferSize is set to be the size of the buffer needed to read the entry. On 
  success, the current position is updated to the next directory entry. If there 
  are no more directory entries, the read returns a zero-length buffer. 
  EFI_FILE_INFO is the structure returned as the directory entry.

  @param FileHandle             the opened file handle
  
  @param ReadSize               on input the size of buffer in bytes.  on return 
                                the number of bytes written.

  @param Buffer                 the buffer to put read data into.

 @retval EFI_SUCCESS	        Data was read.
 @retval EFI_NO_MEDIA	        The device has no media.
 @retval EFI_DEVICE_ERROR	The device reported an error.
 @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
 @retval EFI_BUFFER_TO_SMALL	Buffer is too small. ReadSize contains required 
                                size.

**/
EFI_STATUS
EFIAPI
ShellReadFile(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *ReadSize,
  OUT VOID                      *Buffer
  );

/**
  Write data to a file.

  This function writes the specified number of bytes to the file at the current 
  file position. The current file position is advanced the actual number of bytes 
  written, which is returned in BufferSize. Partial writes only occur when there 
  has been a data error during the write attempt (such as “volume space full”). 
  The file is automatically grown to hold the data if required. Direct writes to 
  opened directories are not supported.

  @param FileHandle             The opened file for writing

  @param BufferSize             on input the number of bytes in Buffer.  On output
                                the number of bytes written.

  @param Buffer                 the buffer containing data to write is stored.

 @retval EFI_SUCCESS	        Data was written.
 @retval EFI_UNSUPPORTED	Writes to an open directory are not supported.
 @retval EFI_NO_MEDIA	        The device has no media.
 @retval EFI_DEVICE_ERROR	The device reported an error.
 @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
 @retval EFI_WRITE_PROTECTED	The device is write-protected.
 @retval EFI_ACCESS_DENIED	The file was open for read only.
 @retval EFI_VOLUME_FULL	The volume is full.
**/
EFI_STATUS
EFIAPI
ShellWriteFile(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
  IN CONST VOID                 *Buffer
  );

/** 
  Close an open file handle.

  This function closes a specified file handle. All “dirty” cached file data is 
  flushed to the device, and the file is closed. In all cases the handle is 
  closed.

@param FileHandle               the file handle to close.

@retval EFI_SUCCESS             the file handle was closed sucessfully.
@retval INVALID_PARAMETER      	One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellCloseFile (
  IN EFI_FILE_HANDLE            *FileHandle
  );

/**
  Delete a file and close the handle

  This function closes and deletes a file. In all cases the file handle is closed.
  If the file cannot be deleted, the warning code EFI_WARN_DELETE_FAILURE is 
  returned, but the handle is still closed.

  @param FileHandle             the file handle to delete

  @retval EFI_SUCCESS           the file was closed sucessfully
  @retval EFI_WARN_DELETE_FAILURE the handle was closed, but the file was not 
                                deleted
  @retval INVALID_PARAMETER    	One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellDeleteFile (
  IN EFI_FILE_HANDLE		*FileHandle
  );

/**
  Set the current position in a file.

  This function sets the current file position for the handle to the position 
  supplied. With the exception of seeking to position 0xFFFFFFFFFFFFFFFF, only
  absolute positioning is supported, and seeking past the end of the file is 
  allowed (a subsequent write would grow the file). Seeking to position 
  0xFFFFFFFFFFFFFFFF causes the current position to be set to the end of the file.
  If FileHandle is a directory, the only position that may be set is zero. This 
  has the effect of starting the read process of the directory entries over.

  @param FileHandle             The file handle on which the position is being set

  @param Position               Byte position from begining of file

  @retval EFI_SUCCESS           Operation completed sucessfully.
  @retval EFI_UNSUPPORTED       the seek request for non-zero is not valid on 
                                directories.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
**/
EFI_STATUS
EFIAPI
ShellSetFilePosition (
  IN EFI_FILE_HANDLE   	FileHandle,
  IN UINT64           	Position
  );

/** 
  Gets a file's current position

  This function retrieves the current file position for the file handle. For 
  directories, the current file position has no meaning outside of the file 
  system driver and as such the operation is not supported. An error is returned
  if FileHandle is a directory.

  @param FileHandle             The open file handle on which to get the position.
  @param Position               Byte position from begining of file.

  @retval EFI_SUCCESS           the operation completed sucessfully.
  @retval INVALID_PARAMETER     One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       the request is not valid on directories.
**/
EFI_STATUS
EFIAPI
ShellGetFilePosition (
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Position
  );

/**
  Flushes data on a file
  
  This function flushes all modified data associated with a file to a device.

  @param FileHandle             The file handle on which to flush data

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
  IN EFI_FILE_HANDLE            FileHandle
  );

/**
  Retrieves the first file from a directory

  This function takes an open directory handle and gets the first file 
  in the directory's info. Caller can use ShellFindNextFile() to get 
  subsequent files.

  Caller must use FreePool on *Buffer opon completion of all file searching.

  @param DirHandle              The file handle of the directory to search
  @param Buffer                 Pointer to pointer to buffer for file's information

  @retval EFI_SUCCESS           Found the first file.
  @retval EFI_NOT_FOUND         Cannot find the directory.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @return ShellReadFile
**/
EFI_STATUS
EFIAPI
ShellFindFirstFile (
  IN EFI_FILE_HANDLE            DirHandle,
  OUT EFI_FILE_INFO             **Buffer
  );

/**
  Retrieves the next file in a directory.

  To use this function, caller must call the ShellFindFirstFile() to get the 
  first file, and then use this function get other files. This function can be 
  called for several times to get each file's information in the directory. If 
  the call of ShellFindNextFile() got the last file in the directory, the next 
  call of this function has no file to get. *NoFile will be set to TRUE and the 
  data in Buffer is meaningless. 

  @param DirHandle              the file handle of the directory
  @param Buffer			            pointer to buffer for file's information
  @param NoFile			            pointer to boolean when last file is found

  @retval EFI_SUCCESS           Found the next file.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
**/
EFI_STATUS
EFIAPI
ShellFindNextFile(
  IN EFI_FILE_HANDLE             DirHandle,
  OUT EFI_FILE_INFO              *Buffer,
  OUT BOOLEAN                    *NoFile
  );

/**
  Retrieve the size of a file.

  This function extracts the file size info from the FileHandle’s EFI_FILE_INFO 
  data.

  @param FileHandle             file handle from which size is retrieved
  @param Size                   pointer to size

  @retval EFI_SUCCESS           operation was completed sucessfully
  @retval EFI_DEVICE_ERROR      cannot access the file
**/
EFI_STATUS
EFIAPI
ShellGetFileSize (
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Size
  );

/**
  Retrieves the status of the break execution flag

  this function is useful to check whether the application is being asked to halt by the shell.

  @retval TRUE                  the execution break is enabled
  @retval FALSE                 the execution break is not enabled
**/
BOOLEAN
EFIAPI
ShellGetExecutionBreakFlag(
  VOID
  );

/**
  return the value of an environment variable

  this function gets the value of the environment variable set by the 
  ShellSetEnvironmentVariable function

  @param EnvKey                 The key name of the environment variable.

  @retval NULL                  the named environment variable does not exist.
  @return != NULL               pointer to the value of the environment variable
**/
CONST CHAR16*
EFIAPI
ShellGetEnvironmentVariable (
  IN CHAR16                     *EnvKey
  );

/**
  set the value of an environment variable

This function changes the current value of the specified environment variable. If the
environment variable exists and the Value is an empty string, then the environment
variable is deleted. If the environment variable exists and the Value is not an empty
string, then the value of the environment variable is changed. If the environment
variable does not exist and the Value is an empty string, there is no action. If the
environment variable does not exist and the Value is a non-empty string, then the
environment variable is created and assigned the specified value.

  This is not supported pre-UEFI Shell 2.0.

  @param EnvKey                 The key name of the environment variable.
  @param EnvVal                 The Value of the environment variable
  @param Volatile               Indicates whether the variable is non-volatile (FALSE) or volatile (TRUE).

  @retval EFI_SUCCESS           the operation was completed sucessfully
  @retval EFI_UNSUPPORTED       This operation is not allowed in pre UEFI 2.0 Shell environments
**/
EFI_STATUS
EFIAPI
ShellSetEnvironmentVariable (
  IN CONST CHAR16               *EnvKey,
  IN CONST CHAR16               *EnvVal,
  IN BOOLEAN                    Volatile
  );

/**
  cause the shell to parse and execute a command line.

  This function creates a nested instance of the shell and executes the specified
command (CommandLine) with the specified environment (Environment). Upon return,
the status code returned by the specified command is placed in StatusCode.
If Environment is NULL, then the current environment is used and all changes made
by the commands executed will be reflected in the current environment. If the
Environment is non-NULL, then the changes made will be discarded.
The CommandLine is executed from the current working directory on the current
device.

EnvironmentVariables and Status are only supported for UEFI Shell 2.0.
Output is only supported for pre-UEFI Shell 2.0

  @param ImageHandle            Parent image that is starting the operation
  @param CommandLine            pointer to null terminated command line.
  @param Output                 true to display debug output.  false to hide it.
  @param EnvironmentVariables   optional pointer to array of environment variables
                                in the form "x=y".  if NULL current set is used.
  @param Status                 the status of the run command line.

  @retval EFI_SUCCESS           the operation completed sucessfully.  Status
                                contains the status code returned.
  @retval EFI_INVALID_PARAMETER a parameter contains an invalid value
  @retval EFI_OUT_OF_RESOURCES  out of resources
  @retval EFI_UNSUPPORTED       the operation is not allowed.
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
  Retreives the current directory path

  If the DeviceName is NULL, it returns the current device’s current directory 
  name. If the DeviceName is not NULL, it returns the current directory name 
  on specified drive.

  @param DeviceName             the name of the drive to get directory on

  @retval NULL                  the directory does not exist
  @return != NULL               the directory
**/
CONST CHAR16*
EFIAPI
ShellGetCurrentDir (
  IN CHAR16                     *DeviceName OPTIONAL
  );

/**
  sets (enabled or disabled) the page break mode

  when page break mode is enabled the screen will stop scrolling 
  and wait for operator input before scrolling a subsequent screen.

  @param CurrentState           TRUE to enable and FALSE to disable
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
  and will process '?' and '*' as such.  the list must be freed with a call to 
  ShellCloseFileMetaArg().

  This function will fail if called sequentially without freeing the list in the middle.

  @param Arg                    pointer to path string
  @param OpenMode               mode to open files with
  @param ListHead               head of linked list of results

  @retval EFI_SUCCESS           the operation was sucessful and the list head 
                                contains the list of opened files
  #retval EFI_UNSUPPORTED       a previous ShellOpenFileMetaArg must be closed first.
                                *ListHead is set to NULL.
  @return != EFI_SUCCESS        the operation failed

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
  Free the linked list returned from ShellOpenFileMetaArg

  @param ListHead               the pointer to free

  @retval EFI_SUCCESS           the operation was sucessful
  @retval EFI_INVALID_PARAMETER A parameter was invalid
**/
EFI_STATUS
EFIAPI
ShellCloseFileMetaArg (
  IN OUT EFI_SHELL_FILE_INFO    **ListHead
  );

typedef enum {
  TypeFlag  = 0,
  TypeValue,
  TypePosition,
  TypeMax,
} ParamType;

typedef struct {
  CHAR16      *Name;
  ParamType   Type;
} SHELL_PARAM_ITEM;


/// Helper structure for no parameters (besides -? and -b)
extern SHELL_PARAM_ITEM EmptyParamList[];

/**
  Checks the command line arguments passed against the list of valid ones.  
  Optionally removes NULL values first.
  
  If no initialization is required, then return RETURN_SUCCESS.
  
  @param CheckList              pointer to list of parameters to check
  @param CheckPackage           Package of checked values
  @param ProblemParam           optional pointer to pointer to unicode string for 
                                the paramater that caused failure.
  @param AutoPageBreak          will automatically set PageBreakEnabled

  @retval EFI_SUCCESS           The operation completed sucessfully.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed
  @retval EFI_INVALID_PARAMETER A parameter was invalid
  @retval EFI_VOLUME_CORRUPTED  the command line was corrupt.  an argument was 
                                duplicated.  the duplicated command line argument 
                                was returned in ProblemParam if provided.
  @retval EFI_DEVICE_ERROR      the commands contained 2 opposing arguments.  one
                                of the command line arguments was returned in 
                                ProblemParam if provided.
  @retval EFI_NOT_FOUND         a argument required a value that was missing.  
                                the invalid command line argument was returned in
                                ProblemParam if provided.
**/
EFI_STATUS
EFIAPI
ShellCommandLineParse (
  IN CONST SHELL_PARAM_ITEM     *CheckList,
  OUT LIST_ENTRY                **CheckPackage,
  OUT CHAR16                    **ProblemParam OPTIONAL,
  IN BOOLEAN                    AutoPageBreak
  );

/**
  Frees shell variable list that was returned from ShellCommandLineParse.

  This function will free all the memory that was used for the CheckPackage
  list of postprocessed shell arguments.

  this function has no return value.

  if CheckPackage is NULL, then return

  @param CheckPackage           the list to de-allocate
  **/
VOID
EFIAPI
ShellCommandLineFreeVarList (
  IN LIST_ENTRY                 *CheckPackage
  );

/**
  Checks for presence of a flag parameter

  flag arguments are in the form of "-<Key>" or "/<Key>", but do not have a value following the key

  if CheckPackage is NULL then return FALSE.
  if KeyString is NULL then ASSERT()
  
  @param CheckPackage           The package of parsed command line arguments
  @param KeyString              the Key of the command line argument to check for

  @retval TRUE                  the flag is on the command line
  @retval FALSE                 the flag is not on the command line
  **/
BOOLEAN
EFIAPI
ShellCommandLineGetFlag (
  IN CONST LIST_ENTRY              *CheckPackage,
  IN CHAR16                        *KeyString
  );

/**
  returns value from command line argument

  value parameters are in the form of "-<Key> value" or "/<Key> value"
  
  if CheckPackage is NULL, then return NULL;

  @param CheckPackage           The package of parsed command line arguments
  @param KeyString              the Key of the command line argument to check for

  @retval NULL                  the flag is not on the command line
  @return !=NULL                pointer to unicode string of the value
  **/
CONST CHAR16*
EFIAPI
ShellCommandLineGetValue (
  IN CONST LIST_ENTRY              *CheckPackage,
  IN CHAR16                        *KeyString
  );

/**
  returns raw value from command line argument

  raw value parameters are in the form of "value" in a specific position in the list
  
  if CheckPackage is NULL, then return NULL;

  @param CheckPackage           The package of parsed command line arguments
  @param Position               the position of the value 

  @retval NULL                  the flag is not on the command line
  @return !=NULL                pointer to unicode string of the value
  **/
CONST CHAR16*
EFIAPI
ShellCommandLineGetRawValue (
  IN CONST LIST_ENTRY              *CheckPackage,
  IN UINT32                        Position
  );

/**
  This function causes the shell library to initialize itself.  If the shell library
  is already initialized it will de-initialize all the current protocol poitners and
  re-populate them again.

  When the library is used with PcdShellLibAutoInitialize set to true this function
  will return EFI_SUCCESS and perform no actions.

  This function is intended for internal access for shell commands only.

  @retval EFI_SUCCESS   the initialization was complete sucessfully

**/
EFI_STATUS
EFIAPI
ShellInitialize (
  );

#endif // __SHELL_LIB__