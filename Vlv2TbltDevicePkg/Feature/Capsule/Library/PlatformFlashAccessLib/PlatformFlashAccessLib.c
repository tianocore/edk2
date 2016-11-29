/** @file
  Platform Flash Access library.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformFlashAccessLib.h>
#include <Library/FlashDeviceLib.h>
#include <Library/MemoryAllocationLib.h>

#define SECTOR_SIZE_64KB  0x10000      // Common 64kBytes sector size
#define ALINGED_SIZE  SECTOR_SIZE_64KB

STATIC EFI_PHYSICAL_ADDRESS     mInternalFdAddress;

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
  )
{
  EFI_STATUS          Status;

  DEBUG((DEBUG_INFO, "PerformFlashWrite - 0x%x(%x) - 0x%x\n", (UINTN)FlashAddress, (UINTN)FlashAddressType, Length));
  if (FlashAddressType == FlashAddressTypeRelativeAddress) {
    FlashAddress = FlashAddress + mInternalFdAddress;
  }

  DEBUG((DEBUG_INFO, "                  - 0x%x(%x) - 0x%x\n", (UINTN)FlashAddress, (UINTN)FlashAddressType, Length));
  LibFvbFlashDeviceBlockLock((UINTN)FlashAddress, Length, FALSE);

  //
  // Erase & Write
  //
  Status = LibFvbFlashDeviceBlockErase((UINTN)FlashAddress, Length);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    LibFvbFlashDeviceBlockLock((UINTN)FlashAddress, Length, TRUE);
    DEBUG((DEBUG_ERROR, "Flash Erase error\n"));
    return Status;
  }

  Status = LibFvbFlashDeviceWrite((UINTN)FlashAddress, &Length, Buffer);
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    LibFvbFlashDeviceBlockLock((UINTN)FlashAddress, Length, TRUE);
    DEBUG((DEBUG_ERROR, "Flash write error\n"));
    return Status;
  }

  LibFvbFlashDeviceBlockLock((UINTN)FlashAddress, Length, TRUE);

  return EFI_SUCCESS;
}

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
  EFI_PHYSICAL_ADDRESS         AlignedFlashAddress;
  VOID                         *AlignedBuffer;
  UINTN                        AlignedLength;
  UINTN                        OffsetHead;
  UINTN                        OffsetTail;
  EFI_STATUS                   Status;

  DEBUG((DEBUG_INFO, "MicrocodeFlashWrite - 0x%x - 0x%x\n", (UINTN)FlashAddress, Length));

  //
  // Need make buffer 64K aligned to support ERASE
  //
  // [Aligned]    FlashAddress    [Aligned]
  // |              |                     |
  // V              V                     V
  // +--------------+========+------------+
  // | OffsetHeader | Length | OffsetTail |
  // +--------------+========+------------+
  // ^
  // |<-----------AlignedLength----------->
  // |
  // AlignedFlashAddress
  //
  OffsetHead = FlashAddress & (ALINGED_SIZE - 1);
  OffsetTail = (FlashAddress + Length) & (ALINGED_SIZE - 1);
  if (OffsetTail != 0) {
    OffsetTail = ALINGED_SIZE - OffsetTail;
  }

  if ((OffsetHead != 0) || (OffsetTail != 0)) {
    AlignedFlashAddress = FlashAddress - OffsetHead;
    AlignedLength = Length + OffsetHead + OffsetTail;

    AlignedBuffer = AllocatePool(AlignedLength);
    if (AlignedBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Save original buffer
    //
    if (OffsetHead != 0) {
      CopyMem((UINT8 *)AlignedBuffer, (VOID *)(UINTN)AlignedFlashAddress, OffsetHead);
    }
    if (OffsetTail != 0) {
      CopyMem((UINT8 *)AlignedBuffer + OffsetHead + Length, (VOID *)(UINTN)(AlignedFlashAddress + OffsetHead + Length), OffsetTail);
    }
    //
    // Override new buffer
    //
    CopyMem((UINT8 *)AlignedBuffer + OffsetHead, Buffer, Length);
  } else {
    AlignedFlashAddress = FlashAddress;
    AlignedBuffer = Buffer;
    AlignedLength = Length;
  }

  Status = PerformFlashWrite(
             PlatformFirmwareTypeSystemFirmware,
             AlignedFlashAddress,
             FlashAddressTypeAbsoluteAddress,
             AlignedBuffer,
             AlignedLength
             );
  if ((OffsetHead != 0) || (OffsetTail != 0)) {
    FreePool (AlignedBuffer);
  }
  return Status;
}

/**
  Platform Flash Access Lib Constructor.
**/
EFI_STATUS
EFIAPI
PerformFlashAccessLibConstructor (
  VOID
  )
{
  mInternalFdAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32(PcdFlashAreaBaseAddress);
  DEBUG((DEBUG_INFO, "PcdFlashAreaBaseAddress - 0x%x\n", mInternalFdAddress));

  return EFI_SUCCESS;
}
