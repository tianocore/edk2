/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MiscNumberOfInstallableLanguagesData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA, NumberOfInstallableLanguages)
= {
  1,    // NumberOfInstallableLanguages
  {     // LanguageFlags
    0,  // AbbreviatedLanguageFormat
    0   // Reserved
  },
  0,    // CurrentLanguageNumber
};

/* eof - MiscNumberOfInstallableLanguagesData.c */
