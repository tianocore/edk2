/** @file
 Provides an implementation of the library class RngLib that uses the Rng protocol.

 Copyright (c) Microsoft Corporation. All rights reserved.
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/RngLib.h>
#include <Protocol/Rng.h>

/**
  Routine Description:

  Generates a random number via the NIST
  800-9A algorithm.  Refer to
  http://csrc.nist.gov/groups/STM/cavp/documents/drbg/DRBGVS.pdf
  for more information.

  @param[out] Buffer      Buffer to receive the random number.
  @param[in]  BufferSize  Number of bytes in Buffer.

  @retval EFI_SUCCESS or underlying failure code.
**/
STATIC
EFI_STATUS
GenerateRandomNumberViaNist800Algorithm (
  OUT UINT8  *Buffer,
  IN  UINTN  BufferSize
  )
{
  EFI_STATUS        Status;
  EFI_RNG_PROTOCOL  *RngProtocol;

  RngProtocol = NULL;

  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Buffer == NULL.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (&gEfiRngProtocolGuid, NULL, (VOID **)&RngProtocol);
  if (EFI_ERROR (Status) || (RngProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Could not locate RNG prototocol, Status = %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = RngProtocol->GetRNG (RngProtocol, &gEfiRngAlgorithmSp80090Ctr256Guid, BufferSize, Buffer);
  DEBUG ((DEBUG_INFO, "%a: GetRNG algorithm CTR-256 - Status = %r\n", __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = RngProtocol->GetRNG (RngProtocol, &gEfiRngAlgorithmSp80090Hmac256Guid, BufferSize, Buffer);
  DEBUG ((DEBUG_INFO, "%a: GetRNG algorithm HMAC-256 - Status = %r\n", __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  Status = RngProtocol->GetRNG (RngProtocol, &gEfiRngAlgorithmSp80090Hash256Guid, BufferSize, Buffer);
  DEBUG ((DEBUG_INFO, "%a: GetRNG algorithm Hash-256 - Status = %r\n", __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  // If all the other methods have failed, use the default method from the RngProtocol
  Status = RngProtocol->GetRNG (RngProtocol, NULL, BufferSize, Buffer);
  DEBUG ((DEBUG_INFO, "%a: GetRNG algorithm Hash-256 - Status = %r\n", __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
    return Status;
  }

  // If we get to this point, we have failed
  DEBUG ((DEBUG_ERROR, "%a: GetRNG() failed, staus = %r\n", __FUNCTION__, Status));

  return Status;
}// GenerateRandomNumberViaNist800Algorithm()

/**
  Generates a 16-bit random number.

  if Rand is NULL, return FALSE.

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber16 (
  OUT UINT16  *Rand
  )
{
  EFI_STATUS  Status;

  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumberViaNist800Algorithm ((UINT8 *)Rand, sizeof (UINT16));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates a 32-bit random number.

  if Rand is NULL, return FALSE.

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber32 (
  OUT UINT32  *Rand
  )
{
  EFI_STATUS  Status;

  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumberViaNist800Algorithm ((UINT8 *)Rand, sizeof (UINT32));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates a 64-bit random number.

  if Rand is NULL, return FALSE.

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber64 (
  OUT UINT64  *Rand
  )
{
  EFI_STATUS  Status;

  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumberViaNist800Algorithm ((UINT8 *)Rand, sizeof (UINT64));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Generates a 128-bit random number.

  if Rand is NULL, return FALSE.

  @param[out] Rand     Buffer pointer to store the 128-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
GetRandomNumber128 (
  OUT UINT64  *Rand
  )
{
  EFI_STATUS  Status;

  if (Rand == NULL) {
    return FALSE;
  }

  Status = GenerateRandomNumberViaNist800Algorithm ((UINT8 *)Rand, 2 * sizeof (UINT64));
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}
