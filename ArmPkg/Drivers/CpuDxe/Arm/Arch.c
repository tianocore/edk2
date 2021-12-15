/** @file
  Architecture specific functions.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CpuDxe.h>

/** Initializes multi-processor support.
 *
**/
VOID
ArchInitializeMpSupport (
  VOID
  )
{
  /* Nothing to do - ARM doesn't support EFI_MP_SERVICES_PROTOCOL */
}
