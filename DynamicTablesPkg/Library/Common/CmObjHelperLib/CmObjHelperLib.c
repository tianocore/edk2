/** @file
  Configuration Manager Helper Library.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <MetadataHelpers.h>
#include <Library/MetadataHandlerLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/**
  This macro expands to a function that retrieves the ACPI Table list
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceStandard,
  EStdObjAcpiTableList,
  CM_STD_OBJ_ACPI_TABLE_INFO
  );

/** Check if an ACPI table is present in the Configuration manager's ACPI table list.

  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  AcpiTableId            Acpi Table Id.

  @retval TRUE if the ACPI table is in the list of ACPI tables to install.
          FALSE otherwise.
**/
BOOLEAN
EFIAPI
CheckAcpiTablePresent (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        ESTD_ACPI_TABLE_ID                            AcpiTableId
  )
{
  EFI_STATUS                  Status;
  CM_STD_OBJ_ACPI_TABLE_INFO  *AcpiTableList;
  UINT32                      AcpiTableListCount;
  UINT32                      Index;

  Status = GetEStdObjAcpiTableList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AcpiTableList,
             &AcpiTableListCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return FALSE;
  }

  for (Index = 0; Index < AcpiTableListCount; Index++) {
    if (AcpiTableList[Index].TableGeneratorId ==
        CREATE_STD_ACPI_TABLE_GEN_ID (AcpiTableId))
    {
      return TRUE;
    }
  }

  return FALSE;
}
