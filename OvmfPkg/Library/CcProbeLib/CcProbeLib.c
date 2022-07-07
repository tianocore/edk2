/** @file

  CcProbeLib is used to probe the Confidential computing guest type.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/CcProbeLib.h>

STATIC UINT8    CcProbeGuestType = 0;
STATIC BOOLEAN  CcProbed         = FALSE;

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
  if (CcProbed) {
    return CcProbeGuestType;
  }

  if (TdIsEnabled ()) {
    CcProbeGuestType = CcGuestTypeIntelTdx;
  } else {
    //
    // SEV code should be added here to determine if it is CcGuestTypeAmdSev.
    // Now we set CcProbeGuestType to CcGuestTypeNonEncrypted.
    //
    CcProbeGuestType = CcGuestTypeNonEncrypted;
  }

  CcProbed = TRUE;

  return CcProbeGuestType;
}
