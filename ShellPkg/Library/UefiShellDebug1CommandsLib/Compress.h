/** @file
  Header file for compression routine.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_SHELL_COMPRESS_H_
#define _EFI_SHELL_COMPRESS_H_

/**
  The compression routine.

  @param[in]       SrcBuffer     The buffer containing the source data.
  @param[in]       SrcSize       Number of bytes in SrcBuffer.
  @param[in]       DstBuffer     The buffer to put the compressed image in.
  @param[in, out]  DstSize       On input the size (in bytes) of DstBuffer, on
                                 return the number of bytes placed in DstBuffer.

  @retval EFI_SUCCESS           The compression was sucessful.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.  DstSize is required.
**/
EFI_STATUS
EFIAPI
Compress (
  IN      VOID    *SrcBuffer,
  IN      UINT64  SrcSize,
  IN      VOID    *DstBuffer,
  IN OUT  UINT64  *DstSize
  );

#endif

