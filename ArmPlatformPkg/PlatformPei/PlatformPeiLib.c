/** @file

  Copyright (c) 2011-2025, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
  @Par Reference(s):
  - Firmware Handoff specification [https://firmwarehandoff.github.io/firmware_handoff/main]
**/

#include <PiPei.h>

#include <Library/ArmPlatformLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Guid/TransferListHob.h>

UINT64  mTransferListBaseAddr = 0;

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  )
{
  VOID    *TransferListBase;
  UINT64  *TransferListHobData;

  TransferListBase = (VOID *)(UINTN)mTransferListBaseAddr;
  if (TransferListBase != NULL) {
    if (TransferListCheckHeader (TransferListBase) != TRANSFER_LIST_OPS_INVALID) {
      DEBUG_CODE_BEGIN ();
      TransferListDump (TransferListBase);
      DEBUG_CODE_END ();

      TransferListHobData = BuildGuidHob (&gArmTransferListHobGuid, sizeof (*TransferListHobData));
      if (TransferListHobData == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: TransferList BuildGuidHob failed\n", __func__));
        ASSERT (0);
        return EFI_OUT_OF_RESOURCES;
      }

      *TransferListHobData = (UINTN)TransferListBase;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: No valid operations possible on TransferList found @ 0x%p\n", __func__, TransferListBase));
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a: No TransferList found, continuing boot\n", __func__));
  }

  BuildFvHob (PcdGet64 (PcdFvBaseAddress), PcdGet32 (PcdFvSize));

  return EFI_SUCCESS;
}
