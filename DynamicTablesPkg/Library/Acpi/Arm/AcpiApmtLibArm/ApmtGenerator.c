/** @file
  APMT Table Generator

  Copyright (c) 2026, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for CoreSight™ Performance Monitoring Unit Architecture 1.0,
    Platform Design Document, Revision 1.0, Jan 2022
    (https://support.arm.com/documentation/den0117/latest)

**/

#include <IndustryStandard/ArmPerformanceMonitoringUnitTable.h>
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
#include <MetadataHelpers.h>
#include <Library/CmObjHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Library/MetadataHandlerLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/// Test if the given Processor Hierarchy Info object has the 'ACPI Processor
/// ID valid' flag set
#define IS_ACPI_PROC_ID_VALID(Node)  ((Node->Flags & BIT1) != 0)

/// Test if the given Processor Hierarchy Info object has the 'Node is a Leaf'
/// flag set
#define IS_PROC_NODE_LEAF(Node)  ((Node->Flags & BIT3) != 0)

#define VALID_APMT_NODE_FLAGS_MASK                              \
        (EFI_ACPI_APMT_DUAL_PAGE_EXTENSION_SUPPORTED |          \
         EFI_ACPI_APMT_PROCESSOR_AFFINITY_TYPE_CONTAINER |      \
         EFI_ACPI_APMT_64BIT_SINGLE_COPY_ATOMICITY_SUPPORTED)

#define VALID_APMT_INTERRUPT_FLAGS_MASK                         \
        (EFI_ACPI_APMT_INTERRUPT_MODE_EDGE_TRIGGERED)

/** Set Node Instance information.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CsPmuInfo            Coresight PMU information.
  @param [in, out]  Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
typedef
EFI_STATUS
(EFIAPI *SET_NODE_INSTANCE)(
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST     CfgMgrProtocol,
  IN      CONST CM_ARM_CORESIGHT_PMU_INFO             *CONST     CsPmuInfo,
  IN OUT  EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE          *Node
  );

STATIC BOOLEAN  mPpttPresent;
STATIC BOOLEAN  mSratPresent;
STATIC BOOLEAN  mIortPresent;

/** Arm standard APMT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjCoresightPmuInfo
  - EArchCommonObjProcHierarchyInfo
  - EArmObjGicCInfo
  - EArchCommonObjMemoryAffinityInfo
  - EArmObjSmmuV1SmmuV2
  - EArmObjSmmuV3
  - EArmObjRootComplex
  - EArchCommonObjCacheInfo
*/

/**
  This macro expands to a function that retrieves the Arm Coresight PMU
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjCoresightPmuInfo,
  CM_ARM_CORESIGHT_PMU_INFO
  );

/**
  This macro expands to a function that retrieves the Processor Hierarchy
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProcHierarchyInfo,
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  );

/**
  This macro expands to a function that retrieves the GIC CPU interface
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/**
  This macro expands to a function that retrieves the Memory Affinity
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryAffinityInfo,
  CM_ARCH_COMMON_MEMORY_AFFINITY_INFO
  );

/** This macro expands to a function that retrieves the
    SMMU v1/v2 node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSmmuV1SmmuV2,
  CM_ARM_SMMUV1_SMMUV2_NODE
  );

/** This macro expands to a function that retrieves the
    SMMU v3 node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSmmuV3,
  CM_ARM_SMMUV3_NODE
  );

/** This macro expands to a function that retrieves the
     Root Complex node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjRootComplex,
  CM_ARM_ROOT_COMPLEX_NODE
  );

/**
  This macro expands to a function that retrieves the cache information
  from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCacheInfo,
  CM_ARCH_COMMON_CACHE_INFO
  );

/** Set memory controller instance information in Arm Coresight PMU node.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CsPmuInfo            Coresight PMU information.
  @param [in, out]  Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
STATIC
EFI_STATUS
EFIAPI
SetMemoryControllerInstance (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN      CONST CM_ARM_CORESIGHT_PMU_INFO             *CONST   CsPmuInfo,
  IN OUT        EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE  *Node
  )
{
  EFI_STATUS                           Status;
  CM_OBJECT_TOKEN                      Token;
  CM_ARCH_COMMON_MEMORY_AFFINITY_INFO  *MemAffInfo;
  UINT32                               MemAffCount;
  UINT32                               ProximityDomain;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CsPmuInfo != NULL);
  ASSERT (CsPmuInfo->TypeInstanceToken != CM_NULL_TOKEN);
  ASSERT (Node != NULL);

  if (!mSratPresent) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: SRAT is required for AMPT's Memory Controller Node.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Token = CsPmuInfo->TypeInstanceToken;

  Status = GetEArchCommonObjMemoryAffinityInfo (
             CfgMgrProtocol,
             Token,
             &MemAffInfo,
             &MemAffCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get Memory Affinity Info. Status=%r, Token=0x%p\n",
      Status,
      Token
      ));
    return Status;
  }

  if (MemAffCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: MemAffInfo should be unique for Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetProximityDomainId (
             CfgMgrProtocol,
             MemAffInfo->ProximityDomain,
             MemAffInfo->ProximityDomainToken,
             &ProximityDomain
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get ProximityDomain for memory controller. Status=%r, Token=0x%p\n",
      Status,
      Token
      ));
    return Status;
  }

  Node->NodeInstancePrimary   = ProximityDomain;
  Node->NodeInstanceSecondary = 0;

  return EFI_SUCCESS;
}

/** Set SMMU instance information in Arm Coresight PMU node.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CsPmuInfo            Coresight PMU information.
  @param [in, out]  Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
STATIC
EFI_STATUS
EFIAPI
SetSmmuInstance (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN      CONST CM_ARM_CORESIGHT_PMU_INFO             *CONST   CsPmuInfo,
  IN OUT        EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE  *Node
  )
{
  EFI_STATUS                 Status;
  CM_OBJECT_TOKEN            Token;
  CM_ARM_SMMUV1_SMMUV2_NODE  *SmmuV1V2Node;
  UINT32                     SmmuV1V2NodeCount;
  CM_ARM_SMMUV3_NODE         *SmmuV3Node;
  UINT32                     SmmuV3NodeCount;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CsPmuInfo != NULL);
  ASSERT (CsPmuInfo->TypeInstanceToken != CM_NULL_TOKEN);
  ASSERT (Node != NULL);

  if (!mIortPresent) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: IORT is required for AMPT's SMMU Node.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Token             = CsPmuInfo->TypeInstanceToken;
  SmmuV1V2NodeCount = 0;
  SmmuV3NodeCount   = 0;

  // Get the SMMUv3 node info
  Status = GetEArmObjSmmuV3 (
             CfgMgrProtocol,
             Token,
             &SmmuV3Node,
             &SmmuV3NodeCount
             );
  if (Status == EFI_NOT_FOUND) {
    Status = GetEArmObjSmmuV1SmmuV2 (
               CfgMgrProtocol,
               Token,
               &SmmuV1V2Node,
               &SmmuV1V2NodeCount
               );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get SMMU Info. Status=%r, Token=0x%p\n",
      Status,
      Token
      ));
    return Status;
  }

  if ((SmmuV1V2NodeCount + SmmuV3NodeCount) != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: SMMU Info must be unique for Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (SmmuV3NodeCount != 0) {
    Node->NodeInstancePrimary = SmmuV3Node->Identifier;
  } else {
    Node->NodeInstancePrimary = SmmuV1V2Node->Identifier;
  }

  Node->NodeInstanceSecondary = 0;

  return EFI_SUCCESS;
}

/** Set PCIe root complex instance information in Arm Coresight PMU node.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CsPmuInfo            Coresight PMU information.
  @param [in, out]  Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
STATIC
EFI_STATUS
EFIAPI
SetPcieRootComplexInstance (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN      CONST CM_ARM_CORESIGHT_PMU_INFO             *CONST   CsPmuInfo,
  IN OUT        EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE  *Node
  )
{
  EFI_STATUS                Status;
  CM_OBJECT_TOKEN           Token;
  CM_ARM_ROOT_COMPLEX_NODE  *RootComplexNode;
  UINT32                    RootComplexNodeCount;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CsPmuInfo != NULL);
  ASSERT (CsPmuInfo->TypeInstanceToken != CM_NULL_TOKEN);
  ASSERT (Node != NULL);

  if (!mIortPresent) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: IORT is required for AMPT's PCIe Root Complex Node.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Token = CsPmuInfo->TypeInstanceToken;

  Status = GetEArmObjRootComplex (
             CfgMgrProtocol,
             Token,
             &RootComplexNode,
             &RootComplexNodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get PCIe root complex Info. Status=%r, Token=0x%p\n",
      Status,
      Token
      ));
    return Status;
  }

  if (RootComplexNodeCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: PCIe root complex should be unique for Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Node->NodeInstancePrimary   = RootComplexNode->Identifier;
  Node->NodeInstanceSecondary = 0;

  return EFI_SUCCESS;
}

/** Set ACPI device instance information in Arm Coresight PMU node.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CsPmuInfo            Coresight PMU information.
  @param [in, out]  Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
STATIC
EFI_STATUS
EFIAPI
SetAcpiDeviceInstance (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN      CONST CM_ARM_CORESIGHT_PMU_INFO             *CONST   CsPmuInfo,
  IN OUT        EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE  *Node
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CsPmuInfo != NULL);
  ASSERT (Node != NULL);

  CopyMem (
    &Node->NodeInstancePrimary,
    CsPmuInfo->AcpiDeviceHid,
    sizeof (Node->NodeInstancePrimary)
    );

  Node->NodeInstanceSecondary = CsPmuInfo->AcpiDeviceUid;

  return EFI_SUCCESS;
}

/** Set CPU Cache instance information in Arm Coresight PMU node.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       CsPmuInfo            Coresight PMU information.
  @param [in, out]      Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
STATIC
EFI_STATUS
EFIAPI
SetCpuCacheInstance (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN      CONST CM_ARM_CORESIGHT_PMU_INFO             *CONST   CsPmuInfo,
  IN OUT        EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE  *Node
  )
{
  EFI_STATUS                 Status;
  CM_OBJECT_TOKEN            Token;
  CM_ARCH_COMMON_CACHE_INFO  *CacheStruct;
  UINT32                     CacheStructCount;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CsPmuInfo != NULL);
  ASSERT (CsPmuInfo->TypeInstanceToken != CM_NULL_TOKEN);
  ASSERT (Node != NULL);
  ASSERT (mPpttPresent);

  Token = CsPmuInfo->TypeInstanceToken;

  Status = GetEArchCommonObjCacheInfo (
             CfgMgrProtocol,
             Token,
             &CacheStruct,
             &CacheStructCount
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get Cache Info. Status=%r, Token=0x%p\n",
      Status,
      Token
      ));
    return Status;
  }

  if (CacheStructCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: CacheInfo should be unique for Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  Node->NodeInstancePrimary   = 0;
  Node->NodeInstanceSecondary = CacheStruct->CacheId;

  return EFI_SUCCESS;
}

/**
  Node Instance Information setup functions.
*/
STATIC SET_NODE_INSTANCE  SetNodeInstanceOps[] = {
  SetMemoryControllerInstance,
  SetSmmuInstance,
  SetPcieRootComplexInstance,
  SetAcpiDeviceInstance,
  SetCpuCacheInstance,
};

/** Set Processor Affinity information in Arm Coresight PMU node.

  @param [in]       CfgMgrProtocol       Pointer to the Configuration Manager
                                         Protocol Interface.
  @param [in]       Token                Node Instance Token.
  @param [in]       IsContainer          Whether the affinity is container
                                         or processor.
  @param [out]      Node                 Arm Coresight PMU Node.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER          A parameter is invalid.
  @retval Others                         Error.
**/
STATIC
EFI_STATUS
EFIAPI
SetProcessorAffinity (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN            CM_OBJECT_TOKEN                                Token,
  IN            BOOLEAN                                        IsContainer,
  OUT           EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE  *Node
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNode;
  UINT32                              ProcHierarchyNodeCount;
  CM_ARM_GICC_INFO                    *GicCInfoList;
  UINT32                              GicCInfoCount;
  METADATA_OBJ_UID                    MetadataUid;
  UINT32                              AcpiProcessorUid;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Token != CM_NULL_TOKEN);
  ASSERT (Node != NULL);
  ASSERT (mPpttPresent);

  Status = GetEArchCommonObjProcHierarchyInfo (
             CfgMgrProtocol,
             Token,
             &ProcHierarchyNode,
             &ProcHierarchyNodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get ProcHierarchy Info. Status=%r, Token=0x%p\n",
      Status,
      Token
      ));
    return Status;
  }

  if (ProcHierarchyNodeCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: ProcHierarchy Info should be unique for Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_ACPI_PROC_ID_VALID (ProcHierarchyNode)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: ProcHierarchy should have valid Processor Id Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((IsContainer != !IS_PROC_NODE_LEAF (ProcHierarchyNode)) ||
      (IsContainer != (ProcHierarchyNode->AcpiIdObjectToken == CM_NULL_TOKEN)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Unmatched Container/Processor type. Token=0x%p\n",
      Token
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsContainer) {
    Status = GetEArmObjGicCInfo (
               CfgMgrProtocol,
               ProcHierarchyNode->AcpiIdObjectToken,
               &GicCInfoList,
               &GicCInfoCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: Failed to get ACPI ID Reference object token.  " \
        "AcpiIdObjectToken = %p. RequestorToken = %p. Status = %r\n",
        ProcHierarchyNode->AcpiIdObjectToken,
        Token,
        Status
        ));
      return Status;
    }

    if (GicCInfoCount != 1) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: Failed to find a unique GICC structure. " \
        "GICC Structure Count = %d. AcpiIdObjectToken = %p. RequestorToken = %p\n ",
        GicCInfoCount,
        ProcHierarchyNode->AcpiIdObjectToken,
        Token
        ));
      return EFI_INVALID_PARAMETER;
    }

    AcpiProcessorUid = GicCInfoList->AcpiProcessorUid;
  } else {
    if (ProcHierarchyNode->OverrideNameUidEnabled) {
      AcpiProcessorUid = ProcHierarchyNode->OverrideUid;
    } else {
      MetadataUid.EisaId = 0;
      AsciiStrCpyS (MetadataUid.NameId, METADATA_UID_NAMEID_SIZE, "ACPI0010");
      Status = MetadataHandlerGenerate (
                 GetMetadataRoot (),
                 MetadataTypeUid,
                 Token,
                 NULL,
                 &MetadataUid,
                 sizeof (METADATA_OBJ_UID)
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: APMT: Failed to get Container's Id. Token = %p\n ",
          Token
          ));
        return Status;
      }

      AcpiProcessorUid = MetadataUid.Uid;
    }
  }

  Node->ProcessorAffinity = AcpiProcessorUid;

  return EFI_SUCCESS;
}

/** Construct the APMT ACPI table.

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
EFIAPI
BuildApmtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                             Status;
  UINT32                                                 TableSize;
  UINT32                                                 Idx, Idx2;
  CM_ARM_CORESIGHT_PMU_INFO                              *NodeList;
  UINT32                                                 NodeCount;
  SET_NODE_INSTANCE                                      SetNodeInstance;
  EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_TABLE_HEADER  *Apmt;
  EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE          *CsPmuNode;
  UINT32                                                 *Identifiers;

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
      "ERROR: APMT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table      = NULL;
  Identifiers = NULL;

  mPpttPresent = CheckAcpiTablePresent (CfgMgrProtocol, EStdAcpiTableIdPptt);
  mSratPresent = CheckAcpiTablePresent (CfgMgrProtocol, EStdAcpiTableIdSrat);
  mIortPresent = CheckAcpiTablePresent (CfgMgrProtocol, EStdAcpiTableIdIort);

  if (!mPpttPresent) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: PPTT is required for APMT."
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Get the Named component node info
  Status = GetEArmObjCoresightPmuInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &NodeList,
             &NodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to get Coresight PMU Info. Status = %r\n",
      Status
      ));
    goto ErrorHandler;
  }

  TableSize = sizeof (*Apmt) + (sizeof (*CsPmuNode) * NodeCount);
  if (((UINTN)(sizeof (*Apmt) + (sizeof (*CsPmuNode) * NodeCount))) >= MAX_UINT32) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Invalid Table Size. TableSize=%lx\n",
      TableSize
      ));
    Status = EFI_INVALID_PARAMETER;
    goto ErrorHandler;
  }

  // Allocate the Buffer for APMT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to allocate memory for APMT Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  Identifiers = AllocateZeroPool (NodeCount * sizeof (UINT32));
  if (Identifiers == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to allocate memory for Identifiers\n"
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  Apmt = (EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_TABLE_HEADER *)*Table;
  DEBUG ((
    DEBUG_INFO,
    "APMT: APMT = 0x%p TableSize = 0x%lx\n",
    Apmt,
    TableSize
    ));

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Apmt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: APMT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto ErrorHandler;
  }

  CsPmuNode = (EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE *)(Apmt + 1);

  // Update APMT table
  for (Idx = 0; Idx < NodeCount; Idx++) {
    if ((NodeList[Idx].Flags & ~(VALID_APMT_NODE_FLAGS_MASK)) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: [%d] Invalid Node Flags=0x%x\n",
        Idx,
        NodeList[Idx].Flags
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    if (NodeList[Idx].Type > EFI_ACPI_APMT_NODE_TYPE_CPU_CACHE) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: [%d] Invalid Node Type=0x%x\n",
        Idx,
        NodeList[Idx].Type
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    if ((NodeList[Idx].OverflowInterruptFlags & ~(VALID_APMT_INTERRUPT_FLAGS_MASK)) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: [%d] Invalid Overflow Interrupt Flags=0x%x\n",
        Idx,
        NodeList[Idx].OverflowInterruptFlags
        ));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    for (Idx2 = 0; Idx2 < Idx; Idx2++) {
      if (Identifiers[Idx2] == NodeList[Idx].Identifier) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: APMT: [%d] Duplicated Identifier: 0x%x\n",
          Idx,
          NodeList[Idx].Identifier
          ));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorHandler;
      }
    }

    Identifiers[Idx2] = NodeList[Idx].Identifier;

    Status = SetProcessorAffinity (
               CfgMgrProtocol,
               NodeList[Idx].ProcNodeToken,
               ((NodeList[Idx].Flags & EFI_ACPI_APMT_PROCESSOR_AFFINITY_TYPE_CONTAINER) != 0),
               CsPmuNode
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: [%d] Failed to set Processor Affinity Field. Status=%r\n",
        Idx,
        Status
        ));
      goto ErrorHandler;
    }

    SetNodeInstance = SetNodeInstanceOps[NodeList[Idx].Type];
    Status          = SetNodeInstance (
                        CfgMgrProtocol,
                        &NodeList[Idx],
                        CsPmuNode
                        );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: APMT: [%d] Failed to set Node Instance Fields. Status=%r\n",
        Idx,
        Status
        ));
      goto ErrorHandler;
    }

    CsPmuNode->Length                 = sizeof (EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_NODE);
    CsPmuNode->NodeType               = NodeList[Idx].Type;
    CsPmuNode->NodeFlags              = NodeList[Idx].Flags;
    CsPmuNode->Identifier             = NodeList[Idx].Identifier;
    CsPmuNode->OverflowInterrupt      = NodeList[Idx].OverflowInterrupt;
    CsPmuNode->OverflowInterruptFlags = NodeList[Idx].OverflowInterruptFlags;
    CsPmuNode->BaseAddress0           = NodeList[Idx].BaseAddress0;

    if ((CsPmuNode->NodeFlags & EFI_ACPI_APMT_DUAL_PAGE_EXTENSION_SUPPORTED) != 0) {
      CsPmuNode->BaseAddress1 = NodeList[Idx].BaseAddress1;
    }

    if (NodeList[Idx].ImplementationId != 0) {
      CsPmuNode->ImplementationId = NodeList[Idx].ImplementationId;
    }

    CsPmuNode++;
  }

  Status =  EFI_SUCCESS;

ErrorHandler:
  if (Identifiers != NULL) {
    FreePool (Identifiers);
  }

  if (EFI_ERROR (Status) && (*Table != NULL)) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free any resources allocated for constructing the APMT

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
FreeApmtTableResources (
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
    DEBUG ((DEBUG_ERROR, "ERROR: APMT: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** The APMT Table Generator revision.
*/
#define APMT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the APMT Table Generator.
*/
STATIC
ACPI_TABLE_GENERATOR  ApmtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdApmt),
  // Generator Description
  L"ACPI.STD.APMT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_ARM_PERFORMANCE_MONITORING_UNIT_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_ARM_PERFORMANCE_MONITORING_UNIT_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  APMT_GENERATOR_REVISION,
  // Build Table function
  BuildApmtTable,
  // Free Resource function
  FreeApmtTableResources,
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
AcpiApmtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&ApmtGenerator);
  DEBUG ((DEBUG_INFO, "APMT: Register Generator. Status = %r\n", Status));
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
AcpiApmtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&ApmtGenerator);
  DEBUG ((DEBUG_INFO, "APMT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
