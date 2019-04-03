/** @file
This driver parses the mSmbiosMiscDataTable structure and reports
any generated data to SMBIOS.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES, NumberOfInstallableLanguages) = {
  2,                                  // NumberOfInstallableLanguages
  {                                   // LanguageFlags
    0,                                // AbbreviatedLanguageFormat
    0                                 // Reserved
  },
  1,                                  // CurrentLanguageNumber
};
