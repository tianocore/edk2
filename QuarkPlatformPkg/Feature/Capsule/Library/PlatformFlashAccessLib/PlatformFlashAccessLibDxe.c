/** @file
  Platform Flash Access library.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PlatformFlashAccessLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Spi.h>

//
// SPI default opcode slots
//
#define SPI_OPCODE_JEDEC_ID_INDEX        0
#define SPI_OPCODE_READ_ID_INDEX         1
#define SPI_OPCODE_WRITE_S_INDEX         2
#define SPI_OPCODE_WRITE_INDEX           3
#define SPI_OPCODE_READ_INDEX            4
#define SPI_OPCODE_ERASE_INDEX           5
#define SPI_OPCODE_READ_S_INDEX          6
#define SPI_OPCODE_CHIP_ERASE_INDEX      7

#define SPI_ERASE_SECTOR_SIZE            SIZE_4KB  //This is the chipset requirement

STATIC EFI_PHYSICAL_ADDRESS     mInternalFdAddress;
EFI_SPI_PROTOCOL                *mSpiProtocol;

/**
  Writes specified number of bytes from the input buffer to the address

  @param[in]      WriteAddress  The flash address to be written.
  @param[in, out] NumBytes      The number of bytes.
  @param[in]      Buffer        The data buffer to be written.

  @return The status of flash write.
**/
EFI_STATUS
FlashFdWrite (
  IN  UINTN                               WriteAddress,
  IN OUT UINTN                            *NumBytes,
  IN  UINT8                               *Buffer
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  Status = mSpiProtocol->Execute (
                           mSpiProtocol,
                           SPI_OPCODE_WRITE_INDEX, // OpcodeIndex
                           0,                      // PrefixOpcodeIndex
                           TRUE,                   // DataCycle
                           TRUE,                   // Atomic
                           TRUE,                   // ShiftOut
                           WriteAddress,           // Address
                           (UINT32) (*NumBytes),   // Data Number
                           Buffer,
                           EnumSpiRegionBios
                           );
  DEBUG((DEBUG_INFO, "FlashFdWrite - 0x%x - %r\n", (UINTN)WriteAddress, Status));

  AsmWbinvd ();

  return Status;
}

/**
  Erase a certain block from address LbaWriteAddress

  @param[in] WriteAddress  The flash address to be erased.

  @return The status of flash erase.
**/
EFI_STATUS
FlashFdErase (
  IN UINTN                                WriteAddress
  )
{
  EFI_STATUS  Status;

  Status = mSpiProtocol->Execute (
                           mSpiProtocol,
                           SPI_OPCODE_ERASE_INDEX, // OpcodeIndex
                           0,                      // PrefixOpcodeIndex
                           FALSE,                  // DataCycle
                           TRUE,                   // Atomic
                           FALSE,                  // ShiftOut
                           WriteAddress,           // Address
                           0,                      // Data Number
                           NULL,
                           EnumSpiRegionBios       // SPI_REGION_TYPE
                           );
  DEBUG((DEBUG_INFO, "FlashFdErase - 0x%x - %r\n", (UINTN)WriteAddress, Status));

  AsmWbinvd ();

  return Status;
}

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
  IN PLATFORM_FIRMWARE_TYPE                         FirmwareType,
  IN EFI_PHYSICAL_ADDRESS                           FlashAddress,
  IN FLASH_ADDRESS_TYPE                             FlashAddressType,
  IN VOID                                           *Buffer,
  IN UINTN                                          Length,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,        OPTIONAL
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_STATUS          Status;
  UINTN               SectorNum;
  UINTN               Index;
  UINTN               NumBytes;

  DEBUG((DEBUG_INFO, "PerformFlashWrite - 0x%x(%x) - 0x%x\n", (UINTN)FlashAddress, (UINTN)FlashAddressType, Length));
  if (FlashAddressType == FlashAddressTypeAbsoluteAddress) {
    FlashAddress = FlashAddress - mInternalFdAddress;
  }

  //
  // Erase & Write
  //
  SectorNum = Length / SPI_ERASE_SECTOR_SIZE;
  for (Index = 0; Index < SectorNum; Index++){
    if (Progress != NULL) {
      Progress (StartPercentage + ((Index * (EndPercentage - StartPercentage)) / SectorNum));
    }

    if (CompareMem(
          (UINT8 *)(UINTN)(FlashAddress + mInternalFdAddress) + Index * SPI_ERASE_SECTOR_SIZE,
          (UINT8 *)Buffer + Index * SPI_ERASE_SECTOR_SIZE,
          SPI_ERASE_SECTOR_SIZE) == 0) {
      DEBUG((DEBUG_INFO, "Sector - 0x%x - skip\n", Index));
      continue;
    }
    DEBUG((DEBUG_INFO, "Sector - 0x%x - update...\n", Index));

    Status = FlashFdErase (
               (UINTN)FlashAddress + Index * SPI_ERASE_SECTOR_SIZE
               );
    if (Status != EFI_SUCCESS){
      break;
    }
    NumBytes = SPI_ERASE_SECTOR_SIZE;
    Status = FlashFdWrite (
               (UINTN)FlashAddress + Index * SPI_ERASE_SECTOR_SIZE,
               &NumBytes,
               (UINT8 *)Buffer + Index * SPI_ERASE_SECTOR_SIZE
               );
    if (Status != EFI_SUCCESS){
      break;
    }
  }
  if (Progress != NULL) {
    Progress (EndPercentage);
  }

  return EFI_SUCCESS;
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
  IN PLATFORM_FIRMWARE_TYPE       FirmwareType,
  IN EFI_PHYSICAL_ADDRESS         FlashAddress,
  IN FLASH_ADDRESS_TYPE           FlashAddressType,
  IN VOID                         *Buffer,
  IN UINTN                        Length
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

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS  Constructor returns successfully.
**/
EFI_STATUS
EFIAPI
PerformFlashAccessLibConstructor (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS  Status;

  mInternalFdAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32(PcdFlashAreaBaseAddress);
  DEBUG((DEBUG_INFO, "PcdFlashAreaBaseAddress - 0x%x\n", mInternalFdAddress));

  Status = gBS->LocateProtocol(&gEfiSpiProtocolGuid, NULL, (VOID **)&mSpiProtocol);
  ASSERT_EFI_ERROR(Status);

  return EFI_SUCCESS;
}
