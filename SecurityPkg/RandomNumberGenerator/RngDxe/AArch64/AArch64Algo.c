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
#include <Guid/RngAlgorithm.h>

#include "RngDxeInternals.h"

// Maximum number of Rng algorithms.
#define RNG_AVAILABLE_ALGO_MAX  2

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
  BOOLEAN     UnSafeAlgo;

  UnSafeAlgo = FALSE;

  // Rng algorithms 2 times, one for the allocation, one to populate.
  mAvailableAlgoArray = AllocateZeroPool (RNG_AVAILABLE_ALGO_MAX * sizeof (EFI_RNG_ALGORITHM));
  if (mAvailableAlgoArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Identify RngLib algorithm.
  Status = GetRngGuid (&RngGuid);
  if (!EFI_ERROR (Status)) {
    if (IsZeroGuid (&RngGuid) ||
        CompareGuid (&RngGuid, &gEdkiiRngAlgorithmUnSafe))
    {
      // Treat zero GUID as an unsafe algorithm
      DEBUG ((
        DEBUG_WARN,
        "RngLib uses an Unsafe algorithm and "
        "must not be used for production builds.\n"
        ));
      // Set the UnSafeAlgo flag to indicate an unsafe algorithm was found
      // so that it can be added at the end of the algorithm list.
      UnSafeAlgo = TRUE;
    } else {
      CopyMem (
        &mAvailableAlgoArray[mAvailableAlgoArrayCount],
        &RngGuid,
        sizeof (RngGuid)
        );
      mAvailableAlgoArrayCount++;
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

  // Add unsafe algorithm at the end of the list.
  if (UnSafeAlgo) {
    CopyMem (
      &mAvailableAlgoArray[mAvailableAlgoArrayCount],
      &gEdkiiRngAlgorithmUnSafe,
      sizeof (EFI_RNG_ALGORITHM)
      );
    mAvailableAlgoArrayCount++;
  }

  return EFI_SUCCESS;
}
