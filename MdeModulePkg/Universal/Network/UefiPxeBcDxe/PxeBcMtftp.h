/** @file
  Mtftp routines for PxeBc.
    
Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_PXEBC_MTFTP_H__
#define __EFI_PXEBC_MTFTP_H__

#define PXE_MTFTP_OPTION_BLKSIZE_INDEX   0
#define PXE_MTFTP_OPTION_TIMEOUT_INDEX   1
#define PXE_MTFTP_OPTION_TSIZE_INDEX     2
#define PXE_MTFTP_OPTION_MULTICAST_INDEX 3
#define PXE_MTFTP_OPTION_MAXIMUM_INDEX   4

#define PXE_MTFTP_ERROR_STRING_LENGTH    127


/**
  This function is to get size of a file by Tftp.
  
  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferSize     Pointer to buffer size

  @retval EFI_SUCCESS        Get the size of file success
  @retval EFI_NOT_FOUND      Parse the tftp ptions failed.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Other              Has not get the size of the file.
  
**/
EFI_STATUS
PxeBcTftpGetFileSize (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN UINTN                      *BlockSize,
  IN OUT UINT64                 *BufferSize
  );


/**
  This function is to get data of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size
  @param  DontUseBuffer  Indicate whether with a receive buffer

  @retval EFI_SUCCESS        Read the data success from the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Read data from file failed.
  
**/
EFI_STATUS
PxeBcTftpReadFile (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN UINTN                      *BlockSize,
  IN UINT8                      *BufferPtr,
  IN OUT UINT64                 *BufferSize,
  IN BOOLEAN                    DontUseBuffer
  );


/**
  This function is put data of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  Overwrite      Indicate whether with overwrite attribute
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size

  @retval EFI_SUCCESS        Write the data success into the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Write data into file failed.
  
**/
EFI_STATUS
PxeBcTftpWriteFile (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN BOOLEAN                    Overwrite,
  IN UINTN                      *BlockSize,
  IN UINT8                      *BufferPtr,
  IN OUT UINT64                 *BufferSize
  );


/**
  This function is to get data(file) from a directory(may be a server) by Tftp.

  @param  Private        Pointer to PxeBc private data.
  @param  Config         Pointer to Mtftp configuration data.
  @param  Filename       Pointer to file name.
  @param  BlockSize      Pointer to block size.
  @param  BufferPtr      Pointer to buffer.
  @param  BufferSize     Pointer to buffer size.
  @param  DontUseBuffer  Indicate whether with a receive buffer.

  @retval EFI_SUCCES         Get the data from the file included in directory success. 
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Operation failed.
  
**/
EFI_STATUS
PxeBcTftpReadDirectory (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_MTFTP4_CONFIG_DATA        *Config,
  IN UINT8                         *Filename,
  IN UINTN                         *BlockSize,
  IN UINT8                         *BufferPtr,
  IN OUT UINT64                    *BufferSize,
  IN BOOLEAN                       DontUseBuffer
  );

#endif

