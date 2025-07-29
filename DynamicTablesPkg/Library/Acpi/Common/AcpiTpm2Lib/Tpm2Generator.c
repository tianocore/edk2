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
#include <Library/PcdLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Library/Tpm2DeviceTableLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include <IndustryStandard/Tpm2Acpi.h>

#define  START_METHOD_ACPI_PARAM_SIZE_MIN      4
#define  START_METHOD_CRB_WITH_SMC_PARAM_SIZE  12
#define  START_METHOD_CRB_WITH_FFA_PARM_SIZE   12

#define  TPM2_DEVICE_UID  0
#define  MAX_TABLE_COUNT  2

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

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjTpm2DeviceInfo,
  CM_ARCH_COMMON_TPM2_DEVICE_INFO
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
    case EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_FFA:
      if (TpmInfo->StartMethodParametersSize != START_METHOD_CRB_WITH_FFA_PARM_SIZE) {
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

/** Build a TPM2 ACPI table.

  @param [in]       This             Pointer to the table generator.
  @param [in]       AcpiTableInfo    Pointer to the ACPI Table Info.
  @param [in]       CfgMgrProtocol   Pointer to the Configuration Manager
                                   Protocol Interface.
  @param [in]       TpmInfo          TpmInfo to describe TPM2 device.
  @param [in, out]  Tpm2AcpiTable    Tpm2AcpiTable.
  @param [in]       TableSize        Size of Tpm2AcpiTable.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildTpm2AcpiTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_ARCH_COMMON_TPM2_INTERFACE_INFO        *TpmInfo,
  IN OUT  EFI_TPM2_ACPI_TABLE                             *Tpm2AcpiTable,
  IN      UINT32                                          TableSize
  )
{
  EFI_STATUS  Status;
  UINT32      *Laml;
  UINT64      *Lasa;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Tpm2AcpiTable->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    return Status;
  }

  Tpm2AcpiTable->Flags                = TpmInfo->PlatformClass;
  Tpm2AcpiTable->AddressOfControlArea = TpmInfo->AddressOfControlArea;
  Tpm2AcpiTable->StartMethod          = TpmInfo->StartMethod;

  CopyMem (
    Tpm2AcpiTable + 1,
    TpmInfo->StartMethodParameters,
    TpmInfo->StartMethodParametersSize
    );

  if (TpmInfo->Laml > 0) {
    Lasa  = (UINT64 *)((UINT8 *)Tpm2AcpiTable + TableSize - sizeof (TpmInfo->Lasa));
    Laml  = (UINT32 *)((UINT8 *)Lasa - sizeof (TpmInfo->Laml));
    *Laml = TpmInfo->Laml;
    *Lasa = TpmInfo->Lasa;
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
  @param [out] Table          Pointer to a list of generated ACPI table(s).
  @param [out] TableCount     Number of generated ACPI table(s).

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
BuildTpm2TableEx (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                   ***Table,
  OUT       UINTN                                 *CONST  TableCount
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_TPM2_INTERFACE_INFO  *TpmInfo;
  CM_ARCH_COMMON_TPM2_DEVICE_INFO     *TpmDevInfo;
  UINT32                              TableSize;
  UINT32                              MaxParameterSize;
  EFI_ACPI_DESCRIPTION_HEADER         **TableList;
  CHAR8                               NewName[AML_NAME_SEG_SIZE + 1];

  *Table      = NULL;
  *TableCount = 0;

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

  if (AcpiTableInfo->AcpiTableRevision == EFI_TPM2_ACPI_TABLE_REVISION_5) {
    MaxParameterSize = EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_5;
  } else {
    MaxParameterSize = EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4;
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
    TableSize += MaxParameterSize;
    TableSize += sizeof (TpmInfo->Laml) + sizeof (TpmInfo->Lasa);
  }

  // Allocate a table to store pointers to the TPM2 table and
  // Ssdt table.for Tpm2 device description.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * MAX_TABLE_COUNT)
                );
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Failed to allocate memory for TableList.\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  // Allocate the Buffer for TPM2 table
  TableList[0] = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (TableList[0] == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Failed to allocate memory for TPM2 Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto ErrorHandler;
  }

  Status = BuildTpm2AcpiTable (
             This,
             AcpiTableInfo,
             CfgMgrProtocol,
             TpmInfo,
             (EFI_TPM2_ACPI_TABLE *)TableList[0],
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: TPM2: Failed to Build TPM2 ACPI Table, " \
      " Status = %r\n",
      Status
      ));
    goto ErrorHandler;
  }

  *TableCount += 1;

  // Generate TPM2 device SSDT table.
  if (FixedPcdGetBool (PcdGenTpm2DeviceTable)) {
    Status = GetEArchCommonObjTpm2DeviceInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &TpmDevInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to get TPM2 Device CM Object %r\n",
        __func__,
        Status
        ));
      goto ErrorHandler;
    }

    NewName[0] = 'T';
    NewName[1] = 'P';
    NewName[2] = 'M';
    NewName[3] = AsciiFromHex ((UINT8)(TPM2_DEVICE_UID));
    NewName[4] = '\0';

    Status = BuildTpm2DeviceTable (
               TpmDevInfo,
               NewName,
               TPM2_DEVICE_UID,
               &TableList[1]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: TPM2: Failed to Build SSDT table for TPM2 device," \
        " Status = %r\n",
        Status
        ));
      goto ErrorHandler;
    }

    *TableCount += 1;
  }

  *Table = TableList;

  return EFI_SUCCESS;

ErrorHandler:
  *TableCount = 0;

  if (TableList != NULL) {
    if (TableList[0] != NULL) {
      FreePool (TableList[0]);
    }

    FreePool (TableList);
  }

  return Status;
}

/** Free any resources allocated for constructing the TPM2.

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to an array of pointers
                                  to ACPI Table(s).
  @param [in]      TableCount     Number of ACPI table(s).


  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeTpm2TableResourcesEx (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST   This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST   AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  )
{
  UINTN                        Idx;
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;

  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  if ((Table == NULL) || (*Table == NULL) || (TableCount == 0)) {
    DEBUG ((DEBUG_ERROR, "ERROR: TPM2: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;

  for (Idx = 0; Idx < TableCount; Idx++) {
    switch (TableList[Idx]->Signature) {
      case EFI_ACPI_6_5_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE:
        FreePool (TableList[Idx]);
        break;
      case EFI_ACPI_6_5_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
        FreeTpm2DeviceTable (TableList[Idx]);
        break;
      default:
        ASSERT (0);
    }
  }

  FreePool (TableList);
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
  EFI_TPM2_ACPI_TABLE_REVISION_5,
  // Minimum supported ACPI Table Revision
  EFI_TPM2_ACPI_TABLE_REVISION_4,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  TPM2_GENERATOR_REVISION,
  // Build Table function
  NULL,
  // Free Resource function
  NULL,
  // Extended build function not needed
  BuildTpm2TableEx,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  FreeTpm2TableResourcesEx
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
