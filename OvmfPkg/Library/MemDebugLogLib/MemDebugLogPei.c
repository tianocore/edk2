/** @file
 *
  Memory Debug Log Library - PEI Phase

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemDebugLogLib.h>

EFI_STATUS
EFIAPI
MemDebugLogWrite (
  IN  CHAR8  *Buffer,
  IN  UINTN  Length
  )
{
  EFI_PHYSICAL_ADDRESS    MemDebugLogBufAddr;
  EFI_HOB_GUID_TYPE       *GuidHob;
  MEM_DEBUG_LOG_HOB_DATA  *HobData;
  EFI_STATUS              Status;

  GuidHob = GetFirstGuidHob (&gMemDebugLogHobGuid);
  if (GuidHob == NULL) {
    MemDebugLogBufAddr = 0;
  } else {
    HobData            = (MEM_DEBUG_LOG_HOB_DATA *)GET_GUID_HOB_DATA (GuidHob);
    MemDebugLogBufAddr = HobData->MemDebugLogBufAddr;
  }

  if (MemDebugLogBufAddr) {
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
