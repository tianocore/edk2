/** @file
  ACPI oem ids setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

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
      default:
        OemId = EFI_ACPI_OEM_ID_DEFAULT;
        OemTableId = EFI_ACPI_OEM_TABLE_ID_DEFAULT;
        break;
    }

    PlatformInfoHob->AcpiOemId = OemId;
    PlatformInfoHob->AcpiOemTableId = OemTableId;
    return  EFI_SUCCESS;
}

