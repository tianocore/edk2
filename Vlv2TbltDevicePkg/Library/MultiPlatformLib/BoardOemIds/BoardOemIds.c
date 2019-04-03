/** @file
  ACPI oem ids setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#include <BoardOemIds.h>

//
// Global module data
//
EFI_STATUS
InitializeBoardOemId (
  IN CONST EFI_PEI_SERVICES       **PeiServices,
  IN EFI_PLATFORM_INFO_HOB        *PlatformInfoHob
  )
{
    UINT64  OemId;
    UINT64  OemTableId;

    //
    // Set OEM ID according to Board ID.
    //
    switch (PlatformInfoHob->BoardId) {

      case BOARD_ID_MINNOW2:
      case BOARD_ID_MINNOW2_TURBOT:
      default:
        OemId = EFI_ACPI_OEM_ID_DEFAULT;
        OemTableId = EFI_ACPI_OEM_TABLE_ID_DEFAULT;
        break;
    }

    PlatformInfoHob->AcpiOemId = OemId;
    PlatformInfoHob->AcpiOemTableId = OemTableId;
    return  EFI_SUCCESS;
}

