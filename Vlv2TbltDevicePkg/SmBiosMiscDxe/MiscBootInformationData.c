/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscBootInformationData.c

Abstract:

  Static data of Boot information.
  Boot information is Misc. subclass type 26 and SMBIOS type 32.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_BOOT_INFORMATION_STATUS_DATA, BootInformationStatus)
= {
  EfiBootInformationStatusNoError,  // BootInformationStatus
  0                                 // BootInformationData
};
