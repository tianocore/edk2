/** @file
  Platform Flash Access library.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/VirtNorFlashDeviceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "PlatformFlashAccessLib.h"

#define ALIGN(v, a)  (UINTN)((((v) - 1) | ((a) - 1)) + 1)

#define FLASH_CODE_BASE  0x20000000
#define FLASH_DATA_BASE  0x22000000
#define FLASH_SIZE       SIZE_32MB
#define BLOCK_SIZE       SIZE_256KB

/**
  Perform flash write operation with progress indicator.  The start and end
  completion percentage values are passed into this function.  If the requested
  flash write operation is broken up, then completion percentage between the
  start and end values may be passed to the provided Progress function.  The
  caller of this function is required to call the Progress function for the
  start and end completion percentage values.  This allows the Progress,
  StartPercentage, and EndPercentage parameters to be ignored if the requested
  flash write operation can not be broken up

  @param[in] FirmwareType      The type of firmware.
  @param[in] FlashAddress      The address of flash device to be accessed.
  @param[in] FlashAddressType  The type of flash device address.
  @param[in] Buffer            The pointer to the data buffer.
  @param[in] Length            The length of data buffer in bytes.
  @param[in] Progress          A function used report the progress of the
                               firmware update.  This is an optional parameter
                               that may be NULL.
  @param[in] StartPercentage   The start completion percentage value that may
                               be used to report progress during the flash
                               write operation.
  @param[in] EndPercentage     The end completion percentage value that may
                               be used to report progress during the flash
                               write operation.

  @retval EFI_SUCCESS           The operation returns successfully.
  @retval EFI_WRITE_PROTECTED   The flash device is read only.
  @retval EFI_UNSUPPORTED       The flash device access is unsupported.
  @retval EFI_INVALID_PARAMETER The input parameter is not valid.
**/
EFI_STATUS
EFIAPI
PerformFlashWriteWithProgress (
  IN PLATFORM_FIRMWARE_TYPE FirmwareType,
  IN EFI_PHYSICAL_ADDRESS FlashAddress,
  IN FLASH_ADDRESS_TYPE FlashAddressType,
  IN VOID *Buffer,
  IN UINTN Length,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS Progress, OPTIONAL
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       LbaNum;
  UINTN       Lba;
  UINTN       Index;
  VOID        *ShadowBuffer;
  UINTN       LastBlock;
  UINTN       NumByte;
  UINTN       FlashBase;

  DEBUG ((DEBUG_INFO, "PerformFlashWrite - 0x%x(%x) - 0x%x\n", (UINTN)FlashAddress, (UINTN)FlashAddressType, Length));

  if (FlashAddressType != FlashAddressTypeAbsoluteAddress) {
    // Only support absolute address for this platform
    return EFI_INVALID_PARAMETER;
  }

  if (FirmwareType == PlatformFirmwareTypeSystemFirmware) {
    if ((FlashAddress < FLASH_CODE_BASE) || ((FlashAddress + Length) > (FLASH_CODE_BASE + FLASH_SIZE))) {
      return EFI_INVALID_PARAMETER;
    }

    FlashBase = FLASH_CODE_BASE;
  } else {
    if ((FlashAddress < FLASH_DATA_BASE) || ((FlashAddress + Length) > (FLASH_DATA_BASE + FLASH_SIZE))) {
      return EFI_INVALID_PARAMETER;
    }

    FlashBase = FLASH_DATA_BASE;
  }

  ShadowBuffer = AllocateZeroPool (BLOCK_SIZE);
  LastBlock    = FLASH_SIZE / BLOCK_SIZE - 1;
  if (  (ALIGN (FlashAddress, BLOCK_SIZE) != FlashAddress)
     || (Length % BLOCK_SIZE))
  {
    // Not expect un-aligned flash address
    return EFI_INVALID_PARAMETER;
  }

  //
  // Erase & Write
  //
  LbaNum = Length / BLOCK_SIZE;
  Lba    = (FlashAddress - FlashBase) / BLOCK_SIZE;
  for (Index = 0; Index < LbaNum; Index++) {
    if (Progress != NULL) {
      Progress (StartPercentage + ((Index * (EndPercentage - StartPercentage)) / LbaNum));
    }

    if (CompareMem (
          (UINT8 *)(UINTN)(GET_NOR_BLOCK_ADDRESS (FlashBase, Lba + Index, BLOCK_SIZE)),
          (UINT8 *)Buffer + Index * BLOCK_SIZE,
          BLOCK_SIZE
          ) == 0)
    {
      DEBUG ((DEBUG_INFO, "Sector - 0x%x - skip\n", Index));
      continue;
    }

    DEBUG ((DEBUG_INFO, "Sector - 0x%x - update...\n", Index));
    NumByte = BLOCK_SIZE;
    Status  = NorFlashWriteSingleBlock (
                FlashBase,
                FlashBase,
                Lba + Index,
                LastBlock,
                BLOCK_SIZE,
                FLASH_SIZE,
                0,
                &NumByte,
                (UINT8 *)Buffer + Index * BLOCK_SIZE,
                ShadowBuffer
                );
    if ((Status != EFI_SUCCESS) || (NumByte != BLOCK_SIZE)) {
      DEBUG ((
        DEBUG_ERROR,
        "Sector - 0x%x - update failed (bytes written)...\n",
        Index,
        NumByte
        ));
      break;
    }
  }

  if (Progress != NULL) {
    Progress (EndPercentage);
  }

  FreePool (ShadowBuffer);
  return Status;
}

/**
  Perform flash write operation.

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
  IN PLATFORM_FIRMWARE_TYPE  FirmwareType,
  IN EFI_PHYSICAL_ADDRESS    FlashAddress,
  IN FLASH_ADDRESS_TYPE      FlashAddressType,
  IN VOID                    *Buffer,
  IN UINTN                   Length
  )
{
  return PerformFlashWriteWithProgress (
           FirmwareType,
           FlashAddress,
           FlashAddressType,
           Buffer,
           Length,
           NULL,
           0,
           0
           );
}

/**
  Platform Flash Access Lib Constructor.

  @retval EFI_SUCCESS  Constructor returns successfully.
**/
EFI_STATUS
EFIAPI
PlatformFlashAccessLibConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // map the code flash region, the data flash region
  // already mapped via variable driver
  //
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  FLASH_CODE_BASE,
                  SIZE_32MB,
                  EFI_MEMORY_UC
                  );
  if (!EFI_ERROR (Status)) {
    Status = gDS->SetMemorySpaceAttributes (
                    FLASH_CODE_BASE,
                    SIZE_32MB,
                    EFI_MEMORY_UC
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
