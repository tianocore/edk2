/** @file
  Topology utility functions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>

#include "Topology/TopologyHierarchyParser.h"
#include "Topology/TopologyParser.h"
#include "Topology/TopologyUtility.h"

/** Get the CPU-mask associated to a Node (i.e. cluster/socket).

  @param [in] Context    Topology parser context.
  @param [in] NodeIndex  Processor hierarchy node index.

  @return Pointer to the CPU mask.
**/
UINT64 *
EFIAPI
GetProcHierarchyCpuMask (
  IN CONST TOPOLOGY_PARSER_CONTEXT  *Context,
  IN UINT32                         NodeIndex
  )
{
  ASSERT (Context != NULL);
  ASSERT (NodeIndex < Context->ProcHierarchyCount);
  return Context->ProcHierarchyBuffers[NodeIndex].CpuMasks;
}

/** Set a CPU bit in a topology CPU mask.

  @param [in, out] Mask      CPU mask to update.
  @param [in]      CpuIndex  CPU index.
**/
VOID
EFIAPI
SetCpuMaskBit (
  IN OUT UINT64  *Mask,
  IN     UINT32  CpuIndex
  )
{
  ASSERT (Mask != NULL);
  Mask[CpuIndex / 64] |= LShiftU64 (1ULL, CpuIndex % 64);
}
