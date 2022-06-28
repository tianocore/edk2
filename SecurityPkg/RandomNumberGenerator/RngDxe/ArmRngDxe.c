/** @file
  RNG Driver to produce the UEFI Random Number Generator protocol.

  The driver can use RNDR instruction (through the RngLib and if FEAT_RNG is
  present) to produce random numbers. It also uses the Arm FW-TRNG interface
  to implement EFI_RNG_ALGORITHM_RAW.

  RNG Algorithms defined in UEFI 2.4:
   - EFI_RNG_ALGORITHM_SP800_90_CTR_256_GUID
   - EFI_RNG_ALGORITHM_RAW
   - EFI_RNG_ALGORITHM_SP800_90_HMAC_256_GUID
   - EFI_RNG_ALGORITHM_SP800_90_HASH_256_GUID
   - EFI_RNG_ALGORITHM_X9_31_3DES_GUID        - Unsupported
   - EFI_RNG_ALGORITHM_X9_31_AES_GUID         - Unsupported

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DrbgLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>
#include <Library/DebugLib.h>
#include <Library/TrngLib.h>
#include <Protocol/Rng.h>

#include "RngDxeInternals.h"

//
// Static array containing the validated Rng algorithm.
// This array is used by RngGetInfo and RngGetRNG and needs to be
// populated only once.
// The valid entry with the lowest index will be the default algorithm.
//
#define RNG_AVAILABLE_ALGO_MAX  3
STATIC BOOLEAN            mAvailableAlgoArrayInit = FALSE;
STATIC UINTN              mAvailableAlgoArrayCount;
STATIC EFI_RNG_ALGORITHM  mAvailableAlgoArray[RNG_AVAILABLE_ALGO_MAX];

/** Initialize mAvailableAlgoArray with the available Rng algorithms.
**/
STATIC
VOID
EFIAPI
RngInitAvailableAlgoArray (
  VOID
  )
{
  UINT16  MajorRevision;
  UINT16  MinorRevision;

 #ifdef MDE_CPU_AARCH64
  // Check FEAT_RNG before advertising PcdCpuRngSupportedAlgorithm.
  if (ArmHasRngExt ()) {
    CopyMem (
      &mAvailableAlgoArray[mAvailableAlgoArrayCount],
      PcdGetPtr (PcdCpuRngSupportedAlgorithm),
      sizeof (EFI_RNG_ALGORITHM)
      );
    mAvailableAlgoArrayCount++;

    DEBUG_CODE_BEGIN ();
    if (IsZeroGuid (PcdGetPtr (PcdCpuRngSupportedAlgorithm))) {
      DEBUG ((
        DEBUG_WARN,
        "PcdCpuRngSupportedAlgorithm should be a non-zero GUID\n"
        ));
    }

    DEBUG_CODE_END ();
  }

 #endif

  // Raw algorithm (Trng)
  if (!EFI_ERROR (GetTrngVersion (&MajorRevision, &MinorRevision))) {
    CopyMem (
      &mAvailableAlgoArray[mAvailableAlgoArrayCount],
      &gEfiRngAlgorithmRaw,
      sizeof (EFI_RNG_ALGORITHM)
      );
    mAvailableAlgoArrayCount++;

    // SP800-90 Ctr 256 bits Drbg.
    // Arm implementation is based on the Trng.
    CopyMem (
      &mAvailableAlgoArray[mAvailableAlgoArrayCount],
      &gEfiRngAlgorithmSp80090Ctr256Guid,
      sizeof (EFI_RNG_ALGORITHM)
      );
    mAvailableAlgoArrayCount++;
  }

  mAvailableAlgoArrayInit = TRUE;
}

/** Produces and returns an RNG value using a specified Drbg algorithm.

  @param[in]  DrbgMechanism     The Drbg mechanism to use.
  @param[in]  RNGValueLength    The length in bytes of the memory buffer pointed to by
                                RNGValue. The driver shall return exactly this numbers of bytes.
  @param[out] RNGValue          A caller-allocated memory buffer filled by the driver with the
                                resulting RNG value.
**/
STATIC
EFI_STATUS
EFIAPI
RngGetDrbgVal (
  IN  DRBG_MECHANISM  DrbgMechanism,
  IN  UINTN           RNGValueLength,
  OUT UINT8           *RNGValue
  )
{
  EFI_STATUS   Status;
  STATIC VOID  *DrbgHandle = NULL;

  // Only instantiate once.
  if (DrbgHandle == NULL) {
    Status = DrbgInstantiateFn (
               DrbgMechanism,
               DrbgEntropyNoCondFn,
               256,
               FALSE,
               NULL,
               0,
               &DrbgHandle
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  // Check overflow.
  if (RNGValueLength > (MAX_UINTN >> 3)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = DrbgGenerateFn (
             256,
             FALSE,
             NULL,
             0,
             RNGValueLength << 3,
             RNGValue,
             DrbgHandle
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

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
  IN EFI_RNG_PROTOCOL   *This,
  IN EFI_RNG_ALGORITHM  *RNGAlgorithm  OPTIONAL,
  IN UINTN              RNGValueLength,
  OUT UINT8             *RNGValue
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  if ((This == NULL) || (RNGValueLength == 0) || (RNGValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mAvailableAlgoArrayInit) {
    RngInitAvailableAlgoArray ();
  }

  if (RNGAlgorithm == NULL) {
    //
    // Use the default RNG algorithm if RNGAlgorithm is NULL.
    //
    for (Index = 0; Index < RNG_AVAILABLE_ALGO_MAX; Index++) {
      if (!IsZeroGuid (&mAvailableAlgoArray[Index])) {
        RNGAlgorithm = &mAvailableAlgoArray[Index];
        goto FoundAlgo;
      }
    }

    if (Index == RNG_AVAILABLE_ALGO_MAX) {
      // No algorithm available.
      ASSERT (Index != RNG_AVAILABLE_ALGO_MAX);
      return EFI_DEVICE_ERROR;
    }
  }

FoundAlgo:
  if (CompareGuid (RNGAlgorithm, PcdGetPtr (PcdCpuRngSupportedAlgorithm))) {
    Status = RngGetBytes (RNGValueLength, RNGValue);
    return Status;
  }

  // Raw algorithm (Trng)
  if (CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmRaw)) {
    return GenerateEntropy (RNGValueLength, RNGValue);
  }

  // SP800-90 Ctr 256 bits Drbg
  if (CompareGuid (RNGAlgorithm, &gEfiRngAlgorithmSp80090Ctr256Guid)) {
    return RngGetDrbgVal (DrbgMechansimCtr, RNGValueLength, RNGValue);
  }

  //
  // Other algorithms are unsupported by this driver.
  //
  return EFI_UNSUPPORTED;
}

/**
  Returns information about the random number generation implementation.

  @param[in]     This                 A pointer to the EFI_RNG_PROTOCOL instance.
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
  @retval EFI_UNSUPPORTED             The services is not supported by this driver.
  @retval EFI_DEVICE_ERROR            The list of algorithms could not be retrieved due to a
                                      hardware or firmware error.
  @retval EFI_INVALID_PARAMETER       One or more of the parameters are incorrect.
  @retval EFI_BUFFER_TOO_SMALL        The buffer RNGAlgorithmList is too small to hold the result.

**/
EFI_STATUS
EFIAPI
RngGetInfo (
  IN EFI_RNG_PROTOCOL    *This,
  IN OUT UINTN           *RNGAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM  *RNGAlgorithmList
  )
{
  UINTN  RequiredSize;

  if ((This == NULL) || (RNGAlgorithmListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mAvailableAlgoArrayInit) {
    RngInitAvailableAlgoArray ();
  }

  RequiredSize = mAvailableAlgoArrayCount * sizeof (EFI_RNG_ALGORITHM);

  if (RequiredSize == 0) {
    // No supported algorithms found.
    return EFI_UNSUPPORTED;
  }

  if (*RNGAlgorithmListSize < RequiredSize) {
    *RNGAlgorithmListSize = RequiredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (RNGAlgorithmList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // There is no gap in the array, so copy the block.
  CopyMem (RNGAlgorithmList, mAvailableAlgoArray, RequiredSize);
  *RNGAlgorithmListSize = RequiredSize;
  return EFI_SUCCESS;
}
