/** @file
  Subsystem IDs setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

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
      case BOARD_ID_MINNOW2_TURBOT:
      default:
        SsidSvidValue = SUBSYSTEM_SVID_SSID;//SUBSYSTEM_SVID_SSID_DEFAULT;
        break;
      }
    PlatformInfoHob->SsidSvid = SsidSvidValue;
    return  EFI_SUCCESS;
}

