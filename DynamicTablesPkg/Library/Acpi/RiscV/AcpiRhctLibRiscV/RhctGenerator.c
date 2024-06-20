/** @file
  RHCT Table Generator

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - TBD

**/

#include <IndustryStandard/IoRemappingTable.h>
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
#include <RiscVAcpi.h>

#include "RhctGenerator.h"

/** RISC-V standard RHCT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - ERiscVObjCmoInfo,
  - ERiscVObjTimerInfo,
  - ERiscVObjRintcInfo,
  - ERiscVObjIsaStringInfo
*/

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjCmoInfo,
  CM_RISCV_CMO_NODE
  );

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjTimerInfo,
  CM_RISCV_TIMER_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjRintcInfo,
  CM_RISCV_RINTC_INFO
  );

/** This macro expands to a function that retrieves the
    Named Component node information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjIsaStringInfo,
  CM_RISCV_ISA_STRING_NODE
  );

/** This macro expands to a function that retrieves the
     Root Complex node information from the Configuration Manager.
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjHartInfo,
  CM_RISCV_HART_INFO_NODE
  );

*/
/** Returns the size of the CMO node.

    @param [in]  Node    Pointer to CMO node.

    @retval Size of the ITS Group Node.
**/
STATIC
UINT32
GetCmoNodeSize (
  IN  CONST CM_RISCV_CMO_NODE  *Node
  )
{
  ASSERT (Node != NULL);

  return (UINT32)(sizeof (EFI_ACPI_6_6_RISCV_RHCT_CMO_NODE));
}

/** Returns the total size required for the CMO nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      RHCT where this node group starts.
    @param [in]       NodeList        Pointer to CMO node list.
    @param [in]       NodeCount       Count of the CMO nodes.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the ITS Group Nodes.
**/
STATIC
UINT64
GetSizeofCmoNodes (
  IN      CONST UINT32                         NodeStartOffset,
  IN      CONST CM_RISCV_CMO_NODE              *NodeList,
  IN            UINT32                         NodeCount,
  IN OUT        RHCT_NODE_INDEXER     **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Object = (VOID *)NodeList;
    (*NodeIndexer)->Offset = (UINT32)(Size + NodeStartOffset);
    DEBUG (
      (
       DEBUG_INFO,
       "RHCT: Node Indexer = %p, Object = %p,"
       " Offset = 0x%x\n",
       *NodeIndexer,
       (*NodeIndexer)->Object,
       (*NodeIndexer)->Offset
      )
      );

    Size += sizeof (EFI_ACPI_6_6_RISCV_RHCT_CMO_NODE);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Returns the size of the Named Component node.

    @param [in]  IsaString    NULL terminated ASCII string.

    @retval Size of the Named Component node.
**/
STATIC
UINT32
GetIsaStringNodeSize (
  IN  CONST CHAR8  *IsaString
  )
{
  ASSERT (IsaString != NULL);

  return (UINT32)(sizeof (EFI_ACPI_6_6_RISCV_RHCT_ISA_NODE) +
                  ALIGN_VALUE (AsciiStrSize ((CHAR8 *)IsaString), 2));
}

/** Returns the size of the Named Component node.

    @param [in]  NumOffsets    Number of offsets in Hart Info node.

    @retval Size of the Named Component node.
**/
STATIC
UINT32
GetHartInfoSize (
  IN  UINT32  NumOffsets
  )
{
  return sizeof (EFI_ACPI_6_6_RISCV_RHCT_HART_INFO_NODE) +
         (sizeof (UINT32) * NumOffsets);
}

/** Returns the total size required for the Root Complex nodes and
    updates the Node Indexer.

    This function calculates the size required for the node group
    and also populates the Node Indexer array with offsets for the
    individual nodes.

    @param [in]       NodeStartOffset Offset from the start of the
                                      RHCT where this node group starts.
    @param [in]       NodeList        Pointer to Root Complex node list.
    @param [in]       NodeCount       Count of the Root Complex nodes.
    @param [in]       NumOffsets      Number of offsets in Hart Info node.
    @param [in, out]  NodeIndexer     Pointer to the next Node Indexer.

    @retval Total size of the Root Complex nodes.
**/
STATIC
UINT64
GetSizeofHartInfoNodes (
  IN      CONST UINT32                              NodeStartOffset,
  IN      CONST CM_RISCV_RINTC_INFO                 *NodeList,
  IN            UINT32                              NodeCount,
  IN            UINT32                              NumOffsets,
  IN OUT        RHCT_NODE_INDEXER          **CONST  NodeIndexer
  )
{
  UINT64  Size;

  ASSERT (NodeList != NULL);

  Size = 0;
  while (NodeCount-- != 0) {
    (*NodeIndexer)->Object = (VOID *)NodeList;
    (*NodeIndexer)->Offset = (UINT32)(Size + NodeStartOffset);
    DEBUG (
      (
       DEBUG_INFO,
       "RHCT: Node Indexer = %p, Object = %p,"
       " Offset = 0x%x\n",
       *NodeIndexer,
       (*NodeIndexer)->Object,
       (*NodeIndexer)->Offset
      )
      );

    Size += GetHartInfoSize (NumOffsets);
    (*NodeIndexer)++;
    NodeList++;
  }

  return Size;
}

/** Update the CMO Node Information.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Rhct             Pointer to RHCT table structure.
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
AddCmoNodes (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE    *Rhct,
  IN  CONST UINT32                                        NodesStartOffset,
  IN  CONST CM_RISCV_CMO_NODE                             *NodeList,
  IN        UINT32                                        NodeCount
  )
{
  EFI_ACPI_6_6_RISCV_RHCT_CMO_NODE  *CmoNode;
  EFI_STATUS                        Status;
  UINT64                            NodeLength;

  ASSERT (Rhct != NULL);

  CmoNode = (EFI_ACPI_6_6_RISCV_RHCT_CMO_NODE *)((UINT8 *)Rhct + NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetCmoNodeSize (NodeList);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: ITS Id Array Node length 0x%lx > MAX_UINT16."
         " Status = %r\n",
         NodeLength,
         Status
        )
        );
      return Status;
    }

    // Populate the node header
    CmoNode->Node.Type     = EFI_ACPI_RHCT_TYPE_CMO_NODE;
    CmoNode->Node.Length   = (UINT16)NodeLength;
    CmoNode->Node.Revision = 1;

    // RHCT specific data
    CmoNode->CbomBlockSize = NodeList->CbomBlockSize;
    CmoNode->CbopBlockSize = NodeList->CbopBlockSize;
    CmoNode->CbozBlockSize = NodeList->CbozBlockSize;
    NodeList++;
  } // RHCT Group Node

  return EFI_SUCCESS;
}

/** Update the ISA Node Information.

    @param [in]     This              Pointer to the table Generator.
    @param [in]     CfgMgrProtocol    Pointer to the Configuration Manager
                                      Protocol Interface.
    @param [in]     AcpiTableInfo     Pointer to the ACPI table info structure.
    @param [in]     Rhct              Pointer to RHCT table structure.
    @param [in]     NodesStartOffset  Node start offset.
    @param [in]     IsaString         RISC-V ISA string.

    @retval EFI_SUCCESS           Table generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
    @retval EFI_NOT_FOUND         The required object was not found.
**/
STATIC
EFI_STATUS
AddIsaStringNode (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE    *Rhct,
  IN  CONST UINT32                                        NodesStartOffset,
  IN  CONST CHAR8                                         *IsaString
  )
{
  EFI_ACPI_6_6_RISCV_RHCT_ISA_NODE  *IsaStringNode;
  EFI_STATUS                        Status;
  UINT64                            NodeLength;
  UINT16                            IsaLength;
  UINT16                            *TgtIsaLength;
  CHAR8                             *TgtIsaString;

  ASSERT (Rhct != NULL);

  IsaStringNode = (EFI_ACPI_6_6_RISCV_RHCT_ISA_NODE *)((UINT8 *)Rhct + NodesStartOffset);

  NodeLength = GetIsaStringNodeSize (IsaString);
  if (NodeLength > MAX_UINT16) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: ISA Node length 0x%lx > MAX_UINT16."
       " Status = %r\n",
       NodeLength,
       Status
      )
      );
    return Status;
  }

  // Populate the node header
  IsaStringNode->Node.Type     = EFI_ACPI_RHCT_TYPE_ISA_NODE;
  IsaStringNode->Node.Length   = (UINT16)NodeLength;
  IsaStringNode->Node.Revision = 1;

  TgtIsaString  = (CHAR8 *)IsaStringNode + sizeof (EFI_ACPI_6_6_RISCV_RHCT_ISA_NODE);
  TgtIsaLength  = (UINT16 *)((CHAR8 *)IsaStringNode + sizeof (EFI_ACPI_6_6_RISCV_RHCT_NODE));
  IsaLength     = AsciiStrSize (IsaString) + 1;
  *TgtIsaLength = IsaLength;
  Status        = AsciiStrCpyS (
                    TgtIsaString,
                    IsaLength,
                    IsaString
                    );
  DEBUG ((DEBUG_INFO, "%a: TgtIsaString = %a, IsaLength = %d\n", __func__, TgtIsaString, IsaLength));
  return EFI_SUCCESS;
}

/** Update the Hart Info Node Information.

    @param [in]     This             Pointer to the table Generator.
    @param [in]     CfgMgrProtocol   Pointer to the Configuration Manager
                                     Protocol Interface.
    @param [in]     AcpiTableInfo    Pointer to the ACPI table info structure.
    @param [in]     Rhct             Pointer to RHCT table structure.
    @param [in]     NodesStartOffset Offset for the start of the ITS Group
                                     Nodes.
    @param [in]     NumOffsets       Number of offsets.
    @param [in]     Offsets          Array of offsets in Hart Info node.
    @param [in]     NodeList         Pointer to an array of ITS Group Node
                                     Objects.
    @param [in]     NodeCount        Number of ITS Group Node Objects.

    @retval EFI_SUCCESS           Added Hart Info node successfully.
    @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
AddHartInfoNodes (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE    *Rhct,
  IN  CONST UINT32                                        NodesStartOffset,
  IN        UINT32                                        NumOffsets,
  IN        UINT32                                        *Offsets,
  IN  CONST CM_RISCV_RINTC_INFO                           *NodeList,
  IN        UINT32                                        NodeCount
  )
{
  EFI_ACPI_6_6_RISCV_RHCT_HART_INFO_NODE  *HartInfoNode;
  EFI_STATUS                              Status;
  UINT64                                  NodeLength;
  UINTN                                   Idx;

  ASSERT (Rhct != NULL);

  HartInfoNode = (EFI_ACPI_6_6_RISCV_RHCT_HART_INFO_NODE *)((UINT8 *)Rhct + NodesStartOffset);

  while (NodeCount-- != 0) {
    NodeLength = GetHartInfoSize (NumOffsets);
    if (NodeLength > MAX_UINT16) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: HartInfo Node length 0x%lx > MAX_UINT16."
         " Status = %r\n",
         NodeLength,
         Status
        )
        );
      return Status;
    }

    DEBUG ((DEBUG_INFO, "%a: HartInfoNode = 0x%llx, NodeList->AcpiProcessorUid=0x%x, NodeLength=%d\n", __func__, HartInfoNode, NodeList->AcpiProcessorUid, NodeLength));
    // Populate the node header
    HartInfoNode->Node.Type     = EFI_ACPI_RHCT_TYPE_HART_INFO_NODE;
    HartInfoNode->Node.Length   = (UINT16)NodeLength;
    HartInfoNode->Node.Revision = 1;

    // RHCT specific data
    HartInfoNode->NumOffsets = NumOffsets;
    HartInfoNode->ACPICpuUid = NodeList->AcpiProcessorUid;
    for (Idx = 0; Idx < NumOffsets; Idx++) {
      HartInfoNode->Offsets[Idx] = Offsets[Idx];
    }

    HartInfoNode = (EFI_ACPI_6_6_RISCV_RHCT_HART_INFO_NODE *)((CHAR8 *)HartInfoNode + NodeLength);
    NodeList++;
  } // RHCT Group Node

  return EFI_SUCCESS;
}

/** Construct the RHCT ACPI table.

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
BuildRhctTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE  *Rhct;
  CM_RISCV_ISA_STRING_NODE                    *IsaStringNode;
  CM_RISCV_TIMER_INFO                         *TimerInfo;
  CM_RISCV_RINTC_INFO                         *RintcInfoNodeList;
  ACPI_RHCT_GENERATOR                         *Generator;
  RHCT_NODE_INDEXER                           *NodeIndexer;
  CM_RISCV_CMO_NODE                           *CmoNodeList;
  EFI_STATUS                                  Status;
  UINT64                                      TableSize, NodeSize;
  UINT32                                      RhctNodeCount;
  UINT32                                      IsaStringNodeCount;
  UINT32                                      IsaStringOffset;
  UINT32                                      CmoNodeCount, CmoOffset;
  UINT32                                      HartInfoNodeCount, HartInfoOffset;
  UINT32                                      TimerInfoCount;
  UINT32                                      *Offsets;
  UINTN                                       Idx;

  // CHAR8  *IsaString;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Requested table revision = %d, is not supported."
       "Supported table revision: Minimum = %d, Maximum = %d\n",
       AcpiTableInfo->AcpiTableRevision,
       This->MinAcpiTableRevision,
       This->AcpiTableRevision
      )
      );
    return EFI_INVALID_PARAMETER;
  }

  Generator = (ACPI_RHCT_GENERATOR *)This;
  *Table    = NULL;

  Status = GetERiscVObjTimerInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &TimerInfo,
             &TimerInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to get Timer Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  // Get the Isa String node info
  Status = GetERiscVObjIsaStringInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &IsaStringNode,
             &IsaStringNodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to get ISA string Node Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  // Add the ISA string node count
  RhctNodeCount = IsaStringNodeCount;

  // Get the CMO node info
  Status = GetERiscVObjCmoInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CmoNodeList,
             &CmoNodeCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to get CMO Node Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  // Add the CMO node count
  RhctNodeCount += CmoNodeCount;

  // Get the hart info node info
  Status = GetERiscVObjRintcInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &RintcInfoNodeList,
             &HartInfoNodeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to get Hart Info Node Info. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  // Add the Root Complex node count
  RhctNodeCount += HartInfoNodeCount;

  // Allocate Node Indexer array
  NodeIndexer = (RHCT_NODE_INDEXER *)AllocateZeroPool (
                                       (sizeof (RHCT_NODE_INDEXER) *
                                        RhctNodeCount)
                                       );
  if (NodeIndexer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to allocate memory for Node Indexer" \
       " Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  DEBUG ((DEBUG_INFO, "INFO: NodeIndexer = %p\n", NodeIndexer));
  Generator->RhctNodeCount = RhctNodeCount;
  Generator->NodeIndexer   = NodeIndexer;

  // Calculate the size of the RHCT table
  TableSize = sizeof (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE);

  if (IsaStringNodeCount > 0) {
    IsaStringOffset = (UINT32)TableSize;
    NodeSize        = GetIsaStringNodeSize (
                        IsaStringNode->IsaString
                        );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: Invalid Size of ISA string. Status = %r\n",
         Status
        )
        );
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG (
      (
       DEBUG_INFO,
       " IsaStringNodeCount = %d\n" \
       " IsaStringNodeSize = %d\n" \
       " IsaStringOffset = %d\n",
       IsaStringNodeCount,
       NodeSize,
       IsaStringOffset
      )
      );
  }

  // Named Component Nodes
  if (CmoNodeCount > 0) {
    CmoOffset = (UINT32)TableSize;
    // Size of Named Component node list.
    NodeSize = GetSizeofCmoNodes (
                 CmoOffset,
                 CmoNodeList,
                 CmoNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: Invalid Size of CMO Nodes. Status = %r\n",
         Status
        )
        );
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG (
      (
       DEBUG_INFO,
       " CmoNodeCount = %d\n" \
       " CmoOffset = %d\n",
       CmoNodeCount,
       CmoOffset
      )
      );
  }

  // Hart Info Nodes
  if (HartInfoNodeCount > 0) {
    HartInfoOffset = (UINT32)TableSize;
    // Size of Root Complex node list.
    NodeSize = GetSizeofHartInfoNodes (
                 HartInfoOffset,
                 RintcInfoNodeList,
                 HartInfoNodeCount,
                 RhctNodeCount - HartInfoNodeCount,
                 &NodeIndexer
                 );
    if (NodeSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: Invalid Size of Hart Info Nodes. Status = %r\n",
         Status
        )
        );
      goto error_handler;
    }

    TableSize += NodeSize;

    DEBUG (
      (
       DEBUG_INFO,
       " HartInfoNodeCount = %d\n" \
       " HartInfoOffset = %d\n",
       HartInfoNodeCount,
       HartInfoOffset
      )
      );
  }

  DEBUG (
    (
     DEBUG_INFO,
     "INFO: RHCT:\n" \
     " RhctNodeCount = %d\n" \
     " TableSize = 0x%lx\n",
     RhctNodeCount,
     TableSize
    )
    );

  if (TableSize > MAX_UINT32) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: RHCT Table Size 0x%lx > MAX_UINT32," \
       " Status = %r\n",
       TableSize,
       Status
      )
      );
    goto error_handler;
  }

  // Allocate the Buffer for RHCT table
  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to allocate memory for RHCT Table, Size = %d," \
       " Status = %r\n",
       TableSize,
       Status
      )
      );
    goto error_handler;
  }

  Rhct = (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE *)*Table;

  DEBUG (
    (
     DEBUG_INFO,
     "RHCT: Rhct = 0x%p TableSize = 0x%lx\n",
     Rhct,
     TableSize
    )
    );

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Rhct->Header,
             AcpiTableInfo,
             (UINT32)TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "ERROR: RHCT: Failed to add ACPI header. Status = %r\n",
       Status
      )
      );
    goto error_handler;
  }

  // Update RHCT table
  Rhct->NumNodes   = RhctNodeCount;
  Rhct->NodeOffset = sizeof (EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE);
  Rhct->TimerFreq  = TimerInfo->TimeBaseFrequency;
  Rhct->Flags      = TimerInfo->TimerCannotWakeCpu ? EFI_ACPI_6_6_RHCT_FLAG_TIMER_CANNOT_WAKE_CPU : 0;

  if (IsaStringNodeCount > 0) {
    Status = AddIsaStringNode (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Rhct,
               IsaStringOffset,
               IsaStringNode->IsaString
               );
    if (EFI_ERROR (Status)) {
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: Failed to add ISA string Node. Status = %r\n",
         Status
        )
        );
      goto error_handler;
    }
  }

  if (CmoNodeCount > 0) {
    Status = AddCmoNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Rhct,
               CmoOffset,
               CmoNodeList,
               CmoNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: Failed to add Named Component Node. Status = %r\n",
         Status
        )
        );
      goto error_handler;
    }
  }

  Offsets = AllocateZeroPool (sizeof (UINT32) * (RhctNodeCount - HartInfoNodeCount));
  Idx     = 0;
  if (IsaStringNodeCount > 0) {
    Offsets[Idx++] = IsaStringOffset;
  }

  if (CmoNodeCount > 0) {
    Offsets[Idx++] = CmoOffset;
  }

  if (HartInfoNodeCount > 0) {
    Status = AddHartInfoNodes (
               This,
               CfgMgrProtocol,
               AcpiTableInfo,
               Rhct,
               HartInfoOffset,
               RhctNodeCount - HartInfoNodeCount,
               Offsets,
               RintcInfoNodeList,
               HartInfoNodeCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG (
        (
         DEBUG_ERROR,
         "ERROR: RHCT: Failed to add Hart Info Node. Status = %r\n",
         Status
        )
        );
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

/** Free any resources allocated for constructing the RHCT

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
FreeRhctTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ACPI_RHCT_GENERATOR  *Generator;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  Generator = (ACPI_RHCT_GENERATOR *)This;

  // Free any memory allocated by the generator
  if (Generator->NodeIndexer != NULL) {
    FreePool (Generator->NodeIndexer);
    Generator->NodeIndexer = NULL;
  }

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: RHCT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** The RHCT Table Generator revision.
*/
#define RHCT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the MADT Table Generator.
*/
STATIC
ACPI_RHCT_GENERATOR  RhctGenerator = {
  // ACPI table generator header
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdRhct),
    // Generator Description
    L"ACPI.STD.RHCT.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE_SIGNATURE,
    // ACPI Table Revision supported by this Generator
    1,
    // Minimum supported ACPI Table Revision
    1,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID_RISCV,
    // Creator Revision
    RHCT_GENERATOR_REVISION,
    // Build Table function
    BuildRhctTable,
    // Free Resource function
    FreeRhctTableResources,
    // Extended build function not needed
    NULL,
    // Extended build function not implemented by the generator.
    // Hence extended free resource function is not required.
    NULL
  },

  // RHCT Generator private data

  // Rhct Node count
  0,
  // Pointer to Rhct node indexer
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
AcpiRhctLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&RhctGenerator.Header);
  DEBUG ((DEBUG_INFO, "RHCT: Register Generator. Status = %r\n", Status));
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
AcpiRhctLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&RhctGenerator.Header);
  DEBUG ((DEBUG_INFO, "Rhct: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
