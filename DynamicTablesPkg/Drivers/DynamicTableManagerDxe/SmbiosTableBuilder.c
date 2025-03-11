/** @file
  SMBIOS Table Builder.

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>
#include <Library/BaseMemoryLib.h>

// Module specific include files.
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <DeviceTreeTableGenerator.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <SmbiosTableGenerator.h>
#include <SmbiosTableDispatcher.h>

/** This macro expands to a function that retrieves the SMBIOS Table
    List from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceStandard,
  EStdObjSmbiosTableList,
  CM_STD_OBJ_SMBIOS_TABLE_INFO
  )

/** A helper function to build and install an SMBIOS table.

  This is a helper function that invokes the Table generator interface
  for building an SMBIOS table. It uses the SmbiosProtocol to install the
  table, then frees the resources allocated for generating it.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  Generator            Pointer to the SMBIOS table Generator.
  @param [in]  SmbiosProtocol       Pointer to the SMBIOS protocol.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Required object is not found.
  @retval EFI_BAD_BUFFER_SIZE   Size returned by the Configuration Manager
                                is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildAndInstallSingleSmbiosTable (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN CONST SMBIOS_TABLE_GENERATOR                *CONST  Generator,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo
  )
{
  EFI_STATUS         Status;
  EFI_STATUS         Status1;
  SMBIOS_STRUCTURE   *SmbiosTable;
  CM_OBJECT_TOKEN    CmObjToken;
  EFI_SMBIOS_HANDLE  TableHandle;

  SmbiosTable = NULL;
  Status      = Generator->BuildSmbiosTable (
                             Generator,
                             TableFactoryProtocol,
                             SmbiosTableInfo,
                             CfgMgrProtocol,
                             &SmbiosTable,
                             &CmObjToken
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Build Table." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      SmbiosTableInfo->TableGeneratorId,
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  if (SmbiosTable == NULL) {
    Status = EFI_NOT_FOUND;
    goto exit_handler;
  }

  TableHandle = SMBIOS_HANDLE_PI_RESERVED;

  // Install SMBIOS table
  Status = SmbiosProtocol->Add (
                             SmbiosProtocol,
                             NULL,
                             &TableHandle,
                             SmbiosTable
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Install SMBIOS Table. Status = %r\n",
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  Status = TableFactoryProtocol->AddSmbiosHandle (
                                   SmbiosProtocol,
                                   &TableHandle,
                                   CmObjToken,
                                   SmbiosTableInfo->TableGeneratorId
                                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Add SMBIOS Handle. Status = %r\n",
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: SMBIOS Table installed. Status = %r\n",
    Status
    ));

exit_handler:
  // Free any resources allocated for generating the tables.
  if (Generator->FreeTableResources != NULL) {
    Status1 = Generator->FreeTableResources (
                           Generator,
                           TableFactoryProtocol,
                           SmbiosTableInfo,
                           CfgMgrProtocol,
                           &SmbiosTable
                           );
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Free Table Resources." \
        "TableGeneratorId = 0x%x. Status = %r\n",
        SmbiosTableInfo->TableGeneratorId,
        Status1
        ));
    }

    // Return the first error status in case of failure
    if (!EFI_ERROR (Status)) {
      Status = Status1;
    }
  }

  return Status;
}

/** A helper function to build and install multiple SMBIOS tables.

  This is a helper function that invokes the Table generator interface
  for building SMBIOS tables. It uses the SmbiosProtocol to install the
  tables, then frees the resources allocated for generating it.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  Generator            Pointer to the SmbiosTable generator.
  @param [in]  SmbiosProtocol       Pointer to the Smbios protocol.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Required object is not found.
  @retval EFI_BAD_BUFFER_SIZE   Size returned by the Configuration Manager
                                is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildAndInstallMultipleSmbiosTables (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN CONST SMBIOS_TABLE_GENERATOR                *CONST  Generator,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo
  )
{
  EFI_STATUS         Status;
  EFI_STATUS         Status1;
  SMBIOS_STRUCTURE   **SmbiosTable;
  CM_OBJECT_TOKEN    *CmObjToken;
  EFI_SMBIOS_HANDLE  TableHandle;
  UINTN              TableCount;
  UINTN              Index;

  TableCount = 0;
  Status     = Generator->BuildSmbiosTableEx (
                            Generator,
                            TableFactoryProtocol,
                            SmbiosTableInfo,
                            CfgMgrProtocol,
                            &SmbiosTable,
                            &CmObjToken,
                            &TableCount
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Build Table." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      SmbiosTableInfo->TableGeneratorId,
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  if ((SmbiosTable == NULL) || (TableCount == 0)) {
    Status = EFI_NOT_FOUND;
    DEBUG ((
      DEBUG_ERROR,
      "%a: TableCount %u SmbiosTable %p \n",
      __func__,
      TableCount,
      SmbiosTable
      ));
    goto exit_handler;
  }

  for (Index = 0; Index < TableCount; Index++) {
    TableHandle = SMBIOS_HANDLE_PI_RESERVED;

    // Install SMBIOS table
    Status = SmbiosProtocol->Add (
                               SmbiosProtocol,
                               NULL,
                               &TableHandle,
                               SmbiosTable[Index]
                               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Install SMBIOS Table. Status = %r\n",
        Status
        ));
      // Free any allocated resources.
      goto exit_handler;
    }

    Status = TableFactoryProtocol->AddSmbiosHandle (
                                     SmbiosProtocol,
                                     &TableHandle,
                                     CmObjToken[Index],
                                     SmbiosTableInfo->TableGeneratorId
                                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Add SMBIOS Handle. Status = %r\n",
        Status
        ));
      // Free any allocated resources.
      goto exit_handler;
    }

    DEBUG ((
      DEBUG_INFO,
      "INFO: SMBIOS Table installed. Status = %r\n",
      Status
      ));
  }

exit_handler:
  // Free any resources allocated for generating the tables.
  if (Generator->FreeTableResourcesEx != NULL) {
    Status1 = Generator->FreeTableResourcesEx (
                           Generator,
                           TableFactoryProtocol,
                           SmbiosTableInfo,
                           CfgMgrProtocol,
                           &SmbiosTable,
                           &CmObjToken,
                           TableCount
                           );
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Free Table Resources." \
        "TableGeneratorId = 0x%x. Status = %r\n",
        SmbiosTableInfo->TableGeneratorId,
        Status1
        ));
    }

    // Return the first error status in case of failure
    if (!EFI_ERROR (Status)) {
      Status = Status1;
    }
  }

  return Status;
}

/** A helper function to invoke a Table generator

  This is a helper function that invokes the Table generator interface
  for building an SMBIOS table. It uses the SmbiosProtocol to install the
  table, then frees the resources allocated for generating it.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  SmbiosProtocol       Pointer to the SMBIOS protocol.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Required object is not found.
  @retval EFI_BAD_BUFFER_SIZE   Size returned by the Configuration Manager
                                is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
BuildAndInstallSmbiosTable (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo
  )
{
  EFI_STATUS                    Status;
  CONST SMBIOS_TABLE_GENERATOR  *Generator;

  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (SmbiosProtocol != NULL);
  ASSERT (SmbiosTableInfo != NULL);

  DEBUG ((
    DEBUG_INFO,
    "INFO: EStdObjSmbiosTableList: Address = 0x%p," \
    " TableGeneratorId = 0x%x\n",
    SmbiosTableInfo,
    SmbiosTableInfo->TableGeneratorId
    ));

  Generator = NULL;
  Status    = TableFactoryProtocol->GetSmbiosTableGenerator (
                                      TableFactoryProtocol,
                                      SmbiosTableInfo->TableGeneratorId,
                                      &Generator
                                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Table Generator not found." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      SmbiosTableInfo->TableGeneratorId,
      Status
      ));
    return Status;
  }

  if (Generator == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Generator found : %s\n",
    Generator->Description
    ));

  if (Generator->BuildSmbiosTableEx != NULL) {
    Status = BuildAndInstallMultipleSmbiosTables (
               TableFactoryProtocol,
               CfgMgrProtocol,
               Generator,
               SmbiosProtocol,
               SmbiosTableInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to find build and install SMBIOS Tables." \
        " Status = %r\n",
        Status
        ));
    }
  } else if (Generator->BuildSmbiosTable != NULL) {
    Status = BuildAndInstallSingleSmbiosTable (
               TableFactoryProtocol,
               CfgMgrProtocol,
               Generator,
               SmbiosProtocol,
               SmbiosTableInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to find build and install SMBIOS Table." \
        " Status = %r\n",
        Status
        ));
    }
  } else {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Table Generator does not implement the" \
      "SMBIOS_TABLE_GENERATOR_BUILD_TABLE  interface." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      SmbiosTableInfo->TableGeneratorId,
      Status
      ));
  }

  return Status;
}

/** Generate and install SMBIOS tables.

  The function gathers the information necessary for installing the
  SMBIOS tables from the Configuration Manager, invokes the generators
  and installs them (via BuildAndInstallAcpiTable).

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.

  @retval EFI_SUCCESS   Success.
  @retval EFI_NOT_FOUND If a mandatory table or a generator is not found.
**/
STATIC
EFI_STATUS
EFIAPI
ProcessSmbiosTables (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  EFI_STATUS                    Status;
  EFI_SMBIOS_PROTOCOL           *SmbiosProtocol;
  CM_STD_OBJ_SMBIOS_TABLE_INFO  *SmbiosTableInfo;
  UINT32                        SmbiosTableCount;

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&SmbiosProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", Status));
    return Status;
  }

  Status = GetEStdObjSmbiosTableList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SmbiosTableInfo,
             &SmbiosTableCount
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      // If the SMBIOS table list is not found the platform
      // may not be publishing SMBIOS tables
      // Log a warning and return success.
      DEBUG ((
        DEBUG_WARN,
        "WARNING: Failed to get SMBIOS table List. Status = %r\n",
        Status
        ));
      Status = EFI_SUCCESS;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to get SMBIOS Table List. Status = %r\n",
        Status
        ));
    }

    return Status;
  }

  if (SmbiosTableCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EStdObjSmbiosTableList: SmbiosTableCount = %d\n",
      SmbiosTableCount
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: EStdObjSmbiosTableList: SmbiosTableCount = %d\n",
    SmbiosTableCount
    ));

  InitSmbiosTableDispatcher (SmbiosTableInfo, SmbiosTableCount);

  Status = DispatchSmbiosTables (
             TableFactoryProtocol,
             CfgMgrProtocol,
             SmbiosProtocol,
             SmbiosTableInfo,
             SmbiosTableCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to install SMBIOS Tables.Status = %r\n",
      Status
      ));
  }

  return Status;
}

/** SMBIOS table Protocol ready event handler.

  This event notification indicates that the SMBIOS protocol is ready.
  Therefore, dispatch the building of the SMBIOS tables.

  @param  [in]  Event     The Event that is signalled.
  @param  [in]  Context   The Context information.

  @retval None
**/
VOID
EFIAPI
SmbiosProtocolReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                             Status;
  EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CfgMgrProtocol;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO  *CfgMfrInfo;
  EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *TableFactoryProtocol;

  // Locate the Dynamic Table Factory
  Status = gBS->LocateProtocol (
                  &gEdkiiDynamicTableFactoryProtocolGuid,
                  NULL,
                  (VOID **)&TableFactoryProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to find Dynamic Table Factory protocol." \
      " Status = %r\n",
      Status
      ));
    return;
  }

  // Locate the Configuration Manager for the Platform
  Status = gBS->LocateProtocol (
                  &gEdkiiConfigurationManagerProtocolGuid,
                  NULL,
                  (VOID **)&CfgMgrProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to find Configuration Manager protocol. Status = %r\n",
      Status
      ));
    return;
  }

  Status = GetCgfMgrInfo (CfgMgrProtocol, &CfgMfrInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Configuration Manager info. Status = %r\n",
      Status
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: Configuration Manager Version = 0x%x, OemID = %c%c%c%c%c%c\n",
    CfgMfrInfo->Revision,
    CfgMfrInfo->OemId[0],
    CfgMfrInfo->OemId[1],
    CfgMfrInfo->OemId[2],
    CfgMfrInfo->OemId[3],
    CfgMfrInfo->OemId[4],
    CfgMfrInfo->OemId[5]
    ));

  Status = ProcessSmbiosTables (TableFactoryProtocol, CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SMBIOS Table processing failure. Status = %r\n",
      Status
      ));
  }

  Status = gBS->CloseEvent (Event);
  ASSERT_EFI_ERROR (Status);
}
