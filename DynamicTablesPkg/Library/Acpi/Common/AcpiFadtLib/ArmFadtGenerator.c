/** @file
  ARM FADT Table Helpers

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "FadtGenerator.h"

/** This macro expands to a function that retrieves the Boot
    Architecture Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjBootArchInfo,
  CM_ARCH_BOOT_ARCH_INFO
  );

/** Updates the Architecture specific information in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Fadt           Pointer to the FADT table

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         The required object was not found.
**/
EFI_STATUS
EFIAPI
FadtAddArchInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE           *Fadt
  )
{
  EFI_STATUS              Status;
  CM_ARCH_BOOT_ARCH_INFO  *BootArchInfo;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the Boot Architecture flags from the Platform Configuration Manager
  Status = GetEArchObjBootArchInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BootArchInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FADT: Failed to get Boot Architecture flags. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "FADT BootArchFlag = 0x%x\n",
    BootArchInfo->BootArchFlags
    ));

  Fadt->ArmBootArch = BootArchInfo->BootArchFlags;

error_handler:
  return Status;
}
