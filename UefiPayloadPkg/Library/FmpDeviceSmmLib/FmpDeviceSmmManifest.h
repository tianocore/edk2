/** @file
  Internal RMAP manifest parsing for SMMSTORE-backed firmware updates.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Base.h>

#define REGION_MANIFEST_SIGNATURE  SIGNATURE_32 ('R', 'M', 'A', 'P')
#define REGION_MANIFEST_VERSION    1

#pragma pack(1)
typedef struct {
  UINT32    Signature;
  UINT16    Version;
  UINT16    EntryCount;
} REGION_MANIFEST_TRAILER;

typedef struct {
  CHAR8    RegionName[16];
} REGION_MANIFEST_ENTRY;
#pragma pack()

EFI_STATUS
FmpDeviceLocateRegionManifest (
  IN  CONST UINT8                  *Image,
  IN  UINTN                        ImageSize,
  OUT UINTN                        *EntryCount,
  OUT CONST REGION_MANIFEST_ENTRY  **Entries,
  OUT UINTN                        *FirmwareImageSize
  );

EFI_STATUS
FmpDeviceGetFlashRangeStepCount (
  IN  UINTN  Offset,
  IN  UINTN  Size,
  IN  UINTN  BlockSize,
  OUT UINTN  *StepCount
  );
