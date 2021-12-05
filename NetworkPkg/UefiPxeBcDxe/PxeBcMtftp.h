/** @file
  Functions declaration related with Mtftp for UefiPxeBc Driver.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_PXEBC_MTFTP_H__
#define __EFI_PXEBC_MTFTP_H__

#define PXE_MTFTP_OPTION_BLKSIZE_INDEX     0
#define PXE_MTFTP_OPTION_TIMEOUT_INDEX     1
#define PXE_MTFTP_OPTION_TSIZE_INDEX       2
#define PXE_MTFTP_OPTION_MULTICAST_INDEX   3
#define PXE_MTFTP_OPTION_WINDOWSIZE_INDEX  4
#define PXE_MTFTP_OPTION_MAXIMUM_INDEX     5
#define PXE_MTFTP_OPTBUF_MAXNUM_INDEX      128

#define PXE_MTFTP_ERROR_STRING_LENGTH  127       // refer to definition of struct EFI_PXE_BASE_CODE_TFTP_ERROR.
#define PXE_MTFTP_DEFAULT_BLOCK_SIZE   512       // refer to rfc-1350.

/**
  This function is wrapper to get the file size using TFTP.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to configure data.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in]      WindowSize     Pointer to required window size.
  @param[in, out] BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully obtained the size of file.
  @retval EFI_NOT_FOUND      Parse the tftp options failed.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Did not obtain the size of the file.

**/
EFI_STATUS
PxeBcTftpGetFileSize (
  IN     PXEBC_PRIVATE_DATA  *Private,
  IN     VOID                *Config,
  IN     UINT8               *Filename,
  IN     UINTN               *BlockSize,
  IN     UINTN               *WindowSize,
  IN OUT UINT64              *BufferSize
  );

/**
  This function is a wrapper to get a file using TFTP.

  @param[in]      Private        Pointer to PxeBc private data.
  @param[in]      Config         Pointer to config data.
  @param[in]      Filename       Pointer to boot file name.
  @param[in]      BlockSize      Pointer to required block size.
  @param[in]      WindowSize     Pointer to required window size.
  @param[in]      BufferPtr      Pointer to buffer.
  @param[in, out] BufferSize     Pointer to buffer size.
  @param[in]      DontUseBuffer  Indicates whether to use a receive buffer.

  @retval EFI_SUCCESS        Successfully read the data from the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Read data from file failed.

**/
EFI_STATUS
PxeBcTftpReadFile (
  IN     PXEBC_PRIVATE_DATA  *Private,
  IN     VOID                *Config,
  IN     UINT8               *Filename,
  IN     UINTN               *BlockSize,
  IN     UINTN               *WindowSize,
  IN     UINT8               *BufferPtr,
  IN OUT UINT64              *BufferSize,
  IN     BOOLEAN             DontUseBuffer
  );

/**
  This function is a wrapper to put file with TFTP.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to config data.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       Overwrite      Indicates whether to use an overwrite attribute.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.

  @retval EFI_SUCCESS        Successfully wrote the data into the special file.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval other              Write data into file failed.

**/
EFI_STATUS
PxeBcTftpWriteFile (
  IN     PXEBC_PRIVATE_DATA  *Private,
  IN     VOID                *Config,
  IN     UINT8               *Filename,
  IN     BOOLEAN             Overwrite,
  IN     UINTN               *BlockSize,
  IN     UINT8               *BufferPtr,
  IN OUT UINT64              *BufferSize
  );

/**
  This function is a wrapper to get the data (file) from a directory using TFTP.

  @param[in]       Private        Pointer to PxeBc private data.
  @param[in]       Config         Pointer to config data.
  @param[in]       Filename       Pointer to boot file name.
  @param[in]       BlockSize      Pointer to required block size.
  @param[in]       WindowSize     Pointer to required window size.
  @param[in]       BufferPtr      Pointer to buffer.
  @param[in, out]  BufferSize     Pointer to buffer size.
  @param[in]       DontUseBuffer  Indicates whether with a receive buffer.

  @retval EFI_SUCCESS        Successfully obtained the data from the file included in directory.
  @retval EFI_DEVICE_ERROR   The network device encountered an error during this operation.
  @retval Others             Operation failed.

**/
EFI_STATUS
PxeBcTftpReadDirectory (
  IN     PXEBC_PRIVATE_DATA  *Private,
  IN     VOID                *Config,
  IN     UINT8               *Filename,
  IN     UINTN               *BlockSize,
  IN     UINTN               *WindowSize,
  IN     UINT8               *BufferPtr,
  IN OUT UINT64              *BufferSize,
  IN     BOOLEAN             DontUseBuffer
  );

#endif
