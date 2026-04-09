/** @file

  This library registers RSA 2048 SHA 256 guided section handler
  to parse RSA 2048 SHA 256 encapsulation section and extract raw data.
  It uses the BaseCryptLib based on OpenSSL to authenticate the signature.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Protocol/Hash.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Guid/WinCertificate.h>
#include <Library/BaseCryptLib.h>
#include <Library/PerformanceLib.h>
#include <Guid/SecurityPkgTokenSpace.h>

///
/// RSA 2048 SHA 256 Guided Section header
///
typedef struct {
  EFI_GUID_DEFINED_SECTION          GuidedSectionHeader;    ///< EFI guided section header
  EFI_CERT_BLOCK_RSA_2048_SHA256    CertBlockRsa2048Sha256; ///< RSA 2048-bit Signature
} RSA_2048_SHA_256_SECTION_HEADER;

typedef struct {
  EFI_GUID_DEFINED_SECTION2         GuidedSectionHeader;    ///< EFI guided section header
  EFI_CERT_BLOCK_RSA_2048_SHA256    CertBlockRsa2048Sha256; ///< RSA 2048-bit Signature
} RSA_2048_SHA_256_SECTION2_HEADER;

///
/// Public Exponent of RSA Key.
///
CONST UINT8  mRsaE[] = { 0x01, 0x00, 0x01 };

/**

  GetInfo gets raw data size and attribute of the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param InputSection       Buffer containing the input GUIDed section to be processed.
  @param OutputBufferSize   The size of OutputBuffer.
  @param ScratchBufferSize  The size of ScratchBuffer.
  @param SectionAttribute   The attribute of the input guided section.

  @retval EFI_SUCCESS            The size of destination buffer, the size of scratch buffer and
                                 the attribute of the input section are successfully retrieved.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Rsa2048Sha256GuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
{
  if (IS_SECTION2 (InputSection)) {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gEfiCertTypeRsa2048Sha256Guid,
           &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Retrieve the size and attribute of the input section data.
    //
    *SectionAttribute  = ((EFI_GUID_DEFINED_SECTION2 *)InputSection)->Attributes;
    *ScratchBufferSize = 0;
    *OutputBufferSize  = SECTION2_SIZE (InputSection) - sizeof (RSA_2048_SHA_256_SECTION2_HEADER);
  } else {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gEfiCertTypeRsa2048Sha256Guid,
           &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Retrieve the size and attribute of the input section data.
    //
    *SectionAttribute  = ((EFI_GUID_DEFINED_SECTION *)InputSection)->Attributes;
    *ScratchBufferSize = 0;
    *OutputBufferSize  = SECTION_SIZE (InputSection) - sizeof (RSA_2048_SHA_256_SECTION_HEADER);
  }

  return EFI_SUCCESS;
}

/**

  Extraction handler tries to extract raw data from the input guided section.
  It also does authentication check for RSA 2048 SHA 256 signature in the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param InputSection    Buffer containing the input GUIDed section to be processed.
  @param OutputBuffer    Buffer to contain the output raw data allocated by the caller.
  @param ScratchBuffer   A pointer to a caller-allocated buffer for function internal use.
  @param AuthenticationStatus A pointer to a caller-allocated UINT32 that indicates the
                              authentication status of the output buffer.

  @retval EFI_SUCCESS            Section Data and Auth Status is extracted successfully.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Rsa2048Sha256GuidedSectionHandler (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  )
{
  EFI_STATUS                      Status;
  UINT32                          OutputBufferSize;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlockRsa2048Sha256;
  BOOLEAN                         CryptoStatus;
  UINT8                           Digest[SHA256_DIGEST_SIZE];
  UINT8                           *PublicKey;
  UINTN                           PublicKeyBufferSize;
  VOID                            *HashContext;
  VOID                            *Rsa;

  HashContext = NULL;
  Rsa         = NULL;

  if (IS_SECTION2 (InputSection)) {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gEfiCertTypeRsa2048Sha256Guid,
           &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Get the RSA 2048 SHA 256 information.
    //
    CertBlockRsa2048Sha256 = &((RSA_2048_SHA_256_SECTION2_HEADER *)InputSection)->CertBlockRsa2048Sha256;
    OutputBufferSize       = SECTION2_SIZE (InputSection) - sizeof (RSA_2048_SHA_256_SECTION2_HEADER);
    if ((((EFI_GUID_DEFINED_SECTION *)InputSection)->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) != 0) {
      PERF_INMODULE_BEGIN ("PeiRsaCopy");
      CopyMem (*OutputBuffer, (UINT8 *)InputSection + sizeof (RSA_2048_SHA_256_SECTION2_HEADER), OutputBufferSize);
      PERF_INMODULE_END ("PeiRsaCopy");
    } else {
      *OutputBuffer = (UINT8 *)InputSection + sizeof (RSA_2048_SHA_256_SECTION2_HEADER);
    }

    //
    // Implicitly RSA 2048 SHA 256 GUIDed section should have STATUS_VALID bit set
    //
    ASSERT ((((EFI_GUID_DEFINED_SECTION2 *)InputSection)->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) != 0);
    *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;
  } else {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gEfiCertTypeRsa2048Sha256Guid,
           &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Get the RSA 2048 SHA 256 information.
    //
    CertBlockRsa2048Sha256 = &((RSA_2048_SHA_256_SECTION_HEADER *)InputSection)->CertBlockRsa2048Sha256;
    OutputBufferSize       = SECTION_SIZE (InputSection) - sizeof (RSA_2048_SHA_256_SECTION_HEADER);
    if ((((EFI_GUID_DEFINED_SECTION *)InputSection)->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) != 0) {
      PERF_INMODULE_BEGIN ("PeiRsaCopy");
      CopyMem (*OutputBuffer, (UINT8 *)InputSection + sizeof (RSA_2048_SHA_256_SECTION_HEADER), OutputBufferSize);
      PERF_INMODULE_END ("PeiRsaCopy");
    } else {
      *OutputBuffer = (UINT8 *)InputSection + sizeof (RSA_2048_SHA_256_SECTION_HEADER);
    }

    //
    // Implicitly RSA 2048 SHA 256 GUIDed section should have STATUS_VALID bit set
    //
    ASSERT ((((EFI_GUID_DEFINED_SECTION *)InputSection)->Attributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) != 0);
    *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;
  }

  //
  // All paths from here return EFI_SUCCESS and result is returned in AuthenticationStatus
  //
  Status = EFI_SUCCESS;

  //
  // Fail if the HashType is not SHA 256
  //
  if (!CompareGuid (&gEfiHashAlgorithmSha256Guid, &CertBlockRsa2048Sha256->HashType)) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: HASH type of section is not supported\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Allocate hash context buffer required for SHA 256
  //
  HashContext = AllocatePool (Sha256GetContextSize ());
  if (HashContext == NULL) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Can not allocate hash context\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Hash public key from data payload with SHA256.
  //
  ZeroMem (Digest, SHA256_DIGEST_SIZE);
  CryptoStatus = Sha256Init (HashContext);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Sha256Init() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  CryptoStatus = Sha256Update (HashContext, &CertBlockRsa2048Sha256->PublicKey, sizeof (CertBlockRsa2048Sha256->PublicKey));
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Sha256Update() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  CryptoStatus = Sha256Final (HashContext, Digest);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Sha256Final() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Fail if the PublicKey is not one of the public keys in PcdRsa2048Sha256PublicKeyBuffer
  //
  PublicKey = (UINT8 *)PcdGetPtr (PcdRsa2048Sha256PublicKeyBuffer);
  DEBUG ((DEBUG_VERBOSE, "PeiPcdRsa2048Sha256: PublicKeyBuffer = %p\n", PublicKey));
  ASSERT (PublicKey != NULL);
  DEBUG ((DEBUG_VERBOSE, "PeiPcdRsa2048Sha256: PublicKeyBuffer Token = %08x\n", PcdToken (PcdRsa2048Sha256PublicKeyBuffer)));
  PublicKeyBufferSize = PcdGetSize (PcdRsa2048Sha256PublicKeyBuffer);
  DEBUG ((DEBUG_VERBOSE, "PeiPcdRsa2048Sha256: PublicKeyBuffer Size = %08x\n", PublicKeyBufferSize));
  ASSERT ((PublicKeyBufferSize % SHA256_DIGEST_SIZE) == 0);
  CryptoStatus = FALSE;
  while (PublicKeyBufferSize != 0) {
    if (CompareMem (Digest, PublicKey, SHA256_DIGEST_SIZE) == 0) {
      CryptoStatus = TRUE;
      break;
    }

    PublicKey           = PublicKey + SHA256_DIGEST_SIZE;
    PublicKeyBufferSize = PublicKeyBufferSize - SHA256_DIGEST_SIZE;
  }

  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Public key in section is not supported\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Generate & Initialize RSA Context.
  //
  Rsa = RsaNew ();
  if (Rsa == NULL) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: RsaNew() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Set RSA Key Components.
  // NOTE: Only N and E are needed to be set as RSA public key for signature verification.
  //
  CryptoStatus = RsaSetKey (Rsa, RsaKeyN, CertBlockRsa2048Sha256->PublicKey, sizeof (CertBlockRsa2048Sha256->PublicKey));
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: RsaSetKey(RsaKeyN) failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  CryptoStatus = RsaSetKey (Rsa, RsaKeyE, mRsaE, sizeof (mRsaE));
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: RsaSetKey(RsaKeyE) failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Hash data payload with SHA256.
  //
  ZeroMem (Digest, SHA256_DIGEST_SIZE);
  CryptoStatus = Sha256Init (HashContext);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Sha256Init() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  PERF_INMODULE_BEGIN ("PeiRsaShaData");
  CryptoStatus = Sha256Update (HashContext, *OutputBuffer, OutputBufferSize);
  PERF_INMODULE_END ("PeiRsaShaData");
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Sha256Update() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  CryptoStatus = Sha256Final (HashContext, Digest);
  if (!CryptoStatus) {
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: Sha256Final() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
    goto Done;
  }

  //
  // Verify the RSA 2048 SHA 256 signature.
  //
  PERF_INMODULE_BEGIN ("PeiRsaVerify");
  CryptoStatus = RsaPkcs1Verify (
                   Rsa,
                   Digest,
                   SHA256_DIGEST_SIZE,
                   CertBlockRsa2048Sha256->Signature,
                   sizeof (CertBlockRsa2048Sha256->Signature)
                   );
  PERF_INMODULE_END ("PeiRsaVerify");
  if (!CryptoStatus) {
    //
    // If RSA 2048 SHA 256 signature verification fails, AUTH tested failed bit is set.
    //
    DEBUG ((DEBUG_ERROR, "PeiRsa2048Sha256: RsaPkcs1Verify() failed\n"));
    *AuthenticationStatus |= EFI_AUTH_STATUS_TEST_FAILED;
  }

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

  DEBUG ((DEBUG_VERBOSE, "PeiRsa2048Sha256: Status = %r  AuthenticationStatus = %08x\n", Status, *AuthenticationStatus));

  return Status;
}

/**
  Register the handler to extract RSA 2048 SHA 256 guided section.

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval  EFI_SUCCESS           Register successfully.
  @retval  EFI_OUT_OF_RESOURCES  Not enough memory to register this handler.

**/
EFI_STATUS
EFIAPI
PeiRsa2048Sha256GuidedSectionExtractLibConstructor (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return ExtractGuidedSectionRegisterHandlers (
           &gEfiCertTypeRsa2048Sha256Guid,
           Rsa2048Sha256GuidedSectionGetInfo,
           Rsa2048Sha256GuidedSectionHandler
           );
}
