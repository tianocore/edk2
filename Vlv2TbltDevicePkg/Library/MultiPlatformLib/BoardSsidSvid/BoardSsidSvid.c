/** @file
  Subsystem IDs setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include <BoardSsidSvid.h>

//
// Global module data
//
EFI_STATUS
InitializeBoardSsidSvid (
    IN CONST EFI_PEI_SERVICES       **PeiServices,
    IN EFI_PLATFORM_INFO_HOB        *PlatformInfoHob
  )
{
    UINT32  SsidSvidValue = 0;

    //
    // Set OEM ID according to Board ID.
    //
    switch (PlatformInfoHob->BoardId) {
      case BOARD_ID_MINNOW2:
             default:
                  SsidSvidValue = SUBSYSTEM_SVID_SSID;//SUBSYSTEM_SVID_SSID_DEFAULT;
                  break;
      }
    PlatformInfoHob->SsidSvid = SsidSvidValue;
    return  EFI_SUCCESS;
}

