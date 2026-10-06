/** @file
  ERST Table Generator

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object

  @par Reference(s):
  - ACPI 6.6 Specification, May 2025
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/Acpi66.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#define ERST_RESERVED_SERIALIZATION_ACTION  0x0C

/** Standard ERST Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjErstInstructionsInfo
*/

/** Retrieve the ERST Serialization Instruction Entry information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjErstInstructionsInfo,
  CM_ARCH_COMMON_ERST_INSTRUCTIONS_INFO
  );

/** Return whether an ERST instruction accesses the Register Region.

  @param [in] Instruction  ERST serialization instruction.

  @retval TRUE   The instruction accesses the Register Region.
  @retval FALSE  The instruction does not access the Register Region.
**/
STATIC
BOOLEAN
EFIAPI
IsRegisterAccessInstruction (
  IN UINT8  Instruction
  )
{
  switch (Instruction) {
    case EFI_ACPI_6_6_ERST_READ_REGISTER:
    case EFI_ACPI_6_6_ERST_READ_REGISTER_VALUE:
    case EFI_ACPI_6_6_ERST_WRITE_REGISTER:
    case EFI_ACPI_6_6_ERST_WRITE_REGISTER_VALUE:
    case EFI_ACPI_6_6_ERST_LOAD_VAR1:
    case EFI_ACPI_6_6_ERST_LOAD_VAR2:
    case EFI_ACPI_6_6_ERST_STORE_VAR1:
    case EFI_ACPI_6_6_ERST_ADD_VALUE:
    case EFI_ACPI_6_6_ERST_SUBTRACT_VALUE:
    case EFI_ACPI_6_6_ERST_STALL_WHILE_TRUE:
    case EFI_ACPI_6_6_ERST_SKIP_NEXT_INSTRUCTION_IF_TRUE:
    case EFI_ACPI_6_6_ERST_SET_SRC_ADDRESS_BASE:
    case EFI_ACPI_6_6_ERST_SET_DST_ADDRESS_BASE:
    case EFI_ACPI_6_6_ERST_MOVE_DATA:
      return TRUE;

    default:
      return FALSE;
  }
}

/** Validate an ERST Mask against the Register Region bit width.

  @param [in] Instruction  Pointer to an ERST CM instruction entry.

  @retval EFI_SUCCESS            The Register Region and Mask are valid.
  @retval EFI_INVALID_PARAMETER  The Register Region width or Mask is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateRegisterMask (
  IN CONST CM_ARCH_COMMON_ERST_INSTRUCTIONS_INFO  *Instruction
  )
{
  CONST EFI_ACPI_6_6_GENERIC_ADDRESS_STRUCTURE  *RegisterRegion;

  ASSERT (Instruction != NULL);

  RegisterRegion = &Instruction->RegisterRegion;

  // The ERST Mask field is 64 bits and cannot describe a wider bit range.
  if (RegisterRegion->RegisterBitWidth > 64) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ERST: Register Region width = %u exceeds 64 bits\n",
      RegisterRegion->RegisterBitWidth
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((RegisterRegion->RegisterBitWidth < 64) &&
      ((Instruction->Mask >> RegisterRegion->RegisterBitWidth) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ERST: Mask = 0x%llx exceeds Register Region width = %u\n",
      Instruction->Mask,
      RegisterRegion->RegisterBitWidth
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Validate the ERST CM instruction entry list.

  @param [in] Instructions       Pointer to the CM instruction entry array.
  @param [in] InstructionCount   Number of entries in Instructions.

  @retval EFI_SUCCESS            The instruction list is valid.
  @retval EFI_INVALID_PARAMETER  An instruction entry is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateErstInstructions (
  IN CONST CM_ARCH_COMMON_ERST_INSTRUCTIONS_INFO  *Instructions,
  IN       UINT32                                 InstructionCount
  )
{
  BOOLEAN     ActionSeen[EFI_ACPI_6_6_ERST_GET_EXECUTE_OPERATION_TIMINGS + 1];
  EFI_STATUS  Status;
  UINT32      GroupEnd;
  UINT32      GroupStart;
  UINT32      Index;
  UINT8       PreviousAction;

  ASSERT (Instructions != NULL);
  ASSERT (InstructionCount != 0);

  ZeroMem (ActionSeen, sizeof (ActionSeen));
  PreviousAction = MAX_UINT8;

  for (Index = 0; Index < InstructionCount; Index++) {
    if ((Instructions[Index].SerializationAction >
         EFI_ACPI_6_6_ERST_GET_EXECUTE_OPERATION_TIMINGS) ||
        (Instructions[Index].SerializationAction ==
         ERST_RESERVED_SERIALIZATION_ACTION))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: ERST: Invalid Serialization Action encoding = 0x%x at index %u\n",
        Instructions[Index].SerializationAction,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (Instructions[Index].Instruction > EFI_ACPI_6_6_ERST_MOVE_DATA) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: ERST: Invalid Instruction encoding = 0x%x at index %u\n",
        Instructions[Index].Instruction,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    if ((Instructions[Index].Flags &
         ~EFI_ACPI_6_6_ERST_PRESERVE_REGISTER) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: ERST: Invalid Flags = 0x%x at index %u\n",
        Instructions[Index].Flags,
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (Instructions[Index].SerializationAction != PreviousAction) {
      if (ActionSeen[Instructions[Index].SerializationAction]) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: ERST: Instructions for Serialization Action = 0x%x "
          "are not consecutive.\n",
          Instructions[Index].SerializationAction
          ));
        return EFI_INVALID_PARAMETER;
      }

      ActionSeen[Instructions[Index].SerializationAction] = TRUE;
      PreviousAction                                      = Instructions[Index].SerializationAction;
    }

    if (IsRegisterAccessInstruction (Instructions[Index].Instruction)) {
      Status = ValidateRegisterMask (&Instructions[Index]);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  GroupStart = 0;
  while (GroupStart < InstructionCount) {
    GroupEnd = GroupStart + 1;
    while ((GroupEnd < InstructionCount) &&
           (Instructions[GroupEnd].SerializationAction ==
            Instructions[GroupStart].SerializationAction))
    {
      GroupEnd++;
    }

    for (Index = GroupStart; Index < GroupEnd; Index++) {
      if ((Instructions[Index].Instruction == EFI_ACPI_6_6_ERST_GOTO) &&
          (Instructions[Index].Value >= (GroupEnd - GroupStart)))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: ERST: GOTO target = 0x%llx at index %u is outside its "
          "Serialization Action.\n",
          Instructions[Index].Value,
          Index
          ));
        return EFI_INVALID_PARAMETER;
      }
    }

    GroupStart = GroupEnd;
  }

  return EFI_SUCCESS;
}

/** Populate the ERST Serialization Instruction Entry list.

  @param [out] ErstEntries       Pointer to the first ACPI instruction entry.
  @param [in]  Instructions      Pointer to the CM instruction entry array.
  @param [in]  InstructionCount  Number of entries in Instructions.
**/
STATIC
VOID
EFIAPI
PopulateErstInstructions (
  OUT EFI_ACPI_6_6_ERST_SERIALIZATION_INSTRUCTION_ENTRY  *ErstEntries,
  IN  CONST CM_ARCH_COMMON_ERST_INSTRUCTIONS_INFO        *Instructions,
  IN        UINT32                                       InstructionCount
  )
{
  UINT32  Index;

  ASSERT (ErstEntries != NULL);
  ASSERT (Instructions != NULL);

  for (Index = 0; Index < InstructionCount; Index++) {
    ErstEntries[Index].SerializationAction = Instructions[Index].SerializationAction;
    ErstEntries[Index].Instruction         = Instructions[Index].Instruction;
    ErstEntries[Index].Flags               = Instructions[Index].Flags;
    ErstEntries[Index].Value               = Instructions[Index].Value;
    ErstEntries[Index].Mask                = Instructions[Index].Mask;

    CopyMem (
      &ErstEntries[Index].RegisterRegion,
      &Instructions[Index].RegisterRegion,
      sizeof (ErstEntries[Index].RegisterRegion)
      );
  }
}

/** Construct the ERST ACPI table.

  This function invokes the Configuration Manager protocol interface to get
  the Serialization Instruction Entries required for generating the table.

  @param [in]  This            Pointer to the table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager Protocol.
  @param [out] Table           Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM instruction is invalid.
  @retval EFI_NOT_FOUND          The required object was not found.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildErstTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_ACPI_6_6_ERROR_RECORD_SERIALIZATION_TABLE_HEADER  *Erst;
  EFI_ACPI_6_6_ERST_SERIALIZATION_INSTRUCTION_ENTRY     *ErstEntries;
  CM_ARCH_COMMON_ERST_INSTRUCTIONS_INFO                 *Instructions;
  UINT32                                                InstructionCount;
  EFI_STATUS                                            Status;
  UINT32                                                TableSize;

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
      "ERROR: ERST: Requested table revision = %u is not supported. "
      "Supported table revision: Minimum = %u, Maximum = %u\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table           = NULL;
  Instructions     = NULL;
  InstructionCount = 0;

  Status = GetEArchCommonObjErstInstructionsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Instructions,
             &InstructionCount
             );
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_INFO, "ERST: No instruction entries found; skipping table.\n"));
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ERST: Failed to get instruction entries. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = ValidateErstInstructions (Instructions, InstructionCount);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TableSize =
    sizeof (EFI_ACPI_6_6_ERROR_RECORD_SERIALIZATION_TABLE_HEADER) +
    (InstructionCount *
     sizeof (EFI_ACPI_6_6_ERST_SERIALIZATION_INSTRUCTION_ENTRY));

  Erst = AllocateZeroPool (TableSize);
  if (Erst == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: ERST: Failed to allocate the ERST table.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Erst->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ERST: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    FreePool (Erst);
    return Status;
  }

  ErstEntries =
    (EFI_ACPI_6_6_ERST_SERIALIZATION_INSTRUCTION_ENTRY *)(Erst + 1);
  PopulateErstInstructions (
    ErstEntries,
    Instructions,
    InstructionCount
    );

  Erst->SerializationHeaderSize = sizeof (*Erst) - sizeof (Erst->Header);
  Erst->InstructionEntryCount   = InstructionCount;
  *Table                        = &Erst->Header;

  return EFI_SUCCESS;
}

/** Free resources allocated for constructing the ERST table.

  @param [in]      This            Pointer to the table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager
                                   Protocol interface.
  @param [in, out] Table           Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS            The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER  The table pointer is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
FreeErstTableResources (
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
    DEBUG ((DEBUG_ERROR, "ERROR: ERST: Invalid Table Pointer\n"));
    ASSERT (Table != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** This macro defines the ERST Table Generator revision. */
#define ERST_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the ERST Table Generator. */
STATIC
CONST
ACPI_TABLE_GENERATOR  ErstGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdErst),
  // Generator Description
  L"ACPI.STD.ERST.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_ERROR_RECORD_SERIALIZATION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_6_ERROR_RECORD_SERIALIZATION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_6_ERROR_RECORD_SERIALIZATION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  ERST_GENERATOR_REVISION,
  // Build Table function
  BuildErstTable,
  // Free Resource function
  FreeErstTableResources,
  // Extended build function not needed.
  NULL,
  // Extended free resource function not needed.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is registered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_ALREADY_STARTED    The Generator is already registered.
**/
EFI_STATUS
EFIAPI
AcpiErstLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&ErstGenerator);
  DEBUG ((DEBUG_INFO, "ERST: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiErstLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&ErstGenerator);
  DEBUG ((DEBUG_INFO, "ERST: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
