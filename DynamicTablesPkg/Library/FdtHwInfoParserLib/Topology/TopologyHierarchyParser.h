/** @file
  Topology hierarchy parser.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "Topology/TopologyParser.h"

/** Create a deterministic abstract token allowing to identify a CPU.

  This is an arch-specific function.
  For Arm, a Token identifying a GicC object will be returned.

  @param [in] CpuNode  CPU DT node offset.

  @retval A CM_OBJECT_TOKEN in the FDT hardware-info abstract-token namespace.
**/
CM_OBJECT_TOKEN
EFIAPI
CreateArchCpuToken (
  IN INT32  CpuNode
  );

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
  );
