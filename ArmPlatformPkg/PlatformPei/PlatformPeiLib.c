/** @file

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/TransferListHob.h>

#include <Library/ArmPlatformLib.h>
#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

/**
  Build event log hob consumed by Tcg2Dxe.

  @param[in]  TransferList  Transfer list header

  @return EFI_SUCCESS       Success to generate HOBs from passed event log.
  @return Others            Failed to create event log hob

**/
EFI_STATUS
EFIAPI
BuildPeiEventLogHobs (
  IN TRANSFER_LIST_HEADER  *TransferList
  );

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  )
{
  EFI_STATUS            Status;
  TRANSFER_LIST_HEADER  *TransferList;

  BuildFvHob (
    PcdGet64 (PcdFvBaseAddress),
    PcdGet32 (PcdFvSize)
    );

  Status = TransferListGetFromHobList (&TransferList);
  if (!EFI_ERROR (Status)) {
    if (TransferListCheckHeader (TransferList) == TRANSFER_LIST_OPS_INVALID) {
      DEBUG ((DEBUG_ERROR, "%a: Invalid transfer list... Stop boot...\n", __func__));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    // Transfer list is not found, this means
    // early event log is not available and
    // measured boot may not be enabled.
    return EFI_SUCCESS;
  }

  // Create HOBs for the early measurement Event logs.
  Status = BuildPeiEventLogHobs (TransferList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to build event log Pei Hobs...: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}
