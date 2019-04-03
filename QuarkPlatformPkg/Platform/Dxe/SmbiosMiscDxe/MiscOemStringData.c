/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to smbios.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"

//
// Static (possibly build generated) OEM String data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_OEM_STRING, MiscOemString)
= { {STRING_TOKEN(STR_MISC_OEM_EN_US) }};
