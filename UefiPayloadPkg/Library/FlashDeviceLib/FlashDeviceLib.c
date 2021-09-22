/** @file
  Flash Device Library based on SPI Flash library.

Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SpiFlashLib.h>

/**
  Initialize spi flash device.

  @retval EFI_SUCCESS              The tested spi flash device is supported.
  @retval EFI_UNSUPPORTED          The tested spi flash device is not supported.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceInit (
  VOID
  )
{
  return SpiConstructor ();
}


/**
  Read NumBytes bytes of data from the address specified by
  PAddress into Buffer.

  @param[in]      PAddress      The starting physical address of the read.
  @param[in,out]  NumBytes      On input, the number of bytes to read. On output, the number
                                of bytes actually read.
  @param[out]     Buffer        The destination data buffer for the read.

  @retval         EFI_SUCCESS.      Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceRead (
  IN      UINTN                           PAddress,
  IN  OUT UINTN                           *NumBytes,
      OUT UINT8                           *Buffer
  )
{
  EFI_STATUS                              Status;
  UINT32                                  ByteCount;
  UINT32                                  RgnSize;
  UINT32                                  AddrOffset;

  Status = SpiGetRegionAddress (FlashRegionBios, NULL, &RgnSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // BIOS region offset can be calculated by (PAddress - (0x100000000 - RgnSize))
  // which equal (PAddress + RgnSize) here.
  AddrOffset = (UINT32)((UINT32)PAddress + RgnSize);
  ByteCount  = (UINT32)*NumBytes;
  return SpiFlashRead (FlashRegionBios, AddrOffset, ByteCount, Buffer);
}


/**
  Write NumBytes bytes of data from Buffer to the address specified by
  PAddresss.

  @param[in]      PAddress        The starting physical address of the write.
  @param[in,out]  NumBytes        On input, the number of bytes to write. On output,
                                  the actual number of bytes written.
  @param[in]      Buffer          The source data buffer for the write.

  @retval         EFI_SUCCESS.      Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceWrite (
  IN        UINTN                           PAddress,
  IN OUT    UINTN                           *NumBytes,
  IN        UINT8                           *Buffer
  )
{
  EFI_STATUS                                Status;
  UINT32                                    ByteCount;
  UINT32                                    RgnSize;
  UINT32                                    AddrOffset;

  Status = SpiGetRegionAddress (FlashRegionBios, NULL, &RgnSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // BIOS region offset can be calculated by (PAddress - (0x100000000 - RgnSize))
  // which equal (PAddress + RgnSize) here.
  AddrOffset = (UINT32)((UINT32)PAddress + RgnSize);
  ByteCount  = (UINT32)*NumBytes;
  return SpiFlashWrite (FlashRegionBios, AddrOffset, ByteCount, Buffer);
}


/**
  Erase the block starting at PAddress.

  @param[in]  PAddress        The starting physical address of the block to be erased.
                              This library assume that caller garantee that the PAddress
                              is at the starting address of this block.
  @param[in]  LbaLength       The length of the logical block to be erased.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceBlockErase (
  IN    UINTN                     PAddress,
  IN    UINTN                     LbaLength
  )
{
  EFI_STATUS                      Status;
  UINT32                          RgnSize;
  UINT32                          AddrOffset;

  Status = SpiGetRegionAddress (FlashRegionBios, NULL, &RgnSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // BIOS region offset can be calculated by (PAddress - (0x100000000 - RgnSize))
  // which equal (PAddress + RgnSize) here.
  AddrOffset = (UINT32)((UINT32)PAddress + RgnSize);
  return SpiFlashErase (FlashRegionBios, AddrOffset, (UINT32)LbaLength);
}


/**
  Lock or unlock the block starting at PAddress.

  @param[in]  PAddress        The starting physical address of region to be (un)locked.
  @param[in]  LbaLength       The length of the logical block to be erased.
  @param[in]  Lock            TRUE to lock. FALSE to unlock.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceBlockLock (
  IN    UINTN                          PAddress,
  IN    UINTN                          LbaLength,
  IN    BOOLEAN                        Lock
  )
{
  return EFI_SUCCESS;
}

