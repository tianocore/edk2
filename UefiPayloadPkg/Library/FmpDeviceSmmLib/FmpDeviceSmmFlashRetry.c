/** @file
  Internal retry helpers for SMMSTORE-backed firmware updates.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>

#include "FmpDeviceSmmFlashRetry.h"

/**
  Delay after a failed flash operation when a stall callback is available.

  @param[in] FlashIo  Flash operation callbacks.
  @param[in] Attempt  Failed attempt number.
**/
STATIC
VOID
StallAfterFailure (
  IN CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN UINTN                      Attempt
  )
{
  if (FlashIo->Stall != NULL) {
    FlashIo->Stall (Attempt);
  }
}

/**
  Read from flash, retrying transient failures and short reads.

  @param[in]     FlashIo   Flash operation callbacks.
  @param[in]     Lba       Flash block number.
  @param[in]     Offset    Offset within the block.
  @param[in,out] NumBytes  Requested and actual byte count.
  @param[out]    Buffer    Destination buffer.

  @retval EFI_SUCCESS           The requested bytes were read.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @return                       The final flash read error.
**/
EFI_STATUS
FmpDeviceFlashReadWithRetry (
  IN     CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN     EFI_LBA                    Lba,
  IN     UINTN                      Offset,
  IN OUT UINTN                      *NumBytes,
  OUT    UINT8                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       Attempt;
  UINTN       RequestedBytes;
  UINTN       ActualBytes;

  if ((FlashIo == NULL) || (FlashIo->Read == NULL) || (NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RequestedBytes = *NumBytes;
  Status         = EFI_DEVICE_ERROR;
  ActualBytes    = 0;

  for (Attempt = 0; Attempt < FMP_DEVICE_SMM_FLASH_RETRY_COUNT; ++Attempt) {
    ActualBytes = RequestedBytes;
    Status      = FlashIo->Read (Lba, Offset, &ActualBytes, Buffer);
    if (!EFI_ERROR (Status) && (ActualBytes == RequestedBytes)) {
      *NumBytes = ActualBytes;
      return EFI_SUCCESS;
    }

    *NumBytes = ActualBytes;
    StallAfterFailure (FlashIo, Attempt);
  }

  return EFI_ERROR (Status) ? Status : EFI_DEVICE_ERROR;
}

/**
  Erase a flash block, retrying transient failures.

  @param[in] FlashIo  Flash operation callbacks.
  @param[in] Lba      Flash block number.

  @retval EFI_SUCCESS           The block was erased.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @return                       The final flash erase error.
**/
EFI_STATUS
FmpDeviceFlashEraseWithRetry (
  IN CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN EFI_LBA                    Lba
  )
{
  EFI_STATUS  Status;
  UINTN       Attempt;

  if ((FlashIo == NULL) || (FlashIo->Erase == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_DEVICE_ERROR;
  for (Attempt = 0; Attempt < FMP_DEVICE_SMM_FLASH_RETRY_COUNT; ++Attempt) {
    Status = FlashIo->Erase (Lba);
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    StallAfterFailure (FlashIo, Attempt);
  }

  return Status;
}

/**
  Write to flash, retrying transient failures and short writes.

  @param[in]     FlashIo   Flash operation callbacks.
  @param[in]     Lba       Flash block number.
  @param[in]     Offset    Offset within the block.
  @param[in,out] NumBytes  Requested and actual byte count.
  @param[in]     Buffer    Source buffer.

  @retval EFI_SUCCESS           The requested bytes were written.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @return                       The final flash write error.
**/
EFI_STATUS
FmpDeviceFlashWriteWithRetry (
  IN     CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN     EFI_LBA                    Lba,
  IN     UINTN                      Offset,
  IN OUT UINTN                      *NumBytes,
  IN     UINT8                      *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       Attempt;
  UINTN       RequestedBytes;
  UINTN       ActualBytes;

  if ((FlashIo == NULL) || (FlashIo->Write == NULL) || (NumBytes == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RequestedBytes = *NumBytes;
  Status         = EFI_DEVICE_ERROR;
  ActualBytes    = 0;

  for (Attempt = 0; Attempt < FMP_DEVICE_SMM_FLASH_RETRY_COUNT; ++Attempt) {
    ActualBytes = RequestedBytes;
    Status      = FlashIo->Write (Lba, Offset, &ActualBytes, Buffer);
    if (!EFI_ERROR (Status) && (ActualBytes == RequestedBytes)) {
      *NumBytes = ActualBytes;
      return EFI_SUCCESS;
    }

    *NumBytes = ActualBytes;
    StallAfterFailure (FlashIo, Attempt);
  }

  return EFI_ERROR (Status) ? Status : EFI_DEVICE_ERROR;
}

/**
  Read back and verify a programmed flash block.

  @param[in]  FlashIo      Flash operation callbacks.
  @param[in]  Lba          Flash block number.
  @param[in]  Expected     Expected block contents.
  @param[out] VerifyBuffer Scratch buffer for the readback.
  @param[in]  BlockSize    Flash block size.

  @retval EFI_SUCCESS           The block matches Expected.
  @retval EFI_DEVICE_ERROR      The block could not be read.
  @retval EFI_VOLUME_CORRUPTED  The block contents differ.
**/
STATIC
EFI_STATUS
VerifyFlashWrite (
  IN  CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN  EFI_LBA                    Lba,
  IN  CONST UINT8                *Expected,
  OUT UINT8                      *VerifyBuffer,
  IN  UINTN                      BlockSize
  )
{
  EFI_STATUS  Status;
  UINTN       NumBytes;

  NumBytes = BlockSize;
  Status   = FmpDeviceFlashReadWithRetry (FlashIo, Lba, 0, &NumBytes, VerifyBuffer);
  if (EFI_ERROR (Status) || (NumBytes != BlockSize)) {
    return EFI_DEVICE_ERROR;
  }

  return (CompareMem (VerifyBuffer, Expected, BlockSize) == 0) ? EFI_SUCCESS : EFI_VOLUME_CORRUPTED;
}

/**
  Erase, write and verify a complete flash block with retries.

  @param[in]  FlashIo      Flash operation callbacks.
  @param[in]  Lba          Flash block number.
  @param[in]  Expected     Block contents to program.
  @param[out] VerifyBuffer Scratch buffer for the readback.
  @param[in]  BlockSize    Flash block size.

  @retval EFI_SUCCESS           The block was programmed and verified.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @return                       The final flash operation error.
**/
EFI_STATUS
FmpDeviceFlashProgramWithRetry (
  IN  CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN  EFI_LBA                    Lba,
  IN  CONST UINT8                *Expected,
  OUT UINT8                      *VerifyBuffer,
  IN  UINTN                      BlockSize
  )
{
  EFI_STATUS  Status;
  UINTN       Attempt;
  UINTN       NumBytes;

  if ((FlashIo == NULL) || (FlashIo->Read == NULL) || (FlashIo->Write == NULL) ||
      (FlashIo->Erase == NULL) || (Expected == NULL) || (VerifyBuffer == NULL) || (BlockSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_DEVICE_ERROR;
  for (Attempt = 0; Attempt < FMP_DEVICE_SMM_FLASH_RETRY_COUNT; ++Attempt) {
    Status = FmpDeviceFlashEraseWithRetry (FlashIo, Lba);
    if (EFI_ERROR (Status)) {
      StallAfterFailure (FlashIo, Attempt);
      continue;
    }

    NumBytes = BlockSize;
    Status   = FmpDeviceFlashWriteWithRetry (FlashIo, Lba, 0, &NumBytes, (UINT8 *)Expected);
    if (EFI_ERROR (Status) || (NumBytes != BlockSize)) {
      Status = EFI_DEVICE_ERROR;
      StallAfterFailure (FlashIo, Attempt);
      continue;
    }

    Status = VerifyFlashWrite (FlashIo, Lba, Expected, VerifyBuffer, BlockSize);
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }

    StallAfterFailure (FlashIo, Attempt);
  }

  return Status;
}
