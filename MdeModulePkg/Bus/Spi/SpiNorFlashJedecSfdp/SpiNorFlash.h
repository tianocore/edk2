/** @file
  Definitions of SPI NOR flash operation functions.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_NOR_FLASH_H_
#define SPI_NOR_FLASH_H_

#include <PiDxe.h>
#include <Protocol/SpiNorFlash.h>
#include <Protocol/SpiIo.h>
#include "SpiNorFlashJedecSfdpInternal.h"

/**
  Fill Write Buffer with Opcode, Address, Dummy Bytes, and Data

  @param[in]    Opcode      - Opcode for transaction
  @param[in]    Address     - SPI Offset Start Address
  @param[in]    WriteBytes  - Number of bytes to write to SPI device
  @param[in]    WriteBuffer - Buffer containing bytes to write to SPI device

  @retval       Size of Data in Buffer
**/
UINT32
FillWriteBuffer (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT8                   Opcode,
  IN      UINT32                  DummyBytes,
  IN      UINT8                   AddressBytesSupported,
  IN      BOOLEAN                 UseAddress,
  IN      UINT32                  Address,
  IN      UINT32                  WriteBytes,
  IN      UINT8                   *WriteBuffer
  );

/**
  Set Write Enable Latch

  @param[in]  Instance                 SPI NOR instance with all protocols, etc.

  @retval EFI_SUCCESS           SPI Write Enable succeeded
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
SetWel (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance
  );

/**
  Check for not device write in progress

  @param[in]  Instance    SPI NOR instance with all protocols, etc.
  @param[in]  Timeout     Timeout in microsecond
  @param[in]  RetryCount  The retry count

  @retval EFI_SUCCESS           Device does not have a write in progress
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
WaitNotWip (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT32                  Timeout,
  IN      UINT32                  RetryCount
  );

/**
  Check for write enable latch set and not device write in progress

  @param[in]  Instance    SPI NOR instance with all protocols, etc.
  @param[in]  Timeout     Timeout in microsecond
  @param[in]  RetryCount  The retry count

  @retval EFI_SUCCESS           Device does not have a write in progress and
                                write enable latch is set
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
WaitWelNotWip (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT32                  Timeout,
  IN      UINT32                  RetryCount
  );

/**
  Check for not write enable latch set and not device write in progress

  @param[in]  Instance    SPI NOR instance with all protocols, etc.
  @param[in]  Timeout     Timeout in microsecond
  @param[in]  RetryCount  The retry count

  @retval EFI_SUCCESS           Device does not have a write in progress and
                                write enable latch is not set
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
WaitNotWelNotWip (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT32                  Timeout,
  IN      UINT32                  RetryCount
  );

/**
  Read the 3 byte manufacture and device ID from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine reads the 3 byte manufacture and device ID from the flash part
  filling the buffer provided.

  @param[in]  This    Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data structure.
  @param[out] Buffer  Pointer to a 3 byte buffer to receive the manufacture and
                      device ID.



  @retval EFI_SUCCESS            The manufacture and device ID was read
                                 successfully.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL
  @retval EFI_DEVICE_ERROR       Invalid data received from SPI flash part.

**/
EFI_STATUS
EFIAPI
GetFlashId (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  OUT UINT8                             *Buffer
  );

/**
  Read data from the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                             structure.
  @param[in]  FlashAddress   Address in the flash to start reading
  @param[in]  LengthInBytes  Read length in bytes
  @param[out] Buffer         Address of a buffer to receive the data

  @retval EFI_SUCCESS            The data was read successfully.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL, or
                                 FlashAddress >= This->FlashSize, or
                                 LengthInBytes > This->FlashSize - FlashAddress

**/
EFI_STATUS
EFIAPI
ReadData (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN  UINT32                            FlashAddress,
  IN  UINT32                            LengthInBytes,
  OUT UINT8                             *Buffer
  );

/**
  Read data from the SPI flash at not fast speed

  This routine must be called at or below TPL_NOTIFY.
  This routine reads data from the SPI part in the buffer provided.

  @param[in]  This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                             structure.
  @param[in]  FlashAddress   Address in the flash to start reading
  @param[in]  LengthInBytes  Read length in bytes
  @param[out] Buffer         Address of a buffer to receive the data

  @retval EFI_SUCCESS            The data was read successfully.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL, or
                                 FlashAddress >= This->FlashSize, or
                                 LengthInBytes > This->FlashSize - FlashAddress

**/
EFI_STATUS
EFIAPI
LfReadData (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN  UINT32                            FlashAddress,
  IN  UINT32                            LengthInBytes,
  OUT UINT8                             *Buffer
  );

/**
  Read the flash status register.

  This routine must be called at or below TPL_NOTIFY.
  This routine reads the flash part status register.

  @param[in]  This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                             structure.
  @param[in]  LengthInBytes  Number of status bytes to read.
  @param[out] FlashStatus    Pointer to a buffer to receive the flash status.

  @retval EFI_SUCCESS  The status register was read successfully.

**/
EFI_STATUS
EFIAPI
ReadStatus (
  IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN  UINT32                            LengthInBytes,
  OUT UINT8                             *FlashStatus
  );

/**
  Write the flash status register.

  This routine must be called at or below TPL_N OTIFY.
  This routine writes the flash part status register.

  @param[in] This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                            structure.
  @param[in] LengthInBytes  Number of status bytes to write.
  @param[in] FlashStatus    Pointer to a buffer containing the new status.

  @retval EFI_SUCCESS           The status write was successful.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate the write buffer.

**/
EFI_STATUS
EFIAPI
WriteStatus (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            LengthInBytes,
  IN UINT8                             *FlashStatus
  );

/**
  Write data to the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine breaks up the write operation as necessary to write the data to
  the SPI part.

  @param[in] This           Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                            structure.
  @param[in] FlashAddress   Address in the flash to start writing
  @param[in] LengthInBytes  Write length in bytes
  @param[in] Buffer         Address of a buffer containing the data

  @retval EFI_SUCCESS            The data was written successfully.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL, or
                                 FlashAddress >= This->FlashSize, or
                                 LengthInBytes > This->FlashSize - FlashAddress
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory to copy buffer.

**/
EFI_STATUS
EFIAPI
WriteData (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            FlashAddress,
  IN UINT32                            LengthInBytes,
  IN UINT8                             *Buffer
  );

/**
  Efficiently erases one or more 4KiB regions in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine uses a combination of 4 KiB and larger blocks to erase the
  specified area.

  @param[in] This          Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                           structure.
  @param[in] FlashAddress  Address within a 4 KiB block to start erasing
  @param[in] BlockCount    Number of 4 KiB blocks to erase

  @retval EFI_SUCCESS            The erase was completed successfully.
  @retval EFI_INVALID_PARAMETER  FlashAddress >= This->FlashSize, or
                                 BlockCount * 4 KiB
                                   > This->FlashSize - FlashAddress

**/
EFI_STATUS
EFIAPI
Erase (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            FlashAddress,
  IN UINT32                            BlockCount
  );

#endif // SPI_NOR_FLASH_H_
