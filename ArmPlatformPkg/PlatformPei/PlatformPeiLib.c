/** @file

  Copyright (c) 2011-2024, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - https://github.com/FirmwareHandoff/firmware_handoff

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
