/** @file
  Microcode flash device access library NULL instance.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MicrocodeFlashAccessLib.h>

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
  )
{
  CopyMem((VOID *)(UINTN)(FlashAddress), Buffer, Length);
  return EFI_SUCCESS;
}
