/** @file
  Platform flash device access library.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __PLATFORM_FLASH_ACCESS_LIB_H__
#define __PLATFORM_FLASH_ACCESS_LIB_H__

typedef enum {
  FlashAddressTypeRelativeAddress,
  FlashAddressTypeAbsoluteAddress,
} FLASH_ADDRESS_TYPE;

//
// Type 0 ~ 0x7FFFFFFF is defined in this library.
// Type 0x80000000 ~ 0xFFFFFFFF is reserved for OEM.
//
typedef enum {
  PlatformFirmwareTypeSystemFirmware,
  PlatformFirmwareTypeNvRam,
} PLATFORM_FIRMWARE_TYPE;

/**
  Perform flash write opreation.

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWrite (
  IN PLATFORM_FIRMWARE_TYPE       FirmwareType,
  IN EFI_PHYSICAL_ADDRESS         FlashAddress,
  IN FLASH_ADDRESS_TYPE           FlashAddressType,
  IN VOID                         *Buffer,
  IN UINTN                        Length
  );

#endif
