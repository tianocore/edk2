/** @file
  IORT Table Generator

  Copyright (c) 2017 - 2022, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - IO Remapping Table, Platform Design Document, Revision E.d, Feb 2022
    (https://developer.arm.com/documentation/den0049/)

**/

#include <IndustryStandard/IoRemappingTable.h>
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

#include "IortGenerator.h"

/** ARM standard IORT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjItsGroup
  - EArmObjNamedComponent
  - EArmObjRootComplex
  - EArmObjSmmuV1SmmuV2
  - EArmObjSmmuV3
  - EArmObjPmcg
  - EArmObjRmr
  - EArmObjGicItsIdentifierArray
  - EArmObjIdMappingArray
  - EArmObjSmmuInterruptArray
  - EArmObjMemoryRangeDescriptor
*/

/** This macro expands to a function that retrieves the ITS
    Group node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjItsGroup,
  CM_ARM_ITS_GROUP_NODE
  );

/** This macro expands to a function that retrieves the
    Named Component node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjNamedComponent,
  CM_ARM_NAMED_COMPONENT_NODE
  );

/** This macro expands to a function that retrieves the
     Root Complex node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjRootComplex,
  CM_ARM_ROOT_COMPLEX_NODE
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
    PMCG node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPmcg,
  CM_ARM_PMCG_NODE
  );

/** This macro expands to a function that retrieves the
    RMR node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjRmr,
  CM_ARM_RMR_NODE
  );

/** This macro expands to a function that retrieves the
    Memory Range Descriptor Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjMemoryRangeDescriptor,
  CM_ARM_MEMORY_RANGE_DESCRIPTOR
  );

/** This macro expands to a function that retrieves the
    ITS Identifier Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicItsIdentifierArray,
  CM_ARM_ITS_IDENTIFIER
  );

/** This macro expands to a function that retrieves the
    Id Mapping Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjIdMappingArray,
  CM_ARM_ID_MAPPING
  );

/** This macro expands to a function that retrieves the
    SMMU Interrupt Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSmmuInterruptArray,
  CM_ARM_SMMU_INTERRUPT
  );

/** Returns the size of the ITS Group node.

    @param [in]  Node    Pointer to ITS Group node.

    @retval Size of the ITS Group Node.
**/
STATIC
UINT32
GetItsGroupNodeSize (
  IN  CONST CM_ARM_ITS_GROUP_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of ITS Group Node +
     Size of ITS Identifier array
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE) +
                  (Node->ItsIdCount * sizeof (UINT32)));
}

/** Returns the total size required for the ITS Group nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to ITS Group node list.
    @param [in]       NodeCount       Count of the ITS Group nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the ITS Group Nodes.
**/
STATIC
UINT64
GetSizeofItsGroupNodes (
  IN      CONST UINT32                         NodeStartOffset,
  IN      CONST CM_ARM_ITS_GROUP_NODE          *NodeList,
  IN            UINT32                         NodeCount,
  IN OUT        IORT_NODE_INDEXER     **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetItsGroupNodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the Named Component node.

    @param [in]  Node    Pointer to Named Component node.

    @retval Size of the Named Component node.
**/
STATIC
UINT32
GetNamedComponentNodeSize (
  IN  CONST CM_ARM_NAMED_COMPONENT_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of Named Component node +
     Size of ID mapping array +
     Size of ASCII string + 'padding to 32-bit word aligned'.
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_NAMED_COMP_NODE) +
                  (Node->IdMappingCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)) +
                  ALIGN_VALUE (AsciiStrSize (Node->ObjectName), 4));
}

/** Returns the total size required for the Named Component nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to Named Component node list.
    @param [in]       NodeCount       Count of the Named Component nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the Named Component nodes.
**/
STATIC
UINT64
GetSizeofNamedComponentNodes (
  IN      CONST UINT32                              NodeStartOffset,
  IN      CONST CM_ARM_NAMED_COMPONENT_NODE         *NodeList,
  IN            UINT32                              NodeCount,
  IN OUT        IORT_NODE_INDEXER          **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetNamedComponentNodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the Root Complex node.

    @param [in]  Node    Pointer to Root Complex node.

    @retval Size of the Root Complex node.
**/
STATIC
UINT32
GetRootComplexNodeSize (
  IN  CONST CM_ARM_ROOT_COMPLEX_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of Root Complex node +
     Size of ID mapping array
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_RC_NODE) +
                  (Node->IdMappingCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)));
}

/** Returns the total size required for the Root Complex nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to Root Complex node list.
    @param [in]       NodeCount       Count of the Root Complex nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the Root Complex nodes.
**/
STATIC
UINT64
GetSizeofRootComplexNodes (
  IN      CONST UINT32                              NodeStartOffset,
  IN      CONST CM_ARM_ROOT_COMPLEX_NODE            *NodeList,
  IN            UINT32                              NodeCount,
  IN OUT        IORT_NODE_INDEXER          **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetRootComplexNodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the SMMUv1/SMMUv2 node.

    @param [in]  Node    Pointer to SMMUv1/SMMUv2 node list.

    @retval Size of the SMMUv1/SMMUv2 node.
**/
STATIC
UINT32
GetSmmuV1V2NodeSize (
  IN  CONST CM_ARM_SMMUV1_SMMUV2_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of SMMU v1/SMMU v2 node +
     Size of ID mapping array +
     Size of context interrupt array +
     Size of PMU interrupt array
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE) +
                  (Node->IdMappingCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)) +
                  (Node->ContextInterruptCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT)) +
                  (Node->PmuInterruptCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT)));
}

/** Returns the total size required for the SMMUv1/SMMUv2 nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to SMMUv1/SMMUv2 node list.
    @param [in]       NodeCount       Count of the SMMUv1/SMMUv2 nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the SMMUv1/SMMUv2 nodes.
**/
STATIC
UINT64
GetSizeofSmmuV1V2Nodes (
  IN      CONST UINT32                              NodeStartOffset,
  IN      CONST CM_ARM_SMMUV1_SMMUV2_NODE           *NodeList,
  IN            UINT32                              NodeCount,
  IN OUT        IORT_NODE_INDEXER          **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetSmmuV1V2NodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the SMMUv3 node.

    @param [in]  Node    Pointer to SMMUv3 node list.

    @retval Total size of the SMMUv3 nodes.
**/
STATIC
UINT32
GetSmmuV3NodeSize (
  IN  CONST CM_ARM_SMMUV3_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of SMMU v1/SMMU v2 node +
     Size of ID mapping array
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE) +
                  (Node->IdMappingCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)));
}

/** Returns the total size required for the SMMUv3 nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to SMMUv3 node list.
    @param [in]       NodeCount       Count of the SMMUv3 nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the SMMUv3 nodes.
**/
STATIC
UINT64
GetSizeofSmmuV3Nodes (
  IN      CONST UINT32                       NodeStartOffset,
  IN      CONST CM_ARM_SMMUV3_NODE           *NodeList,
  IN            UINT32                       NodeCount,
  IN OUT        IORT_NODE_INDEXER   **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetSmmuV3NodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the PMCG node.

    @param [in]  Node    Pointer to PMCG node.

    @retval Size of the PMCG node.
**/
STATIC
UINT32
GetPmcgNodeSize (
  IN  CONST CM_ARM_PMCG_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of PMCG node +
     Size of ID mapping array
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE) +
                  (Node->IdMappingCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)));
}

/** Returns the total size required for the PMCG nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to PMCG node list.
    @param [in]       NodeCount       Count of the PMCG nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the PMCG nodes.
**/
STATIC
UINT64
GetSizeofPmcgNodes (
  IN      CONST UINT32                     NodeStartOffset,
  IN      CONST CM_ARM_PMCG_NODE           *NodeList,
  IN            UINT32                     NodeCount,
  IN OUT        IORT_NODE_INDEXER **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetPmcgNodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the RMR node.

    @param [in]  Node    Pointer to RMR node.

    @retval Size of the RMR node.
**/
STATIC
UINT32
GetRmrNodeSize (
  IN  CONST CM_ARM_RMR_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  /* Size of RMR node +
     Size of ID mapping array +
     Size of Memory Range Descriptor array
  */
  return (UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE) +
                  (Node->IdMappingCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)) +
                  (Node->MemRangeDescCount *
                   sizeof (EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC)));
}

/** Returns the total size required for the RMR nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      IORT where this node group starts.
    @param [in]       NodeList        Pointer to RMR node list.
    @param [in]       NodeCount       Count of the RMR nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the RMR nodes.
**/
STATIC
UINT64
GetSizeofRmrNodes (
  IN      CONST UINT32                     NodeStartOffset,
  IN      CONST CM_ARM_RMR_NODE            *NodeList,
  IN            UINT32                     NodeCount,
  IN OUT        IORT_NODE_INDEXER **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Token      = NodeList->Token;
    (*NodeIndexer)->Object     = (VOID *)NodeList;
    (*NodeIndexer)->Offset     = (UINT32)(Size + NodeStartOffset);
    (*NodeIndexer)->Identifier = NodeList->Identifier;
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer = %p, Token = %p, Object = %p,"
      " Offset = 0x%x, Identifier = 0x%x\n",
      *NodeIndexer,
      (*NodeIndexer)->Token,
      (*NodeIndexer)->Object,
      (*NodeIndexer)->Offset,
      (*NodeIndexer)->Identifier
      ));

    Size += GetRmrNodeSize (NodeList);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the offset of the Node referenced by the Token.

    @param [in]  NodeIndexer  Pointer to node indexer array.
    @param [in]  NodeCount    Count of the nodes.
    @param [in]  Token        Reference token for the node.
    @param [out] NodeOffset   Offset of the node from the
                              start of the IORT table.

    @retval EFI_SUCCESS       Success.
    @retval EFI_NOT_FOUND     No matching token reference
                              found in node indexer array.
**/
STATIC
EFI_STATUS
GetNodeOffsetReferencedByToken (
  IN  IORT_NODE_INDEXER  *NodeIndexer,
  IN  UINT32             NodeCount,
  IN  CM_OBJECT_TOKEN    Token,
  OUT UINT32             *NodeOffset
  )
{
  DEBUG ((
    DEBUG_INFO,
    "IORT: Node Indexer: Search Token = %p\n",
    Token
    ));
  while (NodeCount-- != 0) {
    DEBUG ((
      DEBUG_INFO,
      "IORT: Node Indexer: NodeIndexer->Token = %p, Offset = %d\n",
      NodeIndexer->Token,
      NodeIndexer->Offset
      ));
    if (NodeIndexer->Token == Token) {
      *NodeOffset = NodeIndexer->Offset;
      DEBUG ((
        DEBUG_INFO,
        "IORT: Node Indexer: Token = %p, Found\n",
        Token
        ));
      return EFI_SUCCESS;
    }

    NodeIndexer++;
  }

  DEBUG ((
    DEBUG_INFO,
    "IORT: Node Indexer: Token = %p, Not Found\n",
    Token
    ));
  return EFI_NOT_FOUND;
}

/** Update the Id Mapping Array.

    This function retrieves the Id Mapping Array object referenced by the
    IdMappingToken and updates the IdMapArray.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     IdMapArray       Pointer to an array of Id Mappings.
    @param [in]     IdCount          Number of Id Mappings.
    @param [in]     IdMappingToken   Reference Token for retrieving the
                                     Id Mapping Array object.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddIdMappingArray (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN            EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE             *IdMapArray,
  IN            UINT32                                         IdCount,
  IN      CONST CM_OBJECT_TOKEN                                IdMappingToken
  )
{
  EFI_STATUS           Status;
  CM_ARM_ID_MAPPING    *IdMappings;
  UINT32               IdMappingCount;
  ACPI_IORT_GENERATOR  *Generator;

  ASSERT (IdMapArray != NULL);

  Generator = (ACPI_IORT_GENERATOR *)This;

  // Get the Id Mapping Array
  Status = GetEArmObjIdMappingArray (
             CfgMgrProtocol,
             IdMappingToken,
             &IdMappings,
             &IdMappingCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get Id Mapping array. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (IdMappingCount < IdCount) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get the required number of Id Mappings.\n"
      ));
    return EFI_NOT_FOUND;
  }

  // Populate the Id Mapping array
  while (IdCount-- != 0) {
    Status = GetNodeOffsetReferencedByToken (
               Generator->NodeIndexer,
               Generator->IortNodeCount,
               IdMappings->OutputReferenceToken,
               &IdMapArray->OutputReference
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to get Output Reference for ITS Identifier array."
        "Reference Token = %p"
        " Status = %r\n",
        IdMappings->OutputReferenceToken,
        Status
        ));
      return Status;
    }

    IdMapArray->InputBase  = IdMappings->InputBase;
    IdMapArray->NumIds     = IdMappings->NumIds;
    IdMapArray->OutputBase = IdMappings->OutputBase;
    IdMapArray->Flags      = IdMappings->Flags;

    IdMapArray++;
    IdMappings++;
  } // Id Mapping array

  return EFI_SUCCESS;
}

/** Update the ITS Group Node Information.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the ITS Group
                                     Nodes.
    @param [in]     NodeList         Pointer to an array of ITS Group Node
                                     Objects.
    @param [in]     NodeCount        Number of ITS Group Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddItsGroupNodes (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE               *Iort,
  IN  CONST UINT32                                        NodesStartOffset,
  IN  CONST CM_ARM_ITS_GROUP_NODE                         *NodeList,
  IN        UINT32                                        NodeCount
  )
{
  EFI_STATUS                          Status;
  EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE  *ItsGroupNode;
  UINT32                              *ItsIds;
  CM_ARM_ITS_IDENTIFIER               *ItsIdentifier;
  UINT32                              ItsIdentifierCount;
  UINT32                              IdIndex;
  UINT64                              NodeLength;

  ASSERT (Iort != NULL);

  ItsGroupNode = (EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE *)((UINT8 *)Iort +
                                                        NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetItsGroupNodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: ITS Id Array Node length 0x%lx > MAX_UINT16."
        " Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    // Populate the node header
    ItsGroupNode->Node.Type          = EFI_ACPI_IORT_TYPE_ITS_GROUP;
    ItsGroupNode->Node.Length        = (UINT16)NodeLength;
    ItsGroupNode->Node.NumIdMappings = 0;
    ItsGroupNode->Node.IdReference   = 0;

    if (AcpiTableInfo->AcpiTableRevision <
        EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
    {
      ItsGroupNode->Node.Revision   = 0;
      ItsGroupNode->Node.Identifier = EFI_ACPI_RESERVED_DWORD;
    } else {
      ItsGroupNode->Node.Revision   = 1;
      ItsGroupNode->Node.Identifier = NodeList->Identifier;
    }

    // IORT specific data
    ItsGroupNode->NumItsIdentifiers = NodeList->ItsIdCount;
    ItsIds                          = (UINT32 *)((UINT8 *)ItsGroupNode +
                                                 sizeof (EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE));

    Status = GetEArmObjGicItsIdentifierArray (
               CfgMgrProtocol,
               NodeList->ItsIdToken,
               &ItsIdentifier,
               &ItsIdentifierCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to get ITS Identifier array. Status = %r\n",
        Status
        ));
      return Status;
    }

    if (ItsIdentifierCount < ItsGroupNode->NumItsIdentifiers) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to get the required number of ITS Identifiers.\n"
        ));
      return EFI_NOT_FOUND;
    }

    // Populate the ITS identifier array
    for (IdIndex = 0; IdIndex < ItsGroupNode->NumItsIdentifiers; IdIndex++) {
      ItsIds[IdIndex] = ItsIdentifier[IdIndex].ItsId;
    } // ITS identifier array

    // Next IORT Group Node
    ItsGroupNode = (EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE *)((UINT8 *)ItsGroupNode +
                                                          ItsGroupNode->Node.Length);
    NodeList++;
  } // IORT Group Node

  return EFI_SUCCESS;
}

/** Update the Named Component Node Information.

    This function updates the Named Component node information in the IORT
    table.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the Named
                                     Component Nodes.
    @param [in]     NodeList         Pointer to an array of Named Component
                                     Node Objects.
    @param [in]     NodeCount        Number of Named Component Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddNamedComponentNodes (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE                *Iort,
  IN      CONST UINT32                                         NodesStartOffset,
  IN      CONST CM_ARM_NAMED_COMPONENT_NODE                    *NodeList,
  IN            UINT32                                         NodeCount
  )
{
  EFI_STATUS                                 Status;
  EFI_ACPI_6_0_IO_REMAPPING_NAMED_COMP_NODE  *NcNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE         *IdMapArray;
  CHAR8                                      *ObjectName;
  UINTN                                      ObjectNameLength;
  UINT64                                     NodeLength;

  ASSERT (Iort != NULL);

  NcNode = (EFI_ACPI_6_0_IO_REMAPPING_NAMED_COMP_NODE *)((UINT8 *)Iort +
                                                         NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetNamedComponentNodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Named Component Node length 0x%lx > MAX_UINT16."
        " Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    // Populate the node header
    NcNode->Node.Type          = EFI_ACPI_IORT_TYPE_NAMED_COMP;
    NcNode->Node.Length        = (UINT16)NodeLength;
    NcNode->Node.NumIdMappings = NodeList->IdMappingCount;

    if (AcpiTableInfo->AcpiTableRevision <
        EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
    {
      NcNode->Node.Revision   = 2;
      NcNode->Node.Identifier = EFI_ACPI_RESERVED_DWORD;
    } else {
      NcNode->Node.Revision   = 4;
      NcNode->Node.Identifier = NodeList->Identifier;
    }

    ObjectNameLength         = AsciiStrLen (NodeList->ObjectName) + 1;
    NcNode->Node.IdReference = (NodeList->IdMappingCount == 0) ?
                               0 : ((UINT32)(sizeof (EFI_ACPI_6_0_IO_REMAPPING_NAMED_COMP_NODE) +
                                             (ALIGN_VALUE (ObjectNameLength, 4))));

    // Named Component specific data
    NcNode->Flags             = NodeList->Flags;
    NcNode->CacheCoherent     = NodeList->CacheCoherent;
    NcNode->AllocationHints   = NodeList->AllocationHints;
    NcNode->Reserved          = EFI_ACPI_RESERVED_WORD;
    NcNode->MemoryAccessFlags = NodeList->MemoryAccessFlags;
    NcNode->AddressSizeLimit  = NodeList->AddressSizeLimit;

    // Copy the object name
    ObjectName = (CHAR8 *)((UINT8 *)NcNode +
                           sizeof (EFI_ACPI_6_0_IO_REMAPPING_NAMED_COMP_NODE));
    Status = AsciiStrCpyS (
               ObjectName,
               ObjectNameLength,
               NodeList->ObjectName
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to copy Object Name. Status = %r\n",
        Status
        ));
      return Status;
    }

    if (NodeList->IdMappingCount > 0) {
      if (NodeList->IdMappingToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Id Mapping token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->IdMappingToken,
          Status
          ));
        return Status;
      }

      // Ids for Named Component
      IdMapArray = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)NcNode +
                                                          NcNode->Node.IdReference);

      Status = AddIdMappingArray (
                 This,
                 CfgMgrProtocol,
                 IdMapArray,
                 NodeList->IdMappingCount,
                 NodeList->IdMappingToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to add Id Mapping Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Next Named Component Node
    NcNode = (EFI_ACPI_6_0_IO_REMAPPING_NAMED_COMP_NODE *)((UINT8 *)NcNode +
                                                           NcNode->Node.Length);
    NodeList++;
  } // Named Component Node

  return EFI_SUCCESS;
}

/** Update the Root Complex Node Information.

    This function updates the Root Complex node information in the IORT table.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the Root Complex
                                     Nodes.
    @param [in]     NodeList         Pointer to an array of Root Complex Node
                                     Objects.
    @param [in]     NodeCount        Number of Root Complex Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddRootComplexNodes (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE                *Iort,
  IN      CONST UINT32                                         NodesStartOffset,
  IN      CONST CM_ARM_ROOT_COMPLEX_NODE                       *NodeList,
  IN            UINT32                                         NodeCount
  )
{
  EFI_STATUS                          Status;
  EFI_ACPI_6_0_IO_REMAPPING_RC_NODE   *RcNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE  *IdMapArray;
  UINT64                              NodeLength;

  ASSERT (Iort != NULL);

  RcNode = (EFI_ACPI_6_0_IO_REMAPPING_RC_NODE *)((UINT8 *)Iort +
                                                 NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetRootComplexNodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Root Complex Node length 0x%lx > MAX_UINT16."
        " Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    // Populate the node header
    RcNode->Node.Type          = EFI_ACPI_IORT_TYPE_ROOT_COMPLEX;
    RcNode->Node.Length        = (UINT16)NodeLength;
    RcNode->Node.NumIdMappings = NodeList->IdMappingCount;
    RcNode->Node.IdReference   = (NodeList->IdMappingCount == 0) ?
                                 0 : sizeof (EFI_ACPI_6_0_IO_REMAPPING_RC_NODE);

    if (AcpiTableInfo->AcpiTableRevision <
        EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
    {
      RcNode->Node.Revision     = 1;
      RcNode->Node.Identifier   = EFI_ACPI_RESERVED_DWORD;
      RcNode->PasidCapabilities = EFI_ACPI_RESERVED_WORD;
    } else {
      RcNode->Node.Revision     = 4;
      RcNode->Node.Identifier   = NodeList->Identifier;
      RcNode->PasidCapabilities = NodeList->PasidCapabilities;
      RcNode->Flags             = NodeList->Flags;
    }

    // Root Complex specific data
    RcNode->CacheCoherent     = NodeList->CacheCoherent;
    RcNode->AllocationHints   = NodeList->AllocationHints;
    RcNode->Reserved          = EFI_ACPI_RESERVED_WORD;
    RcNode->MemoryAccessFlags = NodeList->MemoryAccessFlags;
    RcNode->AtsAttribute      = NodeList->AtsAttribute;
    RcNode->PciSegmentNumber  = NodeList->PciSegmentNumber;
    RcNode->MemoryAddressSize = NodeList->MemoryAddressSize;
    RcNode->Reserved1[0]      = EFI_ACPI_RESERVED_BYTE;

    if (NodeList->IdMappingCount > 0) {
      if (NodeList->IdMappingToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Id Mapping token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->IdMappingToken,
          Status
          ));
        return Status;
      }

      // Ids for Root Complex
      IdMapArray = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)RcNode +
                                                          RcNode->Node.IdReference);
      Status = AddIdMappingArray (
                 This,
                 CfgMgrProtocol,
                 IdMapArray,
                 NodeList->IdMappingCount,
                 NodeList->IdMappingToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to add Id Mapping Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Next Root Complex Node
    RcNode = (EFI_ACPI_6_0_IO_REMAPPING_RC_NODE *)((UINT8 *)RcNode +
                                                   RcNode->Node.Length);
    NodeList++;
  } // Root Complex Node

  return EFI_SUCCESS;
}

/** Update the SMMU Interrupt Array.

    This function retrieves the InterruptArray object referenced by the
    InterruptToken and updates the SMMU InterruptArray.

    @param [in]      CfgMgrProtocol   Pointer to the Configuration Manager
                                      Protocol Interface.
    @param [in, out] InterruptArray   Pointer to an array of Interrupts.
    @param [in]      InterruptCount   Number of entries in the InterruptArray.
    @param [in]      InterruptToken   Reference Token for retrieving the SMMU
                                      InterruptArray object.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddSmmuInterruptArray (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT            *InterruptArray,
  IN            UINT32                                        InterruptCount,
  IN      CONST CM_OBJECT_TOKEN                               InterruptToken
  )
{
  EFI_STATUS             Status;
  CM_ARM_SMMU_INTERRUPT  *SmmuInterrupt;
  UINT32                 SmmuInterruptCount;

  ASSERT (InterruptArray != NULL);

  // Get the SMMU Interrupt Array
  Status = GetEArmObjSmmuInterruptArray (
             CfgMgrProtocol,
             InterruptToken,
             &SmmuInterrupt,
             &SmmuInterruptCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get SMMU Interrupt array. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SmmuInterruptCount < InterruptCount) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get the required number of SMMU Interrupts.\n"
      ));
    return EFI_NOT_FOUND;
  }

  // Populate the Id Mapping array
  while (InterruptCount-- != 0) {
    InterruptArray->Interrupt      = SmmuInterrupt->Interrupt;
    InterruptArray->InterruptFlags = SmmuInterrupt->Flags;
    InterruptArray++;
    SmmuInterrupt++;
  } // Id Mapping array

  return EFI_SUCCESS;
}

/** Update the SMMU v1/v2 Node Information.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the SMMU v1/v2
                                     Nodes.
    @param [in]     NodeList         Pointer to an array of SMMU v1/v2 Node
                                     Objects.
    @param [in]     NodeCount        Number of SMMU v1/v2 Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddSmmuV1V2Nodes (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE               *Iort,
  IN      CONST UINT32                                        NodesStartOffset,
  IN      CONST CM_ARM_SMMUV1_SMMUV2_NODE                     *NodeList,
  IN            UINT32                                        NodeCount
  )
{
  EFI_STATUS                           Status;
  EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE  *SmmuNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE   *IdMapArray;

  EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT  *ContextInterruptArray;
  EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT  *PmuInterruptArray;
  UINT64                              NodeLength;
  UINT32                              Offset;

  ASSERT (Iort != NULL);

  SmmuNode = (EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE *)((UINT8 *)Iort +
                                                     NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetSmmuV1V2NodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: SMMU V1/V2 Node length 0x%lx > MAX_UINT16. Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    // Populate the node header
    SmmuNode->Node.Type          = EFI_ACPI_IORT_TYPE_SMMUv1v2;
    SmmuNode->Node.Length        = (UINT16)NodeLength;
    SmmuNode->Node.NumIdMappings = NodeList->IdMappingCount;
    SmmuNode->Node.IdReference   = (NodeList->IdMappingCount == 0) ?
                                   0 : (sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE) +
                                        (NodeList->ContextInterruptCount *
                                         sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT)) +
                                        (NodeList->PmuInterruptCount *
                                         sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT)));

    if (AcpiTableInfo->AcpiTableRevision <
        EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
    {
      SmmuNode->Node.Revision   = 1;
      SmmuNode->Node.Identifier = EFI_ACPI_RESERVED_DWORD;
    } else {
      SmmuNode->Node.Revision   = 3;
      SmmuNode->Node.Identifier = NodeList->Identifier;
    }

    // SMMU v1/v2 specific data
    SmmuNode->Base  = NodeList->BaseAddress;
    SmmuNode->Span  = NodeList->Span;
    SmmuNode->Model = NodeList->Model;
    SmmuNode->Flags = NodeList->Flags;

    // Reference to Global Interrupt Array
    SmmuNode->GlobalInterruptArrayRef =
      OFFSET_OF (EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE, SMMU_NSgIrpt);

    Offset = sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE);
    // Context Interrupt
    SmmuNode->NumContextInterrupts = NodeList->ContextInterruptCount;
    if (NodeList->ContextInterruptCount != 0) {
      SmmuNode->ContextInterruptArrayRef = Offset;
      ContextInterruptArray              =
        (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT *)((UINT8 *)SmmuNode + Offset);
      Offset += (NodeList->ContextInterruptCount *
                 sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT));
    }

    // PMU Interrupt
    SmmuNode->NumPmuInterrupts = NodeList->PmuInterruptCount;
    if (NodeList->PmuInterruptCount != 0) {
      SmmuNode->PmuInterruptArrayRef = Offset;
      PmuInterruptArray              =
        (EFI_ACPI_6_0_IO_REMAPPING_SMMU_INT *)((UINT8 *)SmmuNode + Offset);
    }

    SmmuNode->SMMU_NSgIrpt         = NodeList->SMMU_NSgIrpt;
    SmmuNode->SMMU_NSgIrptFlags    = NodeList->SMMU_NSgIrptFlags;
    SmmuNode->SMMU_NSgCfgIrpt      = NodeList->SMMU_NSgCfgIrpt;
    SmmuNode->SMMU_NSgCfgIrptFlags = NodeList->SMMU_NSgCfgIrptFlags;

    if (NodeList->ContextInterruptCount != 0) {
      if (NodeList->ContextInterruptToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Context Interrupt token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->ContextInterruptToken,
          Status
          ));
        return Status;
      }

      // Add Context Interrupt Array
      Status = AddSmmuInterruptArray (
                 CfgMgrProtocol,
                 ContextInterruptArray,
                 SmmuNode->NumContextInterrupts,
                 NodeList->ContextInterruptToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to Context Interrupt Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Add PMU Interrupt Array
    if (SmmuNode->NumPmuInterrupts != 0) {
      if (NodeList->PmuInterruptToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid PMU Interrupt token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->PmuInterruptToken,
          Status
          ));
        return Status;
      }

      Status = AddSmmuInterruptArray (
                 CfgMgrProtocol,
                 PmuInterruptArray,
                 SmmuNode->NumPmuInterrupts,
                 NodeList->PmuInterruptToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to PMU Interrupt Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    if (NodeList->IdMappingCount > 0) {
      if (NodeList->IdMappingToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Id Mapping token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->IdMappingToken,
          Status
          ));
        return Status;
      }

      // Ids for SMMU v1/v2 Node
      IdMapArray = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)SmmuNode +
                                                          SmmuNode->Node.IdReference);
      Status = AddIdMappingArray (
                 This,
                 CfgMgrProtocol,
                 IdMapArray,
                 NodeList->IdMappingCount,
                 NodeList->IdMappingToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to add Id Mapping Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Next SMMU v1/v2 Node
    SmmuNode = (EFI_ACPI_6_0_IO_REMAPPING_SMMU_NODE *)((UINT8 *)SmmuNode +
                                                       SmmuNode->Node.Length);
    NodeList++;
  } // SMMU v1/v2 Node

  return EFI_SUCCESS;
}

/** Update the SMMUv3 Node Information.

    This function updates the SMMUv3 node information in the IORT table.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the SMMUv3 Nodes.
    @param [in]     NodeList         Pointer to an array of SMMUv3 Node Objects.
    @param [in]     NodeCount        Number of SMMUv3 Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddSmmuV3Nodes (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE               *Iort,
  IN      CONST UINT32                                        NodesStartOffset,
  IN      CONST CM_ARM_SMMUV3_NODE                            *NodeList,
  IN            UINT32                                        NodeCount
  )
{
  EFI_STATUS                            Status;
  EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE  *SmmuV3Node;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE    *IdMapArray;
  UINT64                                NodeLength;

  ASSERT (Iort != NULL);

  SmmuV3Node = (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE *)((UINT8 *)Iort +
                                                        NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetSmmuV3NodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: SMMU V3 Node length 0x%lx > MAX_UINT16. Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    // Populate the node header
    SmmuV3Node->Node.Type          = EFI_ACPI_IORT_TYPE_SMMUv3;
    SmmuV3Node->Node.Length        = (UINT16)NodeLength;
    SmmuV3Node->Node.NumIdMappings = NodeList->IdMappingCount;
    SmmuV3Node->Node.IdReference   = (NodeList->IdMappingCount == 0) ?
                                     0 : sizeof (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE);

    if (AcpiTableInfo->AcpiTableRevision <
        EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
    {
      SmmuV3Node->Node.Revision   = 2;
      SmmuV3Node->Node.Identifier = EFI_ACPI_RESERVED_DWORD;
    } else {
      SmmuV3Node->Node.Revision   = 4;
      SmmuV3Node->Node.Identifier = NodeList->Identifier;
    }

    // SMMUv3 specific data
    SmmuV3Node->Base         = NodeList->BaseAddress;
    SmmuV3Node->Flags        = NodeList->Flags;
    SmmuV3Node->Reserved     = EFI_ACPI_RESERVED_WORD;
    SmmuV3Node->VatosAddress = NodeList->VatosAddress;
    SmmuV3Node->Model        = NodeList->Model;
    SmmuV3Node->Event        = NodeList->EventInterrupt;
    SmmuV3Node->Pri          = NodeList->PriInterrupt;
    SmmuV3Node->Gerr         = NodeList->GerrInterrupt;
    SmmuV3Node->Sync         = NodeList->SyncInterrupt;

    if ((SmmuV3Node->Flags & EFI_ACPI_IORT_SMMUv3_FLAG_PROXIMITY_DOMAIN) != 0) {
      // The Proximity Domain Valid flag is set to 1
      SmmuV3Node->ProximityDomain = NodeList->ProximityDomain;
    } else {
      SmmuV3Node->ProximityDomain = 0;
    }

    if ((SmmuV3Node->Event != 0) && (SmmuV3Node->Pri != 0) &&
        (SmmuV3Node->Gerr != 0) && (SmmuV3Node->Sync != 0))
    {
      // If all the SMMU control interrupts are GSIV based,
      // the DeviceID mapping index field is ignored.
      SmmuV3Node->DeviceIdMappingIndex = 0;
    } else {
      SmmuV3Node->DeviceIdMappingIndex = NodeList->DeviceIdMappingIndex;
    }

    if (NodeList->IdMappingCount > 0) {
      if (NodeList->IdMappingToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Id Mapping token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->IdMappingToken,
          Status
          ));
        return Status;
      }

      // Ids for SMMUv3 node
      IdMapArray = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)SmmuV3Node +
                                                          SmmuV3Node->Node.IdReference);
      Status = AddIdMappingArray (
                 This,
                 CfgMgrProtocol,
                 IdMapArray,
                 NodeList->IdMappingCount,
                 NodeList->IdMappingToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to add Id Mapping Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Next SMMUv3 Node
    SmmuV3Node = (EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE *)((UINT8 *)SmmuV3Node +
                                                          SmmuV3Node->Node.Length);
    NodeList++;
  } // SMMUv3 Node

  return EFI_SUCCESS;
}

/** Update the PMCG Node Information.

    This function updates the PMCG node information in the IORT table.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the PMCG Nodes.
    @param [in]     NodeList         Pointer to an array of PMCG Node Objects.
    @param [in]     NodeCount        Number of PMCG Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddPmcgNodes (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE               *Iort,
  IN      CONST UINT32                                        NodesStartOffset,
  IN      CONST CM_ARM_PMCG_NODE                              *NodeList,
  IN            UINT32                                        NodeCount
  )
{
  EFI_STATUS                           Status;
  EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE  *PmcgNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE   *IdMapArray;
  ACPI_IORT_GENERATOR                  *Generator;
  UINT64                               NodeLength;

  ASSERT (Iort != NULL);

  Generator = (ACPI_IORT_GENERATOR *)This;
  PmcgNode  = (EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE *)((UINT8 *)Iort +
                                                      NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetPmcgNodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: PMCG Node length 0x%lx > MAX_UINT16. Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    // Populate the node header
    PmcgNode->Node.Type          = EFI_ACPI_IORT_TYPE_PMCG;
    PmcgNode->Node.Length        = (UINT16)NodeLength;
    PmcgNode->Node.NumIdMappings = NodeList->IdMappingCount;
    PmcgNode->Node.IdReference   = (NodeList->IdMappingCount == 0) ?
                                   0 : sizeof (EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE);

    if (AcpiTableInfo->AcpiTableRevision <
        EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
    {
      PmcgNode->Node.Revision   = 1;
      PmcgNode->Node.Identifier = EFI_ACPI_RESERVED_DWORD;
    } else {
      PmcgNode->Node.Revision   = 2;
      PmcgNode->Node.Identifier = NodeList->Identifier;
    }

    // PMCG specific data
    PmcgNode->Base                  = NodeList->BaseAddress;
    PmcgNode->OverflowInterruptGsiv = NodeList->OverflowInterrupt;
    PmcgNode->Page1Base             = NodeList->Page1BaseAddress;

    Status = GetNodeOffsetReferencedByToken (
               Generator->NodeIndexer,
               Generator->IortNodeCount,
               NodeList->ReferenceToken,
               &PmcgNode->NodeReference
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to get Output Reference for PMCG Node."
        "Reference Token = %p"
        " Status = %r\n",
        NodeList->ReferenceToken,
        Status
        ));
      return Status;
    }

    if (NodeList->IdMappingCount > 0) {
      if (NodeList->IdMappingToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Id Mapping token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->IdMappingToken,
          Status
          ));
        return Status;
      }

      // Ids for PMCG node
      IdMapArray = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)PmcgNode +
                                                          PmcgNode->Node.IdReference);

      Status = AddIdMappingArray (
                 This,
                 CfgMgrProtocol,
                 IdMapArray,
                 NodeList->IdMappingCount,
                 NodeList->IdMappingToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to add Id Mapping Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Next PMCG Node
    PmcgNode = (EFI_ACPI_6_0_IO_REMAPPING_PMCG_NODE *)((UINT8 *)PmcgNode +
                                                       PmcgNode->Node.Length);
    NodeList++;
  } // PMCG Node

  return EFI_SUCCESS;
}

/** Update the Memory Range Descriptor Array.

    This function retrieves the Memory Range Descriptor objects referenced by
    MemRangeDescToken and updates the Memory Range Descriptor array.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     DescArray        Pointer to an array of Memory Range
                                     Descriptors.
    @param [in]     DescCount        Number of Id Descriptors.
    @param [in]     DescToken        Reference Token for retrieving the
                                     Memory Range Descriptor Array.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddMemRangeDescArray (
  IN  CONST ACPI_TABLE_GENERATOR                      *CONST  This,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST  CfgMgrProtocol,
  IN        EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC          *DescArray,
  IN        UINT32                                            DescCount,
  IN  CONST CM_OBJECT_TOKEN                                   DescToken
  )
{
  EFI_STATUS                      Status;
  CM_ARM_MEMORY_RANGE_DESCRIPTOR  *MemRangeDesc;
  UINT32                          MemRangeDescCount;

  ASSERT (DescArray != NULL);

  // Get the Id Mapping Array
  Status = GetEArmObjMemoryRangeDescriptor (
             CfgMgrProtocol,
             DescToken,
             &MemRangeDesc,
             &MemRangeDescCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get Memory Range Descriptor array. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (MemRangeDescCount < DescCount) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get the required number of Memory"
      " Range Descriptors.\n"
      ));
    return EFI_NOT_FOUND;
  }

  // Populate the Memory Range Descriptor array
  while (DescCount-- != 0) {
    DescArray->Base     = MemRangeDesc->BaseAddress;
    DescArray->Length   = MemRangeDesc->Length;
    DescArray->Reserved = EFI_ACPI_RESERVED_DWORD;

    DescArray++;
    MemRangeDesc++;
  }

  return EFI_SUCCESS;
}

/** Update the RMR Node Information.

    This function updates the RMR node information in the IORT table.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Iort             Pointer to IORT table structure.
    @param [in]     NodesStartOffset Offset for the start of the PMCG Nodes.
    @param [in]     NodeList         Pointer to an array of PMCG Node Objects.
    @param [in]     NodeCount        Number of PMCG Node Objects.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddRmrNodes (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EFI_ACPI_6_0_IO_REMAPPING_TABLE               *Iort,
  IN      CONST UINT32                                        NodesStartOffset,
  IN      CONST CM_ARM_RMR_NODE                               *NodeList,
  IN            UINT32                                        NodeCount
  )
{
  EFI_STATUS                                Status;
  EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE        *RmrNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE        *IdMapArray;
  EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC  *MemRangeDescArray;
  UINT64                                    NodeLength;

  ASSERT (Iort != NULL);

  RmrNode = (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE *)((UINT8 *)Iort +
                                                   NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetRmrNodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: RMR Node length 0x%lx > MAX_UINT16. Status = %r\n",
        NodeLength,
        Status
        ));
      return Status;
    }

    if (NodeList->MemRangeDescCount == 0) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Memory Range Desc count = %d. Status = %r\n",
        NodeList->MemRangeDescCount,
        Status
        ));
      return Status;
    }

    if (NodeList->MemRangeDescToken == CM_NULL_TOKEN) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Memory Range Descriptor token,"
        " Token = 0x%x. Status = %r\n",
        NodeList->MemRangeDescToken,
        Status
        ));
      return Status;
    }

    // Populate the node header
    RmrNode->Node.Type          = EFI_ACPI_IORT_TYPE_RMR;
    RmrNode->Node.Length        = (UINT16)NodeLength;
    RmrNode->Node.Revision      = 3;
    RmrNode->Node.Identifier    = NodeList->Identifier;
    RmrNode->Node.NumIdMappings = NodeList->IdMappingCount;
    RmrNode->Node.IdReference   = (NodeList->IdMappingCount == 0) ?
                                  0 : sizeof (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE);

    // RMR specific data
    RmrNode->Flags           = NodeList->Flags;
    RmrNode->NumMemRangeDesc = NodeList->MemRangeDescCount;
    RmrNode->MemRangeDescRef = (NodeList->MemRangeDescCount == 0) ?
                               0 : (sizeof (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE) +
                                    (NodeList->IdMappingCount *
                                     sizeof (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE)));

    if (NodeList->IdMappingCount > 0) {
      if (NodeList->IdMappingToken == CM_NULL_TOKEN) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Invalid Id Mapping token,"
          " Token = 0x%x, Status =%r\n",
          NodeList->IdMappingToken,
          Status
          ));
        return Status;
      }

      // Ids for RMR node
      IdMapArray = (EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE *)((UINT8 *)RmrNode +
                                                          RmrNode->Node.IdReference);

      Status = AddIdMappingArray (
                 This,
                 CfgMgrProtocol,
                 IdMapArray,
                 NodeList->IdMappingCount,
                 NodeList->IdMappingToken
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: Failed to add Id Mapping Array. Status = %r\n",
          Status
          ));
        return Status;
      }
    }

    // Memory Range Descriptors for RMR node
    MemRangeDescArray = (EFI_ACPI_6_0_IO_REMAPPING_MEM_RANGE_DESC *)(
                                                                     (UINT8 *)RmrNode +
                                                                     RmrNode->MemRangeDescRef
                                                                     );

    Status = AddMemRangeDescArray (
               This,
               CfgMgrProtocol,
               MemRangeDescArray,
               NodeList->MemRangeDescCount,
               NodeList->MemRangeDescToken
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to Memory Range Descriptor Array. Status = %r\n",
        Status
        ));
      return Status;
    }

    // Next RMR Node
    RmrNode = (EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE *)((UINT8 *)RmrNode +
                                                     RmrNode->Node.Length);
    NodeList++;
  } // RMR Node

  return EFI_SUCCESS;
}

/** Validates that the IORT nodes Identifier are unique.

    @param [in]     NodeIndexer      Pointer to the Node Indexer.
    @param [in]     NodeCount        Number of IORT Nodes.

    @retval EFI_SUCCESS             Success.
    @retval EFI_INVALID_PARAMETER   Identifier field not unique.
**/
STATIC
EFI_STATUS
ValidateNodeIdentifiers (
  IN      CONST IORT_NODE_INDEXER                  *CONST  NodeIndexer,
  IN            UINT32                                     NodeCount
  )
{
  UINT32  IndexI;
  UINT32  IndexJ;

  for (IndexI = 0; IndexI < NodeCount; IndexI++) {
    for (IndexJ = 0; IndexJ < NodeCount; IndexJ++) {
      if ((IndexI != IndexJ) &&
          (NodeIndexer[IndexI].Identifier == NodeIndexer[IndexJ].Identifier))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: IORT: UID %d of Token %p matches with that of Token %p.\n",
          NodeIndexer[IndexI].Identifier,
          NodeIndexer[IndexI].Token,
          NodeIndexer[IndexJ].Token
          ));
        return EFI_INVALID_PARAMETER;
      }
    }// IndexJ
  } // IndexI

  return EFI_SUCCESS;
}

/** Construct the IORT ACPI table.

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
BuildIortTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS  Status;

  UINT64  TableSize;
  UINT64  NodeSize;

  UINT32  IortNodeCount;
  UINT32  ItsGroupNodeCount;
  UINT32  NamedComponentNodeCount;
  UINT32  RootComplexNodeCount;
  UINT32  SmmuV1V2NodeCount;
  UINT32  SmmuV3NodeCount;
  UINT32  PmcgNodeCount;
  UINT32  RmrNodeCount;

  UINT32  ItsGroupOffset;
  UINT32  NamedComponentOffset;
  UINT32  RootComplexOffset;
  UINT32  SmmuV1V2Offset;
  UINT32  SmmuV3Offset;
  UINT32  PmcgOffset;
  UINT32  RmrOffset;

  CM_ARM_ITS_GROUP_NODE        *ItsGroupNodeList;
  CM_ARM_NAMED_COMPONENT_NODE  *NamedComponentNodeList;
  CM_ARM_ROOT_COMPLEX_NODE     *RootComplexNodeList;
  CM_ARM_SMMUV1_SMMUV2_NODE    *SmmuV1V2NodeList;
  CM_ARM_SMMUV3_NODE           *SmmuV3NodeList;
  CM_ARM_PMCG_NODE             *PmcgNodeList;
  CM_ARM_RMR_NODE              *RmrNodeList;

  EFI_ACPI_6_0_IO_REMAPPING_TABLE  *Iort;
  IORT_NODE_INDEXER                *NodeIndexer;
  ACPI_IORT_GENERATOR              *Generator;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  RmrNodeCount = 0;

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((AcpiTableInfo->AcpiTableRevision > EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00) &&
      (AcpiTableInfo->AcpiTableRevision < EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Revisions E (1), E.a(2),b(3),c(4) are not supported.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Generator = (ACPI_IORT_GENERATOR *)This;
  *Table    = NULL;

  // Get the ITS group node info
  Status = GetEArmObjItsGroup (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ItsGroupNodeList,
             &ItsGroupNodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get ITS Group Node Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add the ITS group node count
  IortNodeCount = ItsGroupNodeCount;

  // Get the Named component node info
  Status = GetEArmObjNamedComponent (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &NamedComponentNodeList,
             &NamedComponentNodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get Named Component Node Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add the Named Component group count
  IortNodeCount += NamedComponentNodeCount;

  // Get the Root complex node info
  Status = GetEArmObjRootComplex (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &RootComplexNodeList,
             &RootComplexNodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get Root Complex Node Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add the Root Complex node count
  IortNodeCount += RootComplexNodeCount;

  // Get the SMMU v1/v2 node info
  Status = GetEArmObjSmmuV1SmmuV2 (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SmmuV1V2NodeList,
             &SmmuV1V2NodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get SMMUv1/SMMUv2 Node Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add the SMMU v1/v2 node count
  IortNodeCount += SmmuV1V2NodeCount;

  // Get the SMMUv3 node info
  Status = GetEArmObjSmmuV3 (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SmmuV3NodeList,
             &SmmuV3NodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get SMMUv3 Node Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add the SMMUv3 node count
  IortNodeCount += SmmuV3NodeCount;

  // Get the PMCG node info
  Status = GetEArmObjPmcg (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PmcgNodeList,
             &PmcgNodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to get PMCG Node Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Add the PMCG node count
  IortNodeCount += PmcgNodeCount;

  if (AcpiTableInfo->AcpiTableRevision >=
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
  {
    // Get the RMR node info
    Status = GetEArmObjRmr (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &RmrNodeList,
               &RmrNodeCount
               );
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to get RMR Node Info. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    // Add the RMR node count
    IortNodeCount += RmrNodeCount;
  }

  // Allocate Node Indexer array
  NodeIndexer = (IORT_NODE_INDEXER *)AllocateZeroPool (
                                       (sizeof (IORT_NODE_INDEXER) *
                                        IortNodeCount)
                                       );
  if (NodeIndexer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to allocate memory for Node Indexer" \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((DEBUG_INFO, "INFO: NodeIndexer = %p\n", NodeIndexer));
  Generator->IortNodeCount = IortNodeCount;
  Generator->NodeIndexer   = NodeIndexer;

  // Calculate the size of the IORT table
  TableSize = sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE);

  // ITS Group Nodes
  if (ItsGroupNodeCount > 0) {
    ItsGroupOffset = (UINT32)TableSize;
    // Size of ITS Group node list.
    NodeSize = GetSizeofItsGroupNodes (
                 ItsGroupOffset,
                 ItsGroupNodeList,
                 ItsGroupNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of Group Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " ItsGroupNodeCount = %d\n" \
      " ItsGroupOffset = %d\n",
      ItsGroupNodeCount,
      ItsGroupOffset
      ));
  }

  // Named Component Nodes
  if (NamedComponentNodeCount > 0) {
    NamedComponentOffset = (UINT32)TableSize;
    // Size of Named Component node list.
    NodeSize = GetSizeofNamedComponentNodes (
                 NamedComponentOffset,
                 NamedComponentNodeList,
                 NamedComponentNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of Named Component Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " NamedComponentNodeCount = %d\n" \
      " NamedComponentOffset = %d\n",
      NamedComponentNodeCount,
      NamedComponentOffset
      ));
  }

  // Root Complex Nodes
  if (RootComplexNodeCount > 0) {
    RootComplexOffset = (UINT32)TableSize;
    // Size of Root Complex node list.
    NodeSize = GetSizeofRootComplexNodes (
                 RootComplexOffset,
                 RootComplexNodeList,
                 RootComplexNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of Root Complex Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " RootComplexNodeCount = %d\n" \
      " RootComplexOffset = %d\n",
      RootComplexNodeCount,
      RootComplexOffset
      ));
  }

  // SMMUv1/SMMUv2 Nodes
  if (SmmuV1V2NodeCount > 0) {
    SmmuV1V2Offset = (UINT32)TableSize;
    // Size of SMMUv1/SMMUv2 node list.
    NodeSize = GetSizeofSmmuV1V2Nodes (
                 SmmuV1V2Offset,
                 SmmuV1V2NodeList,
                 SmmuV1V2NodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of SMMUv1/v2 Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " SmmuV1V2NodeCount = %d\n" \
      " SmmuV1V2Offset = %d\n",
      SmmuV1V2NodeCount,
      SmmuV1V2Offset
      ));
  }

  // SMMUv3 Nodes
  if (SmmuV3NodeCount > 0) {
    SmmuV3Offset = (UINT32)TableSize;
    // Size of SMMUv3 node list.
    NodeSize = GetSizeofSmmuV3Nodes (
                 SmmuV3Offset,
                 SmmuV3NodeList,
                 SmmuV3NodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of SMMUv3 Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " SmmuV3NodeCount = %d\n" \
      " SmmuV3Offset = %d\n",
      SmmuV3NodeCount,
      SmmuV3Offset
      ));
  }

  // PMCG Nodes
  if (PmcgNodeCount > 0) {
    PmcgOffset = (UINT32)TableSize;
    // Size of PMCG node list.
    NodeSize = GetSizeofPmcgNodes (
                 PmcgOffset,
                 PmcgNodeList,
                 PmcgNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of PMCG Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " PmcgNodeCount = %d\n" \
      " PmcgOffset = %d\n",
      PmcgNodeCount,
      PmcgOffset
      ));
  }

  // RMR Nodes
  if ((AcpiTableInfo->AcpiTableRevision >=
       EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05) &&
      (RmrNodeCount > 0))
  {
    RmrOffset = (UINT32)TableSize;
    // Size of RMR node list.
    NodeSize = GetSizeofRmrNodes (
                 RmrOffset,
                 RmrNodeList,
                 RmrNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Invalid Size of RMR Nodes. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG ((
      DEBUG_INFO,
      " RmrNodeCount = %d\n" \
      " RmrOffset = %d\n",
      RmrNodeCount,
      RmrOffset
      ));
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: IORT:\n" \
    " IortNodeCount = %d\n" \
    " TableSize = 0x%lx\n",
    IortNodeCount,
    TableSize
    ));

  if (TableSize > MAX_UINT32) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: IORT Table Size 0x%lx > MAX_UINT32," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  // Validate that the identifiers for the nodes are unique
  if (AcpiTableInfo->AcpiTableRevision >=
      EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05)
  {
    Status = ValidateNodeIdentifiers (Generator->NodeIndexer, IortNodeCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Node Identifier not unique. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  // Allocate the Buffer for IORT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to allocate memory for IORT Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  Iort = (EFI_ACPI_6_0_IO_REMAPPING_TABLE *)*Table;

  DEBUG ((
    DEBUG_INFO,
    "IORT: Iort = 0x%p TableSize = 0x%lx\n",
    Iort,
    TableSize
    ));

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Iort->Header,
             AcpiTableInfo,
             (UINT32)TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: IORT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update IORT table
  Iort->NumNodes   = IortNodeCount;
  Iort->NodeOffset = sizeof (EFI_ACPI_6_0_IO_REMAPPING_TABLE);
  Iort->Reserved   = EFI_ACPI_RESERVED_DWORD;

  if (ItsGroupNodeCount > 0) {
    Status = AddItsGroupNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               ItsGroupOffset,
               ItsGroupNodeList,
               ItsGroupNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add ITS Group Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if (NamedComponentNodeCount > 0) {
    Status = AddNamedComponentNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               NamedComponentOffset,
               NamedComponentNodeList,
               NamedComponentNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add Named Component Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if (RootComplexNodeCount > 0) {
    Status = AddRootComplexNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               RootComplexOffset,
               RootComplexNodeList,
               RootComplexNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add Root Complex Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if (SmmuV1V2NodeCount > 0) {
    Status = AddSmmuV1V2Nodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               SmmuV1V2Offset,
               SmmuV1V2NodeList,
               SmmuV1V2NodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add SMMU v1/v2 Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if (SmmuV3NodeCount > 0) {
    Status = AddSmmuV3Nodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               SmmuV3Offset,
               SmmuV3NodeList,
               SmmuV3NodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add SMMUv3 Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if (PmcgNodeCount > 0) {
    Status = AddPmcgNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               PmcgOffset,
               PmcgNodeList,
               PmcgNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add PMCG Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if ((AcpiTableInfo->AcpiTableRevision >=
       EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05) &&
      (RmrNodeCount > 0))
  {
    Status = AddRmrNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Iort,
               RmrOffset,
               RmrNodeList,
               RmrNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: IORT: Failed to add RMR Node. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  return EFI_SUCCESS;

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

/** Free any resources allocated for constructing the IORT

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
FreeIortTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ACPI_IORT_GENERATOR  *Generator;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  Generator = (ACPI_IORT_GENERATOR *)This;

  // Free any memory allocated by the generator
  if (Generator->NodeIndexer != NULL) {
    FreePool (Generator->NodeIndexer);
    Generator->NodeIndexer = NULL;
  }

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: IORT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** The IORT Table Generator revision.
*/
#define IORT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the MADT Table Generator.
*/
STATIC
ACPI_IORT_GENERATOR  IortGenerator = {
  // ACPI table generator header
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdIort),
    // Generator Description
    L"ACPI.STD.IORT.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_4_IO_REMAPPING_TABLE_SIGNATURE,
    // ACPI Table Revision supported by this Generator
    EFI_ACPI_IO_REMAPPING_TABLE_REVISION_05,
    // Minimum supported ACPI Table Revision
    EFI_ACPI_IO_REMAPPING_TABLE_REVISION_00,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID_ARM,
    // Creator Revision
    IORT_GENERATOR_REVISION,
    // Build Table function
    BuildIortTable,
    // Free Resource function
    FreeIortTableResources,
    // Extended build function not needed
    NULL,
    // Extended build function not implemented by the generator.
    // Hence extended free resource function is not required.
    NULL
  },

  // IORT Generator private data

  // Iort Node count
  0,
  // Pointer to Iort node indexer
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
AcpiIortLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&IortGenerator.Header);
  DEBUG ((DEBUG_INFO, "IORT: Register Generator. Status = %r\n", Status));
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
AcpiIortLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&IortGenerator.Header);
  DEBUG ((DEBUG_INFO, "Iort: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
