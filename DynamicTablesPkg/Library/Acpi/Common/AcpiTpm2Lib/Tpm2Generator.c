/** @file
  TPM2 Table Generator

  Copyright (c) 2022, ARM Limited. All rights reserved.
  Copyright (c) 2023 - 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - TCG ACPI Specification Family for TPM 1.2 and 2.0 Version 1.3 Revision 8'
    (https://trustedcomputinggroup.org/wp-content/uploads/TCG_ACPIGeneralSpec_v1p3_r8_pub.pdf)

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include <IndustryStandard/Tpm2Acpi.h>

#define  START_METHOD_ACPI_PARAM_SIZE_MIN      4
#define  START_METHOD_CRB_WITH_SMC_PARAM_SIZE  12

/**
  ARM standard TPM2 Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - EArchCommonObjTpm2InterfaceInfo
*/

/**
  This macro expands to a function that retrieves the Processor Hierarchy
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjTpm2InterfaceInfo,
  CM_ARCH_COMMON_TPM2_INTERFACE_INFO
  );

/**
  Sanity check Start Method Specific Parameters field

  @param [in]  TpmInfo            Pointer to the CM TPM2 object

  @retval EFI_SUCCESS           No failure
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
AcpiTpm2CheckStartMethodParameters (
  CM_ARCH_COMMON_TPM2_INTERFACE_INFO  *TpmInfo
  )
{
  ASSERT (TpmInfo != NULL);

  if (sizeof (TpmInfo->StartMethodParameters) < TpmInfo->StartMethodParametersSize) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  // LAML and LASA are either both set or both zeros
  if (((TpmInfo->Laml > 0) && (TpmInfo->Lasa == 0)) ||
      ((TpmInfo->Laml == 0) && (TpmInfo->Lasa != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Verify StartMethodParametersSize based on StartMethod
  switch (TpmInfo->StartMethod) {
    case EFI_TPM2_ACPI_TABLE_START_METHOD_ACPI:
      // If the Start Method value is 2, then this field is at least four
      // bytes in size and the first four bytes must be all zero.
      if (TpmInfo->StartMethodParametersSize < START_METHOD_ACPI_PARAM_SIZE_MIN) {
        return EFI_INVALID_PARAMETER;
      }

      if (((UINT32 *)TpmInfo->StartMethodParameters)[0] != 0) {
        return EFI_INVALID_PARAMETER;
      }

      break;

    case EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_SMC:
      // If the Start Method value is 11 then this field is 12 bytes in size
      if (TpmInfo->StartMethodParametersSize != START_METHOD_CRB_WITH_SMC_PARAM_SIZE) {
        return EFI_INVALID_PARAMETER;
      }

      break;

    case EFI_TPM2_ACPI_TABLE_START_METHOD_TIS:
    case EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE:
    case EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_ACPI:
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
  }

  return EFI_SUCCESS;
}

/** Construct the TPM2 ACPI table.

  Called by the Dynamic Table Manager, this function invokes the
  Configuration Manager protocol interface to get the required hardware
  information for generating the ACPI table.

  If this function allocates any resources then they must be freed
  in the FreeTpm2TableResources function.

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
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildTpm2Table (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                          Status;
  UINT32                              TableSize;
  CM_ARCH_COMMON_TPM2_INTERFACE_INFO  *TpmInfo;
  EFI_TPM2_ACPI_TABLE                 *Tpm2;
  UINT32                              *Laml;
  UINT64                              *Lasa;

  *Table = NULL;

  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (Table != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Requested table revision = %d is not supported. "
      "Supported table revisions: Minimum = %d. Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjTpm2InterfaceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &TpmInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get TPM interface CM Object %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  // Sanity check Start Method Specific Parameters field
  Status = AcpiTpm2CheckStartMethodParameters (TpmInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Unexpected StartMethod %u with parameters size %u\n",
      TpmInfo->StartMethod,
      TpmInfo->StartMethodParametersSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Calculate the size of the TPM2 table
  TableSize = sizeof (EFI_TPM2_ACPI_TABLE);
  if (TpmInfo->Laml == 0) {
    TableSize += TpmInfo->StartMethodParametersSize;
  } else {
    // If LAML and LASA are present, then StartMethodParameters field would get
    // max size regardless of StartMethod value
    TableSize += EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4;
    TableSize += sizeof (TpmInfo->Laml) + sizeof (TpmInfo->Lasa);
  }

  // Allocate the Buffer for TPM2 table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Failed to allocate memory for TPM2 Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  Tpm2 = (EFI_TPM2_ACPI_TABLE *)*Table;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Tpm2->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Tpm2->Flags                = TpmInfo->PlatformClass;
  Tpm2->AddressOfControlArea = TpmInfo->AddressOfControlArea;
  Tpm2->StartMethod          = TpmInfo->StartMethod;

  CopyMem (
    Tpm2 + 1,
    TpmInfo->StartMethodParameters,
    TpmInfo->StartMethodParametersSize
    );

  if (TpmInfo->Laml > 0) {
    Laml = (UINT32 *)((UINT8 *)Tpm2 + sizeof (EFI_TPM2_ACPI_TABLE) +
                      EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4);
    Lasa  = (UINT64 *)((UINT8 *)Laml + sizeof (TpmInfo->Laml));
    *Laml = TpmInfo->Laml;
    *Lasa = TpmInfo->Lasa;
  }

  return EFI_SUCCESS;

error_handler:

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free any resources allocated for constructing the TPM2.

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
FreeTpm2TableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: TPM2: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** The TPM2 Table Generator revision.
*/
#define TPM2_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the TPM2 Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  Tpm2Generator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdTpm2),
  // Generator Description
  L"ACPI.STD.TPM2.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_4_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_TPM2_ACPI_TABLE_REVISION_4,
  // Minimum supported ACPI Table Revision
  EFI_TPM2_ACPI_TABLE_REVISION_4,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  TPM2_GENERATOR_REVISION,
  // Build Table function
  BuildTpm2Table,
  // Free Resource function
  FreeTpm2TableResources,
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
AcpiTpm2LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&Tpm2Generator);
  DEBUG ((DEBUG_INFO, "TPM2: Register Generator. Status = %r\n", Status));
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
AcpiTpm2LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&Tpm2Generator);
  DEBUG ((DEBUG_INFO, "TPM2: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
