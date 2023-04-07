/** @file
  Aarch64 specific code.

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmTrngLib.h>
#include <Library/RngLib.h>

#include "RngDxeInternals.h"

// Maximum number of Rng algorithms.
#define RNG_AVAILABLE_ALGO_MAX  2

/** mAvailableAlgoArray[0] should contain the default Rng algorithm.
    The Rng algorithm at the first index might be unsafe.
    If a safe algorithm is available, choose it as the default one.
**/
VOID
EFIAPI
RngFindDefaultAlgo (
  VOID
  )
{
  EFI_RNG_ALGORITHM  *CurAlgo;
  EFI_RNG_ALGORITHM  TmpGuid;
  UINTN              Index;

  CurAlgo = &mAvailableAlgoArray[0];

  if (IsZeroGuid (CurAlgo) ||
      !CompareGuid (CurAlgo, &gEfiRngAlgorithmUnSafe))
  {
    // mAvailableAlgoArray[0] is a valid Rng algorithm.
    return;
  }

  for (Index = 1; Index < mAvailableAlgoArrayCount; Index++) {
    CurAlgo = &mAvailableAlgoArray[Index];
    if (!IsZeroGuid (CurAlgo) ||
        CompareGuid (CurAlgo, &gEfiRngAlgorithmUnSafe))
    {
      break;
    }
  }

  if (Index == mAvailableAlgoArrayCount) {
    // No valid Rng algorithm available.
    return;
  }

  CopyMem (&TmpGuid, CurAlgo, sizeof (EFI_RNG_ALGORITHM));
  CopyMem (CurAlgo, &mAvailableAlgoArray[0], sizeof (EFI_RNG_ALGORITHM));
  CopyMem (&mAvailableAlgoArray[0], &TmpGuid, sizeof (EFI_RNG_ALGORITHM));

  return;
}

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
  EFI_STATUS  Status;
  UINT16      MajorRevision;
  UINT16      MinorRevision;
  GUID        RngGuid;

  // Rng algorithms 2 times, one for the allocation, one to populate.
  mAvailableAlgoArray = AllocateZeroPool (RNG_AVAILABLE_ALGO_MAX);
  if (mAvailableAlgoArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Identify RngLib algorithm.
  Status = GetRngGuid (&RngGuid);
  if (!EFI_ERROR (Status)) {
    CopyMem (
      &mAvailableAlgoArray[mAvailableAlgoArrayCount],
      &RngGuid,
      sizeof (RngGuid)
      );
    mAvailableAlgoArrayCount++;

    if (IsZeroGuid (&RngGuid)) {
      DEBUG ((
        DEBUG_WARN,
        "RngLib should have a non-zero GUID\n"
        ));
    }
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

  RngFindDefaultAlgo ();

  return EFI_SUCCESS;
}
