/** @file
This driver parses the mSmbiosMiscDataTable structure and reports
any generated data to smbios.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_OPTION_STRING, SystemOptionString) = {
  {STRING_TOKEN (STR_MISC_SYSTEM_OPTION_STRING)}
};
