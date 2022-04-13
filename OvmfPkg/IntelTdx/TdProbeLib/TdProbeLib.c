/** @file

  TdProbeLib is used to probe the Td guest.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/TdProbeLib.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>

/**
  Probe if it is Tdx guest.

  @return TD_PROBE_TDX if it is Tdx guest. Otherwise return TD_PROBE_NON.

**/
UINTN
EFIAPI
TdProbe (
  VOID
  )
{
  OVMF_WORK_AREA  *WorkArea;

  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);

  //
  // Check if it is TDX guest.
  //
  if ((WorkArea != NULL) && (WorkArea->Header.GuestType == GUEST_TYPE_INTEL_TDX)) {
    return TD_PROBE_TDX;
  } else {
    return TD_PROBE_NON;
  }
}
