/** @file
  Arm FW-TRNG interface helper common for AArch32 and AArch64.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TrngLib.h>

/**
  Generate high-quality entropy source using a TRNG.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval EFI_SUCCESS        Entropy generation succeeded.
  @retval EFI_NOT_READY      Failed to request random data.

**/
EFI_STATUS
EFIAPI
GenerateEntropy (
  IN  UINTN        Length,
  OUT UINT8        *Entropy
  )
{
  EFI_STATUS  Status;
  UINTN       CollectedEntropyBits;
  UINTN       RequiredEntropyBits;
  UINTN       EntropyBits;
  UINTN       Index;
  UINTN       MaxBits;

  ZeroMem (Entropy, Length);

  RequiredEntropyBits = (Length << 3);
  Index = 0;
  CollectedEntropyBits = 0;
  MaxBits = GetTrngMaxSupportedEntropyBits ();
  while (CollectedEntropyBits < RequiredEntropyBits) {
    EntropyBits = MIN ((RequiredEntropyBits - CollectedEntropyBits), MaxBits);
    Status = GetEntropy (
               EntropyBits,
               &Entropy[Index],
               (Length - Index)
               );
    if (EFI_ERROR (Status)) {
      // Discard the collected bits.
      ZeroMem (Entropy, Length);
      return Status;
    }
    CollectedEntropyBits += EntropyBits;
    Index += (EntropyBits >> 3);
  } // while

  return Status;
}
