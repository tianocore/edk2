/** @file
  Microcode flash device access library.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __MICROCODE_FLASH_ACCESS_LIB_H__
#define __MICROCODE_FLASH_ACCESS_LIB_H__

/**
  Perform microcode write opreation.

  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
MicrocodeFlashWrite (
  IN EFI_PHYSICAL_ADDRESS         FlashAddress,
  IN VOID                         *Buffer,
  IN UINTN                        Length
  );

#endif
