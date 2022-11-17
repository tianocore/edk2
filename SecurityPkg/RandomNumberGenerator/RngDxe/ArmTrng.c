/** @file
  RNG Driver to produce the UEFI Random Number Generator protocol.

  The driver implements the EFI_RNG_ALGORITHM_RAW using the FW-TRNG
  interface to provide entropy.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmTrngLib.h>
#include <Protocol/Rng.h>

#include "RngDxeInternals.h"

/**
  Generate high-quality entropy source using a TRNG or through RDRAND.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval  RETURN_SUCCESS            The function completed successfully.
  @retval  RETURN_INVALID_PARAMETER  Invalid parameter.
  @retval  RETURN_UNSUPPORTED        Function not implemented.
  @retval  RETURN_BAD_BUFFER_SIZE    Buffer size is too small.
  @retval  RETURN_NOT_READY          No Entropy available.
**/
EFI_STATUS
EFIAPI
GenerateEntropy (
  IN  UINTN  Length,
  OUT UINT8  *Entropy
  )
{
  EFI_STATUS  Status;
  UINTN       CollectedEntropyBits;
  UINTN       RequiredEntropyBits;
  UINTN       EntropyBits;
  UINTN       Index;
  UINTN       MaxBits;

  ZeroMem (Entropy, Length);

  RequiredEntropyBits  = (Length << 3);
  Index                = 0;
  CollectedEntropyBits = 0;
  MaxBits              = GetArmTrngMaxSupportedEntropyBits ();
  while (CollectedEntropyBits < RequiredEntropyBits) {
    EntropyBits = MIN ((RequiredEntropyBits - CollectedEntropyBits), MaxBits);
    Status      = GetArmTrngEntropy (
                    EntropyBits,
                    (Length - Index),
                    &Entropy[Index]
                    );
    if (EFI_ERROR (Status)) {
      // Discard the collected bits.
      ZeroMem (Entropy, Length);
      return Status;
    }

    CollectedEntropyBits += EntropyBits;
    Index                += (EntropyBits >> 3);
  } // while

  return Status;
}
