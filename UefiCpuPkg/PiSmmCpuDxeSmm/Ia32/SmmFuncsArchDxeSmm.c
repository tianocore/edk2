/** @file

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  Return whether access to non-SMRAM is restricted.

  @retval TRUE  Access to non-SMRAM is restricted.
  @retval FALSE Access to non-SMRAM is not restricted.
**/
BOOLEAN
IsRestrictedMemoryAccess (
  VOID
  )
{
  return TRUE;
}
