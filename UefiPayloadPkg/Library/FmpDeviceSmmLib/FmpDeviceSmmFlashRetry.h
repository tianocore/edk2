/** @file
  Internal retry helpers for SMMSTORE-backed firmware updates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

#define FMP_DEVICE_SMM_FLASH_RETRY_COUNT  4

typedef EFI_STATUS (*FMP_DEVICE_FLASH_READ)(
  IN     EFI_LBA  Lba,
  IN     UINTN    Offset,
  IN OUT UINTN    *NumBytes,
  OUT    UINT8    *Buffer
  );

typedef EFI_STATUS (*FMP_DEVICE_FLASH_WRITE)(
  IN     EFI_LBA  Lba,
  IN     UINTN    Offset,
  IN OUT UINTN    *NumBytes,
  IN     UINT8    *Buffer
  );

typedef EFI_STATUS (*FMP_DEVICE_FLASH_ERASE)(
  IN EFI_LBA  Lba
  );

typedef VOID (*FMP_DEVICE_FLASH_STALL)(
  IN UINTN  Attempt
  );

typedef struct {
  FMP_DEVICE_FLASH_READ     Read;
  FMP_DEVICE_FLASH_WRITE    Write;
  FMP_DEVICE_FLASH_ERASE    Erase;
  FMP_DEVICE_FLASH_STALL    Stall;
} FMP_DEVICE_FLASH_IO;

EFI_STATUS
FmpDeviceFlashReadWithRetry (
  IN     CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN     EFI_LBA                    Lba,
  IN     UINTN                      Offset,
  IN OUT UINTN                      *NumBytes,
  OUT    UINT8                      *Buffer
  );

EFI_STATUS
FmpDeviceFlashEraseWithRetry (
  IN CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN EFI_LBA                    Lba
  );

EFI_STATUS
FmpDeviceFlashWriteWithRetry (
  IN     CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN     EFI_LBA                    Lba,
  IN     UINTN                      Offset,
  IN OUT UINTN                      *NumBytes,
  IN     UINT8                      *Buffer
  );

EFI_STATUS
FmpDeviceFlashProgramWithRetry (
  IN  CONST FMP_DEVICE_FLASH_IO  *FlashIo,
  IN  EFI_LBA                    Lba,
  IN  CONST UINT8                *Expected,
  OUT UINT8                      *VerifyBuffer,
  IN  UINTN                      BlockSize
  );
