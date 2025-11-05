/** @file
  This file provides SMBIOS Misc Type.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent\

**/

#include "SmbiosMisc.h"

SMBIOS_MISC_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE0,
  MiscBiosVendor,
  MiscBiosVendor
  )
SMBIOS_MISC_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE1,
  MiscSystemManufacturer,
  MiscSystemManufacturer
  )
SMBIOS_MISC_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE3,
  MiscChassisManufacturer,
  MiscChassisManufacturer
  )
SMBIOS_MISC_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE2,
  MiscBaseBoardManufacturer,
  MiscBaseBoardManufacturer
  )
SMBIOS_MISC_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE13,
  MiscNumberOfInstallableLanguages,
  MiscNumberOfInstallableLanguages
  )
SMBIOS_MISC_TABLE_EXTERNS (
  SMBIOS_TABLE_TYPE32,
  MiscBootInformation,
  MiscBootInformation
  )

SMBIOS_MISC_DATA_TABLE mSmbiosMiscDataTable[] = {
  // Type0
  SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION (
    MiscBiosVendor,
    MiscBiosVendor
    ),
  // Type1
  SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION (
    MiscSystemManufacturer,
    MiscSystemManufacturer
    ),
  // Type3
  SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION (
    MiscChassisManufacturer,
    MiscChassisManufacturer
    ),
  // Type2
  SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION (
    MiscBaseBoardManufacturer,
    MiscBaseBoardManufacturer
    ),
  // Type13
  SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION (
    MiscNumberOfInstallableLanguages,
    MiscNumberOfInstallableLanguages
    ),
  // Type32
  SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION (
    MiscBootInformation,
    MiscBootInformation
    ),
};

//
// Number of Data Table entries.
//
UINTN  mSmbiosMiscDataTableEntries =
  (sizeof (mSmbiosMiscDataTable)) / sizeof (SMBIOS_MISC_DATA_TABLE);
