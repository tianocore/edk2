/** @file

  Copyright (c) 2011-2024, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/ArmPlatformLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/TransferListHob.h>
#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  )
{
  VOID    *TransferListBase;
  UINT64  *TransferListHobData;

  TransferListBase = (VOID *)(UINTN)PcdGet64 (PcdTransferListBaseAddress);
  if ((TransferListBase != NULL) && (TransferListCheckHeader (TransferListBase) != TRANSFER_LIST_OPS_INVALID)) {
    TransferListDump (TransferListBase);
    TransferListHobData = BuildGuidHob (&gTransferListHobGuid, sizeof (*TransferListHobData));
    if (TransferListHobData == NULL) {
      ASSERT (0);
      return EFI_OUT_OF_RESOURCES;
    }

    *TransferListHobData = (UINTN)TransferListBase;
  } else {
    DEBUG ((DEBUG_ERROR, "%a: No Transfer List found @ 0x%p\n", __func__, TransferListBase));
  }

  BuildFvHob (PcdGet64 (PcdFvBaseAddress), PcdGet32 (PcdFvSize));

  return EFI_SUCCESS;
}
