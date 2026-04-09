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
#include <Protocol/Rng.h>

#include "RngDxeInternals.h"

//
// Array containing the validated Rng algorithm.
// The entry with the lowest index will be the default algorithm.
//
UINTN              mAvailableAlgoArrayCount;
EFI_RNG_ALGORITHM  *mAvailableAlgoArray;

//
// The Random Number Generator (RNG) protocol
//
EFI_RNG_PROTOCOL  mRngRdRand = {
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  //
  // Get the list of available algorithm.
  //
  Status = GetAvailableAlgorithms ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mAvailableAlgoArrayCount == 0) {
    return EFI_REQUEST_UNLOAD_IMAGE;
  }

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
  if (EFI_ERROR (Status)) {
    FreeAvailableAlgorithms ();
  }

  return Status;
}

/**
  This is the unload handle for RndgDxe module.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
RngDriverUnLoad (
  IN EFI_HANDLE  ImageHandle
  )
{
  //
  // Free the list of available algorithm.
  //
  FreeAvailableAlgorithms ();
  return EFI_SUCCESS;
}

/**
  Runs CPU RNG instruction to fill a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

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
