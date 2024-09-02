/** @file
  WSMT Table Generator Implementation.

  This file implements the WSMT Table Generator.
  The WSMT table is used to specify the security mitigation
  that are enabled in the Windows OS.

  Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <X64NameSpaceObjects.h>

#define WSMT_PROTECTION_VALID_FLAGS \
  (EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS | \
   EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION | \
   EFI_WSMT_PROTECTION_FLAGS_SYSTEM_RESOURCE_PROTECTION)

/** This macro expands to a function that retrieves the
    WSMT protection flags information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjWsmtFlagsInfo,
  CM_X64_WSMT_FLAGS_INFO
  );

/** The ACPI WSMT Table.
*/
STATIC
EFI_ACPI_WSMT_TABLE  AcpiWsmt = {
  ACPI_HEADER (
    EFI_ACPI_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE,
    EFI_ACPI_WSMT_TABLE,
    EFI_WSMT_TABLE_REVISION
    ),
  // ProtectionFlags
  0
};

/** Update the protection flags information in the WSMT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
  @retval EFI_UNSUPPORTED       If invalid protection flags provided.
**/
STATIC
EFI_STATUS
EFIAPI
WsmtAddProtectionFlagsInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  EFI_STATUS              Status;
  CM_X64_WSMT_FLAGS_INFO  *WsmtFlagInfo;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the WSMT protection flag from the Platform Configuration Manager
  Status = GetEX64ObjWsmtFlagsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &WsmtFlagInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: WSMT: Failed to get WSMT protection flag information." \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "WSMT: Protection flags = 0x%x\n",
    WsmtFlagInfo->ProtectionFlags
    ));

  // Validate the protection flags
  if ((WsmtFlagInfo->ProtectionFlags & ~WSMT_PROTECTION_VALID_FLAGS) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: WSMT: Invalid protection flags = 0x%x\n",
      WsmtFlagInfo->ProtectionFlags
      ));
    return EFI_UNSUPPORTED;
  }

  if ((WsmtFlagInfo->ProtectionFlags & EFI_WSMT_PROTECTION_FLAGS_COMM_BUFFER_NESTED_PTR_PROTECTION) != 0) {
    if ((WsmtFlagInfo->ProtectionFlags & EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS) == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: WSMT: Invalid protection flags. EFI_WSMT_PROTECTION_FLAGS_FIXED_COMM_BUFFERS not set.\n"
        ));
      return EFI_UNSUPPORTED;
    }
  }

  AcpiWsmt.ProtectionFlags = WsmtFlagInfo->ProtectionFlags;
  return Status;
}

/** Construct the WSMT table.

  This function invokes the Configuration Manager protocol interface
  to get the required information for generating the ACPI table.

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
BuildWsmtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS  Status;

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
             (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiWsmt,
             AcpiTableInfo,
             sizeof (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: WSMT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update protection flags Info
  Status = WsmtAddProtectionFlagsInfo (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiWsmt;
error_handler:
  return Status;
}

/** This macro defines the WSMT Table Generator revision.
*/
#define WSMT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the WSMT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  WsmtGenerator = {
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
  TABLE_GENERATOR_CREATOR_ID_ARM,
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

  Status = RegisterAcpiTableGenerator (&WsmtGenerator);
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

  Status = DeregisterAcpiTableGenerator (&WsmtGenerator);
  DEBUG ((DEBUG_INFO, "WSMT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
