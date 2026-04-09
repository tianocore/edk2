/** @file
  ARM implementation of architecture specific routines related to
  PersistAcrossReset capsules

  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleService.h"

/**
  Whether the platform supports capsules that persist across reset. Note that
  some platforms only support such capsules at boot time.

  @return TRUE  if a PersistAcrossReset capsule may be passed to UpdateCapsule()
                at this time
          FALSE otherwise
**/
BOOLEAN
IsPersistAcrossResetCapsuleSupported (
  VOID
  )
{
  //
  // ARM requires the capsule payload to be cleaned to the point of coherency
  // (PoC), but only permits doing so using cache maintenance instructions that
  // operate on virtual addresses. Since at runtime, we don't know the virtual
  // addresses of the data structures that make up the scatter/gather list, we
  // cannot perform the maintenance, and all we can do is give up.
  //
  return FeaturePcdGet (PcdSupportUpdateCapsuleReset) && !EfiAtRuntime ();
}
