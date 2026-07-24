/** @file
  Internal RMAP manifest parsing for SMMSTORE-backed firmware updates.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>

#include "FmpDeviceSmmManifest.h"

/**
  Check that a manifest region name is printable and NUL-padded.

  @param[in] Name  Region name to validate.

  @retval TRUE   The region name is valid.
  @retval FALSE  The region name is invalid.
**/
STATIC
BOOLEAN
RegionNameIsValid (
  IN CONST CHAR8  Name[16]
  )
{
  BOOLEAN  HasCharacter;
  BOOLEAN  Terminated;
  UINTN    Index;

  HasCharacter = FALSE;
  Terminated   = FALSE;

  for (Index = 0; Index < 16; ++Index) {
    if (Name[Index] == '\0') {
      Terminated = TRUE;
      continue;
    }

    if (Terminated || (Name[Index] <= ' ') || (Name[Index] > '~')) {
      return FALSE;
    }

    HasCharacter = TRUE;
  }

  return HasCharacter;
}

/**
  Locate and validate an RMAP manifest appended to a firmware image.

  @param[in]  Image              Firmware image.
  @param[in]  ImageSize          Size of Image, in bytes.
  @param[out] EntryCount         Number of manifest entries.
  @param[out] Entries            Pointer to the manifest entries.
  @param[out] FirmwareImageSize  Size of the firmware data before the manifest.

  @retval EFI_SUCCESS           A valid manifest was found.
  @retval EFI_NOT_FOUND         The image has no manifest.
  @retval EFI_COMPROMISED_DATA  The manifest is malformed.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
FmpDeviceLocateRegionManifest (
  IN  CONST UINT8                  *Image,
  IN  UINTN                        ImageSize,
  OUT UINTN                        *EntryCount,
  OUT CONST REGION_MANIFEST_ENTRY  **Entries,
  OUT UINTN                        *FirmwareImageSize
  )
{
  REGION_MANIFEST_TRAILER      Trailer;
  CONST REGION_MANIFEST_ENTRY  *ManifestEntries;
  UINTN                        EntriesSize;
  UINTN                        ManifestStart;
  UINTN                        EntryIndex;
  UINTN                        OtherIndex;

  if ((Image == NULL) || (EntryCount == NULL) || (Entries == NULL) || (FirmwareImageSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *EntryCount        = 0;
  *Entries           = NULL;
  *FirmwareImageSize = ImageSize;

  if (ImageSize < sizeof (Trailer)) {
    return EFI_NOT_FOUND;
  }

  CopyMem (&Trailer, Image + ImageSize - sizeof (Trailer), sizeof (Trailer));
  if (Trailer.Signature != REGION_MANIFEST_SIGNATURE) {
    return EFI_NOT_FOUND;
  }

  if ((Trailer.Version != REGION_MANIFEST_VERSION) || (Trailer.EntryCount == 0)) {
    return EFI_COMPROMISED_DATA;
  }

  if (Trailer.EntryCount > ((ImageSize - sizeof (Trailer)) / sizeof (REGION_MANIFEST_ENTRY))) {
    return EFI_COMPROMISED_DATA;
  }

  EntriesSize   = (UINTN)Trailer.EntryCount * sizeof (REGION_MANIFEST_ENTRY);
  ManifestStart = ImageSize - sizeof (Trailer) - EntriesSize;
  if (ManifestStart == 0) {
    return EFI_COMPROMISED_DATA;
  }

  ManifestEntries = (CONST REGION_MANIFEST_ENTRY *)(Image + ManifestStart);
  for (EntryIndex = 0; EntryIndex < Trailer.EntryCount; ++EntryIndex) {
    if (!RegionNameIsValid (ManifestEntries[EntryIndex].RegionName)) {
      return EFI_COMPROMISED_DATA;
    }

    for (OtherIndex = 0; OtherIndex < EntryIndex; ++OtherIndex) {
      if (CompareMem (
            ManifestEntries[EntryIndex].RegionName,
            ManifestEntries[OtherIndex].RegionName,
            sizeof (ManifestEntries[EntryIndex].RegionName)
            ) == 0)
      {
        return EFI_COMPROMISED_DATA;
      }
    }
  }

  *EntryCount        = Trailer.EntryCount;
  *Entries           = ManifestEntries;
  *FirmwareImageSize = ManifestStart;
  return EFI_SUCCESS;
}

/**
  Calculate the erase and write steps needed to update a flash range.

  @param[in]  Offset     Flash range offset.
  @param[in]  Size       Flash range size.
  @param[in]  BlockSize  Flash erase block size.
  @param[out] StepCount  Number of erase and write steps.

  @retval EFI_SUCCESS           StepCount was calculated.
  @retval EFI_BAD_BUFFER_SIZE   The range or step count overflows.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
FmpDeviceGetFlashRangeStepCount (
  IN  UINTN  Offset,
  IN  UINTN  Size,
  IN  UINTN  BlockSize,
  OUT UINTN  *StepCount
  )
{
  UINTN  FirstBlock;
  UINTN  LastBlock;
  UINTN  BlockCount;

  if ((Size == 0) || (BlockSize == 0) || (StepCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Offset > (MAX_UINTN - (Size - 1))) {
    return EFI_BAD_BUFFER_SIZE;
  }

  FirstBlock = Offset / BlockSize;
  LastBlock  = (Offset + Size - 1) / BlockSize;
  BlockCount = LastBlock - FirstBlock + 1;
  if (BlockCount > (MAX_UINTN / 2)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  *StepCount = BlockCount * 2;
  return EFI_SUCCESS;
}
