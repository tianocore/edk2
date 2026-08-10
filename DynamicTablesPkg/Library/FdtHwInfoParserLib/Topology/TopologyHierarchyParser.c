/** @file
  Topology hierarchy parser.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <Library/ArmSmcccSocIdLib.h>
#include <IndustryStandard/Acpi63.h>

#include "CmObjectDescUtility.h"
#include "Topology/TopologyHierarchyParser.h"
#include "Topology/TopologyParser.h"
#include "Topology/TopologyUtility.h"

/** Count the number of nodes in a DT subtree.

  Under a "cpu-map" node, the following named node can be found:
  - socket
  - cluster
  - core
  - thread

  Each node will ultimately have to be represented by a ProcHierarchy
  CmObj structure. Count the number of structures required.

  @param [in]      Fdt        Pointer to the Flattened Device Tree.
  @param [in]      Node       Root node of the subtree to count.
  @param [in, out] NodeCount  Accumulated count of nodes in the subtree.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
CountSubTopologyNodes (
  IN  CONST VOID    *Fdt,
  IN        INT32   Node,
  IN OUT    UINT32  *NodeCount
  )
{
  EFI_STATUS  Status;
  INT32       ChildNode;
  UINT32      Count;

  if ((Fdt == NULL) || (NodeCount == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Count = 0;
  FdtForEachSubnode (ChildNode, Fdt, Node) {
    Count++;
    Status = CountSubTopologyNodes (Fdt, ChildNode, &Count);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  *NodeCount += Count;
  return EFI_SUCCESS;
}

/** Check the presence of socket nodes.

  In PPTT, only top-level nodes can have the PHYSICAL_PACKAGE flag.
  If a platform has multiple physical packages, the PPTT will contain
  multiple trees. A DT socket node corresponds to a physical package.

  Check the presence of socket node. In absence of socket nodes,
  a synthetic top-level node will be created wit the PHYSICAL_PACKAGE flag.

  @param [in]  Fdt              Pointer to the Flattened Device Tree.
  @param [in]  CpuMapNode       Offset of the "cpu-map" node.
  @param [out] TopLevelSockets  TRUE if all direct children are socket nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
HasTopLevelSocketNodes (
  IN  CONST VOID     *Fdt,
  IN        INT32    CpuMapNode,
  OUT       BOOLEAN  *TopLevelSockets
  )
{
  INT32    ChildNode;
  BOOLEAN  SawChild;

  if ((Fdt == NULL) || (TopLevelSockets == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  SawChild         = FALSE;
  *TopLevelSockets = FALSE;

  FdtForEachSubnode (ChildNode, Fdt, CpuMapNode) {
    SawChild = TRUE;
    if (!FdtNodeHasNameExt (Fdt, ChildNode, "socket")) {
      return EFI_SUCCESS;
    }
  }

  *TopLevelSockets = SawChild;
  return EFI_SUCCESS;
}

/** Populate the direct CPU child nodes below "\cpus".

  @param [in] Fdt       Pointer to the Flattened Device Tree.
  @param [in] CpusNode  Offset of the "\cpus" node.
  @param [in] CpuCount  Number of CPU nodes.
  @param [in] CpuNodes  On input, optional caller-allocated storage. On
                        output, populated with the discovered CPU nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No CPU nodes were found.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateCpuNodes (
  IN      CONST VOID    *Fdt,
  IN            INT32   CpusNode,
  IN            UINT32  CpuCount,
  IN            INT32   *CpuNodes
  )
{
  INT32   CpuNode;
  UINT32  Count;

  if ((Fdt == NULL) || (CpuCount == 0) || (CpuNodes == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Count = 0;
  FdtForEachSubnode (CpuNode, Fdt, CpusNode) {
    if (!IsCpuDeviceNode (Fdt, CpuNode, NULL)) {
      continue;
    }

    if (Count >= CpuCount) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    CpuNodes[Count] = CpuNode;
    Count++;
  }

  if (Count != CpuCount) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/** Allocate buffers owned by the topology parser context.

  This helper discovers the CPUs present under "\cpus", derives the processor
  hierarchy capacity from "cpu-map" when available, and allocates the
  temporary arrays needed by the topology builder.

  @param [in, out] Context     Topology parser context.
  @param [in]      Fdt         Pointer to the Flattened Device Tree.
  @param [in]      CpusNode    Offset of the "\cpus" node.
  @param [in]      CpuMapNode         Offset of the "cpu-map" node if present,
                                      or a negative libfdt error if absent.
  @param [in]      TopLevelSockets    TRUE if the direct "cpu-map" children
                                      are physical socket nodes.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No CPU nodes were found below "\cpus".
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
AllocateTopologyContext (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     CONST VOID               *Fdt,
  IN     INT32                    CpusNode,
  IN     INT32                    CpuMapNode,
  IN     BOOLEAN                  TopLevelSockets
  )
{
  EFI_STATUS  Status;
  UINT32      NodeCapacity;
  UINT32      Index;

  if ((Context == NULL) || (Fdt == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtCountCondNodeInBranch (Fdt, CpusNode, IsCpuDeviceNode, NULL, &Context->CpuCount);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Context->CpuNodes = AllocateZeroPool (sizeof (*Context->CpuNodes) * Context->CpuCount);
  if (Context->CpuNodes == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PopulateCpuNodes (Fdt, CpusNode, Context->CpuCount, Context->CpuNodes);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Check the default mask size can cover all the CPUs.
  if ((Context->CpuCount + 63) / 64 > TOPOLOGY_MASK_SIZE) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  if (CpuMapNode >= 0) {
    //
    // Add an additional synthetic top-level node if there are no socket nodes.
    //
    NodeCapacity = TopLevelSockets ? 0 : 1;
    Status       = CountSubTopologyNodes (Fdt, CpuMapNode, &NodeCapacity);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  } else {
    //
    // Default to a single package containing all CPUs when DT does not expose
    // a cpu-map hierarchy. This is sufficient for PPTT and SMBIOS consumers.
    //
    NodeCapacity = Context->CpuCount + 1;
  }

  Context->ProcHierarchyBuffers = AllocateZeroPool (
                                    sizeof (*Context->ProcHierarchyBuffers) * NodeCapacity
                                    );
  if (Context->ProcHierarchyBuffers == NULL) {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < NodeCapacity; Index++) {
    Context->ProcHierarchyBuffers[Index].ParentIndex = TOPOLOGY_INVALID_INDEX;
    Context->ProcHierarchyBuffers[Index].CpuNode     = -1;
  }

  Context->ProcHierarchyCount = NodeCapacity;
  return EFI_SUCCESS;
}

/** Find the index of a CPU DT node.

  @param [in]  Context   Topology parser context.
  @param [in]  CpuNode   CPU DT node offset.
  @param [out] CpuIndex  CPU index.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_FOUND         The CPU node is not tracked in the context.
  @retval EFI_INVALID_PARAMETER Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FindCpuIndex (
  IN  CONST TOPOLOGY_PARSER_CONTEXT  *Context,
  IN        INT32                    CpuNode,
  OUT       UINT32                   *CpuIndex
  )
{
  UINT32  Index;

  if ((Context == NULL) || (CpuIndex == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < Context->CpuCount; Index++) {
    if (Context->CpuNodes[Index] == CpuNode) {
      *CpuIndex = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/** Propagate CPU ownership from a leaf to its ancestor hierarchy nodes.

  @param [in, out] Context    Topology parser context.
  @param [in]      NodeIndex  Leaf processor hierarchy node index.
  @param [in]      CpuIndex   CPU index.
**/
STATIC
VOID
EFIAPI
PropagateCpuToAncestors (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     UINT32                   NodeIndex,
  IN     UINT32                   CpuIndex
  )
{
  UINT64  *CpuMask;

  ASSERT (Context != NULL);

  while (NodeIndex != TOPOLOGY_INVALID_INDEX) {
    CpuMask = GetProcHierarchyCpuMask (Context, NodeIndex);
    SetCpuMaskBit (CpuMask, CpuIndex);
    NodeIndex = (UINT32)Context->ProcHierarchyBuffers[NodeIndex].ParentIndex;
  }
}

/**
  Return the SoC ID formatted for the SMBIOS Type 4 Processor ID field.

  @param[out] ProcessorId  Pointer to the SMBIOS Processor ID.

  @retval EFI_SUCCESS            The Processor ID was returned successfully.
  @retval EFI_INVALID_PARAMETER  ProcessorId is NULL.
  @retval EFI_UNSUPPORTED        The SMCCC Architecture SoC ID interface is
                                 unsupported or an SoC ID call failed.
**/
STATIC
EFI_STATUS
GetSocId (
  OUT UINT64  *ProcessorId
  )
{
  EFI_STATUS  Status;
  UINT32      Jep106Code;
  UINT32      SocRevision;

  if (ProcessorId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ArmSmcccGetSocId (&Jep106Code, &SocRevision);
  if (!EFI_ERROR (Status)) {
    *ProcessorId = ((UINT64)SocRevision << 32) | Jep106Code;
  }

  return Status;
}

/** Set ProcessorId for all processor hierarchy nodes.

  Indeed, Device Tree does not provide a standard socket/package identifier.

  @param [in, out] Context  Topology parser context.
**/
STATIC
VOID
EFIAPI
SetProcHierarchyProcessorId (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context
  )
{
  UINT64  SocId;
  UINT32  Index;

  if (Context == NULL) {
    ASSERT (FALSE);
    return;
  }

  if (EFI_ERROR (GetSocId (&SocId))) {
    DEBUG ((DEBUG_WARN, "Could not get SocId.\n"));
    return;
  }

  for (Index = 0; Index < Context->ProcHierarchyCount; Index++) {
    Context->ProcHierarchyBuffers[Index].Info.ProcessorId = SocId;
  }
}

/** Create a deterministic abstract token for a processor hierarchy node.

  @param [in] Index  Zero-based processor hierarchy node index.

  @retval A CM_OBJECT_TOKEN in the FDT topology abstract-token namespace.
**/
STATIC
CM_OBJECT_TOKEN
EFIAPI
CreateProcHierarchyToken (
  IN UINT32  Index
  )
{
  return (CM_OBJECT_TOKEN)CM_ABSTRACT_TOKEN_MAKE (
                            ETokenNameSpaceFdtHwInfo,
                            EFdtHwInfoProcHierarchyObject,
                            Index + 1
                            );
}

/** Append a processor hierarchy node to the topology buffer.

  @param [in, out] Context             Topology parser context.
  @param [in]     ParentIndex          Index of the parent topology node.
  @param [in]     ParentToken          Token of the parent topology node.
  @param [in]     AcpiIdObjectToken    Architecture-specific ACPI ID backing
                                       object token for leaf nodes.
  @param [in]     Flags                Processor hierarchy flags.
  @param [out]    ProcHierarchyNode    Pointer to the appended node.
  @param [out]    CurrProcHierarchyIndex   Index of the appended node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_ABORTED             The topology buffer is full.
**/
STATIC
EFI_STATUS
EFIAPI
AppendProcHierarchyNode (
  IN OUT TOPOLOGY_PARSER_CONTEXT             *Context,
  IN     UINT32                              ParentIndex,
  IN     CM_OBJECT_TOKEN                     ParentToken,
  IN     CM_OBJECT_TOKEN                     AcpiIdObjectToken,
  IN     UINT32                              Flags,
  OUT    CM_ARCH_COMMON_PROC_HIERARCHY_INFO  **ProcHierarchyNode,
  OUT    UINT32                              *CurrProcHierarchyIndex
  )
{
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *Node;
  UINT32                              NodeIndex;

  if ((Context == NULL) || (ProcHierarchyNode == NULL) || (CurrProcHierarchyIndex == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (Context->CurrProcHierarchyIndex >= Context->ProcHierarchyCount) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  NodeIndex = Context->CurrProcHierarchyIndex;
  Node      = &Context->ProcHierarchyBuffers[NodeIndex].Info;
  ZeroMem (Node, sizeof (*Node));
  Node->Token = CreateProcHierarchyToken (NodeIndex);

  //
  // Note: PPTT_VALID_FLAG_MASK should depend on the generation of
  // a SSDT CPU TOPOLOGY table. Assume it is always generated.
  //
  Node->Flags                                          = Flags | PPTT_VALID_FLAG_MASK;
  Node->ParentToken                                    = ParentToken;
  Node->AcpiIdObjectToken                              = AcpiIdObjectToken;
  Context->ProcHierarchyBuffers[NodeIndex].ParentIndex = (INT32)ParentIndex;
  Context->ProcHierarchyBuffers[NodeIndex].Depth       = (ParentIndex == TOPOLOGY_INVALID_INDEX) ? 0 : (Context->ProcHierarchyBuffers[ParentIndex].Depth + 1);
  Context->ProcHierarchyBuffers[NodeIndex].CpuNode     = -1;

  Context->CurrProcHierarchyIndex++;
  *ProcHierarchyNode      = Node;
  *CurrProcHierarchyIndex = NodeIndex;
  return EFI_SUCCESS;
}

/** Add a CPU leaf node to the processor hierarchy.

  @param [in, out] Context      Topology parser context.
  @param [in]     ParentIndex   Index of the parent topology node.
  @param [in]     ParentToken   Token of the parent topology node.
  @param [in]     CpuNode       DT CPU node offset.
  @param [in]     IsThread      Whether the CPU leaf represents a hardware
                                thread.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No backing ACPI ID object token was found.
**/
STATIC
EFI_STATUS
EFIAPI
AddCpuCpuCount (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     UINT32                   ParentIndex,
  IN     CM_OBJECT_TOKEN          ParentToken,
  IN     INT32                    CpuNode,
  IN     BOOLEAN                  IsThread
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNode;
  UINT32                              CurrProcHierarchyIndex;
  UINT32                              Flags;
  UINT32                              CpuIndex;

  Flags = PPTT_LEAF_FLAG_MASK;
  if (IsThread) {
    Flags |= PPTT_THREAD_FLAG_MASK;
  }

  Status = AppendProcHierarchyNode (
             Context,
             ParentIndex,
             ParentToken,
             (CM_OBJECT_TOKEN)CM_ABSTRACT_TOKEN_MAKE (ETokenNameSpaceFdtHwInfo, EFdtHwInfoGicCObject, (UINT32)CpuNode + 1),
             Flags,
             &ProcHierarchyNode,
             &CurrProcHierarchyIndex
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Status = FindCpuIndex (Context, CpuNode, &CpuIndex);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Context->ProcHierarchyBuffers[CurrProcHierarchyIndex].CpuNode = CpuNode;
  PropagateCpuToAncestors (Context, CurrProcHierarchyIndex, CpuIndex);

  return EFI_SUCCESS;
}

/** Parse one DT "cpu-map" topology node.

  This helper recursively converts "socket", "cluster", "core", and "thread"
  cpu-map nodes into processor hierarchy objects. Leaf nodes are expected to
  expose a "cpu" phandle property.

  @param [in, out] Context      Topology parser context.
  @param [in]     Fdt           Pointer to the Flattened Device Tree.
  @param [in]     Node          DT cpu-map node to parse.
  @param [in]     ParentIndex   Index of the parent hierarchy node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             Malformed cpu-map information was found.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ParseCpuMapNode (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     CONST VOID               *Fdt,
  IN     INT32                    Node,
  IN     UINT32                   ParentIndex
  )
{
  EFI_STATUS                          Status;
  CONST UINT32                        *CpuProp;
  INT32                               CpuPropSize;
  INT32                               ChildNode;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *ProcHierarchyNode;
  UINT32                              CurrProcHierarchyIndex;
  CM_OBJECT_TOKEN                     ParentToken;
  UINT32                              Flags;

  ParentToken = (ParentIndex == TOPOLOGY_INVALID_INDEX) ?
                CM_NULL_TOKEN :
                Context->ProcHierarchyBuffers[ParentIndex].Info.Token;

  //
  // If this is a cpu, create a leaf node.
  //
  CpuProp = FdtGetProp (Fdt, Node, "cpu", &CpuPropSize);
  if (CpuProp != NULL) {
    if (CpuPropSize != sizeof (UINT32)) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    ChildNode = FdtNodeOffsetByPhandle (Fdt, Fdt32ToCpu (*CpuProp));
    if (ChildNode < 0) {
      ASSERT (FALSE);
      return EFI_ABORTED;
    }

    return AddCpuCpuCount (
             Context,
             ParentIndex,
             ParentToken,
             ChildNode,
             FdtNodeHasNameExt (Fdt, Node, "thread")
             );
  }

  //
  // Otherwise create a cluster/group and recursively continue.
  //
  Flags = ((ParentIndex == TOPOLOGY_INVALID_INDEX) &&
           FdtNodeHasNameExt (Fdt, Node, "socket")) ?
          PPTT_PACKAGE_PHYSICAL_FLAG_MASK :
          0;

  Status = AppendProcHierarchyNode (
             Context,
             ParentIndex,
             ParentToken,
             CM_NULL_TOKEN,
             Flags,
             &ProcHierarchyNode,
             &CurrProcHierarchyIndex
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  FdtForEachSubnode (ChildNode, Fdt, Node) {
    Status = ParseCpuMapNode (Context, Fdt, ChildNode, CurrProcHierarchyIndex);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Create a flat processor topology from the direct children of "\cpus".

  @param [in, out] Context      Topology parser context.
  @param [in]     Fdt           Pointer to the Flattened Device Tree.
  @param [in]     CpusNode      Offset of the "\cpus" node.
  @param [in]     PackageIndex  Index of tha synthetic package node.

  @retval EFI_SUCCESS   The function completed successfully.
  @retval EFI_NOT_FOUND No CPU nodes were found below "\cpus".
**/
STATIC
EFI_STATUS
EFIAPI
CreateFlatTopology (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     CONST VOID               *Fdt,
  IN     INT32                    CpusNode,
  IN     UINT32                   PackageIndex
  )
{
  EFI_STATUS  Status;
  INT32       CpuNode;

  FdtForEachSubnode (CpuNode, Fdt, CpusNode) {
    if (!IsCpuDeviceNode (Fdt, CpuNode, NULL)) {
      continue;
    }

    Status = AddCpuCpuCount (
               Context,
               PackageIndex,
               Context->ProcHierarchyBuffers[PackageIndex].Info.Token,
               CpuNode,
               FALSE
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Create processor hierarchy objects from Device Tree CPU topology.

  Parse the standard DT CPU topology description and create
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO objects.
  If no "cpu-map" node is present, create a flat topology.

  @param [in, out] Context    Topology parser context.
  @param [in]      CpusNode   Offset of the "\cpus" node.

  @retval EFI_ABORTED             Malformed or inconsistent CPU topology
                                  information was found.
  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No usable CPU topology information was found.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
CreateProcHierarchyInfo (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context,
  IN     INT32                    CpusNode
  )
{
  EFI_STATUS                          Status;
  CONST VOID                          *Fdt;
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *PackageNode;
  UINT32                              PackageIndex;
  UINT32                              CpuLeafCount;
  UINT32                              Index;
  INT32                               CpuMapNode;
  INT32                               ChildNode;
  BOOLEAN                             TopLevelSockets;

  if ((Context == NULL) || (Context->FdtParserHandle == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt             = Context->FdtParserHandle->Fdt;
  CpuMapNode      = FdtSubnodeOffset (Fdt, CpusNode, "cpu-map");
  TopLevelSockets = FALSE;

  if (CpuMapNode >= 0) {
    Status = HasTopLevelSocketNodes (Fdt, CpuMapNode, &TopLevelSockets);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  Status = AllocateTopologyContext (Context, Fdt, CpusNode, CpuMapNode, TopLevelSockets);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (CpuMapNode >= 0) {
    if (!TopLevelSockets) {
      //
      // No socket node, create a synthetic top-level physical package in
      // accordance to PPTT.
      //
      Status = AppendProcHierarchyNode (
                 Context,
                 TOPOLOGY_INVALID_INDEX,
                 CM_NULL_TOKEN,
                 CM_NULL_TOKEN,
                 PPTT_PACKAGE_PHYSICAL_FLAG_MASK,
                 &PackageNode,
                 &PackageIndex
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        return Status;
      }
    }

    FdtForEachSubnode (ChildNode, Fdt, CpuMapNode) {
      Status = ParseCpuMapNode (
                 Context,
                 Fdt,
                 ChildNode,
                 TopLevelSockets ? TOPOLOGY_INVALID_INDEX : PackageIndex
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        return Status;
      }
    }
  } else {
    //
    // No cpu-map: create a synthetic package that owns all CPU leaves.
    //
    Status = AppendProcHierarchyNode (
               Context,
               TOPOLOGY_INVALID_INDEX,
               CM_NULL_TOKEN,
               CM_NULL_TOKEN,
               PPTT_PACKAGE_PHYSICAL_FLAG_MASK,
               &PackageNode,
               &PackageIndex
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }

    Status = CreateFlatTopology (Context, Fdt, CpusNode, PackageIndex);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  //
  // Check that all the ProcHierarchyBuffers entries have been used.
  //
  CpuLeafCount = 0;
  for (Index = 0; Index < Context->CurrProcHierarchyIndex; Index++) {
    if (Context->ProcHierarchyBuffers[Index].CpuNode != -1) {
      CpuLeafCount++;
    }
  }

  if (CpuLeafCount == 0) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  if (CpuLeafCount != Context->CpuCount) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  SetProcHierarchyProcessorId (Context);
  return EFI_SUCCESS;
}
