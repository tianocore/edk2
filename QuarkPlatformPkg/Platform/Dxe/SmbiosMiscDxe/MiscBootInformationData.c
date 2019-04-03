/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to the DataHub.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data. SMBIOS TYPE 32
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_BOOT_INFORMATION_STATUS, MiscBootInfoStatus) = {
  EfiBootInformationStatusNoError,  // BootInformationStatus
  {0}                               // BootInformationData
};
