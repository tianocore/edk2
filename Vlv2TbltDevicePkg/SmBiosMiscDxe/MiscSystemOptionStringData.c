/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscSystemOptionStringData.c

Abstract:

  Static data of System option string.
  System option string is Miscellaneous subclass type: 10 and SMBIOS type: 13.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_OPTION_STRING_DATA, SystemOptionString)
= { STRING_TOKEN(STR_MISC_SYSTEM_OPTION_EN_US) };
