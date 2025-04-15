/** @file
  SLIT Table Generator

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard SLIT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSlitInfo
*/

/** Retrieve the SLIT interface information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSlitInfo,
  CM_ARCH_COMMON_SLIT_INFO
  );

/** Construct the SLIT ACPI table.

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
BuildSlitTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  CM_ARCH_COMMON_SLIT_INFO  *SlitInfo;
  EFI_STATUS                Status;
  UINTN                     Col;
  UINTN                     Row;

  EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER  *AcpiSlitTable;

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
      "ERROR: SLIT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjSlitInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SlitInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to retrieve interface type and base address.\n"
      ));
    return Status;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to retrieve SLIT information. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SlitInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: SLIT: No SLIT information provided by configuration manager.\n"));
    return EFI_NOT_FOUND;
  }

  if (SlitInfo->NumberOfSystemLocalities > EARCH_COMMON_SLIT_LOCALITY_MAX) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Number of system localities (%d) exceeds maximum (%d).\n",
      SlitInfo->NumberOfSystemLocalities,
      EARCH_COMMON_SLIT_LOCALITY_MAX
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (SlitInfo->NumberOfSystemLocalities == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: SLIT: Number of system localities is zero.\n"));
    return EFI_INVALID_PARAMETER;
  }

  for (Row = 0; Row < SlitInfo->NumberOfSystemLocalities; Row++) {
    for (Col = 0; Col < SlitInfo->NumberOfSystemLocalities; Col++) {
      if (SlitInfo->Entity[Row + Col] < 0x10) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SLIT: Distance between localities (%d, %d) is less than 0x10.\n",
          Row,
          Col
          ));
        return EFI_INVALID_PARAMETER;
      }

      if ((Row == Col) && (SlitInfo->Entity[Row + Col] != 0x10)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SLIT: Distance between localities (%d, %d) is not 0x10.\n",
          Row,
          Col
          ));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  AcpiSlitTable = AllocateZeroPool (
                    sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER) +
                    (sizeof (UINT8) * SlitInfo->NumberOfSystemLocalities * SlitInfo->NumberOfSystemLocalities)
                    );
  if (AcpiSlitTable == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: SLIT: Failed to allocate memory for SLIT table.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  AcpiSlitTable->NumberOfSystemLocalities = SlitInfo->NumberOfSystemLocalities;
  CopyMem (
    (UINT8 *)AcpiSlitTable + sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER),
    SlitInfo->Entity,
    SlitInfo->NumberOfSystemLocalities * SlitInfo->NumberOfSystemLocalities
    );
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiSlitTable,
             AcpiTableInfo,
             (sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER) +
              sizeof (UINT8) * SlitInfo->NumberOfSystemLocalities * SlitInfo->NumberOfSystemLocalities)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AcpiSlitTable;
  return Status;
}

/** This macro defines the SLIT Table Generator revision.
*/
#define SLIT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SLIT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SpmiGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSlit),
  // Generator Description
  L"ACPI.STD.SLIT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  SLIT_GENERATOR_REVISION,
  // Build Table function
  BuildSlitTable,
  // Free Resource function
  NULL,
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
AcpiSlitLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SpmiGenerator);
  DEBUG ((DEBUG_INFO, "SLIT: Register Generator. Status = %r\n", Status));
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
AcpiSlitLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SpmiGenerator);
  DEBUG ((DEBUG_INFO, "SLIT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
