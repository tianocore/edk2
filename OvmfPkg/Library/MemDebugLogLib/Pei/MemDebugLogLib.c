/** @file
 *
  Memory Debug Log Library - PEI Phase

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemDebugLogLib.h>
#include <Library/MemDebugLogCommonLib.h>
#include <Ppi/MemDebugLog.h>

EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN  Length
  )
{
  MEM_DEBUG_LOG_PPI  *MemDebugLogPpi;
  EFI_STATUS         Status;

  MemDebugLogPpi = NULL;
  Status         = PeiServicesLocatePpi (
                     &gMemDebugLogPpiGuid,
                     0,
                     NULL,
                     (VOID **)&MemDebugLogPpi
                     );

  if ((Status == EFI_SUCCESS) && MemDebugLogPpi) {
    //
    // PPI is installed, so use the PPI
    //
    Status = MemDebugLogPpi->MemDebugLogPpiWrite (Buffer, Length);
  } else {
    //
    // No PPI installed (yet), so write to the early debug log buffer
    //
    if (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase) != 0x0) {
      Status = MemDebugLogWriteCommon (
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
