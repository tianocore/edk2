/** @file
  Provides interface to EFI_FILE_HANDLE functionality.

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

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
FileHandleGetInfo (
  IN EFI_FILE_HANDLE            FileHandle
  );

/**
  This function will set the information about the file for the opened handle 
  specified.

  @param  FileHandle            The file handle of the file for which information 
  is being set

  @param  FileInfo              The infotmation to set.

  @retval EFI_SUCCESS		The information was set.
  @retval EFI_UNSUPPORTED The InformationType is not known.
  @retval EFI_NO_MEDIA		The device has no medium.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_WRITE_PROTECTED	The file or medium is write protected.
  @retval EFI_ACCESS_DENIED	The file was opened read only.
  @retval EFI_VOLUME_FULL	The volume is full.
**/
EFI_STATUS
EFIAPI
FileHandleSetInfo (
  IN EFI_FILE_HANDLE  	        FileHandle,
  IN CONST EFI_FILE_INFO        *FileInfo
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
  @param BufferSize             on input the size of buffer in bytes.  on return 
                                the number of bytes written.
  @param Buffer                 the buffer to put read data into.

  @retval EFI_SUCCESS	          Data was read.
  @retval EFI_NO_MEDIA	        The device has no media.
  @retval EFI_DEVICE_ERROR	The device reported an error.
  @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
  @retval EFI_BUFFER_TO_SMALL	Buffer is too small. ReadSize contains required 
                                size.

**/
EFI_STATUS
EFIAPI
FileHandleRead(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
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

  @param FileHandle           The opened file for writing
  @param BufferSize           on input the number of bytes in Buffer.  On output
                              the number of bytes written.
  @param Buffer               the buffer containing data to write is stored.

 @retval EFI_SUCCESS	        Data was written.
 @retval EFI_UNSUPPORTED	    Writes to an open directory are not supported.
 @retval EFI_NO_MEDIA	        The device has no media.
 @retval EFI_DEVICE_ERROR	    The device reported an error.
 @retval EFI_VOLUME_CORRUPTED	The file system structures are corrupted.
 @retval EFI_WRITE_PROTECTED	The device is write-protected.
 @retval EFI_ACCESS_DENIED	  The file was open for read only.
 @retval EFI_VOLUME_FULL	    The volume is full.
**/
EFI_STATUS
EFIAPI
FileHandleWrite(
  IN EFI_FILE_HANDLE            FileHandle,
  IN OUT UINTN                  *BufferSize,
  IN VOID                       *Buffer
  );

/** 
  Close an open file handle.

  This function closes a specified file handle. All “dirty” cached file data is 
  flushed to the device, and the file is closed. In all cases the handle is 
  closed.

@param FileHandle               the file handle to close.

@retval EFI_SUCCESS             the file handle was closed sucessfully.
**/
EFI_STATUS
EFIAPI
FileHandleClose (
  IN EFI_FILE_HANDLE            FileHandle
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
FileHandleDelete (
  IN EFI_FILE_HANDLE		FileHandle
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
FileHandleSetPosition (
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
FileHandleGetPosition (
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
FileHandleFlush (
  IN EFI_FILE_HANDLE            FileHandle
  );

/**
  function to determine if a given handle is a directory handle

  if DirHandle is NULL then ASSERT()

  open the file information on the DirHandle and verify that the Attribute
  includes EFI_FILE_DIRECTORY bit set.

  @param DirHandle              Handle to open file

  @retval EFI_SUCCESS           DirHandle is a directory
  @retval EFI_INVALID_PARAMETER DirHandle did not have EFI_FILE_INFO available
  @retval EFI_NOT_FOUND         DirHandle is not a directory
**/
EFI_STATUS
EFIAPI
FileHandleIsDirectory (
  IN EFI_FILE_HANDLE            DirHandle
  );

/**
  Retrieves the first file from a directory

  This function opens a directory and gets the first file’s info in the 
  directory. Caller can use FileHandleFindNextFile() to get other files.  When 
  complete the caller is responsible for calling FreePool() on *Buffer.

  @param DirHandle              The file handle of the directory to search
  @param Buffer                 Pointer to pointer to buffer for file's information

  @retval EFI_SUCCESS           Found the first file.
  @retval EFI_NOT_FOUND         Cannot find the directory.
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
  @return Others                status of FileHandleGetInfo, FileHandleSetPosition,
                                or FileHandleRead
**/
EFI_STATUS
EFIAPI
FileHandleFindFirstFile (
  IN EFI_FILE_HANDLE            DirHandle,
  OUT EFI_FILE_INFO             **Buffer
  );
/**
  Retrieves the next file in a directory.

  To use this function, caller must call the FileHandleFindFirstFile() to get the 
  first file, and then use this function get other files. This function can be 
  called for several times to get each file's information in the directory. If 
  the call of FileHandleFindNextFile() got the last file in the directory, the next 
  call of this function has no file to get. *NoFile will be set to TRUE and the 
  Buffer memory will be automatically freed. 

  @param DirHandle              the file handle of the directory
  @param Buffer			            pointer to buffer for file's information
  @param NoFile			            pointer to boolean when last file is found

  @retval EFI_SUCCESS           Found the next file, or reached last file
  @retval EFI_NO_MEDIA          The device has no media.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_VOLUME_CORRUPTED  The file system structures are corrupted.
**/
EFI_STATUS
EFIAPI
FileHandleFindNextFile(
  IN EFI_FILE_HANDLE             DirHandle,
  OUT EFI_FILE_INFO              *Buffer,
  OUT BOOLEAN                    *NoFile
  );

/**
  Retrieve the size of a file.

  if FileHandle is NULL then ASSERT()
  if Size is NULL then ASSERT()

  This function extracts the file size info from the FileHandle’s EFI_FILE_INFO 
  data.

  @param FileHandle             file handle from which size is retrieved
  @param Size                   pointer to size

  @retval EFI_SUCCESS           operation was completed sucessfully
  @retval EFI_DEVICE_ERROR      cannot access the file
**/
EFI_STATUS
EFIAPI
FileHandleGetSize (
  IN EFI_FILE_HANDLE            FileHandle,
  OUT UINT64                    *Size
  );