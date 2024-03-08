/** @file
  WSMT Table Generator

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022
  - WSMT spec, version 1.0, April 18, 2016

**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include <Library/DebugLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <ArchNameSpaceObjects.h>

/** The Creator ID for the ACPI tables generated using
  the standard ACPI table generators.
*/
#define TABLE_GENERATOR_CREATOR_ID_GENERIC  SIGNATURE_32('D', 'Y', 'N', 'T')

/** This macro defines the WSMT Table Generator revision.
*/
#define WSMT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** This macro expands to a function that retrieves the
    protection flags information for WSMT Table.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjWsmtProtectionFlags,
  CM_ARCH_WSMT_PROTECTION_FLAGS
  );

/** The AcpiWsmt is a template EFI_ACPI_WSMT_TABLE
    structure used for generating the WSMT Table.
*/
STATIC
EFI_ACPI_WSMT_TABLE  mAcpiWsmt = {
  ACPI_HEADER (
    EFI_ACPI_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE,
    EFI_ACPI_WSMT_TABLE,
    EFI_WSMT_TABLE_REVISION
    ),
  EFI_WSMT_PROTECTION_FLAGS_SYSTEM_RESOURCE_PROTECTION|         \
  EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION|  \
  EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS
};

/** Construct the WSMT table.

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
BuildWsmtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                     Status;
  CM_ARCH_WSMT_PROTECTION_FLAGS  *ProtectionFlags;

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
      "ERROR: WSMT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)&mAcpiWsmt,
             AcpiTableInfo,
             sizeof (EFI_ACPI_WSMT_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: WSMT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = GetEArchObjWsmtProtectionFlags (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProtectionFlags,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: WSMT: Failed to get protection flags information." \
      " Status = %r\n",
      Status
      ));
  } else {
    mAcpiWsmt.ProtectionFlags = ProtectionFlags->ProtectionFlags;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)&mAcpiWsmt;

  return Status;
}

/** The interface for the WSMT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  mWsmtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdWsmt),
  // Generator Description
  L"ACPI.STD.WSMT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_WSMT_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_WSMT_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_GENERIC,
  // Creator Revision
  WSMT_GENERATOR_REVISION,
  // Build Table function
  BuildWsmtTable,
  // No additional resources are allocated by the generator.
  // Hence the Free Resource function is not required.
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
AcpiWsmtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&mWsmtGenerator);
  DEBUG ((DEBUG_INFO, "WSMT: Register Generator. Status = %r\n", Status));
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
AcpiWsmtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&mWsmtGenerator);
  DEBUG ((DEBUG_INFO, "WSMT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
