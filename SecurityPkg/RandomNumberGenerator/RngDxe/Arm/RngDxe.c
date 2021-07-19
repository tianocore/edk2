/** @file
  RNG Driver to produce the UEFI Random Number Generator protocol.

  The driver implements the EFI_RNG_ALGORITHM_RAW using the FW-TRNG
  interface to provide entropy.

  RNG Algorithms defined in UEFI 2.4:
   - EFI_RNG_ALGORITHM_SP800_90_CTR_256_GUID
   - EFI_RNG_ALGORITHM_RAW
   - EFI_RNG_ALGORITHM_SP800_90_HMAC_256_GUID
   - EFI_RNG_ALGORITHM_SP800_90_HASH_256_GUID
   - EFI_RNG_ALGORITHM_X9_31_3DES_GUID        - Unsupported
   - EFI_RNG_ALGORITHM_X9_31_AES_GUID         - Unsupported

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TrngLib.h>
#include <Protocol/Rng.h>

#include "RngDxeInternals.h"

/**
  Produces and returns an RNG value using either the default or specified
  RNG algorithm.

  @param[in]  This                  A pointer to the EFI_RNG_PROTOCOL instance.
  @param[in]  RNGAlgorithm          A pointer to the EFI_RNG_ALGORITHM that
                                    identifies the RNG algorithm to use. May be
                                    NULL in which case the function will use its
                                    default RNG algorithm.
  @param[in]  RNGValueLength        The length in bytes of the memory buffer
                                    pointed to by RNGValue. The driver shall
                                    return exactly this numbers of bytes.
  @param[out] RNGValue              A caller-allocated memory buffer filled by
                                    the driver with the resulting RNG value.

  @retval EFI_SUCCESS               The RNG value was returned successfully.
  @retval EFI_UNSUPPORTED           The algorithm specified by RNGAlgorithm is
                                    not supported by this driver.
  @retval EFI_DEVICE_ERROR          An RNG value could not be retrieved due to
                                    a hardware or firmware error.
  @retval EFI_NOT_READY             There is not enough random data available
                                    to satisfy the length requested by
                                    RNGValueLength.
  @retval EFI_INVALID_PARAMETER     RNGValue is NULL or RNGValueLength is zero.

**/
EFI_STATUS
EFIAPI
RngGetRNG (
  IN EFI_RNG_PROTOCOL            *This,
  IN EFI_RNG_ALGORITHM           *RNGAlgorithm, OPTIONAL
  IN UINTN                       RNGValueLength,
  OUT UINT8                      *RNGValue
  )
{
  EFI_STATUS  Status;
  UINT16      MajorRevision;
  UINT16      MinorRevision;

  if ((RNGValueLength == 0) || (RNGValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RNGAlgorithm == NULL) {
    //
    // Use the default RNG algorithm if RNGAlgorithm is NULL.
    //
    RNGAlgorithm = &gEfiRngAlgorithmRaw;
  }

  //
  // The "raw" algorithm is intended to provide entropy directly
  //
  if (CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmRaw)) {
    Status = GetTrngVersion (&MajorRevision, &MinorRevision);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
    return GenerateEntropy (RNGValueLength, RNGValue);
  }

  //
  // Other algorithms are unsupported by this driver.
  //
  return EFI_UNSUPPORTED;
}

/**
  Returns information about the random number generation implementation.

  @param[in,out] RNGAlgorithmListSize On input, the size in bytes of
                                      RNGAlgorithmList.
                                      On output with a return code of
                                      EFI_SUCCESS, the size in bytes of the
                                      data returned in RNGAlgorithmList.
                                      On output with a return code of
                                      EFI_BUFFER_TOO_SMALL, the size of
                                      RNGAlgorithmList required to obtain the
                                      list.
  @param[out] RNGAlgorithmList        A caller-allocated memory buffer filled
                                      by the driver with one EFI_RNG_ALGORITHM
                                      element for each supported RNG algorithm.
                                      The list must not change across multiple
                                      calls to the same driver. The first
                                      algorithm in the list is the default
                                      algorithm for the driver.

  @retval EFI_SUCCESS                 The RNG algorithm list was returned
                                      successfully.
  @retval EFI_UNSUPPORTED             No supported algorithms found.
  @retval EFI_BUFFER_TOO_SMALL        The buffer RNGAlgorithmList is too small
                                      to hold the result.
  @retval EFI_INVALID_PARAMETER       The pointer to the buffer RNGAlgorithmList
                                      is invalid.
**/
UINTN
EFIAPI
ArchGetSupportedRngAlgorithms (
  IN OUT UINTN                     *RNGAlgorithmListSize,
  OUT    EFI_RNG_ALGORITHM         *RNGAlgorithmList
  )
{
  EFI_STATUS  Status;
  UINTN       RequiredSize;
  UINT16      MajorRevision;
  UINT16      MinorRevision;

  RequiredSize = 0;

  Status = GetTrngVersion (&MajorRevision, &MinorRevision);
  if (EFI_ERROR (Status)) {
    // No supported algorithms found.
    return EFI_UNSUPPORTED;
  }

  RequiredSize += sizeof (EFI_RNG_ALGORITHM);

  if (*RNGAlgorithmListSize < RequiredSize) {
    *RNGAlgorithmListSize = RequiredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (RNGAlgorithmList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (
    &RNGAlgorithmList[0],
    &gEfiRngAlgorithmRaw,
    sizeof (EFI_RNG_ALGORITHM)
    );

  *RNGAlgorithmListSize = RequiredSize;
  return EFI_SUCCESS;
}
