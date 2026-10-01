/** @file
  Arm MMU library instance internal header file.

  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

// Maximum VA bit definitions.
#define MAX_VA_BITS_48    48
#define MAX_VA_BITS_LPA2  52

typedef
VOID(
 EFIAPI  *ARM_REPLACE_LIVE_TRANSLATION_ENTRY
 )(
  IN  UINT64  *Entry,
  IN  UINT64  Value,
  IN  UINT64  RegionStart,
  IN  BOOLEAN DisableMmu
  );

/**
  Update the translation table mappings for a memory region.

  @param[in]  RegionStart             Base address of the memory region.
  @param[in]  RegionLength            Size of the memory region in bytes.
  @param[in]  AttributeSetMask        Descriptor attributes to set.
  @param[in]  AttributeClearMask      Mask of existing descriptor bits to
                                      preserve.
  @param[in]  RootTable               Root translation table to update.
  @param[in]  TableIsLive             TRUE if updates are applied to active
                                      translation tables using break-before-make
                                      handling where required.
  @param[in]  Lpa2Enabled             TRUE if FEAT_LPA2 descriptor format is in
                                      use.
  @param[in]  CcaProtectionAttribute  CCA protection attribute bits to preserve.

  @retval EFI_SUCCESS            The region mapping was updated.
  @retval EFI_INVALID_PARAMETER  RegionStart or RegionLength is not page
                                 aligned.
  @retval EFI_OUT_OF_RESOURCES   A page table allocation failed.
**/
EFI_STATUS
UpdateRegionMapping (
  IN  UINT64   RegionStart,
  IN  UINT64   RegionLength,
  IN  UINT64   AttributeSetMask,
  IN  UINT64   AttributeClearMask,
  IN  UINT64   *RootTable,
  IN  BOOLEAN  TableIsLive,
  IN  BOOLEAN  Lpa2Enabled,
  IN  UINT64   CcaProtectionAttribute
  );
