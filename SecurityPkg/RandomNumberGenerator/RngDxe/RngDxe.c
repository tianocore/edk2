/** @file
  RNG Driver to produce the UEFI Random Number Generator protocol.

  The driver uses CPU RNG instructions to produce high-quality,
  high-performance entropy and random number.

  RNG Algorithms defined in UEFI 2.4:
   - EFI_RNG_ALGORITHM_SP800_90_CTR_256_GUID
   - EFI_RNG_ALGORITHM_RAW
   - EFI_RNG_ALGORITHM_SP800_90_HMAC_256_GUID
   - EFI_RNG_ALGORITHM_SP800_90_HASH_256_GUID
   - EFI_RNG_ALGORITHM_X9_31_3DES_GUID
   - EFI_RNG_ALGORITHM_X9_31_AES_GUID

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RngLib.h>
#include <Library/TimerLib.h>
#include <Protocol/Rng.h>

#include "RngDxeInternals.h"

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
  IN EFI_RNG_PROTOCOL             *This,
  IN OUT UINTN                    *RNGAlgorithmListSize,
  OUT EFI_RNG_ALGORITHM           *RNGAlgorithmList
  )
{
  EFI_STATUS    Status;

  if ((This == NULL) || (RNGAlgorithmListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Return algorithm list supported by driver.
  //
  if (RNGAlgorithmList != NULL) {
    Status = ArchGetSupportedRngAlgorithms (RNGAlgorithmListSize, RNGAlgorithmList);
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

//
// The Random Number Generator (RNG) protocol
//
EFI_RNG_PROTOCOL mRngRdRand = {
  RngGetInfo,
  RngGetRNG
};

/**
  The user Entry Point for the Random Number Generator (RNG) driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval EFI_NOT_SUPPORTED Platform does not support RNG.
  @retval Other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
RngDriverEntry (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;

  //
  // Install UEFI RNG (Random Number Generator) Protocol
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiRngProtocolGuid,
                  &mRngRdRand,
                  NULL
                  );

  return Status;
}


/**
  Calls RDRAND to fill a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RngGetBytes (
  IN UINTN         Length,
  OUT UINT8        *RandBuffer
  )
{
  BOOLEAN     IsRandom;
  UINT64      TempRand[2];

  while (Length > 0) {
    IsRandom = GetRandomNumber128 (TempRand);
    if (!IsRandom) {
      return EFI_NOT_READY;
    }
    if (Length >= sizeof (TempRand)) {
      WriteUnaligned64 ((UINT64*)RandBuffer, TempRand[0]);
      RandBuffer += sizeof (UINT64);
      WriteUnaligned64 ((UINT64*)RandBuffer, TempRand[1]);
      RandBuffer += sizeof (UINT64);
      Length -= sizeof (TempRand);
    } else {
      CopyMem (RandBuffer, TempRand, Length);
      Length = 0;
    }
  }

  return EFI_SUCCESS;
}
