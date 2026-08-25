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
  FDT_HW_INFO_PARSER_HANDLE             FdtParserHandle;

  ///
  /// Number of leaf nodes, i.e. cpus or threads.
  ///
  UINT32                                CpuCount;

  ///
  /// Number of processor hierarchy entries.
  ///
  UINT32                                ProcHierarchyCount;

  ///
  /// Current index of the processor hierarchy entry being populated.
  /// This is a convenient context variable.
  ///
  UINT32                                CurrProcHierarchyIndex;

  ///
  /// Buffered processor hierarchy objects to be published.
  ///
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO    *ProcHierarchyInfo;
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
