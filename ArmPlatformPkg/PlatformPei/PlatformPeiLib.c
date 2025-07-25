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

/**
  Get transfer list header.

  @param[out] TransferList  Transfer list header

  @retval EFI_SUCCESS      Transfer list is found.
  @retval EFI_NOT_FOUND    Transfer list is not found.

**/
STATIC
EFI_STATUS
EFIAPI
GetTransferList (
  OUT TRANSFER_LIST_HEADER  **TransferList
  )
{
  VOID               *HobList;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINTN              *GuidHobData;

  *TransferList = NULL;

  HobList = GetHobList ();
  if (HobList == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHob = GetNextGuidHob (&gArmTransferListHobGuid, HobList);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  GuidHobData = GET_GUID_HOB_DATA (GuidHob);

  *TransferList = (TRANSFER_LIST_HEADER *)(*GuidHobData);

  return EFI_SUCCESS;
}

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

  Status = GetTransferList (&TransferList);
  if (!EFI_ERROR (Status)) {
    if (TransferListCheckHeader (TransferList) == TRANSFER_LIST_OPS_INVALID) {
      DEBUG ((DEBUG_ERROR, "%a: Invalid transfer list... Stop boot...\n", __func__));
      return EFI_INVALID_PARAMETER;
    }
  } else {
    return EFI_SUCCESS;
  }

  Status = BuildPeiEventLogHobs (TransferList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to build event log Pei Hobs...: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}
