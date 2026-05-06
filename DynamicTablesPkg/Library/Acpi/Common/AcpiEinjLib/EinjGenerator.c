/** @file
  EINJ Table Generator

  Copyright (c) 2026, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, August 2022

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/
#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/TableHelperLib.h>

/** ARM standard EINJ Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjEinjInstructionsInfo
*/

/** This macro expands to a function that retrieves the EINJ
    Instruction Entry Information from the Configuration Manager.
**/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjEinjInstructionsInfo,
  CM_ARCH_COMMON_EINJ_INSTRUCTIONS_INFO
  );

/** Populate the EINJ injection instruction entry list.

  @param [in] Einj              Pointer to the first injection instruction
                                entry (i.e. immediately after the EINJ header).
  @param [in] Size              Size, in bytes, of the buffer starting at Einj.
  @param [in] Instructions      Pointer to the CM instruction entry array.
  @param [in] InstructionsCount Number of entries in Instructions.

  @retval EFI_SUCCESS           Entries populated successfully.
  @retval EFI_BUFFER_TOO_SMALL  The buffer is too small for the entry list.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateEinjTable (
  IN  VOID                                   *Einj,
  IN  UINT32                                 Size,
  IN  CM_ARCH_COMMON_EINJ_INSTRUCTIONS_INFO  *Instructions,
  IN  UINT32                                 InstructionsCount
  )
{
  UINT32                                         Index;
  EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY  *EinjAcpi;

  ASSERT (Einj != NULL);

  if (InstructionsCount == 0) {
    return EFI_SUCCESS;
  }

  ASSERT (Instructions != NULL);

  EinjAcpi = (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY *)Einj;

  for (Index = 0; Index < InstructionsCount; Index++) {
    EinjAcpi->InjectionAction = Instructions->InjectionAction;
    EinjAcpi->Instruction     = Instructions->Instruction;
    EinjAcpi->Flags           = Instructions->Flags;
    EinjAcpi->Value           = Instructions->Value;
    EinjAcpi->Mask            = Instructions->Mask;

    CopyMem (
      &EinjAcpi->RegisterRegion,
      &Instructions->RegisterRegion,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );

    Instructions++;
    EinjAcpi++;
  }

  return EFI_SUCCESS;
}

/** Construct the EINJ ACPI table.

  Called by the Dynamic Table Manager, this function invokes the
  Configuration Manager protocol interface to get the required hardware
  information for generating the ACPI table.

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
  @retval EFI_BUFFER_TOO_SMALL   Buffer too small.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildEinjTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                 Status;
  CM_ARCH_COMMON_EINJ_INSTRUCTIONS_INFO      *EinjInstructions;
  UINT32                                     EinjInstructionsCount;
  UINT32                                     TableSize;
  UINT8                                      *Buffer;
  EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER  *Einj;

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
      "ERROR: EINJ: Requested table revision = %d, is not supported."
      "Supported table revisions: Min = %d, Max = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjEinjInstructionsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &EinjInstructions,
             &EinjInstructionsCount
             );
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "EINJ: No instruction entries found; skipping table.\n"));
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  TableSize  = sizeof (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER);
  TableSize += EinjInstructionsCount * sizeof (EFI_ACPI_6_5_EINJ_INJECTION_INSTRUCTION_ENTRY);

  // Allocate a Buffer for the EINJ table.
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Einj = (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER *)*Table;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Einj->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EINJ: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Buffer     = (UINT8 *)Einj;
  Buffer    += sizeof (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER);
  TableSize -= sizeof (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER);

  Status = PopulateEinjTable (
             Buffer,
             TableSize,
             EinjInstructions,
             EinjInstructionsCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  Einj->InjectionEntryCount = EinjInstructionsCount;
  Einj->InjectionHeaderSize = sizeof (EFI_ACPI_6_5_ERROR_INJECTION_TABLE_HEADER);

  return EFI_SUCCESS;

error_handler:
  DEBUG ((
    DEBUG_ERROR,
    "ERROR: EINJ: Failed to build table. Status = %r\n",
    Status
    ));

  if ((Table != NULL) && (*Table != NULL)) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free any resources allocated for constructing the EINJ.

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
EFIAPI
FreeEinjTableResources (
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
    DEBUG ((DEBUG_ERROR, "ERROR: EINJ: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the EINJ Table Generator revision.
*/
#define EINJ_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the EINJ Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  EinjGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdEinj),
  // Generator Description
  L"ACPI.STD.EINJ.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_ERROR_INJECTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_ERROR_INJECTION_TABLE_REVISION,
  // Minimum ACPI Table Revision supported by this Generator
  EFI_ACPI_6_4_ERROR_INJECTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  EINJ_GENERATOR_REVISION,
  // Build Table function
  BuildEinjTable,
  // Free Resource function
  FreeEinjTableResources,
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
AcpiEinjLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&EinjGenerator);
  DEBUG ((DEBUG_INFO, "EINJ: Register Generator. Status = %r\n", Status));
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
AcpiEinjLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&EinjGenerator);
  DEBUG ((DEBUG_INFO, "EINJ: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
