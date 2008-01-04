/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PxeBcMtftp.h

Abstract:

  Mtftp routines for PxeBc


**/

#ifndef __EFI_PXEBC_MTFTP_H__
#define __EFI_PXEBC_MTFTP_H__

enum {
  PXE_MTFTP_OPTION_BLKSIZE_INDEX,
  PXE_MTFTP_OPTION_TIMEOUT_INDEX,
  PXE_MTFTP_OPTION_TSIZE_INDEX,
  PXE_MTFTP_OPTION_MULTICAST_INDEX,
  PXE_MTFTP_OPTION_MAXIMUM_INDEX
};


/**
  This function is to get size of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferSize     Pointer to buffer size

  @return EFI_SUCCESS
  @return EFI_NOT_FOUND
  @return EFI_DEVICE_ERROR

**/
EFI_STATUS
PxeBcTftpGetFileSize (
  IN PXEBC_PRIVATE_DATA         *Private,
  IN EFI_MTFTP4_CONFIG_DATA     *Config,
  IN UINT8                      *Filename,
  IN UINTN                      *BlockSize,
  IN OUT UINT64                 *BufferSize
  )
;


/**
  This function is to get data of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size
  @param  DontUseBuffer  Indicate whether with a receive buffer

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR

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
  )
;


/**
  This function is put data of a file by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  Overwrite      Indicate whether with overwrite attribute
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR

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
  )
;


/**
  This function is to get data of a directory by Tftp.

  @param  Private        Pointer to PxeBc private data
  @param  Config         Pointer to Mtftp configuration data
  @param  Filename       Pointer to file name
  @param  BlockSize      Pointer to block size
  @param  BufferPtr      Pointer to buffer
  @param  BufferSize     Pointer to buffer size
  @param  DontUseBuffer  Indicate whether with a receive buffer

  @return EFI_SUCCES
  @return EFI_DEVICE_ERROR

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
  )
;

#endif

