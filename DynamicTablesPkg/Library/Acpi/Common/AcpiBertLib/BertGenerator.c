/** @file
  BERT Table Generator

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object

  @par Reference(s):
  - ACPI 6.6 Specification, January 2025
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/Acpi66.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard BERT Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjBootErrorRegionInfo
*/

/** Retrieve the Boot Error Region information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjBootErrorRegionInfo,
  CM_ARCH_COMMON_BOOT_ERROR_REGION_INFO
  );

/** Construct the BERT ACPI table.

  This function invokes the Configuration Manager protocol interface to get
  the Boot Error Region information required for generating the ACPI table.

  @param [in]  This            Pointer to the table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager Protocol.
  @param [out] Table           Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration Manager
                                is smaller than the requested object.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildBertTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                   Status;
  CM_ARCH_COMMON_BOOT_ERROR_REGION_INFO        *BootErrorRegionInfo;
  EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_HEADER  *Bert;

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
      "ERROR: BERT: Requested table revision = %d is not supported. "
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjBootErrorRegionInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BootErrorRegionInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: BERT: Failed to get Boot Error Region information. "
      "Status = %r\n",
      Status
      ));
    return Status;
  }

  if (BootErrorRegionInfo->BootErrorRegionLength <
      sizeof (EFI_ACPI_6_6_GENERIC_ERROR_STATUS_STRUCTURE))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: BERT: Boot Error Region is too small. "
      "Length = 0x%x, Minimum = 0x%x\n",
      BootErrorRegionInfo->BootErrorRegionLength,
      (UINT32)sizeof (EFI_ACPI_6_6_GENERIC_ERROR_STATUS_STRUCTURE)
      ));
    return EFI_INVALID_PARAMETER;
  }

  Bert = AllocateZeroPool (sizeof (*Bert));
  if (Bert == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: BERT: Failed to allocate the BERT table.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Bert->Header,
             AcpiTableInfo,
             sizeof (*Bert)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: BERT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    FreePool (Bert);
    return Status;
  }

  Bert->BootErrorRegionLength = BootErrorRegionInfo->BootErrorRegionLength;
  Bert->BootErrorRegion       = BootErrorRegionInfo->BootErrorRegion;
  *Table                      = &Bert->Header;

  return EFI_SUCCESS;
}

/** Free resources allocated for constructing the BERT table.

  @param [in]      This            Pointer to the table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager
                                  Protocol interface.
  @param [in, out] Table           Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
FreeBertTableResources (
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

  if (Table == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: BERT: Invalid Table Pointer\n"));
    ASSERT (Table != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** This macro defines the BERT Table Generator revision. */
#define BERT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the BERT Table Generator. */
STATIC
CONST
ACPI_TABLE_GENERATOR  BertGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdBert),
  // Generator Description
  L"ACPI.STD.BERT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_6_BOOT_ERROR_RECORD_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  BERT_GENERATOR_REVISION,
  // Build Table function
  BuildBertTable,
  // Free Resource function
  FreeBertTableResources,
  // Extended build function not needed.
  NULL,
  // Extended free resource function not needed.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator was registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator is already registered.
**/
EFI_STATUS
EFIAPI
AcpiBertLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&BertGenerator);
  DEBUG ((DEBUG_INFO, "BERT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator was deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiBertLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&BertGenerator);
  DEBUG ((DEBUG_INFO, "BERT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
