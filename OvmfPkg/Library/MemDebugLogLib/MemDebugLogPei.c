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

EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN  Length
  )
{
  EFI_PHYSICAL_ADDRESS  MemDebugLogBufAddr;
  EFI_STATUS            Status;

  //
  // Obtain the Memory Debug Log buffer addr from HOB
  // NOTE: This is expected to fail until the HOB is created.
  //
  Status = MemDebugLogAddrFromHOB (&MemDebugLogBufAddr);

  if (EFI_ERROR (Status)) {
    MemDebugLogBufAddr = 0;
  }

  if (MemDebugLogBufAddr != 0) {
    Status = MemDebugLogWriteBuffer (MemDebugLogBufAddr, Buffer, Length);
  } else {
    //
    // HOB has not yet been created, so
    // write to the early debug log buffer.
    //
    if (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase) != 0x0) {
      Status = MemDebugLogWriteBuffer (
                 (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase),
                 Buffer,
                 Length
                 );
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  return Status;
}

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
