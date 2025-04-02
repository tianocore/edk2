/** @file
  HMAT Table Generator

  Copyright (c) 2025, Google Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>
#include <Library/BaseMemoryLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard HMAT Table Generator

  Constructs the HMAT table.

*/

/** Add the MemProxDomainAttr Information
    to the HMAT Table.

  @param [in]  MemProxDomainAttrInfo       Pointer to EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES.
  @param [in]  CmMemProxDomainAttrInfo     Pointer to CM_ARM_MEMORY_PROX_DOMAIN_ATTR_INFO in CM.
  @param [in]  MemProxDomainAttrCount      Counts of MemProxDomainAttrInfo
**/
STATIC
VOID
AddMemProxDomainAttrInfo (
  IN  EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES  *MemProxDomainAttrInfo,
  IN  CONST CM_ARM_MEMORY_PROX_DOMAIN_ATTR_INFO                       *CmMemProxDomainAttrInfo,
  IN  UINT32                                                          MemProxDomainAttrCount
  )
{
  ASSERT (MemProxDomainAttrInfo != NULL);
  ASSERT (CmMemProxDomainAttrInfo != NULL);

  while (MemProxDomainAttrCount-- != 0) {
    MemProxDomainAttrInfo->Type   = CmMemProxDomainAttrInfo->Type;
    MemProxDomainAttrInfo->Length = sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES);
    CopyMem (&MemProxDomainAttrInfo->Flags, &CmMemProxDomainAttrInfo->Flags, sizeof (UINT16));
    MemProxDomainAttrInfo->InitiatorProximityDomain = CmMemProxDomainAttrInfo->ProcessorProximityDomain;
    MemProxDomainAttrInfo->MemoryProximityDomain    = CmMemProxDomainAttrInfo->MemoryProximityDomain;
    MemProxDomainAttrInfo++;
    CmMemProxDomainAttrInfo++;
  }
}

/** Add the MemLatBwInfo Information
    to the HMAT Table.

  @param [in, out]  MemLatBwInfo            Pointer to EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO.
  @param [in]  CmMemLatBwInfo               Pointer to CM_ARM_MEMORY_LAT_BW_INFO in CM.
  @param [in]  CmMemInitTargetInfo          Pointer to CM_ARM_MEMORY_INIT_TARGET_INFO.
  @param [in]  MemLatBwInfoLength           Length of Memort Latency BW info.
**/
STATIC
VOID
AddMemLatBwInfo (
  IN  OUT EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO  *MemLatBwInfo,
  IN  CONST CM_ARM_MEMORY_LAT_BW_INFO                                             *CmMemLatBwInfo,
  IN  CONST CM_ARM_MEMORY_INIT_TARGET_INFO                                        *CmMemInitTargetInfo,
  IN  CONST UINT32                                                                MemLatBwInfoLength
  )
{
  ASSERT (MemLatBwInfo != NULL);
  ASSERT (CmMemLatBwInfo != NULL);
  ASSERT (CmMemInitTargetInfo != NULL);

  MemLatBwInfo->Type   = CmMemLatBwInfo->Type;
  MemLatBwInfo->Length = MemLatBwInfoLength;
  CopyMem (&MemLatBwInfo->Flags, &CmMemLatBwInfo->Flags, sizeof (UINT8));
  MemLatBwInfo->DataType                          = CmMemLatBwInfo->DataType;
  MemLatBwInfo->MinTransferSize                   = CmMemLatBwInfo->MinTransferSize;
  MemLatBwInfo->NumberOfInitiatorProximityDomains = CmMemLatBwInfo->InitiatorProximityDomainsNumber;
  MemLatBwInfo->NumberOfTargetProximityDomains    = CmMemLatBwInfo->TargetProximityDomainsNumber;
  MemLatBwInfo->EntryBaseUnit                     = CmMemLatBwInfo->EntryBaseUnit;

  void  *MemLatBwInfoLocal = (void *)MemLatBwInfo;

  MemLatBwInfoLocal += sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO);

  CopyMem (MemLatBwInfoLocal, CmMemLatBwInfo->InitiatorProximityDomainList, sizeof (UINT32) * CmMemInitTargetInfo->NumInitiator);
  MemLatBwInfoLocal += sizeof (UINT32) * CmMemInitTargetInfo->NumInitiator;
  CopyMem (MemLatBwInfoLocal, CmMemLatBwInfo->TargetProximityDomainList, sizeof (UINT32) * CmMemInitTargetInfo->NumTarget);
  MemLatBwInfoLocal += sizeof (UINT32) * CmMemInitTargetInfo->NumTarget;
  CopyMem (MemLatBwInfoLocal, CmMemLatBwInfo->RelativeDistanceEntry, sizeof (UINT16) * CmMemInitTargetInfo->NumTarget * CmMemInitTargetInfo->NumInitiator);
}

/** Add the MemCache Information
    to the HMAT Table.

  @param [in]  MemCacheInfo            Pointer to EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO.
  @param [in]  CmMemCacheInfo          Pointer to CM_ARM_MEMORY_CACHE_INFO in CM.
  @param [in]  MemCacheLength          Length of MemCacheInfo.
**/
STATIC
VOID
AddMemCacheInfo (
  IN  EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO  *MemCacheInfo,
  IN  CONST CM_ARM_MEMORY_CACHE_INFO                      *CmMemCacheInfo,
  IN  CONST UINT32                                        MemCacheLength
  )
{
  ASSERT (MemCacheInfo != NULL);
  ASSERT (CmMemCacheInfo != NULL);

  MemCacheInfo->Type                  = CmMemCacheInfo->Type;
  MemCacheInfo->Length                = MemCacheLength;
  MemCacheInfo->MemoryProximityDomain = CmMemCacheInfo->MemoryProximityDomain;
  MemCacheInfo->MemorySideCacheSize   = CmMemCacheInfo->MemorySideCacheSize;
  CopyMem (&MemCacheInfo->CacheAttributes, &CmMemCacheInfo->CacheAttributes, sizeof (UINT32));
  MemCacheInfo->NumberOfSmbiosHandles = CmMemCacheInfo->NumSmbiosHandles;

  void  *MemCacheInfoLocal = (void *)MemCacheInfo;
  MemCacheInfoLocal += sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO);
  CopyMem (MemCacheInfoLocal, CmMemCacheInfo->SmbiosHandles, sizeof (UINT16) * CmMemCacheInfo->NumSmbiosHandles);
}

/** This macro expands to a function that retrieves the Memory
    Proximity Domain Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
                 EObjNameSpaceArchCommon,
                 EArchCommonObjMemoryInitTargetInfo,
                 CM_ARM_MEMORY_INIT_TARGET_INFO
                 );

/** This macro expands to a function that retrieves the Memory
    Proximity Domain Attribute Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
                 EObjNameSpaceArchCommon,
                 EArchCommonObjMemoryProxDomainAttrInfo,
                 CM_ARM_MEMORY_PROX_DOMAIN_ATTR_INFO
                 );

/** This macro expands to a function that retrieves the Memory
    Latency and Bandwidth Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
                 EObjNameSpaceArchCommon,
                 EArchCommonObjMemoryLatBwInfo,
                 CM_ARM_MEMORY_LAT_BW_INFO
                 );

/** This macro expands to a function that retrieves the Memory
    Cache Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
                 EObjNameSpaceArchCommon,
                 EArchCommonObjMemoryCacheInfo,
                 CM_ARM_MEMORY_CACHE_INFO
                 );

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
  @retval EFI_UNSUPPORTED        Unsupported configuration.
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
  EFI_STATUS                           Status;
  UINT32                               TableSize;
  UINT32                               MemInitTargetCount;
  UINT32                               MemProxDomainAttrCount;
  UINT32                               MemLatBwCount;
  UINT32                               MemCacheCount;
  UINT32                               MemLatBwInfoLength;
  UINT32                               MemCacheLength;
  CM_ARM_MEMORY_INIT_TARGET_INFO       *MemInitTargetInfo;
  CM_ARM_MEMORY_PROX_DOMAIN_ATTR_INFO  *MemProxDomainAttrInfo;
  CM_ARM_MEMORY_LAT_BW_INFO            *MemLatBwInfo;
  CM_ARM_MEMORY_CACHE_INFO             *MemCacheInfo;
  UINT32                               MemProxDomainAttrOffset;
  UINT32                               MemLatBwOffset;
  UINT32                               MemCacheOffset;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;
  EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER  *Hmat;

  Status = GetEArchCommonObjMemoryInitTargetInfo (
                                           CfgMgrProtocol,
                                           CM_NULL_TOKEN,
                                           &MemInitTargetInfo,
                                           &MemInitTargetCount
                                           );

  if (EFI_ERROR (Status)) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: Failed to get MemInitTarget Info. Status = %r\n",
            Status
           )
           );
    goto error_handler;
  }

  Status = GetEArchCommonObjMemoryProxDomainAttrInfo (
                                               CfgMgrProtocol,
                                               CM_NULL_TOKEN,
                                               &MemProxDomainAttrInfo,
                                               &MemProxDomainAttrCount
                                               );

  if (EFI_ERROR (Status)) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: Failed to get MemProxDomain Info. Status = %r\n",
            Status
           )
           );
    goto error_handler;
  }

  if (MemProxDomainAttrCount == 0) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: MemProxDomain information not provided.\n"
           )
           );
    ASSERT (MemProxDomainAttrCount != 0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Status = GetEArchCommonObjMemoryLatBwInfo (
                                      CfgMgrProtocol,
                                      CM_NULL_TOKEN,
                                      &MemLatBwInfo,
                                      &MemLatBwCount
                                      );

  if (EFI_ERROR (Status)) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: Failed to get MemLatBwInfo. Status = %r\n",
            Status
           )
           );
    goto error_handler;
  }

  if (MemLatBwCount == 0) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: MemLatBwCount information not provided.\n"
           )
           );
    ASSERT (MemLatBwCount != 0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  Status = GetEArchCommonObjMemoryCacheInfo (
                                      CfgMgrProtocol,
                                      CM_NULL_TOKEN,
                                      &MemCacheInfo,
                                      &MemCacheCount
                                      );

  if (EFI_ERROR (Status)) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: Failed to get MemoryCache Info. Status = %r\n",
            Status
           )
           );
    goto error_handler;
  }

  if (MemCacheCount == 0) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: MemCacheCount information not provided.\n"
           )
           );
    ASSERT (MemCacheCount != 0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  TableSize = sizeof (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER);

  MemProxDomainAttrOffset = TableSize;
  TableSize              += (sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES) * MemProxDomainAttrCount);

  MemLatBwOffset = TableSize;

  MemLatBwInfoLength = sizeof (UINT32) * MemInitTargetInfo->NumInitiator +
                       sizeof (UINT32) * MemInitTargetInfo->NumTarget +
                       sizeof (UINT16) * MemInitTargetInfo->NumInitiator * MemInitTargetInfo->NumTarget +
                       sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO);
  TableSize += MemLatBwInfoLength * MemLatBwCount;

  MemCacheOffset = TableSize;
  MemCacheLength = sizeof (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO) + sizeof (UINT16) * MemCacheInfo->NumSmbiosHandles;
  TableSize     += MemCacheLength * MemCacheCount;

  // Allocate the Buffer for HMAT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);

  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: Failed to allocate memory for HMAT Table, Size = %d," \
            " Status = %r\n",
            TableSize,
            Status
           )
           );
    goto error_handler;
  }

  Hmat = (EFI_ACPI_6_4_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER *)*Table;

  // Build HMAT table.
  Status = AddAcpiHeader (
                          CfgMgrProtocol,
                          This,
                          &Hmat->Header,
                          AcpiTableInfo,
                          TableSize
                          );
  if (EFI_ERROR (Status)) {
    DEBUG (
           (
            DEBUG_ERROR,
            "ERROR: HMAT: Failed to add ACPI header. Status = %r\n",
            Status
           )
           );
    goto error_handler;
  }

  AddMemProxDomainAttrInfo (
                            (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES *)((UINT8 *)Hmat + MemProxDomainAttrOffset),
                            MemProxDomainAttrInfo,
                            MemProxDomainAttrCount
                            );

  AddMemLatBwInfo (
                   (EFI_ACPI_6_4_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO *)((UINT8 *)Hmat + MemLatBwOffset),
                   MemLatBwInfo,
                   MemInitTargetInfo,
                   MemLatBwInfoLength
                   );

  AddMemCacheInfo (
                   (EFI_ACPI_6_4_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO *)((UINT8 *)Hmat + MemCacheOffset),
                   MemCacheInfo,
                   MemCacheLength
                   );

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
