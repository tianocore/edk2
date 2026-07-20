/** @file
  Internal retry helpers for SMMSTORE-backed firmware updates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>

#include "FmpDeviceSmmFlashRetry.h"

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
