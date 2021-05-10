/** @file
  RNG Driver to produce the UEFI Random Number Generator protocol.

  The driver will use the new RDRAND instruction to produce high-quality, high-performance
  entropy and random number.

  RNG Algorithms defined in UEFI 2.4:
   - EFI_RNG_ALGORITHM_SP800_90_CTR_256_GUID  - Supported
     (RDRAND implements a hardware NIST SP800-90 AES-CTR-256 based DRBG)
   - EFI_RNG_ALGORITHM_RAW                    - Supported
     (Structuring RDRAND invocation can be guaranteed as high-quality entropy source)
   - EFI_RNG_ALGORITHM_SP800_90_HMAC_256_GUID - Unsupported
   - EFI_RNG_ALGORITHM_SP800_90_HASH_256_GUID - Unsupported
   - EFI_RNG_ALGORITHM_X9_31_3DES_GUID        - Unsupported
   - EFI_RNG_ALGORITHM_X9_31_AES_GUID         - Unsupported

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RdRand.h"
#include "RngDxeInternals.h"

/**
  Produces and returns an RNG value using either the default or specified RNG algorithm.

  @param[in]  This                    A pointer to the EFI_RNG_PROTOCOL instance.
  @param[in]  RNGAlgorithm            A pointer to the EFI_RNG_ALGORITHM that identifies the RNG
                                      algorithm to use. May be NULL in which case the function will
                                      use its default RNG algorithm.
  @param[in]  RNGValueLength          The length in bytes of the memory buffer pointed to by
                                      RNGValue. The driver shall return exactly this numbers of bytes.
  @param[out] RNGValue                A caller-allocated memory buffer filled by the driver with the
                                      resulting RNG value.

  @retval EFI_SUCCESS                 The RNG value was returned successfully.
  @retval EFI_UNSUPPORTED             The algorithm specified by RNGAlgorithm is not supported by
                                      this driver.
  @retval EFI_DEVICE_ERROR            An RNG value could not be retrieved due to a hardware or
                                      firmware error.
  @retval EFI_NOT_READY               There is not enough random data available to satisfy the length
                                      requested by RNGValueLength.
  @retval EFI_INVALID_PARAMETER       RNGValue is NULL or RNGValueLength is zero.

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
  EFI_STATUS    Status;

  if ((RNGValueLength == 0) || (RNGValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_UNSUPPORTED;
  if (RNGAlgorithm == NULL) {
    //
    // Use the default RNG algorithm if RNGAlgorithm is NULL.
    //
    RNGAlgorithm = &gEfiRngAlgorithmSp80090Ctr256Guid;
  }

  //
  // NIST SP800-90-AES-CTR-256 supported by RDRAND
  //
  if (CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmSp80090Ctr256Guid)) {
    Status = RngGetBytes (RNGValueLength, RNGValue);
    return Status;
  }

  //
  // The "raw" algorithm is intended to provide entropy directly
  //
  if (CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmRaw)) {
    //
    // When a DRBG is used on the output of a entropy source,
    // its security level must be at least 256 bits according to UEFI Spec.
    //
    if (RNGValueLength < 32) {
      return EFI_INVALID_PARAMETER;
    }

    Status = RdRandGenerateEntropy (RNGValueLength, RNGValue);
    return Status;
  }

  //
  // Other algorithms were unsupported by this driver.
  //
  return Status;
}

/**
  Returns information about the random number generation implementation.

  @param[in,out] RNGAlgorithmListSize On input, the size in bytes of RNGAlgorithmList.
                                      On output with a return code of EFI_SUCCESS, the size
                                      in bytes of the data returned in RNGAlgorithmList. On output
                                      with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of RNGAlgorithmList required to obtain the list.
  @param[out] RNGAlgorithmList        A caller-allocated memory buffer filled by the driver
                                      with one EFI_RNG_ALGORITHM element for each supported
                                      RNG algorithm. The list must not change across multiple
                                      calls to the same driver. The first algorithm in the list
                                      is the default algorithm for the driver.

  @retval EFI_SUCCESS                 The RNG algorithm list was returned successfully.
  @retval EFI_BUFFER_TOO_SMALL        The buffer RNGAlgorithmList is too small to hold the result.

**/
UINTN
EFIAPI
ArchGetSupportedRngAlgorithms (
  IN OUT UINTN                     *RNGAlgorithmListSize,
  OUT    EFI_RNG_ALGORITHM         *RNGAlgorithmList
  )
{
  UINTN RequiredSize;
  EFI_RNG_ALGORITHM *CpuRngSupportedAlgorithm;

  RequiredSize = 2 * sizeof (EFI_RNG_ALGORITHM);

  if (*RNGAlgorithmListSize < RequiredSize) {
    *RNGAlgorithmListSize = RequiredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  CpuRngSupportedAlgorithm = PcdGetPtr (PcdCpuRngSupportedAlgorithm);

  CopyMem(&RNGAlgorithmList[0], CpuRngSupportedAlgorithm, sizeof (EFI_RNG_ALGORITHM));

  // x86 platforms also support EFI_RNG_ALGORITHM_RAW via RDSEED
  CopyMem(&RNGAlgorithmList[1], &gEfiRngAlgorithmRaw, sizeof (EFI_RNG_ALGORITHM));

  *RNGAlgorithmListSize = RequiredSize;
  return EFI_SUCCESS;
}
