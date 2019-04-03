/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MiscSystemLanguageStringData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA, SystemLanguageString) = {
  0,
  STRING_TOKEN(STR_MISC_SYSTEM_LANGUAGE_STRING)
};

/* eof - MiscSystemLanguageStringData.c */
