/** @file
  GUID for the HOB that contains the TPM CRB info.
  This HOB is used to get information of CRB regions from firmware
  not from fixed build information.

  Copyright (C) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - el3_spmc's usage in TF-A passing this HOB to StandaloneMm
    (https://review.trustedfirmware.org/c/TF-A/trusted-firmware-a/+/55038)

**/

#pragma once

#define TPM_CRB_INFO_HOB_GUID  {\
          0x1b97b3b4, 0xbd84, 0x11f1, \
          { 0xa8, 0xbc, 0x8f, 0xa5, 0x8b, 0x7c, 0xd7, 0x9e } \
        }

/// Defines the structure for TPM CRB region descriptor in Hob.
typedef struct TpmCrbRegionDesc {
  EFI_PHYSICAL_ADDRESS    PhysicalAddress; ///< Physical address of CRB
  EFI_PHYSICAL_ADDRESS    VirtualAddress;  ///< Virtual address of CRB
  UINT64                  Size;            ///< Size of CRB region
} TPM_CRB_REGION_DESC;

/// Defines the structure for HOB data including TPM CRB regions.
typedef struct TpmCrbInfoHobData {
  /*
   * Number of localities. Currently, valid values are 4 and 5.
   * firmware may hide the secure CRB information for locality 4,
   * exposing only four localities.
   *
   * CrbRegions[] is allocated with this many entries.
   */
  UINT32                 NumberOfLocality;

  /*
   * CRB region for each locality.
   * Use the locality number as the index to access its CRB region.
   */
  TPM_CRB_REGION_DESC    CrbRegions[];
} TPM_CRB_INFO_HOB_DATA;

extern EFI_GUID  gArmTpmCrbInfoHobGuid;
