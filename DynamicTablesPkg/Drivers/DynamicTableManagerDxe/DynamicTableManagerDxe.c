/** @file
  Dynamic Table Manager Dxe

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <DeviceTreeTableGenerator.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <SmbiosTableGenerator.h>

/** This macro expands to a function that retrieves the ACPI Table
    List from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceStandard,
  EStdObjAcpiTableList,
  CM_STD_OBJ_ACPI_TABLE_INFO
  )

/** A helper function to build and install a single ACPI table.

  This is a helper function that invokes the Table generator interface
  for building an ACPI table. It uses the AcpiTableProtocol to install the
  table, then frees the resources allocated for generating it.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  Generator            Pointer to the AcpiTable generator.
  @param [in]  AcpiTableProtocol    Pointer to the AcpiTable protocol.
  @param [in]  AcpiTableInfo        Pointer to the ACPI table Info.

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
BuildAndInstallSingleAcpiTable (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN CONST ACPI_TABLE_GENERATOR                  *CONST  Generator,
  IN       EFI_ACPI_TABLE_PROTOCOL                       *AcpiTableProtocol,
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   Status1;
  EFI_ACPI_DESCRIPTION_HEADER  *AcpiTable;
  UINTN                        TableHandle;

  AcpiTable = NULL;
  Status    = Generator->BuildAcpiTable (
                           Generator,
                           AcpiTableInfo,
                           CfgMgrProtocol,
                           &AcpiTable
                           );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Build Table." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      AcpiTableInfo->TableGeneratorId,
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  if (AcpiTable == NULL) {
    Status = EFI_NOT_FOUND;
    goto exit_handler;
  }

  // Dump ACPI Table Header
  DUMP_ACPI_TABLE_HEADER (AcpiTable);

  // Install ACPI table
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                AcpiTable,
                                AcpiTable->Length,
                                &TableHandle
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Install ACPI Table. Status = %r\n",
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: ACPI Table installed. Status = %r\n",
    Status
    ));

exit_handler:
  // Free any resources allocated for generating the tables.
  if (Generator->FreeTableResources != NULL) {
    Status1 = Generator->FreeTableResources (
                           Generator,
                           AcpiTableInfo,
                           CfgMgrProtocol,
                           &AcpiTable
                           );
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Free Table Resources." \
        "TableGeneratorId = 0x%x. Status = %r\n",
        AcpiTableInfo->TableGeneratorId,
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

/** A helper function to build and install multiple ACPI tables.

  This is a helper function that invokes the Table generator interface
  for building an ACPI table. It uses the AcpiTableProtocol to install the
  table, then frees the resources allocated for generating it.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  Generator            Pointer to the AcpiTable generator.
  @param [in]  AcpiTableProtocol    Pointer to the AcpiTable protocol.
  @param [in]  AcpiTableInfo        Pointer to the ACPI table Info.

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
BuildAndInstallMultipleAcpiTable (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN CONST ACPI_TABLE_GENERATOR                  *CONST  Generator,
  IN       EFI_ACPI_TABLE_PROTOCOL                       *AcpiTableProtocol,
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   Status1;
  EFI_ACPI_DESCRIPTION_HEADER  **AcpiTable;
  UINTN                        TableCount;
  UINTN                        TableHandle;
  UINTN                        Index;

  AcpiTable  = NULL;
  TableCount = 0;
  Status     = Generator->BuildAcpiTableEx (
                            Generator,
                            AcpiTableInfo,
                            CfgMgrProtocol,
                            &AcpiTable,
                            &TableCount
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Build Table." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      AcpiTableInfo->TableGeneratorId,
      Status
      ));
    // Free any allocated resources.
    goto exit_handler;
  }

  if ((AcpiTable == NULL) || (TableCount == 0)) {
    Status = EFI_NOT_FOUND;
    goto exit_handler;
  }

  for (Index = 0; Index < TableCount; Index++) {
    // Dump ACPI Table Header
    DUMP_ACPI_TABLE_HEADER (AcpiTable[Index]);
    // Install ACPI table
    Status = AcpiTableProtocol->InstallAcpiTable (
                                  AcpiTableProtocol,
                                  AcpiTable[Index],
                                  AcpiTable[Index]->Length,
                                  &TableHandle
                                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Install ACPI Table. Status = %r\n",
        Status
        ));
      // Free any allocated resources.
      goto exit_handler;
    }

    DEBUG ((
      DEBUG_INFO,
      "INFO: ACPI Table installed. Status = %r\n",
      Status
      ));
  }

exit_handler:
  // Free any resources allocated for generating the tables.
  if (Generator->FreeTableResourcesEx != NULL) {
    Status1 = Generator->FreeTableResourcesEx (
                           Generator,
                           AcpiTableInfo,
                           CfgMgrProtocol,
                           &AcpiTable,
                           TableCount
                           );
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to Free Table Resources." \
        "TableGeneratorId = 0x%x. Status = %r\n",
        AcpiTableInfo->TableGeneratorId,
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
  for building an ACPI table. It uses the AcpiTableProtocol to install the
  table, then frees the resources allocated for generating it.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  AcpiTableProtocol    Pointer to the AcpiTable protocol.
  @param [in]  AcpiTableInfo        Pointer to the ACPI table Info.

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
BuildAndInstallAcpiTable (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN       EFI_ACPI_TABLE_PROTOCOL                       *AcpiTableProtocol,
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo
  )
{
  EFI_STATUS                  Status;
  CONST ACPI_TABLE_GENERATOR  *Generator;

  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableProtocol != NULL);
  ASSERT (AcpiTableInfo != NULL);

  DEBUG ((
    DEBUG_INFO,
    "INFO: EStdObjAcpiTableList: Address = 0x%p," \
    " TableGeneratorId = 0x%x\n",
    AcpiTableInfo,
    AcpiTableInfo->TableGeneratorId
    ));

  Generator = NULL;
  Status    = TableFactoryProtocol->GetAcpiTableGenerator (
                                      TableFactoryProtocol,
                                      AcpiTableInfo->TableGeneratorId,
                                      &Generator
                                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Table Generator not found." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      AcpiTableInfo->TableGeneratorId,
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

  if (Generator->BuildAcpiTableEx != NULL) {
    Status = BuildAndInstallMultipleAcpiTable (
               TableFactoryProtocol,
               CfgMgrProtocol,
               Generator,
               AcpiTableProtocol,
               AcpiTableInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to find build and install ACPI Table." \
        " Status = %r\n",
        Status
        ));
    }
  } else if (Generator->BuildAcpiTable != NULL) {
    Status = BuildAndInstallSingleAcpiTable (
               TableFactoryProtocol,
               CfgMgrProtocol,
               Generator,
               AcpiTableProtocol,
               AcpiTableInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to find build and install ACPI Table." \
        " Status = %r\n",
        Status
        ));
    }
  } else {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Table Generator does not implement the" \
      " ACPI_TABLE_GENERATOR_BUILD_TABLE interface." \
      " TableGeneratorId = 0x%x. Status = %r\n",
      AcpiTableInfo->TableGeneratorId,
      Status
      ));
  }

  return Status;
}

/** The function checks if the Configuration Manager has provided the
    mandatory ACPI tables for installation.

  @param [in]  AcpiTableInfo      Pointer to the ACPI Table Info list.
  @param [in]  AcpiTableCount     Count of ACPI Table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_NOT_FOUND         If mandatory table is not found.
**/
STATIC
EFI_STATUS
EFIAPI
VerifyMandatoryTablesArePresent (
  IN CONST CM_STD_OBJ_ACPI_TABLE_INFO  *CONST  AcpiTableInfo,
  IN       UINT32                              AcpiTableCount
  )
{
  EFI_STATUS  Status;
  BOOLEAN     FadtFound;
  BOOLEAN     MadtFound;
  BOOLEAN     GtdtFound;
  BOOLEAN     DsdtFound;
  BOOLEAN     Dbg2Found;
  BOOLEAN     SpcrFound;

  Status    = EFI_SUCCESS;
  FadtFound = FALSE;
  MadtFound = FALSE;
  GtdtFound = FALSE;
  DsdtFound = FALSE;
  Dbg2Found = FALSE;
  SpcrFound = FALSE;
  ASSERT (AcpiTableInfo != NULL);

  while (AcpiTableCount-- != 0) {
    switch (AcpiTableInfo[AcpiTableCount].AcpiTableSignature) {
      case EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
        FadtFound = TRUE;
        break;
      case EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
        MadtFound = TRUE;
        break;
      case EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE:
        GtdtFound = TRUE;
        break;
      case EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
        DsdtFound = TRUE;
        break;
      case EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE:
        Dbg2Found = TRUE;
        break;
      case EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE:
        SpcrFound = TRUE;
        break;
      default:
        break;
    }
  }

  // We need at least the FADT, MADT, GTDT and the DSDT tables to boot
  if (!FadtFound) {
    DEBUG ((DEBUG_ERROR, "ERROR: FADT Table not found\n"));
    Status = EFI_NOT_FOUND;
  }

  if (!MadtFound) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT Table not found.\n"));
    Status = EFI_NOT_FOUND;
  }

  if (!GtdtFound) {
    DEBUG ((DEBUG_ERROR, "ERROR: GTDT Table not found.\n"));
    Status = EFI_NOT_FOUND;
  }

  if (!DsdtFound) {
    DEBUG ((DEBUG_ERROR, "ERROR: DSDT Table not found.\n"));
    Status = EFI_NOT_FOUND;
  }

  if (!Dbg2Found) {
    DEBUG ((DEBUG_WARN, "WARNING: DBG2 Table not found.\n"));
  }

  if (!SpcrFound) {
    DEBUG ((DEBUG_WARN, "WARNING: SPCR Table not found.\n"));
  }

  return Status;
}

/** Generate and install ACPI tables.

  The function gathers the information necessary for installing the
  ACPI tables from the Configuration Manager, invokes the generators
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
ProcessAcpiTables (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  EFI_STATUS                  Status;
  EFI_ACPI_TABLE_PROTOCOL     *AcpiTableProtocol;
  CM_STD_OBJ_ACPI_TABLE_INFO  *AcpiTableInfo;
  UINT32                      AcpiTableCount;
  UINT32                      Idx;

  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (CfgMgrProtocol != NULL);

  // Find the AcpiTable protocol
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to find AcpiTable protocol. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = GetEStdObjAcpiTableList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AcpiTableInfo,
             &AcpiTableCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get ACPI Table List. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (0 == AcpiTableCount) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EStdObjAcpiTableList: AcpiTableCount = %d\n",
      AcpiTableCount
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: EStdObjAcpiTableList: AcpiTableCount = %d\n",
    AcpiTableCount
    ));

  // Check if mandatory ACPI tables are present.
  Status = VerifyMandatoryTablesArePresent (
             AcpiTableInfo,
             AcpiTableCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to find mandatory ACPI Table(s)."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Add the FADT Table first.
  for (Idx = 0; Idx < AcpiTableCount; Idx++) {
    if (CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt) ==
        AcpiTableInfo[Idx].TableGeneratorId)
    {
      Status = BuildAndInstallAcpiTable (
                 TableFactoryProtocol,
                 CfgMgrProtocol,
                 AcpiTableProtocol,
                 &AcpiTableInfo[Idx]
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: Failed to find build and install ACPI FADT Table." \
          " Status = %r\n",
          Status
          ));
        return Status;
      }

      break;
    }
  } // for

  // Add remaining ACPI Tables
  for (Idx = 0; Idx < AcpiTableCount; Idx++) {
    DEBUG ((
      DEBUG_INFO,
      "INFO: AcpiTableInfo[%d].TableGeneratorId = 0x%x\n",
      Idx,
      AcpiTableInfo[Idx].TableGeneratorId
      ));

    // Skip FADT Table since we have already added
    if (CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt) ==
        AcpiTableInfo[Idx].TableGeneratorId)
    {
      continue;
    }

    // Skip the Reserved table Generator ID for standard generators
    if ((IS_GENERATOR_NAMESPACE_STD (AcpiTableInfo[Idx].TableGeneratorId)) &&
        ((CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdReserved)           >=
          AcpiTableInfo[Idx].TableGeneratorId)                           ||
         (CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMax)                <=
          AcpiTableInfo[Idx].TableGeneratorId)))
    {
      DEBUG ((
        DEBUG_WARN,
        "WARNING: Invalid ACPI Generator table ID = 0x%x, Skipping...\n",
        AcpiTableInfo[Idx].TableGeneratorId
        ));
      continue;
    }

    Status = BuildAndInstallAcpiTable (
               TableFactoryProtocol,
               CfgMgrProtocol,
               AcpiTableProtocol,
               &AcpiTableInfo[Idx]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to find, build, and install ACPI Table." \
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  } // for

  return Status;
}

/** Entrypoint of Dynamic Table Manager Dxe.

  The Dynamic Table Manager uses the Configuration Manager Protocol
  to get the list of ACPI and SMBIOS tables to install. For each table
  in the list it requests the corresponding ACPI/SMBIOS table factory for
  a generator capable of building the ACPI/SMBIOS table.
  If a suitable table generator is found, it invokes the generator interface
  to build the table. The Dynamic Table Manager then installs the
  table and invokes another generator interface to free any resources
  allocated for building the table.

  @param  ImageHandle
  @param  SystemTable

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_NOT_FOUND         Required interface/object was not found.
  @retval EFI_INVALID_PARAMETER Some parameter is incorrect/invalid.
**/
EFI_STATUS
EFIAPI
DynamicTableManagerDxeInitialize (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
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
    return Status;
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
    return Status;
  }

  Status = GetCgfMgrInfo (CfgMgrProtocol, &CfgMfrInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Configuration Manager info. Status = %r\n",
      Status
      ));
    return Status;
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

  Status = ProcessAcpiTables (TableFactoryProtocol, CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ACPI Table processing failure. Status = %r\n",
      Status
      ));
  }

  return Status;
}
