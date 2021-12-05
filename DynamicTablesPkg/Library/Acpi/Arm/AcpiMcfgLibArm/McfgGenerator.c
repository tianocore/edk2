/** @file
  MCFG Table Generator

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.2, January 26, 2015.

**/

#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard MCFG Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjPciConfigSpaceInfo
*/

#pragma pack(1)

/** This typedef is used to shorten the name of the MCFG Table
    header structure.
*/
typedef
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_BASE_ADDRESS_TABLE_HEADER
  MCFG_TABLE;

/** This typedef is used to shorten the name of the Enhanced
    Configuration Space address structure.
*/
typedef
  EFI_ACPI_MEMORY_MAPPED_ENHANCED_CONFIGURATION_SPACE_BASE_ADDRESS_ALLOCATION_STRUCTURE
  MCFG_CFG_SPACE_ADDR;

#pragma pack()

/** Retrieve the PCI Configuration Space Information.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPciConfigSpaceInfo,
  CM_ARM_PCI_CONFIG_SPACE_INFO
  );

/** Add the PCI Enhanced Configuration Space Information to the MCFG Table.

  @param [in]  Mcfg                Pointer to MCFG Table.
  @param [in]  PciCfgSpaceOffset   Offset for the PCI Configuration Space
                                   Info structure in the MCFG Table.
  @param [in]  PciCfgSpaceInfoList Pointer to the PCI Configuration Space
                                   Info List.
  @param [in]  PciCfgSpaceCount    Count of PCI Configuration Space Info.
**/
STATIC
VOID
AddPciConfigurationSpaceList (
  IN       MCFG_TABLE                   *CONST  Mcfg,
  IN CONST UINT32                               PciCfgSpaceOffset,
  IN CONST CM_ARM_PCI_CONFIG_SPACE_INFO         *PciCfgSpaceInfoList,
  IN       UINT32                               PciCfgSpaceCount
  )
{
  MCFG_CFG_SPACE_ADDR  *PciCfgSpace;

  ASSERT (Mcfg != NULL);
  ASSERT (PciCfgSpaceInfoList != NULL);

  PciCfgSpace = (MCFG_CFG_SPACE_ADDR *)((UINT8 *)Mcfg + PciCfgSpaceOffset);

  while (PciCfgSpaceCount-- != 0) {
    // Add PCI Configuration Space entry
    PciCfgSpace->BaseAddress           = PciCfgSpaceInfoList->BaseAddress;
    PciCfgSpace->PciSegmentGroupNumber =
      PciCfgSpaceInfoList->PciSegmentGroupNumber;
    PciCfgSpace->StartBusNumber = PciCfgSpaceInfoList->StartBusNumber;
    PciCfgSpace->EndBusNumber   = PciCfgSpaceInfoList->EndBusNumber;
    PciCfgSpace->Reserved       = EFI_ACPI_RESERVED_DWORD;
    PciCfgSpace++;
    PciCfgSpaceInfoList++;
  }
}

/** Construct the MCFG ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildMcfgTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                    Status;
  UINT32                        TableSize;
  UINT32                        ConfigurationSpaceCount;
  CM_ARM_PCI_CONFIG_SPACE_INFO  *PciConfigSpaceInfoList;
  MCFG_TABLE                    *Mcfg;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MCFG: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;
  Status = GetEArmObjPciConfigSpaceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PciConfigSpaceInfoList,
             &ConfigurationSpaceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MCFG: Failed to get PCI Configuration Space Information." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if (ConfigurationSpaceCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MCFG: Configuration Space Count = %d\n",
      ConfigurationSpaceCount
      ));
    Status = EFI_INVALID_PARAMETER;
    ASSERT (ConfigurationSpaceCount != 0);
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "MCFG: Configuration Space Count = %d\n",
    ConfigurationSpaceCount
    ));

  // Calculate the MCFG Table Size
  TableSize = sizeof (MCFG_TABLE) +
              ((sizeof (MCFG_CFG_SPACE_ADDR) * ConfigurationSpaceCount));

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MCFG: Failed to allocate memory for MCFG Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  Mcfg = (MCFG_TABLE *)*Table;
  DEBUG ((
    DEBUG_INFO,
    "MCFG: Mcfg = 0x%p TableSize = 0x%x\n",
    Mcfg,
    TableSize
    ));

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Mcfg->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MCFG: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Mcfg->Reserved = EFI_ACPI_RESERVED_QWORD;

  AddPciConfigurationSpaceList (
    Mcfg,
    sizeof (MCFG_TABLE),
    PciConfigSpaceInfoList,
    ConfigurationSpaceCount
    );

  return EFI_SUCCESS;

error_handler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free any resources allocated for constructing the MCFG

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
FreeMcfgTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MCFG: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the MCFG Table Generator revision.
*/
#define MCFG_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the MCFG Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  McfgGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMcfg),
  // Generator Description
  L"ACPI.STD.MCFG.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_2_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_MEMORY_MAPPED_CONFIGURATION_SPACE_ACCESS_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  MCFG_GENERATOR_REVISION,
  // Build Table function
  BuildMcfgTable,
  // Free Resource function
  FreeMcfgTableResources,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiMcfgLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&McfgGenerator);
  DEBUG ((DEBUG_INFO, "MCFG: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiMcfgLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&McfgGenerator);
  DEBUG ((DEBUG_INFO, "MCFG: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
