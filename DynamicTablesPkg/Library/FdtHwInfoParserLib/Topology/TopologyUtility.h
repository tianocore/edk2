/** @file
  Topology utility functions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

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
  );

/** Set a CPU bit in a topology CPU mask.

  @param [in, out] Mask      CPU mask to update.
  @param [in]      CpuIndex  CPU index.
**/
VOID
EFIAPI
SetCpuMaskBit (
  IN OUT UINT64  *Mask,
  IN     UINT32  CpuIndex
  );

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
  );

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
  );

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
  );
