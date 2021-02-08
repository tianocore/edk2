/** @file
  This file provides Smbios Type13 Data

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

//
// Static (possibly build generated) Bios Vendor data.
//

SMBIOS_MISC_TABLE_DATA(SMBIOS_TABLE_TYPE13, MiscNumberOfInstallableLanguages) =
{
  {                                                     // Hdr
    EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION,            // Type,
    0,                                                    // Length,
    0                                                     // Handle
  },
  0,                                                    // InstallableLanguages
  0,                                                    // Flags
  {
    0                                                   // Reserved[15]
  },
  1                                                     // CurrentLanguage
};
