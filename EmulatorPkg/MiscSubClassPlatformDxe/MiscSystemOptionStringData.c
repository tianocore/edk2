/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MiscSystemOptionStringData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_OPTION_STRING_DATA, SystemOptionString) = {
  {STRING_TOKEN(STR_MISC_SYSTEM_OPTION_STRING)}
};

/* eof - MiscSystemOptionStringData.c */
