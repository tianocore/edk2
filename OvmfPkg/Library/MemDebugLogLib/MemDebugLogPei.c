/** @file
 *
  Memory Debug Log Library - PEI Phase

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/MemDebugLogLib.h>

UINT32
EFIAPI
MemDebugLogPages (
  VOID
  )
{
  UINT32      FwCfg_MemDebugLogPages;
  UINT32      MemDebugLogBufPages;
  EFI_STATUS  Status;

  //
  // Allow FwCfg value to override Pcd.
  //
  Status = QemuFwCfgParseUint32 ("opt/ovmf/MemDebugLogPages", TRUE, &FwCfg_MemDebugLogPages);
  if (Status == EFI_SUCCESS) {
    MemDebugLogBufPages = FwCfg_MemDebugLogPages;
  } else {
    MemDebugLogBufPages = FixedPcdGet32 (PcdMemDebugLogPages);
  }

  //
  // Cap max memory debug log size at MAX_MEM_DEBUG_LOG_PAGES
  //
  if (MemDebugLogBufPages > MAX_MEM_DEBUG_LOG_PAGES) {
    MemDebugLogBufPages = MAX_MEM_DEBUG_LOG_PAGES;
  }

  return MemDebugLogBufPages;
}
