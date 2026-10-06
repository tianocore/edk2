/** @file
  Arm Error Source Table (AEST) generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for the Armv8-A RAS Extension and RAS System Architecture,
    DEN0085 2.0 EAC2.
    (https://support.arm.com/documentation/den0085/0202/?lang=en)
**/

#include <IndustryStandard/ArmErrorSourceTable.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <AcpiTableGenerator.h>
#include <StandardNameSpaceObjects.h>

/** Construct the AEST ACPI table.

    @param [in]  This           Pointer to the table generator.
    @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
    @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                                Protocol Interface.
    @param [out] Table          Pointer to the constructed ACPI Table.

    @retval EFI_SUCCESS           The table was generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is NULL or the table request
                                  does not describe AEST revision 2.
    @retval EFI_UNSUPPORTED       The requested AEST configuration is not
                                  supported.
**/
STATIC
EFI_STATUS
EFIAPI
BuildAestTable (
  IN  CONST ACPI_TABLE_GENERATOR                          *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                   **Table
  )
{
  if ((This == NULL) ||
      (AcpiTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;
  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature) ||
      (AcpiTableInfo->AcpiTableRevision != EFI_ACPI_AEST_2_0_TABLE_REVISION))
  {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_ERROR, "AEST: Table serialization is not implemented\n"));
  return EFI_UNSUPPORTED;
}

/** Free resources allocated for the AEST table.

    @param [in]      This           Pointer to the table generator.
    @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
    @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                    Protocol Interface.
    @param [in, out] Table          Pointer to the ACPI Table.

    @retval EFI_SUCCESS           The table was freed or no table was allocated.
    @retval EFI_INVALID_PARAMETER A parameter is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
FreeAestTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                          *This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  if ((This == NULL) ||
      (AcpiTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** The AEST Table Generator revision.
*/
#define AEST_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the AEST Table Generator.
*/
STATIC ACPI_TABLE_GENERATOR  mAestGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdAest),
  // Generator Description
  L"ACPI.STD.AEST.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_AEST_2_0_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_AEST_2_0_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_AEST_2_0_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  AEST_GENERATOR_REVISION,
  // Build Table function
  BuildAestTable,
  // Free Resource function
  FreeAestTableResources,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the AEST generator with the ACPI table factory.

    @param [in]  ImageHandle  The handle to the image.
    @param [in]  SystemTable  Pointer to the System Table.

    @retval EFI_SUCCESS           The generator is registered.
    @retval EFI_INVALID_PARAMETER The generator is invalid.
    @retval EFI_ALREADY_STARTED   The generator for the table ID is already
                                  registered.
**/
EFI_STATUS
EFIAPI
AcpiAestLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&mAestGenerator);
  DEBUG ((DEBUG_INFO, "AEST: Register generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the AEST generator from the ACPI table factory.

    @param [in]  ImageHandle  The handle to the image.
    @param [in]  SystemTable  Pointer to the System Table.

    @retval EFI_SUCCESS           The generator is deregistered.
    @retval EFI_INVALID_PARAMETER The generator is invalid.
    @retval EFI_NOT_FOUND         The generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiAestLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&mAestGenerator);
  DEBUG ((DEBUG_INFO, "AEST: Deregister generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
