/** @file
  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

//
// Static (possibly build generated) Bios Vendor data.
//
SMBIOS_MISC_TABLE_DATA (SMBIOS_TABLE_TYPE32, MiscBootInformation) = {
  {                                                     // Hdr
    EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION,            // Type,
    0,                                                  // Length,
    0                                                   // Handle
  },
  {                                                     // Reserved[6]
    0,
    0,
    0,
    0,
    0,
    0
  },
  BootInformationStatusNoError                          // BootInformationStatus
};
