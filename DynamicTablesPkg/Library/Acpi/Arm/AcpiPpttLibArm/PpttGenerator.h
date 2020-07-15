/** @file
  Header file for the dynamic PPTT generator

  Copyright (c) 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.3 Specification, January 2019
  - ARM Architecture Reference Manual ARMv8 (D.a)

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef PPTT_GENERATOR_H_
#define PPTT_GENERATOR_H_

#pragma pack(1)

/// Cache parameters allowed by the architecture with
/// ARMv8.3-CCIDX (Cache extended number of sets)
/// Derived from CCSIDR_EL1 when ID_AA64MMFR2_EL1.CCIDX==0001
#define PPTT_ARM_CCIDX_CACHE_NUMBER_OF_SETS_MAX       (1 << 24)
#define PPTT_ARM_CCIDX_CACHE_ASSOCIATIVITY_MAX        (1 << 21)

/// Cache parameters allowed by the architecture without
/// ARMv8.3-CCIDX (Cache extended number of sets)
/// Derived from CCSIDR_EL1 when ID_AA64MMFR2_EL1.CCIDX==0000
#define PPTT_ARM_CACHE_NUMBER_OF_SETS_MAX             (1 << 15)
#define PPTT_ARM_CACHE_ASSOCIATIVITY_MAX              (1 << 10)

/// Common cache parameters
/// Derived from CCSIDR_EL1
/// The LineSize is represented by bits 2:0
/// (Log2(Number of bytes in cache line)) - 4 is used to represent
/// the LineSize bits.
#define PPTT_ARM_CACHE_LINE_SIZE_MAX                  (1 << 11)
#define PPTT_ARM_CACHE_LINE_SIZE_MIN                  (1 << 4)

/// Test if the given Processor Hierarchy Info object has the 'Node is a Leaf'
/// flag set
#define IS_PROC_NODE_LEAF(Node) ((Node->Flags & BIT3) != 0)

/// Test if the given Processor Hierarchy Info object has the 'ACPI Processor
/// ID valid' flag set
#define IS_ACPI_PROC_ID_VALID(Node) ((Node->Flags & BIT1) != 0)

/**
  The GET_SIZE_OF_PPTT_STRUCTS macro expands to a function that is used to
  calculate the total memory requirement for the PPTT structures represented
  by the given list of Configuration Manager Objects of the same type. This
  function also indexes the input CM objects so that various other CM objects
  (possibly of different type) can reference them.

  The size of memory needed for the specified type of PPTT structures is based
  on the number and type of CM objects provided. The macro assumes that the
  ACPI object PpttObjName has fixed size.

  The macro expands to a function which has the following prototype:

  STATIC
  UINT32
  EFIAPI
  GetSizeof<PpttObjName> (
    IN      CONST UINT32                      StartOffset,
    IN      CONST CmObjectType       *        Nodes,
    IN            UINT32                      NodeCount,
    IN OUT        PPTT_NODE_INDEXER ** CONST  NodeIndexer
  )

  Generated function parameters:
  @param [in]       StartOffset     Offset from the start of PPTT to where
                                    the PPTT structures will be placed.
  @param [in]       NodesToIndex    Pointer to the list of CM objects to be
                                    indexed and size-estimated.
  @param [out]      NodeCount       Number of CM objects in NodesToIndex.
  @param [in, out]  NodeIndexer     Pointer to the list of Node Indexer
                                    elements to populate.
  @retval           Size            Total memory requirement for the PPTT
                                    structures described in NodesToIndex.

  Macro Parameters:
  @param [in]       PpttObjName     Name for the type of PPTT structures which
                                    size is estimated.
  @param [in]       PpttObjSize     Expression to use to calculate the size of
                                    of a single instance of the PPTT structure
                                    which corresponds to the CM object being
                                    indexed.
  @param [in]       CmObjectType    Data type of the CM nodes in NodesToIndex.
**/
#define GET_SIZE_OF_PPTT_STRUCTS(                                             \
  PpttObjName,                                                                \
  PpttObjSize,                                                                \
  CmObjectType                                                                \
)                                                                             \
STATIC                                                                        \
UINT32                                                                        \
GetSizeof##PpttObjName (                                                      \
  IN      CONST UINT32                      StartOffset,                      \
  IN      CONST CmObjectType      *         NodesToIndex,                     \
  IN            UINT32                      NodeCount,                        \
  IN OUT        PPTT_NODE_INDEXER ** CONST  NodeIndexer                       \
  )                                                                           \
{                                                                             \
  UINT32  Size;                                                               \
                                                                              \
  ASSERT (                                                                    \
    (NodesToIndex != NULL) &&                                                 \
    (NodeIndexer != NULL)                                                     \
    );                                                                        \
                                                                              \
  Size = 0;                                                                   \
  while (NodeCount-- != 0) {                                                  \
    (*NodeIndexer)->Token = NodesToIndex->Token;                              \
    (*NodeIndexer)->Object = (VOID*)NodesToIndex;                             \
    (*NodeIndexer)->Offset = Size + StartOffset;                              \
    (*NodeIndexer)->CycleDetectionStamp = 0;                                  \
    (*NodeIndexer)->TopologyParent = NULL;                                    \
    DEBUG ((                                                                  \
      DEBUG_INFO,                                                             \
      "PPTT: Node Indexer = %p, Token = %p, Object = %p, Offset = 0x%x\n",    \
      *NodeIndexer,                                                           \
      (*NodeIndexer)->Token,                                                  \
      (*NodeIndexer)->Object,                                                 \
      (*NodeIndexer)->Offset                                                  \
      ));                                                                     \
                                                                              \
    Size += PpttObjSize;                                                      \
    (*NodeIndexer)++;                                                         \
    NodesToIndex++;                                                           \
  }                                                                           \
  return Size;                                                                \
}

/**
  A structure for indexing CM objects (nodes) used in PPTT generation.

  PPTT_NODE_INDEXER is a wrapper around CM objects which augments these objects
  with additional information that enables generating PPTT structures with
  correct cross-references.

  PPTT_NODE_INDEXER keeps track of each structure's offset from the base
  address of the generated table. It also caches certain information and makes
  PPTT cyclic reference detection possible.
*/
typedef struct PpttNodeIndexer {
  /// Unique identifier for the node
  CM_OBJECT_TOKEN           Token;
  /// Pointer to the CM object being indexed
  VOID                    * Object;
  /// Offset from the start of the PPTT table to the PPTT structure which is
  /// represented by Object
  UINT32                    Offset;
  /// Field used to mark nodes as 'visited' when detecting cycles in processor
  /// and cache topology
  UINT32                    CycleDetectionStamp;
  /// Reference to a Node Indexer element which is the parent of this Node
  /// Indexer element in the processor and cache topology
  /// e.g For a hardware thread the TopologyParent would point to a CPU node
  ///     For a L1 cache the TopologyParent would point to a L2 cache
  struct PpttNodeIndexer  * TopologyParent;
} PPTT_NODE_INDEXER;

typedef struct AcpiPpttGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR  Header;
  /// PPTT structure count
  UINT32                ProcTopologyStructCount;
  /// Count of Processor Hierarchy Nodes
  UINT32                ProcHierarchyNodeCount;
  /// Count of Cache Structures
  UINT32                CacheStructCount;
  /// Count of Id Structures
  UINT32                IdStructCount;
  /// List of indexed CM objects for PPTT generation
  PPTT_NODE_INDEXER   * NodeIndexer;
  /// Pointer to the start of Processor Hierarchy nodes in
  /// the Node Indexer array
  PPTT_NODE_INDEXER   * ProcHierarchyNodeIndexedList;
  /// Pointer to the start of Cache Structures in the Node Indexer array
  PPTT_NODE_INDEXER   * CacheStructIndexedList;
  /// Pointer to the start of Id Structures in the Node Indexer array
  PPTT_NODE_INDEXER   * IdStructIndexedList;
} ACPI_PPTT_GENERATOR;

#pragma pack()

#endif // PPTT_GENERATOR_H_
