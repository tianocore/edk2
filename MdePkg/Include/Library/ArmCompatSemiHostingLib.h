/** @file
  Arm Compatible Semi Hosting library Header file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011, 2012, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_COMPAT_SEMIHOSTING_LIB_H_
#define ARM_COMPAT_SEMIHOSTING_LIB_H_

/*
 *  Please refer to https://github.com/ARM-software/abi-aa/blob/main/semihosting/semihosting.rst
 *  for more information about the semihosting interface.
 */

#define SEMIHOSTING_FILE_MODE_READ    (0 << 2)
#define SEMIHOSTING_FILE_MODE_WRITE   (1 << 2)
#define SEMIHOSTING_FILE_MODE_APPEND  (2 << 2)
#define SEMIHOSTING_FILE_MODE_UPDATE  (1 << 1)
#define SEMIHOSTING_FILE_MODE_BINARY  (1 << 0)
#define SEMIHOSTING_FILE_MODE_ASCII   (0 << 0)

/**
  Check if semihosting connection is supported.

  This function determines whether a semihosting connection is currently
  available and operational.

  @retval TRUE   Semihosting connection is supported and available.
  @retval FALSE  Semihosting is not supported or connection not established.
**/
BOOLEAN
EFIAPI
SemiHostingConnectionSupported (
  VOID
  );

/**
  Open a file on the host system using semihosting interface.

  @param[in]  FileName   Null-terminated ASCII string of the file name.
  @param[in]  Mode       Access mode (read, write, append, etc.).
  @param[out] FileHandle Returned file handle identifier if successful.

  @retval  RETURN_SUCCESS            File opened successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter passed.
  @retval  RETURN_NOT_FOUND          File could not be opened.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileOpen (
  IN  CHAR8   *FileName,
  IN  UINT32  Mode,
  OUT UINTN   *FileHandle
  );

/**
  Move the file position to a specified offset.

  @param[in]  FileHandle  Handle of the open file.
  @param[in]  Offset      Offset (in bytes) to seek to, relative to start of file.

  @retval  RETURN_SUCCESS            File pointer repositioned successfully.
  @retval  RETURN_ABORTED            Seek operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileSeek (
  IN UINTN  FileHandle,
  IN UINTN  Offset
  );

/**
  Read data from a file on the host system via semihosting.

  @param[in]      FileHandle  Handle of the open file.
  @param[in, out] Length      On input, size of Buffer in bytes.
                              On output, number of bytes actually read.
  @param[out]     Buffer      Buffer to store the read data.

  @retval  RETURN_SUCCESS            Data read successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid buffer or handle.
  @retval  RETURN_ABORTED            Read failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileRead (
  IN     UINTN  FileHandle,
  IN OUT UINTN  *Length,
  OUT    VOID   *Buffer
  );

/**
  Write data to a file on the host system via semihosting.

  @param[in]      FileHandle  Handle of the open file.
  @param[in, out] Length      On input, number of bytes to write.
                              On output, number of bytes actually written.
  @param[in]      Buffer      Pointer to data buffer to write.

  @retval  RETURN_SUCCESS            Data written successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid handle or buffer.
  @retval  RETURN_ABORTED            Write operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileWrite (
  IN     UINTN  FileHandle,
  IN OUT UINTN  *Length,
  IN     VOID   *Buffer
  );

/**
  Close an open file on the host system via semihosting.

  @param[in]  FileHandle  Handle of the file to close.

  @retval  RETURN_SUCCESS            File closed successfully.
  @retval  RETURN_ABORTED            Close operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileClose (
  IN UINTN  FileHandle
  );

/**
  Get the total length of a file.

  @param[in]   FileHandle  Handle of the open file.
  @param[out]  Length      Pointer to store the file length in bytes.

  @retval  RETURN_SUCCESS            File length retrieved successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid length pointer.
  @retval  RETURN_ABORTED            Operation failed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileLength (
  IN  UINTN  FileHandle,
  OUT UINTN  *Length
  );

/**
  Get a temporary name for a file from the host running the debug agent.

  @param[out]  Buffer      Pointer to the buffer where the temporary name has to
                           be stored
  @param[in]   Identifier  File name identifier (integer in the range 0 to 255)
  @param[in]   Length      Length of the buffer to store the temporary name

  @retval  RETURN_SUCCESS            Temporary name returned
  @retval  RETURN_INVALID_PARAMETER  Invalid buffer address
  @retval  RETURN_ABORTED            Temporary name not returned

**/
RETURN_STATUS
EFIAPI
SemiHostingFileTmpName (
  OUT  VOID   *Buffer,
  IN   UINT8  Identifier,
  IN   UINTN  Length
  );

/**
  Delete a specified file on the host via semihosting.

  @param[in]  FileName  Null-terminated ASCII string of the file name to remove.

  @retval  RETURN_SUCCESS            File removed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid file name.
  @retval  RETURN_ABORTED            File could not be removed.
**/
RETURN_STATUS
EFIAPI
SemiHostingFileRemove (
  IN CHAR8  *FileName
  );

/**
  Rename a specified file.

  @param[in]  FileName     Name of the file to rename.
  @param[in]  NewFileName  The new name of the file.

  @retval  RETURN_SUCCESS            File Renamed
  @retval  RETURN_INVALID_PARAMETER  Either the current or the new name is not specified
  @retval  RETURN_ABORTED            Rename failed

**/
RETURN_STATUS
EFIAPI
SemiHostingFileRename (
  IN  CHAR8  *FileName,
  IN  CHAR8  *NewFileName
  );

/**
  Read a single character from the host input via semihosting.

  @retval  The ASCII value of the character read.
**/
CHAR8
EFIAPI
SemiHostingReadCharacter (
  VOID
  );

/**
  Write a single character to the host output via semihosting.

  @param[in]  Character  ASCII character to write.

**/
VOID
EFIAPI
SemiHostingWriteCharacter (
  IN CHAR8  Character
  );

/**
  Write a null-terminated string to the host output via semihosting.

  @param[in]  String  Pointer to a null-terminated ASCII string.
**/
VOID
EFIAPI
SemiHostingWriteString (
  IN CHAR8  *String
  );

/**
  Execute a command line on the host system.

  @param[in]  CommandLine  Null-terminated ASCII string containing the command to execute.

  @retval  The return code from the host system for the executed command.
**/
UINT32
EFIAPI
SemiHostingSystem (
  IN CHAR8  *CommandLine
  );

#endif // ARM_COMPAT_SEMIHOSTING_LIB_H_
