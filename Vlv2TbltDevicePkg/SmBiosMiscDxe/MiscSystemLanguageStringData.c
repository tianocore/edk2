/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscSystemLanguageStringData.c

Abstract:

  Static data of System language string information.
  System language string information is Misc. subclass type 12 and SMBIOS type 13.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA, SystemLanguageString)
= {
  0,
  STRING_TOKEN(STR_MISC_SYSTEM_LANGUAGE_EN_US)
};
