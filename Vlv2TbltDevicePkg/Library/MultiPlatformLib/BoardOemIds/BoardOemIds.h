/**@file
  ACPI oem ids setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
**/

#include <Guid/PlatformInfo.h>
#include <Library/BaseMemoryLib.h>

#define EFI_ACPI_OEM_ID_DEFAULT    SIGNATURE_64('I', 'N', 'T', 'E', 'L', ' ', ' ', ' ')     // max 6 chars
#define EFI_ACPI_OEM_ID1           SIGNATURE_64('I', 'N', 'T', 'E', 'L', '1', ' ', ' ')     // max 6 chars
#define EFI_ACPI_OEM_ID2           SIGNATURE_64('I', 'N', 'T', 'E', 'L', '2', ' ', ' ')     // max 6 chars

#define EFI_ACPI_OEM_TABLE_ID_DEFAULT   SIGNATURE_64('E', 'D', 'K', '2', ' ', ' ', ' ', ' ')
#define EFI_ACPI_OEM_TABLE_ID1          SIGNATURE_64('E', 'D', 'K', '2', '_', '1', ' ', ' ')
#define EFI_ACPI_OEM_TABLE_ID2          SIGNATURE_64('E', 'D', 'K', '2', '_', '2', ' ', ' ')


EFI_STATUS
InitializeBoardOemId (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PLATFORM_INFO_HOB        *PlatformInfoHob
  );
