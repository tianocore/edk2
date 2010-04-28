/**@file

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    MiscResetCapabilitiesData.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubclassDriver.h"

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
