/** @file

  CcProbeLib is used to probe the Confidential computing guest type.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CcProbeLib.h>
#include <WorkArea.h>

STATIC UINT8    mCcProbeGuestType = 0;
STATIC BOOLEAN  mCcProbed         = FALSE;

/**
  Probe the ConfidentialComputing Guest type. See defition of
  CC_GUEST_TYPE in <ConfidentialComputingGuestAttr.h>.

  @return The guest type

**/
UINT8
EFIAPI
CcProbe (
  VOID
  )
{
  OVMF_WORK_AREA  *WorkArea;

  if (!mCcProbed) {
    WorkArea          = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);
    mCcProbeGuestType = WorkArea != NULL ? WorkArea->Header.GuestType : CcGuestTypeNonEncrypted;
    mCcProbed         = TRUE;
  }

  return mCcProbeGuestType;
}
