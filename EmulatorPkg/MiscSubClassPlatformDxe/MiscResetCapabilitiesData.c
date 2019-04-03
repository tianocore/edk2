/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

    MiscResetCapabilitiesData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_RESET_CAPABILITIES, MiscResetCapabilities) = {
  {     // ResetCapabilities
    0,  // Status
    0,  // BootOption
    0,  // BootOptionOnLimit
    0,  // WatchdogTimerPresent
    0   // Reserved
  },
  0,    // ResetCount
  0,    // ResetLimit
  0,    // ResetTimerInterval
  0     // ResetTimeout
};

/* eof - MiscResetCapabilities.c */
