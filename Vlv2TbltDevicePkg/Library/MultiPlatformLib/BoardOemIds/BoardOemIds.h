/**@file
  ACPI oem ids setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   
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
