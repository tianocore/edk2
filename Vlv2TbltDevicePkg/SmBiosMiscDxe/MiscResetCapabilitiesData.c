/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

    MiscResetCapabilitiesData.c

Abstract:

  Static data of Reset Capabilities information.
  Reset Capabilities information is Misc. subclass type 17 and SMBIOS type 23.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_RESET_CAPABILITIES, MiscResetCapabilities)
= {
  {       // ResetCapabilities
    0,    // Status
    0,    // BootOption
    0,    // BootOptionOnLimit
    0,    // WatchdogTimerPresent
    0     // Reserved
  },
  0xFFFF, // ResetCount
  0xFFFF, // ResetLimit
  0xFFFF, // ResetTimerInterval
  0xFFFF  // ResetTimeout
};
