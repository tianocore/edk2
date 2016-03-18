/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to smbios.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"

//
// Static (possibly build generated) OEM String data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_OEM_STRING, MiscOemString)
= { {STRING_TOKEN(STR_MISC_OEM_EN_US) }};
