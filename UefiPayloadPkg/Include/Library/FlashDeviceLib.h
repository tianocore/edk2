/** @file
  Flash device library class header file.

  Copyright (c) 2017 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLASHDEVICE_LIB_H_
#define FLASHDEVICE_LIB_H_

/**
  Read NumBytes bytes of data from the address specified by
  PAddress into Buffer.

  @param[in]      PAddress    The starting physical address of the read.
  @param[in,out]  NumBytes    On input, the number of bytes to read. On output, the number
                              of bytes actually read.
  @param[out]     Buffer      The destination data buffer for the read.

  @retval EFI_SUCCESS.        Opertion is successful.
  @retval EFI_DEVICE_ERROR    If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceRead (
  IN      UINTN  PAddress,
  IN  OUT UINTN  *NumBytes,
  OUT UINT8      *Buffer
  );

/**
  Write NumBytes bytes of data from Buffer to the address specified by
  PAddresss.

  @param[in]      PAddress The starting physical address of the write.
  @param[in,out]  NumBytes On input, the number of bytes to write. On output,
                           the actual number of bytes written.
  @param[in]      Buffer   The source data buffer for the write.

  @retval EFI_SUCCESS.            Opertion is successful.
  @retval EFI_DEVICE_ERROR        If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceWrite (
  IN        UINTN  PAddress,
  IN OUT    UINTN  *NumBytes,
  IN        UINT8  *Buffer
  );

/**
  Erase the block starting at PAddress.

  @param[in]  PAddress The starting physical address of the region to be erased.
  @param[in]  LbaLength   The length of the region to be erased. This parameter is necessary
                       as the physical block size on a flash device could be different than
                       the logical block size of Firmware Volume Block protocol. Erase on
                       flash chip is always performed block by block. Therefore, the ERASE
                       operation to a logical block is converted a number of ERASE operation
                       (or a partial erase) on the hardware.

  @retval EFI_SUCCESS.            Opertion is successful.
  @retval EFI_DEVICE_ERROR        If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceBlockErase (
  IN    UINTN  PAddress,
  IN    UINTN  LbaLength
  );

/**
  Lock or unlock the block starting at PAddress.

  @param[in]  PAddress The starting physical address of region to be (un)locked.
  @param[in]  LbaLength   The length of the region to be (un)locked. This parameter is necessary
                       as the physical block size on a flash device could be different than
                       the logical block size of Firmware Volume Block protocol. (Un)Lock on
                       flash chip is always performed block by block. Therefore, the (Un)Lock
                       operation to a logical block is converted a number of (Un)Lock operation
                       (or a partial erase) on the hardware.
  @param[in]  Lock     TRUE to lock. FALSE to unlock.

  @retval EFI_SUCCESS. Opertion is successful.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceBlockLock (
  IN    UINTN    PAddress,
  IN    UINTN    LbaLength,
  IN    BOOLEAN  Lock
  );

PHYSICAL_ADDRESS
EFIAPI
LibFvbFlashDeviceMemoryMap (
  );

#endif
