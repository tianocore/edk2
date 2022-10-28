/** @file
  Arm specific code.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmTrngLib.h>

#include "RngDxeInternals.h"

// Maximum number of Rng algorithms.
#define RNG_AVAILABLE_ALGO_MAX  1

/** Allocate and initialize mAvailableAlgoArray with the available
    Rng algorithms. Also update mAvailableAlgoArrayCount.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
GetAvailableAlgorithms (
  VOID
  )
{
  UINT16  MajorRevision;
  UINT16  MinorRevision;

  // Rng algorithms 2 times, one for the allocation, one to populate.
  mAvailableAlgoArray = AllocateZeroPool (RNG_AVAILABLE_ALGO_MAX);
  if (mAvailableAlgoArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Raw algorithm (Trng)
  if (!EFI_ERROR (GetArmTrngVersion (&MajorRevision, &MinorRevision))) {
    CopyMem (
      &mAvailableAlgoArray[mAvailableAlgoArrayCount],
      &gEfiRngAlgorithmRaw,
      sizeof (EFI_RNG_ALGORITHM)
      );
    mAvailableAlgoArrayCount++;
  }

  return EFI_SUCCESS;
}
