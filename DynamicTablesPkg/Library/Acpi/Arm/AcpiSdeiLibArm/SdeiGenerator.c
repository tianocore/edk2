/** @file
  SDEI Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Software Delegated Exception Interface (SDEI), DEN0054C
    (https://support.arm.com/documentation/den0054/c)
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/Acpi66.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Construct the header-only SDEI table.

  The common header helper obtains OEM metadata from the Configuration
  Manager. The caller must release the table with FreeSdeiTableResources.

  @param [in]  This            Pointer to the table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager Protocol.
  @param [out] Table           Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_BAD_BUFFER_SIZE   Configuration Manager information is too small.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Configuration Manager information is missing.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSdeiTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                                         Status;
  EFI_ACPI_6_6_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_HEADER  *Sdei;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;
  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SDEI: Requested table revision = %d is not supported. "
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  Sdei = AllocateZeroPool (sizeof (*Sdei));
  if (Sdei == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: SDEI: Failed to allocate table\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Sdei->Header,
             AcpiTableInfo,
             sizeof (*Sdei)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SDEI: Failed to add ACPI header. Status = %r\n", Status));
    FreePool (Sdei);
    return Status;
  }

  *Table = &Sdei->Header;
  return EFI_SUCCESS;
}

/** Free the resources allocated for constructing the SDEI table.

  The framework also calls this function after an unsuccessful build.

  @param [in]      This            Pointer to the table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager Protocol.
  @param [in, out] Table           Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSdeiTableResources (
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
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** Generator Revision */
#define SDEI_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SDEI Table Generator. */
STATIC
CONST
ACPI_TABLE_GENERATOR  SdeiGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSdei),
  // Generator Description
  L"ACPI.STD.SDEI.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_6_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_REVISION,
  // Minimum ACPI Table Revision supported by this Generator
  EFI_ACPI_6_2_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SDEI_GENERATOR_REVISION,
  // Build Table function
  BuildSdeiTable,
  // Free Resource function
  FreeSdeiTableResources,
  // Extended build function not needed
  NULL,
  // Extended free resource function not needed
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_ALREADY_STARTED   The Generator is already registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
AcpiSdeiLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SdeiGenerator);
  DEBUG ((DEBUG_INFO, "SDEI: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSdeiLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SdeiGenerator);
  DEBUG ((DEBUG_INFO, "SDEI: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
