/** @file
  PPTT Table Generator

  Copyright (c) 2021, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.4 Specification, January 2021

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "PpttGenerator.h"

/**
  ARM standard PPTT Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - EArchCommonObjProcHierarchyInfo (REQUIRED)
    - EArchCommonObjCacheInfo
    - EArchCommonObjCmRef
    - EArmObjGicCInfo (REQUIRED)
*/

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
  This macro expands to a function that retrieves the cache information
  from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCacheInfo,
  CM_ARCH_COMMON_CACHE_INFO
  );

/**
  This macro expands to a function that retrieves the cross-CM-object-
  reference information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
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
  Returns the size of the PPTT Processor Hierarchy Node (Type 0) given a
  Processor Hierarchy Info CM object.

  @param [in]  Node     Pointer to Processor Hierarchy Info CM object which
                        represents the Processor Hierarchy Node to be generated.

  @retval               Size of the Processor Hierarchy Node in bytes.
**/
STATIC
UINT32
GetProcHierarchyNodeSize (
  IN  CONST CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *Node
  )
{
  ASSERT (Node != NULL);

  // <size of Processor Hierarchy Node> + <size of Private Resources array>
  return sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR) +
         (Node->NoOfPrivateResources * sizeof (UINT32));
}

/**
  This macro expands to a function that retrieves the amount of memory required
  to store the Processor Hierarchy Nodes (Type 0) and updates the Node Indexer.
*/
GET_SIZE_OF_PPTT_STRUCTS (
  ProcHierarchyNodes,
  GetProcHierarchyNodeSize (NodesToIndex),
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  );

/**
  This macro expands to a function that retrieves the amount of memory required
  to store the Cache Type Structures (Type 1) and updates the Node Indexer.
*/
GET_SIZE_OF_PPTT_STRUCTS (
  CacheTypeStructs,
  sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE),
  CM_ARCH_COMMON_CACHE_INFO
  );

/**
  Search the Node Indexer and return the indexed PPTT node with the given
  Token.

  @param [in]  NodeIndexer          Pointer to the Node Indexer array.
  @param [in]  NodeCount            Number of elements in Node Indexer.
  @param [in]  SearchToken          Token used for Node Indexer lookup.
  @param [out] IndexedNodeFound     Pointer to the Node Indexer array element
                                    with the given Token.

  @retval EFI_SUCCESS               Success.
  @retval EFI_NOT_FOUND             No element with a matching token was
                                    found in the Node Indexer array.
**/
STATIC
EFI_STATUS
GetPpttNodeReferencedByToken (
  IN          PPTT_NODE_INDEXER  *NodeIndexer,
  IN  UINT32                     NodeCount,
  IN  CONST   CM_OBJECT_TOKEN    SearchToken,
  OUT         PPTT_NODE_INDEXER  **IndexedNodeFound
  )
{
  EFI_STATUS  Status;

  ASSERT (NodeIndexer != NULL);

  DEBUG ((
    DEBUG_INFO,
    "PPTT: Node Indexer: SearchToken = %p\n",
    SearchToken
    ));

  while (NodeCount-- != 0) {
    DEBUG ((
      DEBUG_INFO,
      "PPTT: Node Indexer: NodeIndexer->Token = %p. Offset = %d\n",
      NodeIndexer->Token,
      NodeIndexer->Offset
      ));

    if (NodeIndexer->Token == SearchToken) {
      *IndexedNodeFound = NodeIndexer;
      Status            = EFI_SUCCESS;
      DEBUG ((
        DEBUG_INFO,
        "PPTT: Node Indexer: Token = %p. Found, Status = %r\n",
        SearchToken,
        Status
        ));
      return Status;
    }

    NodeIndexer++;
  }

  Status = EFI_NOT_FOUND;
  DEBUG ((
    DEBUG_ERROR,
    "PPTT: Node Indexer: SearchToken = %p. Status = %r\n",
    SearchToken,
    Status
    ));

  return Status;
}

/**
  Detect cycles in the processor and cache topology graph represented in
  the PPTT table.

  @param [in]  Generator            Pointer to the PPTT Generator.

  @retval EFI_SUCCESS               There are no cyclic references in the graph.
  @retval EFI_INVALID_PARAMETER     Processor or cache references form a cycle.
**/
STATIC
EFI_STATUS
DetectCyclesInTopology (
  IN  CONST ACPI_PPTT_GENERATOR         *CONST  Generator
  )
{
  EFI_STATUS         Status;
  PPTT_NODE_INDEXER  *Iterator;
  PPTT_NODE_INDEXER  *CycleDetector;
  UINT32             NodesRemaining;

  ASSERT (Generator != NULL);

  Iterator       = Generator->NodeIndexer;
  NodesRemaining = Generator->ProcTopologyStructCount;

  while (NodesRemaining != 0) {
    DEBUG ((
      DEBUG_INFO,
      "INFO: PPTT: Cycle detection for element with index %d\n",
      Generator->ProcTopologyStructCount - NodesRemaining
      ));

    CycleDetector = Iterator;

    // Walk the topology tree
    while (CycleDetector->TopologyParent != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: PPTT: %p -> %p\n",
        CycleDetector->Token,
        CycleDetector->TopologyParent->Token
        ));

      // Check if we have already visited this node
      if (CycleDetector->CycleDetectionStamp == NodesRemaining) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Cycle in processor and cache topology detected for " \
          "a chain of references originating from a node with: Token = %p " \
          "Status = %r\n",
          Iterator->Token,
          Status
          ));
        return Status;
      }

      // Stamp the visited node
      CycleDetector->CycleDetectionStamp = NodesRemaining;
      CycleDetector                      = CycleDetector->TopologyParent;
    } // Continue topology tree walk

    Iterator++;
    NodesRemaining--;
  } // Next Node Indexer

  return EFI_SUCCESS;
}

/**
  Update the array of private resources for a given Processor Hierarchy Node.

  @param [in]  Generator            Pointer to the PPTT Generator.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  PrivResArray         Pointer to the array of private resources.
  @param [in]  PrivResCount         Number of private resources.
  @param [in]  PrivResArrayToken    Reference Token for the CM_ARCH_COMMON_OBJ_REF
                                    array describing node's private resources.

  @retval EFI_SUCCESS               Array updated successfully.
  @retval EFI_INVALID_PARAMETER     A parameter is invalid.
  @retval EFI_NOT_FOUND             A private resource was not found.
**/
STATIC
EFI_STATUS
AddPrivateResources (
  IN  CONST ACPI_PPTT_GENERATOR                    *CONST  Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN        UINT32                                         *PrivResArray,
  IN        UINT32                                         PrivResCount,
  IN  CONST CM_OBJECT_TOKEN                                PrivResArrayToken
  )
{
  EFI_STATUS              Status;
  CM_ARCH_COMMON_OBJ_REF  *CmObjRefs;
  UINT32                  CmObjRefCount;
  PPTT_NODE_INDEXER       *PpttNodeFound;

  ASSERT (
    (Generator != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (PrivResArray != NULL) &&
    (PrivResCount != 0)
    );

  // Validate input arguments
  if (PrivResArrayToken == CM_NULL_TOKEN) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: The number of private resources is %d while " \
      "PrivResToken = CM_NULL_TOKEN. Status = %r\n",
      PrivResCount,
      Status
      ));
    return Status;
  }

  CmObjRefCount = 0;
  // Get the CM Object References
  Status = GetEArchCommonObjCmRef (
             CfgMgrProtocol,
             PrivResArrayToken,
             &CmObjRefs,
             &CmObjRefCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to get CM Object References. " \
      "PrivResToken = %p. Status = %r\n",
      PrivResArrayToken,
      Status
      ));
    return Status;
  }

  if (CmObjRefCount != PrivResCount) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: The number of CM Object References retrieved and the " \
      "number of private resources don't match. CmObjRefCount = %d. " \
      "PrivResourceCount = %d. PrivResToken = %p. Status = %r\n",
      CmObjRefCount,
      PrivResCount,
      PrivResArrayToken,
      Status
      ));
    return Status;
  }

  while (PrivResCount-- != 0) {
    if (CmObjRefs->ReferenceToken == CM_NULL_TOKEN) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: CM_NULL_TOKEN provided as reference token for a " \
        "private resource. Status = %r\n",
        Status
        ));
      return Status;
    }

    // The Node indexer has the Processor hierarchy nodes at the begining
    // followed by the cache structs. Therefore we can skip the Processor
    // hierarchy nodes in the node indexer search.
    Status = GetPpttNodeReferencedByToken (
               Generator->CacheStructIndexedList,
               (Generator->ProcTopologyStructCount -
                Generator->ProcHierarchyNodeCount),
               CmObjRefs->ReferenceToken,
               &PpttNodeFound
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: Failed to get a private resource with Token = %p from " \
        "Node Indexer. Status = %r\n",
        CmObjRefs->ReferenceToken,
        Status
        ));
      return Status;
    }

    // Update the offset of the private resources in the Processor
    // Hierarchy Node structure
    *(PrivResArray++) = PpttNodeFound->Offset;
    CmObjRefs++;
  }

  return EFI_SUCCESS;
}

/**
  Function to test if two indexed Processor Hierarchy Info objects map to the
  same GIC CPU Interface Info object.

  This is a callback function that can be invoked by FindDuplicateValue ().

  @param [in]  Object1        Pointer to the first indexed Processor Hierarchy
                              Info object.
  @param [in]  Object2        Pointer to the second indexed Processor Hierarchy
                              Info object.
  @param [in]  Index1         Index of Object1 to be displayed for debugging
                              purposes.
  @param [in]  Index2         Index of Object2 to be displayed for debugging
                              purposes.

  @retval TRUE                Object1 and Object2 have the same
                              AcpiIdObjectToken.
  @retval FALSE               Object1 and Object2 have different
                              AcpiIdObjectTokens.
**/
BOOLEAN
EFIAPI
IsAcpiIdObjectTokenEqual (
  IN  CONST VOID   *Object1,
  IN  CONST VOID   *Object2,
  IN        UINTN  Index1,
  IN        UINTN  Index2
  )
{
  PPTT_NODE_INDEXER                   *IndexedObject1;
  PPTT_NODE_INDEXER                   *IndexedObject2;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcNode1;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcNode2;

  ASSERT (
    (Object1 != NULL) &&
    (Object2 != NULL)
    );

  IndexedObject1 = (PPTT_NODE_INDEXER *)Object1;
  IndexedObject2 = (PPTT_NODE_INDEXER *)Object2;
  ProcNode1      = (CM_ARCH_COMMON_PROC_HIERARCHY_INFO *)IndexedObject1->Object;
  ProcNode2      = (CM_ARCH_COMMON_PROC_HIERARCHY_INFO *)IndexedObject2->Object;

  if (IS_ACPI_PROC_ID_VALID (ProcNode1) &&
      IS_ACPI_PROC_ID_VALID (ProcNode2) &&
      (ProcNode1->AcpiIdObjectToken != CM_NULL_TOKEN) &&
      (ProcNode2->AcpiIdObjectToken != CM_NULL_TOKEN) &&
      (ProcNode1->AcpiIdObjectToken == ProcNode2->AcpiIdObjectToken))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Two Processor Hierarchy Info objects (%d and %d) map to " \
      "the same ACPI ID reference object. ACPI Processor IDs are not unique. " \
      "AcpiIdObjectToken = %p.\n",
      Index1,
      Index2,
      ProcNode1->AcpiIdObjectToken
      ));
    return TRUE;
  }

  return FALSE;
}

/**
  Update the Processor Hierarchy Node (Type 0) information.

  This function populates the Processor Hierarchy Nodes with information from
  the Configuration Manager and adds this information to the PPTT table.

  @param [in]  Generator            Pointer to the PPTT Generator.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  Pptt                 Pointer to PPTT table structure.
  @param [in]  NodesStartOffset     Offset from the start of PPTT table to the
                                    start of Processor Hierarchy Nodes.

  @retval EFI_SUCCESS               Node updated successfully.
  @retval EFI_INVALID_PARAMETER     A parameter is invalid.
  @retval EFI_NOT_FOUND             The required object was not found.
**/
STATIC
EFI_STATUS
AddProcHierarchyNodes (
  IN  CONST ACPI_PPTT_GENERATOR                   *CONST             Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST             CfgMgrProtocol,
  IN  CONST EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  *Pptt,
  IN  CONST UINT32                                                   NodesStartOffset
  )
{
  EFI_STATUS                             Status;
  EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR  *ProcStruct;
  UINT32                                 *PrivateResources;
  BOOLEAN                                IsAcpiIdObjectTokenDuplicated;

  CM_ARM_GICC_INFO  *GicCInfoList;
  UINT32            GicCInfoCount;
  UINT32            UniqueGicCRefCount;

  PPTT_NODE_INDEXER                   *PpttNodeFound;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcInfoNode;

  PPTT_NODE_INDEXER  *ProcNodeIterator;
  UINT32             NodeCount;
  UINT32             Length;

  ASSERT (
    (Generator != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (Pptt != NULL)
    );

  ProcStruct = (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR *)((UINT8 *)Pptt +
                                                         NodesStartOffset);

  ProcNodeIterator = Generator->ProcHierarchyNodeIndexedList;
  NodeCount        = Generator->ProcHierarchyNodeCount;

  // Check if every GICC Object is referenced by onlu one Proc Node
  IsAcpiIdObjectTokenDuplicated = FindDuplicateValue (
                                    ProcNodeIterator,
                                    NodeCount,
                                    sizeof (PPTT_NODE_INDEXER),
                                    IsAcpiIdObjectTokenEqual
                                    );
  // Duplicate GIC CPU Interface Token was found so two PPTT Processor Hierarchy
  // Nodes map to the same MADT GICC structure
  if (IsAcpiIdObjectTokenDuplicated) {
    return EFI_INVALID_PARAMETER;
  }

  UniqueGicCRefCount = 0;

  while (NodeCount-- != 0) {
    ProcInfoNode = (CM_ARCH_COMMON_PROC_HIERARCHY_INFO *)ProcNodeIterator->Object;

    // Check if the private resource count is within the size limit
    // imposed on the Processor Hierarchy node by the specification.
    // Note: The length field is 8 bit wide while the number of private
    // resource field is 32 bit wide.
    Length = GetProcHierarchyNodeSize (ProcInfoNode);
    if (Length > MAX_UINT8) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: Too many private resources. Count = %d. " \
        "Maximum supported Processor Node size exceeded. " \
        "Token = %p. Status = %r\n",
        ProcInfoNode->NoOfPrivateResources,
        ProcInfoNode->ParentToken,
        Status
        ));
      return Status;
    }

    // Populate the node header
    ProcStruct->Type        = EFI_ACPI_6_4_PPTT_TYPE_PROCESSOR;
    ProcStruct->Length      = (UINT8)Length;
    ProcStruct->Reserved[0] = EFI_ACPI_RESERVED_BYTE;
    ProcStruct->Reserved[1] = EFI_ACPI_RESERVED_BYTE;

    // Populate the flags
    ProcStruct->Flags.PhysicalPackage         = ProcInfoNode->Flags & BIT0;
    ProcStruct->Flags.AcpiProcessorIdValid    = (ProcInfoNode->Flags & BIT1) >> 1;
    ProcStruct->Flags.ProcessorIsAThread      = (ProcInfoNode->Flags & BIT2) >> 2;
    ProcStruct->Flags.NodeIsALeaf             = (ProcInfoNode->Flags & BIT3) >> 3;
    ProcStruct->Flags.IdenticalImplementation =
      (ProcInfoNode->Flags & BIT4) >> 4;
    ProcStruct->Flags.Reserved = 0;

    // Populate the parent reference
    if (ProcInfoNode->ParentToken == CM_NULL_TOKEN) {
      ProcStruct->Parent = 0;
    } else {
      Status = GetPpttNodeReferencedByToken (
                 Generator->ProcHierarchyNodeIndexedList,
                 Generator->ProcHierarchyNodeCount,
                 ProcInfoNode->ParentToken,
                 &PpttNodeFound
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Failed to get parent processor hierarchy node " \
          "reference. ParentToken = %p. ChildToken = %p. Status = %r\n",
          ProcInfoNode->ParentToken,
          ProcInfoNode->Token,
          Status
          ));
        return Status;
      }

      // Test if the reference is to a 'leaf' node
      if (IS_PROC_NODE_LEAF (
            ((CM_ARCH_COMMON_PROC_HIERARCHY_INFO *)PpttNodeFound->Object)
            ))
      {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Reference to a leaf Processor Hierarchy Node. " \
          "ParentToken = %p. ChildToken = %p. Status = %r\n",
          ProcInfoNode->ParentToken,
          ProcInfoNode->Token,
          Status
          ));
        return Status;
      }

      // Update Proc Structure with the offset of the parent node
      ProcStruct->Parent = PpttNodeFound->Offset;

      // Store the reference for the parent node in the Node Indexer
      // so that this can be used later for cycle detection
      ProcNodeIterator->TopologyParent = PpttNodeFound;
    }

    // Populate ACPI Processor ID
    if (!IS_ACPI_PROC_ID_VALID (ProcInfoNode)) {
      // Default invalid ACPI Processor ID to 0
      ProcStruct->AcpiProcessorId = 0;
    } else if (ProcInfoNode->AcpiIdObjectToken == CM_NULL_TOKEN) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: The 'ACPI Processor ID valid' flag is set but no " \
        "ACPI ID Reference object token was provided. " \
        "AcpiIdObjectToken = %p. RequestorToken = %p. Status = %r\n",
        ProcInfoNode->AcpiIdObjectToken,
        ProcInfoNode->Token,
        Status
        ));
      return Status;
    } else {
      Status = GetEArmObjGicCInfo (
                 CfgMgrProtocol,
                 ProcInfoNode->AcpiIdObjectToken,
                 &GicCInfoList,
                 &GicCInfoCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Failed to get ACPI ID Reference object token.  " \
          "ACPI Processor ID can't be populated. " \
          "AcpiIdObjectToken = %p. RequestorToken = %p. Status = %r\n",
          ProcInfoNode->AcpiIdObjectToken,
          ProcInfoNode->Token,
          Status
          ));
        return Status;
      }

      if (GicCInfoCount != 1) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Failed to find a unique GICC structure. " \
          "ACPI Processor ID can't be populated. " \
          "GICC Structure Count = %d. AcpiIdObjectToken = %p. RequestorToken = %p " \
          "Status = %r\n",
          GicCInfoCount,
          ProcInfoNode->AcpiIdObjectToken,
          ProcInfoNode->Token,
          Status
          ));
        return Status;
      }

      // Update the ACPI Processor Id
      ProcStruct->AcpiProcessorId = GicCInfoList->AcpiProcessorUid;

      // Increment the reference count for the number of
      // Unique GICC objects that were retrieved.
      UniqueGicCRefCount++;
    }

    ProcStruct->NumberOfPrivateResources = ProcInfoNode->NoOfPrivateResources;
    PrivateResources                     = (UINT32 *)((UINT8 *)ProcStruct +
                                                      sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR));

    if (ProcStruct->NumberOfPrivateResources != 0) {
      // Populate the private resources array
      Status = AddPrivateResources (
                 Generator,
                 CfgMgrProtocol,
                 PrivateResources,
                 ProcStruct->NumberOfPrivateResources,
                 ProcInfoNode->PrivateResourcesArrayToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Failed to populate the private resources array. " \
          "Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Next Processor Hierarchy Node
    ProcStruct = (EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR *)((UINT8 *)ProcStruct +
                                                           ProcStruct->Length);
    ProcNodeIterator++;
  } // Processor Hierarchy Node

  // Knowing the total number of GICC references made and that all GICC Token
  // references are unique, we can test if no GICC instances have been left out.
  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicCInfoList,
             &GicCInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to get GICC Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  // MADT - PPTT cross validation
  // This checks that one and only one GICC structure is referenced by a
  // Processor Hierarchy Node in the PPTT.
  // Since we have already checked that the GICC objects referenced by the
  // Proc Nodes are unique, the UniqueGicCRefCount cannot be greater than
  // the total number of GICC objects in the platform.
  if (GicCInfoCount > UniqueGicCRefCount) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: %d GICC structure(s) exposed by MADT don't have " \
      "a corresponding Processor Hierarchy Node. Status = %r\n",
      GicCInfoCount - UniqueGicCRefCount,
      Status
      ));
  }

  return Status;
}

/**
  Test whether CacheId is unique among the CacheIdList.

  @param [in]  CacheId          Cache ID to check.
  @param [in]  CacheIdList      List of already existing cache IDs.
  @param [in]  CacheIdListSize  Size of CacheIdList.

  @retval TRUE                  CacheId does not exist in CacheIdList.
  @retval FALSE                 CacheId already exists in CacheIdList.
**/
STATIC
BOOLEAN
IsCacheIdUnique (
  IN CONST UINT32  CacheId,
  IN CONST UINT32  *CacheIdList,
  IN CONST UINT32  CacheIdListSize
  )
{
  UINT32  Index;

  for (Index = 0; Index < CacheIdListSize; Index++) {
    if (CacheIdList[Index] == CacheId) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Update the Cache Type Structure (Type 1) information.

  This function populates the Cache Type Structures with information from
  the Configuration Manager and adds this information to the PPTT table.

  @param [in]  Generator            Pointer to the PPTT Generator.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  Pptt                 Pointer to PPTT table structure.
  @param [in]  NodesStartOffset     Offset from the start of PPTT table to the
                                    start of Cache Type Structures.
  @param [in]  Revision             Revision of the PPTT table being requested.

  @retval EFI_SUCCESS               Structures updated successfully.
  @retval EFI_INVALID_PARAMETER     A parameter is invalid.
  @retval EFI_NOT_FOUND             A required object was not found.
  @retval EFI_OUT_OF_RESOURCES      Out of resources.
**/
STATIC
EFI_STATUS
AddCacheTypeStructures (
  IN  CONST ACPI_PPTT_GENERATOR                   *CONST             Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST             CfgMgrProtocol,
  IN  CONST EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  *Pptt,
  IN  CONST UINT32                                                   NodesStartOffset,
  IN  CONST UINT32                                                   Revision
  )
{
  EFI_STATUS                         Status;
  EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE  *CacheStruct;
  PPTT_NODE_INDEXER                  *PpttNodeFound;
  CM_ARCH_COMMON_CACHE_INFO          *CacheInfoNode;
  PPTT_NODE_INDEXER                  *CacheNodeIterator;
  UINT32                             NodeCount;
  BOOLEAN                            CacheIdUnique;
  UINT32                             NodeIndex;
  UINT32                             *FoundCacheIds;

  ASSERT (
    (Generator != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (Pptt != NULL)
    );

  CacheStruct = (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE *)((UINT8 *)Pptt +
                                                      NodesStartOffset);

  CacheNodeIterator = Generator->CacheStructIndexedList;
  NodeCount         = Generator->CacheStructCount;

  FoundCacheIds = AllocateZeroPool (NodeCount * sizeof (*FoundCacheIds));
  if (FoundCacheIds == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: PPTT: Failed to allocate resources.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  for (NodeIndex = 0; NodeIndex < NodeCount; NodeIndex++) {
    CacheInfoNode = (CM_ARCH_COMMON_CACHE_INFO *)CacheNodeIterator->Object;

    // Populate the node header
    CacheStruct->Type        = EFI_ACPI_6_4_PPTT_TYPE_CACHE;
    CacheStruct->Length      = sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE);
    CacheStruct->Reserved[0] = EFI_ACPI_RESERVED_BYTE;
    CacheStruct->Reserved[1] = EFI_ACPI_RESERVED_BYTE;

    // "On Arm-based systems, all cache properties must be provided in the
    // table." (ACPI 6.4, Section 5.2.29.2)
    CacheStruct->Flags.SizePropertyValid   = 1;
    CacheStruct->Flags.NumberOfSetsValid   = 1;
    CacheStruct->Flags.AssociativityValid  = 1;
    CacheStruct->Flags.AllocationTypeValid = 1;
    CacheStruct->Flags.CacheTypeValid      = 1;
    CacheStruct->Flags.WritePolicyValid    = 1;
    CacheStruct->Flags.LineSizeValid       = 1;
    CacheStruct->Flags.CacheIdValid        = 1;
    CacheStruct->Flags.Reserved            = 0;

    // Populate the reference to the next level of cache
    if (CacheInfoNode->NextLevelOfCacheToken == CM_NULL_TOKEN) {
      CacheStruct->NextLevelOfCache = 0;
    } else {
      Status = GetPpttNodeReferencedByToken (
                 Generator->CacheStructIndexedList,
                 Generator->CacheStructCount,
                 CacheInfoNode->NextLevelOfCacheToken,
                 &PpttNodeFound
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: Failed to get the reference to the Next Level of " \
          "Cache. NextLevelOfCacheToken = %p. RequestorToken = %p. " \
          "Status = %r\n",
          CacheInfoNode->NextLevelOfCacheToken,
          CacheInfoNode->Token,
          Status
          ));
        goto cleanup;
      }

      // Update Cache Structure with the offset for the next level of cache
      CacheStruct->NextLevelOfCache = PpttNodeFound->Offset;

      // Store the next level of cache information in the Node Indexer
      // so that this can be used later for cycle detection
      CacheNodeIterator->TopologyParent = PpttNodeFound;
    }

    CacheStruct->Size = CacheInfoNode->Size;

    // Validate and populate the 'Number of sets' field
    if (CacheInfoNode->NumberOfSets > PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: When ARMv8.3-CCIDX is implemented the maximum number " \
        "of sets can be %d. NumberOfSets = %d. Status = %r\n",
        PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX,
        CacheInfoNode->NumberOfSets,
        Status
        ));
      goto cleanup;
    }

    if (CacheInfoNode->NumberOfSets > PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: PPTT: When ARMv8.3-CCIDX is not implemented the maximum " \
        "number of sets can be %d. NumberOfSets = %d\n",
        PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX,
        CacheInfoNode->NumberOfSets
        ));
    }

    CacheStruct->NumberOfSets = CacheInfoNode->NumberOfSets;

    // Validate Associativity field based on maximum associativity
    // supported by ACPI Cache type structure.
    if (CacheInfoNode->Associativity > MAX_UINT8) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: The maximum associativity supported by ACPI " \
        "Cache type structure is %d. Associativity = %d, Status = %r\n",
        MAX_UINT8,
        CacheInfoNode->Associativity,
        Status
        ));
      goto cleanup;
    }

    // Validate the Associativity field based on the architecture specification
    // The architecture supports much larger associativity values than the
    // current ACPI specification.
    // These checks will be needed in the future when the ACPI specification
    // is extended. Disabling this code for now.
 #if 0
    if (CacheInfoNode->Associativity > PPTT_ARM_CCIDX_CACHE_ASSOCIATIVITY_MAX) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: When ARMv8.3-CCIDX is implemented the maximum cache " \
        "associativity can be %d. Associativity = %d. Status = %r\n",
        PPTT_ARM_CCIDX_CACHE_ASSOCIATIVITY_MAX,
        CacheInfoNode->Associativity,
        Status
        ));
      goto cleanup;
    }

    if (CacheInfoNode->Associativity > PPTT_ARM_CACHE_ASSOCIATIVITY_MAX) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: PPTT: When ARMv8.3-CCIDX is not implemented the maximum " \
        "cache associativity can be %d. Associativity = %d\n",
        PPTT_ARM_CACHE_ASSOCIATIVITY_MAX,
        CacheInfoNode->Associativity
        ));
    }

 #endif

    // Note a typecast is needed as the maximum associativity
    // supported by ACPI Cache type structure is MAX_UINT8.
    CacheStruct->Associativity = (UINT8)CacheInfoNode->Associativity;

    // Populate cache attributes
    CacheStruct->Attributes.AllocationType =
      CacheInfoNode->Attributes & (BIT0 | BIT1);
    CacheStruct->Attributes.CacheType =
      (CacheInfoNode->Attributes & (BIT2 | BIT3)) >> 2;
    CacheStruct->Attributes.WritePolicy =
      (CacheInfoNode->Attributes & BIT4) >> 4;
    CacheStruct->Attributes.Reserved = 0;

    // Validate and populate cache line size
    if ((CacheInfoNode->LineSize < PPTT_ARM_CACHE_LINE_SIZE_MIN) ||
        (CacheInfoNode->LineSize > PPTT_ARM_CACHE_LINE_SIZE_MAX))
    {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: The cache line size must be between %d and %d bytes " \
        "on ARM Platforms. LineSize = %d. Status = %r\n",
        PPTT_ARM_CACHE_LINE_SIZE_MIN,
        PPTT_ARM_CACHE_LINE_SIZE_MAX,
        CacheInfoNode->LineSize,
        Status
        ));
      goto cleanup;
    }

    if ((CacheInfoNode->LineSize & (CacheInfoNode->LineSize - 1)) != 0) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: The cache line size is not a power of 2. " \
        "LineSize = %d. Status = %r\n",
        CacheInfoNode->LineSize,
        Status
        ));
      goto cleanup;
    }

    CacheStruct->LineSize = CacheInfoNode->LineSize;

    if (Revision >= 3) {
      // Validate and populate cache id
      if (CacheInfoNode->CacheId == 0) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: The cache id cannot be zero. Status = %r\n",
          Status
          ));
        goto cleanup;
      }

      CacheIdUnique = IsCacheIdUnique (
                        CacheInfoNode->CacheId,
                        FoundCacheIds,
                        NodeIndex
                        );
      if (!CacheIdUnique) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: PPTT: The cache id is not unique. " \
          "CacheId = %d. Status = %r\n",
          CacheInfoNode->CacheId,
          Status
          ));
        goto cleanup;
      }

      // Store the cache id so we can check future cache ids for uniqueness
      FoundCacheIds[NodeIndex] = CacheInfoNode->CacheId;

      CacheStruct->CacheId = CacheInfoNode->CacheId;
    }

    // Next Cache Type Structure
    CacheStruct = (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE *)((UINT8 *)CacheStruct +
                                                        CacheStruct->Length);
    CacheNodeIterator++;
  } // for Cache Type Structure

  Status = EFI_SUCCESS;

cleanup:
  FreePool (FoundCacheIds);

  return Status;
}

/**
  Construct the PPTT ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This                 Pointer to the table generator.
  @param [in]  AcpiTableInfo        Pointer to the ACPI table generator to be used.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [out] Table                Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS               Table generated successfully.
  @retval EFI_INVALID_PARAMETER     A parameter is invalid.
  @retval EFI_NOT_FOUND             The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE       The size returned by the Configuration
                                    Manager is less than the Object size for
                                    the requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildPpttTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS  Status;
  UINT32      TableSize;
  UINT32      ProcTopologyStructCount;
  UINT32      ProcHierarchyNodeCount;
  UINT32      CacheStructCount;

  UINT32  ProcHierarchyNodeOffset;
  UINT32  CacheStructOffset;

  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNodeList;
  CM_ARCH_COMMON_CACHE_INFO           *CacheStructList;

  ACPI_PPTT_GENERATOR  *Generator;

  // Pointer to the Node Indexer array
  PPTT_NODE_INDEXER  *NodeIndexer;

  EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  *Pptt;

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
      "ERROR: PPTT: Requested table revision = %d is not supported. "
      "Supported table revisions: Minimum = %d. Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  Generator = (ACPI_PPTT_GENERATOR *)This;
  *Table    = NULL;

  // Get the processor hierarchy info and update the processor topology
  // structure count with Processor Hierarchy Nodes (Type 0)
  Status = GetEArchCommonObjProcHierarchyInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProcHierarchyNodeList,
             &ProcHierarchyNodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to get processor hierarchy info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  ProcTopologyStructCount           = ProcHierarchyNodeCount;
  Generator->ProcHierarchyNodeCount = ProcHierarchyNodeCount;

  // Get the cache info and update the processor topology structure count with
  // Cache Type Structures (Type 1)
  Status = GetEArchCommonObjCacheInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CacheStructList,
             &CacheStructCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to get cache info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  ProcTopologyStructCount    += CacheStructCount;
  Generator->CacheStructCount = CacheStructCount;

  // Allocate Node Indexer array
  NodeIndexer = (PPTT_NODE_INDEXER *)AllocateZeroPool (
                                       sizeof (PPTT_NODE_INDEXER) *
                                       ProcTopologyStructCount
                                       );
  if (NodeIndexer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to allocate memory for Node Indexer. Status = %r\n ",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((DEBUG_INFO, "INFO: NodeIndexer = %p\n", NodeIndexer));
  Generator->ProcTopologyStructCount = ProcTopologyStructCount;
  Generator->NodeIndexer             = NodeIndexer;

  // Calculate the size of the PPTT table
  TableSize = sizeof (EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER);

  // Include the size of Processor Hierarchy Nodes and index them
  if (Generator->ProcHierarchyNodeCount != 0) {
    ProcHierarchyNodeOffset                 = TableSize;
    Generator->ProcHierarchyNodeIndexedList = NodeIndexer;
    TableSize                              += GetSizeofProcHierarchyNodes (
                                                ProcHierarchyNodeOffset,
                                                ProcHierarchyNodeList,
                                                Generator->ProcHierarchyNodeCount,
                                                &NodeIndexer
                                                );

    DEBUG ((
      DEBUG_INFO,
      " ProcHierarchyNodeCount = %d\n" \
      " ProcHierarchyNodeOffset = 0x%x\n" \
      " ProcHierarchyNodeIndexedList = 0x%p\n",
      Generator->ProcHierarchyNodeCount,
      ProcHierarchyNodeOffset,
      Generator->ProcHierarchyNodeIndexedList
      ));
  }

  // Include the size of Cache Type Structures and index them
  if (Generator->CacheStructCount != 0) {
    CacheStructOffset                 = TableSize;
    Generator->CacheStructIndexedList = NodeIndexer;
    TableSize                        += GetSizeofCacheTypeStructs (
                                          CacheStructOffset,
                                          CacheStructList,
                                          Generator->CacheStructCount,
                                          &NodeIndexer
                                          );
    DEBUG ((
      DEBUG_INFO,
      " CacheStructCount = %d\n" \
      " CacheStructOffset = 0x%x\n" \
      " CacheStructIndexedList = 0x%p\n",
      Generator->CacheStructCount,
      CacheStructOffset,
      Generator->CacheStructIndexedList
      ));
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: PPTT:\n" \
    " ProcTopologyStructCount = %d\n" \
    " TableSize = %d\n",
    ProcTopologyStructCount,
    TableSize
    ));

  // Allocate the Buffer for the PPTT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to allocate memory for PPTT Table. " \
      "Size = %d. Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  Pptt = (EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER *)*Table;

  DEBUG ((
    DEBUG_INFO,
    "PPTT: Pptt = 0x%p. TableSize = 0x%x\n",
    Pptt,
    TableSize
    ));

  // Add ACPI header
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Pptt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add Processor Hierarchy Nodes (Type 0) to the generated table
  if (Generator->ProcHierarchyNodeCount != 0) {
    Status = AddProcHierarchyNodes (
               Generator,
               CfgMgrProtocol,
               Pptt,
               ProcHierarchyNodeOffset
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: Failed to add Processor Hierarchy Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  // Add Cache Type Structures (Type 1) to the generated table
  if (Generator->CacheStructCount != 0) {
    Status = AddCacheTypeStructures (
               Generator,
               CfgMgrProtocol,
               Pptt,
               CacheStructOffset,
               AcpiTableInfo->AcpiTableRevision
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: PPTT: Failed to add Cache Type Structures. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  // Validate CM object cross-references in PPTT
  Status = DetectCyclesInTopology (Generator);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: PPTT: Invalid processor and cache topology. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  return Status;

error_handler:
  if (Generator->NodeIndexer != NULL) {
    FreePool (Generator->NodeIndexer);
    Generator->NodeIndexer = NULL;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/**
  Free any resources allocated for constructing the PPTT

  @param [in]      This             Pointer to the table generator.
  @param [in]      AcpiTableInfo    Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol   Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in, out] Table            Pointer to the ACPI Table.

  @retval EFI_SUCCESS               The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER     The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreePpttTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ACPI_PPTT_GENERATOR  *Generator;

  ASSERT (
    (This != NULL) &&
    (AcpiTableInfo != NULL) &&
    (CfgMgrProtocol != NULL) &&
    (AcpiTableInfo->TableGeneratorId == This->GeneratorID) &&
    (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature)
    );

  Generator = (ACPI_PPTT_GENERATOR *)This;

  // Free any memory allocated by the generator
  if (Generator->NodeIndexer != NULL) {
    FreePool (Generator->NodeIndexer);
    Generator->NodeIndexer = NULL;
  }

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: PPTT: Invalid Table Pointer\n"));
    ASSERT (
      (Table != NULL) &&
      (*Table != NULL)
      );
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** The PPTT Table Generator revision.
*/
#define PPTT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the PPTT Table Generator.
*/
STATIC
ACPI_PPTT_GENERATOR  PpttGenerator = {
  // ACPI table generator header
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdPptt),
    // Generator Description
    L"ACPI.STD.PPTT.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
    // ACPI Table Revision supported by this Generator
    EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
    // Minimum supported ACPI Table Revision
    EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID,
    // Creator Revision
    PPTT_GENERATOR_REVISION,
    // Build Table function
    BuildPpttTable,
    // Free Resource function
    FreePpttTableResources,
    // Extended build function not needed
    NULL,
    // Extended build function not implemented by the generator.
    // Hence extended free resource function is not required.
    NULL
  },

  // PPTT Generator private data

  // Processor topology node count
  0,
  // Count of Processor Hierarchy Nodes
  0,
  // Count of Cache Structures
  0,
  // Pointer to PPTT Node Indexer
  NULL
};

/**
  Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle        The handle to the image.
  @param [in]  SystemTable        Pointer to the System Table.

  @retval EFI_SUCCESS             The Generator is registered.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_ALREADY_STARTED     The Generator for the Table ID
                                  is already registered.
**/
EFI_STATUS
EFIAPI
AcpiPpttLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&PpttGenerator.Header);
  DEBUG ((DEBUG_INFO, "PPTT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle        The handle to the image.
  @param [in]  SystemTable        Pointer to the System Table.

  @retval EFI_SUCCESS             The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiPpttLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&PpttGenerator.Header);
  DEBUG ((DEBUG_INFO, "PPTT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
