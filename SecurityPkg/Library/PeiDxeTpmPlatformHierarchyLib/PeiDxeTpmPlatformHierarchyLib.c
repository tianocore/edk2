/** @file
    TPM Platform Hierarchy configuration library.

    This library provides functions for customizing the TPM's Platform Hierarchy
    Authorization Value (platformAuth) and Platform Hierarchy Authorization
    Policy (platformPolicy) can be defined through this function.

    Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
    Copyright (c) Microsoft Corporation.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

    @par Specification Reference:
    https://trustedcomputinggroup.org/resource/tcg-tpm-v2-0-provisioning-guidance/
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RngLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>

//
// The authorization value may be no larger than the digest produced by the hash
//   algorithm used for context integrity.
//

UINT16       mAuthSize;

/**
  Generate high-quality entropy source through RDRAND.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval EFI_SUCCESS        Entropy generation succeeded.
  @retval EFI_NOT_READY      Failed to request random data.

**/
EFI_STATUS
EFIAPI
RdRandGenerateEntropy (
  IN UINTN         Length,
  OUT UINT8        *Entropy
  )
{
  EFI_STATUS  Status;
  UINTN       BlockCount;
  UINT64      Seed[2];
  UINT8       *Ptr;

  Status = EFI_NOT_READY;
  BlockCount = Length / sizeof(Seed);
  Ptr = (UINT8 *)Entropy;

  //
  // Generate high-quality seed for DRBG Entropy
  //
  while (BlockCount > 0) {
    Status = GetRandomNumber128 (Seed);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    CopyMem (Ptr, Seed, sizeof(Seed));

    BlockCount--;
    Ptr = Ptr + sizeof(Seed);
  }

  //
  // Populate the remained data as request.
  //
  Status = GetRandomNumber128 (Seed);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  CopyMem (Ptr, Seed, (Length % sizeof(Seed)));

  return Status;
}

/**
  This function returns the maximum size of TPM2B_AUTH; this structure is used for an authorization value
  and limits an authValue to being no larger than the largest digest produced by a TPM.

  @param[out] AuthSize                 Tpm2 Auth size

  @retval EFI_SUCCESS                  Auth size returned.
  @retval EFI_DEVICE_ERROR             Can not return platform auth due to device error.

**/
EFI_STATUS
EFIAPI
GetAuthSize (
  OUT UINT16            *AuthSize
  )
{
  EFI_STATUS            Status;
  TPML_PCR_SELECTION    Pcrs;
  UINTN                 Index;
  UINT16                DigestSize;

  Status = EFI_SUCCESS;

  while (mAuthSize == 0) {

    mAuthSize = SHA1_DIGEST_SIZE;
    ZeroMem (&Pcrs, sizeof (TPML_PCR_SELECTION));
    Status = Tpm2GetCapabilityPcrs (&Pcrs);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Tpm2GetCapabilityPcrs fail!\n"));
      break;
    }

    DEBUG ((DEBUG_ERROR, "Tpm2GetCapabilityPcrs - %08x\n", Pcrs.count));

    for (Index = 0; Index < Pcrs.count; Index++) {
      DEBUG ((DEBUG_ERROR, "alg - %x\n", Pcrs.pcrSelections[Index].hash));

      switch (Pcrs.pcrSelections[Index].hash) {
      case TPM_ALG_SHA1:
        DigestSize = SHA1_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA256:
        DigestSize = SHA256_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA384:
        DigestSize = SHA384_DIGEST_SIZE;
        break;
      case TPM_ALG_SHA512:
        DigestSize = SHA512_DIGEST_SIZE;
        break;
      case TPM_ALG_SM3_256:
        DigestSize = SM3_256_DIGEST_SIZE;
        break;
      default:
        DigestSize = SHA1_DIGEST_SIZE;
        break;
      }

      if (DigestSize > mAuthSize) {
        mAuthSize = DigestSize;
      }
    }
    break;
  }

  *AuthSize = mAuthSize;
  return Status;
}

/**
  Set PlatformAuth to random value.
**/
VOID
RandomizePlatformAuth (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINT16                            AuthSize;
  TPM2B_AUTH                        NewPlatformAuth;

  //
  // Send Tpm2HierarchyChange Auth with random value to avoid PlatformAuth being null
  //

  GetAuthSize (&AuthSize);

  NewPlatformAuth.size = AuthSize;

  //
  // Create the random bytes in the destination buffer
  //

  RdRandGenerateEntropy (NewPlatformAuth.size, NewPlatformAuth.buffer);

  //
  // Send Tpm2HierarchyChangeAuth command with the new Auth value
  //
  Status = Tpm2HierarchyChangeAuth (TPM_RH_PLATFORM, NULL, &NewPlatformAuth);
  DEBUG ((DEBUG_INFO, "Tpm2HierarchyChangeAuth Result: - %r\n", Status));
  ZeroMem (NewPlatformAuth.buffer, AuthSize);
}

/**
  Disable the TPM platform hierarchy.

  @retval   EFI_SUCCESS       The TPM was disabled successfully.
  @retval   Others            An error occurred attempting to disable the TPM platform hierarchy.

**/
EFI_STATUS
DisableTpmPlatformHierarchy (
  VOID
  )
{
  EFI_STATUS  Status;

  // Make sure that we have use of the TPM.
  Status = Tpm2RequestUseTpm ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%a() - Tpm2RequestUseTpm Failed! %r\n", gEfiCallerBaseName, __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Let's do what we can to shut down the hierarchies.

  // Disable the PH NV.
  // IMPORTANT NOTE: We *should* be able to disable the PH NV here, but TPM parts have
  //                 been known to store the EK cert in the PH NV. If we disable it, the
  //                 EK cert will be unreadable.

  // Disable the PH.
  Status =  Tpm2HierarchyControl (
              TPM_RH_PLATFORM,     // AuthHandle
              NULL,                // AuthSession
              TPM_RH_PLATFORM,     // Hierarchy
              NO                   // State
              );
  DEBUG ((DEBUG_VERBOSE, "%a:%a() -  Disable PH = %r\n", gEfiCallerBaseName, __FUNCTION__, Status));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%a() -  Disable PH Failed! %r\n", gEfiCallerBaseName, __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
   This service defines the configuration of the Platform Hierarchy Authorization Value (platformAuth)
   and Platform Hierarchy Authorization Policy (platformPolicy).

**/
VOID
EFIAPI
ConfigureTpmPlatformHierarchy (
  )
{
  if (PcdGetBool (PcdRandomizePlatformHierarchy)) {
    //
    // Send Tpm2HierarchyChange Auth with random value to avoid PlatformAuth being null
    //
    RandomizePlatformAuth ();
  } else {
    //
    // Disable the hierarchy entirely (do not randomize it)
    //
    DisableTpmPlatformHierarchy ();
  }
}
