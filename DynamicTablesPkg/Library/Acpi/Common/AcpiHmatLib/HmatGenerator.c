/** @file
  HMAT Table Generator

  Copyright (c) 2026, Google LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>
#include <Library/BaseMemoryLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/CmObjHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard HMAT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjMemoryLatBwInfo
  - EArchCommonObjMemoryProximityDomainAttrInfo
  - EArchCommonObjProximityDomainInfo
  - EArchCommonObjProximityDomainRelationInfo

*/

/** Retrieve the Proximity Domain Info */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProximityDomainInfo,
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO
  );

/** Retrieve the Proximity Domain Relation Info. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProximityDomainRelationInfo,
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO
  );

/** Retrieve the Proximity Domain Attr Info */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryProximityDomainAttrInfo,
  CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO
  );

/** Retrieve the SSLBI Info */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryLatBwInfo,
  CM_ARCH_COMMON_MEMORY_LAT_BW_INFO
  );

/** Find an index of the specified domain ID in an array.

  @param [in]  DomainId         Domain ID to find.
  @param [in]  DomainIds        Pointer to an array with domain IDs.
  @param [in]  DomainIdCount    Number of elements in DomainIds array.
  @param [out] DomainIdIndex    Pointer where the index of the found ID is stored.
                                Can be NULL if the actual index doesn't matter.

  @retval EFI_SUCCESS   If DomainId was found
  @retval EFI_NOT_FOUND If DomainId wasn't found
**/
STATIC
EFI_STATUS
FindDomainIndex (
  IN  UINT32  DomainId,
  IN  UINT32  *DomainIds,
  IN  UINTN   DomainIdCount,
  OUT UINTN   *DomainIdIndex OPTIONAL
  )
{
  UINTN  Index;

  for (Index = 0; Index < DomainIdCount; Index++) {
    if (DomainIds[Index] == DomainId) {
      if (DomainIdIndex != NULL) {
        *DomainIdIndex = Index;
      }

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Computes how many bytes are required to hold an ACPI HMAT SSLBI table.

  @param [in] InitiatorProximityDomainCount Number of initiator domains in an SSLBI record.
  @param [in] TargetProximityDomainCount    Number of target domains in an SSLBI record.

  @retval Bytes required to store SSLBI record.
**/
STATIC
UINT32
GetSslbiTableSize (
  IN UINT32  InitiatorProximityDomainCount,
  IN UINT32  TargetProximityDomainCount
  )
{
  return sizeof (UINT32) * InitiatorProximityDomainCount +
         sizeof (UINT32) * TargetProximityDomainCount +
         sizeof (UINT16) * InitiatorProximityDomainCount * TargetProximityDomainCount +
         sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO);
}

/** Populate SSLBI record and return the actual size of the record to caller.

  @param [in]       CfgMgrProtocol          ConfigurationManager protocol.
  @param [in]       CmMemLatBwInfo          SSLBI record information.
  @param [in]       CmMemLatBwRelations     Array of relations to be populated into SSLBI.
  @param [in]       CmMemLatBwRelationCount Number of elements in the relations array.
  @param [in]       ProximityDomainCount    Maximum number of proximity domains, used to
                                            sanity check the relations array data.
  @param [in, out]  MemLatBwInfo            SSLBI to be populated. Caller is responsible for
                                            allocating memory area large enough to fit the data.
  @param [out]      RecordLength            Actual size of the SSLBI record in bytes,
                                            header + relations matrix.

  @retval EFI_INVALID_PARAMETER The relations matrix data is not valid: it's either spare
                                or number of initiators/targets larger than number of domains
                                in the system.
**/
STATIC
EFI_STATUS
AddMemLatBwInfo (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST                           CfgMgrProtocol,
  IN      CONST CM_ARCH_COMMON_MEMORY_LAT_BW_INFO                                       *CmMemLatBwInfo,
  IN      CONST CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO                           *CmMemLatBwRelations,
  IN            UINTN                                                                   CmMemLatBwRelationCount,
  IN            UINT32                                                                  ProximityDomainCount,
  IN  OUT       EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO  *MemLatBwInfo,
  OUT           UINTN                                                                   *RecordLength
  )
{
  UINTN       Index;
  UINT32      *InitiatorDomains;
  UINT32      InitiatorDomainCount;
  UINT32      *TargetDomains;
  UINT32      TargetDomainCount;
  EFI_STATUS  Status;
  UINT32      DomainId;
  UINTN       InitiatorIdx;
  UINTN       TargetIdx;
  UINT16      *Relations;
  UINT8       *TablePtr;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (MemLatBwInfo != NULL);
  ASSERT (CmMemLatBwInfo != NULL);
  ASSERT (CmMemLatBwRelations != NULL);
  ASSERT (CmMemLatBwRelationCount > 0);
  ASSERT (ProximityDomainCount > 0);
  ASSERT (RecordLength != NULL);

  if (CmMemLatBwRelationCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  MemLatBwInfo->Type = EFI_ACPI_6_4_HMAT_TYPE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO;

  CopyMem (&MemLatBwInfo->Flags, &CmMemLatBwInfo->Flags, sizeof (UINT8));
  if ((MemLatBwInfo->Flags.MemoryHierarchy > EFI_ACPI_6_4_HMAT_MEMORY_HIERARCHY_L3_CACHE) ||
      (MemLatBwInfo->Flags.Reserved != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  MemLatBwInfo->DataType        = CmMemLatBwInfo->DataType;
  MemLatBwInfo->MinTransferSize = CmMemLatBwInfo->MinTransferSize;
  MemLatBwInfo->EntryBaseUnit   = CmMemLatBwInfo->EntryBaseUnit;

  TablePtr = (UINT8 *)MemLatBwInfo;

  TablePtr            += sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO);
  InitiatorDomainCount = 0;
  InitiatorDomains     = (UINT32 *)TablePtr;

  // Append Initiator ProximityDomains.
  for (Index = 0; Index < CmMemLatBwRelationCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               CmMemLatBwRelations[Index].FirstDomainToken,
               &DomainId
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    if (FindDomainIndex (DomainId, InitiatorDomains, InitiatorDomainCount, NULL) == EFI_NOT_FOUND) {
      if (InitiatorDomainCount >= ProximityDomainCount) {
        return EFI_INVALID_PARAMETER;
      }

      InitiatorDomains[InitiatorDomainCount] = DomainId;
      InitiatorDomainCount++;
    }
  }

  // Append Target ProximityDomains.
  TargetDomainCount = 0;
  TargetDomains     = InitiatorDomains + InitiatorDomainCount;
  for (Index = 0; Index < CmMemLatBwRelationCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               CmMemLatBwRelations[Index].SecondDomainToken,
               &DomainId
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    if (FindDomainIndex (DomainId, TargetDomains, TargetDomainCount, NULL) == EFI_NOT_FOUND) {
      if (TargetDomainCount >= ProximityDomainCount) {
        return EFI_INVALID_PARAMETER;
      }

      TargetDomains[TargetDomainCount] = DomainId;
      TargetDomainCount++;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "HMAT: Lat/Bw info table dimensions: %d x %d\n",
    InitiatorDomainCount,
    TargetDomainCount
    ));

  if (InitiatorDomainCount * TargetDomainCount != CmMemLatBwRelationCount) {
    DEBUG ((
      DEBUG_ERROR,
      "HMAT: Lat/Bw info table dimensions %dx%d do not match elements count %d (should be %d)\n",
      InitiatorDomainCount,
      TargetDomainCount,
      CmMemLatBwRelationCount,
      InitiatorDomainCount * TargetDomainCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  Relations = (UINT16 *)(TargetDomains + TargetDomainCount);
  for (Index = 0; Index < CmMemLatBwRelationCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               CmMemLatBwRelations[Index].FirstDomainToken,
               &DomainId
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = FindDomainIndex (DomainId, InitiatorDomains, InitiatorDomainCount, &InitiatorIdx);
    ASSERT_EFI_ERROR (Status);

    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               CmMemLatBwRelations[Index].SecondDomainToken,
               &DomainId
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = FindDomainIndex (DomainId, TargetDomains, TargetDomainCount, &TargetIdx);
    ASSERT_EFI_ERROR (Status);

    if (CmMemLatBwRelations[Index].Relation > MAX_UINT16) {
      DEBUG ((
        DEBUG_ERROR,
        "HMAT: relation value of element %d does not fit in 16 bits: %d\n",
        Index,
        CmMemLatBwRelations[Index].Relation
        ));
      return EFI_INVALID_PARAMETER;
    }

    Relations[InitiatorIdx * TargetDomainCount + TargetIdx] = CmMemLatBwRelations[Index].Relation & 0xFFFF;
  }

  // Update record info
  MemLatBwInfo->Length                            = GetSslbiTableSize (InitiatorDomainCount, TargetDomainCount);
  MemLatBwInfo->NumberOfInitiatorProximityDomains = InitiatorDomainCount;
  MemLatBwInfo->NumberOfTargetProximityDomains    = TargetDomainCount;
  *RecordLength                                   = MemLatBwInfo->Length;

  return EFI_SUCCESS;
}

/** Add the MemProxDomainAttr Information record to the HMAT Table.

  @param [in]  CfgMgrProtocol                   Pointer to the Configuration Manager Protocol Interface.
  @param [in]  MemProximityDomainAttrInfo       Pointer to a Proximity Domain Attribute array in an HMAT table.
                                                The caller is responsible for allocating enough memory to fit
                                                the data.
  @param [in]  CmMemProximityDomainAttrInfo     Pointer ConfigurationManager objects array with Domain Attribute data.
  @param [in]  MemProximityDomainAttrCount      Number of entries in CmMemProximityDomainAttrInfo.

  @retval EFI_SUCCESS if attributes were added
  @retval error code otherwise
**/
STATIC
EFI_STATUS
AddMemProximityDomainAttrInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST               CfgMgrProtocol,
  IN  EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES  *MemProximityDomainAttrInfo,
  IN  CONST CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO          *CmMemProximityDomainAttrInfo,
  IN  UINT32                                                          MemProximityDomainAttrCount
  )
{
  EFI_STATUS  Status;

  if (MemProximityDomainAttrCount == 0) {
    return EFI_SUCCESS;
  }

  ASSERT (MemProximityDomainAttrInfo != NULL);
  ASSERT (CmMemProximityDomainAttrInfo != NULL);

  while (MemProximityDomainAttrCount-- != 0) {
    MemProximityDomainAttrInfo->Type   = EFI_ACPI_6_4_HMAT_TYPE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES;
    MemProximityDomainAttrInfo->Length = sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES);
    CopyMem (&MemProximityDomainAttrInfo->Flags, &CmMemProximityDomainAttrInfo->Flags, sizeof (UINT16));
    if (MemProximityDomainAttrInfo->Flags.Reserved != 0) {
      return EFI_INVALID_PARAMETER;
    }

    if (CmMemProximityDomainAttrInfo->InitiatorProximityDomain
        == CmMemProximityDomainAttrInfo->MemoryProximityDomain)
    {
      return EFI_INVALID_PARAMETER;
    }

    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               CmMemProximityDomainAttrInfo->InitiatorProximityDomain,
               &MemProximityDomainAttrInfo->InitiatorProximityDomain
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               CmMemProximityDomainAttrInfo->MemoryProximityDomain,
               &MemProximityDomainAttrInfo->MemoryProximityDomain
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    MemProximityDomainAttrInfo++;
    CmMemProximityDomainAttrInfo++;
  }

  return EFI_SUCCESS;
}

/** Free any resources allocated for constructing the table.

  @param [in]      This           Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeHmatTable (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: HMAT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** Construct the HMAT ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResourcesEx function.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to the generated ACPI table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                 Manager is less than the Object size for
                                 the requested object.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildHmatTable (
  IN  CONST ACPI_TABLE_GENERATOR                            *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST   AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST   CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER            **CONST  Table
  )
{
  EFI_STATUS                                                Status;
  UINTN                                                     TableSize;
  UINT32                                                    MemProximityDomainAttrCount;
  UINT32                                                    MemLatBwCount;
  CM_ARCH_COMMON_MEMORY_PROXIMITY_DOMAIN_ATTR_INFO          *MemProximityDomainAttrInfo;
  CM_ARCH_COMMON_MEMORY_LAT_BW_INFO                         *MemLatBwInfos;
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO             *MemLatBwRelations;
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO                      *ProximityDomainInfo;
  UINT32                                                    ProximityDomainInfoCount;
  UINT32                                                    MemLatBwRelationsCount;
  UINTN                                                     MemProximityDomainAttrOffset;
  UINTN                                                     MemLatBwOffset;
  UINT32                                                    Index;
  UINT64                                                    MaxLatBwSize;
  UINTN                                                     LatBwEntrySize;
  EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER  *Hmat;
  VOID                                                      *FinalTable;

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
      "ERROR: HMAT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjProximityDomainInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProximityDomainInfo,
             &ProximityDomainInfoCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HMAT: Failed to get ProximityDomainInfo. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  MemProximityDomainAttrCount = 0;

  Status = GetEArchCommonObjMemoryProximityDomainAttrInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MemProximityDomainAttrInfo,
             &MemProximityDomainAttrCount
             );

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HMAT: Failed to get MemProximityDomain Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  MemLatBwCount = 0;

  Status = GetEArchCommonObjMemoryLatBwInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MemLatBwInfos,
             &MemLatBwCount
             );

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HMAT: Failed to get MemLatBwInfo. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  TableSize = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER);

  MemProximityDomainAttrOffset = TableSize;
  TableSize                   +=
    (sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES) * MemProximityDomainAttrCount);

  MemLatBwOffset = TableSize;

  // The largest SSLBI entry is full matrix where all proximity
  // domains act as initiators and targets. Go for max case, adjust size later.
  MaxLatBwSize = GetSslbiTableSize (ProximityDomainInfoCount, ProximityDomainInfoCount);
  TableSize   += MemLatBwCount * MaxLatBwSize;

  if (TableSize > MAX_UINT32) {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate the Buffer for HMAT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);

  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HMAT: Failed to allocate memory for HMAT Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  Hmat = (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER *)*Table;

  // Build HMAT table.
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Hmat->Header,
             AcpiTableInfo,
             (UINT32)TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HMAT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Status = AddMemProximityDomainAttrInfo (
             CfgMgrProtocol,
             (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES *)((UINT8 *)Hmat + MemProximityDomainAttrOffset),
             MemProximityDomainAttrInfo,
             MemProximityDomainAttrCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to add Proximity Domain Attr structures: %r\n", __func__, Status));
    goto error_handler;
  }

  for (Index = 0; Index < MemLatBwCount; Index++) {
    Status = GetEArchCommonObjProximityDomainRelationInfo (
               CfgMgrProtocol,
               MemLatBwInfos[Index].RelativeDistanceArray,
               &MemLatBwRelations,
               &MemLatBwRelationsCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to get relative distances array: %r\n", __func__, Status));
      goto error_handler;
    }

    Status = AddMemLatBwInfo (
               CfgMgrProtocol,
               &MemLatBwInfos[Index],
               MemLatBwRelations,
               MemLatBwRelationsCount,
               ProximityDomainInfoCount,
               (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *)((UINT8 *)Hmat + MemLatBwOffset),
               &LatBwEntrySize
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to add Lat/Bw info: %r\n", __func__, Status));
      goto error_handler;
    }

    MemLatBwOffset += LatBwEntrySize;
  }

  // Update header with the actual length of the generated table
  // and re-allocate it to an actual size
  Hmat->Header.Length = (UINT32)MemLatBwOffset;
  FinalTable          = AllocateCopyPool (Hmat->Header.Length, *Table);
  if (FinalTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HMAT: Failed to allocate memory for HMAT Table, Size = %d," \
      " Status = %r\n",
      Hmat->Header.Length,
      Status
      ));
    goto error_handler;
  }

  FreePool (*Table);
  *Table = FinalTable;

  return Status;

error_handler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** This macro defines the HMAT Table Generator revision.
*/
#define HMAT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the HMAT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  HmatGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdHmat),
  // Generator Description
  L"ACPI.STD.HMAT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  HMAT_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  BuildHmatTable,
  // Free table function. Use the extended version instead.
  FreeHmatTable,
  // Extended Build table function.
  NULL,
  // Extended free function.
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
AcpiHmatLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&HmatGenerator);
  DEBUG ((DEBUG_INFO, "HMAT: Register Generator. Status = %r\n", Status));
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
AcpiHmatLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&HmatGenerator);
  DEBUG ((DEBUG_INFO, "HMAT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
