/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


  Flash device library class header file.

  Flash Device Library common type, MACRO and API definition. The basic idea for
  this library is to provide API to abstract the different between flash
  technology (SPI, FWH etc..), flash controller (SPI host controller on
  ICH, MMIO type access for FWH), flash chip (programming command, method
  of status checking). This library class can be consumed by drivers or applications
  such as Firmware Volume Block driver, Flash Update application. These driver
  can be written in a generic manner so that they are more easy to be
  ported to other platforms.

  This library can be build on a set of APIs which can touch flash controller, flash
  chip directly for a platform with simple flash device configuration.

  For a platform with complex flash device configuration, this library can be built
  on the Flash Device Operate Library. Please see the header file for that library
  class for detailed usage.

**/

#ifndef __FLASHDEVICE_LIB_H__
#define __FLASHDEVICE_LIB_H__

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
  IN      UINTN                           PAddress,
  IN  OUT UINTN                           *NumBytes,
      OUT UINT8                           *Buffer
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
  IN        UINTN                           PAddress,
  IN OUT    UINTN                           *NumBytes,
  IN        UINT8                           *Buffer
  );

/**
  Erase the block staring at PAddress.

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
  IN    UINTN                      PAddress,
  IN    UINTN                      LbaLength
);

/**
  Lock or unlock the block staring at PAddress.

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
  IN    UINTN                          PAddress,
  IN    UINTN                          LbaLength,
  IN    BOOLEAN                        Lock
);

#endif


