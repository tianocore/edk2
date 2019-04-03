/** @file
  FMP Authentication RSA2048SHA256 handler.
  Provide generic FMP authentication functions for DXE/PEI post memory phase.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  FmpAuthenticatedHandlerRsa2048Sha256(), AuthenticateFmpImage() will receive
  untrusted input and do basic validation.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/SystemResourceTable.h>
#include <Guid/FirmwareContentsSigned.h>
#include <Guid/WinCertificate.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/FmpAuthenticationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Guid/SystemResourceTable.h>

///
/// Public Exponent of RSA Key.
///
STATIC CONST UINT8 mRsaE[] = { 0x01, 0x00, 0x01 };

/**
  The handler is used to do the authentication for FMP capsule based upon
  EFI_FIRMWARE_IMAGE_AUTHENTICATION.

  Caution: This function may receive untrusted input.

  This function assumes the caller AuthenticateFmpImage()
  already did basic validation for EFI_FIRMWARE_IMAGE_AUTHENTICATION.

  @param[in]  Image                   Points to an FMP authentication image, started from EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               Size of the authentication image in bytes.
  @param[in]  PublicKeyData           The public key data used to validate the signature.
  @param[in]  PublicKeyDataLength     The length of the public key data.

  @retval RETURN_SUCCESS            Authentication pass.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_SUCCESS.
  @retval RETURN_SECURITY_VIOLATION Authentication fail.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR.
  @retval RETURN_INVALID_PARAMETER  The image is in an invalid format.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_OUT_OF_RESOURCES   No Authentication handler associated with CertType.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES.
**/
RETURN_STATUS
FmpAuthenticatedHandlerRsa2048Sha256 (
  IN EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN UINTN                              ImageSize,
  IN CONST UINT8                        *PublicKeyData,
  IN UINTN                              PublicKeyDataLength
  )
{
  RETURN_STATUS                             Status;
  EFI_CERT_BLOCK_RSA_2048_SHA256            *CertBlockRsa2048Sha256;
  BOOLEAN                                   CryptoStatus;
  UINT8                                     Digest[SHA256_DIGEST_SIZE];
  UINT8                                     *PublicKey;
  UINTN                                     PublicKeyBufferSize;
  VOID                                      *HashContext;
  VOID                                      *Rsa;

  DEBUG ((DEBUG_INFO, "FmpAuthenticatedHandlerRsa2048Sha256 - Image: 0x%08x - 0x%08x\n", (UINTN)Image, (UINTN)ImageSize));

  if (Image->AuthInfo.Hdr.dwLength != OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData) + sizeof(EFI_CERT_BLOCK_RSA_2048_SHA256)) {
    DEBUG((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256 - dwLength: 0x%04x, dwLength - 0x%04x\n", (UINTN)Image->AuthInfo.Hdr.dwLength, (UINTN)OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData) + sizeof(EFI_CERT_BLOCK_RSA_2048_SHA256)));
    return RETURN_INVALID_PARAMETER;
  }

  CertBlockRsa2048Sha256 = (EFI_CERT_BLOCK_RSA_2048_SHA256 *)Image->AuthInfo.CertData;
  if (!CompareGuid(&CertBlockRsa2048Sha256->HashType, &gEfiHashAlgorithmSha256Guid)) {
    DEBUG((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256 - HashType: %g, expect - %g\n", &CertBlockRsa2048Sha256->HashType, &gEfiHashAlgorithmSha256Guid));
    return RETURN_INVALID_PARAMETER;
  }

  HashContext = NULL;
  Rsa = NULL;

  //
  // Allocate hash context buffer required for SHA 256
  //
  HashContext = AllocatePool (Sha256GetContextSize ());
  if (HashContext == NULL) {
    CryptoStatus = FALSE;
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Can not allocate hash context\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Hash public key from data payload with SHA256.
  //
  ZeroMem (Digest, SHA256_DIGEST_SIZE);
  CryptoStatus = Sha256Init (HashContext);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Init() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }
  CryptoStatus = Sha256Update (HashContext, &CertBlockRsa2048Sha256->PublicKey, sizeof(CertBlockRsa2048Sha256->PublicKey));
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Update() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }
  CryptoStatus  = Sha256Final (HashContext, Digest);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Final() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Fail if the PublicKey is not one of the public keys in the input PublicKeyData.
  //
  PublicKey = (VOID *)PublicKeyData;
  PublicKeyBufferSize = PublicKeyDataLength;
  CryptoStatus = FALSE;
  while (PublicKeyBufferSize != 0) {
    if (CompareMem (Digest, PublicKey, SHA256_DIGEST_SIZE) == 0) {
      CryptoStatus = TRUE;
      break;
    }
    PublicKey = PublicKey + SHA256_DIGEST_SIZE;
    PublicKeyBufferSize = PublicKeyBufferSize - SHA256_DIGEST_SIZE;
  }
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Public key in section is not supported\n"));
    Status = RETURN_SECURITY_VIOLATION;
    goto Done;
  }

  //
  // Generate & Initialize RSA Context.
  //
  Rsa = RsaNew ();
  if (Rsa == NULL) {
    CryptoStatus = FALSE;
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: RsaNew() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Set RSA Key Components.
  // NOTE: Only N and E are needed to be set as RSA public key for signature verification.
  //
  CryptoStatus = RsaSetKey (Rsa, RsaKeyN, CertBlockRsa2048Sha256->PublicKey, sizeof(CertBlockRsa2048Sha256->PublicKey));
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: RsaSetKey(RsaKeyN) failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }
  CryptoStatus = RsaSetKey (Rsa, RsaKeyE, mRsaE, sizeof (mRsaE));
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: RsaSetKey(RsaKeyE) failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Hash data payload with SHA256.
  //
  ZeroMem (Digest, SHA256_DIGEST_SIZE);
  CryptoStatus = Sha256Init (HashContext);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Init() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  // It is a signature across the variable data and the Monotonic Count value.
  CryptoStatus = Sha256Update (
                   HashContext,
                   (UINT8 *)Image + sizeof(Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength,
                   ImageSize - sizeof(Image->MonotonicCount) - Image->AuthInfo.Hdr.dwLength
                   );
  if (!CryptoStatus) {
    DEBUG((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Update() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }
  CryptoStatus = Sha256Update (
                   HashContext,
                   (UINT8 *)&Image->MonotonicCount,
                   sizeof(Image->MonotonicCount)
                   );
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Update() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }
  CryptoStatus  = Sha256Final (HashContext, Digest);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: Sha256Final() failed\n"));
    Status = RETURN_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Verify the RSA 2048 SHA 256 signature.
  //
  CryptoStatus = RsaPkcs1Verify (
                   Rsa,
                   Digest,
                   SHA256_DIGEST_SIZE,
                   CertBlockRsa2048Sha256->Signature,
                   sizeof (CertBlockRsa2048Sha256->Signature)
                   );
  if (!CryptoStatus) {
    //
    // If RSA 2048 SHA 256 signature verification fails, AUTH tested failed bit is set.
    //
    DEBUG ((DEBUG_ERROR, "FmpAuthenticatedHandlerRsa2048Sha256: RsaPkcs1Verify() failed\n"));
    Status = RETURN_SECURITY_VIOLATION;
    goto Done;
  }
  DEBUG ((DEBUG_INFO, "FmpAuthenticatedHandlerRsa2048Sha256: PASS verification\n"));

  Status = RETURN_SUCCESS;

Done:
  //
  // Free allocated resources used to perform RSA 2048 SHA 256 signature verification
  //
  if (Rsa != NULL) {
    RsaFree (Rsa);
  }
  if (HashContext != NULL) {
    FreePool (HashContext);
  }

  return Status;
}

/**
  The function is used to do the authentication for FMP capsule based upon
  EFI_FIRMWARE_IMAGE_AUTHENTICATION.

  The FMP capsule image should start with EFI_FIRMWARE_IMAGE_AUTHENTICATION,
  followed by the payload.

  If the return status is RETURN_SUCCESS, the caller may continue the rest
  FMP update process.
  If the return status is NOT RETURN_SUCCESS, the caller should stop the FMP
  update process and convert the return status to LastAttemptStatus
  to indicate that FMP update fails.
  The LastAttemptStatus can be got from ESRT table or via
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL.GetImageInfo().

  Caution: This function may receive untrusted input.

  @param[in]  Image                   Points to an FMP authentication image, started from EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               Size of the authentication image in bytes.
  @param[in]  PublicKeyData           The public key data used to validate the signature.
  @param[in]  PublicKeyDataLength     The length of the public key data.

  @retval RETURN_SUCCESS            Authentication pass.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_SUCCESS.
  @retval RETURN_SECURITY_VIOLATION Authentication fail.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR.
  @retval RETURN_INVALID_PARAMETER  The image is in an invalid format.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_UNSUPPORTED        No Authentication handler associated with CertType.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_UNSUPPORTED        Image or ImageSize is invalid.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_OUT_OF_RESOURCES   No Authentication handler associated with CertType.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES.
**/
RETURN_STATUS
EFIAPI
AuthenticateFmpImage (
  IN EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN UINTN                              ImageSize,
  IN CONST UINT8                        *PublicKeyData,
  IN UINTN                              PublicKeyDataLength
  )
{
  GUID                                      *CertType;
  EFI_STATUS                                Status;

  if ((Image == NULL) || (ImageSize == 0)) {
    return RETURN_UNSUPPORTED;
  }

  if ((PublicKeyDataLength % SHA256_DIGEST_SIZE) != 0) {
    DEBUG ((DEBUG_ERROR, "PublicKeyDataLength is not multiple SHA256 size\n"));
    return RETURN_UNSUPPORTED;
  }

  if (ImageSize < sizeof(EFI_FIRMWARE_IMAGE_AUTHENTICATION)) {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage - ImageSize too small\n"));
    return RETURN_INVALID_PARAMETER;
  }
  if (Image->AuthInfo.Hdr.dwLength <= OFFSET_OF(WIN_CERTIFICATE_UEFI_GUID, CertData)) {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage - dwLength too small\n"));
    return RETURN_INVALID_PARAMETER;
  }
  if ((UINTN) Image->AuthInfo.Hdr.dwLength > MAX_UINTN - sizeof(UINT64)) {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage - dwLength too big\n"));
    return RETURN_INVALID_PARAMETER;
  }
  if (ImageSize <= sizeof(Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage - ImageSize too small\n"));
    return RETURN_INVALID_PARAMETER;
  }
  if (Image->AuthInfo.Hdr.wRevision != 0x0200) {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage - wRevision: 0x%02x, expect - 0x%02x\n", (UINTN)Image->AuthInfo.Hdr.wRevision, (UINTN)0x0200));
    return RETURN_INVALID_PARAMETER;
  }
  if (Image->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID) {
    DEBUG((DEBUG_ERROR, "AuthenticateFmpImage - wCertificateType: 0x%02x, expect - 0x%02x\n", (UINTN)Image->AuthInfo.Hdr.wCertificateType, (UINTN)WIN_CERT_TYPE_EFI_GUID));
    return RETURN_INVALID_PARAMETER;
  }

  CertType = &Image->AuthInfo.CertType;
  DEBUG((DEBUG_INFO, "AuthenticateFmpImage - CertType: %g\n", CertType));

  if (CompareGuid (&gEfiCertTypeRsa2048Sha256Guid, CertType)) {
    //
    // Call the match handler to extract raw data for the input section data.
    //
    Status = FmpAuthenticatedHandlerRsa2048Sha256 (
               Image,
               ImageSize,
               PublicKeyData,
               PublicKeyDataLength
               );
    return Status;
  }

  //
  // Not found, the input guided section is not supported.
  //
  return RETURN_UNSUPPORTED;
}

