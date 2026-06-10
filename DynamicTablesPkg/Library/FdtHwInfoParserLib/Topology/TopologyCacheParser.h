/** @file
  Cache topology parser.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "Topology/TopologyParser.h"

/** Parse cache information for all CPUs in the topology context.

  This helper derives CM_ARCH_COMMON_CACHE_INFO objects and attaches them to
  the appropriate processor hierarchy nodes through private-resource
  references.

  @param [in, out] Context  Topology parser context.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             Malformed cache information was found.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           A CPU leaf could not be found for a DT CPU.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
ParseCacheInfo (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context
  );
