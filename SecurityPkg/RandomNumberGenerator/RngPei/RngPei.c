/** @file
  RNG Driver to produce the Random Number Generator PPI.

  The driver uses a platform provided RNG service to produce random numbers.

  Copyright (c) Microsoft Corporation.
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/RngLib.h>
#include <Ppi/Rng.h>

/**
  Returns information about the random number generation implementation.

  @param[in]     This                 A pointer to the RNG_PPI instance.
  @param[in,out] RngAlgorithmListSize On input, the size in bytes of RngAlgorithmList.
                                      On output with a return code of EFI_SUCCESS, the size
                                      in bytes of the data returned in RngAlgorithmList. On output
                                      with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of RngAlgorithmList required to obtain the list.
  @param[out] RngAlgorithmList        A caller-allocated memory buffer filled by the driver
                                      with one EFI_RNG_ALGORITHM element for each supported
                                      RNG algorithm. The list must not change across multiple
                                      calls to the same driver. The first algorithm in the list
                                      is the default algorithm for the driver.

  @retval EFI_SUCCESS                 The RNG algorithm list was returned successfully.
  @retval EFI_UNSUPPORTED             The services is not supported by this driver.
  @retval EFI_DEVICE_ERROR            The list of algorithms could not be retrieved due to a
                                      hardware or firmware error.
  @retval EFI_INVALID_PARAMETER       One or more of the parameters are incorrect.
  @retval EFI_BUFFER_TOO_SMALL        The buffer RngAlgorithmList is too small to hold the result.

**/
EFI_STATUS
EFIAPI
RngGetInfo (
  IN RNG_PPI             *This,
  IN OUT UINTN           *RngAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM  *RngAlgorithmList
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (RngAlgorithmListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Only a single algorithm is supported at this time. The algorithm provided by the
  // RngLib instance used with this PEIM.
  if (*RngAlgorithmListSize < sizeof (EFI_GUID)) {
    *RngAlgorithmListSize = sizeof (EFI_GUID);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (RngAlgorithmList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Return algorithm list supported by driver.
  //
  Status = GetRngGuid (RngAlgorithmList);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "[%a] - RNG algorithm successfully retrieved {%g}.\n", __func__, RngAlgorithmList));
    *RngAlgorithmListSize = sizeof (EFI_GUID);
  }

  return Status;
}

/**
  Fills a buffer of arbitrary size with random bytes.

  @param[in]   Length           Size of the buffer, in bytes.
  @param[out]  RandBuffer       Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS           Random bytes generation succeeded.
  @retval EFI_INVALID_PARAMETER The RandBuffer argument is null.
  @retval EFI_NOT_READY         Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RngGetBytes (
  IN UINTN   Length,
  OUT UINT8  *RandBuffer
  )
{
  BOOLEAN  IsRandom;
  UINT64   TempRand[2];

  if (RandBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  while (Length > 0) {
    IsRandom = GetRandomNumber128 (TempRand);
    if (!IsRandom) {
      return EFI_NOT_READY;
    }

    if (Length >= sizeof (TempRand)) {
      WriteUnaligned64 ((UINT64 *)RandBuffer, TempRand[0]);
      RandBuffer += sizeof (UINT64);
      WriteUnaligned64 ((UINT64 *)RandBuffer, TempRand[1]);
      RandBuffer += sizeof (UINT64);
      Length     -= sizeof (TempRand);
    } else {
      CopyMem (RandBuffer, TempRand, Length);
      Length = 0;
    }
  }

  return EFI_SUCCESS;
}

/**
  Produces and returns an RNG value using either the default or specified RNG algorithm.

  @param[in]  This                    A pointer to the RNG_PPI instance.
  @param[in]  RngAlgorithm            A pointer to the EFI_RNG_ALGORITHM that identifies the RNG
                                      algorithm to use. May be NULL in which case the function will
                                      use its default RNG algorithm.
  @param[in]  RngValueLength          The length in bytes of the memory buffer pointed to by
                                      RngValue. The driver shall return exactly this numbers of bytes.
  @param[out] RngValue                A caller-allocated memory buffer filled by the driver with the
                                      resulting RNG value.

  @retval EFI_SUCCESS                 The RNG value was returned successfully.
  @retval EFI_UNSUPPORTED             The algorithm specified by RngAlgorithm is not supported by
                                      this driver.
  @retval EFI_DEVICE_ERROR            An RNG value could not be retrieved due to a hardware or
                                      firmware error.
  @retval EFI_NOT_READY               There is not enough random data available to satisfy the length
                                      requested by RngValueLength.
  @retval EFI_INVALID_PARAMETER       RngValue is NULL or RngValueLength is zero.

**/
EFI_STATUS
EFIAPI
RngGetRNG (
  IN RNG_PPI            *This,
  IN EFI_RNG_ALGORITHM  *RngAlgorithm  OPTIONAL,
  IN UINTN              RngValueLength,
  OUT UINT8             *RngValue
  )
{
  EFI_STATUS         Status;
  UINTN              AlgorithmSize;
  EFI_RNG_ALGORITHM  SupportedAlgorithm;

  if ((RngValueLength == 0) || (RngValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RngAlgorithm != NULL) {
    AlgorithmSize = sizeof (SupportedAlgorithm);
    Status        = RngGetInfo (This, &AlgorithmSize, &SupportedAlgorithm);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  if ((RngAlgorithm == NULL) || CompareGuid (RngAlgorithm, &SupportedAlgorithm)) {
    Status = RngGetBytes (RngValueLength, RngValue);
    return Status;
  }

  //
  // Other algorithms are unsupported by this driver.
  //
  return EFI_UNSUPPORTED;
}

//
// The Random Number Generator (RNG) PPI
//
RNG_PPI  mRngPpi = {
  RngGetInfo,
  RngGetRNG
};

EFI_PEI_PPI_DESCRIPTOR  mRngPpiDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiRngPpiGuid,
  &mRngPpi
};

/**
  Entry point.

  @param[in]  FileHandle   Handle of the file being invoked.
  @param[in]  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS      The RNG PPI was successfully installed.
  @retval Others           Returned from PeiServicesInstallPpi().

**/
EFI_STATUS
EFIAPI
RngPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  return PeiServicesInstallPpi (&mRngPpiDesc);
}
