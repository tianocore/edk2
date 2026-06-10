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

/** Get the CPU-mask associated to a cache.

  @param [in] Context    Topology parser context.
  @param [in] CacheIndex Cache node index.

  @return Pointer to the CPU mask.
**/
UINT64 *
EFIAPI
GetCacheCpuMask (
  IN CONST TOPOLOGY_PARSER_CONTEXT  *Context,
  IN UINT32                         CacheIndex
  )
{
  ASSERT (Context != NULL);
  ASSERT (
    (CacheIndex < Context->CacheCount) ||
    ((CacheIndex == Context->CurrCacheIndex) && (CacheIndex < Context->CacheCapacity))
    );
  return Context->CacheBuffers[CacheIndex].CpuMasks;
}

/** Test whether one CPU mask is a subset of another.

  @param [in] Candidate    Candidate subset.
  @param [in] Container    Candidate superset.

  @retval TRUE   Candidate is a subset of Container.
  @retval FALSE  Otherwise.
**/
BOOLEAN
EFIAPI
IsCpuMaskSubset (
  IN CONST UINT64  *Candidate,
  IN CONST UINT64  *Container
  )
{
  UINT32  WordIndex;

  ASSERT ((Candidate != NULL) && (Container != NULL));

  for (WordIndex = 0; WordIndex < TOPOLOGY_MASK_SIZE; WordIndex++) {
    if ((Candidate[WordIndex] & ~Container[WordIndex]) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/** Test whether two CPU masks are identical.

  @param [in] Left     First CPU mask.
  @param [in] Right    Second CPU mask.

  @retval TRUE   Both masks are identical.
  @retval FALSE  Otherwise.
**/
BOOLEAN
EFIAPI
IsCpuMaskEqual (
  IN CONST UINT64  *Left,
  IN CONST UINT64  *Right
  )
{
  ASSERT ((Left != NULL) && (Right != NULL));

  return (CompareMem (Left, Right, sizeof (UINT64) * TOPOLOGY_MASK_SIZE) == 0);
}
