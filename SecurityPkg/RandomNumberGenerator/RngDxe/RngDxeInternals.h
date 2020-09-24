/** @file
  Function prototypes for UEFI Random Number Generator protocol support.

  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RNGDXE_INTERNALS_H_
#define RNGDXE_INTERNALS_H_

extern EFI_RNG_ALGORITHM *mSUpportedRngAlgorithms;

EFI_STATUS
EFIAPI
RngGetInfo (
  IN EFI_RNG_PROTOCOL             *This,
  IN OUT UINTN                    *RNGAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM           *RNGAlgorithmList
  );


EFI_STATUS
EFIAPI
RngGetRNG (
  IN EFI_RNG_PROTOCOL            *This,
  IN EFI_RNG_ALGORITHM           *RNGAlgorithm, OPTIONAL
  IN UINTN                       RNGValueLength,
  OUT UINT8                      *RNGValue
  );

UINTN
EFIAPI
ArchGetSupportedRngAlgorithmsSize (
  VOID
  );

#endif  // RNGDXE_INTERNALS_H_
