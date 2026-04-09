/** @file
  Dispatch the block task to each AP in Smm mode for parallelhash algorithm.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CryptParallelHash.h"
#include <Library/MmServicesTableLib.h>

/**
  Dispatch the block task to each AP in SMM mode.

**/
VOID
EFIAPI
DispatchBlockToAp (
  VOID
  )
{
  UINTN  Index;

  if (gMmst == NULL) {
    return;
  }

  for (Index = 0; Index < gMmst->NumberOfCpus; Index++) {
    if (Index != gMmst->CurrentlyExecutingCpu) {
      gMmst->MmStartupThisAp (ParallelHashApExecute, Index, NULL);
    }
  }

  return;
}
