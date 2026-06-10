/** @file
  Generic topology parser.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <ConfigurationManagerObject.h>

#include "FdtHwInfoParser.h"

// PPTT Flag masks
#define PPTT_PACKAGE_PHYSICAL_FLAG_MASK  EFI_ACPI_6_3_PPTT_PACKAGE_PHYSICAL
#define PPTT_VALID_FLAG_MASK             (EFI_ACPI_6_3_PPTT_PROCESSOR_ID_VALID << 1)
#define PPTT_THREAD_FLAG_MASK            (EFI_ACPI_6_3_PPTT_PROCESSOR_IS_THREAD << 2)
#define PPTT_LEAF_FLAG_MASK              (EFI_ACPI_6_3_PPTT_NODE_IS_LEAF << 3)

// Invalid Fdt index.
#define TOPOLOGY_INVALID_INDEX  MAX_UINT32

//
// Number of UINT64 words needed to represent CpuCount CPUs as a bitmask.
// Currently set to 4, i.e. 256 (=4 * 64) CPUs
//
#define TOPOLOGY_MASK_SIZE  4U

/** Buffered representation of one processor-hierarchy node.

  Each entry carries the CM_ARCH_COMMON_PROC_HIERARCHY_INFO object that will
  be published along with parser-only metadata used to resolve parents,
  ownership, and CPU sharing.
**/
typedef struct {
  ///
  /// Buffered processor hierarchy object to be published.
  ///
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO    Info;

  ///
  /// CPU-sharing mask for this processor hierarchy node.
  ///
  UINT64                                CpuMasks[TOPOLOGY_MASK_SIZE];

  ///
  /// Parent index for this processor hierarchy entry.
  ///
  INT32                                 ParentIndex;

  ///
  /// Depth of this processor hierarchy node.
  ///
  UINT32                                Depth;

  ///
  /// Back-reference from hierarchy leaf nodes to the DT CPU node.
  ///
  INT32                                 CpuNode;
} TOPOLOGY_PROC_HIERARCHY_BUFFERS;

/** Buffered representation of one cache object.

  Each entry carries the CM_ARCH_COMMON_CACHE_INFO object that will be
  published along with parser-only metadata describing the DT cache node,
  sharing mask, and owning processor-hierarchy node.
**/
typedef struct {
  ///
  /// Buffered cache objects to be published.
  ///
  CM_ARCH_COMMON_CACHE_INFO    Info;

  ///
  /// Per-cache CPU-sharing masks.
  ///
  UINT64                       CpuMasks[TOPOLOGY_MASK_SIZE];

  ///
  /// DT node offsets for shared cache nodes, or -1 for synthetic L1 entries.
  ///
  INT32                        DtNode;

  ///
  /// Owning processor hierarchy node index for each cache object.
  ///
  UINT32                       OwnerProcIndex;
} TOPOLOGY_CACHE_BUFFERS;

/** Working state for building processor-hierarchy and cache objects from DT.

  The context owns all temporary arrays used while deriving topology and cache
  sharing information. The caller is responsible for zero-initializing the
  structure before first use and releasing any allocated buffers when parsing
  is complete.
**/
typedef struct {
  ///
  /// Parser instance used to access the DT and publish generated CM objects.
  ///
  FDT_HW_INFO_PARSER_HANDLE          FdtParserHandle;

  ///
  /// Number of leaf nodes, i.e. cpus or threads.
  ///
  UINT32                             CpuCount;

  ///
  /// Number of processor hierarchy entries.
  ///
  UINT32                             ProcHierarchyCount;

  ///
  /// Current index of the processor hierarchy entry being populated.
  /// This is a convenient context variable.
  ///
  UINT32                             CurrProcHierarchyIndex;

  ///
  /// Buffered processor hierarchy objects and metadata arrays.
  ///
  TOPOLOGY_PROC_HIERARCHY_BUFFERS    *ProcHierarchyBuffers;

  ///
  /// Ordered list of CPU DT nodes under "\cpus".
  ///
  INT32                              *CpuNodes;

  ///
  /// Buffered cache objects and parallel cache metadata arrays.
  ///
  TOPOLOGY_CACHE_BUFFERS             *CacheBuffers;

  ///
  /// Number of caches.
  ///
  UINT32                             CacheCount;

  ///
  /// Allocated capacity of the CacheBuffers array.
  ///
  UINT32                             CacheCapacity;

  ///
  /// Index of the cache entry currently being parsed.
  ///
  UINT32                             CurrCacheIndex;
} TOPOLOGY_PARSER_CONTEXT;

/** Processor topology parser.

  Parse a Device Tree and populate the following objects:
  - CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  - CM_ARCH_COMMON_CACHE_INFO
  - CM_ARCH_COMMON_OBJ_REF arrays referenced by processor hierarchy nodes

  @param [in] FdtParserHandle  Parser instance handle.
  @param [in] FdtBranch        Unused. The parser always resolves the global
                               "\cpus" node.

  @retval EFI_ABORTED             Malformed topology or cache information was
                                  found.
  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The "\cpus" node or CPU topology was not
                                  found.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
TopologyInfoParser (
  IN CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN INT32                            FdtBranch
  );
