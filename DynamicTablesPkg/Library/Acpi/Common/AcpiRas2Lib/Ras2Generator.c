/** @file
  RAS2 Table Generator

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.6 Specification, Section 5.2.21
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/Acpi66.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CmObjHelperLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard RAS2 Generator

  The following Configuration Manager objects are required:
  - EArchCommonObjRas2PccInfo
  - EArchCommonObjProximityDomainInfo
  - EArchCommonObjPccSubspaceType0Info, EArchCommonObjPccSubspaceType1Info,
    or EArchCommonObjPccSubspaceType2Info, as selected by PccSubspaceType.

  Memory RAS is supported. The platform must publish the referenced PCC
  subspaces in PCCT and the memory proximity domains in SRAT. Initializing
  and servicing the RAS2 PCC shared memory is the platform's responsibility.
*/

/** Number of PCC IDs representable by the 8-bit RAS2 PCC Identifier.
    Cf. ACPI 6.6, Table 5.92.
**/
#define RAS2_MAX_PCC_COUNT  (MAX_UINT8 + 1)

/** Retrieve the RAS2 PCC information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjRas2PccInfo,
  CM_ARCH_COMMON_RAS2_PCC_INFO
  );

/** Retrieve PCC Subspace Type 0 information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPccSubspaceType0Info,
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE0_INFO
  );

/** Retrieve PCC Subspace Type 1 information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPccSubspaceType1Info,
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE1_INFO
  );

/** Retrieve PCC Subspace Type 2 information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPccSubspaceType2Info,
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE2_INFO
  );

/** Resolve a RAS2 PCC reference to its PCCT subspace ID.

  RAS2 uses the shared-memory header defined in ACPI 6.6, Table 5.95.
  PCC types 0, 1 and 2 use this header. Types 3/4 and 5 use different
  shared-memory headers (Sections 14.3 and 14.4).

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  Ras2PccInfo     RAS2 PCC information.
  @param [out] PccId           PCCT subspace ID.

  @retval EFI_SUCCESS            The PCC reference was resolved.
  @retval EFI_INVALID_PARAMETER  The referenced CM object is invalid.
  @retval EFI_UNSUPPORTED        The PCC subspace type is not supported.
  @retval other                  Failed to retrieve the referenced object.
**/
STATIC
EFI_STATUS
GetRas2PccId (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_ARCH_COMMON_RAS2_PCC_INFO          *CONST  Ras2PccInfo,
  OUT       UINT8                                *CONST   PccId
  )
{
  EFI_STATUS                              Status;
  PCC_SUBSPACE_GENERIC_INFO               *PccInfo;
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE1_INFO  *PccType1Info;
  CM_ARCH_COMMON_PCC_SUBSPACE_TYPE2_INFO  *PccType2Info;

  if (Ras2PccInfo->PccSubspaceToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Missing PCC subspace token.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  PccInfo      = NULL;
  PccType1Info = NULL;
  PccType2Info = NULL;
  switch (Ras2PccInfo->PccSubspaceType) {
    case EFI_ACPI_6_6_PCCT_SUBSPACE_TYPE_GENERIC:
      Status = GetEArchCommonObjPccSubspaceType0Info (
                 CfgMgrProtocol,
                 Ras2PccInfo->PccSubspaceToken,
                 &PccInfo,
                 NULL
                 );
      break;

    case EFI_ACPI_6_6_PCCT_SUBSPACE_TYPE_1_HW_REDUCED_COMMUNICATIONS:
      Status = GetEArchCommonObjPccSubspaceType1Info (
                 CfgMgrProtocol,
                 Ras2PccInfo->PccSubspaceToken,
                 &PccType1Info,
                 NULL
                 );
      PccInfo = (PCC_SUBSPACE_GENERIC_INFO *)PccType1Info;
      break;

    case EFI_ACPI_6_6_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS:
      Status = GetEArchCommonObjPccSubspaceType2Info (
                 CfgMgrProtocol,
                 Ras2PccInfo->PccSubspaceToken,
                 &PccType2Info,
                 NULL
                 );
      PccInfo = (PCC_SUBSPACE_GENERIC_INFO *)PccType2Info;
      break;

    default:
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: RAS2: Unsupported PCC subspace type %u.\n",
        Ras2PccInfo->PccSubspaceType
        ));
      return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Failed to get PCC subspace. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (PccInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: PCC subspace information is NULL.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  *PccId = PccInfo->SubspaceId;
  return EFI_SUCCESS;
}

/** Resolve the component instance associated with a RAS2 PCC descriptor.

  For Memory RAS, resolve the SRAT proximity domain using
  GetProximityDomainId().

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  Ras2PccInfo     RAS2 PCC information.
  @param [out] Instance        Component instance identifier.

  @retval EFI_SUCCESS            The instance was resolved.
  @retval EFI_INVALID_PARAMETER  The instance reference is invalid.
  @retval EFI_UNSUPPORTED        The RAS feature type is not supported.
  @retval other                  Failed to resolve the proximity domain.
**/
STATIC
EFI_STATUS
GetRas2Instance (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_ARCH_COMMON_RAS2_PCC_INFO          *CONST  Ras2PccInfo,
  OUT       UINT32                                *CONST  Instance
  )
{
  if (Ras2PccInfo->FeatureType != EFI_ACPI_6_6_RAS2_FEATURE_TYPE_MEMORY) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Unsupported RAS feature type %u.\n",
      Ras2PccInfo->FeatureType
      ));
    return EFI_UNSUPPORTED;
  }

  if (Ras2PccInfo->InstanceToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Missing instance token.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  return GetProximityDomainId (
           CfgMgrProtocol,
           0,
           Ras2PccInfo->InstanceToken,
           Instance
           );
}

/** Build the RAS2 ACPI table.

  @param [in]  This            Pointer to the table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [out] Table           Pointer to the constructed ACPI table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          Required information is missing.
  @retval EFI_BAD_BUFFER_SIZE    A CM object buffer is too small.
  @retval EFI_UNSUPPORTED        A feature or PCC type is not supported.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval other                  Failed to resolve a referenced CM object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildRas2Table (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                       Status;
  CM_ARCH_COMMON_RAS2_PCC_INFO     *Ras2PccInfo;
  EFI_ACPI_6_6_RAS2_FEATURE_TABLE  *Ras2;
  EFI_ACPI_RAS2_PCC_DESCRIPTOR     *PccDescriptor;
  UINT32                           PccCount;
  UINT32                           TableSize;
  UINT32                           Index;
  BOOLEAN                          PccIdUsed[RAS2_MAX_PCC_COUNT];

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
      "ERROR: RAS2: Requested table revision = %d is not supported. "
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjRas2PccInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Ras2PccInfo,
             &PccCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Failed to get RAS2 PCC information. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (PccCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: No PCC descriptors found.\n"
      ));
    return EFI_NOT_FOUND;
  }

  if ((Ras2PccInfo == NULL) || (PccCount > RAS2_MAX_PCC_COUNT)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Invalid PCC descriptor list. Count = %u\n",
      PccCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  // PccCount is bounded by the number of possible 8-bit PCC IDs, so the
  // table length fits in UINT32 and PccCount fits in the UINT16 table field.
  TableSize = sizeof (*Ras2) + (PccCount * sizeof (*PccDescriptor));
  Ras2      = AllocateZeroPool (TableSize);
  if (Ras2 == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Failed to allocate the RAS2 table.\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Ras2->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RAS2: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Ras2->PccCount = (UINT16)PccCount;
  PccDescriptor  = (EFI_ACPI_RAS2_PCC_DESCRIPTOR *)(Ras2 + 1);
  ZeroMem (PccIdUsed, sizeof (PccIdUsed));

  for (Index = 0; Index < PccCount; Index++) {
    Status = GetRas2PccId (
               CfgMgrProtocol,
               &Ras2PccInfo[Index],
               &PccDescriptor[Index].PccId
               );
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }

    // Each PCC channel is dedicated to a component instance.
    // Cf. ACPI 6.6, Section 5.2.21.1.1.
    if (PccIdUsed[PccDescriptor[Index].PccId]) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: RAS2: Duplicate PCC ID %u.\n",
        PccDescriptor[Index].PccId
        ));
      Status = EFI_INVALID_PARAMETER;
      goto error_handler;
    }

    PccIdUsed[PccDescriptor[Index].PccId] = TRUE;

    Status = GetRas2Instance (
               CfgMgrProtocol,
               &Ras2PccInfo[Index],
               &PccDescriptor[Index].Instance
               );
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }

    PccDescriptor[Index].RasFeatureType = Ras2PccInfo[Index].FeatureType;
  }

  *Table = &Ras2->Header;
  return EFI_SUCCESS;

error_handler:
  FreePool (Ras2);
  return Status;
}

/** Free resources allocated for constructing the RAS2 table.

  @param [in]      This            Pointer to the table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol  Configuration Manager protocol.
  @param [in, out] Table           Pointer to the constructed ACPI table.

  @retval EFI_SUCCESS            The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER  The table pointer is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
FreeRas2TableResources (
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

/** The RAS2 Table Generator revision. */
#define RAS2_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the RAS2 Table Generator. */
STATIC
CONST
ACPI_TABLE_GENERATOR  Ras2Generator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdRas2),
  // Generator Description
  L"ACPI.STD.RAS2.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_ACPI_RAS2_FEATURE_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_6_RAS2_FEATURE_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_6_RAS2_FEATURE_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  RAS2_GENERATOR_REVISION,
  // Build Table function
  BuildRas2Table,
  // Free Resource function
  FreeRas2TableResources,
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
AcpiRas2LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&Ras2Generator);
  DEBUG ((
    DEBUG_INFO,
    "RAS2: Register Generator. Status = %r\n",
    Status
    ));
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
AcpiRas2LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&Ras2Generator);
  DEBUG ((
    DEBUG_INFO,
    "RAS2: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
