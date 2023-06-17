/** @file
  SPI NOR Flash operation functions.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TimerLib.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include "SpiNorFlash.h"

/**
  Fill Write Buffer with Opcode, Address, Dummy Bytes, and Data.

  @param[in]    Instance               The instance of SPI_NOR_FLASH
  @param[in]    Opcode                 Opcode for transaction
  @param[in]    DummyBytes             The dummy bytes send to SPI flash device
  @param[in]    AddressBytesSupported  Bytes of address supported by SPI flash device
  @param[in]    UseAddress             Send the address for SPI flash command
  @param[in]    Address                SPI Offset Start Address
  @param[in]    WriteBytes             Number of bytes to write to SPI device
  @param[in]    WriteBuffer            Buffer containing bytes to write to SPI device

  @retval       Size of Data in Buffer
**/
UINT32
FillWriteBuffer (
  IN      SPI_NOR_FLASH_INSTANCE  *Instance,
  IN      UINT8                   Opcode,
  IN      UINT32                  DummyBytes,
  IN      UINT8                   AddressBytesSupported,
  IN      BOOLEAN                 UseAddress,
  IN      UINT32                  Address,
  IN      UINT32                  WriteBytes,
  IN      UINT8                   *WriteBuffer
  )
{
  UINT32  AddressSize;
  UINT32  BigEndianAddress;
  UINT32  Index;
  UINT8   SfdpAddressBytes;

  SfdpAddressBytes = (UINT8)Instance->SfdpBasicFlash->AddressBytes;

  // Copy Opcode into Write Buffer
  Instance->SpiTransactionWriteBuffer[0] = Opcode;
  Index                                  = 1;
  if (UseAddress) {
    if (AddressBytesSupported == SPI_ADDR_3BYTE_ONLY) {
      if (SfdpAddressBytes != 0) {
        // Check if the supported address length is already initiated.
        if ((SfdpAddressBytes != SPI_ADDR_3BYTE_ONLY) && (SfdpAddressBytes != SPI_ADDR_3OR4BYTE)) {
          DEBUG ((DEBUG_ERROR, "%a: Unsupported Address Bytes: 0x%x, SFDP is: 0x%x\n", __func__, AddressBytesSupported, SfdpAddressBytes));
          ASSERT (FALSE);
        }
      }

      AddressSize = 3;
    } else if (AddressBytesSupported == SPI_ADDR_4BYTE_ONLY) {
      if (SfdpAddressBytes != 0) {
        // Check if the supported address length is already initiated.
        if ((SfdpAddressBytes != SPI_ADDR_4BYTE_ONLY) && (SfdpAddressBytes != SPI_ADDR_3OR4BYTE)) {
          DEBUG ((DEBUG_ERROR, "%a: Unsupported Address Bytes: 0x%x, SFDP is: 0x%x\n", __func__, AddressBytesSupported, SfdpAddressBytes));
          ASSERT (FALSE);
        }
      }

      AddressSize = 4;
    } else if (AddressBytesSupported == SPI_ADDR_3OR4BYTE) {
      if (SfdpAddressBytes != 0) {
        // Check if the supported address length is already initiated.
        if (SfdpAddressBytes != SPI_ADDR_3OR4BYTE) {
          DEBUG ((DEBUG_ERROR, "%a: Unsupported Address Bytes: 0x%x, SFDP is: 0x%x\n", __func__, AddressBytesSupported, SfdpAddressBytes));
          ASSERT (FALSE);
        }
      }

      if (Instance->Protocol.FlashSize <= SIZE_16MB) {
        AddressSize = 3;
      } else {
        // SPI part is > 16MB use 4-byte addressing.
        AddressSize = 4;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Invalid Address Bytes\n", __func__));
      ASSERT (FALSE);
    }

    BigEndianAddress   = SwapBytes32 ((UINT32)Address);
    BigEndianAddress >>= ((sizeof (UINT32) - AddressSize) * 8);
    CopyMem (
      &Instance->SpiTransactionWriteBuffer[Index],
      &BigEndianAddress,
      AddressSize
      );
    Index += AddressSize;
  }

  if (SfdpAddressBytes == SPI_ADDR_3OR4BYTE) {
    //
    // TODO:
    // We may need to enter/exit 4-Byte mode if SPI flash
    // device is currently operated in 3-Bytes mode.
    //
  }

  // Fill DummyBytes
  if (DummyBytes != 0) {
    SetMem (
      &Instance->SpiTransactionWriteBuffer[Index],
      DummyBytes,
      0
      );
    Index += DummyBytes;
  }

  // Fill Data
  if (WriteBytes > 0) {
    CopyMem (
      &Instance->SpiTransactionWriteBuffer[Index],
      WriteBuffer,
      WriteBytes
      );
    Index += WriteBytes;
  }

  return Index;
}

/**
  Internal Read the flash status register.

  This routine reads the flash part status register.

  @param[in]  Instance       SPI_NOR_FLASH_INSTANCE
                             structure.
  @param[in]  LengthInBytes  Number of status bytes to read.
  @param[out] FlashStatus    Pointer to a buffer to receive the flash status.

  @retval EFI_SUCCESS  The status register was read successfully.

**/
EFI_STATUS
EFIAPI
InternalReadStatus (
  IN                SPI_NOR_FLASH_INSTANCE  *Instance,
  IN  UINT32                                LengthInBytes,
  OUT UINT8                                 *FlashStatus
  )
{
  EFI_STATUS  Status;
  UINT32      TransactionBufferLength;

  // Read Status register
  TransactionBufferLength = FillWriteBuffer (
                              Instance,
                              SPI_FLASH_RDSR,
                              SPI_FLASH_RDSR_DUMMY,
                              SPI_FLASH_RDSR_ADDR_BYTES,
                              FALSE,
                              0,
                              0,
                              NULL
                              );
  Status = Instance->SpiIo->Transaction (
                              Instance->SpiIo,
                              SPI_TRANSACTION_WRITE_THEN_READ,
                              FALSE,
                              0,
                              1,
                              8,
                              TransactionBufferLength,
                              Instance->SpiTransactionWriteBuffer,
                              1,
                              FlashStatus
                              );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Set Write Enable Latch.

  @param[in]  Instance          SPI NOR instance with all protocols, etc.

  @retval EFI_SUCCESS           SPI Write Enable succeeded
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
SetWel (
  IN      SPI_NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS  Status;
  UINT32      TransactionBufferLength;

  TransactionBufferLength = FillWriteBuffer (
                              Instance,
                              Instance->WriteEnableLatchCommand,
                              SPI_FLASH_WREN_DUMMY,
                              SPI_FLASH_WREN_ADDR_BYTES,
                              FALSE,
                              0,
                              0,
                              NULL
                              );
  Status = Instance->SpiIo->Transaction (
                              Instance->SpiIo,
                              SPI_TRANSACTION_WRITE_ONLY,
                              FALSE,
                              0,
                              1,
                              8,
                              TransactionBufferLength,
                              Instance->SpiTransactionWriteBuffer,
                              0,
                              NULL
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Set WEL fail.\n", __func__));
    ASSERT (FALSE);
  }

  return Status;
}

/**
  Check for not device write in progress.

  @param[in]  SpiNorFlashInstance  SPI NOR instance with all protocols, etc.
  @param[in]  Timeout              Timeout in microsecond
  @param[in]  RetryCount           The retry count

  @retval EFI_SUCCESS           Device does not have a write in progress
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
WaitNotWip (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT32                  Timeout,
  IN      UINT32                  RetryCount
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceStatus;
  UINT32      AlreadyDelayedInMicroseconds;

  if (Timeout == 0) {
    return EFI_SUCCESS;
  }

  if (RetryCount == 0) {
    RetryCount = 1;
  }

  do {
    AlreadyDelayedInMicroseconds = 0;
    while (AlreadyDelayedInMicroseconds < Timeout) {
      Status = InternalReadStatus (SpiNorFlashInstance, 1, &DeviceStatus);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Read status error\n", __func__));
        ASSERT (FALSE);
        return Status;
      }

      if ((DeviceStatus & SPI_FLASH_SR_WIP) == SPI_FLASH_SR_NOT_WIP) {
        return Status;
      }

      MicroSecondDelay (FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds));
      AlreadyDelayedInMicroseconds += FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds);
    }

    RetryCount--;
  } while (RetryCount > 0);

  DEBUG ((DEBUG_ERROR, "%a: Timeout error\n", __func__));
  return EFI_DEVICE_ERROR;
}

/**
  Check for write enable latch set and not device write in progress.

  @param[in]  SpiNorFlashInstance  SPI NOR instance with all protocols, etc.
  @param[in]  Timeout              Timeout in microsecond
  @param[in]  RetryCount           The retry count

  @retval EFI_SUCCESS           Device does not have a write in progress and
                                write enable latch is set
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
WaitWelNotWip (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT32                  Timeout,
  IN      UINT32                  RetryCount
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceStatus;
  UINT32      AlreadyDelayedInMicroseconds;

  if (Timeout == 0) {
    return EFI_SUCCESS;
  }

  if (RetryCount == 0) {
    RetryCount = 1;
  }

  do {
    AlreadyDelayedInMicroseconds = 0;
    while (AlreadyDelayedInMicroseconds < Timeout) {
      Status = InternalReadStatus (SpiNorFlashInstance, 1, &DeviceStatus);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Fail to read WEL.\n", __func__));
        ASSERT_EFI_ERROR (Status);
        return Status;
      }

      if ((DeviceStatus & (SPI_FLASH_SR_WIP | SPI_FLASH_SR_WEL)) == SPI_FLASH_SR_WEL) {
        return Status;
      }

      MicroSecondDelay (FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds));
      AlreadyDelayedInMicroseconds += FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds);
    }

    RetryCount--;
  } while (RetryCount > 0);

  DEBUG ((DEBUG_ERROR, "%a: Timeout error\n", __func__));
  return EFI_DEVICE_ERROR;
}

/**
  Check for not write enable latch set and not device write in progress.

  @param[in]  SpiNorFlashInstance  SPI NOR instance with all protocols, etc.
  @param[in]  Timeout              Timeout in microsecond
  @param[in]  RetryCount           The retry count

  @retval EFI_SUCCESS           Device does not have a write in progress and
                                write enable latch is not set
  @retval EFI_DEVICE_ERROR      SPI Flash part did not respond properly
**/
EFI_STATUS
WaitNotWelNotWip (
  IN      SPI_NOR_FLASH_INSTANCE  *SpiNorFlashInstance,
  IN      UINT32                  Timeout,
  IN      UINT32                  RetryCount
  )
{
  EFI_STATUS  Status;
  UINT8       DeviceStatus;
  UINT32      AlreadyDelayedInMicroseconds;

  if (Timeout == 0) {
    return EFI_SUCCESS;
  }

  if (RetryCount == 0) {
    RetryCount = 1;
  }

  do {
    AlreadyDelayedInMicroseconds = 0;
    while (AlreadyDelayedInMicroseconds < Timeout) {
      Status = InternalReadStatus (SpiNorFlashInstance, 1, &DeviceStatus);
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR (Status) ||
          ((DeviceStatus & (SPI_FLASH_SR_WIP | SPI_FLASH_SR_WEL)) == SPI_FLASH_SR_NOT_WIP))
      {
        return Status;
      }

      MicroSecondDelay (FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds));
      AlreadyDelayedInMicroseconds += FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds);
    }

    RetryCount--;
  } while (RetryCount > 0);

  DEBUG ((DEBUG_ERROR, "SpiNorFlash:%a: Timeout error\n", __func__));
  return EFI_DEVICE_ERROR;
}

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
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                  TransactionBufferLength;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __func__));

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = SPI_NOR_FLASH_FROM_THIS (This);

  // Check not WIP
  Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));

  if (!EFI_ERROR (Status)) {
    TransactionBufferLength = FillWriteBuffer (
                                Instance,
                                SPI_FLASH_RDID,
                                SPI_FLASH_RDID_DUMMY,
                                SPI_FLASH_RDID_ADDR_BYTES,
                                FALSE,
                                0,
                                0,
                                NULL
                                );
    Status = Instance->SpiIo->Transaction (
                                Instance->SpiIo,
                                SPI_TRANSACTION_WRITE_THEN_READ,
                                FALSE,
                                0,
                                1,
                                8,
                                TransactionBufferLength,
                                Instance->SpiTransactionWriteBuffer,
                                3,
                                Buffer
                                );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Read data from the SPI flash at not fast speed.

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
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                  ByteCounter;
  UINT32                  CurrentAddress;
  UINT8                   *CurrentBuffer;
  UINT32                  Length;
  UINT32                  TransactionBufferLength;
  UINT32                  MaximumTransferBytes;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __func__));

  Status = EFI_DEVICE_ERROR;
  if ((Buffer == NULL) ||
      (FlashAddress >= This->FlashSize) ||
      (LengthInBytes > This->FlashSize - FlashAddress))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance             = SPI_NOR_FLASH_FROM_THIS (This);
  MaximumTransferBytes = Instance->SpiIo->MaximumTransferBytes;

  CurrentBuffer = Buffer;
  Length        = 0;
  for (ByteCounter = 0; ByteCounter < LengthInBytes;) {
    CurrentAddress = FlashAddress + ByteCounter;
    CurrentBuffer  = Buffer + ByteCounter;
    Length         = LengthInBytes - ByteCounter;
    // Length must be MaximumTransferBytes or less
    if (Length > MaximumTransferBytes) {
      Length = MaximumTransferBytes;
    }

    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
    if (EFI_ERROR (Status)) {
      break;
    }

    TransactionBufferLength = FillWriteBuffer (
                                Instance,
                                SPI_FLASH_READ,
                                SPI_FLASH_READ_DUMMY,
                                SPI_FLASH_READ_ADDR_BYTES,
                                TRUE,
                                CurrentAddress,
                                0,
                                NULL
                                );
    Status = Instance->SpiIo->Transaction (
                                Instance->SpiIo,
                                SPI_TRANSACTION_WRITE_THEN_READ,
                                FALSE,
                                0,
                                1,
                                8,
                                TransactionBufferLength,
                                Instance->SpiTransactionWriteBuffer,
                                Length,
                                CurrentBuffer
                                );
    ASSERT_EFI_ERROR (Status);
    ByteCounter += Length;
  }

  return Status;
}

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
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                  ByteCounter;
  UINT32                  CurrentAddress;
  UINT8                   *CurrentBuffer;
  UINT32                  Length;
  UINT32                  TransactionBufferLength;
  UINT32                  MaximumTransferBytes;
  UINT8                   FastReadInstruction;
  UINT8                   FastReadWaitStateDummyClocks;
  UINT8                   FastReadModeClock;

  DEBUG ((DEBUG_INFO, "%a: Entry, Read address = 0x%08x, Length = 0x%08x\n", __func__, FlashAddress, LengthInBytes));

  Status = EFI_DEVICE_ERROR;
  if ((Buffer == NULL) ||
      (FlashAddress >= This->FlashSize) ||
      (LengthInBytes > This->FlashSize - FlashAddress))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance             = SPI_NOR_FLASH_FROM_THIS (This);
  MaximumTransferBytes = Instance->SpiIo->MaximumTransferBytes;

  //
  // Initial the default read operation parameters.
  //
  FastReadInstruction          = SPI_FLASH_FAST_READ;
  FastReadWaitStateDummyClocks = SPI_FLASH_FAST_READ_DUMMY * 8;
  FastReadModeClock            = 0;
  //
  // Override by the Fast Read capabiity table.
  //
  // Get the first supported fast read comamnd.
  // This will be the standard fast read command (0x0b),
  // which is the first fast read command added to the
  // supported list.
  // TODO: The mechanism to choose the advanced fast read
  //       is not determined yet in this version of
  //       SpiNorFlash driver.
  Status = GetFastReadParameter (
             Instance,
             &FastReadInstruction,
             &FastReadModeClock,
             &FastReadWaitStateDummyClocks
             );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "  Use below Fast Read mode:\n"));
  } else {
    DEBUG ((DEBUG_VERBOSE, "  Use the default Fast Read mode:\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "    Instruction                        : 0x%x\n", FastReadInstruction));
  DEBUG ((DEBUG_VERBOSE, "    Mode Clock                         : 0x%x\n", FastReadModeClock));
  DEBUG ((DEBUG_VERBOSE, "    Wait States (Dummy Clocks) in clock: 0x%x\n", FastReadWaitStateDummyClocks));
  DEBUG ((DEBUG_VERBOSE, "     Supported erase address bytes by device: 0x%02x.\n", Instance->SfdpBasicFlash->AddressBytes));
  DEBUG ((DEBUG_VERBOSE, "        (00: 3-Byte, 01: 3 or 4-Byte. 10: 4-Byte)\n"));

  CurrentBuffer = Buffer;
  Length        = 0;
  for (ByteCounter = 0; ByteCounter < LengthInBytes;) {
    CurrentAddress = FlashAddress + ByteCounter;
    CurrentBuffer  = Buffer + ByteCounter;
    Length         = LengthInBytes - ByteCounter;
    // Length must be MaximumTransferBytes or less
    if (Length > MaximumTransferBytes) {
      Length = MaximumTransferBytes;
    }

    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
    if (EFI_ERROR (Status)) {
      break;
    }

    TransactionBufferLength = FillWriteBuffer (
                                Instance,
                                FastReadInstruction,
                                FastReadWaitStateDummyClocks / 8,
                                (UINT8)Instance->SfdpBasicFlash->AddressBytes,
                                TRUE,
                                CurrentAddress,
                                0,
                                NULL
                                );
    Status = Instance->SpiIo->Transaction (
                                Instance->SpiIo,
                                SPI_TRANSACTION_WRITE_THEN_READ,
                                FALSE,
                                0,
                                1,
                                8,
                                TransactionBufferLength,
                                Instance->SpiTransactionWriteBuffer,
                                Length,
                                CurrentBuffer
                                );
    ASSERT_EFI_ERROR (Status);
    ByteCounter += Length;
  }

  return Status;
}

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
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;

  if (LengthInBytes != 1) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = SPI_NOR_FLASH_FROM_THIS (This);

  Status = InternalReadStatus (Instance, LengthInBytes, FlashStatus);

  return Status;
}

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
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                  TransactionBufferLength;

  if (LengthInBytes != 1) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = SPI_NOR_FLASH_FROM_THIS (This);

  // Check not WIP
  Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));

  // Set Write Enable
  if (!EFI_ERROR (Status)) {
    if (Instance->WriteEnableLatchRequired) {
      Status = SetWel (Instance);
      DEBUG ((DEBUG_ERROR, "%a: set Write Enable Error.\n", __func__));
      ASSERT_EFI_ERROR (Status);
      // Check not WIP & WEL enabled
      Status = WaitWelNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
    }

    // Write the Status Register
    if (!EFI_ERROR (Status)) {
      TransactionBufferLength = FillWriteBuffer (
                                  Instance,
                                  SPI_FLASH_WRSR,
                                  SPI_FLASH_WRSR_DUMMY,
                                  SPI_FLASH_WRSR_ADDR_BYTES,
                                  FALSE,
                                  0,
                                  0,
                                  NULL
                                  );
      Status = Instance->SpiIo->Transaction (
                                  Instance->SpiIo,
                                  SPI_TRANSACTION_WRITE_ONLY,
                                  FALSE,
                                  0,
                                  1,
                                  8,
                                  TransactionBufferLength,
                                  Instance->SpiTransactionWriteBuffer,
                                  0,
                                  NULL
                                  );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return Status;
}

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
  )
{
  EFI_STATUS              Status;
  SPI_NOR_FLASH_INSTANCE  *Instance;
  UINT32                  ByteCounter;
  UINT32                  CurrentAddress;
  UINT32                  Length;
  UINT32                  BytesUntilBoundary;
  UINT8                   *CurrentBuffer;
  UINT32                  TransactionBufferLength;
  UINT32                  MaximumTransferBytes;
  UINT32                  SpiFlashPageSize;

  DEBUG ((DEBUG_INFO, "%a: Entry: Write address = 0x%08x, Length = 0x%08x\n", __func__, FlashAddress, LengthInBytes));

  Status = EFI_DEVICE_ERROR;
  if ((Buffer == NULL) ||
      (LengthInBytes == 0) ||
      (FlashAddress >= This->FlashSize) ||
      (LengthInBytes > This->FlashSize - FlashAddress))
  {
    return EFI_INVALID_PARAMETER;
  }

  Instance             = SPI_NOR_FLASH_FROM_THIS (This);
  MaximumTransferBytes = Instance->SpiIo->MaximumTransferBytes;
  if (Instance->SfdpBasicFlashByteCount >= 11 * 4) {
    // JESD216C spec DWORD 11
    SpiFlashPageSize = 1 << Instance->SfdpBasicFlash->PageSize;
  } else {
    SpiFlashPageSize = 256;
  }

  CurrentBuffer = Buffer;
  Length        = 0;
  for (ByteCounter = 0; ByteCounter < LengthInBytes;) {
    CurrentAddress = FlashAddress + ByteCounter;
    CurrentBuffer  = Buffer + ByteCounter;
    Length         = LengthInBytes - ByteCounter;
    // Length must be MaximumTransferBytes or less
    if (Length > MaximumTransferBytes) {
      Length = MaximumTransferBytes;
    }

    // Cannot cross SpiFlashPageSize boundary
    BytesUntilBoundary = SpiFlashPageSize
                         - (CurrentAddress % SpiFlashPageSize);
    if ((BytesUntilBoundary != 0) && (Length > BytesUntilBoundary)) {
      Length = BytesUntilBoundary;
    }

    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Instance->WriteEnableLatchRequired) {
      // Set Write Enable
      Status = SetWel (Instance);
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR (Status)) {
        break;
      }

      // Check not WIP & WEL enabled
      Status = WaitWelNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
      if (EFI_ERROR (Status)) {
        break;
      }
    }

    //  Write Data
    TransactionBufferLength = FillWriteBuffer (
                                Instance,
                                SPI_FLASH_PP,
                                SPI_FLASH_PP_DUMMY,
                                SPI_FLASH_PP_ADDR_BYTES,
                                TRUE,
                                CurrentAddress,
                                Length,
                                CurrentBuffer
                                );
    Status = Instance->SpiIo->Transaction (
                                Instance->SpiIo,
                                SPI_TRANSACTION_WRITE_ONLY,
                                FALSE,
                                0,
                                1,
                                8,
                                TransactionBufferLength,
                                Instance->SpiTransactionWriteBuffer,
                                0,
                                NULL
                                );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Instance->WriteEnableLatchRequired) {
      // Check not WIP & not WEL
      Status = WaitNotWelNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
      if (EFI_ERROR (Status)) {
        break;
      }
    } else {
      Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
      if (EFI_ERROR (Status)) {
        break;
      }
    }

    ByteCounter += Length;
  }

  return Status;
}

/**
  Efficiently erases blocks in the SPI flash.

  This routine must be called at or below TPL_NOTIFY.
  This routine may use the combination of variable earse sizes to erase the
  specified area accroding to the flash region.

  @param[in] This          Pointer to an EFI_SPI_NOR_FLASH_PROTOCOL data
                           structure.
  @param[in] FlashAddress  Address to start erasing
  @param[in] BlockCount    Number of blocks to erase. The block size is indicated
                           in EraseBlockBytes in EFI_SPI_NOR_FLASH_PROTOCOL.

  @retval EFI_SUCCESS            The erase was completed successfully.
  @retval EFI_DEVICE_ERROR       The flash devices has problems.
  @retval EFI_INVALID_PARAMETER  The given FlashAddress and/or BlockCount
                                 is invalid.

**/
EFI_STATUS
EFIAPI
Erase (
  IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
  IN UINT32                            FlashAddress,
  IN UINT32                            BlockCount
  )
{
  EFI_STATUS                 Status;
  SPI_NOR_FLASH_INSTANCE     *Instance;
  UINT8                      Opcode;
  UINT32                     Dummy;
  UINT32                     ByteCounter;
  UINT32                     EraseLength;
  UINT32                     TotalEraseLength;
  UINT32                     CurrentAddress;
  UINT32                     TransactionBufferLength;
  UINT32                     BlockCountToErase;
  UINT32                     BlockSizeToErase;
  UINT8                      BlockEraseCommand;
  UINT32                     TypicalEraseTime;
  UINT64                     MaximumEraseTimeout;
  SFDP_SECTOR_REGION_RECORD  *FlashRegion;

  DEBUG ((DEBUG_INFO, "%a: Entry: Erase address = 0x%08x, Block count = 0x%x\n", __func__, FlashAddress, BlockCount));

  Status   = EFI_DEVICE_ERROR;
  Instance = SPI_NOR_FLASH_FROM_THIS (This);

  // Get the region of this flash address.
  Status = GetRegionByFlashAddress (Instance, FlashAddress, &FlashRegion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "  Failed to get the flash region of this flash address.\n"));
    ASSERT (FALSE);
    return Status;
  }

  CurrentAddress    = FlashAddress;
  BlockCountToErase = BlockCount;
  BlockSizeToErase  = FlashRegion->SectorSize; // This is also the minimum block erase size.
  TotalEraseLength  = BlockCountToErase * FlashRegion->SectorSize;
  if ((FlashAddress + TotalEraseLength) > (FlashRegion->RegionAddress + FlashRegion->RegionTotalSize)) {
    DEBUG ((DEBUG_ERROR, "  The blocks to erase exceeds the region boundary.\n"));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "  Region starting address: 0x%08x.\n", FlashRegion->RegionAddress));
  DEBUG ((DEBUG_VERBOSE, "  Region size            : 0x%08x.\n", FlashRegion->RegionTotalSize));
  DEBUG ((DEBUG_VERBOSE, "  Region sector size     : 0x%08x.\n", FlashRegion->SectorSize));
  DEBUG ((DEBUG_VERBOSE, "  Supported erase address bytes by device: 0x%02x.\n", Instance->SfdpBasicFlash->AddressBytes));
  DEBUG ((DEBUG_VERBOSE, "    (00: 3-Byte, 01: 3 or 4-Byte. 10: 4-Byte)\n"));

  // Loop until all blocks are erased.
  ByteCounter = 0;
  while (ByteCounter < TotalEraseLength) {
    CurrentAddress = FlashAddress + ByteCounter;

    // Is this the whole device erase.
    if (TotalEraseLength == This->FlashSize) {
      Opcode      = SPI_FLASH_CE;
      Dummy       = SPI_FLASH_CE_DUMMY;
      EraseLength = TotalEraseLength;
      DEBUG ((DEBUG_VERBOSE, "  This is the chip erase.\n"));
    } else {
      //
      // Get the erase block attributes.
      //
      Status = GetEraseBlockAttribute (
                 Instance,
                 FlashRegion,
                 CurrentAddress,
                 TotalEraseLength - ByteCounter,
                 &BlockSizeToErase,
                 &BlockCountToErase,
                 &BlockEraseCommand,
                 &TypicalEraseTime,
                 &MaximumEraseTimeout
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "  Failed to get erase block attribute.\n"));
        ASSERT (FALSE);
      }

      Opcode      = BlockEraseCommand;
      Dummy       = SPI_FLASH_BE_DUMMY;
      EraseLength = BlockCountToErase * BlockSizeToErase;
      DEBUG ((
        DEBUG_VERBOSE,
        "  Erase command 0x%02x at adddress 0x%08x for length 0x%08x.\n",
        BlockEraseCommand,
        CurrentAddress,
        EraseLength
        ));
    }

    //
    // Process the erase command.
    //

    // Check not WIP
    Status = WaitNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Instance->WriteEnableLatchRequired) {
      // Set Write Enable
      Status = SetWel (Instance);
      if (EFI_ERROR (Status)) {
        break;
      }

      // Check not WIP & WEL enabled
      Status = WaitWelNotWip (Instance, FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds), FixedPcdGet32 (PcdSpiNorFlashFixedTimeoutRetryCount));
      if (EFI_ERROR (Status)) {
        break;
      }
    }

    // Erase Block
    TransactionBufferLength = FillWriteBuffer (
                                Instance,
                                Opcode,
                                Dummy,
                                (UINT8)Instance->SfdpBasicFlash->AddressBytes,
                                TRUE,
                                CurrentAddress,
                                0,
                                NULL
                                );
    Status = Instance->SpiIo->Transaction (
                                Instance->SpiIo,
                                SPI_TRANSACTION_WRITE_ONLY,
                                FALSE,
                                0,
                                1,
                                8,
                                TransactionBufferLength,
                                Instance->SpiTransactionWriteBuffer,
                                0,
                                NULL
                                );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      break;
    } else {
      DEBUG ((DEBUG_VERBOSE, "Erase command sucessfully.\n"));
    }

    if (Instance->WriteEnableLatchRequired) {
      //
      // Check not WIP & not WEL
      // Use the timeout value calculated by SPI NOR flash SFDP.
      //
      Status = WaitNotWelNotWip (Instance, (UINT32)MaximumEraseTimeout * 1000, FixedPcdGet32 (PcdSpiNorFlashOperationRetryCount));
      if (EFI_ERROR (Status)) {
        break;
      }
    } else {
      //
      // Use the timeout value calculated by SPI NOR flash SFDP.
      //
      Status = WaitNotWip (Instance, (UINT32)MaximumEraseTimeout * 1000, FixedPcdGet32 (PcdSpiNorFlashOperationRetryCount));
      if (EFI_ERROR (Status)) {
        break;
      }
    }

    ByteCounter += EraseLength;
  }

  return Status;
}
